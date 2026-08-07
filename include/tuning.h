#ifndef TUNING_H
#define TUNING_H

#include <stdint.h>

/* Architectural capacities only; no corresponding gameplay exists yet. */
#define MAX_EQUIPPED_WEAPONS    4U
#define LEVEL_UP_CHOICE_COUNT   3U
#define MAX_PLAYABLE_CHARACTERS 8U
#define MAX_WEAPON_TYPES        16U
#define MAX_ENEMY_TYPES         16U
#define MAX_STAGE_COUNT         8U

#define TARGET_FRAME_RATE_NTSC  60U
#define INITIAL_RNG_SEED        UINT16_C(0xACE1)

/* Player position is the top-left of a logical 24x24-pixel metasprite. */
#define PLAYER_WIDTH_PIXELS    24U
#define PLAYER_HEIGHT_PIXELS   24U
#define PLAYER_INITIAL_X      116U
#define PLAYER_INITIAL_Y      108U
#define PLAYER_MOVEMENT_SPEED   1U /* pixels per axis per game frame */
#define PLAYER_MIN_X            0U
#define PLAYER_MAX_X          232U
/* NES OAM Y stores screen Y minus one, so logical screen row zero is avoided. */
#define PLAYER_MIN_Y            1U
#define PLAYER_MAX_Y          216U

#endif
