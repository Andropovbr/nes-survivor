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

#endif
