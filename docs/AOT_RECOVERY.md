# AOT recovery: 13 September 2026

The accepted normal build has 329 AOT bodies, an increase of 20 over the
309-body checkpoint. The maintainer explicitly waived the manual gameplay
check and requested promotion on 13 September 2026. Tracked cfg and the normal
generation policy now reproduce this selection without a private profile.

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

## Remaining work

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
