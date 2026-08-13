#include "soldier_animation_data.h"

/* Sprite entry: signed x, signed y, CHR tile index, NES OAM attributes. */
static const MetaspriteTile soldier_animation_sprites[] = {
    { 8, 0, 0x00, 0x00 },
    { 16, 0, 0x00, 0x40 },
    { 8, 8, 0x01, 0x00 },
    { 16, 8, 0x02, 0x00 },
    { 0, 16, 0x03, 0x00 },
    { 8, 16, 0x04, 0x00 },
    { 16, 16, 0x05, 0x00 },
    { 8, 0, 0x00, 0x00 },
    { 16, 0, 0x00, 0x40 },
    { 8, 8, 0x01, 0x00 },
    { 16, 8, 0x02, 0x00 },
    { 0, 16, 0x03, 0x00 },
    { 8, 16, 0x06, 0x00 },
    { 16, 16, 0x07, 0x00 },
    { 8, 0, 0x00, 0x00 },
    { 16, 0, 0x00, 0x40 },
    { 8, 8, 0x01, 0x00 },
    { 16, 8, 0x02, 0x00 },
    { 0, 16, 0x03, 0x00 },
    { 8, 16, 0x04, 0x00 },
    { 16, 16, 0x05, 0x00 },
};

/* Frame entry: sprite-array offset, sprite count, duration in game frames. */
static const AnimationFrame soldier_animation_frames[] = {
    { 0, 7, 12 },
    { 7, 7, 12 },
    { 14, 7, 12 },
};

/* Animation entry: frame offset, count, size, playback mode, flip flags. */
static const AnimationDefinition soldier_animations[] = {
    { 0, 1, 3, 3, SOLDIER_ANIMATION_TYPE_IDLE, SOLDIER_GENERATED_H_FLIP },
    { 1, 2, 3, 3, SOLDIER_ANIMATION_TYPE_MOVEMENT,
      SOLDIER_GENERATED_H_FLIP },
};

const AnimationData soldier_animation_data = {
    soldier_animation_sprites,
    soldier_animation_frames,
    soldier_animations,
    SOLDIER_ANIMATION_COUNT,
};
