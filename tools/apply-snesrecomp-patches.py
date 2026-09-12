#!/usr/bin/env python3
"""Apply or verify the tracked patch series without creating local commits."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
PATCH_DIRECTORY = ROOT / "patches" / "snesrecomp"


def git(dependency, *args):
    return subprocess.check_output(
        ["git", "-C", str(dependency), *args], stderr=subprocess.PIPE
    ).decode().strip()


def text_digest(path):
    # Git for Windows may check text out with CRLF. The patches and their
    # postimages describe LF text; line endings do not change this contract.
    return hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest()


def verify(dependency, check_only=False):
    series = json.loads((PATCH_DIRECTORY / "series.json").read_text())
    for patch in series["patches"]:
        digest = hashlib.sha256((PATCH_DIRECTORY / patch["file"]).read_bytes()).hexdigest()
        if digest != patch["sha256"]:
            raise ValueError(f"Patch integrity check failed: {patch['file']}")

    revision = git(dependency, "rev-parse", "HEAD")
    applied_counts = {
        series["base_revision"]: 0,
        series["previous_integrated_revision"]: series["previous_integrated_patch_count"],
        series["integrated_revision"]: series["integrated_patch_count"],
    }
    if revision not in applied_counts:
        raise ValueError(f"Unexpected snesrecomp revision: {revision}")

    def mismatches():
        return [name for name, digest in series["patched_files"].items()
                if not (dependency / name).is_file()
                or text_digest(dependency / name) != digest]

    missing = mismatches()
    if not missing:
        print("snesrecomp: all 19 patches verified.")
        return
    if check_only or revision == series["integrated_revision"]:
        raise ValueError(
            "snesrecomp does not match the required patch series. "
            "Run tools/apply-snesrecomp-patches.sh (or the Python equivalent). "
            "First differing file: " + missing[0])
    if git(dependency, "status", "--porcelain", "--untracked-files=all"):
        raise ValueError(
            "Refusing to patch a modified or partly patched snesrecomp checkout. "
            "Preserve your changes before restoring the pinned dependency. "
            "First differing file: " + missing[0])

    start = applied_counts[revision]
    for patch in series["patches"][start:]:
        subprocess.run(
            ["git", "-C", str(dependency), "apply", "--whitespace=nowarn",
             str(PATCH_DIRECTORY / patch["file"])], check=True)
    missing = mismatches()
    if missing:
        raise ValueError("Patched source verification failed: " + missing[0])
    print(f"snesrecomp: applied {len(series['patches']) - start} patches; "
          "all 19 patches verified. The public submodule pin is unchanged.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="verify without writing")
    parser.add_argument("--dependency", type=Path, default=ROOT / "snesrecomp",
                        help="dependency checkout to apply or verify")
    args = parser.parse_args()
    try:
        verify(args.dependency.resolve(), args.check)
    except (ValueError, OSError, subprocess.CalledProcessError) as exc:
        print(f"snesrecomp: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
