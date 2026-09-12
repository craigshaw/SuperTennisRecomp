#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
ROM=${1:-${SUPER_TENNIS_ROM:-}}
EXPECTED_SHA256=6e45a80ea148654514cb4e8604a0ffcbc726946e70f9e0b9860e36c0f3fa4877

if [ -z "$ROM" ]; then
  printf 'usage: %s /path/to/Super Tennis (USA).sfc\n' "$0" >&2
  exit 2
fi
if [ ! -f "$ROM" ]; then
  printf 'ROM not found: %s\n' "$ROM" >&2
  exit 1
fi
if [ ! -f "$ROOT/config/bank00.cfg" ]; then
  printf '%s\n' 'Missing config/bank00.cfg.' >&2
  printf '%s\n' 'It is tracked in the repository; restore it with:' >&2
  printf '%s\n' '  git checkout -- config/bank00.cfg' >&2
  exit 1
fi
if [ ! -f "$ROOT/snesrecomp/tools/v2_emit.py" ]; then
  printf '%s\n' 'Missing patched snesrecomp dependency.' >&2
  printf '%s\n' 'Initialize the submodule and apply its patch series first.' >&2
  exit 1
fi

actual_sha256=$(python3 -c \
  'import hashlib, pathlib, sys; print(hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest())' \
  "$ROM")
if [ "$actual_sha256" != "$EXPECTED_SHA256" ]; then
  printf 'Unsupported ROM SHA-256: %s\n' "$actual_sha256" >&2
  printf 'Expected: %s\n' "$EXPECTED_SHA256" >&2
  exit 1
fi

exec python3 "$ROOT/tools/generate-normal.py" "$ROM"
