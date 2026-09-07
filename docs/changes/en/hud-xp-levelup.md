# Branch Development Log: hud-xp-levelup

## 2026-09-06 — Added Full-Background HUD, XP Progression and Level-Up Menu

### What changed

Implemented a complete HUD rendered on background Nametable 0 (rows 0-1), XP accumulation from collected gems with an integer progression curve, and a deterministic 3-choice level-up choice modal under `GAME_STATE_LEVEL_UP`:

1. **Background Pattern Table & Tiles:**
   - Populated Background Pattern Table 1 (`$1000-$1FFF`) in `src/chr.s` with font glyphs (ASCII uppercase, digits `0`-`9`, punctuation `+`, `-`, `:`, `>`, `|`) and HUD tiles (heart, slot frame, sword icon, empty bar, filled bar).
   - Preserved Sprite Pattern Table 0 (`$0000-$0FFF`) for player, sword, enemy, and gem sprites.

2. **Full-Background HUD (Rows 0-1):**
   - Row 0: HP label, heart icon, 6-segment health bar, XP label, 10-segment XP bar, and 2-digit level counter.
   - Row 1: 4 weapon slots (pre-equipped sword in slot 0) and 4 passive bonus slots.
   - Consumes 0 hardware sprites, preserving all 64 sprites for gameplay entities.

3. **Bounded VRAM Buffer & VBlank Delivery:**
   - Implemented a bounded single-burst VRAM transfer buffer (`_vram_buffer_len <= 10` bytes) in `src/nes.s` and `src/hud.c`.
   - Transferred safely during VBlank in `src/nmi.s` before scroll restoration, taking < 200 cycles.

4. **XP Progression Curve:**
   - Integrated `xp_gem_update` return value directly with `player_add_xp()`.
   - Precomputed integer threshold table (`level_thresholds: 5, 12, 22, 35, 52, 75, 105, 145, 200`) avoiding slow software floating-point operations.

5. **Level-Up Choice System:**
   - Added `GAME_STATE_LEVEL_UP` to `game_flow`.
   - Deterministic pause halts player, enemy movement, collision checks, attack timers, and gem collection.
   - Central 16x8 modal presents 3 choices navigated by D-Pad UP/DOWN and confirmed with `BUTTON_A` or `BUTTON_START`.
   - Confirmation applies upgrade effect, deducts threshold XP, advances level, cleans up overlay, and smoothly resumes gameplay.

### Relevant code

```c
/* Bounded VRAM queueing in src/hud.c */
if ((dirty_flags & DIRTY_XP) != 0U) {
    fill_count = (uint8_t)(((uint32_t)cached_xp * HUD_XP_BAR_TILES) / cached_next_level_xp);
    vram_buffer_addr_hi = 0x20U;
    vram_buffer_addr_lo = 0x12U;
    for (i = 0U; i < HUD_XP_BAR_TILES; ++i) {
        vram_buffer_data[i] = (i < fill_count) ? HUD_TILE_BAR_FULL : HUD_TILE_BAR_EMPTY;
    }
    vram_buffer_len = HUD_XP_BAR_TILES;
    dirty_flags &= ~DIRTY_XP;
}
```

```s
; VBlank transfer execution in src/nmi.s
@check_vram_buffer:
    lda _vram_buffer_len
    beq @restore_scroll
    lda PPUSTATUS
    lda _vram_buffer_addr_hi
    sta PPUADDR
    lda _vram_buffer_addr_lo
    sta PPUADDR
    ldx #$00
@vram_copy:
    lda _vram_buffer_data,x
    sta PPUDATA
    inx
    cpx _vram_buffer_len
    bne @vram_copy
    lda #$00
    sta _vram_buffer_len
```

### NES considerations

- **0 Sprites for HUD:** Moving all HUD elements to background tiles prevents sprite flicker and preserves the 8-sprites-per-scanline hardware limit for gameplay entities.
- **NMI/VBlank Safety:** PPU writes are strictly isolated to VBlank and capped at 10 bytes per frame, well within the 2,273-cycle NTSC VBlank limit.
- **Integer Math:** All scaling and threshold math uses integer operations (`uint8_t`, `uint16_t`, `uint32_t`).

### Resource and memory impact

- **Zero Page:** 28 bytes used (0 bytes delta).
- **RAM / BSS:** 188 bytes used (+60 bytes delta, 287 bytes free headroom).
- **PRG-ROM:** 10,597 bytes used (+2,194 bytes delta, 22,171 bytes / 67.7% free headroom).
- **CHR-ROM:** 8,192 bytes (Pattern Table 0 for sprites, Pattern Table 1 for background).
- **OAM Shadow:** 256 bytes (0 sprites allocated to HUD).

### Exact validation performed

- `make clean; make test`: Clean build with zero warnings under `--warnings-as-errors`, all host-side logic unit tests passed in `sim65`.
- `python tests/validate_rom.py`: ROM header, mapping, and label integrity verified.
- `make test-runtime`: Mesen emulator automated test runners passed:
  - `tests/mesen_game_states.lua`: Passed (175 frames).
  - `tests/mesen_player.lua`: Passed (450 frames).
  - `tests/mesen_player_damage.lua`: Passed (546 frames).
