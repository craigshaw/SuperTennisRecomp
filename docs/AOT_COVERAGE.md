# AOT coverage workflow

Accepted AOT coverage must be reproducible from tracked analysis inputs,
pinned tools, and the contributor's verified ROM. A private gameplay profile
helps discover those inputs; it must not become an undocumented requirement
for the normal build.

## Discover and preserve evidence

1. Preserve the baseline revision, dependency revisions, generation options,
   analysis manifest, and executable hashes in an ignored experiment folder.
2. Capture representative gameplay with `SNESRECOMP_TIER2_CAPTURE=1`. Use a
   fresh private directory for each run, explicit absolute manifest and
   journal paths, and a diagnostic log. Exit normally to write final counts.
3. Check capture completeness, including overflow and journal write failures.
   Audit the profile with `snesrecomp/tools/tier2_ingest.py` and rank exact
   `(pc24, M, X)` targets. Preserve the raw evidence unchanged.

Tier-2 capture records execution gaps, not controller inputs. The desktop
host currently supports `.sri` playback but has no input-recording option.
Use the lab's shared recorder when the exact session must be replayable.
Existing title replays can test an experiment, but they do not reproduce a
new manual match unless its inputs were recorded.

Hit counts measure frequency, not CPU time. For `call_gap` and `goto_gap`,
`clean_hits` counts sightings rather than independently verified complete
returns. A zero bailout count does not establish a safe function contract.

Check timing as well as successful execution. The current generated path
charges static blocks at code-region speed; the interpreter accounts for bus
transfers and internal cycles separately. New AOT bodies can therefore change
event timing without a crash or tier-2 bailout. Compare frame output and state,
record the first clock difference, and distinguish a frame-boundary resume PC
from the instruction that caused the difference. An event-crossing audit does
not detect every difference between the two timing models.

The ROM-free timing tests described in
[SNESRECOMP_PATCHES.md](SNESRECOMP_PATCHES.md#evidence-and-limits)
compare actual generated C with the interpreter bridge. They check SlowROM,
FastROM, operand width, WRAM/MMIO stores, branches, and indexed page crossing.
The initial 14 cases agree on checked registers and write values, but all
differ in timing. The folded taken branch also lost one CPU cycle; pending
patch 0014 restores that cycle and adds 96 executable branch cases. Keep this
known failing diagnostic separate from the existing passing suites. Fixing a
clock total alone does not prove correct write or event timing.

Patches 0015 and 0016 correct indexed-write cycle modifiers and add
experimental bus-clock accounting. With `SNESRECOMP_EMIT_BUS_TIMING=1`, the
initial 14 cases agree on final CPU/master totals, while five write-timestamp
comparisons still fail. A wider final-clock/state suite covers 376 cases and
reports byte-write order differences separately. Neither result establishes
correct MMIO side effects or event ordering.

Patch 0017 added exact-entry, opt-in instruction timing for a bounded
set of native straight-line leaf routines. It commits each opcode after its
effects and uses the interpreter's refresh, beam, coprocessor and APU completion
step. The initial timestamp-only version still failed the title comparison:
the beam lagged despite equal instruction clocks. Sharing completion work
made the small MMIO leaf pass the controlled replay comparison against the
experimental bus-clock baseline.

The subsequent baseline isolation separated two changes. The indexed-write
interpreter correction first changes the recorded frame comparison at frame
75. The bus-clock option causes the earlier frame-11 difference, starting in
existing boot setup code. Default regeneration without the bus-clock option
produces identical C for all 171 existing variants. A rebuilt control without
either timing change matches the working baseline through the neutral check
and the 4500-frame replay.

Bounded Mesen captures through the existing lab adapter independently support
the first affected instruction costs. The observed native M1X0 indexed store
takes 5 CPU cycles and 38 master clocks, compared with the old interpreter's
6 and 44. The elapsed PPU-setup-to-next-setup interval is 2160 master clocks
in both Mesen and the bus-clock build, compared with 2286 in the working
build. Two captures reproduce the same CPU states and clocks. These findings
support the corrections at those sites; they do not establish whole-game
hardware accuracy or correct generated write visibility.

Retain those bounded reference checks and extend executable write/event tests
before widening selected bodies. Global bus timing remains experimental and
opt-in. Accepted native leaves use selected instruction timing in
normal generation. Treat exact comparison with the old working build as a change detector,
not an unconditional acceptance gate after an independently supported timing
correction. Same-model controls must still match, and later state/video changes
still need behavioural validation. Peripheral phase and generated block
precharge remain relevant limits before normal generation or cfg promotion.

## Accepted normal-build checkpoint: 345 variants and eight continuations

The `07D8A5:M1X0` sound upload now uses instruction timing and the internal
`07D8CA:M1X0` continuation at local stack depth one. Patch 0035 supplies
pre-port APU flushes and the required tested instruction forms. The terminal
`07D90F:M1X0` status-restore block stays interpreted to preserve the caller's
cooperative-wait behaviour. The long input passes all 4,816 frame comparisons;
the new entry executes 130 times and removes 1,841,899 interpreted instructions,
a 19.77% overall reduction and 99.88% for this graph. Work outside the graph,
tracked cfg and the normal analyzer manifest are unchanged. Normal generation
reproduces the candidate, with 345 bodies and 42 timing selections. See
[AOT_RECOVERY.md](AOT_RECOVERY.md) for rejected candidates and retained limits.

## Previous checkpoint: 345 variants and seven continuations

The `00CE3F:M1X0` change adds instruction timing and the `00CE52:M1X0`
continuation at local stack depth two. Patch 0034 permits a known interpreted
JML tail to use the existing owner handoff while retaining local saves.
`00CE51:M1X0` remains excluded and still executes 3,382 times in the interpreter.
Both saved title inputs pass. The new continuation executes 4,787 times and
removes 1,356,971 interpreted instructions, a 12.71% overall reduction and
99.04% for this graph. The analyzer manifest and cfg are unchanged. See
[AOT_RECOVERY.md](AOT_RECOVERY.md) for stack observations, validation and limits.

## Previous checkpoint: 345 variants and six continuations

The second bank-00 scan selects instruction timing for `00D0DE:M1X0` and
adds `00D0F1:M1X0` at proven local stack depth two. Its observed interpreted
entry follows JML at `01C119`. Patch 0033 enables tested direct-page ORA at
both M widths; the existing continuation mechanism needs no change.
The long replay executes the new entry 1,352 times and removes 376,410
interpreted instructions, a 3.41% overall reduction and 97.55% for this graph.
Both saved inputs match their frame controls. The build retains 345 bodies
and uses 40 instruction-timing roots. See
[AOT_RECOVERY.md](AOT_RECOVERY.md) for validation and remaining limits.

## Previous checkpoint: 345 variants and five continuations

The bank-00 scan change selects instruction timing for `00CFE8:M1X0` and
adds `00CFFB:M1X0` as a continuation at proven local stack depth two. Its
observed interpreted entry comes from a JML tail transfer. The shared patch
adds the required tested instruction forms and supports preserved local
saves. The long replay executes the new entry 1,925 times and removes
1,274,232 interpreted instructions, a 10.34% reduction. Both saved inputs,
shared suites and fresh normal generation pass. Three normal-build pairs
suggest 2.59% less local wall time. See
[AOT_RECOVERY.md](AOT_RECOVERY.md) for exact scope, failures, evidence and limits.

## Previous checkpoint: 345 variants and four continuations

The second 29 September continuation selection adds `01EC64:M1X0` inside
`01EBAE:M1X0` and `01ED81:M1X0` inside `01ECCB:M1X0`. Direct traces confirm
NMI deadline handoffs. The unchanged generator validates both existing loop
entries, and both execute in the long replay. The change removes another
414,547 interpreted instructions, or 3.25% overall, recovering 99.55% of the
two graphs' remaining interpreted work. Fresh normal generation reproduces
the candidate; both saved replay comparisons and the neutral check pass.
Three normal-build pairs suggest 1.14% less local wall time. Shared source
is unchanged and its passing suites are reused. See
[AOT_RECOVERY.md](AOT_RECOVERY.md) for evidence and limits.

## Previous checkpoint: 345 variants and two continuations

The 29 September continuation change recovers execution inside existing
instruction-timed bodies. `01E695:M1X0` and `01E7E5:M1X0` are validated internal
block entries, separate from cfg function roots and subroutine dispatch.
The normal analysis manifest is unchanged. Both saved replays pass, and the
long input removes 852,416 interpreted instructions, a 6.27% reduction.
The two target loops retain less than 1% of their previous interpreted work.
Three normal-build benchmark pairs suggest about 1.9% less local wall time.
See [AOT_RECOVERY.md](AOT_RECOVERY.md) for the event trace, stack and temporary
checks, actual continuation counts and remaining limits.

## Previous checkpoint: 345 variants

`01E5DF:M1X0`, `01E72F:M1X0` and `01EE3E:M1X0` pass both saved replays
with selected word-operation and local X-save timing. Word memory shifts and
X pushes use the interpreter's high-byte-first write order. The analysis
manifest is unchanged and no exit contract is added. The batch removes
225,091 interpreted instructions in the longer replay, a 1.63% reduction,
with about 1.1% less local rendered wall time. Most caller-loop work still
runs interpreted; see [AOT_RECOVERY.md](AOT_RECOVERY.md) for the next bounded
continuation question and measurement limits.

## Previous checkpoint: 342 variants

`01EBAE:M1X0`, `01ECCB:M1X0` and their helper `01EF24:M1X0` are validated
with selected arithmetic and byte memory timing. Their analysis facts are
unchanged; only the existing interpreted selection changes. Compiling the
pair alone left their busy loops interpreted after the helper call. The full
three-entry batch removes 1,115,522 interpreted instructions in the longer
replay, a 7.47% reduction. Local rendered wall time improves by about 1.6%,
with the limits recorded in [AOT_RECOVERY.md](AOT_RECOVERY.md). No bodies are
removed and no exit declaration is added.

## Previous checkpoint: 339 variants

`00C818:M1X0` is validated with balanced byte-stack instruction timing and
HVBJOY reads that leave beam advancement to instruction completion. The old
frame-74 discrepancy is resolved. There are no removals or new exit declarations.
This is a correctness and coverage improvement; the small rendered benchmark
shows no measurable speed gain. See [AOT_RECOVERY.md](AOT_RECOVERY.md).

## Previous checkpoint: 338 variants

The next focused milestone adds `00CE11:M1X0` using tested CPX and INX
instruction timing. The primary and 4,816-frame replays pass, with 4,594
compiled entries in the longer run. Normal generation reproduces the clean
candidate. No accepted bodies are removed and no exit declaration is added.
See [AOT_RECOVERY.md](AOT_RECOVERY.md) for the modest measured gain and limits.

## Previous checkpoint: 337 variants

The [28 September hot-root checkpoint](AOT_RECOVERY.md) adds `0088C1`,
`019D3B`, `01A14A` and `02858C`, each M1X0. These frequent roots use selected
instruction timing. Their unvalidated dependencies remain interpreted through
`interpret_only`, which permits normal exit analysis without asserting an exit
contract. The combined candidate passes the saved 4,816-frame replay, with
about 348,600 compiled entries per addition and no tier-2 diagnostics.
Normal generation reproduces the clean candidate C and manifest without a
private profile or runtime deny guard. See the recovery report for the final
integration checks, performance measurement and limits.

## Previous checkpoint: 333 variants

The [28 September recovery report](AOT_RECOVERY.md) records four additional
callers of interpreted `02A3E2:M1X0`. A new exact-entry exit declaration uses
repeat Mesen evidence without declaring exits for the other entry widths.
Normal generation reproduces all seven passing candidate C files and the full
manifest. The final runtime and generated library passed the 4,816-frame
comparison and the installed headless runner passed the 180-frame smoke check.
That checkpoint preceded the four hot-root promotions above.

## Previous checkpoint: 329 variants

The [13 September recovery report](AOT_RECOVERY.md) records 20 further adopted
bodies, their exact execution evidence and the remaining classification. The
combined clean selection passed a focused 4,816-frame replay. The maintainer
waived manual gameplay and requested promotion. Fresh normal generation and
the installed desktop code/data match the tested candidate.

## Previous checkpoint: 309 variants

The [bulk screening report](AOT_BATCH_SCREENING.md) records the accepted
118-body addition. All five saved replays pass across 18,354 frames, and the
maintainer accepted gameplay. Tracked cfg and timing policy reproduce the
309-body candidate without a private profile. Use the bulk workflow for
further screening rather than imposing a fixed small batch size or manually
testing every routine.

## Previous checkpoint: 191 variants

The next accepted entry is `$01:9B33 M1X0`, observed as a call target in the
original tier-2 match. It has one block, 26 instructions and no calls. Ordinary
AOT completed the selected replay without bailouts but first diverged at frame
1157. Five bounded call comparisons agreed on registers, flags, CPU cycles,
multiplication outputs and WRAM. Master clocks and beam position differed.
The ordinary body charged 648 master clocks instead of the bus model's 614,
before refresh stalls. This identified a cost difference, without establishing
that per-instruction scheduling was required.

Selecting the existing bus-cost implementation for this entry, while retaining
block timing, restored exact agreement on every recorded field across the
2,125-frame replay. It removed 8,367 interpreter call gaps. The maintainer
accepted the 191-body gameplay candidate; its capture exited normally with no
bailouts or capture loss. Inputs were not recorded for that manual session.
Temporary 40-clock refresh differences still occur within sampled calls and
catch up outside the interval. Passing frame comparisons do not establish
exact within-routine write/event timing or hardware multiplier latency.

Tracked bank01 cfg now declares this exact entry, with no exit-width contract.
Patch 0020 adds `SNESRECOMP_EMIT_BUS_TIMING_TARGETS`, and the shared normal
generation helper selects `019B33:1:0`. Global bus timing stays disabled.
The existing nineteen instruction-timed entries remain selected. No new CLC
implementation, instruction-timing support, or runtime change was required.

The private cfg proposal passed lab manifest validation and reproduced the
accepted full manifest and all seven generated C files without a profile.
Focused selector/cache checks, the 391-test Python v2 suite, the shared C suite
and seven patch-setup checks passed at this integration milestone. Fresh normal
generation reproduced that output and the desktop build succeeded. Its raw
SHA-256 differs from the accepted private executable in exactly 48 bytes:
the 16-byte Mach-O UUID and the corresponding 32-byte signature page hash.
All other bytes and every generated archive member match. Both signatures
verify, and every code-page signature hash was checked. This establishes the
same executable code and data, so the accepted replay and gameplay results
were reused. The publication check passed. Built on macOS Apple Silicon;
Windows uses the same generation policy but was not executed.

Patch 0020 is now published on the owner's snesrecomp integration branch at
`1deba06c24336295a67bb95703a98bf3fcfd9766`. That title pin included all twenty patches;
normal setup verifies a clean dependency checkout. Publishing and repinning
changed no tested generator or runtime source.

Private evidence and promotion records are named by
`captures/aot-019b33-block-current.txt` and
`captures/aot-019b33-promotion-current.txt`.

For the next candidate, first try ordinary AOT on a representative replay.
If it differs, distinguish operation/state errors, incorrect clock costs and
event-order differences using the earliest reproducible evidence. Test a
bounded bus-cost correction when justified. Add instruction-timing opcode
support only when the candidate's evidence requires that mode. A call-bearing
body or an opcode outside the instruction-timing allowlist is not by itself
proof that ordinary AOT cannot work. Use bounded Mesen evidence when the
question needs an independent hardware observation.

## Previous normal-build checkpoint: 190 variants

The 12 September 2026 leaf batch adds seventeen exact call-entry variants to
the previous 173-body build. They use the existing selected instruction-timing
mode. No generator/runtime patch or dependency repin was needed. Tracked
entry declarations are in bank00 through bank03 cfg files; the shared normal
generation helper selects nineteen instruction-timed variants in total.
Global bus timing remains disabled. No new exit-width contracts are asserted.

Two targeted replays matched the accepted control on every recorded field
across 6,625 frames, including CPU state, clocks, memory and pixels. Sixteen
additions were exercised, removing 9,201 interpreter fallbacks. This measures
coverage, not host speed. `$00:B82C M1X0` has an observed call entry in the
original full-match profile but no coverage in the saved replay set. Retain
that limitation when discussing the batch; manual acceptance does not prove
that each routine ran.

The maintainer tested the 190-body desktop candidate and reported correct
gameplay with no noticeable difference. Its capture exited normally with no
bailouts, tuple overflow or journal write failures. The session did not record
controller input. The private cfg proposal was validated with the lab manifest
reader and reproduced the entire accepted manifest and all seven generated C
files byte-for-byte without a profile.

Fresh normal generation reproduced that output, and the rebuilt
`build/super_tennis` has the same SHA-256 hash as the desktop candidate the
maintainer tested. Its accepted gameplay and replay checks were reused.
The seven build-workflow checks passed. The shared runtime and generator
suites were not repeated because their source was unchanged. This promotion
was built on macOS Apple Silicon; Windows shares the generation helper but
was not executed.

Private evidence is under the directories named in
`captures/aot-leaf-batch-current.txt` and
`captures/aot-batch-promotion-current.txt`. This batch leaves 670 of the
860 variants from the broad profile analysis outside the normal AOT set.
Call-bearing candidates need separate validation; some may need additional
instruction-timing support if their observed behaviour requires that mode.

## Previous normal-build checkpoint: 173 variants

The accepted candidate has 173 AOT variants. In addition to the small
`$00:C7A0 M1X0` leaf, it selects `$00:C3DE M1X0`, the only leaf among the five
frequent profile candidates. Static inspection shows 18 instructions and no
calls. Patch 0019 adds its required local branching and arithmetic
support, and fixes IRQ sampling at selected instruction/return boundaries.
Global bus timing remains disabled.

The initial branch candidate passed four replays but diverged in the fifth at
frame 2893. A pending unmasked hardware IRQ stopped the interpreter after a
load; selected AOT continued for 312 more master clocks through its return.
The new instruction boundary check fixes this specific gap. The final
candidate and the corrected 172-body control match every checked field across
18,534 frames. The control also matches the previous 172-body build. The new
coverage removes 1,250,136 interpreted C3DE calls across those runs, with no
recorded bailouts. Frequency is not a performance measurement; the private
report records a separate benchmark with diagnostic capture disabled.

Repeated bounded Mesen captures through the lab independently confirm native
M1X0 entry at both selected addresses. The private cfg proposal declares only
these two functions and uses `--cfg-roots`, with the same exact instruction
selection. It reproduces the entire profile-generated manifest and all six C
files byte-for-byte without the profile. The authoritative cfg now contains these two `func` declarations. Enabling cfg
roots adds only these two roots to the previous normal analysis. No exit-width
contract is inferred from the observed entry modes.

The gameplay handoff used `captures/play-frequent-aot-candidate.command`. It launches a
separate desktop executable with copied settings and saves a fresh diagnostic
log and tier-2 capture on each run. It does not record controller input.
Detailed evidence, generation commands, the private cfg proposal, initial
failure, final comparisons and patch reproduction are under the directory
named in `captures/tier2-session.IysKCO/frequent-leaf-current.txt`.

On 12 September 2026, the maintainer tested the 173-body candidate and reported
that it plays correctly with no noticeable difference from the original build.
The captured session exited normally, reached frame 5997, and recorded no
bailouts, tuple overflow or journal write failures. This is acceptance of the
reported gameplay session; full-match completion was not explicitly reported.
Diagnose any later failure from its first observable point and add a
deterministic replay before another timing change.

The dependency fixes and cfg declarations are now part of the normal build
inputs. Both platform regeneration scripts call `tools/generate-normal.py`.
It enables `--cfg-roots` and selects exactly `00C7A0:1:0,00C3DE:1:0` for
instruction timing, with global bus timing disabled. Inherited experimental
emitter flags and precision profiles are cleared. The existing title diagnostic
tools request instrumentation through explicit helper arguments.

The public submodule pin now includes all nineteen patches at commit
`3a383fb8348140cd49371641e26086f81e1b2da3`. Normal setup verifies the committed
source without modifying it. The exported patches retain the recovery path
from the older supported revisions. See [SNESRECOMP_PATCHES.md](SNESRECOMP_PATCHES.md).
Normal regeneration needs the tracked sources and the verified ROM only.

Normal-build integration was verified on 12 September 2026. Fresh generation
from a separate clean dependency checkout and from the main checkout produced
the same six C files and complete analysis manifest as the accepted candidate.
The clean generated build matched all 18,534 recorded frames in the private
frame-digest harness. The normal headless executable also passed all five
replays and the neutral check with matching final state summaries. The normal
desktop passed a 180-frame display smoke check. The Python v2 suite, shared C
suite, six patch-workflow tests and publication boundary check passed.

The tested platform was macOS Apple Silicon. Windows uses the same Python
generation and patch helpers; CRLF patch application was tested, but PowerShell
and MSVC were not executed. Integration evidence and original build backups
are under the private directory named in
`captures/normal-aot-integration-current.txt`. Use `build/super_tennis` for the
normal accepted build. The earlier private launchers remain diagnostic artifacts.

The subsequent repin to the published dependency commit used a clean rebuild.
Both the desktop and headless executables had identical SHA-256 hashes before
and after, and all generated C and the manifest were unchanged. At the
maintainer's request, the runtime suites were not repeated for this repin.
The new-pin setup check and publication boundary check passed. Private hash
records are under the directory named in `captures/repin-publication-current.txt`.

This checkpoint does not complete the broad profile pass. That pass generated
860 AOT variants against the original 171, an increase of 689. Only two of
those additional variants are in the accepted gameplay candidate. Classify
the remaining variants by current instruction-timing support, missing shared
support, and entry-boundary evidence. Rank eligible batches by observed use,
then compare them through the deterministic replays. The other four roots in
the five-target frequency audit contain calls and need call/control-flow
support. Do not treat that five-target audit as a classification of the entire
profile, or treat generated eligibility alone as runtime validation.

## Generate and test privately

1. Run the emitter with `--profile-manifest` into a separate ignored output
   directory. Keep the baseline cfg and generation options fixed for the
   first comparison. Do not replace the working generated tree or build.
2. Compare roots, exact AOT variants, interpreter-only variants, and rejection
   reasons. The profile supplies observed entry points and widths. The ROM
   supplies instructions; the analyser still decides eligibility.
3. Build a separate experimental executable. Start with a representative
   deterministic title replay and appropriate runtime diagnostics. Reuse
   verified unchanged controls. Add checks for uncovered entries or a concrete
   failure; do not repeat all suites on each selection change. Compiler
   acceptance alone is insufficient.
4. Iterate on a bounded candidate set. For a mismatch, record the earliest
   failing frame, PC, M, and X before changing analysis or scheduling. Use
   independent bounded Mesen evidence through the lab where needed.
5. Retain interpreter fallback for unresolved paths. A gap count of zero or
   a target AOT percentage is not an acceptance requirement.

The profile loader accepts clean hardware call landings and independently
declared function boundaries as roots. An indirect jump can land inside an
existing function or at a continuation. Do not declare every jump target as
a standalone function. Do not infer exit widths from observed entry widths.

Game-neutral analyser, generator, or runtime fixes belong in `snesrecomp`.
Follow [SNESRECOMP_PATCHES.md](SNESRECOMP_PATCHES.md) for the pinned dependency
and ordered patch series. Title cfg proposals belong to this repository and
must follow the lab evidence and validation process in `AGENTS.md`.

## Promote accepted knowledge into tracked inputs

Once an experimental candidate set passes its behavioural checks:

1. Express the accepted entry boundaries and exact entry M/X widths in the
   relevant `config/bankNN.cfg` files, with concise evidence rationale.
   Record only the needed seeds; let static analysis discover their callees.
   Add dispatch-table or exit-contract directives only with the evidence
   required for those stronger claims.
2. Make the root policy explicit in both `tools/regenerate.sh` and
   `tools/regenerate.ps1`. A `func` declaration alone is not a reachability
   root under the current default policy. The existing `--cfg-roots` option
   makes declared functions analysis roots. Review its effect on the entire
   cfg set before enabling it in the normal commands.
3. Keep `funcs.h` for host declarations and aliases. It is not the storage
   mechanism for profile discoveries.
4. Document any generation-option or dependency changes needed to reproduce
   the accepted result. Do not commit the raw profile, input recording,
   generated C, manifests, or private comparison reports.

Both normal regeneration scripts now use the shared cfg-root and exact
timing-selection policy. Neither requires a profile. Extend the tracked
selection only after applying the same acceptance gate to another candidate set.

## Final reproducibility and behaviour gate

1. Generate into a fresh directory using the proposed tracked inputs and the
   normal generation options, with no `--profile-manifest` and no reliance on
   a previous generated tree or analysis cache. Prefer a fresh checkout using
   the documented regeneration command to test the contributor path itself.
2. Compare against the accepted experimental result: exact roots and variants,
   AOT versus interpreter disposition, exit contracts, unresolved edges, and
   emitted code. Do not accept equal totals as evidence of equal output.
   Explicit cfg names or boundaries can change code partitioning; investigate
   differences, document any expected ones, and validate the resulting code.
   With identical effective inputs and generator, require reproducible output.
3. Build from that freshly regenerated output. If its desktop binary hash is
   identical to the accepted candidate, reuse the candidate's gameplay and
   replay checks. Otherwise investigate the difference and run the relevant
   focused checks on the final build.
4. Run the publication boundary check. Run the Python v2 and shared C suites
   when their generator/runtime source changed; reuse prior results for an
   unchanged dependency. Generation-glue changes require the build-workflow
   check. Verify both platform scripts share the generation policy and record
   platforms and entry variants that were not exercised.

The work is complete when contributors can generate the accepted code through
the documented normal build path without private capture files, and that
regenerated build passes the required checks. Further untested game modes or
remaining interpreter coverage are separate follow-up work.
