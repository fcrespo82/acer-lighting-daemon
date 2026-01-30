# acer-lighting

Small tools to control Acer laptop RGB zones and a daemon that persists/apply states.

## Build

- Build the daemon only:

```sh
make acer-rgbd
```

- Build the CLI (used for manual commands and test targets):

```sh
make acer-rgb-cli
```

Or build everything with:

```sh
make build
```

## Install

Install the daemon, helper script and systemd units:

```sh
sudo make install
```

The `install` target will:
- Copy `acer-rgbd` to `/usr/local/bin/acer-rgbd` and `acer-rgb` to `/usr/local/bin/acer-rgb`.
- Install systemd unit and socket under `/etc/systemd/system/` and enable/start the service.
- Create `/var/lib/acer-rgbd/state.txt` with an initial "all green" state so the daemon applies green on first start.

If you need to undo the install:

```sh
sudo make uninstall
```

## Usage

- Send commands to the daemon using the `acer-rgb` helper (it talks to the daemon socket):

```sh
acer-rgb SET dev=keyboard hidraw=/dev/hidraw2 effect=static bright=100 r=0 g=255 b=0 zone=all
acer-rgb GET
```

- You can also use the CLI binary for direct HID control (for testing):

```sh
sudo ./acer-rgb-cli /dev/hidraw2 keyboard static --brightness 100 --rgb 0 255 0 --zone all
```

## State file

The daemon persists three lines (keyboard, lid, button) in `/var/lib/acer-rgbd/state.txt`. Editing that file (as root) changes the values the daemon will reapply on start.

## Logs & troubleshooting

View the daemon logs with:

```sh
journalctl -u acer-rgbd.service -f
```

If you modify the state file and want the daemon to reapply immediately, restart it:

```sh
sudo systemctl restart acer-rgbd.service
```

## Notes

- The installer writes an initial all-green state (keyboard/lid/button) to `/var/lib/acer-rgbd/state.txt` so newly installed systems show green LEDs by default.
- Running the daemon requires root privileges (or appropriate udev rules) to access the HID device.
