#!/usr/bin/env bash
set -euo pipefail

SOCK="/run/acer-rgbd.sock"

if [[ $# -lt 1 ]]; then
  echo "Usage:"
  echo "  acer-rgb GET"
  echo "  acer-rgb SET hidraw=/dev/hidraw2 dev=keyboard effect=static bright=80 r=255 g=0 b=0 zone=all"
  exit 2
fi

cmd="$*"
if command -v socat >/dev/null 2>&1; then
  printf "%s\n" "$cmd" | socat - UNIX-CONNECT:"$SOCK"
else
  printf "%s\n" "$cmd" | nc -U "$SOCK"
fi
