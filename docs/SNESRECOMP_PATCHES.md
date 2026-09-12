# snesrecomp integration

This is the source of truth for the snesrecomp patches required by Super Tennis.

## Public pin and recovery patches

The submodule is pinned to the published integration commit
[`3a383fb8348140cd49371641e26086f81e1b2da3`](https://github.com/craigshaw/snesrecomp/commit/3a383fb8348140cd49371641e26086f81e1b2da3)
on `craigshaw/snesrecomp`, branch `codex/super-tennis-runtime`.
It contains all nineteen patches listed below. Normal setup verifies the
committed files and leaves the dependency working tree clean.

The previous public pin was `3678d0a6d7036217f26e32f6b9087d2933783690`,
which contained the first twelve patches. The upstream base was
`a64932f1af958f7e71a728ac1235d6cf911f71a0`. The exported series remains
available to reconstruct the same source from either older revision.

Patches 0013 through 0019 were published as seven ordered commits on
12 September 2026. Their final source matches the accepted patched source
exactly. The Python v2 and shared C suites passed before publication,
cfg-only output matched the accepted generated code, and the 4500-frame
title replay passed. Repinning changes dependency history and setup metadata;
it introduces no further runtime source changes.

## Ordered series

| Patch | Purpose |
| --- | --- |
| `0001-Recognize-direct-long-call-trampolines.patch` | Treat source-assembled long-call forms as calls with the correct return frame. |
| `0002-Fix-open-bus-polling-and-quiescent-resume.patch` | Correct RDNMI open-bus bits and resume parked polling loops at the hardware read. |
| `0003-Prefer-active-interpreter-ownership-for-mixed-tier-r.patch` | Give an active interpreter continuation priority over a coincident generated ancestor. |
| `0004-Decode-dispatch-helpers-at-observed-entry-widths.patch` | Classify dispatch helpers at observed M/X widths and retract disagreements. |
| `0005-Add-shared-binary-input-replay-reader.patch` | Validate and play the shared two-controller `.sri` replay format. |
| `0006-Retract-stale-exit-facts-after-unresolved-paths.patch` | Retract inferred exit facts after unresolved execution. |
| `0007-Expose-interpreted-RTI-completion-hooks.patch` | Expose exact interpreted RTI completion to event-driven hosts. |
| `0008-Audit-generated-charges-crossing-event-deadlines.patch` | Report generated clock charges that cross host event deadlines. |
| `0009-Add-audit-guided-event-precision-path.patch` | Route affected variants through an audit-guided interpreter precision path. |
| `0010-Replay-Mode-7-raster-state-in-frame-hosts.patch` | Replay scanline-timed display state from generated and emulated writes. |
| `0011-Preserve-BG3-raster-character-addressing.patch` | Preserve active-display BG34NBA changes in raster replay. |
| `0012-Deliver-delayed-enable-NMI-requests.patch` | Raise a pending NMI when software enables it during active vblank. |
| `0013-Add-ROM-free-timing-differential-tests.patch` | Add ROM-free generated/interpreter timing comparisons. |
| `0014-Preserve-folded-taken-branch-cycle.patch` | Preserve the extra CPU cycle for a constant-folded taken branch. |
| `0015-Correct-indexed-write-cycle-modifiers.patch` | Correct indexed store/RMW CPU-cycle modifiers in the interpreter and generator. |
| `0016-Add-opt-in-generated-bus-clock-accounting.patch` | Add opt-in generated bus-clock accounting and cache-key coverage. |
| `0017-Add-selected-native-leaf-instruction-timing.patch` | Add selected native leaf instruction timing with shared runtime completion. |
| `0018-Decouple-selected-leaf-from-global-bus-timing.patch` | Allow selected leaf timing with global bus timing disabled. |
| `0019-Extend-native-leaf-timing-and-sample-IRQ.patch` | Extend selected leaf timing to local branches and arithmetic; sample IRQ at instruction and return boundaries. |

All patches contain game-neutral implementation or synthetic tests. ROMs,
generated title code, profiles, recordings and private reports are excluded.

## Applying and verifying

On macOS or Linux:

```sh
git submodule update --init
sh tools/apply-snesrecomp-patches.sh
sh tools/apply-snesrecomp-patches.sh --check
```

On Windows, `build.ps1` and `tools/regenerate.ps1` verify the same series
using the selected Python interpreter. The underlying command is also
available directly on every platform:

```sh
python tools/apply-snesrecomp-patches.py
python tools/apply-snesrecomp-patches.py --check
```

The tool verifies the current public pin without applying patches. It also
accepts the previous public pin or upstream base for recovery, applying seven
or nineteen patches respectively to a clean checkout. It verifies patch
SHA-256 values and the contents of all 55 affected files against
[`series.json`](../patches/snesrecomp/series.json). Source-file comparisons
normalise CRLF line endings for Windows; patch files retain their exact bytes.
An already complete series is verified without writing. An incomplete series
on a modified checkout is refused, preserving local changes. Unrelated changes
are left alone when all required patched files already match.

`--check` never writes. Normal generation and `build.sh` use this check, so the
old public pin alone cannot silently stand in for the required fixes. Windows
also checks the dependency when `-SkipGenerate` is used. Use `--dependency PATH`
only when intentionally applying or verifying a separate checkout.

For recovery, preserve any local work before restoring the pinned checkout.
Do not reset or discard dependency changes just to make the applicator pass.

## Normal generation policy

The tracked cfg declares the accepted exact call entries. Both platform
regeneration scripts call `tools/generate-normal.py`, which enables cfg roots
and selects nineteen native leaves for instruction timing: the original
`$00:C3DE M1X0` and `$00:C7A0 M1X0`, plus the seventeen-entry leaf batch.
The normal output has 190 AOT bodies. No profile manifest is required. Global
bus timing remains disabled, and inherited experimental settings are cleared.

The shared helper keeps the existing event diagnostics available through
explicit `--event-crossing-audit` and `--event-precision-profile` options.
The title diagnostic scripts use these options. For other experiments, invoke
the snesrecomp emitter directly into a private output directory.

Instruction timing commits each opcode after its effects and runs the same
refresh, beam, coprocessor and APU completion work as the interpreter. Selected
bodies check deadlines, NMI and unmasked pending IRQ before each instruction
and after popping a return frame. Emulation-mode invocations use the
interpreter. Unselected bodies retain their existing generation policy.

Supported selected leaves have tested load/store operations, local branches,
CMP, ADC and accumulator ASL. Calls, external branch targets, indirect
addressing, stack manipulation, memory RMW, RTI and block moves remain outside
this mode. Unsupported selections fail generation.

## Evidence and limits

The subsequent seventeen-entry batch uses this same pinned runtime and adds
no patches. Two targeted replays match every recorded field across 6,625
frames and exercise sixteen additions; `$00:B82C M1X0` remains outside saved
replay coverage. The maintainer accepted gameplay, and cfg-only generation
reproduces all seven candidate C files and the full manifest. See
[AOT_COVERAGE.md](AOT_COVERAGE.md) for the current checkpoint and test policy.

The 173-body candidate passed all 18,534 checked frames across five private
replays and a neutral run against the corrected 172-body control. The added
C3DE body removed 1,250,136 interpreted calls in those comparisons. Repeated
bounded Mesen observations confirmed both exact native entry modes. A cfg-only
proposal reproduced the accepted profile-generated manifest and all six C
files. The maintainer reports correct gameplay with no noticeable change;
the captured session exited normally with no bailouts or capture loss.

Patch 0019 fixes a specific pending-IRQ gap exposed at frame 2893 of one replay.
The earlier selected body continued for 312 master clocks after the interpreter
would have stopped. Shared IRQ sampling and the return-boundary check restore
parity in all recorded comparisons. These results validate the selected bodies,
not every new profile candidate or whole-game hardware accuracy.

The original broader timing diagnostic remains a known failing investigation:

```sh
python3 snesrecomp/tests/timing/run.py --out-dir captures/timing-check
python3 snesrecomp/tests/timing/run.py --bus-timing --out-dir captures/timing-bus-check
```

Use fresh private output directories. The bus model fixes the fourteen initial
final-clock totals, but five write-timestamp comparisons still differ. Broader
byte-write ordering and generated block precharge remain limits. These are
separate from the passing bounded instruction-timing tests and do not justify
enabling global bus timing. See [AOT_COVERAGE.md](AOT_COVERAGE.md) for the wider
coverage workflow and evidence distinctions.

## Required checks

After changing generator/runtime source, run the relevant suites once at the
integration milestone. Generation-glue changes require the workflow check;
cfg-only batches use focused replay and reproducibility checks as described
in [AOT_COVERAGE.md](AOT_COVERAGE.md).

Available integration checks:

```sh
python3 tools/test-build-workflow.py
python3 snesrecomp/tests/v2/run_tests.py
bash snesrecomp/tests/run_c_tests.sh
sh tools/check-publication-boundary.sh
```

Regenerate and build the title, then apply the checks described in
[`AGENTS.md`](../AGENTS.md). Final promotion must compare fresh normal
cfg-generated output with the accepted candidate. An identical desktop binary
hash permits reuse of accepted gameplay results.
Record any platform that was not executed. Patch application tests cover clean
setup, repeat setup, modified/partial refusal and CRLF checkout behaviour.

## Upstreaming

All nineteen fixes are published on the project owner's integration branch
and included in the title pin. The exported patches remain the recovery and
review form until the required fixes are accepted upstream. For later updates:

1. Rebase each logical change against the intended upstream revision.
2. Preserve its synthetic tests and run the relevant Python, C and Rust suites.
3. Submit through normal upstream review and publish the resulting revision.
4. Pin this repository to a public commit containing all required fixes.
5. Rerun cfg-only generation and title regressions before removing superseded
   patches or changing the application tool.
