#ifndef HUD_H
#define HUD_H

#include <stdint.h>

#define HUD_TILE_HEART      0x10U
#define HUD_TILE_SLOT_EMPTY 0x11U
#define HUD_TILE_SWORD      0x12U
#define HUD_TILE_BAR_EMPTY  0x13U
#define HUD_TILE_BAR_FULL   0x14U

#define HUD_HP_BAR_TILES    6U
#define HUD_XP_BAR_TILES    10U

void hud_init(void);
void hud_update(void);
uint8_t hud_vram_length(void);

#ifdef UNIT_TEST
uint8_t hud_dirty_flags(void);
#endif

#endif
