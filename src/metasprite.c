#include "metasprite.h"

#include "nes.h"

#define OAM_BYTES_PER_SPRITE 4U
#define OAM_Y_OFFSET         0U
#define OAM_TILE_OFFSET      1U
#define OAM_ATTRIBUTE_OFFSET 2U
#define OAM_X_OFFSET         3U
#define OAM_HIDDEN_Y         UINT8_C(0xFF)

void oam_renderer_init(OamRenderer *renderer)
{
    uint8_t sprite_count = NES_OAM_SPRITE_CAPACITY;
    uint8_t offset = 0U;

    renderer->next_sprite = 0U;
    do {
        oam_shadow[offset] = OAM_HIDDEN_Y;
        offset = (uint8_t)(offset + OAM_BYTES_PER_SPRITE);
        --sprite_count;
    } while (sprite_count != 0U);
}

void oam_renderer_begin(OamRenderer *renderer)
{
    uint8_t sprite_count = renderer->next_sprite;
    uint8_t offset = 0U;

    renderer->next_sprite = 0U;
    while (sprite_count != 0U) {
        oam_shadow[offset] = OAM_HIDDEN_Y;
        offset = (uint8_t)(offset + OAM_BYTES_PER_SPRITE);
        --sprite_count;
    }
}

uint8_t oam_renderer_draw_metasprite(OamRenderer *renderer,
                                     int16_t anchor_x,
                                     int16_t anchor_y,
                                     const MetaspriteTile *tiles,
                                     uint8_t tile_count,
                                     uint8_t width_pixels,
                                     uint8_t horizontal_flip)
{
    uint8_t emitted = 0U;

    while (tile_count != 0U &&
           renderer->next_sprite < NES_OAM_SPRITE_CAPACITY) {
        int16_t relative_x = tiles->x;
        int16_t screen_x;
        int16_t screen_y = (int16_t)(anchor_y + tiles->y);
        uint8_t attributes = tiles->attributes;
        uint8_t offset = (uint8_t)(renderer->next_sprite * OAM_BYTES_PER_SPRITE);

        if (horizontal_flip != 0U) {
            relative_x = (int16_t)((int16_t)width_pixels -
                                   (int16_t)NES_SPRITE_WIDTH_PIXELS -
                                   relative_x);
            attributes ^= NES_SPRITE_FLIP_HORIZONTAL;
        }
        screen_x = (int16_t)(anchor_x + relative_x);

        /* OAM stores sprite top minus one; callers keep geometry on screen. */
        oam_shadow[offset + OAM_Y_OFFSET] = (uint8_t)(screen_y - 1);
        oam_shadow[offset + OAM_TILE_OFFSET] = tiles->tile;
        oam_shadow[offset + OAM_ATTRIBUTE_OFFSET] = attributes;
        oam_shadow[offset + OAM_X_OFFSET] = (uint8_t)screen_x;

        ++renderer->next_sprite;
        ++emitted;
        ++tiles;
        --tile_count;
    }

    return emitted;
}
