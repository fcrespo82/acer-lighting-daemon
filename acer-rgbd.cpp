// acer-rgbd.cpp
// Daemon que salva 3 estados (keyboard/lid/button) e reaplica no boot.
// Protocolo (uma linha por conexão):
//   SET dev=keyboard hidraw=/dev/hidraw2 effect=static bright=100 r=255 g=255 b=0 zone=all dir=none
//   SET dev=lid      hidraw=/dev/hidraw2 effect=breathing bright=60 speed=5
//   SET dev=button   hidraw=/dev/hidraw2 effect=neon bright=80 speed=7
//   GET
//
// Estado persistido (3 linhas) em: /var/lib/acer-rgbd/state.txt
// Socket: /run/acer-rgbd.sock

#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define RGB_FEATURE_ID 0xa4

#define KEYBOARD_RGB_ID 0x21
#define LID_RGB_ID      0x83
#define BUTTON_RGB_ID   0x65

#define RGB_EFFECT_STATIC    0x02
#define RGB_EFFECT_BREATHING 0x04
#define RGB_EFFECT_NEON      0x05
#define RGB_EFFECT_WAVE      0x07
#define RGB_EFFECT_RIPPLE    0x08
#define RGB_EFFECT_ZOOM      0x09
#define RGB_EFFECT_SNAKE     0x0a
#define RGB_EFFECT_DISCO     0x0b
#define RGB_EFFECT_SHIFTING  0xff

static constexpr const char* SOCK_PATH  = "/run/acer-rgbd.sock";
static constexpr const char* STATE_DIR  = "/var/lib/acer-rgbd";
static constexpr const char* STATE_PATH = "/var/lib/acer-rgbd/state.txt";

struct Settings {
  std::string hidraw = "/dev/hidraw2";
  uint8_t device = KEYBOARD_RGB_ID;
  uint8_t effect = RGB_EFFECT_STATIC;
  uint8_t brightness = 100;
  uint8_t speed = 0;
  uint8_t direction = 0;
  uint8_t r = 255, g = 255, b = 255;
  uint8_t zone = 0x0f; // all (keyboard)
};

static bool write_file_atomic(const std::string& path, const std::string& content) {
  std::string tmp = path + ".tmp";
  int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) return false;
  ssize_t w = ::write(fd, content.data(), content.size());
  ::close(fd);
  if (w < 0 || (size_t)w != content.size()) return false;
  return ::rename(tmp.c_str(), path.c_str()) == 0;
}

static std::optional<std::string> read_file(const std::string& path) {
  int fd = ::open(path.c_str(), O_RDONLY);
  if (fd < 0) return std::nullopt;
  std::string out;
  char buf[4096];
  for (;;) {
    ssize_t r = ::read(fd, buf, sizeof(buf));
    if (r == 0) break;
    if (r < 0) { ::close(fd); return std::nullopt; }
    out.append(buf, buf + r);
  }
  ::close(fd);
  return out;
}

static std::vector<std::string> split_ws(std::string_view s) {
  std::vector<std::string> v;
  size_t i = 0;
  while (i < s.size()) {
    while (i < s.size() && std::isspace((unsigned char)s[i])) i++;
    if (i >= s.size()) break;
    size_t j = i;
    while (j < s.size() && !std::isspace((unsigned char)s[j])) j++;
    v.emplace_back(s.substr(i, j - i));
    i = j;
  }
  return v;
}

static std::unordered_map<std::string, std::string> parse_kv(const std::vector<std::string>& toks, size_t start) {
  std::unordered_map<std::string, std::string> m;
  for (size_t i = start; i < toks.size(); i++) {
    auto& t = toks[i];
    auto pos = t.find('=');
    if (pos == std::string::npos) continue;
    m.emplace(t.substr(0, pos), t.substr(pos + 1));
  }
  return m;
}

static bool parse_u8(const std::string& s, int minv, int maxv, uint8_t& out) {
  char* end = nullptr;
  long v = std::strtol(s.c_str(), &end, 10);
  if (!end || *end != '\0') return false;
  if (v < minv || v > maxv) return false;
  out = (uint8_t)v;
  return true;
}

// dev string -> device id
static std::optional<uint8_t> parse_device(const std::string& s) {
  if (s == "keyboard") return KEYBOARD_RGB_ID;
  if (s == "lid") return LID_RGB_ID;
  if (s == "button") return BUTTON_RGB_ID;
  return std::nullopt;
}

// effect string -> effect id
static std::optional<uint8_t> parse_effect(const std::string& s) {
  if (s == "static") return RGB_EFFECT_STATIC;
  if (s == "breathing") return RGB_EFFECT_BREATHING;
  if (s == "neon") return RGB_EFFECT_NEON;
  if (s == "wave") return RGB_EFFECT_WAVE;
  if (s == "ripple") return RGB_EFFECT_RIPPLE;
  if (s == "zoom") return RGB_EFFECT_ZOOM;
  if (s == "snake") return RGB_EFFECT_SNAKE;
  if (s == "disco") return RGB_EFFECT_DISCO;
  if (s == "shifting") return RGB_EFFECT_SHIFTING;
  return std::nullopt;
}

// dir string -> 0/1/2
static std::optional<uint8_t> parse_dir(const std::string& s) {
  if (s == "none")  return 0;
  if (s == "right") return 1;
  if (s == "left")  return 2;
  return std::nullopt;
}

// zone string -> bitmask
static std::optional<uint8_t> parse_zone(const std::string& s) {
  if (s == "all") return 0x0f;
  if (s == "1")   return 0x01;
  if (s == "2")   return 0x02;
  if (s == "3")   return 0x04;
  if (s == "4")   return 0x08;
  return std::nullopt;
}

static bool hid_write_feature(const Settings& cfg, std::string& err) {
  int fd = ::open(cfg.hidraw.c_str(), O_RDWR | O_NONBLOCK);
  if (fd < 0) { err = "open hidraw failed"; return false; }

  std::vector<uint8_t> bytes = {
    RGB_FEATURE_ID,
    cfg.device,
    cfg.effect,
    cfg.brightness,
    cfg.speed,
    cfg.direction,
    cfg.r, cfg.g, cfg.b,
    cfg.zone,
    0x00
  };

  int retval = ::ioctl(fd, HIDIOCSFEATURE(bytes.size()), bytes.data());
  ::close(fd);
  if (retval < 0) { err = "ioctl HIDIOCSFEATURE failed"; return false; }
  return true;
}

static bool apply_from_kv(Settings& s, const std::unordered_map<std::string,std::string>& kv, std::string& err) {
  if (auto it = kv.find("hidraw"); it != kv.end()) s.hidraw = it->second;

  // dev já é resolvido fora para escolher o slot correto, mas se vier aqui também ok:
  if (auto it = kv.find("dev"); it != kv.end()) {
    auto d = parse_device(it->second);
    if (!d) { err="invalid dev"; return false; }
    s.device = *d;
  }

  if (auto it = kv.find("effect"); it != kv.end()) {
    auto e = parse_effect(it->second);
    if (!e) { err="invalid effect"; return false; }
    s.effect = *e;
  }

  if (auto it = kv.find("bright"); it != kv.end()) {
    if (!parse_u8(it->second, 0, 100, s.brightness)) { err="bright 0-100"; return false; }
  }

  if (auto it = kv.find("speed"); it != kv.end()) {
    if (!parse_u8(it->second, 0, 9, s.speed)) { err="speed 0-9"; return false; }
  }

  if (auto it = kv.find("dir"); it != kv.end()) {
    auto d = parse_dir(it->second);
    if (!d) { err="dir none|right|left"; return false; }
    s.direction = *d;
  }

  if (auto it = kv.find("r"); it != kv.end()) {
    if (!parse_u8(it->second, 0, 255, s.r)) { err="r 0-255"; return false; }
  }
  if (auto it = kv.find("g"); it != kv.end()) {
    if (!parse_u8(it->second, 0, 255, s.g)) { err="g 0-255"; return false; }
  }
  if (auto it = kv.find("b"); it != kv.end()) {
    if (!parse_u8(it->second, 0, 255, s.b)) { err="b 0-255"; return false; }
  }

  if (auto it = kv.find("zone"); it != kv.end()) {
    auto z = parse_zone(it->second);
    if (!z) { err="zone all|1|2|3|4"; return false; }
    s.zone = *z;
  }

  // Regras coerentes com o programa original:
  if (s.effect == RGB_EFFECT_STATIC) {
    s.speed = 0;
  }
  if (s.device != KEYBOARD_RGB_ID) {
    s.direction = 0;
    s.zone = 0x00;
  } else {
    if (s.effect == RGB_EFFECT_STATIC || s.effect == RGB_EFFECT_BREATHING) {
      s.direction = 0;
    }
    if (s.zone == 0x00) s.zone = 0x0f; // se alguém mandar zone=0 por engano
  }

  return true;
}

static bool ensure_dirs() {
  ::mkdir(STATE_DIR, 0755);
  return true;
}

static std::string device_to_str(uint8_t dev) {
  if (dev == KEYBOARD_RGB_ID) return "keyboard";
  if (dev == LID_RGB_ID) return "lid";
  return "button";
}

static std::string effect_to_str(uint8_t eff) {
  switch (eff) {
    case RGB_EFFECT_STATIC: return "static";
    case RGB_EFFECT_BREATHING: return "breathing";
    case RGB_EFFECT_NEON: return "neon";
    case RGB_EFFECT_WAVE: return "wave";
    case RGB_EFFECT_RIPPLE: return "ripple";
    case RGB_EFFECT_ZOOM: return "zoom";
    case RGB_EFFECT_SNAKE: return "snake";
    case RGB_EFFECT_DISCO: return "disco";
    case RGB_EFFECT_SHIFTING: return "shifting";
    default: return "static";
  }
}

static std::string dir_to_str(uint8_t d) {
  if (d == 1) return "right";
  if (d == 2) return "left";
  return "none";
}

static std::string zone_to_str(uint8_t z) {
  if (z == 0x01) return "1";
  if (z == 0x02) return "2";
  if (z == 0x04) return "3";
  if (z == 0x08) return "4";
  return "all";
}

static std::string serialize_one(const Settings& s) {
  return std::format(
    "SET dev={} hidraw={} effect={} bright={} speed={} dir={} r={} g={} b={} zone={}\n",
    device_to_str(s.device),
    s.hidraw,
    effect_to_str(s.effect),
    (int)s.brightness,
    (int)s.speed,
    dir_to_str(s.direction),
    (int)s.r, (int)s.g, (int)s.b,
    zone_to_str(s.zone)
  );
}

static std::string serialize_all(const std::array<Settings,3>& st) {
  return serialize_one(st[0]) + serialize_one(st[1]) + serialize_one(st[2]);
}

static int dev_index(uint8_t dev) {
  if (dev == KEYBOARD_RGB_ID) return 0;
  if (dev == LID_RGB_ID) return 1;
  return 2;
}

static bool load_and_apply_all(std::array<Settings,3>& states) {
  auto content = read_file(STATE_PATH);
  if (!content) return false;

  bool any = false;
  size_t pos = 0;

  while (pos < content->size()) {
    size_t end = content->find('\n', pos);
    if (end == std::string::npos) end = content->size();
    std::string line = content->substr(pos, end - pos);
    pos = end + 1;

    auto toks = split_ws(line);
    if (toks.size() < 2 || toks[0] != "SET") continue;

    auto kv = parse_kv(toks, 1);

    auto it = kv.find("dev");
    if (it == kv.end()) continue;

    auto devOpt = parse_device(it->second);
    if (!devOpt) continue;

    int idx = dev_index(*devOpt);
    Settings newState = states[idx];
    std::string perr;
    if (!apply_from_kv(newState, kv, perr)) continue;

    newState.device = *devOpt;

    std::string hwerr;
    if (hid_write_feature(newState, hwerr)) {
      states[idx] = newState;
      any = true;
    }
  }

  return any;
}

static int make_server_socket(std::string& err) {
  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) { err="socket failed"; return -1; }

  ::unlink(SOCK_PATH);

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", SOCK_PATH);

  if (::bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { err="bind failed"; ::close(fd); return -1; }
  ::chmod(SOCK_PATH, 0666);

  if (::listen(fd, 16) < 0) { err="listen failed"; ::close(fd); return -1; }
  return fd;
}

static std::string read_line(int fd) {
  std::string s;
  char c;
  while (true) {
    ssize_t r = ::read(fd, &c, 1);
    if (r <= 0) break;
    if (c == '\n') break;
    s.push_back(c);
    if (s.size() > 8192) break;
  }
  return s;
}

static void write_all(int fd, std::string_view s) {
  ::write(fd, s.data(), s.size());
}

int main() {
  if (geteuid() != 0) {
    std::println("[ERR] Run as root (or use udev permissions).");
    return 1;
  }

  ensure_dirs();

  // 3 estados: keyboard/lid/button
  std::array<Settings,3> states;

  states[0].device = KEYBOARD_RGB_ID;
  states[0].zone   = 0x0f;

  states[1].device = LID_RGB_ID;
  states[1].zone   = 0x00;

  states[2].device = BUTTON_RGB_ID;
  states[2].zone   = 0x00;

  // restore no boot
  (void)load_and_apply_all(states);

  std::string err;
  int sfd = make_server_socket(err);
  if (sfd < 0) {
    std::println("[ERR] {}", err);
    return 1;
  }

  for (;;) {
    int cfd = ::accept(sfd, nullptr, nullptr);
    if (cfd < 0) continue;

    std::string line = read_line(cfd);
    auto toks = split_ws(line);

    if (toks.empty()) {
      write_all(cfd, "ERR empty\n");
      ::close(cfd);
      continue;
    }

    if (toks[0] == "GET") {
      auto content = read_file(STATE_PATH);
      if (content) write_all(cfd, *content);
      else write_all(cfd, "ERR no-state\n");
      ::close(cfd);
      continue;
    }

    if (toks[0] != "SET") {
      write_all(cfd, "ERR unknown-cmd\n");
      ::close(cfd);
      continue;
    }

    auto kv = parse_kv(toks, 1);

    auto it = kv.find("dev");
    if (it == kv.end()) {
      write_all(cfd, "ERR missing dev\n");
      ::close(cfd);
      continue;
    }

    auto devOpt = parse_device(it->second);
    if (!devOpt) {
      write_all(cfd, "ERR invalid dev\n");
      ::close(cfd);
      continue;
    }

    int idx = dev_index(*devOpt);
    Settings newState = states[idx];

    std::string perr;
    if (!apply_from_kv(newState, kv, perr)) {
      write_all(cfd, std::format("ERR {}\n", perr));
      ::close(cfd);
      continue;
    }

    // garantir coerência
    newState.device = *devOpt;

    // defaults de zone por device (se não veio e for teclado)
    if (newState.device == KEYBOARD_RGB_ID && newState.zone == 0x00) newState.zone = 0x0f;
    if (newState.device != KEYBOARD_RGB_ID) newState.zone = 0x00;

    std::string hwerr;
    if (!hid_write_feature(newState, hwerr)) {
      write_all(cfd, std::format("ERR {}\n", hwerr));
      ::close(cfd);
      continue;
    }

    states[idx] = newState;
    (void)write_file_atomic(STATE_PATH, serialize_all(states));

    write_all(cfd, "OK\n");
    ::close(cfd);
  }
}
