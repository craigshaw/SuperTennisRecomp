# Focused AOT experiments

Start with `AOT_CURRENT.json` and the latest section of `AOT_RECOVERY.md`.
Read older evidence only for the selected mechanism or a matching failure.
The saved classification and accepted controls remain valid until their
inputs change. Do not repeat the bulk screen to select another target.

## Decision and test gates

1. State one hypothesis, the exact `(pc24,M,X)` targets, the expected benefit
   and a stop condition. Rank by measured work and likely implementation cost.
   Interpreted instruction counts do not measure host CPU time.
2. Check the saved rejection notes. Use deja-vu only when those notes leave a
   specific historical question. Inspect the existing compiler rule before
   writing a new experiment. Check obsolete rejection tests when extending it.
3. Run focused tests for changed shared code, then a short replay that reaches
   the target and its caller continuation. Stop on the first difference, missing
   target execution or unsupported compiler boundary.
4. Run one representative full replay after the short probe passes. Reuse
   unchanged controls. Expand coverage only for a concrete gap or failure.
5. Run the shared Python v2 and C suites once for the final shared source change.
   A title tooling or cfg-only change does not require those suites. Record the
   exact checks reused, with source identity and evidence paths. The runner
   does not claim that unrun checks passed.
6. Promote through normal generation, compare fresh output, and use normal
   binaries for performance measurements. An instrumented experiment binary
   is evidence for behavior and execution coverage, not a performance result.

Stop a small candidate when it requires a separate scheduler mechanism unless
that mechanism has a clear larger use. Do not broaden a failing experiment
into an unbounded timing investigation. Save the first difference and reassess
its expected benefit. A failed or capped observation is useful evidence, not a
promotion gate.

## Runner

`tools/aot-experiment/run.py` replaces copied private build and trace scripts.
It requires Python 3.10+, CMake, Ninja and the normal C toolchain. This initial
version is validated on macOS. It creates only private files below `captures/`.
It does not change tracked cfg, normal generated output or the shared source.

Create one private config. The shell variables below are paths supplied by the
contributor. The frame reference and profile must come from accepted evidence
for the same replay. The example keys are the completed upload validation,
not a request to reopen that optimization.

```sh
python3 tools/aot-experiment/run.py init \
  --rom "$SUPER_TENNIS_ROM" --replay "$SUPER_TENNIS_REPLAY" \
  --frames "$ACCEPTED_FRAME_CSV" --profile "$ACCEPTED_WORK_CSV" \
  --cfg config --root 07D8A5:1:0 \
  --require-executed 07D8A5:1:0 --require-executed 07D8CA:1:0 \
  --short-frames 90 --full-frames 4816 --out captures/experiment/config.json
python3 tools/aot-experiment/run.py preflight captures/experiment/config.json
python3 tools/aot-experiment/run.py run captures/experiment/config.json \
  --out captures/experiment/attempt-01
```

`init` locks the ROM, replay, cfg, frame control and optional work profile with
SHA-256. Change a lock deliberately when its input changes. Every attempt gets
a new directory. Existing evidence is never overwritten. `--short-only` runs
just the probe. Both probe and full replay require all `require_executed` keys;
choose a probe long enough to cover them. Failures retain their logs and report.

Normal selection is imported from `tools/generate-normal.py`. For a private
candidate, edit the config's `selection` with `instruction_timing` and/or
`continuations`, each containing `add` and/or `remove` arrays. Keys use
`07D8A5:1:0` and `07D8A5:1:0>07D8CA:1:0`, respectively. Use a private cfg copy
when analysis inputs must change. This does not bypass the lab's evidence and
proposal requirements for cfg facts, including cross-variant exit contracts.

To reuse saved generated output, pass `--generated PATH` to `init`. This locks
and copies the entire directory; selection overrides are then rejected. Both
saved control and candidate use the current shared runtime when rebuilt.
This does not recreate a historical runtime. The source snapshots record that
limit. Frozen generation avoids repeating the analyzer during diagnosis.

The runner records source hashes and Git status before and after the run.
Changed inputs invalidate acceptance. It saves generation options, command
arguments, timings, raw logs, generated files, private instrumented copies,
frame comparisons, exact interpreter work and execution counts. It checks
bailouts, tuple overflow, journal failures and changed opcodes. The compact
`result.json` is the first file to read. Inspect a raw log only for a reported
failure. No test or runtime settings are inherited from ambient `ST_*` or
`SNESRECOMP_*` variables.

CMake/Ninja reuse a private build cache across attempts. Unchanged overlay
files keep their timestamps; shared source dependencies remain tracked by the
build system. Measurement and trace builds have separate caches. A cache lock
rejects concurrent use. Set a different `cache` name in the config only for
intentional concurrent work or a different toolchain. After an interrupted
process, remove a stale `busy` file only after verifying no process uses it.
The cache is disposable; the completed attempt and its evidence are separate.

Root counts observe the first timed instruction after its deadline guard.
They prove native execution of that exact key, not merely dispatch into a
wrapper. A root that lacks instruction timing is rejected by this harness.
Continuation counts use the existing criterion of guest cycle progress.
The runner records all continuations and checks the requested exact keys.
It does not discover or promote aggregate-timed roots automatically.

## Bounded first-difference trace

Add this object to a private config, then run `trace` instead of `run`:

```json
{
  "trace": {
    "from": 10,
    "to": 10,
    "limit": 100000,
    "ranges": ["07D8A5-07D91C"]
  }
}
```

Trace frame numbers are zero-based; frame digest rows are one-based. `trace`
runs the short replay only. Produce a control and a candidate in separate
attempt directories, then compare their `short/trace.csv` files:

```sh
python3 tools/aot-experiment/run.py compare \
  captures/experiment/control/short/trace.csv \
  captures/experiment/candidate/short/trace.csv
```

The comparison returns the first different row and fields with up to three
preceding rows. Unequal lengths and empty files fail. Completion metadata
reports whether either trace hit its cap. A capped match establishes only
prefix agreement. Keep `result.json` beside its trace CSV when comparing.

The tracer copies live interpreter registers before packing P. It does not
change live state or read MMIO. Native samples occur after the instruction
boundary guard and before instruction effects. Records include CPU registers,
widths, CPU/master clocks, APU clocks, pending time and direct port state.
Coverage is limited to executed interpreter opcodes and instruction-timed AOT
instructions in the selected frame/address window. Aggregate AOT instructions,
interrupt entry microsteps and scheduler-only waits are not trace records.
Matching traces therefore do not establish whole-program equivalence or new
hardware facts. Use frame controls as well. Mesen, snesref or cosim remain
available when a specific question needs independent evidence.

## Checkpoint maintenance

Keep `AOT_CURRENT.json` small: baseline identity, private evidence pointers,
work totals, ranked next questions and explicit exclusions. Record detailed
results and remaining limits in `AOT_RECOVERY.md`. Record build/replay time and
reused checks rather than estimating token savings. Do not build a new harness
for each address or copy the historical report into each handoff.
