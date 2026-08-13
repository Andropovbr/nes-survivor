# Architecture

## Current modules

- `src/crt0.s` owns the iNES header, reset path, RAM and PPU initialization,
  cc65 runtime startup, rendering enable and interrupt vectors.
- `src/nmi.s` is the bounded NMI handler. It uploads the OAM shadow page, restores
  zero scroll and advances the frame counter.
- `src/nes.s` owns the page-aligned OAM allocation, frame wait primitive and
  controller-port read routine.
- `src/main.c` orchestrates initialization and the synchronized main loop.
- `src/game.c` owns the explicit `BOOT` to `RUNNING` transition and orchestrates
  the player update plus deterministic OAM reconstruction.
- `src/input.c` derives current, pressed and released button masks from the raw
  hardware sample.
- `src/rng.c` implements deterministic `xorshift16` state and output functions.
- `src/player.c` owns the compact mutable player state, bounded 8-direction
  movement, horizontal facing, animation selection and player render policy.
- `src/animation.c` is a reusable data-driven frame player. It stores only an
  animation ID, local frame and countdown timer; generated durations control
  looping, and a changed animation alone resets playback to frame zero.
- `src/metasprite.c` hides unused OAM entries and expands signed relative tile
  records into the existing OAM shadow. Its optional horizontal mirror adjusts
  both geometry and the hardware flip bit.
- `src/soldier_animation_data.c` consolidates the separately generated Soldier
  idle and walking exports under `soldier` symbols. The 21 tile records, 3
  frames and 2 definitions retain their generated values; only aggregate
  offsets and names changed.
- `include/tuning.h` contains the player start, speed, logical 24x24 bounds and
  initial architectural capacities.

## Player and Soldier naming boundary

`player` means the runtime entity controlled through controller 1. `PlayerState`,
`PlayerFacing`, the `player_*` API and the `PLAYER_*` position/movement limits
therefore remain character-independent. `AnimationPlayer` also remains generic:
it is a reusable animation playback cursor, not the playable character identity.

`soldier` means the concrete art currently bound to that runtime entity.
`soldier_animation_data`, `SOLDIER_ANIMATION_*`, the internal
`soldier_animation_sprites`/frames/definitions tables and
`soldier_sprite_palette` are asset-specific. The player module is the only C
integration point that selects Soldier definitions and mirrors the current
metasprite when the facing direction is left.

The 8 KiB `assets/game.chr` bank is linked through `src/chr.s`. Soldier sprites
use pattern table `$0000`, matching generated tile indexes `$00-$07`.
Backgrounds use `$1000`, whose tile zero is blank, so a cleared nametable remains
black. Startup loads the 16-byte Soldier sprite palette into `$3F10-$3F1F` from
the `soldier_sprite_palette` constant while rendering and NMI are disabled;
Soldier's metasprite records select sprite palette 0.

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
5. The main loop reads controller 1, updates player policy and animation, hides
   old OAM entries, then emits the current seven-sprite metasprite. This work is
   outside NMI and completes immediately after frame synchronization.

Because OAM DMA runs before that main-loop reconstruction, a newly built shadow
becomes visible at the following NMI. Runtime tests therefore sample movement
phase boundaries one frame later; this is the intended one-frame render pipeline,
not missed input or an extra gameplay update.

The controller bit layout is A, B, Select, Start, Up, Down, Left and Right in
bits 7 through 0. DMC is disabled, so DMA cannot corrupt the serial controller
read. Opposite directions on one axis cancel on that axis. A pure vertical move
selects movement according to remembered horizontal facing. Diagonals update
both axes without normalization.

OAM behavior is deterministic: all 64 entries upload every NMI; construction
begins by hiding all entries, then first-come render calls receive priority. The
controlled Soldier currently consumes seven entries even though its logical
area is 3x3 tiles because transparent tiles were omitted by the exporter. The
player integration keeps the same 24-pixel anchor for both facings and mirrors
the current metasprite at render time when the Soldier faces left, so there is
no separate movement-left anchor shift.

## Animation data and reuse

`AnimationData` keeps immutable sprite, frame and animation tables separate from
the three-byte `AnimationPlayer`. It has no knowledge of input, player state or
OAM. `OamRenderer` likewise accepts any generated metasprite records and has no
player dependency. Enemies, NPCs and pickups can therefore own their own compact
playback state and call the same renderer without duplicating controller or
character policy.

The JSON exports remain authoring reference only and are not parsed by the ROM.
Regeneration requires reconsolidating names/offsets in
`src/soldier_animation_data.c`; no gameplay switch contains hardcoded frame
tiles. The generic `player` module currently selects `soldier_animation_data` at
its character-integration boundary and relies on runtime mirroring for left
facing; no character registry or selection system exists yet.

## Deterministic RNG

`xorshift16` uses two bytes of state and shifts `(7, 9, 8)`. A zero seed is
normalized to one because zero is the algorithm's absorbing state. The generator
is fast and reproducible but is not cryptographic. Gameplay and cosmetic streams
should be separated later if shared consumption would prevent reproducible tests.

## Incremental boundaries for future milestones

Future systems should be added only when their milestone requires them:

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
