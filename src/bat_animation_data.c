#include "bat_animation_data.h"

/* Palette 1 preserves Soldier and sword palette 0 while using bat.pal. */
static const MetaspriteTile bat_animation_sprites[] = {
    { 0, 0, 0x0A, 0x01 },
    { 8, 0, 0x0B, 0x01 },
    { 0, 0, 0x0C, 0x01 },
    { 8, 0, 0x0D, 0x01 },
};

static const AnimationFrame bat_animation_frames[] = {
    { 0, 2, 16 },
    { 2, 2, 16 },
};

static const AnimationDefinition bat_animations[] = {
    { 0, 2, 2, 1, 0, 0x40 },
};

const AnimationData bat_animation_data = {
    bat_animation_sprites,
    bat_animation_frames,
    bat_animations,
    BAT_ANIMATION_COUNT,
};
