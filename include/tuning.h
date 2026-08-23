#ifndef TUNING_H
#define TUNING_H

#include <stdint.h>

/* Reserved gameplay/content capacities; active pool limits live with systems. */
#define MAX_EQUIPPED_WEAPONS    4U
#define LEVEL_UP_CHOICE_COUNT   3U
#define MAX_PLAYABLE_CHARACTERS 8U
#define MAX_WEAPON_TYPES        16U
#define MAX_ENEMY_TYPES         16U
#define MAX_STAGE_COUNT         8U

#define TARGET_FRAME_RATE_NTSC  60U
#define INITIAL_RNG_SEED        UINT16_C(0xACE1)

/* Initial screens use deterministic NTSC frame counts. */
#define PRESENTED_BY_DURATION_FRAMES   150U /* 2.5 seconds at 60 Hz */
#define TITLE_BLINK_HALF_PERIOD_FRAMES  30U /* 0.5 seconds per phase */

/* Player position is the top-left of a logical 24x24-pixel metasprite. */
#define PLAYER_WIDTH_PIXELS    24U
#define PLAYER_HEIGHT_PIXELS   24U
/* Damage uses the lower body, excluding the rear scabbard/empty back area. */
#define PLAYER_HITBOX_WIDTH_PIXELS          16U
#define PLAYER_HITBOX_HEIGHT_PIXELS         16U
#define PLAYER_HITBOX_Y_OFFSET_PIXELS        8U
#define PLAYER_HITBOX_RIGHT_X_OFFSET_PIXELS  8U
#define PLAYER_HITBOX_LEFT_X_OFFSET_PIXELS   0U
#define PLAYER_INITIAL_X      116U
#define PLAYER_INITIAL_Y      108U
#define PLAYER_MOVEMENT_SPEED   1U /* pixels per axis per game frame */
#ifndef PLAYER_INITIAL_HP
#define PLAYER_INITIAL_HP        5U /* contact damage removes one HP */
#endif
#define PLAYER_HIT_COOLDOWN_FRAMES 30U /* invulnerability after contact */
#define PLAYER_MIN_X            0U
#define PLAYER_MAX_X          232U
/* NES OAM Y stores screen Y minus one, so logical screen row zero is avoided. */
#define PLAYER_MIN_Y            1U
#define PLAYER_MAX_Y          216U

/* Automatic sword: attack starts once per second on the 60 Hz NTSC target. */
#define SWORD_ATTACK_COOLDOWN_FRAMES 60U
#define SWORD_ATTACK_ACTIVE_FRAMES   12U
#define SWORD_ATTACK_SPRITE_COUNT     2U
#define SWORD_WIDTH_PIXELS             8U
#define SWORD_VERTICAL_OFFSET_PIXELS   4U

/* Bat speed uses Q4 subpixels; one shared accumulator emits pixel steps. */
#define BAT_POSITION_SUBPIXELS_PER_PIXEL 16U
#define MAX_ACTIVE_ENEMIES             12U /* fixed runtime pool capacity */
#define BAT_WIDTH_PIXELS                16U
#define BAT_HEIGHT_PIXELS                8U
#define BAT_MOVEMENT_SPEED_SUBPIXELS    6U /* 0.375 pixels per axis/frame */
/* Nearby Bat top-left positions use this cheap rectangular separation range. */
#define BAT_SEPARATION_X_PIXELS         12U
#define BAT_SEPARATION_Y_PIXELS          6U
#define BAT_INITIAL_SPAWN_DELAY_FRAMES 120U /* 2 seconds at 60 Hz */
#define BAT_SPAWN_INTERVAL_FRAMES      120U /* 2 seconds at 60 Hz */
/* Bat position is its top-left corner; bounds keep the full 16x8 sprite visible. */
#define BAT_MIN_X                        0U
#define BAT_MAX_X                      240U
#define BAT_MIN_Y                        1U
#define BAT_MAX_Y                      232U

/* XP drops use one 8x8 sprite each; pickup currently grants no progression. */
#define MAX_ACTIVE_XP_GEMS               8U /* fixed runtime pool capacity */
#define XP_GEM_WIDTH_PIXELS               8U
#define XP_GEM_HEIGHT_PIXELS              8U

#endif
