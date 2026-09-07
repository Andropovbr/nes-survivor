#include "hud.h"

#include <stdint.h>

#include "nes.h"
#include "player.h"

#define DIRTY_HP    UINT8_C(0x01)
#define DIRTY_XP    UINT8_C(0x02)
#define DIRTY_LEVEL UINT8_C(0x04)

static uint8_t cached_hp;
static uint8_t cached_max_hp;
static uint16_t cached_xp;
static uint16_t cached_next_level_xp;
static uint8_t cached_level;
static uint8_t dirty_flags;

void hud_init(void)
{
    dirty_flags = 0U;
    cached_hp = player_hp();
    cached_max_hp = player_max_hp();
    cached_xp = player_xp();
    cached_next_level_xp = player_next_level_xp();
    cached_level = player_level();
    vram_buffer_len = 0U;
}

void hud_update(void)
{
    uint8_t current_hp = player_hp();
    uint8_t current_max_hp = player_max_hp();
    uint16_t current_xp = player_xp();
    uint16_t current_next_level_xp = player_next_level_xp();
    uint8_t current_level = player_level();
    uint8_t fill_count;
    uint8_t i;

    if (current_hp != cached_hp || current_max_hp != cached_max_hp) {
        cached_hp = current_hp;
        cached_max_hp = current_max_hp;
        dirty_flags = (uint8_t)(dirty_flags | DIRTY_HP);
    }
    if (current_xp != cached_xp || current_next_level_xp != cached_next_level_xp) {
        cached_xp = current_xp;
        cached_next_level_xp = current_next_level_xp;
        dirty_flags = (uint8_t)(dirty_flags | DIRTY_XP);
    }
    if (current_level != cached_level) {
        cached_level = current_level;
        dirty_flags = (uint8_t)(dirty_flags | DIRTY_LEVEL);
    }

    if (vram_buffer_len != 0U) {
        return;
    }

    if ((dirty_flags & DIRTY_HP) != 0U) {
        if (cached_max_hp != 0U) {
            fill_count = (uint8_t)(((uint16_t)cached_hp * HUD_HP_BAR_TILES) / cached_max_hp);
        } else {
            fill_count = 0U;
        }
        if (fill_count > HUD_HP_BAR_TILES) {
            fill_count = HUD_HP_BAR_TILES;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x05U;
        for (i = 0U; i < HUD_HP_BAR_TILES; ++i) {
            if (i < fill_count) {
                vram_buffer_data[i] = HUD_TILE_BAR_FULL;
            } else {
                vram_buffer_data[i] = HUD_TILE_BAR_EMPTY;
            }
        }
        vram_buffer_len = HUD_HP_BAR_TILES;
        dirty_flags = (uint8_t)(dirty_flags & (uint8_t)~DIRTY_HP);
    } else if ((dirty_flags & DIRTY_XP) != 0U) {
        if (cached_next_level_xp != 0U) {
            fill_count = (uint8_t)(((uint32_t)cached_xp * HUD_XP_BAR_TILES) / cached_next_level_xp);
        } else {
            fill_count = 0U;
        }
        if (fill_count > HUD_XP_BAR_TILES) {
            fill_count = HUD_XP_BAR_TILES;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x12U;
        for (i = 0U; i < HUD_XP_BAR_TILES; ++i) {
            if (i < fill_count) {
                vram_buffer_data[i] = HUD_TILE_BAR_FULL;
            } else {
                vram_buffer_data[i] = HUD_TILE_BAR_EMPTY;
            }
        }
        vram_buffer_len = HUD_XP_BAR_TILES;
        dirty_flags = (uint8_t)(dirty_flags & (uint8_t)~DIRTY_XP);
    } else if ((dirty_flags & DIRTY_LEVEL) != 0U) {
        uint8_t display_level = cached_level;
        if (display_level > 99U) {
            display_level = 99U;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x1EU;
        vram_buffer_data[0] = (uint8_t)('0' + (display_level / 10U));
        vram_buffer_data[1] = (uint8_t)('0' + (display_level % 10U));
        vram_buffer_len = 2U;
        dirty_flags = (uint8_t)(dirty_flags & (uint8_t)~DIRTY_LEVEL);
    }
}

uint8_t hud_vram_length(void)
{
    return vram_buffer_len;
}

#ifdef UNIT_TEST
uint8_t hud_dirty_flags(void)
{
    return dirty_flags;
}
#endif
