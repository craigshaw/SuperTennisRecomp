# AOT recovery

The accepted normal build generates **345 AOT bodies**. The previous
342-body checkpoint is title `b4b2bae` with snesrecomp `1314cd4`. This batch
adds `01E5DF`, `01E72F` and their shared helper `01EE3E`, all M1X0, with no
removals. The new shared pin is `f4451a3`, described in `SNESRECOMP_PATCHES.md`.
This report supersedes the older handoff in `AOT_BATCH_SCREENING.md`.

## Latest checkpoint: word operations and X saves, 28 September 2026

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
