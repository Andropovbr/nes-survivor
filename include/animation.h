#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdint.h>

typedef struct {
    int8_t x;
    int8_t y;
    uint8_t tile;
    uint8_t attributes;
} MetaspriteTile;

typedef struct {
    uint16_t sprite_offset;
    uint8_t sprite_count;
    uint8_t duration;
} AnimationFrame;

typedef struct {
    uint16_t frame_offset;
    uint8_t frame_count;
    uint8_t width_tiles;
    uint8_t height_tiles;
    uint8_t type;
    uint8_t direction_flags;
} AnimationDefinition;

typedef struct {
    const MetaspriteTile *sprites;
    const AnimationFrame *frames;
    const AnimationDefinition *animations;
    uint8_t animation_count;
} AnimationData;

typedef struct {
    uint8_t animation;
    uint8_t frame;
    uint8_t frame_timer;
} AnimationPlayer;

void animation_player_init(AnimationPlayer *player,
                           const AnimationData *data,
                           uint8_t animation);
uint8_t animation_player_select(AnimationPlayer *player,
                                const AnimationData *data,
                                uint8_t animation);
void animation_player_update(AnimationPlayer *player,
                             const AnimationData *data);
const AnimationDefinition *animation_player_definition(
    const AnimationPlayer *player, const AnimationData *data);
const AnimationFrame *animation_player_frame(const AnimationPlayer *player,
                                             const AnimationData *data);

#endif
