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

#endif
