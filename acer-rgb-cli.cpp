#include <print>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <string>
#include <iostream>
#include <limits>
#include <cstdint>
#include <cstdlib>

#define RGB_FEATURE_ID 0xa4

#define KEYBOARD_RGB_ID 0x21
#define LID_RGB_ID 0x83
#define BUTTON_RGB_ID 0x65

#define RGB_EFFECT_STATIC 0x02
#define RGB_EFFECT_BREATHING 0x04
#define RGB_EFFECT_NEON 0x05
#define RGB_EFFECT_WAVE 0x07
#define RGB_EFFECT_RIPPLE 0x08
#define RGB_EFFECT_ZOOM 0x09
#define RGB_EFFECT_SNAKE 0x0a
#define RGB_EFFECT_DISCO 0x0b
#define RGB_EFFECT_SHIFTING 0xff

static int g_fd = -1;

static void usage(const char* prog) {
	std::println(
		"Usage:\n"
		"  sudo {0} <hidraw_path> <device> <effect> [options]\n\n"
		"Devices:\n"
		"  keyboard | lid | button\n\n"
		"Effects:\n"
		"  static | breathing | neon | wave | ripple | zoom | snake | disco | shifting\n\n"
		"Options:\n"
		"  --brightness <0-100>        (required)\n"
		"  --speed <0-9>               (required for non-static effects)\n"
		"  --direction <none|right|left>   (keyboard only; ignored for static/breathing)\n"
		"  --rgb <R> <G> <B>           (required for static)\n"
		"  --zone <all|1|2|3|4>        (keyboard only; default: all)\n\n"
		"Examples:\n"
		"  sudo {0} /dev/hidraw2 keyboard static --brightness 80 --rgb 255 0 0 --zone all\n"
		"  sudo {0} /dev/hidraw2 keyboard wave --brightness 70 --speed 6 --direction right\n"
		"  sudo {0} /dev/hidraw2 lid breathing --brightness 60 --speed 5\n",
		prog
	);
}

static bool hidrawWrite(const std::vector<uint8_t>& bytes) {
	for (uint8_t byte : bytes) {
		std::print("0x{:0x} ", static_cast<uint32_t>(byte));
	}
	std::println(" <--- hidraw values");

	int retval = ioctl(g_fd, HIDIOCSFEATURE(bytes.size()), bytes.data());
	if (retval < 0) {
		std::println("[ERR] FAILED TO WRITE TO HIDRAW DEVICE!");
		return false;
	}

	std::println("Successfully wrote to hidraw file");
	return true;
}

static bool streq(const std::string& a, const char* b) {
	return a == b;
}

static bool parse_u8_int(const std::string& s, int minv, int maxv, uint8_t& out) {
	char* end = nullptr;
	long v = std::strtol(s.c_str(), &end, 10);
	if (!end || *end != '\0') return false;
	if (v < minv || v > maxv) return false;
	out = static_cast<uint8_t>(v);
	return true;
}

static bool parse_device(const std::string& s, uint8_t& device, uint8_t& zone_default) {
	if (streq(s, "keyboard")) { device = KEYBOARD_RGB_ID; zone_default = 0x0f; return true; }
	if (streq(s, "lid"))      { device = LID_RGB_ID;      zone_default = 0x00; return true; }
	if (streq(s, "button"))   { device = BUTTON_RGB_ID;   zone_default = 0x00; return true; }
	return false;
}

static bool parse_effect(const std::string& s, uint8_t& effect) {
	if (streq(s, "static"))    { effect = RGB_EFFECT_STATIC; return true; }
	if (streq(s, "breathing")) { effect = RGB_EFFECT_BREATHING; return true; }
	if (streq(s, "neon"))      { effect = RGB_EFFECT_NEON; return true; }
	if (streq(s, "wave"))      { effect = RGB_EFFECT_WAVE; return true; }
	if (streq(s, "ripple"))    { effect = RGB_EFFECT_RIPPLE; return true; }
	if (streq(s, "zoom"))      { effect = RGB_EFFECT_ZOOM; return true; }
	if (streq(s, "snake"))     { effect = RGB_EFFECT_SNAKE; return true; }
	if (streq(s, "disco"))     { effect = RGB_EFFECT_DISCO; return true; }
	if (streq(s, "shifting"))  { effect = RGB_EFFECT_SHIFTING; return true; }
	return false;
}

static bool parse_direction(const std::string& s, uint8_t& dir) {
	if (streq(s, "none"))  { dir = 0; return true; }
	if (streq(s, "right")) { dir = 1; return true; }
	if (streq(s, "left"))  { dir = 2; return true; }
	return false;
}

static bool parse_zone(const std::string& s, uint8_t& zone) {
	if (streq(s, "all")) { zone = 0x0f; return true; }
	if (streq(s, "1"))   { zone = 0x01; return true; }
	if (streq(s, "2"))   { zone = 0x02; return true; }
	if (streq(s, "3"))   { zone = 0x04; return true; }
	if (streq(s, "4"))   { zone = 0x08; return true; }
	return false;
}

int main(int argc, char* argv[]) {
	if (geteuid() != 0) {
		std::println("[ERR] THIS APPLICATION MUST BE RUN AS ROOT!");
		return 1;
	}

	if (argc < 4) {
		usage(argv[0]);
		return 2;
	}

	std::string hidraw_path = argv[1];
	std::string device_str = argv[2];
	std::string effect_str = argv[3];

	uint8_t device = 0x00;
	uint8_t zone = 0x00;
	uint8_t effect = 0x00;

	if (!parse_device(device_str, device, zone)) {
		std::println("[ERR] Invalid device: {}", device_str);
		usage(argv[0]);
		return 2;
	}

	if (!parse_effect(effect_str, effect)) {
		std::println("[ERR] Invalid effect: {}", effect_str);
		usage(argv[0]);
		return 2;
	}

	// Defaults
	uint8_t brightness = 0x00;
	uint8_t speed = 0x00;
	uint8_t direction = 0x00;
	uint8_t r = 0x00, g = 0x00, b = 0x00;

	bool brightness_set = false;
	bool speed_set = false;
	bool rgb_set = false;
	bool zone_set = false;
	bool direction_set = false;

	// Parse options
	for (int i = 4; i < argc; i++) {
		std::string opt = argv[i];

		auto need_arg = [&](int n) -> bool {
			if (i + n >= argc) {
				std::println("[ERR] Missing argument(s) for {}", opt);
				return false;
			}
			return true;
		};

		if (opt == "--brightness") {
			if (!need_arg(1)) return 2;
			if (!parse_u8_int(argv[++i], 0, 100, brightness)) {
				std::println("[ERR] brightness must be 0-100");
				return 2;
			}
			brightness_set = true;
		} else if (opt == "--speed") {
			if (!need_arg(1)) return 2;
			if (!parse_u8_int(argv[++i], 0, 9, speed)) {
				std::println("[ERR] speed must be 0-9");
				return 2;
			}
			speed_set = true;
		} else if (opt == "--direction") {
			if (!need_arg(1)) return 2;
			if (!parse_direction(argv[++i], direction)) {
				std::println("[ERR] direction must be none|right|left");
				return 2;
			}
			direction_set = true;
		} else if (opt == "--zone") {
			if (!need_arg(1)) return 2;
			if (!parse_zone(argv[++i], zone)) {
				std::println("[ERR] zone must be all|1|2|3|4");
				return 2;
			}
			zone_set = true;
		} else if (opt == "--rgb") {
			if (!need_arg(3)) return 2;
			uint8_t rr, gg, bb;
			if (!parse_u8_int(argv[++i], 0, 255, rr) ||
				!parse_u8_int(argv[++i], 0, 255, gg) ||
				!parse_u8_int(argv[++i], 0, 255, bb)) {
				std::println("[ERR] rgb values must be 0-255");
				return 2;
			}
			r = rr; g = gg; b = bb;
			rgb_set = true;
		} else if (opt == "--help" || opt == "-h") {
			usage(argv[0]);
			return 0;
		} else {
			std::println("[ERR] Unknown option: {}", opt);
			usage(argv[0]);
			return 2;
		}
	}

	// Validate required combos
	if (!brightness_set) {
		std::println("[ERR] --brightness is required");
		return 2;
	}

	if (effect == RGB_EFFECT_STATIC) {
		if (!rgb_set) {
			std::println("[ERR] static requires --rgb R G B");
			return 2;
		}
		// speed ignored for static
		speed = 0;
	} else {
		if (!speed_set) {
			std::println("[ERR] non-static effects require --speed 0-9");
			return 2;
		}
	}

	// Only keyboard supports zone; for others force 0x00
	if (device != KEYBOARD_RGB_ID) {
		zone = 0x00;
	} else {
		if (!zone_set) zone = 0x0f; // default all
		// direction only used for certain keyboard effects (like original)
		if (effect == RGB_EFFECT_STATIC || effect == RGB_EFFECT_BREATHING) {
			direction = 0;
		} else {
			// If user didn't set, keep default 0 (none)
			(void)direction_set;
		}
	}

	// Open + identify
	std::println("Using file: {}", hidraw_path);
	g_fd = open(hidraw_path.c_str(), O_RDWR | O_NONBLOCK);
	if (g_fd < 0) {
		std::println("[ERR] FAILED TO OPEN HIDRAW DEVICE!");
		return 1;
	}

	char name[256];
	int retval = ioctl(g_fd, HIDIOCGRAWNAME(256), name);
	if (retval < 0) {
		std::println("[ERR] FAILED TO GET HIDRAW DEVICE NAME!");
		close(g_fd);
		return 1;
	}
	std::println("Device name: {}", name);

	std::vector<uint8_t> hidraw_data = {
		RGB_FEATURE_ID, device, effect, brightness, speed, direction, r, g, b, zone, 0x00
	};

	bool ok = hidrawWrite(hidraw_data);
	close(g_fd);

	return ok ? 0 : 1;
}
