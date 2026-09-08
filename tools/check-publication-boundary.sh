#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

git diff --check -- . ':(exclude)patches/snesrecomp/*.patch'

for required_document in \
  README.md LICENSE AGENTS.md \
  docs/ARCHITECTURE.md docs/SNESRECOMP_PATCHES.md \
  tools/input-sync/README.md
do
  if [ ! -f "$required_document" ]; then
    printf 'Missing required publication document: %s\n' "$required_document" >&2
    exit 1
  fi
done

for script in tools/*.sh; do
  sh -n "$script"
done

failed=0
for forbidden in \
  'build/*' 'generated/*' 'reference/*' \
  '*.sfc' '*.smc' '*.zip' '*.srm' '*.state' '*.jsonl' '.DS_Store'
do
  if git ls-files -- "$forbidden" | grep -q .; then
    printf 'Tracked files match forbidden pattern: %s\n' "$forbidden" >&2
    failed=1
  fi
done

large_files=$(git ls-files -s |
  awk '$1 != "160000" {print $2, $4}' |
  git cat-file --batch-check='%(objectsize) %(rest)' |
  awk '$1 > 1048576')
if [ -n "$large_files" ]; then
  printf '%s\n' 'Tracked files exceeding 1 MiB:' "$large_files" >&2
  failed=1
fi

if git grep -n -E '/Users/[^/]+/|[A-Za-z]:\\\\Users\\\\' -- \
    ':!patches/snesrecomp/*.patch' \
    ':!tools/check-publication-boundary.sh'; then
  printf '%s\n' 'Tracked text contains a local user path.' >&2
  failed=1
fi

if [ "$failed" -ne 0 ]; then
  exit 1
fi

printf '%s\n' 'Publication boundary check passed.'
