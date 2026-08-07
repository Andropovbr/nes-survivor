#!/usr/bin/env python3
"""Dependency-free structural validation for the milestone-one cartridge."""

from __future__ import annotations

import re
import sys
from pathlib import Path


EXPECTED_SIZE = 16 + (2 * 16 * 1024) + (1 * 8 * 1024)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def label_address(labels: str, name: str) -> int:
    match = re.search(rf"^al\s+([0-9A-Fa-f]{{6}})\s+\.?{re.escape(name)}$", labels, re.MULTILINE)
    require(match is not None, f"missing linker label: {name}")
    return int(match.group(1), 16)


def main() -> int:
    if len(sys.argv) != 4:
        print("usage: validate_rom.py ROM MAP LABELS", file=sys.stderr)
        return 2

    rom_path, map_path, labels_path = map(Path, sys.argv[1:])
    rom = rom_path.read_bytes()
    map_text = map_path.read_text(encoding="utf-8")
    labels = labels_path.read_text(encoding="utf-8")

    require(len(rom) == EXPECTED_SIZE, f"ROM size is {len(rom)}, expected {EXPECTED_SIZE}")
    require(rom[:4] == b"NES\x1a", "invalid iNES signature")
    require(rom[4] == 2, "expected two 16 KiB PRG banks")
    require(rom[5] == 1, "expected one 8 KiB CHR bank")
    require((rom[6] & 0xF1) == 0, "expected mapper 0, horizontal mirroring, no trainer")
    require((rom[7] & 0xF0) == 0, "expected mapper 0 upper nibble")
    chr_asset = Path(__file__).resolve().parent.parent / "assets" / "game.chr"
    require(chr_asset.stat().st_size == 8192, "assets/game.chr is not 8 KiB")
    require(rom[-8192:] == chr_asset.read_bytes(), "ROM CHR does not match assets/game.chr")

    vectors = rom[16 + 0x7FFA : 16 + 0x8000]
    require(len(vectors) == 6, "interrupt vector table is incomplete")
    for offset, vector_name in ((0, "NMI"), (2, "RESET"), (4, "IRQ")):
        address = vectors[offset] | (vectors[offset + 1] << 8)
        require(0x8000 <= address <= 0xFFFF, f"{vector_name} vector is outside PRG-ROM")

    require(label_address(labels, "_oam_shadow") == 0x0200, "OAM shadow is not at $0200")
    require(label_address(labels, "_nes_frame_counter") < 0x0100, "frame counter is not in zero page")
    require("OAM" in map_text and "ZEROPAGE" in map_text, "map omits required RAM segments")

    print("ROM validation passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
