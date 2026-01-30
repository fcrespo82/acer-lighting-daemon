#include <print>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <string>
#include <iostream>

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

std::string hidraw_path = "/dev/hidraw2";
int g_fd = -1;

bool hidrawWrite(const std::vector<uint8_t>&bytes) {
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

int main(int argc, char* argv[]) {
	if (argc > 1) {
		hidraw_path = std::string(argv[1]);
	}

	if (geteuid() != 0) {
		std::println("[ERR] THIS APPLICATION MUST BE RUN AS ROOT!");
		return 1;
	}

	std::println("Acer Predator Helios Neo 16S (PHN16S-71) RGB Control\nWritten by ZoeBattleSand\n");

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

		return 1;
	}
	std::println("Device name: {}", name);

	std::string user_input;

input_dev_start:
	std::print("Enter device (0 for keyboard, 1 for lid, 2 for mode button): ");
	std::cin >> user_input;
	if (user_input != "0" && user_input != "1" && user_input != "2") {
		std::println("Invalid input! Try again.");
		goto input_dev_start;
	}

	uint8_t device = 0x00;
	uint8_t zone = 0x00;
	uint8_t effect = 0x00;
	uint8_t brightness = 0x00;
	uint8_t speed = 0x00;
	uint8_t direction = 0x00;
	uint8_t r = 0x00;
	uint8_t g = 0x00;
	uint8_t b = 0x00;

	if (user_input == "0") {
		device = KEYBOARD_RGB_ID;
		zone = 0x0f;
	} else if (user_input == "1") {
		device = LID_RGB_ID;
		zone = 0x00;
	} else if (user_input == "2") {
		device = BUTTON_RGB_ID;
		zone = 0x00;
	}

	int effect_val = 0;
input_effect_start:
	std::print("Enter effect (0 for static, 1 for breathing, 2 for neon, 3 for wave, 4 for ripple, 5 for zoom, 6 for snake, 7 for disco, 8 for shifting): ");
	std::cin >> user_input;
	effect_val = std::stoi(user_input);
	switch(effect_val) {
		case 0:
			effect = RGB_EFFECT_STATIC;
			break;
		case 1:
			effect = RGB_EFFECT_BREATHING;
			break;
		case 2:
			effect = RGB_EFFECT_NEON;
			break;
		case 3:
			effect = RGB_EFFECT_WAVE;
			break;
		case 4:
			effect = RGB_EFFECT_RIPPLE;
			break;
		case 5:
			effect = RGB_EFFECT_ZOOM;
			break;
		case 6:
			effect = RGB_EFFECT_SNAKE;
			break;
		case 7:
			effect = RGB_EFFECT_DISCO;
			break;
		case 8:
			effect = RGB_EFFECT_SHIFTING;
			break;
		default:
			std::println("Invalid effect! Try again.");
			goto input_effect_start;
	}

	if (effect != RGB_EFFECT_STATIC) {
		int speed_val = 0;
input_speed_start:
		std::print("Enter speed (0-9): ");
		std::cin >> user_input;
		speed_val = std::stoi(user_input);
		if (speed_val > 9) {
			std::println("Invalid speed! Try again.");
			goto input_speed_start;
		}
		speed = static_cast<uint8_t>(speed_val);
	}

	if (device == KEYBOARD_RGB_ID) {
		if (effect != RGB_EFFECT_STATIC && effect != RGB_EFFECT_BREATHING) {
			int direction_val = 0;
input_direction_start:
			std::print("Enter direction (0 for none, 1 for right, 2 for left): ");
			std::cin >> user_input;
			direction_val = std::stoi(user_input);
			if (direction_val > 2) {
				std::println("Invalid direction! Try again.");
				goto input_direction_start;
			}
			direction = static_cast<uint8_t>(direction_val);
		}

		int zone_val = 0;
input_zone_start:
		std::print("Enter zone (0 for all, 1-4 to select a specific zone): ");
		std::cin >> user_input;
		zone_val = std::stoi(user_input);
		if (zone_val > 4) {
			std::println("Invalid zone! Try again.");
			goto input_zone_start;
		}
		switch (zone_val) {
			case 0:
				zone = 0x0f;
				break;
			case 1:
				zone = 0x01;
				break;
			case 2:
				zone = 0x02;
				break;
			case 3:
				zone = 0x04;
				break;
			case 4:
				zone = 0x08;
				break;
			default:
				zone = 0x00;
				break;
		}
	}

	if (effect == RGB_EFFECT_STATIC) {
		int red_val = 0;
input_red_start:
		std::print("Enter red value (0-255): ");
		std::cin >> user_input;
		red_val = std::stoi(user_input);
		if (red_val > std::numeric_limits<uint8_t>::max()) {
			std::println("Red value {:d} out of range! Try again.", red_val);
			goto input_red_start;
		}
		r = static_cast<uint8_t>(red_val);

		int green_val = 0;
input_green_start:
		std::print("Enter green value (0-255): ");
		std::cin >> user_input;
		green_val = std::stoi(user_input);
		if (green_val > std::numeric_limits<uint8_t>::max()) {
			std::println("Green value {:d} out of range! Try again.", green_val);
			goto input_green_start;
		}
		g = static_cast<uint8_t>(green_val);

		int blue_val = 0;
input_blue_start:
		std::print("Enter blue value (0-255): ");
		std::cin >> user_input;
		blue_val = std::stoi(user_input);
		if (blue_val > std::numeric_limits<uint8_t>::max()) {
			std::println("Blue value {:d} out of range! Try again.", blue_val);
			goto input_blue_start;
		}
		b = static_cast<uint8_t>(blue_val);
	}

	int bright_val = 0;
input_bright_start:
	std::print("Enter brightness value (0-100): ");
	std::cin >> user_input;
	bright_val = std::stoi(user_input);
	if (bright_val > 100) {
		std::println("Brightness value {:d} out of range! Try again.", bright_val);
		goto input_bright_start;
	}
	brightness = static_cast<uint8_t>(bright_val);

	std::vector<uint8_t> hidraw_data = { RGB_FEATURE_ID, device, effect, brightness, speed, direction, r, g, b, zone, 0x00 };

	if (!hidrawWrite(hidraw_data)) {
		std::println("[ERR] FAILED TO WRITE HIDRAW DATA!");
		return 1;
	}

	close(g_fd);

	return 0;
}
