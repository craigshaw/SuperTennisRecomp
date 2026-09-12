# Super Tennis recompilation agent guide

Read `README.md`, `docs/ARCHITECTURE.md`, and `docs/SNESRECOMP_PATCHES.md`
before changing the project.

For tier-2 capture, AOT coverage experiments, or promotion into tracked cfg,
also read [docs/AOT_COVERAGE.md](docs/AOT_COVERAGE.md). A private profile is a
discovery input. Accepted coverage must be reproducible through the normal
generation commands using tracked inputs and the contributor's verified ROM.

## Repository boundaries

- This repository owns the Super Tennis host, frame scheduler, input harness,
  build glue, the evidence-backed per-bank cfg, and reproducible smoke tools.
- `snesrecomp/` is a pinned submodule. Game-neutral decoder, generator,
  interpreter, PPU, APU, or runner fixes belong there first. Until they are
  upstream, export them as ordered patches under `patches/snesrecomp/` and
  update `docs/SNESRECOMP_PATCHES.md`.
- The sibling `../snesrecomp-lab` repository is a focused external-evidence
  companion. It owns bounded Mesen execute-point capture, immutable evidence,
  the existing exit-M/X proposal rule, manifest comparison, and the generic
  shared binary input recorder and replay protocol.
- This repository owns deterministic Super Tennis regression inputs and
  playback. Use snesrecomp tier-2 manifests, tripwires, and reference-debugging
  paths for normal coverage and runtime diagnosis.

## Publication boundary

Never commit ROMs, generated C, SRAM, savestates, input recordings, traces,
memory dumps, screenshots, audio captures, or private comparison reports.
These belong in ignored local directories or the ignored
`../snesrecomp-lab/workspaces/super-tennis/` workspace.

The maintainer-approved README images at
`assets/screenshots/title-screen.png`, `assets/screenshots/match-start.png`,
and `assets/screenshots/rally.png` are the only screenshot exceptions. Keep
other captures private unless the maintainer explicitly approves them.

The tracked `config/bankNN.cfg` files, currently `config/bank00.cfg`, may contain
evidence-backed title facts with their rationale. The `generated/` tree remains
a reproducible private product; do not weaken `.gitignore` to publish it.

## Evidence and correctness

- Treat `(pc24, M, X)` as the minimum exact CPU execution key.
- Prefer runtime observation over inferring width state from an address.
- Distinguish an observed fact, a static deduction, and a hypothesis in notes.
- Do not change analysis cfg merely to silence analysis. Evidence-backed cfg
  changes should be proposed and validated through snesrecomp-lab.
- Keep raw evidence immutable. Corrections belong in derived reports.

## Working loop

1. Initialize and patch the submodule with
   `sh tools/apply-snesrecomp-patches.sh`. The current public pin already
   includes all required patches, so normal setup only verifies its files.
   Older supported revisions can use the exported patches for recovery.
2. Regenerate from the user's ROM with
   `sh tools/regenerate.sh /path/to/rom.sfc`.
3. Build with `sh build.sh`.
4. Run the neutral headless check:
   `build/super_tennis_headless /path/to/rom.sfc --frames 180`.
5. If a private title replay is available, run it with
   `SUPER_TENNIS_ROM=/path/to/rom.sfc`,
   `SUPER_TENNIS_REPLAY=/path/to/replay.sri`, and
   `sh tools/run-input-replay.sh`.
6. For a behavioral mismatch, extend a title-owned deterministic replay and
   record the earliest failing frame/PC/M/X before altering analysis or
   scheduling. Use the lab only when the resulting question needs an
   independent bounded Mesen observation.

Before handing work on, run `sh tools/check-publication-boundary.sh`, the
snesrecomp Python v2 suite, the shared C suite, and the relevant title replay.
After changing patch application or generation glue, also run
`python3 tools/test-build-workflow.py` and verify fresh normal generation.
