# AOT recovery

The accepted working tree now generates **337 AOT bodies**. Four frequent
M1X0 roots are promoted through normal generation: `0088C1`, `019D3B`,
`01A14A` and `02858C`. Their unvalidated dependencies remain interpreted.
The 337-body checkpoint is committed in the title commit containing this
report and pins published snesrecomp `b1d8ece`. Dependency patches 0023
through 0026 are committed in order and exported for recovery. The previous
published baseline was title `75a4410` and snesrecomp `60058e9`.
This checkpoint supersedes the 333-body checkpoint below and the older
handoff in `AOT_BATCH_SCREENING.md`.

## Latest checkpoint: four hot roots, 28 September 2026

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
