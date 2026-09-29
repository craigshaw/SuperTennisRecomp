# AOT recovery

The accepted normal build generates **345 AOT bodies**, with nine validated
scheduler continuation entries. The accepted runtime checkpoint is title `95b90e2`.
The shared pin is `9f94625`, recorded in `SNESRECOMP_PATCHES.md`. This report
supersedes the older handoff in `AOT_BATCH_SCREENING.md`.

## Latest checkpoint: main-loop continuation accepted, 29 September 2026

The normal build now resumes at `008035:M1X0` within existing owner
`00801E:M1X0`. The main loop uses AOT for five calls and its long back-edge,
then hands back to the interpreter at `008031`. The callee `00804D` stays
interpreted and keeps its progress history. The selection has **345 bodies,
43 instruction-timing roots and nine continuations**. No cfg function, exit
contract or runtime scheduler mechanism was added.

### Proof and the focused correction

The saved generation rejection identified an unnamed same-bank JML as the
blocking gate. Patch 0036 permits this narrow case only when the target is
a decoded exact M/X instruction before the jump, separate from the root,
with zero local stack depth at both ends and no compiled exact entry.
It collects instruction depths during the existing stack proof. The ordinary
stack validator runs first, so this rule cannot borrow the saved-stack
exception for named interpreted tails. Missing or mismatched exact targets,
nonzero depths, forward jumps, root jumps and compiled destinations fail.

The first synthetic test exposed a second gate: the emitter abandoned unnamed
same-bank transfers, returning after one loop iteration. The correction sends
only the proven back-edge through the existing interpreter-owner tail handoff.
The final suite passes 32 complete and 768 event comparisons, with 860 native
entries. Eight added cases cover both M widths, SlowROM/FastROM, RTS/RTL roots,
interpreted callees and resumed execution. Event checks include deadlines,
refresh, NMI and IRQ. The initial failure remains in private evidence.

### Replay result and useful work

The 100-frame probe matches and executes the new continuation 27 times.
One representative 4,816-frame replay then matches every saved frame field,
executes it 348,746 times and reports no bailouts, tuple overflow, journal
failures or changed opcodes. All eight earlier continuation counts agree
with the accepted control. The normal Python analyzer manifest is byte
identical to the earlier checkpoint.

| Exact M1X0 instruction | Interpreted before | Interpreted after |
| --- | ---: | ---: |
| `008035` | 348,746 | 0 |
| `008039` | 348,746 | 390 |
| `00803D` | 348,746 | 968 |
| `008041` | 348,746 | 2,576 |
| `008045` | 348,746 | 3,538 |
| `008049` | 348,745 | 3,927 |

Only these six instruction counts change. `008031`, `00804D` and all work
outside this set remain unchanged. Total interpreted work falls from
7,475,516 to **5,394,440 instructions**, a reduction of **2,081,076 or 27.84%**.
Interpreted CPU cycles fall from 37,742,735 to 22,473,399. These counters
measure tier movement, not guest instructions or time removed from the game.
The 11,399 residual later-key instructions are retained after event handoffs;
do not start another continuation expansion for this small remainder.

Three quiet warm pairs of normal rendered headless runs average **3.057512
seconds before and 2.632804 seconds after**, or **13.89% less elapsed time**.
Process CPU time falls by 13.90%. All final image hashes agree. The first
pair is preserved but treated as warm-up because the first copied baseline
launch had 0.269 seconds of wall time without CPU time. One extra pair gives
three warm comparisons. This is a local result, not a desktop frame-rate claim.

### Promotion, reuse and remaining limits

The final shared source passes Python v2 403/403 and the shared C suite once.
Seven workflow checks pass, including recovery from the old base and CRLF
checkout. Patch 0036 and the updated source hashes reproduce the new shared
pin. Fresh normal generation in an empty output directory matches all seven
candidate C files and the complete manifest. The normal desktop and headless
build succeeds, and the headless neutral 180-frame check passes. Publication
boundary and whitespace checks pass. Unrelated UI and lab changes are preserved.

The cached experiment takes 3.45 seconds to generate, 2.93 seconds to build
and 5.38 seconds for the instrumented full replay. It reuses the accepted
`aot-apu-upload-20260929/normal-long/frames.csv` and
`candidate3-long/opcode-work.csv`, plus the saved host-cost ranking and
generation rejection. No bulk screen, classification rerun or new harness
was needed. Wider title replays, Windows, manual gameplay and audio listening
were not repeated. There is no new Mesen hardware-equivalence claim.

Private evidence is indexed by `captures/aot-main-loop-current.txt`, under
`captures/aot-main-loop-20260929`. Start with `validation-summary.json`,
`candidate1/result.json`, `work-delta.json`, `normal-reproducibility.json`
and `benchmark-warm.json`. Raw initial failures and warm-up measurements are
retained. The summary records both normal executable hashes and exact commits.

**Next task:** make one static feasibility check for the NMI clear loop at
`00899F`, using the saved boundaries and existing interrupt owner. It retains
588,032 interpreted instructions but only about 0.064 seconds of measured own
host cost in the earlier build. Its `008917` owner still has the `07AC3F`
exit-analysis gap. Identify the missing proof before requesting new captures
or building. Do not assume ordinary continuation ownership applies to NMI or
invent a callee exit. If it needs a separate scheduler mechanism, park it and
check the `00D271`/`00D285`/`00D2A0` compiled-tail group once. Keep `00804D`
and the completed main-loop residual out of the immediate queue.

## Previous checkpoint: main-loop host cost and next compiler gate, 29 September 2026

The main dispatch loop is the next priority. Three quiet rendered comparisons
attribute about **0.651 seconds, or 20.8% of instrumented replay time**, to
its own interpreter work, excluding native callees. The NMI clear loop accounts
for **0.064 seconds, or 2.04%**. These are elapsed host costs that include
instruction completion, device work and profiler overhead. They are not
predicted speedups. Runtime selection remains **345 bodies, 42 timing roots
and eight continuations**; no cfg or shared source changed.

### Measurement and one rejected method

The experiment runner now accepts disjoint exact-key `host_cost` groups and
has a `profile` mode. A private bridge overlay assigns each interpreter
iteration to its exact key, separates native callees, and restores the prior
owner after nested interpreter calls. It reads a monotonic clock only when
the category changes. This covers short loops that a coarse sample can miss.
The profile mode renders but disables frame hashing, work/entry counting and
tier-2 capture. Normal title binaries contain none of this instrumentation.

An initial macOS `sample` attachment failed for lack of process inspection
access. A private SIGPROF experiment then undercounted short regions: it
reported roughly 1.4% to 3% for the main loop and almost no NMI samples,
while transition timing observed substantial work in both. Those signal
counts are rejected for ranking. The retained implementation uses elapsed
attribution only. Do not treat the rejected zero samples as zero CPU cost.

| Region | Mean elapsed time | Range across three runs | Share of instrumented run |
| --- | ---: | ---: | ---: |
| Seven main-loop keys, M1X0 | 0.650997 s | 0.646016 to 0.655820 s | 20.67% to 20.87% |
| Four NMI clear keys, M1X0 | 0.064085 s | 0.063532 to 0.064828 s | 2.02% to 2.06% |

The accepted normal executable averages 3.045288 seconds; the attributed
build averages 3.135667 seconds, about **2.97% instrumentation overhead**.
Process CPU time is close to elapsed time in these quiet single-threaded runs.
All final images match. These figures establish a useful ordering, not exact
removable cost. A normal-build benchmark must establish any eventual gain.
There is no desktop or sustained frame-rate claim.

### Observed boundaries and static compiler findings

The saved long profile still contains 2,441,221 interpreted instructions in
`008031`, `008035`, `008039`, `00803D`, `008041`, `008045` and `008049`, all
M1X0. The first six are long calls; the last is a long jump back to `008031`.
The NMI work set has 588,032 instructions at `00899F`, `0089A2`, `0089A3`
and `0089A6`. Its loop clears 32 bytes at `$0F00,X`.

A bounded 100-frame replay produces 414 trace records in frames 73 through
85, with no truncation. The observed main-loop keys use M1X0 and S=`0FFF`.
The NMI loop uses M1X0 and S=`0FEB`, with store indices zero through 31.
These are observed states in this input, not universal entry/exit contracts.
The profiler sees 182 additional main-key visits before opcode execution;
pre-instruction boundary checks can yield, so visits must not be reported as
additional executed instructions.

The title scheduler already delivers events at instruction boundaries and
keeps active interrupt frames through RTI. No new interrupt system is needed
for this assessment. The normal analyzer's proven `00801E:M1X0` prefix ends
at the call to interpreted `00804D`. The emitted body nevertheless contains
the later loop blocks. Do not infer the emitted region solely from the
manifest's prefix instruction count.

A single private generation probe selects instruction timing for existing
`00801E:M1X0` and continuation `008035:M1X0`. Instruction and entry/depth
validation complete, then generation rejects the long back-edge with:
`continuations require known interpreted JMP tails`. No candidate is built
or promoted. This is a specific compiler gate, not evidence that a new
scheduler-region architecture is required. The five callees after `00804D`
already have compiled M1X0 bodies and normal-analyzer M1X0 exit facts.

The NMI owner `008917:M1X0` remains LLE-only because its call to `07AC3F`
has an unproven exit. With approximately one tenth of the measured main-loop
cost, it stays behind the main-loop work. Do not start a second exit-proof
investigation for it now.

### Validation, limits and next task

One full 4,816-frame diagnostic replay reproduces both the accepted frame
file and entire interpreter profile byte for byte. All eight continuation
counts agree; bailouts, overflow, journal failures and changed opcodes remain
zero. After replacing the rejected signal method, the final timing overlay
passes the 100-frame probe. Three quiet timing pairs provide the final ranking.
Eleven tool tests pass, including a deterministic C clock test that proves
nested ownership restoration and exclusion of native callee time. Normal
generated files, analyzer manifest and both executable hashes are unchanged.
Shared suites, workflow checks and accepted gameplay are reused. Publication
boundary and whitespace checks pass. Unrelated UI and lab files are preserved.

No new Mesen observation, bulk screen, full replay sweep, shared compiler
suite, normal rebuild, Windows run, manual gameplay or audio listening check
was performed. The profiling extension is for the single-threaded POSIX host;
its validation here is macOS only. Raw decode output that printed default
M/X fields was corrected in a separate derived report using explicit decoder
arguments and instruction lengths. The original report was retained.

Private evidence is indexed by `captures/aot-host-cost-current.txt` under
`captures/aot-host-cost-20260929`. Start with `ranking.json`,
`validation-summary.json`, `boundary-summary.json` and the saved generation
failure in `main-continuation-feasibility/generate.log`. Earlier signal runs
are retained as rejected measurement evidence. Reuse the final quiet pairs.

**Next task:** test the narrow compiler rule needed for the existing long
back-edge, aiming to resume at `008035:M1X0` after the interpreted `00804D`
call. Preserve exact widths, proven stack state, event deadlines and owning
interpreter handoffs. Keep `00804D` interpreted and retain its progress history.
Do not add a fake function root or invent its exit contract. Start from the
saved rejection and existing tail tests; do not repeat classification or
profiling. If that narrow rule cannot be proved, stop and report its specific
missing invariant before designing a larger mechanism.

## Previous checkpoint: reusable experiment tooling, 29 September 2026

This pass improves the working loop. Runtime selection remains **345 bodies,
42 instruction-timing roots and eight continuations**. No shared runtime,
compiler, cfg fact or patch changes were made. The accepted runtime commits
above remain the behavioral baseline.

### Reusable runner and evidence

`tools/aot-experiment/run.py` now locks private inputs, imports the normal
selection policy, generates a clean candidate, builds private source overlays,
runs a short probe before one full replay, and produces compact results.
It records source identity, build/replay commands and elapsed times. Both
probes require the selected exact native keys to execute. Unchanged overlays
use the CMake/Ninja cache; failed or stale inputs stop the attempt. Every
attempt gets a separate evidence directory. No generated source or raw
capture enters Git.

The instruction tracer uses a copy of live interpreter registers and samples
native instructions after their deadline guard. It records CPU registers,
widths, clocks, APU timing and direct port state without MMIO reads. The CSV
comparison identifies the first different fields and includes three preceding
records. Missing rows fail; capped traces are labelled as prefix evidence.
Root execution counters observe native instruction visits, not wrapper calls.
Continuation counts retain the previously validated guest-cycle criterion.

The normal policy was extracted into an importable function without changing
its selections. `AOT_CURRENT.json` is the compact baseline and ranked queue.
`AOT_EXPERIMENTS.md` defines the stage gates and stop conditions. `AGENTS.md`
now directs agents to these current records before the older overnight handoff.
The runner does not automate shared suite approval, cfg promotion or publication.

### Checks performed

The completed upload milestone was reused as the fixture. One new full
**4,816-frame** run reproduces the accepted frame file and the entire
interpreter profile byte for byte: **7,475,516 instructions and 37,742,735
cycles**, 7,950 keys, zero overflow and changed opcodes. Bailouts and journal
failures are zero. It records four native `07D8A5:M1X0` starts and 130
`07D8CA:M1X0` continuation invocations; all seven earlier counts agree.

A 90-frame control and candidate run compare **13,881 pre-instruction records**
in upload frame 10. Registers, clocks and APU state match, and neither trace
hits its 100,000-record limit. The control uses the saved pre-upload generated
output with the current shared runtime. This checks interpreter/native tracer
parity; it does not recreate the older runtime or add hardware evidence.

A deliberately wrong private frame control stops at frame one and reports
only the differing PC field. A separate five-record trace confirms the cap
is reported as prefix evidence. Ten focused tool tests pass, covering stale
locks, exclusive output, first differences, missing records, empty files,
changed instrumentation anchors, native counter placement, command failures,
private output paths and cache timestamp reuse. An unchanged 90-frame probe
requires no compilation and retains the same measurement binary hash; its
build invocation takes 0.016 seconds locally. This is a tooling measurement,
not a title performance claim or a measured token saving.

All seven workflow tests and 35 patch checks pass. Fresh normal generation
emits five banks with zero reuse and reproduces all seven accepted C files
and the analyzer manifest. Both existing normal binary hashes remain unchanged;
production source is unchanged, so accepted gameplay and shared suites are
reused. No new normal build, bulk screen, full input sweep, Mesen capture,
Windows build, manual gameplay or audio listening check was needed. Publication
boundary and whitespace checks pass. Unrelated UI and lab files are preserved.

### Evidence, limits and next decision

Private results are under `captures/aot-tooling-20260929`, indexed by
`captures/aot-tooling-current.txt`. Start with `validation-summary.json`, then
`validation1/full/result.json`, `trace-comparison.json` and
`final-cache-proof/build-result.json`. The negative control is deliberately
failing evidence, not a runtime regression. Older raw evidence stays unchanged.

Tracing covers interpreter opcodes and instruction-timed AOT in a bounded
window. Aggregate AOT, interrupt microsteps and scheduler-only waits are not
instruction records. A passing probe does not establish unobserved entry
variants. The harness currently rejects aggregate-only roots as execution
counter targets. Its initial host validation is macOS only.

The next decision is where further scheduler work pays. Use the saved main
loop work set at `008031` (2,441,221 interpreted instructions) to establish
host cost and event ownership, then compare the NMI clear loop at `00899F`
(588,032). No sampled host CPU evidence exists yet. Instruction counts alone
must not justify a new scheduler mechanism. The overlapping
`00D271`/`00D285`/`00D2A0` group remains a bounded feasibility option with a
139,392-instruction union. Defer it if compiled-tail ownership requires a
separate mechanism without broader benefit. Preserve the existing exclusions
and terminal-PLP boundary; do not reopen completed upload failures.

## Runtime milestone: timed sound-data upload, 29 September 2026

### Observed path and bounded change

The saved profile assigns 1,844,138 interpreted instructions to
`07D8A5:M1X0`, across four sound-data uploads in frames 10 through 1571.
The existing compiled body immediately took its scheduler memory-poll guard.
A bounded 90-frame control trace observes entry at frame 10 with S=`0FF9`,
PHP leaving S=`0FF8`, and the `07D8CA:M1X0` byte-transfer loop at that depth.
PLP restores S=`0FF9`; RTL returns to `028558` with S=`0FFC` at frame 49.
The trace matches the saved control. Its first launch used an incorrect frame
reference path and stopped before useful execution; `trace90` is the corrected
capture. No raw evidence was overwritten.

Patch 0035 enables the narrowly required forms in selected native instruction
timing: PHP, terminal PLP, idempotent X-bit updates, M0/X0 LDA [dp],Y and M1
accumulator ROL. Actual X-width changes and nonterminal PLP still fail validation.
Long-indirect data words carry across the 24-bit address boundary; direct-page
pointer words still wrap within bank zero. Contiguous words preserve atomic
MMIO callbacks. Stack depth and join checks remain required. The title adds
`07D8A5:1:0>07D8CA:1:0` at proven local depth one and selects the existing root
for instruction timing. No cfg, function root or exit-width contract changes.

### Two rejected boundaries and the retained limit

The first candidate differed at frame 11. A bounded instruction comparison
finds identical CPU state and clocks through the first transfer, then a
different reply at `07D8CF`. The interpreter flushes pending relative APU time
before port accesses; the timed AOT helpers did not. Patch 0035 shares that
pre-port flush and preserves the APU pacing scope around timed reads/writes.
It leaves the existing title clock policy in place. Ordinary cartridges still
use the bridge's relative catch-up as well as frame scheduling; the shared
absolute-only policy is restricted to SA-1. Do not assume Super Tennis uses
that absolute-only branch.

With port pacing corrected, a whole-native epilogue matched through the
upload but differed in APU RAM at frame 55. The first later CPU trace difference
was in the caller's cooperative wait at `02855D`/`028560`, after the upload
returned in frame 49. The interpreter's repeated-state history detects that
wait at a different point after a native epilogue. Changing the wait algorithm
would broaden this milestone and invalidate saved timing controls.

The accepted compiler boundary therefore keeps terminal PLP blocks in the
scheduler interpreter. For this body it unwinds at `07D90F:M1X0`, before the
four final port clears, status restore and return. All six epilogue instructions
still execute four times in the interpreter. Aggregate-timed memory polls also
retain their original entry guard. A future agent must preserve this boundary
until interpreter progress-history parity is handled and validated separately.
The failed candidates and traces are retained; do not repeat their experiments.

### Execution and validation

The accepted clean candidate matches all **4,816 frames** of the saved long
input, including frame-boundary CPU state/clock, WRAM, VRAM, palette, OAM,
APU RAM and rendered-pixel digests. It has zero bailouts, tuple overflow,
journal failures and changed opcodes. The new continuation commits native work
on **130 invocations**. All seven earlier continuation counts are unchanged.

| Exact work set | Before | After |
| --- | ---: | ---: |
| `07D8A5:M1X0` graph | 1,844,138 | 2,239 |
| Whole long replay | 9,317,415 | 7,475,516 |

This removes **1,841,899 interpreted instructions, or 19.77% overall**, and
6,117,439 interpreted CPU cycles. It recovers **99.88%** of this graph's
interpreted work. Every instruction count outside this graph is unchanged.
The selection remains **345 bodies**, with **42 instruction-timing entries
and eight continuations**. These uploads occur during transitions; this is
not a claim of a 19.77% steady gameplay frame-rate gain.

The new synthetic suite passes **60 complete and 1,036 event comparisons**,
with 1,011 native continuation entries. It covers real timed device waits,
byte/word ports, saved status, SlowROM/FastROM, RTS/RTL, carry/N/Z, data-bank
crossings, direct-page pointer wrap, IRQ, NMI, refresh and interpreted starts.
Python v2 passes **403/403**. The full shared C check set passes, reusing its
passing prefix and resuming after two old rejection tests were updated for
newly supported forms. They now reject actual X changes and word SBC [dp],Y.
The optional DSP-1 firmware test is skipped because the external ROM is unset.

Fresh normal cfg-only generation emits five banks with zero cache reuse and
reproduces all seven candidate C files plus the entire analyzer manifest.
The manifest is also byte identical to the previous checkpoint and passes the
lab reader. Only `bank07_v2.c` and the continuation table change in generated
text. Desktop/headless builds pass. The normal library passes the same
4,816-frame comparison with zero capture diagnostics; the normal neutral
180-frame check passes. All seven workflow tests, 35 patch checks,
publication-boundary and whitespace checks pass.

Three alternating rendered normal-build pairs, after one warm-up each,
average 3.19631 seconds for the control and
3.07209 seconds for the candidate. All three pairs
improve and their final images match. This suggests **3.89% less local wall time**.
It is a short local measurement, not a sustained gameplay frame-rate result.

### Evidence and next step

Private evidence is under `captures/aot-apu-upload-20260929`, indexed by
`captures/aot-apu-upload-current.txt` and the recovery follow-up chain. Use
`candidate3-long` and `normal-long` as the accepted evidence. `candidate/wide`
and `candidate-long` are rejected experiments. The retained profile,
classification, control and unchanged checks were reused. No bulk screen,
full input sweep, new Mesen capture, native-analyzer rebuild, Windows build,
manual gameplay or audio listening check was performed. This is equivalence
to the accepted runtime model, with no new Mesen hardware claim. Unrelated
UI and lab files are preserved.

The next small package remains the overlapping `00D271`/`00D285`/`00D2A0`
group, **139,392 instructions in its union**. Its tail reaches an existing
compiled target, so the known-interpreted-tail rule does not apply. The larger
remaining areas are the seven-instruction `008031` main dispatch loop
(2,441,221 instructions) and the four-instruction `00899F` NMI clear loop
(588,032 instructions). Both need separate scheduling analysis before a
compiler change. Keep `00CE51` excluded. Do not reopen `00804D`, chase the
2,239-instruction upload residual, or remove the terminal-status guard.

## Previous checkpoint: preserve interpreted tail ownership, 29 September 2026

### Observed path and bounded change

The accepted profile assigns 1,370,188 interpreted instructions to the
`00CE3F:M1X0` graph. A 650-frame probe of the saved long input observes the
routine already running in the interpreter at frame 377, entry S=`0FE5`.
PHX leaves S=`0FE3` and saves X=`017C`. JML at `00CEBE:M1X0` reaches
`00CE51:M1X0` without changing those saved bytes or the return frame. INY
then reaches the existing `00CE52:M1X0` loop header. PLX restores S=`0FE5`,
and RTL returns to `02DECA` with S=`0FE8`. The probe matches the frame control.

The runtime already has the required handoff. `interp_tier_dispatch_tail`
unwinds to the active interpreter owner while leaving guest state intact.
Patch 0034 extends the compiler checks to accept this case in explicitly
selected continuation bodies: a direct JML with a known exact target that
has no compiled variant. Local stack depth must remain proven and bounded.
Calls and returns still require zero local depth. Compiled and unresolved
external tails remain rejected, as do inconsistent joins, over-pulls,
predecessor IR temporaries and guarded continuation bodies. No runtime source
or opcode implementation changes are needed.

The title selects instruction timing for `00CE3F:M1X0` and continuation
`00CE3F:1:0>00CE52:1:0`, at validated local depth two. `00CE51` keeps its
tracked `force_lle` exclusion and named boundary. No cfg directive, function
root or exit contract changes. The normal Python analyzer manifest is byte
identical and passes the lab reader. Generated code outside the selected
body and continuation table is unchanged. This adds no Mesen hardware claim.

### Execution and measured result

A separate 400-frame candidate probe confirms the actual handoff at frame
377. The continuation enters with S=`0FE3`, reconstructed entry S=`0FE5`,
and owner/depth `1/1`. The JML handoff retains those values and `hrv=0`.
`00CE51` executes in that same interpreter frame, and the compiled loop
returns to `02DECA` with S=`0FE8` and unwind owner `1`. There is no nested
interpreter frame at these transfers. This probe also matches its control.

The clean candidate passes the primary 2,125-frame and long 4,816-frame
inputs. Both frame files are byte identical to the saved controls, with zero
bailouts, tuple overflow and journal failures. The new continuation commits
guest work on **4,787 invocations**. All six earlier continuation counts are
unchanged. The interpreter still records **3,382 executions of `00CE51:M1X0`**,
exactly matching the baseline.

| Exact work set | Before | After |
| --- | ---: | ---: |
| `00CE3F:M1X0` graph | 1,370,188 | 13,217 |
| Whole long replay | 10,674,386 | 9,317,415 |

This removes **1,356,971 interpreted instructions, or 12.71% overall**, and
4,410,179 interpreted CPU cycles. It recovers **99.04%** of this graph's
interpreted work. Counts for the other five bank-00 graphs stay unchanged.
The accepted selection remains **345 AOT bodies**, now with **41
instruction-timing entries and seven continuations**. This is more native
execution within an existing body. The graph's recorded work occurs in
frames 377 through 1349; the result does not establish a steady gameplay
frame-rate increase.

The new synthetic suite passes **24 complete and 576 event comparisons**,
with 562 native continuation entries. It covers local depths zero, two and
six; a returning interpreted suffix; an interpreted jump back to the native
loop; RTS/RTL; same-bank and cross-bank JML; SlowROM/FastROM; M changes;
interpreted starts; refresh, IRQ and NMI. Rejection cases preserve checks on
compiled tails, saves at calls/returns, inconsistent joins and over-pulls.
Python v2 passes 403/403 and the full shared C suite passes. Its optional
DSP-1 firmware test is skipped because the external ROM is not configured.
No title replay or synthetic test failed.

Fresh normal cfg-only generation emits all five banks with zero cache reuse
and reproduces all seven candidate C files and the complete manifest. Desktop
and headless builds pass. The normal generated library passes a final
4,816-frame comparison with zero capture diagnostics; the normal 180-frame
neutral check also passes. All seven workflow tests and all 34 patch checks
pass, as do publication-boundary and whitespace checks. The shared change
is published and exported as patch 0034.

Three alternating rendered normal-build pairs, after one warm-up each,
average 3.29848 seconds for the control and 3.23178 seconds for the candidate.
All three pairs improve and their final images match. This suggests **2.02%
less local wall time**. It remains a short local benchmark, not a sustained
gameplay frame-rate result.

### Evidence, limits and next step

Evidence is under `captures/aot-ce3f-20260929`, indexed by
`captures/aot-ce3f-current.txt` and follow-up records reached through
`captures/aot-recovery-current.txt` and the previous checkpoint. It contains
the baseline, both bounded traces, clean candidate, exact profiles, proposal
review, shared-test logs, normal validation and benchmark. Raw captures remain
private and immutable. The retained 515-body classification is reused.
No bulk screen, full input sweep, new Mesen capture, native analyzer rebuild,
Windows build or manual gameplay was repeated. Unrelated UI and lab files
are preserved.

The updated graph ranking points to `07D8A5:M1X0`, with 1,844,138 interpreted
instructions in frames 10 through 1571. It already has an AOT body, but its
generated scheduler guard sends execution back to the interpreter. Next,
inspect why that guard exists and trace a bounded invocation before changing
it. Its PHP/REP setup requires separate status/width and scheduling analysis;
do not simply remove the guard. The overlapping `00D271`/`00D285`/`00D2A0`
group has only **139,392 instructions in its union**, not the sum of its three
rows. It tails to an existing compiled target and remains outside this
interpreted-tail extension. Keep it separate. Leave `00CE51` excluded, do not
chase small scan residuals, and do not reopen the parked `00804D` experiment.

## Previous checkpoint: recover the second bank-00 scan, 29 September 2026

### Observed entry and bounded change

The retained exact-key profile assigns 385,874 interpreted instructions to
`00D0DE:M1X0`. A 650-frame probe of the existing long input observes its first
interpreted entry at frame 633, PC=`00D0DE:M1X0`, S=`0FE2`, master clock
225826354. The previous interpreted instruction is JML at `01C119` to
`00D0DE`. The probe matches the saved frame control. This establishes an
interpreted tail entry in that capture, not an AOT interrupt handoff.

The normal Python generator validates the existing `00D0F1:M1X0` loop block
at local stack depth two after PHX. The saved-stack continuation mechanism
from patch 0032 already supports it. Patch 0033 only enables direct-page
ORA at both accumulator widths in the instruction-timing gate and adds
synthetic tests. No scheduler or opcode implementation changes are needed.
The title selects `00D0DE:M1X0` for instruction timing and adds
`00D0DE:1:0>00D0F1:1:0` as the sixth continuation.

The full analyzer manifest is byte identical and passes the lab reader.
All generated code outside the target body and continuation table is unchanged.
No cfg directive, function root or exit contract changes. This experiment
makes no new Mesen hardware claim and needs no new reference captures.

### Measured work and checks

The clean candidate passes the primary 2,125-frame and long 4,816-frame
inputs. Both frame files are byte identical to their accepted controls,
with zero bailouts, tuple overflow and journal failures. The new continuation
commits guest CPU work on **1,352 invocations** in the long replay. All five
earlier continuation counts are unchanged.

| Exact work set | Before | After |
| --- | ---: | ---: |
| `00D0DE:M1X0` graph | 385,874 | 9,464 |
| Whole long replay | 11,050,796 | 10,674,386 |

This removes **376,410 interpreted instructions, or 3.41% overall**, and
1,162,217 interpreted CPU cycles. It recovers **97.55%** of the target graph's
remaining interpreted work. The other five bank-00 graph counts are unchanged.
The build still has **345 bodies**, now with **40 instruction-timing entries
and six continuations**. These are execution gains within an existing body.

The new synthetic tests pass **100 complete and 240 event comparisons**, with
92 native continuation entries. They cover both M widths, the hidden A high
byte, C/V/D preservation, direct-page alignment penalties and 16-bit wrap,
SlowROM/FastROM, saved X, RTS/RTL, interpreted starts, M transitions, refresh,
IRQ and NMI. The first derived source-scope check omitted the generated group
comment from its comparison boundary. The corrected check confirms that
unrelated C is unchanged; no source or title replay failure resulted.

Python v2 passes 403/403 and the full shared C suite passes, including the
existing continuation suites. The optional DSP-1 firmware test is skipped
because its external ROM is not configured. Fresh normal cfg-only generation
emits all five banks with zero cache reuse and reproduces all seven candidate
C files and the full manifest. Both hosts build. The normal generated library
passes a final 4,816-frame comparison with zero capture diagnostics, and the
normal 180-frame neutral check passes. All seven workflow tests and all 33
patch checks pass, as do publication-boundary and whitespace checks. Patch
0033 is exported and the shared commit is published.

Three alternating rendered normal-build pairs, after one warm-up each,
average 3.37169 seconds for the control and 3.34686 seconds for the candidate.
Two pairs improve and one regresses slightly; all final images match. The
mean is **0.74% lower**, but this small, noisy sample does not establish a
stable wall-time improvement. The measured interpreter-work reduction is
the stronger result.

### Evidence, limits and next step

Private evidence is under `captures/aot-d0de-20260929`, indexed by
`captures/aot-d0de-current.txt` and a follow-up record reached through
`captures/aot-recovery-current.txt`. It retains the original baseline,
bounded trace, clean candidate, exact profiles, proposal review, shared-test
logs, normal generation comparison and benchmark. No bulk screen, five-input
sweep, native analyzer rebuild, Windows build, manual gameplay or new Mesen
capture was repeated. Unrelated UI and lab files are preserved.

Reuse the retained classification of 515 unadopted bodies. The next larger
candidate is `00CE3F:M1X0`, still accounting for 1,370,188 interpreted
instructions. Its external JML to interpreted `00CE51:M1X0` blocks the current
continuation rules. First trace that transfer and its guest stack/return owner
with the existing long input. Then determine whether a narrow, game-neutral
tail handoff can preserve that owner. Keep `00CE51` interpreted unless separate
execution evidence justifies promotion; its saved classification has no
passing compiled evidence. Do not invent an exit or continuation contract.
Keep the overlapping `00D271`/`00D285`/`00D2A0` family separate. Do not chase
the small `00CFE8` or `00D0DE` residuals or reopen `00804D`.

## Previous checkpoint: recover the bank-00 table scan, 29 September 2026

### Observed entry path and bounded change

The saved profile ranked six overlapping bank-00 graphs at 3,183,161
interpreted instructions. Inspection selected `00CFE8:M1X0`, a large routine
without the external tail obligations of `00CE3F` and the `00D271` family.
The primary 1,200-frame native-entry probe records no deadline yield from
this routine. A separate 650-frame long-input probe finds its first
interpreted entry at frame 596, PC=`00CFE8:M1X0`, S=`0FE5`, master clock
212605016, after interpreted JML at `01ACF7`. Thus an interpreted tail entry
is an observed source of this work. Do not describe it as a proven interrupt
handoff. Both diagnostic frame comparisons match their saved controls.

The internal `00CFFB:M1X0` loop header has static local stack depth two after
PHX. Patch 0032 extends the existing continuation mechanism to a proven,
consistent local depth. The saved bytes remain on the guest stack. The
generated entry reconstructs the owning entry S without a new guest frame
or hidden host state. Existing checks still reject inconsistent joins,
unknown depths, predecessor IR temporaries and external JMP tails. Calls,
tails and returns must have zero local depth. Local absolute JMP is now
accepted when its successor stays in the validated graph.

The patch also enables the bounded instruction forms needed by this scan:
word A and Y saves, register transfers, INY, word accumulator LSR, word
direct-page DEC, word ORA abs,Y and byte LDA/SBC [dp],Y. Synthetic boundary
tests caught two issues in the new word ORA path: the high byte must carry
across a bank boundary, and a large index must still count a page crossing.
Both are corrected for that selected path. Long indirect pointer reads are
evaluated once, and word saves and DEC writes use high-byte-first order.
Other word-read forms keep their existing behavior; these tests do not
establish new bank-boundary guarantees for those forms.

The title changes only the instruction-timing selection for `00CFE8:M1X0`
and adds `00CFE8:1:0>00CFFB:1:0`. The full analyzer manifest is byte identical
and passes the lab reader. Only that routine's generated body and the separate
continuation table change. No cfg directive, function root or exit contract
is added. There is no new Mesen hardware claim.

### Measured result and validation

The clean candidate passes the primary 2,125-frame and long 4,816-frame
replays with byte-identical frame files and zero bailouts, tuple overflow or
journal failures. The new continuation commits guest CPU work on **1,925
invocations** in the long input. All four earlier continuation counts remain
unchanged.

Long-replay interpreted instructions fall from 12,325,028 to **11,050,796**.
This removes **1,274,232 instructions, or 10.34% overall**, and 4,182,247
interpreted CPU cycles. The target graph falls from 1,287,707 to 13,475
instructions, recovering **98.95%** of its interpreted work. The other five
bank-00 graphs retain their previous counts. There are still **345 AOT
bodies**, now with **39 instruction-timing entries and five continuations**.

The new synthetic suite passes **104 complete and 704 event comparisons**,
with 313 native continuation entries. It covers nested saves at depths two
and six, interpreted starts, compiled and interpreted callees, local JMP,
RTS/RTL, exact widths, SlowROM/FastROM, pointer wrapping, IRQ, NMI and refresh.
The previous continuation suite also passes 300 comparisons. Python v2 passes
403/403 and the full shared C suite passes. The first shared run stopped on
an obsolete test that rejected now-supported word PHA/PLA. That expectation
and its companion were changed to reject an unbalanced word save; the final
suite passes. Earlier probe/test setup failures and boundary failures remain
in private evidence, including a synthetic callee that collided with a
reserved fixture address. No title replay failed.

Fresh normal cfg-only generation emits all five banks without cache reuse
and reproduces all seven candidate C files and the full manifest. Desktop
and headless builds pass. The normal generated library passes a final
4,816-frame comparison, and the normal headless 180-frame neutral check
passes. All seven workflow checks, 32 patch checks, publication boundary
and whitespace checks pass. The shared fix is exported as patch 0032 and
pinned to its published commit.

Three alternating rendered normal-build pairs, after one warm-up each,
average 3.43031 seconds for the control and 3.34161 seconds for the candidate.
All pairs improve and final images match. This suggests **2.59% less local
wall time**, with the usual limit of a short local benchmark.

### Evidence, limits and next step

Evidence is under `captures/aot-bank00-20260929`, indexed by
`captures/aot-bank00-current.txt` and a follow-up record reached through
`captures/aot-recovery-current.txt`. It includes the baseline, both bounded
probes, instruction inventory, synthetic diagnostics, clean candidate,
profiles, proposal review, normal comparison and benchmark. No bulk screen,
five-replay sweep, native analyzer rebuild, new Mesen capture, Windows build
or manual gameplay was repeated. Unrelated UI and lab files are preserved.

Reuse the retained classification of 515 unadopted bodies. The next bounded
candidate is `00D0DE:M1X0`, with 385,874 interpreted instructions. Most of its
operation requirements are now supported; direct-page ORA at both M widths
remains outside the selected timing set. Validate that form and its existing
loop block before assuming it can use the same saved-stack continuation.
The larger `00CE3F:M1X0` graph still has 1,370,188 interpreted instructions,
but its external JML to interpreted `00CE51:M1X0` needs separate stack and
ownership analysis. The saved classification has no passing execution
evidence for compiling `00CE51`; do not remove its exclusion or invent an
entry/exit contract. Keep the `00D271`/`00D285`/`00D2A0` overlap separate.
Do not chase the small residual in `00CFE8` or reopen `00804D` now.

## Previous checkpoint: resume the two ALU loops, 29 September 2026

### Observed cause and selected entries

The bounded direct-entry trace confirms deadline handoffs in both remaining
instruction-timed callers. `01EBAE:M1X0` yields at frame 84 at `01EC86:M1X0`,
S=`0FF9`, master clock 29968450 against deadline 29968444. `01ECCB:M1X0`
yields at frame 86 at `01ED86:M0X0`, S=`0FF9`, clock and deadline 30683180.
Both transfers are marked as NMI deadlines. The following frames include
the NMI handler and interpreted loop entries `01EC64:M1X0` and
`01ED81:M1X0`, respectively, at the same S as their root entry. The corrected
300-frame diagnostic replay matches its saved control. The two traces end
when the guest stack rises above the root entry S.

The first probe did not clear its root label after a normal compiled return.
It therefore misattributed a later helper yield to `01ECCB`. Its raw output
is retained but rejected. The corrected probe wraps both direct compiled
entries and records their returns; only that capture supports the claims above.

Normal generation now also selects `01EBAE:1:0>01EC64:1:0` and
`01ECCB:1:0>01ED81:1:0`. These existing CFG loop entries pass the unchanged
generator checks for exact widths, zero local stack depth and no predecessor
IR temporaries. This is a static safety check, followed by observed replay
validation. There is no cfg edit, new function root, exit-width declaration,
or shared compiler/runtime change. The normal Python analyzer manifest stays
byte identical and passes the lab manifest reader. Existing Mesen exit
evidence remains valid; this selection adds no independent hardware claim.

### Measured result and validation

The clean candidate passes the 4,816-frame long replay. Its frame comparison
file is byte identical to the accepted control, with zero bailouts, tuple
overflow or journal failures. The new entries commit guest CPU work on 71
and 80 invocations. The two existing entries retain their counts of 142 and
139. Interpreted work changes as follows:

| Exact routine graph | Before | After |
| --- | ---: | ---: |
| `01EBAE:M1X0` | 170,273 | 756 |
| `01ECCB:M1X0` | 246,143 | 1,113 |
| Whole long replay | 12,739,575 | 12,325,028 |

This removes **414,547 interpreted instructions, or 3.25% overall**, and
1,634,803 interpreted CPU cycles. It recovers **99.55%** of the remaining
interpreted work in the two target graphs. The earlier continuation pair's
remaining counts are unchanged. The accepted selection has **345 bodies,
38 instruction-timing entries and four separate continuation entries**.

Fresh normal cfg-only generation emits all five banks without cache reuse
and reproduces all seven candidate C files and the full manifest. The desktop
and headless builds pass. A harness linked to the normal generated library
passes the second, 2,125-frame primary replay with an identical frame file
and zero capture diagnostics. The normal headless 180-frame neutral check,
seven workflow checks, 31 patch checks, publication boundary and whitespace
checks pass. Shared source is unchanged, so the accepted shared C and Python
v2 suites are reused instead of repeated.

Three alternating rendered pairs of normal builds, after one warm-up each,
average 3.46647 seconds for the control and 3.42682 seconds for the candidate.
All three improve and their final images match. This suggests **1.14% less
local wall time**; it is a short measurement, not a sustained frame-rate claim.

### Evidence, limits and next step

Private evidence is in `captures/aot-alu-resume-20260929`, indexed by
`captures/aot-alu-resume-current.txt` and a follow-up record reached through
`captures/aot-recovery-current.txt`. It contains the preserved baseline,
rejected and corrected probes, clean candidate, exact continuation counts,
cost profiles, proposal review, fresh normal comparison and benchmark.
No bulk screen, full compiler suites, five-replay sweep, native analyzer
rebuild, new Mesen capture, Windows build or manual gameplay was repeated.
The unrelated UI file and lab work log remain untouched.

Do not chase the residual 1,869 instructions in this pair without evidence
of a useful benefit. The current profile confirms **3,183,161 interpreted
instructions** in the union of the six overlapping bank-00 graphs `00CE3F`,
`00CFE8`, `00D0DE`, `00D271`, `00D285` and `00D2A0`. This is about 25.8% of
the remaining interpreted work, not a predicted saving. Reuse the saved
membership map and classification of 515 unadopted bodies. The next bounded
task is to inspect operation and call blockers in this group, then choose
the smallest useful instruction-timing conversion and necessary dependencies.
Only add continuation selections after their generated blocks pass the
existing checks. Do not rescreen all bodies or reopen the parked `00804D`
prefix. Expand shared tests only if shared implementation changes.

## Previous checkpoint: resume compiled loops after events, 29 September 2026

### Observed cause and bounded change

The first direct compiled invocation of `01E5DF:M1X0` starts at frame 933,
S=`0FF9`. It yields at `01E69F:M1X0`, with the same S, at master clock
333372520 against IRQ deadline 333372512. The unwind is marked as a deadline
transfer. The next frame includes the NMI handler, followed by interpreted
`01E6DB:M0X0` and `01E6EA:M1X0` at S=`0FF9`. Thus the scheduler handoff is an
observed cause of lost compiled loop work. The bounded trace stops after
5,000 interpreted steps; it does not claim to follow the entire invocation.
The 1,000-frame diagnostic comparison matches the accepted control.

Patch 0031 adds an opt-in shared continuation table, separate from normal
subroutine dispatch. The title selects only `01E695:M1X0` within `01E5DF:M1X0`
and `01E7E5:M1X0` within `01E72F:M1X0`. Both are existing CFG block entries
with zero local stack depth. The generator checks exact widths, stack depth
and absence of predecessor IR temporaries. It rejects folded branches,
external JMP tails, adjusted entry stacks and LLE guards. Continuation wrappers
and their shared body stay together when a large bank is split.

Only the native whole-program scheduler uses these entries. A continuation
pushes no guest return frame. It uses live CPU state, executes the real
RTS/RTL pop, and yields the actual destination back to its interpreter owner.
Deadline yields use the same ownership path. No hidden host continuation is
kept across interrupts. Disabled bouncing and owner exclusions still apply.
Arbitrary instruction PCs, emulation mode and nonzero local stack entries
remain outside the contract.

This is a generation-policy change, with no cfg edit, added function root or
exit-width declaration. The normal Python analyzer manifest remains byte
identical to the 345-body baseline and passes the lab manifest reader. There
is no new Mesen exit claim, so no repeat Mesen capture is needed. The old
exact `02A3E2` evidence and all unrelated controls remain valid.

### Measured result

The primary 2,125-frame and long 4,816-frame replays match the saved frame
references with zero bailouts, tuple overflow or journal failures. The final
normal-library long run records 142 and 139 continuation invocations that
commit guest CPU work at the two selected entries.

Long-replay interpreted instructions fall from 13,591,991 to **12,739,575**.
That removes **852,416 instructions, or 6.27%**, and 3,186,214 interpreted CPU
cycles. The remaining interpreted work inside the two target graphs is only
3,926 and 3,834 instructions, down from 433,455 and 426,721. This recovers
**99.1% of their previously interpreted work**. It does not add bodies: the
accepted count remains **345**, with 38 instruction-timing entries and two
separate continuation entries.

Three alternating rendered pairs of normal builds, after one warm-up per
build, average 3.55492 seconds for the baseline and 3.48686 seconds for the
candidate. All three improve, about **1.9% less local wall time**, and all
final images match. This remains a short local measurement, not a sustained
match frame-rate claim. An earlier private-binary benchmark was mixed and is
not used: review found different generated-code trace build flags between its
control and candidate. Its raw results remain saved.

### Validation, evidence and next step

The final Python v2 suite passes 403/403, including a real split-bank compile
check. The shared C suite passes once for the runtime change. The final
focused continuation suite passes 300 event comparisons, with 327 counted
native entries, plus disabled-bounce parity and nine fail-closed checks.
It covers repeated deadlines, local saves, both M widths at one address,
compiled and interpreted callees, rewritten return frames, RTS/RTL,
SlowROM/FastROM, IRQ, NMI and refresh. The separate generation-cache test
passes and confirms that invalid selections do not replace accepted output.
After the shared C run, only test coverage and the split-bank grouping changed;
no runtime timing logic changed. The focused checks cover those final edits.

Fresh normal cfg-only generation reproduces all seven clean candidate C files
and the full manifest. Desktop and headless builds, the normal-library long
comparison, the 180-frame neutral check and all seven workflow checks pass.
The final split-bank grouping adds only comments to this unsharded title
output. Both desktop and headless binary hashes are identical before and
after that change, so the accepted gameplay and benchmark checks are reused.
All 31 patch checks and the final publication and whitespace checks pass.

Private evidence is under `captures/aot-resume-20260929`, indexed by
`captures/aot-resume-current.txt` and follow-up records reachable through
`captures/aot-recovery-current.txt`. It retains the initial bridge-only probe
that did not catch a direct compiled entry, the corrected trace, prototype,
clean candidates, synthetic failures and passes, cost profiles, lab manifest
review, benchmarks and normal-generation comparisons. A malformed synthetic
branch and test harness setup failures are preserved as diagnostics, not
runtime failures. No bulk screen, native analyzer rebuild, manual gameplay,
Windows build or five-replay sweep was repeated. Unrelated UI and lab changes
remain untouched.

The next bounded selection experiment can reuse this mechanism for the
already instruction-timed `01EBAE` and `01ECCB` loops. Their saved current
interpreted work is 170,273 and 246,143 instructions. Confirm the handoff and
choose balanced existing block entries, then measure actual continuation
execution. This needs selection and representative replay checks, not another
full shared suite unless shared source changes. Do not invent cfg roots or
exit contracts, and do not assume an entry is safe from its address alone.

The larger remaining group is the six overlapping bank-00 graphs identified
by the saved cost profile: `00CE3F`, `00CFE8`, `00D0DE`, `00D271`, `00D285` and
`00D2A0`. They already have bodies but use aggregate timing. Their saved union
was 3,183,161 interpreted instructions. Inspect the existing operation and
call blockers before choosing a bounded instruction-timing conversion.
Do not rerun the bulk classification or reopen the parked `00804D` prefix.

## Previous checkpoint: word operations and X saves, 28 September 2026

### Bounded change and evidence

Inspection confirmed that `01EE3E` is the early shared dependency of both
callers. All three already have normal-analysis roots and inferred exits.
The helper uses word direct-page SBC, ROL and LSR. The callers additionally
need word accumulator ROR and local X0 PHX/PLX brackets. The candidate therefore
included the helper from the start, reusing the previous batch's lesson.

The first ROM-free comparisons exposed low-byte-first writes for word memory
shifts. After correcting those, the remaining differences were the same byte
order issue in word X pushes. The interpreter writes the high byte first in
both cases. The private first and intermediate failure logs remain preserved.
Patch 0030 routes only these selected word operations through a high-byte-first
write helper. Ordinary word stores and unselected generation are unchanged.

Stack validation now counts two bytes for X0 PHX/PLX, alongside existing byte
PHA/PLA. Depth must agree at joins, never consume the caller's frame, and be
zero at calls, tails and returns. X1 PHX/PLX, word PHA/PLA, index-width changes,
and other word memory RMW forms remain excluded. The prior negative test for
word ROL now rejects the still-unsupported word ASL form instead.

The focused suite passes 100 complete interpreter comparisons and 198
event/resume comparisons, including flags, decimal subtraction, direct-page
penalties and address wrap, ordered writes and timestamps, mixed local stacks,
width changes, SlowROM/FastROM, IRQ, NMI and refresh. Nine unsupported stack or
operation paths fail closed. Both old title failures, at primary frames 934
and 936, are retained as historical evidence; the new candidate passes them.

### Title result and measured limits

The candidate has exactly three additions and no removals. Its full normal
Python analysis manifest is byte-identical to the 342-body manifest, and both
pass the lab manifest reader. The private selection proposal removes only
three `interpret_only` directives and selects the exact M1X0 variants. No new
root, function boundary or exit contract is declared. There is no new Mesen
claim, so no repeat Mesen capture is required.

The primary replay passes all 2,125 frames, with 50, 49 and 99 compiled first
instruction commits for `01E5DF`, `01E72F` and `01EE3E`. The longer replay passes
all 4,816 frames with 49, 49 and 98 commits. Counts are taken beyond deadline
and native-mode guards. Both runs have zero bailouts, tuple overflow or
journal failures. Existing controls were reused; no bulk screen was repeated.

On the longer input, interpreted instructions fall from 13,817,082 to
**13,591,991**, a reduction of **225,091, or 1.63%**. Interpreted CPU cycles
fall by 854,657. The new helper retains only 659 interpreted instructions, but
the two caller bodies still account for 433,455 and 426,721 interpreted
instructions. This is much less than their original full work moving to AOT.
Do not describe all execution within these bodies as compiled.

Two quiet alternating rendered pairs average 3.68878 seconds for 342 bodies
and 3.64718 seconds for the candidate, about **1.1% less wall time**. Both
pairs improve and all final images match. Capture and frame hashing are off.
This is a modest local indication, not a stable performance estimate or a
claim about sustained match frame rate. Guest instruction counts and host
wall time remain separate measures.

### Integration and next question

The final Python v2 suite passes 402/402 and the shared C suite passes once
for the final source, after useful work was measured. Fresh normal cfg-only
generation reproduces all seven candidate C files and the full manifest, with
345 bodies and 38 instruction-timing entries. Desktop and headless builds,
the normal-library 4,816-frame reference comparison, the 180-frame neutral
check, all seven workflow checks and all 30 patch checks pass. Publication
and whitespace checks pass.

The next useful question is why these now-compiled caller loops still do most
of their work in the interpreter. Reuse this profile and make one bounded
trace of the first compiled-to-interpreted transfer in `01E5DF`, recording
PC/M/X, stack state, event/deadline reason and the eventual loop continuation.
Hot remaining exact keys include `01E6EA:M1X0` and `01E6DB:M0X0`; the sibling
loop includes `01E838:M1X0` and `01E834:M0X0`. A scheduler resume is a hypothesis
to verify, not a new observed contract. Check whether a shared safe continuation
can recover meaningful work before adding more operation support. Do not turn
an internal PC into a new cfg function merely to make it dispatchable; stack
context and generated temporary values also matter.

Private evidence is under `captures/aot-word-20260928`, indexed by
`captures/aot-word-current.txt` and follow-up records reachable through
`captures/aot-recovery-current.txt`. It contains the baseline, candidate,
synthetic failure and passing logs, entry counters, opcode profiles, lab
selection review, benchmark and normal-generation comparison. The retained
classification has 515 unadopted variants, with only these three removed.
No manual gameplay, Windows build, native analyzer rebuild or five-replay
sweep was run. Unrelated UI and lab work remains untouched.

## Previous checkpoint: arithmetic loops and their helper, 28 September 2026

### Why the helper belongs in this batch

The first clean candidate compiled only `01EBAE` and `01ECCB`. It passed the
primary and longer replay, but removed only 5,580 interpreted instructions in
4,816 frames, or 0.037% of the baseline total. Its measured prefixes accounted
for 20 instructions per compiled entry. Generated code shows both routines
transferring scheduler ownership to interpreted `01EF24` at their early call.
The busy caller continuations then stay interpreted. These observations and
the generated transfer explain why simply enabling the pair has little value.
That candidate and its measurements are preserved; it was not promoted alone.

This was the concrete coverage gap that justified including `01EF24`. It adds
only byte direct-page ASL/ROL and SBC requirements to the first pair's SEC,
SBC immediate/absolute and byte direct-page INC/DEC timing. Patch 0029 enables
these forms in selected instruction timing. Existing lowering and emission
perform the operations. Word memory RMW, other addressing modes, index-width
changes and word stacks remain excluded. The existing scoped MMIO-read and
balanced byte-stack rules are unchanged.

The final focused tests pass 134 complete interpreter comparisons and 198
event/resume comparisons. They cover arithmetic flags, decimal subtraction,
operand widths, byte carry chains and wraps, direct-page penalties and wrap,
SlowROM/FastROM, writes and their timestamps, local stacks, IRQ, NMI and
refresh. Four unsupported memory-operation paths fail closed. The initial
smaller source passed 73 complete and 126 event comparisons before the helper
gap was measured. No behavioral failure was found in either title candidate.

### Validated execution and measured benefit

The final clean candidate generates exactly three additions, with no removals.
Its full normal Python analysis manifest is byte-identical to the baseline.
The lab manifest reader validates both files. Existing roots and inferred
exits suffice; the cfg change removes only three `interpret_only` directives.
There is no exit declaration, new boundary, private-profile dependency or
Mesen exit-contract claim. No repeat Mesen capture is needed for this selection.

The primary 2,125-frame replay passes with 78, 79 and 157 compiled executions
of `01EBAE`, `01ECCB` and `01EF24`, respectively. The longer 4,816-frame replay
passes with 139, 140 and 292. Private counters increment after each routine's
first generated instruction commits, beyond its deadline and native-mode
guards. Both runs have zero bailouts, tuple overflow and journal failures.
The accepted controls and saved old frame-85/87 failures were reused.

The longer replay falls from 14,932,604 to **13,817,082 interpreted instructions**:
**1,115,522 fewer, or 7.47%**. Interpreted CPU cycles fall by 4,367,405.
These are guest-work measurements. Some loop work still resumes in the
interpreter after scheduler events: the three decoded bodies retain 417,504
interpreted instructions. Do not claim that their entire profiled work moved
to AOT, or that their work-share reduction predicts host speed.

Two quiet alternating rendered benchmark pairs average 3.84294 seconds for
339 bodies and 3.78300 seconds for the clean 342-body candidate: about **1.6%
less wall time**. Both pairs improve and all four final images match. Capture
and frame hashing are disabled. This is a modest local indication, not a
stable benchmark or a sustained match-frame-rate claim. Most work in these
routines occurs early in this input.

### Integration and next step

Final Python v2 tests pass 402/402 and the final shared C suite passes. An
initial full pass also ran before the helper gap was identified; the source
extension required the final pass. For the next batch, measure the candidate's
useful work before starting the final full suites. No bulk screen was repeated.

Normal cfg-only generation reproduces all seven clean C files and the complete
manifest exactly, with 342 bodies and 35 selected instruction-timing entries.
Fresh desktop and headless builds succeed. The normal generated library passes
the 4,816-frame reference comparison; the installed runner passes the 180-frame
neutral check. All seven workflow checks and 29 patch checks pass. Publication
and whitespace checks pass. The shared change is committed and pushed as
`1314cd4`, with ordered patch 0029 retained for recovery.

The next bounded family remains `01E5DF` and `01E72F`, but first inspect their
early call to interpreted `01EE3E`. Do not repeat the pair-only mistake above.
Then assess the required X0 PHX/PLX and M0 accumulator ROR timing together
with any measured helper requirement. Their previous failures remain evidence;
two successful additions are not assumed. The parked 804D prefix, unrelated
exit-proof groups and historical bulk failures stay outside this batch.

Private evidence is under `captures/aot-alu-20260928`, indexed by
`captures/aot-alu-current.txt` and follow-up records reachable from
`captures/aot-recovery-current.txt`. It preserves both candidates, exact build
and replay commands, raw opcode counts, committed-instruction entry counters,
lab proposal review, benchmarks, test logs and the normal-generation comparison.
The historical classification now has 518 unadopted variants, with only these
three keys removed. No manual gameplay, Windows build, native analyzer rebuild
or five-replay sweep was run. The unrelated UI and lab changes remain untouched.

## Previous checkpoint: measured interpreter work, 28 September 2026

### Measurement and validation

One private runner profiled the saved 4,816-frame replay. It links the accepted
normal generated library and runtime objects, replacing only the interpreter
object with an instrumented private copy. Each completed opcode records its
exact entry PC/M/X, instruction count and CPU cycles, plus a 600-frame window.
M/X is sampled before execution. Interrupt service and parked idle time are
excluded. These are guest CPU work counts, not master clocks or host CPU time.
The instrumented run time is not a performance benchmark.

The capture contains 14,932,604 interpreted instructions and 63,657,896 CPU
cycles at 8,400 exact keys. Both totals equal the existing interpreter counters.
There are no profile overflows or changing opcodes at a recorded key. All 4,816
frame records are byte-identical to the accepted normal reference, with zero
bailouts, tier-2 tuple overflow or journal failures.

The normal Python cfg-root analyzer was run read-only. Its complete manifest
equals the accepted manifest. Instruction membership comes from its decoded
graphs, not nearest-address attribution. Shared graph regions are counted once
within each group. Different groups can overlap. Unknown-exit continuations
are absent from those graphs, so the two exit-proof group counts are lower
bounds for their caller bodies, not complete savings estimates.

### Ranked findings

| Measured group | Interpreted instructions | Share of all interpreted instructions | Meaning |
| --- | ---: | ---: | --- |
| Already emitted `00CE3F`, `00CFE8`, `00D0DE`, `00D271`, `00D285`, `00D2A0` | 3,183,161 | 21.32% | A compiled body does not ensure that all its execution stays compiled; graph overlaps removed |
| `01EBAE`, `01ECCB`, `01E5DF`, `01E72F`, all M1X0 entries | 2,486,791 | 16.65% | Four related interpreted routines, with saved timing failures and shared operation requirements |
| Seven M1X0 instructions in the `008031` call loop | 2,441,221 | 16.35% | Calls and the loop transfer still execute in the interpreter |
| Already emitted `07D8A5:M1X0` graph | 1,844,138 | 12.35% | Its generated scheduler guard can refuse compiled execution |
| Four M1X0 instructions at `00899F` through `0089A6` | 588,032 | 3.94% | Observed NMI clear loop, active through the end of the replay |
| Six callers blocked by `00EF63:M1X0`, proven prefixes only | 83,174 | 0.56% | Coverage opportunity; complete caller continuation cost is not measured by this subtotal |
| Five callers blocked by `00C3B6:M1X0`, proven prefixes only | 67,340 | 0.45% | Same limitation; retain the saved callee failure |

The six EF63 callers remain `02AEF7`, `02AF10`, `02B022`, `02B037`, `02B04C`
and `02B066`. EF63 is already emitted but lacks a proven exit; its decoded
tails include excluded `00F322`. The C3B6 group remains `02B0A8`, `00DBBD`,
`00D94B`, `028821` and `00DB7E`. These dependencies are confirmed against the
current normal analyzer, not only historical classification strings. Neither
group is promised to become fully available from one exit fact. Any later
exit declaration still requires the lab proposal and repeat-capture workflow,
exact entry scope, and normal-analyzer validation.

### Recommended next batch

Start with `01EBAE:M1X0` and `01ECCB:M1X0`. Together their non-overlapping
decoded bodies account for **1,432,697 instructions, or 9.59%** of this replay's
interpreted instructions. Both have proven normal-analysis entries and exits
and remain under `interpret_only`. Static inspection identifies shared missing
selected instruction timing for SEC, SBC immediate/absolute, and direct-page
byte INC/DEC. Their byte PHA/PLA support already exists. Inspect complete
generation and stack joins before claiming that this operation list is enough.

Use this bounded sequence:

1. Implement and compare only the required shared instruction forms. Cover
   arithmetic flags and widths, read-modify-write bus behavior, and event/resume
   boundaries. Preserve the scoped MMIO-read behavior established by C818.
2. Validate one clean two-routine candidate with the saved primary replay.
   Reuse its accepted control and the old failures at frames 85 and 87. Confirm
   actual compiled execution and reduced interpreter work. Keep their helper
   `01EF24` interpreted unless a separate measured need appears.
3. Measure a quiet rendered candidate/control comparison. These routines run
   at frames 84 through 240 in the longer input, so a gain here would not prove
   faster sustained match play. Do not translate the 9.59% work share into a
   speed claim. Stop expansion if the candidate does not reduce useful work.
4. Only after that result, assess `01E5DF` and `01E72F`. They add X0 PHX/PLX and
   M0 accumulator ROR requirements. Their measured work occurs around frames
   1350 through 1712. Retain the saved failures at primary frames 934 and 936;
   these are different replay coordinates. Four additions are a possibility,
   not an accepted batch size.

Run the full shared suites once for a final shared-source change, then promote
only executed, passing additions through fresh normal generation. This profiling
task itself changed no shared source and required no compiler suite or new
Mesen capture. Do not reopen the parked 804D prefix or remove scheduler guards
on the strength of frequency alone. The larger already-emitted groups need a
separate continuation/timing question, not additional body-count claims.

### Evidence and limits

Private evidence is under `captures/aot-cost-profile-20260928`, indexed by
`captures/aot-cost-profile-current.txt`. Follow-up records in the stack and
original recovery workspaces make it discoverable from
`captures/aot-recovery-current.txt`. It includes the instrumentation, exact
compile/link/run commands, raw tally, frame comparison, decoded membership,
normal-analyzer comparison, target instruction forms and recommendation.

The normal desktop and headless binary hashes remain unchanged. The original
interpreter source remains identical to the profiled source input. No cfg,
runtime, generation policy or patch changed. No bulk screen, second replay,
full suite, manual gameplay or platform build was run. Publication-boundary
and source-whitespace checks pass. The unrelated UI file and lab work remain
untouched. There is no host CPU attribution or demonstrated speed gain yet.

## Previous checkpoint: controller polling and byte stacks, 28 September 2026

### Finding and correction

The saved classification identified a 64-master-clock frame difference at
frame 74 when compiling `00C818:M1X0`. Adding selected PHA/PLA and DEX timing
alone reproduced exactly that failure. The first private candidate remains
preserved. Do not describe stack support alone as the fix.

A bounded 74-frame comparison used an interpreted control and the candidate.
Both entered at guest master clock 26,050,684 with identical recorded registers, stack and
beam position. Immediately after the first `$4212` read, at `00C81B:M1X0`,
both had charged four CPU cycles and 30 master clocks. The interpreted beam
was line 234, position 1042, and A was `AC80`; the compiled beam was at 1106
and A was `ACC0`. The extra HBlank bit was then hidden by AND #1. Immediately before RTL,
CPU and master totals still matched but the compiled beam remained 64 clocks
ahead. The frame scheduler consequently reached its boundary with 64 fewer
master clocks. These are observed differences, not an inferred exit contract.

The shared HVBJOY handler applies a legacy 64-clock polling tick when execution
is not owned by the interpreter. Selected compiled instruction timing already
advances the beam at opcode completion. Patch 0028 gives these reads a
saved/restored scope and suppresses the extra tick. Ordinary compiled reads
retain the tick; interpreter reads retain their existing behavior. This does
not change APU ownership, the title scheduler or an instruction's bus cost.

The same patch permits M1 PHA/PLA and DEX in selected instruction timing.
Generation requires equal byte-stack depth at joins, prohibits pulling the
caller frame, and requires depth zero at calls, tails and returns. Word stack
operations and general coroutine support remain excluded. Eighteen complete
synthetic comparisons and 432 event/resume comparisons cover nested stacks,
a balanced loop, high-byte preservation, index widths, wrap, SlowROM/FastROM,
IRQ, NMI, refresh and deadlines. Six malformed or unsupported paths are rejected.
A real SNES MMIO test checks HBlank and auto-joypad phase, byte and mirrored
word reads, scope restoration, and the unchanged legacy/interpreter paths.

### Title validation and measured result

With the MMIO correction, the primary replay matches all 2,125 frames, including
1,900 compiled `00C818:M1X0` entries, with zero bailouts, tuple overflow or
journal failures. The saved 338-body control and classifications are reused.
Only the short diagnosis needed a new disabled control. No bulk screen ran.

Normal Python cfg-root analysis reaches the new exact entry and infers its
exit. The lab manifest reader validates both manifests. Only the C818 node
and its inferred exit change. The private proposal cites the saved entry
observation, bounded failure probe and passing replay. It adds no exit
declaration, new function partition or exclusion. No Mesen contract proposal
or fresh Mesen observation was required.

Two quiet alternating rendered benchmark pairs average 3.83693 seconds for
338 bodies and 3.83619 seconds for the clean candidate. The difference is
about 0.02%, effectively unchanged, and both pairs do not agree on direction.
All four final images match. Capture and frame hashing are disabled. This is
a correctness and coverage result, **not a measured speed improvement**.
Do not use the 1,900 compiled entries to claim a meaningful performance gain.

### Integration and next step

The Python v2 suite passes 402/402. The final shared C suite passes. Its first
run found an old assertion that rejected all PHA/PLA pairs; that test now
rejects the still-unsupported word pair instead. The new failure-path tests
continue to reject unsafe byte stacks. The focused MMIO and stack checks pass.
Normal cfg-only generation reproduces all seven clean C files and the full
manifest exactly, with 339 bodies and 32 selected instruction-timing entries.
Fresh desktop and headless builds succeed. A runner linked to the normal
generated library matches all 4,816 reference frames with zero diagnostics;
the installed runner passes the 180-frame neutral check. All seven workflow
checks and all 28 patch checks pass. The publication boundary and source
whitespace checks pass.

The original 860-body discovery set now has 521 variants outside adoption.
The saved classification is carried forward with only the accepted C818 key
removed. Historical rejection strings describe the compiler at capture time;
recheck a selected candidate instead of rerunning the entire screen.

The next priority is one bounded cost profile of the current build on the
existing longer replay. The 804D prefix and C818 results show why call-gap
frequency alone is insufficient. Reuse the existing interpreter instruction
and cycle counters and, if needed, a private per-PC tally or an available host
profiler. Guest-cycle counts and host CPU cost must remain separate measures.
Do not repeat the unavailable macOS sample attach without a concrete setup
change. Choose the next caller group or loop from that evidence before adding
more instruction support. The old D45C frame-1505 failure and wider nine-address
failure group remain unmodified evidence, not newly established priorities.

Private evidence is under `captures/aot-stack-20260928`, indexed by
`captures/aot-stack-current.txt`. Follow-up records in the earlier recovery
workspaces make it discoverable from `captures/aot-recovery-current.txt`.
The workspace preserves the 338 build, both candidates, first failure, exact
entry/after-read/return probe, source/build commands, proposal, tests and benchmark.
No new manual gameplay, Windows build, native analyzer rebuild or five-replay
sweep was performed. The lab repository and unrelated UI work were untouched.

## Previous checkpoint: index-loop timing, 28 September 2026

### Accepted result

`00CE11:M1X0` uses selected instruction timing for CPX and INX. The old
bus-policy failure at frame 740 is absent in the primary and longer saved
replays. The addition executes 1,900 times in 2,125 frames and 4,594 times in
4,816 frames. Both captures match all saved comparison fields, with zero
bailouts, tuple overflow and journal failures. No bulk screen was repeated.

Patch 0027 permits CPX immediate, direct-page and absolute, plus INX. It changes
only the selected instruction validator and synthetic tests. Existing emitted
operations perform the comparison and increment. Fifty complete interpreter
comparisons and 716 event/resume pairs cover both register widths, comparison
flags, index wrap, direct-page penalties, repeated stores, deadlines, refresh,
NMI and IRQ. An initial fixture failure used a masked IRQ with a scheduler
encoded for M1; the corrected fixture enables IRQs. That failure was not a
title or compiler regression. Both logs are retained privately.

The cfg adds the observed exact root. It adds no exit declaration, boundary
split or new exclusion. Normal Python analysis infers the new node's exit.
The lab manifest reader validates both manifests; only the CE11 node and its
inferred exit change. The private proposal records the exact selection and
replay evidence. This is a title selection review, not acceptance by the lab's
exit-M/X proposal rule. No new Mesen fact was needed.

A quiet rendered benchmark averages 3.855 seconds for published 337 and
3.756 seconds for the clean 338 candidate, about **2.6% less wall time**.
Both alternating pairs improve and all final images match. Capture and frame
hashing are disabled. This is a small local indication, not a stable benchmark
or a desktop frame-rate claim. An earlier set overlapped another replay and
is explicitly rejected; do not reuse it as performance evidence.

### Parked 00804D experiment

A bounded prefix ending at `008053` passed both saved replays. It executed
348,609 times in the longer replay; the setup branch transferred to the
interpreter four times. A separate branch counter verified that rare path.
Call-gap sightings fell from 443,259 to 94,513, but rendered wall time improved
only about 0.94% across two alternating pairs. That does not establish a useful
speed gain. High entry frequency alone was a poor predictor of benefit here.

The candidate also removed `00801E:M1X0`: exposing the compiled prefix made
normal analysis require its unresolved setup-path exit proof. No exit contract
was invented to restore the caller. The lab review records that removal and
no exit-fact changes. The full 804D routine still has structural poison and
unknown callee exits. The experiment did not solve that decoding question.

The prefix cfg and external conditional-branch timing extension are **not
promoted**. Their source diff, synthetic test, clean binaries, passing captures
and benchmark remain under `parked-prefix-source` and related private folders.
Tracked 804D behavior is unchanged. Do not repeat this experiment without a
specific cost or correctness question that the saved evidence cannot answer.

### Integration and remaining work

The Python v2 suite passes 402/402 and the shared C suite passes once for the
final CPX/INX change. All seven patch/workflow checks pass. Normal cfg-only
Python generation produces 338 bodies and reproduces all seven clean C files
and the full manifest exactly. The accepted policy selects 31 instruction-timed
entries. The existing bus-cost policy is unchanged. Fresh desktop and headless builds
succeed. A runner linked to the normal generated library matches all 4,816
reference frames with zero diagnostics. The installed headless runner passes
the 180-frame neutral check. All 27 patches verify; publication-boundary and
source whitespace checks pass. The exported patch contains normal diff context
markers, which are not source whitespace errors.

The saved classification now has 522 of the original 860 variants outside
normal adoption. Other historical failures retain their previous status.
The next bounded instruction question is `00C818:M1X0`, starting from its
saved frame-74 clock discrepancy and PHA/PLA plus DEX requirements. Keep stack
balance and event resumes in scope. Do not broaden this into a bulk screen.
For larger speed gains, obtain a cost profile before following entry counts.
One short macOS `sample` attempt could not attach to our headless process;
no usable cost profile was produced and no profiler setup work was pursued.

No new manual gameplay, Windows build, five-replay sweep, native analyzer
rebuild or Mesen capture was performed. The analyzer itself is unchanged.
All ROM-derived evidence is private under `captures/aot-next-milestone-20260928`,
indexed by `captures/aot-next-milestone-current.txt` and follow-up records reached
through `captures/aot-recovery-current.txt`. Raw prior evidence is unchanged.
Unrelated `recomp-ui/.DS_Store` and lab work were preserved.

## Previous checkpoint: four hot roots, 28 September 2026

### Publication

The four dependency commits are `3556f36`, `4c0f622`, `fd957ad` and
`b1d8ece`, on `codex/super-tennis-runtime`. Repinning changes no source or
accepted executable. The existing passing runtime evidence is reused.

### Result and measured effect

The combined 337-body candidate and the final normal build each match every
saved comparison field over the 4,816-frame focused replay. There are exactly
four additions and no removals. The instrumented candidate records:

| Exact addition | Compiled entries in 4,816 frames |
| --- | ---: |
| `0088C1:M1X0` | 348,598 |
| `019D3B:M1X0` | 348,590 |
| `01A14A:M1X0` | 348,582 |
| `02858C:M1X0` | 348,598 |

Call-gap sightings in the same replay fall from 1,838,644 at 333 bodies to
443,259 at 337 bodies: 1,395,385 fewer, or **75.9%**. These are capture counts,
not a percentage of CPU time or instructions. The bridge's separate
`total_tier_hits` counter remains 4,546; do not confuse these metrics.

A small local benchmark with rendering enabled averages **4.744 seconds**
for the 333-body control and **3.775 seconds** for the 337-body build,
**20.4% less wall time**. Two pairs ran in alternating order, without capture
or per-frame hashing. Both policies used the same current runtime objects,
normal headless host and compiler flags. The control rebuilt the preserved
333-body generated C. All four final images have the same hash. This is an
indicative local result, not a stable benchmark or desktop frame-rate claim.

### Shared changes and exact selection

Patch 0025 adds `interpret_only <pc24>` to the manifest-driven pipeline.
Unlike `force_lle`, it permits normal reachable-code analysis, then suppresses
AOT emission at the address and its valid LoROM mirror for every entry width.
It asserts no exit contract, creates no analysis root and excludes no other
address implicitly. Both analyzers keep their existing exact proof rules and
unresolved-path checks. The common emitter applies the exclusion, including
when it consumes a native manifest. The legacy emitter rejects this directive
because it cannot honor the policy. Synthetic tests cover changed widths,
transitive proofs, mirrors, unresolved exits, force-LLE precedence, parser
validation, null dispatch slots, cache invalidation and analyzer parity.

This is deliberately separate from the exact `exit_mx_for` declaration.
`interpret_only` controls execution selection across widths; it does not
broadcast a width fact. Analysis manifests may still call these nodes
`aot_eligible`. Count actual emitted bodies or dispatch slots, not eligible
analysis nodes, when reporting adoption.

The former analysis exclusions at `00C69D`, `01911F` and `019023` are now
emission exclusions. They remain interpreted. This restores static proofs
for the two caller groups without copying guessed or unobserved exit widths
into cfg. Thirty-four other newly reached dependencies are also explicitly
kept interpreted. In particular, `019DB9`, `0285B9`, `0285E4` and `02860F`
have no generated body. `0285E4` still has no saved execution evidence.
`02A3E2` retains its existing `force_lle` and exact M1X0 declaration.

Patch 0026 adds ordinary direct JML to selected instruction timing. The
instruction commits its clocks, changes PB and checks for an event at the
destination before either tier executes. It preserves the guest return frame.
The real-interpreter differential tests cover compiled, missing and disabled
tail targets, same-bank and cross-bank transfers, SlowROM/FastROM, both
register widths, skipped tails, deadlines, IRQ, NMI and refresh. Each of two
bus-option modes passes 48 complete and 3,168 event/resume comparisons.
Indirect jumps, return trampolines, dispatch helpers and external short
branches remain outside this selected support.

Normal generation now selects 30 instruction-timed entries, including the
four new roots. The prior bus-cost selection is unchanged. The final seven
C files and full analysis manifest equal the clean private candidate byte
for byte. Normal generation uses tracked cfg and policy, no private profile
and no runtime deny guard. Fresh desktop and headless builds succeed.

### Focused evidence and checks

The saved 333-body controls, classifications and previous passing probes
were reused. No bulk screen was repeated. The two-root clean primary replay
passes with 127,866 `0088C1` and 127,850 `02858C` compiled entries. Adding
`019D3B` passes the same 2,125 frames with 127,863 compiled entries. A separate
clean `01A14A` candidate passes with 127,867 entries. The final combined
4,816-frame capture above covers all four. Every run reports zero bailouts,
tuple overflow and journal failures.

The old `019D3B` frame-85 failure is absent under the corrected ownership
path and instruction timing. This pass did not isolate which change removed
it; the old nested-guard explanation remains a hypothesis for that historical
failure. The prior `01A14A` frame-194 failure is also absent. Do not reinterpret
all historical screening failures as fixed or repeat the bulk screen.

The Python v2 suite passes 402/402. After adding the legacy-emitter rejection,
its focused cache/rejection test passes again. The shared C suite passes,
including the final direct-call and JML tests. The 22 native cfg tests pass;
the native release analyzer was rebuilt before the two-analyzer parity check.
All seven patch/workflow tests pass, including reconstruction from supported
pins and CRLF application. All 26 patches verify. Final normal generation
matches the clean candidate, the normal-library replay matches 4,816 frames,
and the installed headless runner passes the 180-frame neutral check.
The publication boundary and title/dependency diff-whitespace checks pass.

Private evidence is under `captures/aot-hot-roots-20260928`, indexed by
`captures/aot-hot-roots-current.txt`. It includes generation inputs, the
preserved pre-change source for ordered patch export, focused and combined
results, exact counters, clean/normal equality, final binary hashes, benchmark
commands, test logs and an updated classification. Follow-up records in both
earlier recovery workspaces point here. Raw earlier evidence is unchanged.
The original 860-body discovery profile now has **523** variants outside
normal adoption. This count reuses the historical classification with the
four new accepted keys removed.

### Remaining limits and next useful work

No fresh manual gameplay, Windows build or full five-replay sweep was run.
Acceptance is bounded by the primary and focused saved inputs. The unchanged
unrelated `recomp-ui/.DS_Store` and lab work log are preserved. The user approved publication of this checkpoint before the next milestone.

The largest remaining call-gap count in the focused replay is `00804D:M1X0`
at 348,746. Its current decode has structural poison and several unproven
callees; it is not a safe next cfg promotion. A separate decoder/boundary
investigation would be needed before considering it.

For the next bounded timing work, reuse the saved `00C818:M1X0` frame-74
clock failure and `00CE11:M1X0` frame-740 WRAM failure. Their historical
instruction blockers are PHA and CPX respectively. Both have 4,594 call-gap
sightings in the new focused replay and 17,240 across the old five-replay
classification. Inspect their required instruction families and first saved
failure before extending support. These are smaller targets than the four
roots just adopted. Do not reopen the `07AC47` coroutine work without a new
concrete ownership or boundary question.

## Previous checkpoint: shared compiler work, 28 September 2026

### Exact exit declaration and promotion

Patch 0023 adds `exit_mx_for <pc24> <entry_m> <entry_x> <exit_m> <exit_x>`.
It stores explicit exact facts separately from inferred routes, so inference
refresh cannot erase them. Both cfg parsers and both analyzers support it.
Explicit exact facts take priority over legacy address-wide declarations and
inference. Invalid addresses, widths and argument counts are rejected.

Tracked bank02 cfg now contains `exit_mx_for 02A3E2 1 0 1 0`, with its evidence
rationale. `force_lle 02A3E2` remains in place. The declaration uses the frozen
repeat Mesen evidence and proposal described below. No new observation or
exit fact was invented. The derived scope review declares only
`02A3E2:M1X0` and its `82A3E2:M1X0` ROM alias. The other three entry widths
have no new exit declaration.

The normal Python analyzer produces exactly four additions, with no removals:
`02AD46:M1X0`, `02AD56:M1X0`, `02AD5E:M1X0` and `02AD66:M1X0`. Each executed
2,661 times in the saved passing 4,816-frame candidate replay. The callee has
no emitted body. All seven C files equal that tested candidate byte for byte;
the exact manifest contains only the two supported callee exit keys.

The lab manifest comparison was reused against the exact overlay. Its report
currently hard-codes a legacy cfg-scope label, so the actual exit-key set was
checked separately and retained with the derived proposal. Raw captures and
the frozen legacy proposal were not changed. Synthetic tests cover exact
variant isolation, ROM aliases, interpreted callees, Python/native parity,
precedence, inference refresh and output-cache invalidation.

### Shared timing work

Patch 0024 extends selected instruction timing to ordinary direct JSR/JSL and
immediate ORA. Each call pushes its real guest return frame, commits the
instruction clocks, then checks the scheduler before entering the callee.
A missing exact compiled callee in scheduler mode transfers control to the
owning interpreter, which also owns the caller continuation. This avoids a
nested bounded interpreter running past the scheduler deadline.

Cross-bank synthetic tests found a separate PB defect: generated JSL wrappers
restore the caller bank while propagating a deadline unwind. The owning
interpreter now restores PB from the saved resume address before returning to
the scheduler. The fix is general and does not use title addresses.

These changes do not supply missing exit proofs. Indirect and special calls,
external branches, index-width changes, stack operations, memory RMW, RTI and
block moves remain outside the selected instruction mode. The normal title
policy still selects the same 26 validated leaves and the same bus-cost keys.
No experimental hot root is promoted.

### Focused hot-root results

`0088C1:M1X0` reproduces its saved bus-timing failure at frame 212. The
all-additions-disabled control passes. At entry, both tiers have master clock
75,354,172, stack `0FFC`, native M1X0 and beam `224:1356`. The interpreter
stops after LDA at master 75,354,204, before `0088C4`. Block-timed AOT also
executes the taken BEQ and stops at `0088D6`, master 75,354,226. The extra
22 clocks and delayed beam update reproduce the saved first failure. This is
an observed instruction-boundary error, not a hypothesis about total bus cost.

Selected instruction timing passes the complete 2,125-frame primary replay.
The instruction-timed experiment records 127,866 compiled `0088C1:M1X0` entries,
with zero bailouts, tuple overflow or journal failures. The six other new
bodies in the 340-body diagnostic build remain runtime-disabled. Their
execution and exit contracts are not accepted by this experiment.

Normal analysis still blocks clean `0088C1` promotion: its callee `019DB9`
needs an exit proof, and that routine reaches interpreted `00C69D`. The
primary replay never enters `019DB9`, so it cannot justify an exit declaration.
The private experiment temporarily exposes `00C69D` for analysis and disables
it at runtime. Normal cfg retains its original exclusion. Do not copy this
private analysis selection into tracked cfg.

A second focused candidate for `02858C:M1X0` generates five additions:
`00C7A6`, `02858C`, `0285B9`, `0285E4` and `02860F`, all M1X0. The disabled
control passes 2,125 frames. Enabling only instruction-timed `02858C` removes
its old frame-94 failure but exposes a failure at frame 1079 after 65,599
compiled entries. The control stops inside `02860F`; the candidate overruns
into the caller continuation. The diagnostic deny guard used a nested
bounded interpreter for its compiled-but-disabled callee. Adding that path to
the synthetic test reproduced 42 event/resume failures. Patch 0024 now transfers
a disabled callee to the owning interpreter. The expanded tests pass.

Fresh generation with the corrected guard passes both the disabled control
and the complete primary replay with only `02858C` enabled. It executes
127,850 compiled root entries, with zero bailouts, tuple overflow or journal
failures. The four new dependency bodies remain disabled. This establishes a
second passing hot-root experiment, not clean promotion of that group.
The last `0088C1` run predates the guard correction; its callee was never
entered, so this later change does not affect the exercised root path.

The guard defect can cause false failures when a compiled caller reaches a
disabled callee. Preserve the old failures as observations under the old
screening policy. Do not reclassify every failure as fixed or repeat the bulk
screen. Revisit a relevant saved failure only when its path uses this guard.

### Checks and evidence

The Python v2 suite passed 398/398 tests. The native Rust suite passed after
the cfg change, and the native release analyzer was rebuilt before the parity
test (61 native tests passed). The expanded call test compares 48 complete
executions and 3,168 scheduler stop/resume cases in each of two bus-option
modes. The shared C suite and all seven patch/workflow tests passed. Final
normal regeneration reproduces the exact 333-body manifest and all seven
candidate C files. The installed desktop and headless builds pass compilation.
A private runner linked to the freshly built normal generated library and
runtime matches every saved field across the 4,816-frame replay, with zero
bailouts, tuple overflow or journal failures. The installed headless runner
also passes the 180-frame neutral check. The final guard-only edit does not
change any normal generated C or runtime binary, so this replay is reused.
Patch verification, the publication boundary and diff whitespace checks pass.

Private evidence is under `captures/aot-shared-progress-20260928`, indexed by
`captures/aot-shared-progress-current.txt` and by a follow-up record in the
original recovery workspace. It contains exact proposal scope, generation
inputs, candidate comparisons, test logs, preserved 333-body baseline, first
failure probes, per-entry counters, final normal replay and executable hashes.
Earlier captures remain available through `captures/aot-exit-02a3e2-current.txt`
and `captures/aot-recovery-current.txt`.

No bulk screen was repeated. No new manual gameplay check, Windows build or
whole-game claim was made. The unrelated `recomp-ui/.DS_Store` and the existing
lab work log remain untouched. The original 860-body profile now has 527
variants outside normal adoption; the saved 531-row classification should be
reused with the four promoted keys removed, not rebuilt from scratch.

### Next high-value work

The next shared capability should separate callee exit analysis from the
choice to emit that callee as AOT. A routine that must remain interpreted can
still have a statically provable exact exit, but the current normal pipeline
loses required proofs when those dependencies are excluded. Establish those
proofs without weakening unresolved-path checks, and validate the normal
Python analyzer as well as any native implementation. Exact repeated Mesen
observations remain an alternative when a contract can be bounded reliably.

Apply that capability first to the now-passing `02858C` and `0088C1` groups,
retaining their unsupported dependencies in the interpreter. Generate clean
candidates and confirm actual execution before promotion. Do not compile
excluded callees merely to obtain their callers' exit proofs. `0285E4` has
no saved execution evidence and must not be counted as validated.

After that, investigate `019D3B` and `01A14A` using their saved first failures.
Their failure shapes differ; the two tests above do not prove a common fix.
The four frequent roots account for most recorded remaining call-gap sightings,
but nested sightings overlap and are not measurements of CPU time. Judge the
next batch by executed coverage and measured runtime, as well as body count.

## Initial exit experiment: 28 September 2026

The focused `02A3E2:M1X0` exit experiment is complete. The accepted build
remains at **329 AOT bodies**, title commit `75a4410` and snesrecomp commit
`60058e9`. Both commits were already published. No cfg, generation policy,
compiler source or installed executable changed during this experiment.
The unrelated `recomp-ui/.DS_Store` and existing lab work-log changes were
preserved.

### Repeat Mesen evidence

The lab used MesenCE 2.2.1, its existing adapter and the saved primary replay
`8426d0e5`. ROM identity, adapter/probe/replay digests, commands and raw wire
records are retained with each capture. Two identical bounded captures
observed the first call from `009E71`, entry at `02A3E2`, RTL at `02A417`
and continuation at `009E75`. All four points had native `M1X0`. Each run
contained exactly one observation per role, and the comparable event digests
matched. The lab's existing proposal rule produced and froze
`exit_mx_at 02a3e2 1 0` from that pair.

A separate repeat pair observed the relevant call at `02ADCA` and continuation
at `02ADCE`, at Mesen frame 1908. Entry, return and continuation again had
native `M1X0`; the stack pointer at continuation matched the caller's pointer
before JSL. Both full 12-event streams matched. They include four earlier
callee visits and therefore cannot themselves satisfy the proposal rule's
single-observation requirement. They support the caller-specific check without
filtering or rewriting raw evidence. An initial probe of a different caller
did not reach every point and was not used for the proposal. A separate
three-event discovery probe identified the first actual continuation.

Static inspection under the observed `M1X0` entry decoded 23 instructions,
one RTL, no calls and no operations that change register widths. This supports
width preservation for that decoded path. It is a static deduction, distinct
from the repeat observations. No exit was established by the bounded decoder
checks for `M0X0`, `M0X1` or `M1X1`.

### Analyzer and replay result

The isolated lab validation used the title's pinned Python analyzer with
`--all-cfg-roots`. Normal title generation explicitly uses the same Python
backend with `--cfg-roots`; a native analyzer comparison was not required.
The lab baseline manifest equals the accepted normal manifest exactly.

The candidate retains `force_lle 02A3E2`. It makes all four callers eligible:

| Exact addition | Executions in the focused replay |
| --- | ---: |
| `02AD46:M1X0` | 2,661 |
| `02AD56:M1X0` | 2,661 |
| `02AD5E:M1X0` | 2,661 |
| `02AD66:M1X0` | 2,661 |

Fresh private generation with the normal timing options, proposed cfg and no
profile produces 333 bodies, with no accepted bodies removed. Its full manifest
equals the lab candidate manifest. Only bank02 and dispatch C change;
`02A3E2` has no generated body.

One relevant 4,816-frame replay, `0bb33b16`, matches every recorded field in
the saved accepted 329-body comparison. All four additions execute as listed
above. The callee remains interpreted, including four calls from `02ADCA`.
Bailouts, tuple overflow and journal write failures are all zero. Execution
counts come from a separate private diagnostic copy with four counters after
entry deadline checks and no runtime selection guards. The clean generated
files were preserved unchanged. This is bounded regression evidence, not a
whole-game or within-instruction timing proof.

### Promotion limit and next step

**No additions were promoted.** The legacy directive seeds the same exit for
all four entry variants at both `02A3E2` and its `82A3E2` LoROM mirror. The
manifest explicitly gains all eight exit facts. The lab correctly returns
review-required status despite reporting an improvement with no observed
analysis regression. The other three entry widths have no supporting runtime
evidence or established static exits. Passing caller execution does not justify
those broader contracts.

The next compiler question is how to express the observed exact entry-variant
exit, or infer exits safely for interpreted callees, while retaining
`force_lle`. Do not promote this address-wide overlay merely because the current
four callers pass. A compiler change belongs in snesrecomp and requires its
normal source-change checks and integration workflow. Reuse the frozen captures,
proposal, classification and passing replay evidence for that work.

Private evidence is indexed by `captures/aot-exit-02a3e2-current.txt`.
The original recovery workspace also contains `exit-02a3e2-followup.json`,
so it remains discoverable through `captures/aot-recovery-current.txt`.
The new workspace preserves the accepted cfg, generated output and executable
hashes; capture references and digests; static scope review; lab proposal and
validation paths; clean generation inputs; diagnostic build commands; exact
execution counts; and replay results. Lab work item: `lab-2z9`.

The 22-patch verification, lab evidence ingestion, repeat comparisons, isolated
analysis, normal-policy candidate generation and focused replay passed. The
publication boundary and diff whitespace checks passed. Accepted cfg, generated
files and executable hashes were checked again and remain unchanged. No bulk
screen, compiler suite, extra replay, manual gameplay or Windows build was run.
There was no promotion, so fresh installed normal generation and a new neutral
smoke run were not needed.

## Accepted recovery: 13 September 2026

## Recovery and evidence

The 40-body handoff set was derived from the saved guarded selection and clean
manifest, retaining exact M/X identity. Two excluded tail routines, 019A8B M1X0
and 019ACD M1X0, blocked exit proofs for a large caller group. Enabling those two
tested bodies recovers nine callers in clean generation. Further callers expose
additional excluded tail paths and remain interpreted. No exit-width contracts
were added.

A dependency screen retained the accepted 309 bodies and generated 42 additions.
Its all-additions-disabled control matched the primary replay. Six dependencies
each reproduced a failure under the tested bus-cost policy: 00BF26, 00C3B6,
00C69D, 00F8A0, 01941E and 02A3E2, all M1X0. These are separate failure records,
not a claim that every caller is faulty. The surviving merged selection passed
the primary and four wider recordings. Clean recovery retained the two tails
and nine callers whose required proofs and execution checks were complete.

The remaining-profile classification then identified 34 variants with saved
replay sightings outside the original executed set. Screening one relevant
wider replay recovered 02E98C M1X0 and 02EC0B M1X0 for clean generation.
00FEC0 passed guarded execution but required two additional unvalidated callees
in clean generation, so it was not included. Two new singleton failures and
one nine-address failure-associated group remain recorded privately.

## Selected instruction timing

The frequent leaf 01EDF2 M1X0 failed the bus-cost comparison at frame 935.
A bounded caller-side probe first observed identical entry state, returned
registers, WRAM and CPU/master totals, but a 12-clock beam lag at return.
The failing frame stopped at 01EE33 in AOT instead of 01EE2D in the control,
with 22 more master clocks. These observations support an instruction-scheduling
experiment rather than another adjustment to total bus costs.

Patch 0022 enables tested AND, EOR, BIT, accumulator INC, CLC, DEY, TYA, XBA and
REP/SEP without index-width changes in the existing selected timing path.
Index-width narrowing remains excluded after a synthetic comparison exposed
uncleared index-register high bytes in the existing generic SEP emission.
The patch does not enable calls, stack operations or memory RMW in this mode.

01EDF2 then passed the primary replay. A static scan found 49 eligible leaves
among 182 remaining leaf candidates. Preserving an already passing bus-timed
selection left 48 leaves in the instruction-timing experiment, including
01EDF2. One wider replay passed with six further new leaves exercised:
00B1F6, 00BD44, 00BE5F, 01E87F, 02906B and 07CD2B, all M1X0.
Only these six and 01EDF2 enter the clean selection. Passing runs with no new
execution were not counted as validation.

## Validation and handoff

The final clean candidate has 20 additions: eleven from dependency recovery,
two from wider-replay discovery and seven instruction-timed leaves. It retains
every accepted body and uses no private profile or runtime selection guards.
All 20 have exact entry execution evidence from the relevant guarded comparisons.

After the maintainer requested fewer repeated checks, validation used a primary
replay during the timing change and one relevant wider replay at the combined
clean milestone. The latter compares all recorded fields across 4,816 frames,
with no bailouts, tuple overflow or journal write failures. Earlier unchanged
controls and relevant coverage observations were reused.

The Python v2 suite passed 392/392 tests. The shared C suite passed once after
the generator change, including 29 new complete synthetic comparisons and 38
scheduler comparisons. The seven setup/workflow checks passed. A fresh run of
the normal regeneration script, with the proposed cfg and policy, reproduces
the full manifest and all seven generated C files exactly. The lab's manifest
reader validated both manifests. macOS Apple Silicon was tested; Windows was not.

The focused 4,816-frame comparison removes 110,499 call-gap sightings. Two
alternating benchmark pairs on the 2,125-frame headless workload average 2.597
seconds for the accepted control and 2.197 seconds for the candidate, about
15.4% less wall time. Rendering is enabled; capture and frame hashing are off.
This is an indicative local result, not a stable benchmark or desktop frame-rate
measurement.

The private workspace is identified by `captures/aot-recovery-current.txt`.
It contains the preserved 309-body baseline, exact dependency map, classification,
immutable failure runs, focused timing probe, generation inputs, reproducibility
record, benchmark and preserved desktop candidate. The normal desktop is
`build/super_tennis`. Fresh normal generation reproduces every generated file.
The installed desktop has identical code and data to the candidate; the only
differences are its Mach-O UUID and the corresponding first-page signature hash.
All signature page hashes and both code signatures were verified. The normal
headless build completed the 180-frame neutral smoke check. The publication
boundary check and diff whitespace check passed.

## Remaining work at the 329-body checkpoint

The promoted selection leaves 531 variants outside normal adoption from the original
860-body profile set. A count outside AOT does not establish a bug. The saved
classification distinguishes prior failures, caller proof gaps, wider exclusions
and unexercised candidates. Thirty-one of the original 40 handoff targets remain
outside this selection. The nine-address wider failure group has not been
minimized. Four frequent call-bearing roots remain separate investigations.

Use the saved classification and first failures to choose the next bounded
question. Reuse the accepted selection and saved controls. Do not repeat
the original bulk screen, count unexecuted bodies as accepted coverage, or infer
exit contracts from entry widths.
