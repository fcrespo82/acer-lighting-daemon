
all: 

rgb:
	clang++ -std=c++23 ./rgb.cpp -o ./rgb

acer-rgb:
	clang++ -std=c++23 ./acer-rgb.cpp -o ./acer-rgb


all-red: acer-rgb
	sudo ./acer-rgb /dev/hidraw2 keyboard static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb /dev/hidraw2 lid static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb /dev/hidraw2 button static --brightness 100 --rgb 255 0 0 --zone all

all-green: acer-rgb
	sudo ./acer-rgb /dev/hidraw2 keyboard static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb /dev/hidraw2 lid static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb /dev/hidraw2 button static --brightness 100 --rgb 0 255 0 --zone all

all-blue: acer-rgb
	sudo ./acer-rgb /dev/hidraw2 keyboard static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 lid static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 button static --brightness 100 --rgb 0 0 255 --zone all

all-magenta: acer-rgb
	sudo ./acer-rgb /dev/hidraw2 keyboard static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 lid static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 button static --brightness 100 --rgb 255 0 255 --zone all

all-cyan: acer-rgb
	sudo ./acer-rgb /dev/hidraw2 keyboard static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 lid static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb /dev/hidraw2 button static --brightness 100 --rgb 0 255 255 --zone all