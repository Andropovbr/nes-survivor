#include "hud.h"

#include <stdint.h>

#include "nes.h"
#include "player.h"

#define DIRTY_HP    UINT8_C(0x01)
#define DIRTY_XP    UINT8_C(0x02)
#define DIRTY_LEVEL UINT8_C(0x04)

static uint8_t dirty_flags;

void hud_init(void)
{
    dirty_flags = 0U;
    vram_buffer_len = 0U;
}

void hud_notify_hp_changed(void)
{
    dirty_flags = (uint8_t)(dirty_flags | DIRTY_HP);
}

void hud_notify_xp_changed(void)
{
    dirty_flags = (uint8_t)(dirty_flags | DIRTY_XP);
}

void hud_notify_level_changed(void)
{
    dirty_flags = (uint8_t)(dirty_flags | DIRTY_LEVEL);
}

void hud_update(void)
{
    uint8_t fill_count;
    uint8_t i;

    if (dirty_flags == 0U || vram_buffer_len != 0U) {
        return;
    }

    if ((dirty_flags & DIRTY_HP) != 0U) {
        uint8_t current_hp = player_hp();
        uint8_t current_max_hp = player_max_hp();
        if (current_max_hp != 0U) {
            fill_count = (uint8_t)(((uint16_t)current_hp * HUD_HP_BAR_TILES) / current_max_hp);
        } else {
            fill_count = 0U;
        }
        if (fill_count > HUD_HP_BAR_TILES) {
            fill_count = HUD_HP_BAR_TILES;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x05U;
        for (i = 0U; i < HUD_HP_BAR_TILES; ++i) {
            vram_buffer_data[i] = (i < fill_count) ? HUD_TILE_BAR_FULL : HUD_TILE_BAR_EMPTY;
        }
        vram_buffer_len = HUD_HP_BAR_TILES;
        dirty_flags = (uint8_t)(dirty_flags & (uint8_t)~DIRTY_HP);
    } else if ((dirty_flags & DIRTY_XP) != 0U) {
        uint16_t current_xp = player_xp();
        uint16_t current_next_level_xp = player_next_level_xp();
        if (current_next_level_xp != 0U) {
            fill_count = (uint8_t)(((uint16_t)current_xp * HUD_XP_BAR_TILES) / current_next_level_xp);
        } else {
            fill_count = 0U;
        }
        if (fill_count > HUD_XP_BAR_TILES) {
            fill_count = HUD_XP_BAR_TILES;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x12U;
        for (i = 0U; i < HUD_XP_BAR_TILES; ++i) {
            vram_buffer_data[i] = (i < fill_count) ? HUD_TILE_BAR_FULL : HUD_TILE_BAR_EMPTY;
        }
        vram_buffer_len = HUD_XP_BAR_TILES;
        dirty_flags = (uint8_t)(dirty_flags & (uint8_t)~DIRTY_XP);
    } else if ((dirty_flags & DIRTY_LEVEL) != 0U) {
        uint8_t current_level = player_level();
        if (current_level > 99U) {
            current_level = 99U;
        }
        vram_buffer_addr_hi = 0x20U;
        vram_buffer_addr_lo = 0x1EU;
        vram_buffer_data[0] = (uint8_t)('0' + (current_level / 10U));
        vram_buffer_data[1] = (uint8_t)('0' + (current_level % 10U));
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
