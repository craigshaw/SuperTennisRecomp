# Mesen input-poll synchronization proof

This tool proves the narrow input contract needed before recording a Super
Tennis replay. It does not use the commercial ROM.

The generated synthetic LoROM enables NMI and automatic SNES joypad polling.
Its NMI handler waits for `$4212.0` to assert and clear, reads `$4218/$4219`,
and stores each 16-bit joypad word in WRAM.

The proof runs Mesen twice:

1. Inject a fixed eight-poll controller sequence during `inputPolled` and
   record the state returned by `getInput`.
2. Replay the recorded states from a clean power-on run.

Both runs must have identical poll indexes, completed-frame anchors, master
clocks, and guest-visible joypad words. Generated ROMs, launchers, and logs are
written below the ignored `build/input-sync/` directory.

Observed fact: the real MesenCE 2.2.1 proof saw refresh 0 at completed-frame
index 1. An input supplied during that callback was returned immediately by
`getInput` and was the value captured by the ROM's automatic joypad read in the
same field. Eight input refreshes and eight guest samples matched in both runs.

Static source fact: Mesen calls `inputPolled` once when it refreshes frontend
input at the end of each field. It is not a callback for each guest `$4016`
latch or serial read. Mesen's movie poll index is therefore a frontend input
refresh index. For the current Super Tennis host, the corresponding input
boundary is one state supplied to each `RtlRunFrame` call.

Run it with MesenCE 2.2 or newer:

```sh
python3 tools/input-sync/run_mesen_poll_sync.py --mesen /absolute/path/to/Mesen
```

This checkpoint first proved that one Mesen frontend input refresh supplies the
automatic joypad value captured in that field. The shared binary replay work
then extended the proof: MesenCE wrote an eight-sample `.sri`, the bounded Mesen
adapter replayed those exact bytes, and the snesrecomp C reader consumed the
same file. Manual guest reads consume the state held for that field and do not
advance the replay index.

## MesenCE 2.2.1 setInput compatibility

The original proof intentionally controlled only controller 1. The MesenCE 2.2.1
implementation of the documented three-argument `setInput(input, port,
subPort)` call reads the effective port and subport from arguments 3 and 4.

The snesrecomp-lab replay adapter supplies a compatibility placeholder as
argument 2 and reads every controlled button back before advancing its poll
index. A real synthetic replay with non-neutral states on both controllers
passed after that correction. This workaround must be retested if Mesen's Lua
API implementation changes.
