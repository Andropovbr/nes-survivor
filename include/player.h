#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>

#include "metasprite.h"

typedef enum PlayerFacing {
    PLAYER_FACING_RIGHT = 0,
    PLAYER_FACING_LEFT
} PlayerFacing;

void player_init(void);
uint8_t player_update(uint8_t buttons);
void player_render(OamRenderer *renderer);
uint8_t player_take_contact_damage(void);

uint8_t player_x(void);
uint8_t player_y(void);
uint8_t player_hitbox_x(void);
uint8_t player_hitbox_y(void);
uint8_t player_hp(void);
uint8_t player_hit_cooldown(void);
PlayerFacing player_facing(void);
uint8_t player_is_moving(void);
uint8_t player_current_animation(void);
uint8_t player_current_frame(void);
uint8_t player_frame_timer(void);

void player_add_xp(uint16_t amount);
uint16_t player_xp(void);
uint16_t player_next_level_xp(void);
uint8_t player_level(void);
uint8_t player_level_up_pending(void);
void player_apply_level_up(uint8_t choice_index);
uint8_t player_weapon(uint8_t slot);
uint8_t player_bonus(uint8_t slot);
uint8_t player_max_hp(void);

#endif
