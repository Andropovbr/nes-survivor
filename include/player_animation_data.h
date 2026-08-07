#ifndef PLAYER_ANIMATION_DATA_H
#define PLAYER_ANIMATION_DATA_H

#include <stdint.h>

#include "animation.h"

#define PLAYER_ANIMATION_IDLE           0U
#define PLAYER_ANIMATION_MOVEMENT_RIGHT 1U
#define PLAYER_ANIMATION_MOVEMENT_LEFT  2U
#define PLAYER_ANIMATION_COUNT           3U

#define PLAYER_ANIMATION_TYPE_IDLE     UINT8_C(0)
#define PLAYER_ANIMATION_TYPE_MOVEMENT UINT8_C(1)
#define PLAYER_DIRECTION_NONE          UINT8_C(0x00)
#define PLAYER_DIRECTION_LEFT          UINT8_C(0x01)
#define PLAYER_DIRECTION_RIGHT         UINT8_C(0x02)
#define PLAYER_DIRECTION_MASK          UINT8_C(0x03)
#define PLAYER_GENERATED_H_FLIP        UINT8_C(0x80)

extern const AnimationData player_animation_data;

#endif
