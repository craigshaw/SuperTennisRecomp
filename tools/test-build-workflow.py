#!/usr/bin/env python3
"""ROM-free patch setup checks using disposable local dependency clones."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
SERIES = json.loads((ROOT / "patches/snesrecomp/series.json").read_text())
HELPER = ROOT / "tools/apply-snesrecomp-patches.py"


class PatchSetupTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="st-patch-test.")
        self.addCleanup(self.temporary.cleanup)
        self.dependency = Path(self.temporary.name) / "snesrecomp"
        self.command("git", "clone", "--shared", "--no-checkout",
                     str(ROOT / "snesrecomp"), str(self.dependency))

    def command(self, *args):
        return subprocess.run(args, check=True, capture_output=True, text=True)

    def git(self, *args):
        return self.command("git", "-C", str(self.dependency), *args)

    def checkout(self, crlf=False, current=False):
        if crlf:
            self.git("config", "core.autocrlf", "true")
        revision = SERIES["integrated_revision" if current else "previous_integrated_revision"]
        self.git("checkout", "--detach", revision)

    def test_current_pin_applies_only_pending_patches(self):
        self.checkout(current=True)
        pending = len(SERIES["patches"]) - SERIES["integrated_patch_count"]
        self.apply("--check", success=not pending)
        result = self.apply()
        if pending:
            self.assertIn(f"applied {pending} patches", result.stdout)
        before = self.git("diff", "--binary").stdout
        self.apply("--check")
        self.apply()
        self.assertEqual(self.git("diff", "--binary").stdout, before)
        self.assertEqual(self.git("rev-parse", "HEAD").stdout.strip(),
                         SERIES["integrated_revision"])

    def apply(self, *args, success=True):
        result = subprocess.run(
            [sys.executable, str(HELPER), "--dependency", str(self.dependency), *args],
            capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, success, result.stdout + result.stderr)
        return result

    def test_clean_and_repeated_application(self):
        self.checkout()
        before = self.git("status", "--porcelain").stdout
        self.apply("--check", success=False)
        self.assertEqual(self.git("status", "--porcelain").stdout, before)
        self.apply()
        before = self.git("diff", "--binary").stdout
        self.apply()
        self.apply("--check")
        self.assertEqual(self.git("diff", "--binary").stdout, before)
        self.assertEqual(self.git("rev-parse", "HEAD").stdout.strip(),
                         SERIES["previous_integrated_revision"])

    def test_dirty_checkout_is_preserved(self):
        self.checkout()
        path = self.dependency / "README.md"
        changed = path.read_bytes() + b"\nlocal contributor edit\n"
        path.write_bytes(changed)
        self.apply(success=False)
        self.assertEqual(path.read_bytes(), changed)
        self.assertFalse((self.dependency / "recompiler/v2/bus_timing.py").exists())

    def test_partial_series_is_preserved(self):
        self.checkout()
        self.git("apply", str(ROOT / "patches/snesrecomp" / SERIES["patches"][12]["file"]))
        before = self.git("status", "--porcelain").stdout
        self.apply(success=False)
        self.assertEqual(self.git("status", "--porcelain").stdout, before)

    def test_modified_patched_file_is_preserved(self):
        self.checkout()
        self.apply()
        path = self.dependency / "recompiler/v2/bus_timing.py"
        changed = path.read_bytes() + b"\n# local contributor edit\n"
        path.write_bytes(changed)
        self.apply(success=False)
        self.apply("--check", success=False)
        self.assertEqual(path.read_bytes(), changed)

    def test_crlf_checkout(self):
        self.checkout(crlf=True)
        self.apply()
        self.apply()
        self.apply("--check")

    def test_complete_series_from_reconstructed_base(self):
        self.checkout()
        # The public pin may be a shallow clone without its base commit.
        # Reverse the twelve published patches to reconstruct the base source.
        for patch in reversed(SERIES["patches"][:12]):
            self.git("apply", "-R", "--whitespace=nowarn",
                     str(ROOT / "patches/snesrecomp" / patch["file"]))
        for patch in SERIES["patches"]:
            self.git("apply", "--whitespace=nowarn",
                     str(ROOT / "patches/snesrecomp" / patch["file"]))
        self.apply("--check")


if __name__ == "__main__":
    unittest.main(verbosity=2)
