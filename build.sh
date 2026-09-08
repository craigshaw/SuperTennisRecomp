#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ ! -f "$ROOT/snesrecomp/runner/runner.cmake" ]; then
  printf '%s\n' 'Missing snesrecomp submodule. Run: git submodule update --init' >&2
  exit 1
fi
if [ ! -f "$ROOT/recomp-ui/recomp_ui.cmake" ]; then
  printf '%s\n' 'Missing recomp-ui submodule. Run: git submodule update --init' >&2
  exit 1
fi
if [ ! -f "$ROOT/generated/dispatch_v2.c" ]; then
  printf '%s\n' 'Missing generated code. Run tools/regenerate.sh first.' >&2
  exit 1
fi
cmake -S "$ROOT" -B "$ROOT/build" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build" --config Release --parallel
printf '
%s
' 'Built the desktop host, generated-code library, and headless smoke runner.'
printf '%s
' 'Run from this directory: build/super_tennis'
