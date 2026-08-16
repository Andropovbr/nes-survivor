#ifndef METASPRITE_H
#define METASPRITE_H

#include <stdint.h>

#include "animation.h"

#define NES_SPRITE_FLIP_HORIZONTAL UINT8_C(0x40)
#define NES_SPRITE_FLIP_VERTICAL   UINT8_C(0x80)
#define NES_OAM_SPRITE_CAPACITY    64U
#define NES_SPRITE_WIDTH_PIXELS     8U

typedef struct {
    uint8_t next_sprite;
} OamRenderer;

void oam_renderer_init(OamRenderer *renderer);
void oam_renderer_begin(OamRenderer *renderer);
uint8_t oam_renderer_draw_metasprite(OamRenderer *renderer,
                                     int16_t anchor_x,
                                     int16_t anchor_y,
                                     const MetaspriteTile *tiles,
                                     uint8_t tile_count,
                                     uint8_t width_pixels,
                                     uint8_t horizontal_flip);

#endif
