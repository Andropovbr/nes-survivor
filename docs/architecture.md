# Architecture

## Milestone 1 modules

- `src/crt0.s` owns the iNES header, reset path, RAM and PPU initialization,
  cc65 runtime startup, rendering enable and interrupt vectors.
- `src/nmi.s` is the bounded NMI handler. It uploads the OAM shadow page, restores
  zero scroll and advances the frame counter.
- `src/nes.s` owns the page-aligned OAM allocation, frame wait primitive and
  controller-port read routine.
- `src/main.c` orchestrates initialization and the synchronized main loop.
- `src/game.c` contains only the explicit `BOOT` to `RUNNING` state transition.
- `src/input.c` derives current, pressed and released button masks from the raw
  hardware sample.
- `src/rng.c` implements deterministic `xorshift16` state and output functions.
- `include/tuning.h` holds only initial capacities with architectural value.

The blank CHR-ROM is a real 8 KiB source asset linked through `src/chr.s`. The
small files under `chr/` are deliberately not consumed in this milestone.

## C and Assembly boundary

C owns policy, state transitions and pure logic. Assembly is limited to the NES
reset sequence, interrupt work, memory-mapped I/O, OAM DMA, frame waiting and the
serial controller read. Every C-callable Assembly entry point documents its ABI
in `include/nes.h` and at its implementation.

Gameplay must remain in C unless generated code inspection or emulator
measurement identifies a concrete bottleneck. A routine running every frame is
not by itself a reason to move it into Assembly.

## Frame lifecycle

1. Reset disables rendering and interrupt sources, waits for PPU stabilization,
   clears all 2 KiB of internal RAM, and initializes the cc65 software stack.
2. With rendering disabled, startup clears `$2000-$2FFF`, fills all palette
   entries with NES black (`$0F`), fills OAM shadow with `$FF`, initializes the C
   runtime and enables NMI plus background/sprite rendering.
3. NMI preserves A/X/Y, performs one 256-byte OAM DMA from `$0200`, resets scroll
   to zero, increments an 8-bit zero-page frame counter, restores registers and
   returns. Worst-case work is approximately 583 CPU cycles including interrupt
   entry, comfortably inside the roughly 2,273-cycle NTSC VBlank.
4. `nes_wait_frame` snapshots the counter and waits until NMI changes it. An
   8-bit comparison is atomic on 6502; wraparound is safe because 256 NMIs cannot
   occur between the snapshot and comparison.
5. The main loop reads controller 1 and updates the minimal game state outside
   NMI. OAM construction will also remain outside NMI when rendering is added.

The controller bit layout is A, B, Select, Start, Up, Down, Left and Right in
bits 7 through 0. DMC is disabled, so DMA cannot corrupt the serial controller
read. OAM DMA behavior is deterministic: all 64 sprite entries upload every NMI,
and all are currently hidden.

## Deterministic RNG

`xorshift16` uses two bytes of state and shifts `(7, 9, 8)`. A zero seed is
normalized to one because zero is the algorithm's absorbing state. The generator
is fast and reproducible but is not cryptographic. Gameplay and cosmetic streams
should be separated later if shared consumption would prevent reproducible tests.

## Incremental boundaries for future milestones

Future systems should be added only when their milestone requires them:

- player input, movement and rendering;
- immutable character definitions separated from per-run character state;
- immutable weapon definitions and compact runtime slots for automatic weapons;
- fixed-size enemy, projectile and XP pools with documented saturation behavior;
- table-driven arena and wave definitions;
- eligibility, rarity and application layers for upgrades;
- unlock objectives and versioned password persistence.

Content should use compact IDs and array indexes, not ownership pointers or heap
allocation. Definition tables remain immutable; mutable run state remains in
fixed-size pools. Adding a character, weapon, enemy or stage should add one table
entry and only introduce specialized code for genuinely distinct behavior.

No empty future modules or speculative runtime structures exist yet. This keeps
the linker map honest and each future change reviewable.
