# Architecture Specification: Full-Background HUD, XP Progression & Level-Up System

- **Date:** 2026-09-06
- **Status:** Approved Draft for Implementation
- **Target Platform:** NES (NROM-256 / Mapper 0, NTSC 60 Hz)
- **Toolchain:** cc65 / ca65 / ld65

---

## 1. Overview & Objectives

This specification defines the architecture, data structures, rendering strategy, and state machine integration for:
1. **Full-Background HUD:** Displays player HP, XP progress, player level, 4 weapon slots (slot 0 pre-equipped with Soldier's Sword), and 4 passive bonus slots. Rendered entirely on the background nametable using Background Pattern Table 1.
2. **Pattern Table Separation:**
   - **Pattern Table 0 (`$0000-$0FFF`):** Reserved exclusively for sprites (Player, Sword attack, Bat enemies, XP gems).
   - **Pattern Table 1 (`$1000-$1FFF`):** Reserved exclusively for background tiles (arena boundaries, font/typography glyphs, HUD frames, bars, icons, and menus).
   - Graphic data source: `assets/game.chr` (8 KiB total; first 4 KiB for sprites, second 4 KiB for background).
3. **XP Accumulation & Progression:** Collection of dropped XP gems feeds player XP accumulation. When XP reaches the level threshold, level advancement occurs.
4. **Level-Up Choice System:** A dedicated game state (`GAME_STATE_LEVEL_UP`) pauses active gameplay and presents a 3-choice upgrade modal. The player navigates choices with the controller and confirms selection with the 'A' or 'START' button.

---

## 2. Pattern Table and CHR-ROM Architecture

### 2.1 Hardware Configuration
- `PPUCTRL` (`$2000`) configuration during gameplay:
  - Bit 7: NMI enabled (`1`)
  - Bit 4: Background pattern table address = `$1000` (Pattern Table 1) (`1`)
  - Bit 3: Sprite pattern table address = `$0000` (Pattern Table 0) (`0`)
  - Bit 0-1: Base nametable address = `$2000` (`00`)
  - Constant in `include/nes.h`: `NES_PPUCTRL_GAME = $90` (`NES_PPUCTRL_NMI_ENABLE | NES_PPUCTRL_BACKGROUND_TABLE_1000`).

### 2.2 CHR-ROM Layout (`assets/game.chr`)
The cartridge CHR-ROM consists of an 8,192-byte binary:
- **`$0000-$0FFF` (Pattern Table 0 - Sprites):**
  - `$00-$07`: Soldier player metasprite frames.
  - `$08-$09`: Animated Sword attack frames.
  - `$0A-$0D`: Bat enemy metasprite frames.
  - `$14`: XP gem 8x8 sprite.
  - Remaining sprite tile space reserved for future entities.
- **`$1000-$1FFF` (Pattern Table 1 - Background):**
  - Font glyphs (ASCII uppercase `$20-$5A`, digits `'0'-'9'`).
  - HUD frame tiles: corners, horizontal borders, vertical borders.
  - Gauge tiles: segmented bar fills (empty `$00`, 25%, 50%, 75%, 100% full).
  - Slot tiles: empty weapon/bonus slot icon box, Soldier Sword icon.
  - Menu borders and selection cursor tile.

---

## 3. HUD Layout and Background Rendering

### 3.1 Nametable Layout
The HUD occupies the top two rows of Nametable 0 (Row 0 and Row 1: 32 columns x 2 rows = 64 tiles at `$2000-$203F`):

```text
Row 0 ($2000-$201F):
 Col  0- 4: "HP:" [Heart Icon]
 Col  5-10: [HP BAR: 6 segmented bar tiles] (current HP / max HP)
 Col 11-13: " "
 Col 14-17: "XP:"
 Col 18-27: [XP BAR: 10 segmented bar tiles] (current XP / next level XP)
 Col 28-29: "LV"
 Col 30-31: [2-digit Level, e.g. "01"]

Row 1 ($2020-$203F):
 Col  0- 4: "WPN:"
 Col  5- 6: [Slot 0: SWORD]
 Col  7- 8: [Slot 1: EMPTY]
 Col  9-10: [Slot 2: EMPTY]
 Col 11-12: [Slot 3: EMPTY]
 Col 13-15: " "
 Col 16-20: "BNS:"
 Col 21-22: [Bonus 0: EMPTY]
 Col 23-24: [Bonus 1: EMPTY]
 Col 25-26: [Bonus 2: EMPTY]
 Col 27-28: [Bonus 3: EMPTY]
 Col 29-31: "   "
```

### 3.2 OAM Budget Independence
Because the entire HUD is constructed on the background nametable:
- **HUD OAM Consumption:** 0 sprites.
- **Gameplay Sprite Availability:** 64 sprites remain 100% dedicated to player metasprites, sword attacks, bat enemies, and XP gems.
- No scanline sprite-limit pressure is introduced in rows 0-1.

### 3.3 Dynamic Tile Updates via Bounded VRAM Transfer Buffer
Static labels, borders, and empty slot outlines are drawn once during screen transition into gameplay (`screen_show_gameplay()`).

During active gameplay:
1. Dynamic components are:
   - HP bar fill tiles (up to 6 tiles).
   - XP bar fill tiles (up to 10 tiles).
   - Level digits (2 tiles).
   - Slot changes (only when weapons or bonuses are acquired or upgraded).
2. A lightweight VRAM transfer buffer (up to 32 bytes) is queued in BSS during the main loop when changes occur (`hud_dirty` flag).
3. The NMI handler applies the queued transfer to `$2000-$203F` during VBlank before restoring scroll, safely within the ~2,273-cycle NTSC VBlank window (~180 CPU cycles for a 16-byte transfer).

---

## 4. XP and Progression System

### 4.1 Player Progression State
The player runtime state in `src/player.c` is extended:
```c
typedef struct {
    uint8_t  hp;
    uint8_t  max_hp;
    uint16_t xp;            /* XP accumulated toward next level */
    uint16_t next_level_xp; /* XP threshold to reach next level */
    uint8_t  level;         /* Current level (starts at 1) */
    uint8_t  weapons[4];    /* Equipped weapon IDs (0: Sword, 0xFF: Empty) */
    uint8_t  bonuses[4];    /* Equipped bonus IDs (0xFF: Empty) */
    /* ... existing position, movement, hit_cooldown, and animation fields ... */
} PlayerState;
```

### 4.2 Integer-Only Progression Curve
In compliance with `AGENTS.md` (no floating point, deterministic integer math):
Level thresholds are managed via a precomputed integer threshold table or integer arithmetic:
```c
static const uint16_t level_thresholds[] = {
    5U,    /* Level 1 -> 2: 5 gems */
    12U,   /* Level 2 -> 3: 12 gems */
    22U,   /* Level 3 -> 4: 22 gems */
    35U,   /* Level 4 -> 5: 35 gems */
    52U,   /* Level 5 -> 6: 52 gems */
    75U,   /* Level 6 -> 7: 75 gems */
    105U,  /* Level 7 -> 8: 105 gems */
    145U,  /* Level 8 -> 9: 145 gems */
    200U   /* Level 9 -> 10: 200 gems */
};
```
For levels exceeding the table, a deterministic linear increment is applied (`threshold += 60U`).

### 4.3 Gem Collection Integration
In `src/xp_gem.c`:
- When player collision with an active gem occurs:
  - The drop count `gem_drop_units[index]` is transferred to `player_add_xp(count)`.
  - The gem slot is deactivated (`gem_active[index] = 0; gem_drop_units[index] = 0;`).
- In `player_add_xp(uint16_t amount)`:
  - `player.xp += amount;`
  - If `player.xp >= player.next_level_xp`, the player sets `level_up_pending = 1`.

---

## 5. Level-Up Choice System (`GAME_STATE_LEVEL_UP`)

### 5.1 Game State Transition
- `GameState` enum in `include/game_flow.h` is extended:
  ```c
  typedef enum GameState {
      GAME_STATE_PRESENTED_BY = 0,
      GAME_STATE_TITLE,
      GAME_STATE_PLAYING,
      GAME_STATE_LEVEL_UP,
      GAME_STATE_GAME_OVER
  } GameState;
  ```
- When `level_up_pending` is detected at the end of the gameplay update, `game_flow_enter_level_up()` transitions the game to `GAME_STATE_LEVEL_UP`.

### 5.2 Deterministic Pause Behavior
- While in `GAME_STATE_LEVEL_UP`:
  - Player movement and animation updates are paused.
  - Bat enemy movement, spawning, and separation are paused.
  - Weapon cooldowns and attack active timers are paused.
  - XP gem timers and collection are paused.
  - Existing sprites (player, enemies, gems) remain visible at their fixed coordinates.

### 5.3 Level-Up Overlay Dialog
- A 16-column x 8-row background window is drawn in the center of the screen (Rows 10-17, Columns 8-23):
  ```text
  +----------------+
  |   LEVEL UP!    |
  |                |
  | > 1. SWORD +1  |
  |   2. MAX HP +1 |
  |   3. SPEED +1  |
  |                |
  +----------------+
  ```
- Three placeholder upgrade choices are presented:
  1. Upgrade Weapon: Sword damage/speed increase.
  2. Stat Upgrade: Max HP + 1 (heals player for 1 HP).
  3. Stat Upgrade: Movement speed / cooldown improvement.

### 5.4 Choice Selection and Application
- **Navigation:** D-pad UP and DOWN move the cursor index (`cursor_index` from 0 to 2).
- **Confirmation:** Pressing `BUTTON_A` or `BUTTON_START`:
  - Applies selected upgrade effect (or placeholder stub).
  - Deducts `player.next_level_xp` from `player.xp`.
  - Increments `player.level`.
  - Updates `player.next_level_xp` to the next milestone.
  - Clears the overlay window (restores arena background tiles).
  - Flags HUD as dirty to refresh Level, HP, and XP bars.
  - Transitions `game_flow` state back to `GAME_STATE_PLAYING`.

---

## 6. Resource and Timing Budgets

### 6.1 RAM Budget
- Extended player state: +6 bytes (XP, next threshold, level, equipment arrays).
- HUD update buffer and dirty flags: ~16-20 bytes in BSS.
- Level-up menu cursor and choice state: 2 bytes.
- Total new RAM impact: < 30 bytes (well within the 347 bytes of free general RAM).

### 6.2 ROM Budget
- Code, strings, and threshold tables: estimated ~400-600 bytes PRG-ROM.
- PRG-ROM currently has 24,365 bytes free (74.3% free headroom).

### 6.3 VBlank Timing Budget
- NMI baseline: ~590 cycles.
- HUD nametable write (up to 16 bytes): ~180 cycles.
- Total VBlank usage: ~770 cycles out of 2,273 available cycles (< 35% of VBlank limit).

---

## 7. Validation and Verification Plan

1. **Compilation & Static Checks:**
   - Compile-time asserts for threshold tables and array sizes.
   - Zero compiler warnings with `--warnings-as-errors`.
2. **Logic Unit Tests (`sim65`):**
   - Test XP addition and threshold crossing logic.
   - Test level increment, XP rollover/deduction, and threshold advancement.
   - Test level-up state entry and exit.
3. **Mesen Emulator Verification:**
   - Verify HUD visual layout: HP bar, XP bar, Level, 4 weapon slots, 4 bonus slots.
   - Verify gem pickup advances XP bar in real time.
   - Verify reaching threshold pauses gameplay and displays Level-Up dialog.
   - Verify cursor navigation with D-Pad and confirmation with A/Start button.
   - Verify unpausing resumes gameplay seamlessly with updated stats and zero sprite flicker/corruption.
4. **Linker & Budget Audit:**
   - Execute `nes-budget` to record zero page, BSS, and ROM delta.
