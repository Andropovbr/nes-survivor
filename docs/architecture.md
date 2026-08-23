# Architecture

## Current modules

- `src/crt0.s` owns the iNES header, reset path, RAM and PPU initialization,
  cc65 runtime startup, rendering enable and interrupt vectors.
- `src/nmi.s` is the bounded NMI handler. It uploads the OAM shadow page, restores
  zero scroll and advances the frame counter.
- `src/nes.s` owns the page-aligned OAM allocation, frame wait primitive,
  controller-port read routine and bounded noise-channel hit effect.
- `src/main.c` orchestrates initialization and the synchronized main loop.
- `src/game.c` coordinates screen transitions, initializes gameplay on entry to
  `PLAYING`, applies scheduled player/sword collisions, triggers hit audio and
  reconstructs OAM. Non-playing states use a cold dispatcher.
- `src/game_flow.c` owns the explicit `PRESENTED_BY`, `TITLE`, `PLAYING` and
  `GAME_OVER` states plus their deterministic transitions and frame timers.
- `src/screen.c` performs fixed-screen nametable, palette and text writes for
  credit, title, gameplay entry and game over.
- `src/input.c` derives current, pressed and released button masks from the raw
  hardware sample.
- `src/rng.c` implements deterministic `xorshift16` state and output functions.
- `src/player.c` owns compact HP/invulnerability state, bounded 8-direction
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
- `src/weapon_sword.c` owns the two-byte automatic-sword runtime and the exact
  two-tile generated attack frame. It renders after the player, in front of the
  remembered horizontal facing, exposes the matching active hitbox, and omits
  fully offscreen attacks at arena edges.
- `src/enemy.c` owns the fixed 12-slot Bat pool, edge spawning, shared Q4
  pursuit timing and animation, sword collision and deterministic enemy
  rendering.
- `src/bat_animation_data.c` adapts the attached generated Bat frames and tiles
  to the shared immutable animation format.
- `src/xp_gem.c` owns the fixed eight-slot gem pool, saturated-drop
  condensation, staggered player-contact checks and one-sprite rendering.
- `include/tuning.h` contains player/sword/Bat geometry, speeds, attack and spawn
  timing, arena bounds and fixed gameplay capacities.

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

The first 4 KiB of `assets/game.chr` is linked through `src/chr.s`. Soldier uses
`$00-$07`, the animated sword `$08-$09`, Bat `$0A-$0D`, and the XP gem at `$14`
in sprite pattern table `$0000`. The background table at `$1000` contains only
the sparse uppercase one-bitplane ASCII glyphs required by the initial screens;
tile zero remains blank. Startup
loads the sprite palettes, while the screen module loads the minimal black/white
background palette with rendering and NMI disabled.
Soldier and sword select palette 0; Bat selects palette 1 and the gem selects
palette 3.

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
3. NMI preserves A/X/Y, performs one 256-byte OAM DMA from `$0200`, optionally
   writes the fixed 11-tile title prompt, resets scroll to zero, increments an
   8-bit zero-page frame counter, restores registers and returns. Normal work is
   approximately 590 CPU cycles; the bounded prompt-show path is approximately
   788 cycles, comfortably inside the roughly 2,273-cycle NTSC VBlank.
4. `nes_wait_frame` snapshots the counter and waits until NMI changes it. An
   8-bit comparison is atomic on 6502; wraparound is safe because 256 NMIs cannot
   occur between the snapshot and comparison.
5. During initialization, `game.c` advances the credit/title state machine on
   NMI-synchronized input samples. Full nametable changes temporarily disable
   NMI and rendering, clear 1,024 bytes, write fixed text, wait for VBlank and
   restore zero scroll before re-enabling rendering.
6. After entry to `PLAYING`, initialization returns to `main`. The main loop
   updates player, automatic sword and Bat pursuit, schedules bounded contact
   and sword collision work, checks one gem, then rebuilds OAM in player,
   optional sword, enemy-pool and gem-pool order. Work remains outside NMI.

## Initial game states

`game_flow` starts at `GAME_STATE_PRESENTED_BY`, changes to `GAME_STATE_TITLE`
after 150 NTSC updates or a START edge, and changes to `GAME_STATE_PLAYING` only
on another START edge. The title prompt starts visible and toggles every 30
updates. Input edges come from the existing `current & ~previous` mask, so a
held START has no edge on the title screen.

The initial state loop lives in `game_init()`. Gameplay pools and the first OAM
image are created only while entering `PLAYING`. Runtime dispatch is one early
state check: `PLAYING` takes the hot path, while `GAME_OVER` and the returned
title use the cold screen coordinator. Reaching zero HP enters `GAME_OVER` and
writes its nametable; a new START edge enters `TITLE`, and another starts a new
run with all pools and HP reinitialized.

The 11-tile blink update is requested by C and consumed by the following NMI.
NMI reads `PPUSTATUS` to reset the shared `$2005/$2006` latch, sets `PPUADDR` to
`$220A`, writes the complete visible or blank prompt and restores scroll. It
does not toggle `PPUCTRL` or `PPUMASK`, so rendering remains stable. Full changes
are still direct cuts and may span multiple video frames while rendering is off.

The fixed `PPUCTRL` value is `$90`: bit 7 enables NMI, bit 4 selects background
pattern table 1 (`CHR $1000-$1FFF`), and clear bit 3 selects sprite pattern table
0 (`CHR $0000-$0FFF`). Nametable bytes remain ordinary 8-bit tile indexes; the
PPU bit chooses which 4 KiB table those indexes address.

Because OAM DMA runs before that main-loop reconstruction, a newly built shadow
becomes visible at the following NMI. Runtime tests therefore sample movement
phase boundaries one frame later; this is the intended one-frame render pipeline,
not missed input or an extra gameplay update.

The controller bit layout is A, B, Select, Start, Up, Down, Left and Right in
bits 7 through 0. DMC is disabled, so DMA cannot corrupt the serial controller
read. Opposite directions on one axis cancel on that axis. A pure vertical move
selects movement according to remembered horizontal facing. Diagonals update
both axes without normalization.

OAM behavior is deterministic: all 64 entries upload every NMI. Initialization
hides all entries once; each later construction pass hides only the entries used
by the preceding frame before first-come render calls receive priority. The
controlled Soldier currently consumes seven entries even though its logical
area is 3x3 tiles because transparent tiles were omitted by the exporter. The
player integration keeps the same 24-pixel anchor for both facings and mirrors
the current metasprite at render time when the Soldier faces left, so there is
no separate movement-left anchor shift.

The sword attacks at a fixed 60-frame period and is active for the first 12
frames of each period. Its 8x16 frame is vertically centered against the
player's 24-pixel logical area, anchored at `player.x + 24` when facing right or
`player.x - 8` when facing left, and horizontally flipped for left facing. A
fully offscreen sword is skipped instead of allowing unsigned OAM coordinates
to wrap it to the opposite screen edge. Its 8x16 hitbox uses the exact same
active-state, anchor and edge rules as rendering; overlapping Bats are removed.

## Bat pool and spawning

The first Bat is due after 120 gameplay frames. Successful later spawns reset
the timer to 120 frames (two seconds). Positions use deterministic gameplay RNG
and one of four arena edges. If all 12 slots are active, the due spawn retries on
a later frame rather than overwriting memory.

Bat positions are stored as byte-sized pixel coordinates. A shared Q4 movement
accumulator advances by six subpixels per update and emits a one-pixel step at
16 subpixels, preserving an average speed of 0.375 pixel per axis per update.
This is below the player's one-pixel step and follows the existing unnormalized
diagonal convention. Cosmetic animation state is also shared by all Bats. A
shrinking high-water mark keeps loop cost proportional to used pool slots.

Each Bat stores one byte of horizontal facing. Spawning initializes it from the
Bat's X position relative to the current target; later horizontal pursuit or
separation updates it, while purely vertical movement preserves it. The source
art faces right; left-facing rendering swaps the fixed metasprite's left/right
tiles and toggles each tile's hardware horizontal-flip bit. The specialized render path
uses one facing branch per Bat and keeps the same two OAM entries.

Separation uses top-left coordinate differences and a tunable 12x6-pixel
proximity box. One rotating active pair is inspected on each frame without a Q4
position step. If close, the cached result replaces pursuit on only the more
separated axis at the next movement step; pursuit continues on the other axis.
The two Bats move in opposite directions, exact overlaps use pool index order,
and arena bounds saturate the result. The rotating pair cursor eventually
inspects every used-slot pair without placing an O(n-squared) spike on one
frame. At 12 used slots a complete 66-pair scan takes up to about 106 gameplay
frames, so this first version deliberately favors bounded CPU cost over an
immediate rigid response.

Collision compares Bat 16x8 AABBs against the animated sword's 8x16 AABB only
during active attack frames. Bat/player contact uses a 16x16 lower-body AABB and
one rotating used slot per eligible frame. Its Y offset is eight pixels. Facing
right places it at `x + 8`; facing left places it at `x`, keeping the empty back
and rear scabbard extension outside damage. A detected contact removes one HP
and starts the tunable 30-frame cooldown; HP saturates at zero.

When the player is vulnerable while the sword is active, a one-byte gameplay
phase alternates contact and sword collision work instead of stacking both
peaks. Sword parity is therefore visited at least once every four active frames
in that overlap case. At 12 used slots, player contact can take up to 12 scan
opportunities to detect. This bounded latency is the trade-off that retained
zero skipped updates in the measured stress test.

Each accepted hit calls the C-facing Assembly routine
`nes_play_player_hit_sfx`. It enables the APU noise channel and writes `$1C`,
`$04` and `$10` to `$400C/$400E/$400F`; the hardware length counter ends the
impact after about ten NTSC frames without NMI or RAM state. No music engine or
other APU owner exists yet.

Inspection of cc65 output and Mesen frame counters identified repeated 16-bit
struct indexing, per-enemy animation state and generic metasprite calls as the
hot path. The pool now uses compact byte arrays, shared timing and a bounded
two-sprite Bat renderer in C. The current 1,750-frame Mesen stress run compensates
for the initial screens, reaches all 12 slots and records 1,735 gameplay
NMIs/updates after its post-transition baseline, with no skipped update. Enemy
and collision hot paths remain in C; the only new Assembly routine is the
bounded APU trigger described above.

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

## XP gem pool

The gem pool has eight fixed slots. Each slot stores byte X/Y, an active flag
and a 16-bit represented-drop count: five bytes per slot, 40 bytes total. Two
shared bytes hold the high-water mark and rotating collection cursor. Allocation
reuses the first inactive slot. If all slots are active, a new drop increments
the represented count of the nearest gem using an inexpensive Chebyshev-distance
comparison; no heap, multiplication or recurring nearest search is used.

One active gem is tested against the player's logical 24x24 AABB per frame, so
contact can take at most eight gameplay frames to remove a saturated-pool gem.
Collection clears the represented count but deliberately grants no XP yet.
Rendering occurs after enemies, giving the player, sword and threats priority;
each visible gem consumes one sprite using CHR tile `$14` and palette 3.

## Deterministic RNG

`xorshift16` uses two bytes of state and shifts `(7, 9, 8)`. A zero seed is
normalized to one because zero is the algorithm's absorbing state. The generator
is fast and reproducible but is not cryptographic. Gameplay and cosmetic streams
should be separated later if shared consumption would prevent reproducible tests.

## Incremental boundaries for future milestones

Future systems should be added only when their milestone requires them:

- immutable character definitions separated from per-run character state;
- immutable weapon definitions and compact runtime slots for automatic weapons;
- a fixed-size projectile pool and progression integration for the existing
  enemy and XP-gem pools;
- table-driven arena and wave definitions;
- eligibility, rarity and application layers for upgrades;
- unlock objectives and versioned password persistence.

Content should use compact IDs and array indexes, not ownership pointers or heap
allocation. Definition tables remain immutable; mutable run state remains in
fixed-size pools. Adding a character, weapon, enemy or stage should add one table
entry and only introduce specialized code for genuinely distinct behavior.

No empty future modules or speculative runtime structures exist yet. This keeps
the linker map honest and each future change reviewable.
