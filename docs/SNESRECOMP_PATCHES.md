# snesrecomp integration

This is the source of truth for the snesrecomp patches required by Super Tennis.

## Public pin and recovery patches

The submodule is pinned to integration commit
[`9f9462528f21faa42ec3a7296df3d07c34db9fb3`](https://github.com/craigshaw/snesrecomp/commit/9f9462528f21faa42ec3a7296df3d07c34db9fb3)
on `craigshaw/snesrecomp`, branch `codex/super-tennis-runtime`.
It contains patches 0001 through 0036. All are published and retained under
`patches/snesrecomp/` for recovery. Patches 0023 through 0026 add exact-entry exit-width
declarations, interpreted selection and direct call/tail instruction timing. Patch 0024 also corrects the resume bank after a deadline
unwind through compiled JSL calls. Patch 0025 separates exit analysis from
AOT selection with `interpret_only`. Patch 0026 adds direct JML instruction
timing. Patch 0027 enables tested CPX and INX instruction timing. Patch 0028
adds balanced byte stacks and prevents a duplicate beam tick on instruction-timed HVBJOY reads. Normal setup applies pending patches and
verifies the complete source.

Patch 0022 extends selected native leaf instruction timing. Patch 0021
preserves timing headers when generated banks are split. Patch 0020 adds the
exact-entry bus-cost selector to the previous nineteen-patch pin
`3a383fb8348140cd49371641e26086f81e1b2da3`.

The older recovery pin is `3678d0a6d7036217f26e32f6b9087d2933783690`,
which contained the first twelve patches. The upstream base was
`a64932f1af958f7e71a728ac1235d6cf911f71a0`. The exported series remains
available to reconstruct the same source from either older revision.

Patches 0013 through 0019 were published as seven ordered commits on
12 September 2026. Their final source matches the accepted patched source
exactly. The Python v2 and shared C suites passed before publication,
cfg-only output matched the accepted generated code, and the 4500-frame
title replay passed. Repinning changes dependency history and setup metadata;
it introduces no further runtime source changes.

Patch 0031 adds opt-in scheduler continuations for existing instruction-timed
blocks. It keeps continuation lookup separate from subroutine dispatch and
analysis roots. The title selects four balanced loop headers in `01E5DF`,
`01E72F`, `01EBAE` and `01ECCB`; its normal analyzer manifest is unchanged.
The original shared change passed the Python v2 suite, shared C suite and
focused continuation/cache tests. The later selection-only change reuses
those checks and passes both title replay comparisons.
See [AOT_RECOVERY.md](AOT_RECOVERY.md) for measured work and remaining limits.

Patch 0032 extends continuations to statically proven local stack depths.
Saved registers remain on the guest stack; generated entries reconstruct the
owning frame's entry S. Local absolute JMP stays within the validated CFG;
external JMP tails remain excluded from continuation bodies. The title adds
instruction timing for `00CFE8:M1X0` and the internal `00CFFB:M1X0` entry at
depth two. Normal generation still emits 345 bodies, with five continuations.
The new synthetic suite passes 104 complete and 704 event comparisons,
including 313 native entries, interpreted starts, nested saves, calls and
RTS/RTL. Python v2, shared C, both title inputs and normal generation pass.

Patch 0033 enables direct-page ORA at both M widths in selected instruction
timing, using the existing opcode and continuation implementation. Its tests
pass 100 complete and 240 event comparisons, including 92 native entries.
The title selects `00D0DE:M1X0` and `00D0F1:M1X0` at depth two, for 345
bodies and six continuations. Both saved title inputs pass; the normal
Python analysis manifest remains byte identical. This adds no cfg contract.

Patch 0034 allows selected continuations in bodies with a known interpreted
JML tail. The exact target must have no emitted variant. Local saves remain
on the guest stack, and the existing interpreter-owner handoff handles the
transfer. Calls and returns still require zero local depth; compiled and
unresolved tails remain excluded from continuation bodies. No runtime source
changes or exit-state declarations are added. The title selects `00CE3F:M1X0`
and its `00CE52:M1X0` loop at depth two, keeping `00CE51:M1X0` interpreted.
The synthetic suite passes 24 complete and 576 event comparisons, with 562
native entries. The build has 345 bodies and seven continuations.

Patch 0035 adds pre-port APU flushes and saves/restores the pacing scope for
instruction-timed bus access. Contiguous data words retain atomic MMIO access;
long-indirect data words carry across banks while their direct-page pointers
wrap within bank zero. The compiler admits PHP, terminal PLP, idempotent X-bit
updates, M0/X0 LDA [dp],Y and M1 accumulator ROL. Actual index-width changes
and nonterminal PLP remain rejected. Validated instruction-timed polls can run
natively; aggregate-timed polls retain their scheduler guard. Terminal PLP
blocks still unwind to the interpreter so their progress history and status
restore remain available to a following cooperative wait.

The title selects `07D8A5:M1X0` and internal `07D8CA:M1X0` at depth one.
The final `07D90F:M1X0` block remains interpreted. The long input passes all
4,816 saved frame comparisons and removes 1,841,899 interpreted instructions.
The new synthetic checks cover 60 complete and 1,036 event comparisons.
The build retains 345 bodies, with 42 timing selections and eight continuations.

Patch 0036 permits unnamed internal JML back-edges in selected continuation
bodies. The destination must be a decoded exact M/X instruction in the same
bank, before the jump and separate from the root, with zero local stack depth
at both ends and no compiled exact entry. Emission uses the existing owning
interpreter handoff rather than abandoning the unresolved transfer. It adds
no runtime mechanism, cfg function or exit-state declaration.

The title selects existing `00801E:M1X0` for instruction timing and resumes
at `008035:M1X0`. The `008031` call and `00804D` remain interpreted. The
expanded tail suite passes 32 complete and 768 event comparisons, including
860 native entries. The long title input removes 2,081,076 interpreted
instructions with all 4,816 frames equal. The normal analyzer manifest stays
unchanged: 345 bodies, 43 timing selections and nine continuations.

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
| `0020-Select-bus-clock-accounting-by-exact-entry.patch` | Scope existing bus-cost generation to exact entries, with cache invalidation and selection tests. |
| `0021-Preserve-timing-header-in-split-bank-sources.patch` | Include the existing timing declarations in every generated bank shard. |
| `0022-Extend-selected-leaf-status-and-ALU-timing.patch` | Add tested logical, accumulator, register and accumulator-width operations to selected leaf timing; keep index-width changes excluded. |
| `0023-Declare-callee-exits-by-exact-entry-width.patch` | Add `exit_mx_for`, scoped to entry M/X, with Python/native analysis, refresh and cache checks. |
| `0024-Time-direct-calls-at-instruction-boundaries.patch` | Time ordinary direct calls and immediate ORA, transfer missing or disabled callees to the owning interpreter, and preserve the resume bank across deadline unwinds. |
| `0025-Separate-exit-analysis-from-AOT-selection.patch` | Infer normal exact exits while keeping selected dependencies interpreted; preserve unresolved-path checks and reject unsupported legacy emission. |
| `0026-Time-direct-tail-transfers-at-instruction-boundaries.patch` | Commit direct JML clocks and PB, then check events at the destination before either tier executes. |
| `0027-Time-selected-index-compare-and-increment.patch` | Enable CPX and INX with real-interpreter flag, width, clock and event/resume comparisons. |
| `0028-Time-byte-stacks-and-instruction-timed-HVBJOY.patch` | Add balanced M1 PHA/PLA and DEX timing; scope reads so HVBJOY does not add a second beam advance. |
| `0029-Time-selected-subtract-and-byte-memory-operations.patch` | Enable SEC, selected SBC forms and byte direct-page INC/DEC/ASL/ROL with interpreter and event/resume comparisons. |
| `0030-Time-selected-word-shifts-and-local-X-saves.patch` | Add selected word ROL/LSR/SBC, accumulator ROR and local X0 PHX/PLX, with high-byte-first memory and stack writes. |
| `0031-Resume-selected-timed-blocks-through-the-owning-scheduler.patch` | Add exact internal block continuations with stack and IR checks, real guest returns, scheduler ownership and event/resume tests. |
| `0032-Resume-timed-table-scans-with-proven-guest-stack-saves.patch` | Resume at proven local stack depths; add tested table-read, word-save, word DEC, transfer and local JMP instruction timing. |
| `0033-Time-direct-page-ORA-in-selected-native-bodies.patch` | Enable tested byte and word direct-page ORA timing, including event recovery through saved-stack continuations. |
| `0034-Allow-continuation-handoffs-to-known-interpreted-tails.patch` | Validate known interpreted JML handoffs with preserved local saves and existing scheduler ownership. |
| `0035-Time-AOT-port-polls-with-bounded-status-epilogues.patch` | Match timed APU bus pacing, allow validated polls, and retain terminal status blocks in the interpreter. |
| `0036-Allow-proven-internal-JML-continuation-backedges.patch` | Hand off unnamed internal back-edges with exact widths and zero local stack depth to the existing interpreter owner. |

Patches 0023 through 0026 are published and included in the title pin.
The exact `02A3E2:M1X0` contract recovers four callers while keeping the callee
interpreted. The normal generated C matches the passing private replay candidate.
The Python v2 suite passes 402 tests, the shared C suite passes, and the
22 native cfg tests pass. Normal generation includes the four validated hot
roots at the previous 337-body checkpoint. The native release analyzer was
rebuilt for those parity checks. Patch 0027 adds one further validated body,
`00CE11:M1X0`, for 338. Its final Python and shared C suites pass.
See [AOT_RECOVERY.md](AOT_RECOVERY.md) for scope and evidence.

Patch 0022 is included in the title pin and retained as an exported patch. It changes
the generator's supported instruction selection and adds synthetic tests;
unselected generated output stays unchanged. The recovery candidate and its
current acceptance state are recorded in [AOT_RECOVERY.md](AOT_RECOVERY.md).

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

The tool verifies the complete source at the current pin without applying
patches. At the older recovery pin or upstream base it applies twenty-four or
thirty-six patches respectively to a clean checkout. It verifies patch SHA-256
values and the contents of all 77 affected files against
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
and selects 43 exact entries for instruction timing. It also selects nine
validated internal scheduler continuations, separate from function roots.
The normal output has 345 AOT bodies. The bus-cost selection contains 170
exact keys, of which 136 emit bodies. Instruction timing is selected separately.
The other 34 remain interpreted under the validated cfg exclusions and exit
proofs. Preserve this selection with the cfg; see [the recovery report](AOT_RECOVERY.md).
No profile manifest is required. Global bus timing remains disabled, and
inherited experimental settings are cleared.

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
CMP, ADC and accumulator ASL. Patch 0024 adds ordinary direct JSR/JSL and
immediate ORA. Unknown callee exits still block compiled continuations. External
branch targets, indirect or special calls, unselected indirect addressing, other word stacks,
unbalanced local stacks, other stack operations, unselected memory RMW, RTI and block moves remain outside this mode.
Unsupported selections fail generation. The normal selection uses 43 validated
exact entries, including the arithmetic-loop and word-operation entries below. Direct JML, CPX and INX are
also supported by patches 0026 and 0027. Patch 0028 supports PHA/PLA with M=1
and DEX. Stack depth must agree at joins and be zero at calls, compiled tails and
returns. Patch 0034 permits a proven local depth at known interpreted JML
tails in selected continuation bodies.
Patch 0029 adds SEC, SBC immediate/absolute, M1 direct-page SBC and M1
direct-page INC/DEC/ASL/ROL. Other word memory RMW forms remain excluded. Its 134 complete
and 198 event/resume comparisons cover carry, overflow, decimal subtraction,
byte wraps, direct-page penalties, bus writes, width changes and local stacks.
The validated title selection adds `01EBAE`, `01ECCB` and `01EF24`, all M1X0,
for 342 bodies. See the recovery report for measured work and performance limits.
Patch 0030 additionally supports M0 direct-page ROL/LSR/SBC, M0 accumulator
ROR and X0 PHX/PLX. Selected word memory shifts and X saves write high byte
first, matching the interpreter; ordinary stores remain unchanged. Stack
validation counts bytes and retains the zero-depth transfer rule. Its 100
complete and 198 event/resume comparisons support three further validated
entries, `01E5DF`, `01E72F` and `01EE3E`, all M1X0, for 345 bodies.

Patch 0032 adds word PHA/PLA, X0 PHY/PLY, M0X0 TXA/TAX/TAY, INY, M0
accumulator LSR and direct-page DEC, M0 ORA abs,Y, M1X0 LDA/SBC [dp],Y,
and local absolute JMP. Word saves and DEC write high byte first. Long
indirect pointers are evaluated once in instruction timing, including when
aggregate bus timing is disabled. The new word ORA path carries its high
byte across a bank boundary and counts large-index page crossings. Other
word-read modes retain their existing behavior and are outside these new
boundary claims. Stack joins must agree; calls, tails and returns still
require zero local depth. No title address is embedded in the shared code.

## Evidence and limits

The 191-body candidate retains block timing for the added multiplication
routine. Corrected bus costs restore exact agreement with the 190-body control
across the selected 2,125-frame replay, removing 8,367 call gaps. The maintainer
also accepted gameplay. Patch 0020 reproduces that existing bus-cost output
without changing runtime code or adding instruction-timed opcode support.
Temporary differences within calls remain, so this is bounded title evidence.

The earlier seventeen-entry batch uses this same pinned runtime and adds
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

Patches 0001 through 0022 are included in the title pin on the project owner's
integration branch. The exported patches remain the recovery and
review form until the required fixes are accepted upstream. For later updates:

1. Rebase each logical change against the intended upstream revision.
2. Preserve its synthetic tests and run the relevant Python, C and Rust suites.
3. Submit through normal upstream review and publish the resulting revision.
4. Pin this repository to a public commit containing all required fixes.
5. Rerun cfg-only generation and title regressions before removing superseded
   patches or changing the application tool.
