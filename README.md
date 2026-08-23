[English](README.md) | [Português (Brasil)](README.pt-BR.md)

# NES Survivor

NES Survivor is a fixed-arena, survivor-like action game targeting original NES
constraints. The project uses C for high-level systems and focused 6502 Assembly
for hardware startup and bounded low-level work.

## Current status

The ROM now boots through a centered `PRESENTED BY` credit and a title screen
before entering the fixed black arena. The credit lasts 150 NTSC frames (2.5
seconds) or can be skipped with a new START press. `PRESS START` on the title
screen alternates every 30 frames, and a separate new START press initializes
the run.

In gameplay, Soldier is centered on screen. The player moves Soldier in all eight D-pad
directions, remembers the last horizontal facing direction, and uses Soldier's
generated one-frame idle and two-frame movement animation. Animation durations,
signed metasprite offsets, tile indexes and OAM attributes come from the
consolidated png2chr-studio data.

Soldier automatically swings a sword in front of the current facing direction
once every 60 frames. The generated sword is active for 12 frames and removes
Bats that overlap its hitbox. Bats first appear from an arena edge after two
seconds, then every two seconds, and pursue the player at an average 0.375 pixel
per axis per frame. Every defeated Bat leaves an 8x8 XP gem at its position.
Touching the player removes the gem; XP totals and progression are not yet
implemented. Up to eight gems are visible, with excess drops condensed into
the nearest active gem.

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
make test-performance
make clean
```

The ROM is generated at `build/nes-survivor.nes`; the linker map and labels are
generated beside it. `make test` executes the C logic tests through cc65's
`sim65` and validates the built iNES cartridge with Python.
`make test-runtime` is optional and first validates the initial screens for 175
frames, then runs the gameplay ROM for 450 frames with Mesen 2's headless Lua
test runner. `make test-performance` runs a 1,750-frame stress test,
fills all 12 Bat slots, observes a gem drop and fails if the gameplay loop
misses an NMI.

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
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 performance
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 clean
```

The first 4 KiB of `assets/game.chr` supplies sprite patterns. `src/chr.s`
builds the second pattern table from the small ASCII font used by the initial
screens.

## Controls

Controller 1 is sampled every frame. The D-pad moves one pixel per axis per game
frame, including diagonals. Left and Right update horizontal facing; Up and Down
alone preserve it. Releasing the D-pad returns to idle while preserving facing.
START skips the credit and starts a run from the title screen, but only on a
newly pressed edge; holding it cannot cross both screens. A, B and Select have
no action yet. The sword attacks automatically and requires no button.

## Hardware target and limitations

- NROM-256 / Mapper 0, 32 KiB PRG-ROM and 8 KiB CHR-ROM
- horizontal nametable mirroring
- NTSC timing assumption (60 frames per second)
- one fixed screen with scrolling held at zero
- no audio and no PAL/Dendy timing adaptation yet
- one player character, one automatic sword and one enemy type; no player
  damage, XP gain, wave transitions, HUD or progression yet
- diagonals intentionally use the full one-pixel speed on both axes
- player and active sword consume 9 OAM slots; 12 Bats plus eight gems raise
  the worst case to 41/64, and overlapping objects may flicker due to the
  scanline sprite limit

Architecture and frame details are in [docs/architecture.md](docs/architecture.md).
Measured memory usage is in [docs/memory-map.md](docs/memory-map.md).
