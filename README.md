[English](README.md) | [Português (Brasil)](README.pt-BR.md)

# NES Survivor

NES Survivor is a fixed-arena, survivor-like action game targeting original NES
constraints. The project uses C for high-level systems and focused 6502 Assembly
for hardware startup and bounded low-level work.

## Current status

The ROM now boots into a fixed black arena with Soldier, the first playable
character, centered on screen. The player moves Soldier in all eight D-pad
directions, remembers the last horizontal facing direction, and uses Soldier's
generated 6-frame idle and two 8-frame movement sequences. Animation durations,
signed metasprite offsets, tile indexes and OAM attributes come from the
consolidated png2chr-studio data.

The NROM foundation still performs bounded OAM DMA in NMI and runs controller,
player, animation and OAM construction logic in the synchronized C main loop.
The `player` module represents whichever character controller 1 owns; concrete
graphics and animation symbols are prefixed `soldier`.

## Requirements

- cc65 toolchain 2.19 or compatible (`cc65`, `ca65`, `ld65`, `cl65`, `sim65`)
- GNU Make for the primary commands
- Python 3 for cartridge validation tests
- Mesen 2 is recommended for runtime inspection

No tool path is hard-coded. The build uses the executables available on `PATH`.

## Build and test

```sh
make
make test
make test-runtime
make clean
```

The ROM is generated at `build/nes-survivor.nes`; the linker map and labels are
generated beside it. `make test` executes the C logic tests through cc65's
`sim65` and validates the built iNES cartridge with Python.
`make test-runtime` is optional and runs the ROM for 130 frames with Mesen 2's
headless Lua test runner.

On Windows, `make` uses `python` for portable build-directory creation and
cleanup. On Unix-like hosts it uses `python3`. Override `PYTHON` or `MESEN` only
when those executables are not on `PATH`, for example:

```powershell
make test-runtime MESEN="F:/Emulators/Mesen.exe"
```

On Windows systems without GNU Make, the equivalent checked workflow is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 build
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 test
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 runtime
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 clean
```

`assets/game.chr` is the source-controlled 8 KiB CHR-ROM bank used by the build.

## Controls

Controller 1 is sampled every frame. The D-pad moves one pixel per axis per game
frame, including diagonals. Left and Right update horizontal facing; Up and Down
alone preserve it. Releasing the D-pad returns to idle while preserving facing.
A, B, Select and Start are sampled but have no gameplay action yet.

## Hardware target and limitations

- NROM-256 / Mapper 0, 32 KiB PRG-ROM and 8 KiB CHR-ROM
- horizontal nametable mirroring
- NTSC timing assumption (60 frames per second)
- one fixed screen with scrolling held at zero
- no audio and no PAL/Dendy timing adaptation yet
- one player character only; no combat, enemies, waves, HUD or collision yet
- diagonals intentionally use the full one-pixel speed on both axes
- no sprite-flicker rotation is needed yet; the player consumes 7 of 64 OAM slots

Architecture and frame details are in [docs/architecture.md](docs/architecture.md).
Measured memory usage is in [docs/memory-map.md](docs/memory-map.md).
