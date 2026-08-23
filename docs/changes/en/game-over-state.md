# Branch development log: game-over-state

## 2026-08-23 — Added player contact damage, hit audio and game over

### What changed

Added tunable five-HP player state, a 30-frame post-hit cooldown, a facing-aware
16x16 lower-body hurtbox, bounded Bat contact detection, an explicit
`GAME_OVER` state and screen, a visible `M` glyph, START return to title, and a
short noise-channel hit effect. Added host, runtime and performance coverage
plus synchronized architecture, memory and README documentation.

### Why

Enemies previously overlapped the player without consequence, so a run had no
failure condition or hit feedback. This establishes the smallest complete
damage loop without adding HUD, knockback, music or progression.

### Relevant code

```c
if (player.hp == 0U || player.hit_cooldown != 0U) {
    return 0U;
}
--player.hp;
player.hit_cooldown = PLAYER_HIT_COOLDOWN_FRAMES;
```

The saturating guard prevents underflow and repeated damage. The countdown is
advanced once per gameplay update, so another hit becomes eligible after 30
complete updates.

```c
return (uint8_t)(player.x +
    (player.facing == PLAYER_FACING_LEFT ? 0U : 8U));
```

The 16x16 damage box follows the lower body: `y + 8`, with its horizontal
origin moved toward the facing direction. The rear empty/scabbard pixels are
therefore visual only. The CHR font also defines tile `$4D` instead of reserving
it blank, so the existing `GAME OVER` nametable string renders its `M`.

```c
if (player_vulnerable != 0U &&
    (sword_active == 0U || collision_phase == 0U)) {
    /* scan one Bat contact slot */
}
```

One rotating slot bounds the AABB work. When sword and player collisions are
both due, a deterministic phase alternates them rather than stacking peaks.

### NES considerations

All gameplay policy remains in C. The only new Assembly is the small stable APU
ABI: no parameters, no return, A/flags clobbered, no RAM/ZP, main-thread only.
It configures the noise length counter, so NMI receives no audio work. OAM and
CHR usage do not change. The game-over nametable transition follows the existing
render-disabled PPU path and physical OAM hides on the following DMA.

### Performance

Measured in Mesen using the dedicated 255-HP instrumentation build:

- full Bat contact scan: 21 skipped updates;
- one-slot scan without collision scheduling: 4 skipped updates;
- final scheduled version: 0 skipped updates;
- final scenario: 1,750 frames, 12 Bats, 2 gems, 1,735 NMIs/updates.

The HP override changes only the initial tuning immediate so the load run cannot
end; the released ROM defaults to five HP. No standalone cycle count was taken.

### Resource impact

- PRG-ROM: 7,994 -> 8,403 bytes (`+409`);
- BSS: 124 -> 128 bytes (`+4`: HP, cooldown, contact cursor, phase);
- RAM free: 351 -> 347 general bytes;
- zero page, DATA, OAM shadow and stacks: unchanged;
- meaningful CHR: +16 bytes for the missing `M` glyph (18 text glyphs total);
- hardware sprites: unchanged at a 41/64 worst case.

### Main files affected

`include/tuning.h`, player/enemy/game-flow/screen/NES interfaces and sources,
`src/chr.s`, tests and build scripts, both READMEs, architecture/memory
documents, and the Portuguese implementation note.

### Validation

- explicit Make discovery commands: unavailable on this Windows host;
- clean PowerShell ROM build: PASS;
- sim65 logic tests: PASS;
- structural ROM/CHR/map validation: PASS;
- Mesen initial screens and gameplay: PASS;
- Mesen five-hit/APU/game-over/START scenario: PASS (game over at frame 549);
- Mesen 12-Bat performance scenario: PASS, zero skipped updates;
- compiler/assembler/linker warnings-as-errors: PASS.

### Limitations / follow-up

There is no HP HUD, invulnerability flash, knockback, music mixer or per-enemy
damage. The 16x16 hurtbox intentionally excludes the upper eight pixels and the
back eight pixels. Full-pool contact recognition can take up to 12 scan opportunities.
The headless test verified APU register writes but did not perform subjective
listening. PAL/Dendy timing remains unsupported.
