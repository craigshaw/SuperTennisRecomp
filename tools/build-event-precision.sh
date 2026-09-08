#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROM=${SUPER_TENNIS_ROM:-"$ROOT/reference/Super Tennis (USA).sfc"}
REPORT=${SNESRECOMP_EVENT_PRECISION_PROFILE:-"$ROOT/build/event-crossing-audit.json"}

if [ ! -f "$ROM" ]; then
  printf '%s\n' 'Set SUPER_TENNIS_ROM to the supported user-owned ROM.' >&2
  exit 2
fi
if [ ! -f "$REPORT" ]; then
  printf '%s\n' 'Run tools/run-event-crossing-audit.sh first or set SNESRECOMP_EVENT_PRECISION_PROFILE.' >&2
  exit 2
fi

printf 'Generating from event precision profile: %s\n' "$REPORT"
SNESRECOMP_EVENT_PRECISION_PROFILE="$REPORT" \
  sh "$ROOT/tools/regenerate.sh" "$ROM"
sh "$ROOT/build.sh"
printf '%s\n' 'Event precision build ready. A normal regeneration removes it.'
