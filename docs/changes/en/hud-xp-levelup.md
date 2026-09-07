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
/* Event-driven VRAM buffering with 16-bit math in src/hud.c */
if ((dirty_flags & DIRTY_XP) != 0U) {
    fill_count = (uint8_t)(((uint16_t)cached_xp * HUD_XP_BAR_TILES) / cached_next_level_xp);
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
    beq @check_cursor_update
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

@check_cursor_update:
    lda _screen_level_up_cursor_update
    beq @restore_scroll
    ; transfers 3 cursor column tiles safely during VBlank
```

### NES considerations

- **0 Sprites for HUD:** Moving all HUD elements to background tiles prevents sprite flicker and preserves the 8-sprites-per-scanline hardware limit for gameplay entities.
- **NMI/VBlank Safety:** PPU writes are strictly isolated to VBlank and capped at 10 bytes per frame (plus 3 cursor bytes during modal navigation), avoiding forced mid-frame blanking and OAM corruption.
- **Integer Math & Event-Driven Dirty Flags:** Replaced per-frame XP polling and 32-bit math runtime helper calls with event-driven notifications (`hud_notify_*`) and 16-bit arithmetic, maintaining 0 skipped frames under maximum enemy load.

### Resource and memory impact

- **Zero Page:** 28 bytes used ($1C of $1E, 2 bytes free headroom, unchanged).
- **RAM (DATA + BSS):** 219 bytes used (37 bytes DATA + 182 bytes BSS, +54 bytes delta from 165 bytes in base, 293 bytes free headroom).
- **PRG-ROM:** 10,307 bytes used (+1,904 bytes delta from 8,403 bytes in base, 22,461 bytes / 68.5% free headroom).
- **CHR-ROM:** 8,192 bytes (Pattern Table 0 for sprites, Pattern Table 1 for background font and HUD).
- **OAM Shadow:** 256 bytes (0 sprites allocated to HUD, all 64 sprites available for gameplay).

### Exact validation performed

- `make clean; make test`: Clean build with zero warnings under `--warnings-as-errors`, all host-side logic unit tests passed in `sim65`.
- `python tests/validate_rom.py`: ROM header, mapping, and label integrity verified.
- `make test-runtime`: Mesen emulator automated test runners passed:
  - `tests/mesen_game_states.lua`: Passed (175 frames).
  - `tests/mesen_player.lua`: Passed (450 frames).
  - `tests/mesen_player_damage.lua`: Passed (546 frames).
  - `tests/mesen_level_up.lua`: Passed (77 frames; verifies XP gain, modal pause, cursor navigation, stat application, and unpause).
- `make test-performance`: Mesen bat stress runner passed:
  - `tests/mesen_bat_stress.lua`: Passed (1,750 frames, 12 bats max, 1,740 NMIs, 1,740 updates, `gameplay_skipped=0`).
