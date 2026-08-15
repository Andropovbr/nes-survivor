#ifndef ENEMY_H
#define ENEMY_H

#include <stdint.h>

#include "metasprite.h"
#include "weapon_sword.h"

void enemy_init(void);
void enemy_update(uint8_t target_x, uint8_t target_y);
void enemy_apply_sword_hitbox(const WeaponSwordHitbox *hitbox);
void enemy_render(OamRenderer *renderer);

#ifdef UNIT_TEST
uint8_t enemy_is_active(uint8_t index);
uint8_t enemy_x(uint8_t index);
uint8_t enemy_y(uint8_t index);
#endif

#endif
