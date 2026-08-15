#ifndef WEAPON_SWORD_H
#define WEAPON_SWORD_H

#include <stdint.h>

#include "metasprite.h"

void weapon_sword_init(void);
void weapon_sword_update(void);
uint8_t weapon_sword_render(OamRenderer *renderer,
                            uint8_t player_x,
                            uint8_t player_y,
                            uint8_t facing_left);

uint8_t weapon_sword_is_attacking(void);
uint8_t weapon_sword_active_frames(void);
uint8_t weapon_sword_frames_until_attack(void);

#endif
