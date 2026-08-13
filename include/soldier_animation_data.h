#ifndef SOLDIER_ANIMATION_DATA_H
#define SOLDIER_ANIMATION_DATA_H

#include <stdint.h>

#include "animation.h"

#define SOLDIER_ANIMATION_IDLE     0U
#define SOLDIER_ANIMATION_MOVEMENT 1U
#define SOLDIER_ANIMATION_COUNT    2U

#define SOLDIER_ANIMATION_TYPE_IDLE     UINT8_C(0)
#define SOLDIER_ANIMATION_TYPE_MOVEMENT UINT8_C(1)
#define SOLDIER_GENERATED_H_FLIP        UINT8_C(0x40)

extern const AnimationData soldier_animation_data;

#endif
