# Bulk AOT screening

The accepted normal build has 309 AOT bodies, an increase of 118 from the
191-body checkpoint. The maintainer accepted gameplay on 12 September 2026.
Tracked cfg and generation policy reproduce the tested candidate without a
private profile or runtime selection guards.

## Current result

The 12 September bulk pass screened the existing 669 additional exact variants
at 668 addresses. It retained the nineteen accepted instruction-timing entries
and the accepted bus-cost setting for 019B33 throughout.

The final candidate was generated from a private cfg proposal without a profile
or runtime selection guards. All five saved replays agree with the accepted
191-body control on every recorded field across 18,354 frames. There are no
bailouts, tuple overflows or journal write failures. Its 118 additions remove
44,643 call-gap sightings in the 2,125-frame primary replay.

Five alternating control/candidate benchmark pairs, with rendering enabled
and capture and frame hashing disabled, give median times of 2.894 seconds
and 2.685 seconds. This is approximately 7.2% less wall time, or 7.8% greater
throughput, for this headless workload on this machine. It is not a desktop
frame-rate measurement or a promise about other game modes.

Run the accepted build with `build/super_tennis`. The tested private desktop
remains available through `build/test-aot-bulk.command` for evidence comparison.
The accepted gameplay capture exited normally without capture loss. It did
not record controller input.

The tracked policy preserves all 166 screened bus-cost keys. There are 119
emitted bodies under this selection, including the previously accepted 019B33;
47 keys stay interpreted. The cfg also retains named boundaries for 68
unvalidated callees and explicit exclusions for seven wider replay failures.
These boundaries affect tail transfers and caller exit proofs. Do not remove
them merely because their own bodies are interpreted. No new exit-width
contracts were added.

## Promotion verification

Fresh normal regeneration reproduces all seven candidate C files byte for
byte and the complete validated manifest. The rebuilt desktop has identical
code and data to the accepted executable. Its raw hash differs only within
the Mach-O UUID and corresponding first-page signature hash: 47 bytes differ
within those 48 metadata bytes. Every code-page hash and both signatures
verify. Existing replay and gameplay results therefore apply to this build.
The seven setup checks and publication boundary check passed after repinning.
The Python v2 and shared C suites were reused from the unchanged tested source.
Validation ran on macOS Apple Silicon; Windows was not run for this batch.

`captures/aot-bulk-promotion-current.txt` identifies the private regeneration,
build, setup and binary comparison records. Contributor builds need only the
tracked cfg, generation helper, pinned dependency and their own verified ROM.

## Efficient screening procedure

1. Preserve accepted source, options, generated output and executable hashes.
   Generate the full candidate set once with the accepted timing selections
   plus `SNESRECOMP_EMIT_AOT_DENY_GATE=1`.
2. Use `SNESRECOMP_LLE_INTERP_TARGET_FILE` to disable candidate addresses between
   runs. This existing runtime option guards generated body entry, including
   direct calls, as well as interpreter-to-AOT dispatch. It avoids rebuilding
   for each subset. The switch acts on PC, not exact M/X: group variants at
   the same address, and refuse a set that would also disable an accepted
   variant at that address.
3. Prove that denying all additions reproduces the accepted control. Extra
   profile roots can change existing generated callers. A failing control
   invalidates the screening setup; it is not evidence against a new routine.
4. Compare deterministic frame records and stop each failure at its first
   mismatch. Reuse runtime objects and verified reference recordings. Keep
   independent screening processes bounded so diagnostics do not overload the
   host. Do performance measurements serially afterwards.
5. Count actual generated entries by exact live M/X. A passing group with no
   executed additions is unexercised, not validated. The first ordinary-AOT
   screen found 223 passing addresses, but none ran in the primary replay.
6. Test a separate corrected-bus-cost policy when the first differences justify
   it. Preserve accepted timing choices and leave global bus timing disabled.
   Entry counters narrow failed groups to new bodies reached before a failure.
   Quarantine these groups initially, then recover passing subsets. Membership
   of a failed group alone does not establish that every member is faulty.
7. Combine recovered groups and test the combination. In this pass, 552
   addresses passed in separate groups but their combination failed at frame
   1586. Incremental merging isolated one interaction exclusion. The merged
   primary result exercised 165 new variants. Wider replays excluded seven
   more; the surviving guarded selection exercised 158 additions.
8. Generate a clean candidate from explicit cfg facts and checked timing
   options. Preserve named boundaries for interpreted tail-call targets.
   Excluding unvalidated callees can retract callers' proven exit widths;
   those callers must remain interpreted unless independently resolved.
   The guarded screening result is therefore not the final AOT count.
9. Validate and benchmark the actual clean generated candidate. Perform wider
   replay checks at this combined milestone, then hand over one desktop build
   for gameplay. Promote tracked cfg only through the existing evidence and
   reproducibility gate.

Do not infer instruction-timing requirements from an opcode allowlist alone.
The final batch includes operations and control flow outside the current
instruction-timing leaf subset, using the existing block bus-cost model.
Conversely, the four high-frequency candidates 0088C1, 019D3B, 01A14A and
02858C each have a separate reproduced failure under the tested bus-cost
policy. They remain a shared timing/control-flow investigation. First frame
and field differences are evidence; opcode-based categories are static
classification, not established causes.

## Build issue found

Large profile output activates bank sharding. A timing header emitted before
a later function could remain in an earlier shard, causing compilation to
fail. Published snesrecomp patch 0021 copies that existing header into each
shard's preamble. It changes no runtime implementation or opcode timing.
The final clean candidate compiles without the initial private forced-include
workaround. The Python v2 suite (392 tests), shared C suite and seven setup
checks passed once at this integration milestone.

## Private evidence and continuation

`captures/aot-bulk-current.txt` identifies the immutable raw runs and derived
reports. This directory contains:

- `generation.json`, `sets.json`, and `structural-comparison.json` for inputs.
- `st-bulk-*.py` for the local build, screening, recovery, merging, wider
  validation and benchmark automation, with the private instrumented source.
- `screen-progress.json` for the ordinary screen.
- `bus/guided-result.json`, `bus/recovery-result.json`, `bus/merge-result.json`
  and `bus/wide-clean-result.json` for successive screening results.
- `handoff/config/`, `handoff/generated/`, `handoff/generation.json` and
  `handoff/validation.json` for the actual clean candidate.
- `handoff/benchmark/results.json`, `handoff/desktop.json` and `gameplay/`
  for performance, executable identity and maintainer sessions.

Earlier failed proposals remain in separate directories. Do not replace their
logs or describe their checks as passing. Preserve the full input/profile and
exact selection when resuming an experiment. No new gameplay capture is needed
just to repeat this screen; additional inputs are useful for modes not covered
by the five saved recordings.
