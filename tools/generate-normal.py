#!/usr/bin/env python3
"""Shared, profile-free generation policy for the supported title build."""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--event-crossing-audit", action="store_true")
    parser.add_argument("--event-precision-profile", type=Path)
    args = parser.parse_args()
    expected = "6e45a80ea148654514cb4e8604a0ffcbc726946e70f9e0b9860e36c0f3fa4877"
    if hashlib.sha256(args.rom.read_bytes()).hexdigest() != expected:
        parser.error("unsupported ROM SHA-256")
    subprocess.run([sys.executable, str(ROOT / "tools/apply-snesrecomp-patches.py"),
                    "--check"], check=True)
    env = os.environ.copy()
    # Normal builds have one reviewed policy. Use the emitter directly in
    # a private output directory for experiments with other generation flags.
    for key in list(env):
        if key.startswith("SNESRECOMP_EMIT_") or key in (
                "SNESRECOMP_ROOT", "SNESRECOMP_EVENT_PRECISION_PROFILE"):
            del env[key]
    env["SNESRECOMP_EMIT_INSTRUCTION_TIMING"] = ",".join((
        "00C7A0:1:0", "00C3DE:1:0",
        "00B5BE:1:0", "00B82C:1:0", "00B8F0:1:0", "00B92D:1:0",
        "00BBD1:1:0", "00BD1B:1:0", "00C137:1:0", "00C3AF:1:0",
        "00DA64:1:0", "00DC7F:1:0", "01A40E:1:0", "01B506:1:0",
        "01D0C1:1:0", "02B15C:1:0", "02B2C5:1:0", "02B4AB:0:0",
        "0381D8:1:0",
    ))
    if args.event_crossing_audit:
        env["SNESRECOMP_EMIT_EVENT_CROSSING_AUDIT"] = "1"
    if args.event_precision_profile:
        env["SNESRECOMP_EVENT_PRECISION_PROFILE"] = str(args.event_precision_profile.resolve())
    return subprocess.call(
        [sys.executable, str(ROOT / "snesrecomp/tools/v2_emit.py"),
         "--rom", str(args.rom), "--cfg-dir", str(ROOT / "config"),
         "--out-dir", str(ROOT / "generated"), "--source-root", str(ROOT / "src"),
         "--analysis-backend", "python", "--cfg-roots"], env=env)


if __name__ == "__main__":
    sys.exit(main())
