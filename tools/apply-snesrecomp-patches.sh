#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEPENDENCY="$ROOT/snesrecomp"
PATCH_DIRECTORY="$ROOT/patches/snesrecomp"
BASE_REVISION=a64932f1af958f7e71a728ac1235d6cf911f71a0
PATCHED_REVISION=3678d0a6d7036217f26e32f6b9087d2933783690

if [ ! -d "$DEPENDENCY/.git" ] && [ ! -f "$DEPENDENCY/.git" ]; then
  printf '%s\n' 'snesrecomp submodule is not initialized.' >&2
  printf '%s\n' 'Run: git submodule update --init' >&2
  exit 1
fi

current=$(git -C "$DEPENDENCY" rev-parse HEAD)
if [ "$current" = "$PATCHED_REVISION" ]; then
  printf '%s\n' 'snesrecomp patch series is already present.'
  exit 0
fi

if git -C "$DEPENDENCY" cat-file -e "$BASE_REVISION^{commit}" 2>/dev/null; then
  all_present=1
  for subject in \
    'Recognize direct long-call trampolines' \
    'Fix open-bus polling and quiescent resume' \
    'Prefer active interpreter ownership for mixed-tier returns' \
    'Decode dispatch helpers at observed entry widths' \
    'Add shared binary input replay reader' \
    'Retract stale exit facts after unresolved paths' \
    'Expose interpreted RTI completion hooks' \
    'Audit generated charges crossing event deadlines' \
    'Add audit-guided event precision path' \
    'Replay Mode 7 raster state in frame hosts' \
    'Preserve BG3 raster character addressing' \
    'Deliver delayed-enable NMI requests'
  do
    if ! git -C "$DEPENDENCY" log --format=%s "$BASE_REVISION"..HEAD |
        grep -Fqx "$subject"; then
      all_present=0
    fi
  done

  if [ "$all_present" -eq 1 ]; then
    printf '%s\n' 'snesrecomp patch series is already present.'
    exit 0
  fi
fi

if [ "$current" != "$BASE_REVISION" ]; then
  printf 'Unexpected snesrecomp revision: %s\n' "$current" >&2
  printf 'Expected public base revision: %s\n' "$BASE_REVISION" >&2
  exit 1
fi

if [ -n "$(git -C "$DEPENDENCY" status --porcelain)" ]; then
  printf '%s\n' 'Refusing to patch a dirty snesrecomp submodule.' >&2
  exit 1
fi

git -C "$DEPENDENCY" \
  -c user.name='st-recomp patch applicator' \
  -c user.email='st-recomp@invalid.local' \
  am --3way --keep-cr "$PATCH_DIRECTORY"/*.patch

printf '%s\n' 'Applied the twelve local snesrecomp patches.'
