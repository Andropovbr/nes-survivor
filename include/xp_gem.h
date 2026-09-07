#ifndef XP_GEM_H
#define XP_GEM_H

#include <stdint.h>

#include "metasprite.h"

void xp_gem_init(void);
void xp_gem_spawn(uint8_t x, uint8_t y);
uint16_t xp_gem_update(uint8_t player_x, uint8_t player_y);
void xp_gem_render(OamRenderer *renderer);

#ifdef UNIT_TEST
uint8_t xp_gem_active_count(void);
uint8_t xp_gem_is_active(uint8_t index);
uint8_t xp_gem_x(uint8_t index);
uint8_t xp_gem_y(uint8_t index);
uint16_t xp_gem_drop_units(uint8_t index);
#endif

#endif
