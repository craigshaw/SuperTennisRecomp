# Architecture

This document describes the stable boundary between the Super Tennis host,
the shared recompilation framework, and private validation data.

## Runtime flow

```text
user-owned ROM
    |
    +--> snesrecomp analysis
    |        |
    |        +--> ignored generated C and analysis manifest
    |
    +--> Super Tennis host
             |
             +--> generated exact (pc24, M, X) variants
             +--> 65816 interpreter fallback
             +--> shared PPU, APU, DMA, input, and cartridge devices
             |
             +--> SDL desktop output or deterministic headless frames
```

The project does not contain a hand-written game decompilation.
`src/super_tennis_rtl.c` connects generated and interpreted execution to the
title frame scheduler. `src/main.c` provides the SDL desktop host, while
`src/headless_main.c` provides bounded deterministic execution for tests.

## Ownership

| Concern | Owner |
| --- | --- |
| Super Tennis scheduling, host integration, and replay commands | this repository |
| 65816 analysis, generation, interpreter, and SNES devices | `snesrecomp` |
| Pre-launch and in-game user interfaces | `recomp-ui` |
| Shared binary replay reader | `snesrecomp` |
| Mesen capture, evidence, and replay recording | `snesrecomp-lab` |
| Super Tennis replay inputs and assertions | this repository |
| ROMs, generated C, captures, and comparison output | private local workspace |

Game-neutral changes belong in `snesrecomp`. Until such a change is available
from the pinned upstream dependency, this repository carries it as an ordered
patch under `patches/snesrecomp/`. The integration process is documented in
[SNESRECOMP_PATCHES.md](SNESRECOMP_PATCHES.md).

## Desktop host

The desktop target links the shared `recomp-ui` launcher. The launcher verifies
the ROM, collects display, audio, and input settings, and stores them in
`config.ini` and `keybinds.ini`.

The in-game settings menu uses the same settings model. Opening the menu pauses
the simulation, applies supported changes immediately, and persists them
through the same configuration path. The headless target does not include the
launcher, in-game menu, or their assets.

The pinned UI dependency does not provide the SDL3 renderer backend required by
the game window. `src/imgui_impl_sdlrenderer3.cpp` is a local adaptation of the
Dear ImGui renderer backend. It should be removed when the pinned dependency
provides a compatible implementation.

## Frame and display model

The title host advances CPU execution towards the next frame event and delivers
enabled raster IRQ and NMI requests at scheduler boundaries. Interrupt handlers
remain schedulable continuations, allowing a long handler to span a host frame
or be interrupted by a later event.

At the start of each field, the host snapshots display state and journals
relevant PPU writes with their scanline. The renderer restores the line-zero
state and applies the journal before rendering each visible line. This preserves
mid-frame changes such as the Mode 7 court and Mode 1 menu split. VRAM, CGRAM,
and OAM data ports are excluded from the journal because those writes have
already updated their target memories.

Both hosts can read the shared `.sri` replay format. One sample is applied
before each `RtlRunFrame` call and remains stable for all automatic and manual
controller reads in that frame.

## Generation and evidence boundary

The tracked files under `config/` contain evidence-backed analysis directives.
Whole-program analysis starts from reset and interrupt roots, emits exact
variants where state is proven, and retains interpreter fallback for unresolved
execution.

The [AOT coverage workflow](AOT_COVERAGE.md) describes how private tier-2
observations become validated, tracked analysis inputs. The acceptance gate
includes fresh generation without a private profile, output comparison, and
regression checks on that regenerated build.

Generated C, manifests, ROMs, replays, diagnostic screenshots, traces,
savestates, and raw comparison data are private artifacts. They are
reproducible inputs or evidence, not source material for publication. The
three maintainer-approved README images under `assets/screenshots/` are the
only published screenshot exceptions.

Optional event-crossing diagnostics can identify generated work that spans a
scheduler deadline. `tools/run-event-crossing-audit.sh` produces a private
report, and `tools/build-event-precision.sh` can use it to route only affected
variants through instruction-timed execution. Normal regeneration does not
enable this instrumentation.
