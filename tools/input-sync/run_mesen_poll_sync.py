#!/usr/bin/env python3
"""Run and verify the two-pass Mesen input-poll calibration."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from typing import Any

from build_poll_sync_rom import build_rom


PATTERN = (0x000, 0x001, 0x140, 0x008, 0x000, 0x820, 0x202, 0x000)


class ProofError(RuntimeError):
    pass


def lua_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def render_launcher(adapter: Path, output: Path, masks: tuple[int, ...]) -> str:
    rendered_masks = ", ".join(f"0x{mask:03X}" for mask in masks)
    loader_error = output.with_name("loader-error.txt")
    return (
        "INPUT_SYNC_CONFIG = {\n"
        f"  output_path = {lua_string(str(output))},\n"
        "  max_frames = 120,\n"
        f"  masks = {{ {rendered_masks} }},\n"
        "}\n"
        f"local adapter, load_error = loadfile({lua_string(str(adapter))})\n"
        "if adapter == nil then\n"
        f"  local error_file = io.open({lua_string(str(loader_error))}, \"w\")\n"
        "  if error_file ~= nil then\n"
        "    error_file:write(tostring(load_error), \"\\n\")\n"
        "    error_file:close()\n"
        "  end\n"
        "  emu.stop(65)\n"
        "else\n"
        "  adapter()\n"
        "end\n"
    )


def reverse_joypad_word(mask: int) -> int:
    word = 0
    state = mask & 0x0FFF
    for _ in range(16):
        word = (word << 1) | (state & 1)
        state >>= 1
    return word


def read_records(path: Path) -> list[dict[str, Any]]:
    if not path.is_file():
        raise ProofError(f"Mesen did not create {path}")
    records: list[dict[str, Any]] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        try:
            record = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ProofError(f"{path.name}:{line_number}: invalid JSON: {exc}") from exc
        if not isinstance(record, dict):
            raise ProofError(f"{path.name}:{line_number}: record is not an object")
        records.append(record)
    return records


def validate_run(
    records: list[dict[str, Any]], expected_masks: tuple[int, ...], label: str
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    if not records or records[0].get("record_type") != "start":
        raise ProofError(f"{label}: missing start record")
    errors = [record for record in records if record.get("record_type") == "error"]
    if errors:
        raise ProofError(f"{label}: Mesen adapter failed: {errors[0].get('detail')}")

    polls = [record for record in records if record.get("record_type") == "poll"]
    complete = [record for record in records if record.get("record_type") == "complete"]
    if len(complete) != 1 or records[-1] is not complete[0]:
        raise ProofError(f"{label}: missing final completion record")
    if len(polls) != len(expected_masks):
        raise ProofError(
            f"{label}: expected {len(expected_masks)} polls, observed {len(polls)}"
        )

    for index, (poll, expected_mask) in enumerate(zip(polls, expected_masks)):
        if poll.get("poll") != index:
            raise ProofError(f"{label}: expected poll index {index}, got {poll.get('poll')}")
        if poll.get("supplied_mask") != expected_mask:
            raise ProofError(
                f"{label}: poll {index} supplied {poll.get('supplied_mask')}, "
                f"expected {expected_mask}"
            )
        if poll.get("observed_mask") != expected_mask:
            raise ProofError(
                f"{label}: poll {index} getInput returned "
                f"{poll.get('observed_mask')}, expected {expected_mask}"
            )
        for key in ("frame", "master_cycle"):
            if not isinstance(poll.get(key), int) or poll[key] < 0:
                raise ProofError(f"{label}: poll {index} has invalid {key}")

    completion = complete[0]
    expected_words = [reverse_joypad_word(mask) for mask in expected_masks]
    if completion.get("poll_count") != len(expected_masks):
        raise ProofError(f"{label}: completion poll count is incorrect")
    if completion.get("guest_sample_count") != len(expected_masks):
        raise ProofError(f"{label}: guest sample count is incorrect")
    if completion.get("guest_words") != expected_words:
        raise ProofError(
            f"{label}: guest joypad words {completion.get('guest_words')} "
            f"do not match {expected_words}"
        )
    return polls, completion


def run_mesen(
    mesen: Path, launcher: Path, rom: Path, run_dir: Path
) -> subprocess.CompletedProcess[str]:
    command = [
        str(mesen),
        "--testRunner",
        "--enableStdout",
        "--doNotSaveSettings",
        "--debug.scriptWindow.allowIoOsAccess=true",
        "--emulation.runAheadFrames=0",
        "--snes.enableRandomPowerOnState=false",
        "--snes.ramPowerOnState=AllZeros",
        "--snes.port1.type=SnesController",
        "--snes.port2.type=SnesController",
        "--timeout=10",
        str(launcher),
        str(rom),
    ]
    try:
        return subprocess.run(
            command,
            cwd=run_dir,
            check=False,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=15,
        )
    except subprocess.TimeoutExpired as exc:
        raise ProofError("Mesen exceeded the 15-second host timeout") from exc


def run_pass(
    mesen: Path,
    adapter: Path,
    rom: Path,
    root: Path,
    label: str,
    masks: tuple[int, ...],
) -> tuple[list[dict[str, Any]], dict[str, Any], Path]:
    run_dir = root / label
    run_dir.mkdir()
    log = run_dir / "polls.jsonl"
    launcher = run_dir / "launcher.lua"
    launcher.write_text(render_launcher(adapter, log, masks), encoding="utf-8")
    result = run_mesen(mesen, launcher, rom, run_dir)
    (run_dir / "stdout.txt").write_text(result.stdout, encoding="utf-8")
    (run_dir / "stderr.txt").write_text(result.stderr, encoding="utf-8")
    if result.returncode != 0:
        loader_error = run_dir / "loader-error.txt"
        detail = (
            loader_error.read_text(encoding="utf-8").strip()
            if loader_error.is_file()
            else f"see {run_dir / 'stderr.txt'}"
        )
        raise ProofError(
            f"{label}: Mesen exited with {result.returncode}; {detail}"
        )
    polls, completion = validate_run(read_records(log), masks, label)
    return polls, completion, log


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mesen",
        type=Path,
        default=Path(os.environ["MESEN"]) if "MESEN" in os.environ else None,
        help="path to the MesenCE 2.2 executable, or set MESEN",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        help="parent for the private run directory; defaults to build/input-sync",
    )
    args = parser.parse_args(argv)
    if args.mesen is None:
        parser.error("pass --mesen or set MESEN")
    mesen = args.mesen.expanduser().resolve()
    if not mesen.is_file():
        parser.error(f"Mesen executable does not exist: {mesen}")

    tool_dir = Path(__file__).resolve().parent
    project_root = tool_dir.parent.parent
    output_parent = (
        args.output_root.expanduser().resolve()
        if args.output_root
        else project_root / "build" / "input-sync"
    )
    output_parent.mkdir(parents=True, exist_ok=True)
    run_root = Path(tempfile.mkdtemp(prefix="proof-", dir=output_parent))
    adapter = tool_dir / "mesen_poll_sync.lua"
    rom = run_root / "poll-sync-test.sfc"
    rom_bytes = build_rom()
    rom.write_bytes(rom_bytes)

    try:
        first_polls, first_completion, first_log = run_pass(
            mesen, adapter, rom, run_root, "record", PATTERN
        )
        recorded_masks = tuple(int(poll["observed_mask"]) for poll in first_polls)
        second_polls, second_completion, second_log = run_pass(
            mesen, adapter, rom, run_root, "replay", recorded_masks
        )

        first_anchors = [
            (poll["poll"], poll["frame"], poll["master_cycle"])
            for poll in first_polls
        ]
        second_anchors = [
            (poll["poll"], poll["frame"], poll["master_cycle"])
            for poll in second_polls
        ]
        if second_anchors != first_anchors:
            raise ProofError(
                "replay: poll frame/master-clock anchors changed between identical runs"
            )
        if second_completion != first_completion:
            raise ProofError("replay: completion and guest WRAM results changed")
    except ProofError as exc:
        print(f"input-poll synchronization proof: FAIL: {exc}", file=sys.stderr)
        print(f"private artifacts: {run_root}", file=sys.stderr)
        return 1

    digest = hashlib.sha256(rom_bytes).hexdigest()
    print("input-poll synchronization proof: PASS")
    print(f"synthetic ROM sha256: {digest}")
    print(f"polls: {len(PATTERN)}; guest samples: {first_completion['guest_sample_count']}")
    print(f"record log: {first_log}")
    print(f"replay log: {second_log}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
