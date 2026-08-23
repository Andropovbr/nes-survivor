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
