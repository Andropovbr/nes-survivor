# Full-Background HUD, XP Progression & Level-Up Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a full-background HUD (Nametable rows 0-1), XP accumulation from collected gems with an integer progression curve, and a deterministic 3-choice Level-Up modal (`GAME_STATE_LEVEL_UP`) for NES Survivor.

**Architecture:** The HUD is drawn entirely in background Nametable 0 (rows 0-1) using Pattern Table 1 (`$1000-$1FFF`), consuming 0 hardware sprites. Dynamic values (HP bar, XP bar, Level number, equipment slots) are buffered in BSS and transferred during VBlank in NMI. XP gem collection yields progression units to the player state, triggering a clean pause and overlay in `GAME_STATE_LEVEL_UP` upon reaching level thresholds.

**Tech Stack:** 6502 Assembly (ca65), C (cc65), NROM-256 linker config (ld65), sim6502 test runner (`make test`), Mesen emulator for runtime validation.

**Spec:** [2026-09-06-hud-xp-levelup-design.md](file:///G:/github/nes-survivor/docs/superpowers/specs/2026-09-06-hud-xp-levelup-design.md)

## Global Constraints

- Target: NES, NROM-256 / Mapper 0, NTSC 60 Hz.
- Sprite Pattern Table: Pattern Table 0 (`$0000-$0FFF`).
- Background Pattern Table: Pattern Table 1 (`$1000-$1FFF`).
- Game `PPUCTRL`: `$90` (`NES_PPUCTRL_NMI_ENABLE | NES_PPUCTRL_BACKGROUND_TABLE_1000`).
- No floating-point operations; all arithmetic is fixed-width integer (`uint8_t`, `uint16_t`).
- No heap allocation; fixed-size pools and static/BSS allocations only.
- 0 sprites consumed by HUD during active gameplay.
- Build must pass `make test` without warnings (`--warnings-as-errors`).

---

## File Structure

```text
include/
  player.h            -> Extended with XP, Level, and equipment query/mutator functions
  game_flow.h         -> Extended with GAME_STATE_LEVEL_UP and state transition helpers
  tuning.h            -> Level-up tuning constants (threshold curve, slot counts)
  hud.h               -> [NEW] HUD definitions, tile constants, buffer structures, and API
  level_up.h          -> [NEW] Level-up modal choice state and navigation API
  nes.h               -> PPU VRAM transfer definitions
src/
  player.c            -> Implements XP accumulation, level progression, and choice application
  xp_gem.c            -> Updated to return collected XP drop units on player contact
  game_flow.c         -> Handles GAME_STATE_LEVEL_UP state transitions and prompt/pause logic
  level_up.c          -> [NEW] Implements 3-choice menu navigation and confirmation
  hud.c               -> [NEW] Implements static layout init and dynamic dirty tile buffering
  nmi.s               -> Bounded VBlank VRAM transfer packet handler
  game.c              -> Integrates XP collection, level-up trigger, and render pipelines
  screen.c            -> Renders static HUD layout during gameplay entry
tests/
  test_logic.c        -> Unit tests for XP, level-up, menu navigation, and HUD buffering
```

---

### Task 1: Player Progression State & XP Thresholds

**Files:**
- Modify: `include/tuning.h:5-15`
- Modify: `include/player.h:12-30`
- Modify: `src/player.c:8-30, 47-58`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `tuning.h` constants
- Produces:
  - `void player_add_xp(uint16_t amount);`
  - `uint16_t player_xp(void);`
  - `uint16_t player_next_level_xp(void);`
  - `uint8_t player_level(void);`
  - `uint8_t player_level_up_pending(void);`
  - `void player_apply_level_up(uint8_t choice_index);`
  - `uint8_t player_weapon(uint8_t slot);`
  - `uint8_t player_bonus(uint8_t slot);`

- [ ] **Step 1: Write failing unit tests for player progression**

In `tests/test_logic.c`, add `test_player_xp_and_level_up()`:
```c
static void test_player_xp_and_level_up(void)
{
    player_init();
    CHECK(player_level() == 1U);
    CHECK(player_xp() == 0U);
    CHECK(player_next_level_xp() == 5U);
    CHECK(player_level_up_pending() == 0U);
    CHECK(player_weapon(0U) == 0U); /* Slot 0 = Sword */
    CHECK(player_weapon(1U) == 0xFFU); /* Slot 1 = Empty */

    /* Add XP without reaching threshold */
    player_add_xp(3U);
    CHECK(player_xp() == 3U);
    CHECK(player_level_up_pending() == 0U);

    /* Add XP to reach threshold */
    player_add_xp(2U);
    CHECK(player_xp() == 5U);
    CHECK(player_level_up_pending() == 1U);

    /* Apply level up choice 1 (Max HP + 1) */
    player_apply_level_up(1U);
    CHECK(player_level() == 2U);
    CHECK(player_xp() == 0U);
    CHECK(player_next_level_xp() == 12U);
    CHECK(player_level_up_pending() == 0U);
    CHECK(player_hp() == (uint8_t)(PLAYER_INITIAL_HP + 1U));
}
```
And add `test_player_xp_and_level_up();` in `main()` in `tests/test_logic.c`.

- [ ] **Step 2: Run test to verify it fails to compile**

Run: `make test`
Expected: FAIL with undefined identifiers (`player_xp`, `player_level`, etc.).

- [ ] **Step 3: Implement player progression state in player.h and player.c**

In `include/tuning.h`:
```c
#define LEVEL_UP_MAX_TABLE_LEVEL 10U
#define WEAPON_SLOT_EMPTY        0xFFU
#define BONUS_SLOT_EMPTY         0xFFU
```

In `include/player.h`:
```c
void player_add_xp(uint16_t amount);
uint16_t player_xp(void);
uint16_t player_next_level_xp(void);
uint8_t player_level(void);
uint8_t player_level_up_pending(void);
void player_apply_level_up(uint8_t choice_index);
uint8_t player_weapon(uint8_t slot);
uint8_t player_bonus(uint8_t slot);
uint8_t player_max_hp(void);
```

In `src/player.c`:
Add `xp`, `next_level_xp`, `level`, `level_up_pending`, `max_hp`, `weapons[MAX_EQUIPPED_WEAPONS]`, `bonuses[MAX_EQUIPPED_WEAPONS]` to `PlayerState`.
Initialize `level = 1U`, `xp = 0U`, `next_level_xp = 5U`, `max_hp = PLAYER_INITIAL_HP`, `weapons[0] = 0U`, others `0xFFU`.
Implement threshold lookup table in `src/player.c`:
```c
static const uint16_t level_thresholds[LEVEL_UP_MAX_TABLE_LEVEL - 1U] = {
    5U, 12U, 22U, 35U, 52U, 75U, 105U, 145U, 200U
};
```
Implement `player_add_xp` (saturating at `UINT16_MAX`, checking threshold), `player_apply_level_up` (deducts threshold, increments level, handles choice 0: sword upgrade placeholder, choice 1: `max_hp++` and `hp++`, choice 2: speed upgrade placeholder, updates `next_level_xp`).

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS with 0 errors.

- [ ] **Step 5: Commit**

Run:
```powershell
git add include/tuning.h include/player.h src/player.c tests/test_logic.c; git commit -m "feat(player): add XP accumulation, level progression, and upgrade application"
```

---

### Task 2: XP Gem Collection Integration

**Files:**
- Modify: `include/xp_gem.h:10-15`
- Modify: `src/xp_gem.c:90-120`
- Modify: `src/game.c:100-110`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `xp_gem_update`, `player_add_xp`
- Produces: `uint16_t xp_gem_update(uint8_t player_x, uint8_t player_y)` returning collected units.

- [ ] **Step 1: Write failing unit test for gem collection yielding XP**

In `tests/test_logic.c`, update `test_xp_gem_collection_condensation_and_rendering()`:
```c
/* Verify xp_gem_update returns the collected drop units */
uint16_t collected;
xp_gem_init();
xp_gem_spawn(PLAYER_INITIAL_X, PLAYER_INITIAL_Y);
collected = xp_gem_update(PLAYER_INITIAL_X, PLAYER_INITIAL_Y);
CHECK(collected == 1U);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL due to `void` return type of `xp_gem_update`.

- [ ] **Step 3: Update xp_gem_update to return collected units and call player_add_xp in game.c**

In `include/xp_gem.h`:
```c
uint16_t xp_gem_update(uint8_t player_x, uint8_t player_y);
```

In `src/xp_gem.c`:
Change `void xp_gem_update(...)` to `uint16_t xp_gem_update(...)`.
When gem contact occurs:
```c
uint16_t collected = gem_drop_units[index];
gem_active[index] = 0U;
gem_drop_units[index] = 0U;
/* ... cleanup ... */
return collected;
```

In `src/game.c`:
```c
uint16_t collected_xp = xp_gem_update(player_x(), player_y());
if (collected_xp != 0U) {
    player_add_xp(collected_xp);
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

Run:
```powershell
git add include/xp_gem.h src/xp_gem.c src/game.c tests/test_logic.c; git commit -m "feat(game): integrate XP gem collection with player progression"
```

---

### Task 3: Game State Flow Extension (`GAME_STATE_LEVEL_UP`)

**Files:**
- Modify: `include/game_flow.h:6-20`
- Modify: `src/game_flow.c:20-65`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `input.h`
- Produces:
  - `GAME_STATE_LEVEL_UP` enum value
  - `void game_flow_enter_level_up(void);`
  - `void game_flow_exit_level_up(void);`

- [ ] **Step 1: Write failing unit test for GAME_STATE_LEVEL_UP**

In `tests/test_logic.c`, add to `test_game_flow()`:
```c
game_flow_init();
/* Transition to PLAYING */
game_flow_update(BUTTON_START);
game_flow_update(BUTTON_START);
CHECK(game_flow_state() == GAME_STATE_PLAYING);

/* Enter level up */
game_flow_enter_level_up();
CHECK(game_flow_state() == GAME_STATE_LEVEL_UP);

/* Exit level up */
game_flow_exit_level_up();
CHECK(game_flow_state() == GAME_STATE_PLAYING);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL with undeclared `GAME_STATE_LEVEL_UP` and functions.

- [ ] **Step 3: Implement GAME_STATE_LEVEL_UP in game_flow.h and game_flow.c**

In `include/game_flow.h`:
```c
typedef enum GameState {
    GAME_STATE_PRESENTED_BY = 0,
    GAME_STATE_TITLE,
    GAME_STATE_PLAYING,
    GAME_STATE_LEVEL_UP,
    GAME_STATE_GAME_OVER
} GameState;

void game_flow_enter_level_up(void);
void game_flow_exit_level_up(void);
```

In `src/game_flow.c`:
Implement `game_flow_enter_level_up` (`current_state = GAME_STATE_LEVEL_UP;`) and `game_flow_exit_level_up` (`current_state = GAME_STATE_PLAYING;`).
In `game_flow_update`: add `case GAME_STATE_LEVEL_UP: break;`.

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

Run:
```powershell
git add include/game_flow.h src/game_flow.c tests/test_logic.c; git commit -m "feat(game_flow): add GAME_STATE_LEVEL_UP state and transitions"
```

---

### Task 4: Level-Up Choice Modal Logic

**Files:**
- Create: `include/level_up.h`
- Create: `src/level_up.c`
- Modify: `Makefile:22-25, 80-85`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `input.h`, `LEVEL_UP_CHOICE_COUNT`
- Produces:
  - `void level_up_init(void);`
  - `void level_up_update(uint8_t pressed_buttons);`
  - `uint8_t level_up_cursor(void);`
  - `uint8_t level_up_is_confirmed(void);`

- [ ] **Step 1: Write failing unit test for level-up menu**

In `tests/test_logic.c`, add `test_level_up_menu()`:
```c
static void test_level_up_menu(void)
{
    level_up_init();
    CHECK(level_up_cursor() == 0U);
    CHECK(level_up_is_confirmed() == 0U);

    /* Navigate down */
    level_up_update(BUTTON_DOWN);
    CHECK(level_up_cursor() == 1U);

    level_up_update(BUTTON_DOWN);
    CHECK(level_up_cursor() == 2U);

    /* Wrap or clamp at boundary */
    level_up_update(BUTTON_DOWN);
    CHECK(level_up_cursor() == 0U);

    /* Navigate up */
    level_up_update(BUTTON_UP);
    CHECK(level_up_cursor() == 2U);

    /* Confirm with A */
    level_up_update(BUTTON_A);
    CHECK(level_up_is_confirmed() == 1U);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL with missing `level_up.h`.

- [ ] **Step 3: Implement level_up.h and level_up.c, and add to Makefile**

Create `include/level_up.h`:
```c
#ifndef LEVEL_UP_H
#define LEVEL_UP_H

#include <stdint.h>

void level_up_init(void);
void level_up_update(uint8_t pressed_buttons);
uint8_t level_up_cursor(void);
uint8_t level_up_is_confirmed(void);

#endif
```

Create `src/level_up.c`:
```c
#include "level_up.h"
#include "input.h"
#include "tuning.h"

static uint8_t cursor;
static uint8_t confirmed;

void level_up_init(void)
{
    cursor = 0U;
    confirmed = 0U;
}

void level_up_update(uint8_t pressed_buttons)
{
    if ((pressed_buttons & BUTTON_DOWN) != 0U) {
        cursor = (uint8_t)((cursor + 1U) % LEVEL_UP_CHOICE_COUNT);
    } else if ((pressed_buttons & BUTTON_UP) != 0U) {
        cursor = (uint8_t)((cursor + LEVEL_UP_CHOICE_COUNT - 1U) % LEVEL_UP_CHOICE_COUNT);
    }

    if ((pressed_buttons & (BUTTON_A | BUTTON_START)) != 0U) {
        confirmed = 1U;
    }
}

uint8_t level_up_cursor(void)
{
    return cursor;
}

uint8_t level_up_is_confirmed(void)
{
    return confirmed;
}
```

Update `Makefile` to include `level_up` in `C_MODULES` and `TEST_SOURCES`.

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

Run:
```powershell
git add include/level_up.h src/level_up.c Makefile tests/test_logic.c; git commit -m "feat(level_up): implement level-up menu selection and confirmation"
```

---

### Task 5: Background HUD Buffering & VRAM Update Pipeline

**Files:**
- Create: `include/hud.h`
- Create: `src/hud.c`
- Modify: `src/nmi.s:10-50`
- Modify: `include/nes.h:20-30`
- Modify: `src/screen.c:140-160`
- Modify: `Makefile:22-25, 80-85`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `player.h`, `screen.h`
- Produces:
  - `void hud_init(void);`
  - `void hud_update(void);`
  - `hud_vram_buffer`, `hud_vram_addr`, `hud_vram_len` consumed by NMI during VBlank.

- [ ] **Step 1: Write unit tests for HUD buffer generation**

In `tests/test_logic.c`, add `test_hud_buffering()`:
```c
static void test_hud_buffering(void)
{
    player_init();
    hud_init();
    /* Force update */
    hud_update();
    CHECK(hud_vram_length() > 0U);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL with missing `hud.h`.

- [ ] **Step 3: Implement HUD tile definitions, buffer generator, and NMI VRAM packet**

In `include/hud.h`:
Define tile IDs for HP frame, segmented bars, numbers, and slots.
Declare:
```c
void hud_init(void);
void hud_update(void);
uint8_t hud_vram_length(void);
```

In `src/hud.c`:
Track cached player HP, XP, level, and equipment. When changed, populate `hud_vram_buffer` with target PPUADDR (rows 0 or 1) and replacement tile indices, setting `hud_vram_len`.

In `src/nmi.s`:
Add check for `_hud_vram_len`. If non-zero:
1. Write high byte of `_hud_vram_addr` to `PPUADDR`.
2. Write low byte of `_hud_vram_addr` to `PPUADDR`.
3. Loop copy `_hud_vram_buffer` bytes to `PPUDATA`.
4. Reset `_hud_vram_len` to 0.

In `src/screen.c`:
During `screen_show_gameplay()`: write the initial static HUD background tiles into rows 0 and 1 before enabling rendering.

Update `Makefile` to include `hud` in `C_MODULES` and `TEST_SOURCES`.

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

Run:
```powershell
git add include/hud.h src/hud.c src/nmi.s include/nes.h src/screen.c Makefile tests/test_logic.c; git commit -m "feat(hud): implement background HUD buffering and NMI VRAM transfer"
```

---

### Task 6: Game Loop Integration (Pause, Level-Up Overlay & Resumption)

**Files:**
- Modify: `src/game.c:64-114`
- Modify: `src/screen.c`
- Test: `tests/test_logic.c`

**Interfaces:**
- Consumes: `level_up.h`, `game_flow.h`, `hud.h`, `player.h`
- Produces: Integrated game loop supporting pause, modal display, upgrade application, and seamless resume.

- [ ] **Step 1: Write test for level-up integration flow**

In `tests/test_logic.c`:
Test simulated loop in `test_gameplay_level_up_flow()`:
- Advance XP to trigger `player_level_up_pending()`.
- Check transition to `GAME_STATE_LEVEL_UP`.
- Simulate `level_up_update(BUTTON_A)`.
- Verify transition back to `GAME_STATE_PLAYING` with incremented level.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL.

- [ ] **Step 3: Implement state switching and pause in game.c**

In `src/game.c`:
```c
if (game_flow_state() == GAME_STATE_LEVEL_UP) {
    level_up_update(input_pressed());
    if (level_up_is_confirmed() != 0U) {
        player_apply_level_up(level_up_cursor());
        screen_restore_arena_from_level_up();
        hud_update();
        game_flow_exit_level_up();
    }
    return;
}

if (player_level_up_pending() != 0U) {
    level_up_init();
    screen_draw_level_up_modal();
    game_flow_enter_level_up();
    return;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `make test`
Expected: PASS.

- [ ] **Step 5: Commit**

Run:
```powershell
git add src/game.c src/screen.c tests/test_logic.c; git commit -m "feat(game): integrate level-up pause, modal overlay, and resumption"
```

---

### Task 7: Validation, Memory Budget Check & Documentation

**Files:**
- Create: `docs/changes/en/hud-xp-levelup.md`
- Create: `docs/changes/pt-BR/hud-xp-levelup.md`
- Create: `docs/implementation-notes/hud-xp-levelup.md`

- [ ] **Step 1: Run complete build and test suite**

Run: `make clean; make test`
Expected: 0 errors, ROM validation passed.

- [ ] **Step 2: Run Mesen runtime checks**

Run: `make test-runtime`
Verify state transitions, player movement, and collision scripts pass cleanly.

- [ ] **Step 3: Check memory and ROM budget deltas**

Inspect `build/nes-survivor.map` to verify BSS RAM usage (< 30 bytes added) and PRG-ROM usage.

- [ ] **Step 4: Create branch change logs and Brazilian Portuguese implementation note**

Create:
- `docs/changes/en/hud-xp-levelup.md`
- `docs/changes/pt-BR/hud-xp-levelup.md`
- `docs/implementation-notes/hud-xp-levelup.md` (in Portuguese as required by `AGENTS.md`).

- [ ] **Step 5: Commit documentation and final artifacts**

Run:
```powershell
git add docs/changes/ docs/implementation-notes/; git commit -m "docs: add implementation notes and change logs for HUD and level up"
```
