# Branch development log: main

## 2026-08-23 — Added the first initial-screen game-state machine

### What changed

Added explicit `PRESENTED_BY`, `TITLE` and `PLAYING` states, centered credit and
title nametables, a 150-frame credit timeout, a 30-frame title blink, deferred
gameplay initialization, a sparse background font, host tests and Mesen runtime
coverage. Build scripts and bilingual architecture/memory/readme documentation
were updated with the final behavior and budgets.

### Why

The ROM previously initialized gameplay immediately and only changed an
internal `BOOT` marker to `RUNNING`. Explicit pre-run states establish a small,
reviewable lifecycle boundary for later character selection without adding the
menus, pause or progression systems outside this milestone.

### Relevant code

```c
case GAME_STATE_PRESENTED_BY:
    if ((pressed_buttons & BUTTON_START) != 0U) {
        enter_title();
    } else if (state_timer > 1U) {
        --state_timer;
    } else {
        enter_title();
    }
    break;
```

The state policy consumes the existing pressed-edge mask, so held START cannot
cross two screens. Screen entry remains separate from state logic.

### NES considerations

Full nametable changes disable NMI/rendering, clear 1,024 bytes, wait for VBlank
and restore the fixed scroll/PPU configuration. The blink writes 11 tiles with the same latch
protection. Text uses background CHR and consumes no OAM. Gameplay state dispatch
is completed inside `game_init()` so the measured gameplay hot path has no new
per-frame branch or C call.

### Performance

The first runtime-dispatch integration lost four updates in the 12-Bat stress
scenario and was removed. Final Mesen measurement: 1,750 video frames, 12 active
Bats reached, 1,735 post-transition gameplay NMIs/updates and zero skipped
gameplay updates. VRAM-write cycle counts were not measured separately.

### Resource impact

- PRG-ROM: 6,205 -> 7,099 bytes (`+894`).
- BSS: 78 -> 80 bytes (`+2`).
- Zero page, DATA, OAM shadow, hardware sprites and stacks: unchanged.
- CHR: 21 nonblank background glyphs added in the previously unused `$1000`
  pattern table; sprite tiles `$00-$0D` remain unchanged.

### Main files affected

`src/game.c`, `src/game_flow.c`, `src/screen.c`, `src/chr.s`, related headers,
build scripts, C/Python/Lua tests, READMEs, architecture/memory documents and
`docs/implementation-notes/game-states.md`.

### Validation

- clean ROM build: PASS
- sim65 host-side logic tests: PASS
- structural ROM/CHR/map validation: PASS
- Mesen initial-screen runtime (110 frames): PASS
- Mesen player/sword/Bat runtime (450 frames): PASS
- Mesen 12-Bat stress (1,750 frames): PASS, zero gameplay skips
- warnings-as-errors build and `git diff --check`: PASS at final validation

### Limitations / follow-up

Transitions are direct cuts and may keep rendering off across more than one
video frame. The font is fixed and monochrome. Timing assumes NTSC. A future
state that interrupts active gameplay needs a separately measured dispatcher;
this change only establishes the pre-run lifecycle.

## 2026-08-23 — Added bounded XP-gem drops and uppercase screens

### What changed

Every sword defeat now creates an 8x8 gem centered on the Bat. A fixed
eight-slot pool handles spawn, contact disappearance, saturated condensation
and deterministic rendering. The supplied CHR adds gem tile `$14`. All visible
strings and their sparse background font are now uppercase in pattern table 1.

### Why

Defeated enemies needed to leave visible pickups without implementing XP gain
or progression. The pool and condensation policy provide bounded RAM/OAM cost
and avoid silently dropping events when all visible slots are occupied.

### Relevant code

```c
index = sword_hitbox_scan_parity;
sword_hitbox_scan_parity ^= 1U;
for (; index < pool_high_water; index = (uint8_t)(index + 2U)) {
    xp_gem_spawn((uint8_t)(bat_x + 4U), bat_y);
}
```

The active sword alternates even and odd Bat slots. Each enemy is still tested
within two active frames, while a defeat emits its gem before clearing the slot.

### NES considerations

The gem pool uses no heap. Eight visible gems consume eight OAM entries and
render after enemies, preserving player/sword/threat priority. A full pool
increments the nearest gem's represented-drop count. Player contact checks one
slot per frame, bounding recurring work with at most eight frames of latency.
Uppercase nametable indexes still select the background table at `$1000`; sprite
tile `$14` remains in the sprite table at `$0000`.

### Performance

Scenario: 1,750-frame Mesen stress with 12 Bats.

- baseline: 1,735 NMIs / 1,735 updates / 0 skipped;
- first gem integration: 1,735 / 1,727 / 8 skipped;
- final staggered collision: 1,735 / 1,735 / 0 skipped.

Individual routine cycle counts were not measured.

### Resource impact

- PRG-ROM: 7,043 -> 7,994 bytes (`+951`);
- BSS: 81 -> 124 bytes (`+43`);
- zero page, DATA, OAM shadow and stack reservations: unchanged;
- CHR-ROM capacity: unchanged at 8 KiB;
- sprite content: one new tile at `$14`;
- worst-case hardware sprites: 33 -> 41 of 64.

### Main files affected

`src/xp_gem.c`, `include/xp_gem.h`, `src/enemy.c`, `src/game.c`, `src/screen.c`,
`src/chr.s`, `assets/game.chr`, tuning/build files, host/Python/Mesen tests,
READMEs, architecture, memory budgets and the Portuguese implementation note.

### Validation

- clean warnings-as-errors ROM build: PASS
- sim65 logic tests, including spawn/contact/saturation/OAM: PASS
- structural ROM/CHR/map validation: PASS
- Mesen initial screens (175 frames): PASS
- Mesen gameplay (450 frames): PASS
- Mesen 12-Bat/gem stress (1,750 frames): PASS, zero skipped updates
- `git diff --check`: PASS

### Limitations / follow-up

Collection grants no XP and discards the represented count as explicitly scoped.
The single-frame gem has no visible animation change. Contact processing may
take eight frames. Scanline flicker management and wave-end collection remain
future work.
