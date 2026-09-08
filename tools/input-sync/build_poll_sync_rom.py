#!/usr/bin/env python3
"""Build the synthetic LoROM used by the Mesen input-poll proof."""

from __future__ import annotations

from pathlib import Path
import sys


ROM_SIZE = 0x8000
RESET_PC = 0x8000
NMI_PC = 0x8100


RESET_PROGRAM = bytes(
    [
        0x78,  # SEI
        0xD8,  # CLD
        0x18,  # CLC
        0xFB,  # XCE: native mode
        0xC2,
        0x30,  # REP #$30: 16-bit A and index registers
        0xA9,
        0x00,
        0x00,  # LDA #$0000
        0x5B,  # TCD
        0xA2,
        0xFF,
        0x1F,  # LDX #$1FFF
        0x9A,  # TXS
        0xE2,
        0x20,  # SEP #$20: 8-bit A, 16-bit index registers
        0x64,
        0x00,  # STZ $00: sample byte count, low
        0x64,
        0x01,  # STZ $01: sample byte count, high
        0xA9,
        0x81,  # NMI and automatic joypad polling
        0x8D,
        0x00,
        0x42,  # STA $4200
        0x58,  # CLI
        0xCB,  # WAI
        0x80,
        0xFD,  # BRA back to WAI
    ]
)


NMI_PROGRAM = bytes(
    [
        0x48,  # PHA, 8-bit
        0xDA,  # PHX, 16-bit
        0xE2,
        0x20,  # SEP #$20
        0xAD,
        0x12,
        0x42,  # wait_started: LDA $4212
        0x29,
        0x01,  # AND #$01
        0xF0,
        0xF9,  # BEQ wait_started
        0xAD,
        0x12,
        0x42,  # wait_finished: LDA $4212
        0x29,
        0x01,  # AND #$01
        0xD0,
        0xF9,  # BNE wait_finished
        0xA6,
        0x00,  # LDX $00
        0xC2,
        0x20,  # REP #$20
        0xAD,
        0x18,
        0x42,  # LDA $4218
        0x9D,
        0x00,
        0x01,  # STA $0100,X
        0xE2,
        0x20,  # SEP #$20
        0xE8,  # INX
        0xE8,  # INX
        0x86,
        0x00,  # STX $00
        0xFA,  # PLX
        0x68,  # PLA
        0x40,  # RTI
    ]
)


def _put_word(rom: bytearray, offset: int, value: int) -> None:
    rom[offset] = value & 0xFF
    rom[offset + 1] = (value >> 8) & 0xFF


def build_rom() -> bytes:
    rom = bytearray(ROM_SIZE)
    rom[RESET_PC - 0x8000 : RESET_PC - 0x8000 + len(RESET_PROGRAM)] = RESET_PROGRAM
    rom[NMI_PC - 0x8000 : NMI_PC - 0x8000 + len(NMI_PROGRAM)] = NMI_PROGRAM

    header = 0x7FC0
    rom[header : header + 21] = b"INPUT POLL SYNC TEST".ljust(21, b" ")
    rom[header + 0x15] = 0x20  # LoROM, slow
    rom[header + 0x16] = 0x00  # ROM only
    rom[header + 0x17] = 0x05  # 32 KiB
    rom[header + 0x18] = 0x00  # no cartridge RAM
    rom[header + 0x19] = 0x01  # NTSC
    rom[header + 0x1A] = 0x33
    rom[header + 0x1B] = 0x00

    for offset in range(0x7FE0, 0x8000, 2):
        _put_word(rom, offset, RESET_PC)
    _put_word(rom, 0x7FEA, NMI_PC)  # native NMI
    _put_word(rom, 0x7FFA, NMI_PC)  # emulation NMI
    _put_word(rom, 0x7FFC, RESET_PC)

    _put_word(rom, 0x7FDC, 0)
    _put_word(rom, 0x7FDE, 0)
    checksum = sum(rom) & 0xFFFF
    _put_word(rom, 0x7FDE, checksum)
    _put_word(rom, 0x7FDC, checksum ^ 0xFFFF)
    return bytes(rom)


def main(argv: list[str]) -> int:
    output = Path(argv[0]) if argv else Path("poll-sync-test.sfc")
    output.parent.mkdir(parents=True, exist_ok=True)
    data = build_rom()
    output.write_bytes(data)
    print(f"wrote {output} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
