
all: build

# rgb:
# 	clang++ -std=c++23 ./rgb.cpp -o ./rgb

acer-rgb-cli:
	clang++ -std=c++23 acer-rgb-cli.cpp -o acer-rgb-cli

build-cli: acer-rgb-cli

clean-cli:
	rm -f acer-rgb-cli

acer-rgbd: 
	g++ -O2 -std=c++23 acer-rgbd.cpp -o acer-rgbd

build: acer-rgbd

clean:
	rm -f acer-rgbd

install: acer-rgbd acer-rgb
	sudo install -Dm755 acer-rgbd /usr/local/bin/acer-rgbd
	sudo install -Dm755 acer-rgb.sh  /usr/local/bin/acer-rgb
	sudo install -Dm644 acer-rgbd.service /etc/systemd/system/acer-rgbd.service
	sudo install -Dm644 acer-rgbd.socket /etc/systemd/system/acer-rgbd.socket

	sudo systemctl daemon-reload
	sudo systemctl enable --now acer-rgbd.service

uninstall:
	sudo systemctl disable --now acer-rgbd.service
	sudo rm -f /usr/local/bin/acer-rgbd
	sudo rm -f /usr/local/bin/acer-rgb
	sudo rm -f /etc/systemd/system/acer-rgbd.service
	sudo rm -f /etc/systemd/system/acer-rgbd.socket
	sudo systemctl daemon-reload


all-red: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 lid static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 button static --brightness 100 --rgb 255 0 0 --zone all

all-green: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 lid static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 button static --brightness 100 --rgb 0 255 0 --zone all

all-blue: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 lid static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 button static --brightness 100 --rgb 0 0 255 --zone all

all-magenta: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 lid static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 button static --brightness 100 --rgb 255 0 255 --zone all

all-cyan: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 lid static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb-cli /dev/hidraw2 button static --brightness 100 --rgb 0 255 255 --zone all