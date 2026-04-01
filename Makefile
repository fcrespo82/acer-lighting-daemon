GROUP := acer_rgb
USER := $(or $(SUDO_USER),$(shell id -un))

CXX := g++
CXXFLAGS := -O2 -std=c++23

.PHONY: all install uninstall clean clean-cli build build-cli \
        all-red all-green all-blue all-magenta all-cyan

all: build

acer-rgb-cli:
	$(CXX) $(CXXFLAGS) acer-rgb-cli.cpp -o acer-rgb-cli

build-cli: acer-rgb-cli

clean-cli:
	rm -f acer-rgb-cli

acer-rgbd: 
	$(CXX) $(CXXFLAGS) acer-rgbd.cpp -o acer-rgbd

build: acer-rgbd

clean:
	rm -f acer-rgbd

install: acer-rgbd
	sudo install -Dm755 acer-rgbd /usr/local/bin/acer-rgbd
	sudo install -Dm755 acer-rgb.sh  /usr/local/bin/acer-rgb
	sudo install -Dm644 acer-rgbd.service /etc/systemd/system/acer-rgbd.service
	sudo install -Dm644 acer-rgbd.socket /etc/systemd/system/acer-rgbd.socket
	sudo install -Dm644 99-acer-rgb.rules /etc/udev/rules.d/99-acer-rgb.rules
	# install default state only if it doesn't already exist
	@if [ ! -f /var/lib/acer-rgbd/state.txt ]; then \
		echo "Installing default state to /var/lib/acer-rgbd/state.txt"; \
		sudo install -Dm644 state.default /var/lib/acer-rgbd/state.txt; \
	fi
	-sudo groupadd $(GROUP)
	-sudo usermod -aG $(GROUP) $(USER)
	sudo udevadm control --reload-rules
	sudo udevadm trigger
	sudo systemctl daemon-reload
	sudo systemctl enable --now acer-rgbd.service
	sudo systemctl enable --now acer-rgbd.socket

uninstall:
	-sudo systemctl disable --now acer-rgbd.socket
	-sudo systemctl disable --now acer-rgbd.service
	-sudo rm -f /usr/local/bin/acer-rgbd
	-sudo rm -f /usr/local/bin/acer-rgb
	-sudo rm -f /etc/systemd/system/acer-rgbd.service
	-sudo rm -f /etc/systemd/system/acer-rgbd.socket
	-sudo rm -f /etc/udev/rules.d/99-acer-rgb.rules
	-sudo gpasswd -d $(USER) $(GROUP) 2>/dev/null
	-sudo groupdel $(GROUP) 2>/dev/null
	-sudo udevadm control --reload-rules
	-sudo udevadm trigger
	-sudo systemctl daemon-reload


all-red: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb lid static --brightness 100 --rgb 255 0 0 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb button static --brightness 100 --rgb 255 0 0 --zone all

all-green: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb lid static --brightness 100 --rgb 0 255 0 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb button static --brightness 100 --rgb 0 255 0 --zone all

all-blue: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb lid static --brightness 100 --rgb 0 0 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb button static --brightness 100 --rgb 0 0 255 --zone all

all-magenta: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb lid static --brightness 100 --rgb 255 0 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb button static --brightness 100 --rgb 255 0 255 --zone all

all-cyan: acer-rgb-cli
	sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb lid static --brightness 100 --rgb 0 255 255 --zone all
	sudo ./acer-rgb-cli /dev/acer-rgb button static --brightness 100 --rgb 0 255 255 --zone all
