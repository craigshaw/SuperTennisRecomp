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
opt-in. The two accepted native leaves now use selected instruction timing in
normal generation. Treat exact comparison with the old working build as a change detector,
not an unconditional acceptance gate after an independently supported timing
correction. Same-model controls must still match, and later state/video changes
still need behavioural validation. Peripheral phase and generated block
precharge remain relevant limits before normal generation or cfg promotion.

## Accepted normal-build checkpoint

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
3. Build a separate experimental executable. Run the neutral headless check,
   relevant deterministic title replays, and appropriate runtime tripwires
   and event-timing diagnostics. Compiler acceptance alone is insufficient.
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
instruction-selection policy. Neither requires a profile. Extend the tracked
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
3. Build from that freshly regenerated output and rerun the neutral check,
   relevant title replays, and diagnostics. This checks the final cfg-driven
   build, not just the earlier profile-driven experiment.
4. Run the publication boundary check, Python v2 suite, and shared C suite.
   Verify both platform scripts express the same generation policy and record
   any platform that was not tested.

The work is complete when contributors can generate the accepted code through
the documented normal build path without private capture files, and that
regenerated build passes the required checks. Further untested game modes or
remaining interpreter coverage are separate follow-up work.
