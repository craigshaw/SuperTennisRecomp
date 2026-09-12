#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROM=${SUPER_TENNIS_ROM:-"$ROOT/reference/Super Tennis (USA).sfc"}
REPLAY=${SUPER_TENNIS_REPLAY:-}
REPORT=${SNESRECOMP_EVENT_CROSSING_AUDIT:-"$ROOT/build/event-crossing-audit.json"}

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

restore_normal_build() {
  printf '%s\n' 'Restoring the normal non-instrumented generated build.'
  sh "$ROOT/tools/regenerate.sh" "$ROM"
  sh "$ROOT/build.sh"
}

# Always remove generated audit calls after an interrupted or failed run.
trap 'restore_normal_build' EXIT

mkdir -p "$(dirname -- "$REPORT")"
printf '%s\n' 'Generating the event-crossing audit build.'
python3 "$ROOT/tools/generate-normal.py" "$ROM" --event-crossing-audit
sh "$ROOT/build.sh"

if [ "$#" -eq 1 ]; then
  SNESRECOMP_EVENT_CROSSING_AUDIT="$REPORT" \
    sh "$ROOT/tools/run-input-replay.sh" "$1"
else
  SNESRECOMP_EVENT_CROSSING_AUDIT="$REPORT" \
    sh "$ROOT/tools/run-input-replay.sh"
fi

trap - EXIT
restore_normal_build
printf 'Event-crossing report: %s\n' "$REPORT"
printf '%s\n' 'Build its precision path with: sh tools/build-event-precision.sh'
