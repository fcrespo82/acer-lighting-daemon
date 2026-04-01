# acer-lighting

**Thanks**

This project benefited greatly from the work, testing and research of several community contributors — especially: @ZoeBattleSand, @0x189D7997, and @JakeBrxwn. Their reverse-engineering, testing, tooling and discussion (see https://github.com/0x7375646F/Linuwu-Sense/pull/65) made much of this possible.

Small tools to control Acer laptop RGB zones and a daemon that persists/apply states.

> [!CAUTION]
> The code here was based on the work of others but it was written with the help of AI too 

## Supported Models

Different Acer laptop generations use different HID controllers for RGB:

| Model                       | Controller        | VID:PID   |
|-----------------------------|-------------------|-----------|
| PHN16-73 (Helios Neo 16 AI) | ENEK5130          | 0cf2:5130 |
| Older Predator/Nitro        | Embedded keyboard | 1025:174b |

The installer script creates a udev rule that sets up `/dev/acer-rgb` as a stable symlink to the correct device.

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

Install the daemon, helper script, udev rules, and systemd units, creates group and add user to group:
```sh
sudo make install
```

The `install` target will:

- Copy `acer-rgbd` to `/usr/local/bin/acer-rgbd` and `acer-rgb` to `/usr/local/bin/acer-rgb`.
- Install udev rules to `/etc/udev/rules.d/99-acer-rgb.rules` (creates `/dev/acer-rgb` symlink).
- Create group `acer_rgb` to control the device.
- Add the current user to the `acer_rgb` group.
- Install systemd unit and socket under `/etc/systemd/system/` and enable/start the service.
- Create `/var/lib/acer-rgbd/state.txt` with an initial "all green" state so the daemon applies green on first start.

If you need to undo the install:
```sh
sudo make uninstall
```

## Usage

- Send commands to the daemon using the `acer-rgb` helper (it talks to the daemon socket):
```sh
acer-rgb SET dev=keyboard hidraw=/dev/acer-rgb effect=static bright=100 r=0 g=255 b=0 zone=all
acer-rgb GET
```

- You can also use the CLI binary for direct HID control (for testing):
```sh
sudo ./acer-rgb-cli /dev/acer-rgb keyboard static --brightness 100 --rgb 0 255 0 --zone all
```

## Finding your device manually

If `/dev/acer-rgb` does not exist after install, you can find the correct hidraw device:
```sh
for dev in /sys/class/hidraw/hidraw*; do
echo "=== $(basename $dev) ==="
cat $dev/device/uevent 2>/dev/null | grep -E "HID_NAME|HID_ID"
done
```

Look for either:
- `HID_ID=...00000CF2...00005130` (ENEK5130 - PHN16-73)
- `HID_ID=...00001025...0000174B` (older models)

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
- The udev rule sets MODE="0660" and GROUP="plugdev" for non-root access. Add your user to the `plugdev` group if needed: `sudo usermod -aG plugdev $USER`
