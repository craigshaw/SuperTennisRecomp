#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROM=${SUPER_TENNIS_ROM:-"$ROOT/reference/Super Tennis (USA).sfc"}
REPLAY=${SUPER_TENNIS_REPLAY:-}

if [ ! -f "$ROM" ]; then
  printf '%s\n' 'Set SUPER_TENNIS_ROM to the supported user-owned ROM.' >&2
  exit 2
fi
if [ -z "$REPLAY" ] || [ ! -f "$REPLAY" ]; then
  printf '%s\n' 'Set SUPER_TENNIS_REPLAY to a complete private .sri replay.' >&2
  exit 2
fi

if [ "$#" -gt 1 ]; then
  printf 'usage: %s [FRAME_LIMIT]\n' "$0" >&2
  exit 2
fi
if [ "$#" -eq 1 ]; then
  exec "$ROOT/build/super_tennis_headless" "$ROM" \
    --frames "$1" --replay "$REPLAY"
fi
exec "$ROOT/build/super_tennis_headless" "$ROM" --replay "$REPLAY"
