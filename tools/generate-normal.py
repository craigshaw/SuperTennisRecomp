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
    # Preserve the validated bulk selection, including 47 keys kept in LLE.
    # Of 166 selected keys, 119 emit bodies. Global bus timing stays disabled.
    # See docs/AOT_BATCH_SCREENING.md before changing selection or cfg exclusions.
    env["SNESRECOMP_EMIT_BUS_TIMING_TARGETS"] = ",".join((
        "019B33:1:0", "00B2C6:1:0", "00B2DC:1:0", "00B8D9:1:0", "00BF1C:1:0",
        "00C0CB:1:0", "00C4B7:1:0", "00CB56:1:0", "00CE3F:1:0", "00CFE8:1:0",
        "00D0DE:1:0", "00D271:1:0", "00D285:1:0", "00D2A0:1:0", "00D354:1:0",
        "00D368:1:0", "00D4B7:1:0", "00D851:1:0", "00D94B:1:0", "00DA9B:1:0",
        "00DAE4:1:0", "00DB7E:1:0", "00DBBD:1:0", "00DC5A:1:0", "00DCB3:1:0",
        "00DCDC:1:0", "00E496:1:0", "00E49F:1:0", "00E82E:1:0", "00E92A:1:0",
        "00E98B:1:0", "00E9D1:1:0", "00EA14:1:0", "00EA7E:1:0", "00EB1D:1:0",
        "00EB92:1:0", "00EBC2:1:0", "00EEDA:1:0", "00EF63:1:0", "00F14E:1:0",
        "00F194:1:0", "00F1F9:1:0", "00F24B:1:0", "00F276:1:0", "00F408:1:0",
        "00F43C:1:0", "00F4AB:1:0", "00F67A:1:0", "00F68B:1:0", "00F6DA:1:0",
        "00F757:1:0", "00F76F:1:0", "018114:1:0", "0181A0:1:0", "0181C8:1:0",
        "0181EB:1:0", "0181EF:1:0", "0181F3:1:0", "0181F5:1:0", "01825F:1:0",
        "0182FF:1:0", "018318:1:0", "01833D:1:0", "01836D:1:0", "01839D:1:0",
        "0183CD:1:0", "0184FC:1:0", "018712:0:0", "01873C:0:0", "0187BA:1:0",
        "0187E5:1:0", "0188EF:1:0", "018989:1:0", "018F76:1:0", "019023:1:0",
        "019037:1:0", "019067:1:0", "01911F:1:0", "0198D8:1:0", "0198E4:1:0",
        "0199EB:1:0", "019A62:1:0", "019A68:1:0", "019A85:1:0", "019AAB:1:0",
        "019AB1:1:0", "019AC7:1:0", "019AEE:1:0", "01A372:1:0", "01A7EA:1:0",
        "01A84B:1:0", "01AF47:1:0", "01AFEF:1:0", "01B1B6:1:0", "01B37D:1:0",
        "01B3D7:1:0", "01B3E8:1:0", "01B3F9:1:0", "01B40A:1:0", "01B7C6:1:0",
        "01B825:1:0", "01BDFE:1:0", "01BF0C:1:0", "01BF16:1:0", "01C00A:1:0",
        "01C6D5:1:0", "01C779:1:0", "01C7FE:1:0", "01CEB8:1:0", "01D114:1:0",
        "0287EC:1:0", "0287F2:1:0", "028804:1:0", "028821:1:0", "028919:1:0",
        "0290CB:1:0", "02A2F5:1:0", "02A418:1:0", "02A447:1:0", "02A4AC:1:0",
        "02AD46:1:0", "02AD56:1:0", "02AD5E:1:0", "02AD66:1:0", "02AED9:1:0",
        "02AEF7:1:0", "02AF10:1:0", "02B022:1:0", "02B037:1:0", "02B04C:1:0",
        "02B066:1:0", "02B080:1:0", "02B0A8:1:0", "02B11D:1:0", "02B3C3:1:0",
        "02B42B:1:0", "02B434:1:0", "02B441:1:0", "02B534:0:0", "02B5BF:1:0",
        "02B5F4:1:0", "02E572:1:0", "02E5BD:1:0", "02E5CA:1:0", "02E60E:1:0",
        "02E964:1:0", "02EA3A:1:0", "02EA78:1:0", "02EAE0:1:0", "02EB0E:1:0",
        "02EB5E:1:0", "02EB9C:1:0", "02EBF5:1:0", "02EC00:1:0", "02ED04:1:0",
        "02ED7B:1:0", "038000:1:0", "03802A:1:0", "0380E1:1:0", "0380F2:1:0",
        "07CCCD:1:0", "07CDB1:1:0", "07D3A5:1:0", "07D3CD:1:0", "07D413:1:0",
        "07D510:1:0",
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
