#include "enemy.h"

#include "animation.h"
#include "bat_animation_data.h"
#include "nes.h"
#include "rng.h"
#include "tuning.h"

#define MOVEMENT_FRACTION_BITS 4U
#define OAM_BYTES_PER_SPRITE   4U

#if BAT_POSITION_SUBPIXELS_PER_PIXEL != (1U << MOVEMENT_FRACTION_BITS)
#error "Bat movement accumulator scale must remain Q4"
#endif

#if BAT_MOVEMENT_SPEED_SUBPIXELS >= BAT_POSITION_SUBPIXELS_PER_PIXEL
#error "Bat speed must remain below one pixel per gameplay update"
#endif

#if BAT_MOVEMENT_SPEED_SUBPIXELS >= \
    (PLAYER_MOVEMENT_SPEED * BAT_POSITION_SUBPIXELS_PER_PIXEL)
#error "Bat must remain slower than the player"
#endif

#if BAT_SEPARATION_X_PIXELS == 0U || BAT_SEPARATION_Y_PIXELS == 0U
#error "Bat separation ranges must be nonzero"
#endif

#if BAT_INITIAL_SPAWN_DELAY_FRAMES == 0U || BAT_SPAWN_INTERVAL_FRAMES == 0U
#error "Bat spawn timers must be nonzero"
#endif

#if 7U + SWORD_ATTACK_SPRITE_COUNT + (MAX_ACTIVE_ENEMIES * 2U) > NES_OAM_SPRITE_CAPACITY
#error "Player, sword and Bat pool exceed OAM capacity"
#endif

static uint8_t enemy_x_positions[MAX_ACTIVE_ENEMIES];
static uint8_t enemy_y_positions[MAX_ACTIVE_ENEMIES];
static uint8_t enemy_active[MAX_ACTIVE_ENEMIES];
static uint8_t enemy_facing_right[MAX_ACTIVE_ENEMIES];
static uint16_t spawn_timer;
static uint8_t pool_high_water;
static uint8_t movement_subpixels;
static uint8_t shared_animation_frame;
static uint8_t shared_animation_timer;
static uint8_t separation_pair_index;
static uint8_t separation_pair_other;
static uint8_t pending_separation;
static uint8_t pending_separation_on_x;
static uint8_t pending_separation_index;
static uint8_t pending_separation_other;
static uint8_t pending_separation_index_position;
static uint8_t pending_separation_other_position;

static uint8_t scale_random_to_range(uint8_t random, uint8_t range)
{
    return (uint8_t)(((uint16_t)random * (uint16_t)range) >> 8);
}

static uint8_t spawn_bat(uint8_t target_x)
{
    uint8_t index;
    uint8_t edge;
    uint8_t coordinate;
    uint8_t pool_was_empty = (uint8_t)(pool_high_water == 0U);

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemy_active[index] == 0U) {
            break;
        }
    }
    if (index >= MAX_ACTIVE_ENEMIES) {
        return 0U;
    }
    if (index == pool_high_water) {
        ++pool_high_water;
    }
    if (pending_separation != 0U &&
        (pending_separation_index == index ||
         pending_separation_other == index)) {
        pending_separation = 0U;
    }

    edge = (uint8_t)(rng_next_u8() & 3U);
    if ((edge & 1U) == 0U) {
        coordinate = scale_random_to_range(
            rng_next_u8(), (uint8_t)(BAT_MAX_Y - BAT_MIN_Y + 1U));
        enemy_x_positions[index] = edge == 0U ? BAT_MIN_X : BAT_MAX_X;
        enemy_y_positions[index] = (uint8_t)(BAT_MIN_Y + coordinate);
    } else {
        coordinate = scale_random_to_range(
            rng_next_u8(), (uint8_t)(BAT_MAX_X - BAT_MIN_X + 1U));
        enemy_x_positions[index] = (uint8_t)(BAT_MIN_X + coordinate);
        enemy_y_positions[index] = edge == 1U ? BAT_MIN_Y : BAT_MAX_Y;
    }
    enemy_facing_right[index] =
        (uint8_t)(enemy_x_positions[index] < target_x);
    enemy_active[index] = 1U;
    if (pool_was_empty != 0U) {
        shared_animation_frame = 0U;
        shared_animation_timer = bat_animation_data.frames[0].duration;
    }
    return 1U;
}

void enemy_init(void)
{
    uint8_t index;

    for (index = 0U; index < MAX_ACTIVE_ENEMIES; ++index) {
        enemy_active[index] = 0U;
    }
    pool_high_water = 0U;
    movement_subpixels = 0U;
    shared_animation_frame = 0U;
    shared_animation_timer = bat_animation_data.frames[0].duration;
    separation_pair_index = 0U;
    separation_pair_other = 1U;
    pending_separation = 0U;
    spawn_timer = BAT_INITIAL_SPAWN_DELAY_FRAMES;
}

void enemy_update(uint8_t target_x, uint8_t target_y)
{
    uint8_t index;
    uint8_t other;
    uint8_t move_positions = 0U;

    if (spawn_timer > 1U) {
        --spawn_timer;
    } else if (spawn_bat(target_x) != 0U) {
        spawn_timer = BAT_SPAWN_INTERVAL_FRAMES;
    }

    movement_subpixels =
        (uint8_t)(movement_subpixels + BAT_MOVEMENT_SPEED_SUBPIXELS);
    if (movement_subpixels >= BAT_POSITION_SUBPIXELS_PER_PIXEL) {
        movement_subpixels =
            (uint8_t)(movement_subpixels - BAT_POSITION_SUBPIXELS_PER_PIXEL);
        move_positions = 1U;
    }

    if (pool_high_water != 0U) {
        if (shared_animation_timer > 1U) {
            --shared_animation_timer;
        } else {
            shared_animation_frame = (uint8_t)(shared_animation_frame ^ 1U);
            shared_animation_timer =
                bat_animation_data.frames[shared_animation_frame].duration;
        }
    }

    if (move_positions == 0U) {
        /* Scan one rotating pair while positions are idle, then apply its
         * cached one-axis result on the next Q4 movement step. */
        if (pool_high_water > 1U) {
            uint8_t delta_x;
            uint8_t delta_y;

            if (separation_pair_other >= pool_high_water) {
                ++separation_pair_index;
                separation_pair_other =
                    (uint8_t)(separation_pair_index + 1U);
                if (separation_pair_other >= pool_high_water) {
                    separation_pair_index = 0U;
                    separation_pair_other = 1U;
                }
            }
            index = separation_pair_index;
            other = separation_pair_other;
            ++separation_pair_other;
            if (enemy_active[index] != 0U && enemy_active[other] != 0U) {
                delta_x = enemy_x_positions[index] < enemy_x_positions[other]
                              ? (uint8_t)(enemy_x_positions[other] -
                                          enemy_x_positions[index])
                              : (uint8_t)(enemy_x_positions[index] -
                                          enemy_x_positions[other]);
                delta_y = enemy_y_positions[index] < enemy_y_positions[other]
                              ? (uint8_t)(enemy_y_positions[other] -
                                          enemy_y_positions[index])
                              : (uint8_t)(enemy_y_positions[index] -
                                          enemy_y_positions[other]);
                if (delta_x < BAT_SEPARATION_X_PIXELS &&
                    delta_y < BAT_SEPARATION_Y_PIXELS) {
                    pending_separation = 1U;
                    pending_separation_index = index;
                    pending_separation_other = other;
                    pending_separation_on_x = (uint8_t)(
                        (delta_x == 0U && delta_y == 0U) ||
                        delta_x >= delta_y);
                }
            }
            if (pending_separation != 0U &&
                pending_separation_index == index &&
                pending_separation_other == other) {
                if (pending_separation_on_x != 0U) {
                    if (enemy_x_positions[index] <=
                        enemy_x_positions[other]) {
                        pending_separation_index_position =
                            enemy_x_positions[index] > BAT_MIN_X
                                ? (uint8_t)(enemy_x_positions[index] - 1U)
                                : BAT_MIN_X;
                        pending_separation_other_position =
                            enemy_x_positions[other] < BAT_MAX_X
                                ? (uint8_t)(enemy_x_positions[other] + 1U)
                                : BAT_MAX_X;
                    } else {
                        pending_separation_index_position =
                            enemy_x_positions[index] < BAT_MAX_X
                                ? (uint8_t)(enemy_x_positions[index] + 1U)
                                : BAT_MAX_X;
                        pending_separation_other_position =
                            enemy_x_positions[other] > BAT_MIN_X
                                ? (uint8_t)(enemy_x_positions[other] - 1U)
                                : BAT_MIN_X;
                    }
                } else if (enemy_y_positions[index] <
                           enemy_y_positions[other]) {
                    pending_separation_index_position =
                        enemy_y_positions[index] > BAT_MIN_Y
                            ? (uint8_t)(enemy_y_positions[index] - 1U)
                            : BAT_MIN_Y;
                    pending_separation_other_position =
                        enemy_y_positions[other] < BAT_MAX_Y
                            ? (uint8_t)(enemy_y_positions[other] + 1U)
                            : BAT_MAX_Y;
                } else {
                    pending_separation_index_position =
                        enemy_y_positions[index] < BAT_MAX_Y
                            ? (uint8_t)(enemy_y_positions[index] + 1U)
                            : BAT_MAX_Y;
                    pending_separation_other_position =
                        enemy_y_positions[other] > BAT_MIN_Y
                            ? (uint8_t)(enemy_y_positions[other] - 1U)
                            : BAT_MIN_Y;
                }
            }
        }
        return;
    }

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemy_active[index] != 0U) {
            if (enemy_x_positions[index] < target_x) {
                ++enemy_x_positions[index];
                enemy_facing_right[index] = 1U;
            } else if (enemy_x_positions[index] > target_x) {
                --enemy_x_positions[index];
                enemy_facing_right[index] = 0U;
            }
            if (enemy_y_positions[index] < target_y) {
                ++enemy_y_positions[index];
            } else if (enemy_y_positions[index] > target_y) {
                --enemy_y_positions[index];
            }
        }
    }

    if (pending_separation != 0U &&
        enemy_active[pending_separation_index] != 0U &&
        enemy_active[pending_separation_other] != 0U) {
        if (pending_separation_on_x != 0U) {
            if (pending_separation_index_position <
                enemy_x_positions[pending_separation_index]) {
                enemy_facing_right[pending_separation_index] = 0U;
            } else if (pending_separation_index_position >
                       enemy_x_positions[pending_separation_index]) {
                enemy_facing_right[pending_separation_index] = 1U;
            }
            if (pending_separation_other_position <
                enemy_x_positions[pending_separation_other]) {
                enemy_facing_right[pending_separation_other] = 0U;
            } else if (pending_separation_other_position >
                       enemy_x_positions[pending_separation_other]) {
                enemy_facing_right[pending_separation_other] = 1U;
            }
            enemy_x_positions[pending_separation_index] =
                pending_separation_index_position;
            enemy_x_positions[pending_separation_other] =
                pending_separation_other_position;
        } else {
            enemy_y_positions[pending_separation_index] =
                pending_separation_index_position;
            enemy_y_positions[pending_separation_other] =
                pending_separation_other_position;
        }
    }
    pending_separation = 0U;
}

void enemy_apply_sword_hitbox(const WeaponSwordHitbox *hitbox)
{
    uint8_t index;
    uint8_t sword_x = (uint8_t)hitbox->x;
    uint8_t sword_y = (uint8_t)hitbox->y;
    uint8_t sword_width = hitbox->width;
    uint8_t sword_height = hitbox->height;

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemy_active[index] != 0U) {
            uint8_t bat_x = enemy_x_positions[index];
            uint8_t bat_y = enemy_y_positions[index];

            if ((uint8_t)(bat_x - sword_x) < sword_width ||
                (uint8_t)(sword_x - bat_x) < BAT_WIDTH_PIXELS) {
                if ((uint8_t)(bat_y - sword_y) < sword_height ||
                    (uint8_t)(sword_y - bat_y) < BAT_HEIGHT_PIXELS) {
                    enemy_active[index] = 0U;
                    if (pending_separation != 0U &&
                        (pending_separation_index == index ||
                         pending_separation_other == index)) {
                        pending_separation = 0U;
                    }
                }
            }
        }
    }

    while (pool_high_water != 0U &&
           enemy_active[pool_high_water - 1U] == 0U) {
        --pool_high_water;
    }
}

void enemy_render(OamRenderer *renderer)
{
    uint8_t index;
    uint8_t offset = (uint8_t)(renderer->next_sprite * OAM_BYTES_PER_SPRITE);
    const AnimationFrame *frame =
        &bat_animation_data.frames[shared_animation_frame];
    const MetaspriteTile *tiles =
        &bat_animation_data.sprites[frame->sprite_offset];

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemy_active[index] != 0U) {
            uint8_t x = enemy_x_positions[index];
            uint8_t y = enemy_y_positions[index];

            if (renderer->next_sprite > NES_OAM_SPRITE_CAPACITY - 2U) {
                return;
            }

            /* Mirroring swaps the two tiles as well as flipping each tile. */
            oam_shadow[offset] = (uint8_t)(y - 1U);
            oam_shadow[offset + 3U] = x;
            oam_shadow[offset + 4U] = (uint8_t)(y - 1U);
            oam_shadow[offset + 7U] =
                (uint8_t)(x + NES_SPRITE_WIDTH_PIXELS);
            if (enemy_facing_right[index] != 0U) {
                oam_shadow[offset + 1U] = tiles[0].tile;
                oam_shadow[offset + 2U] = tiles[0].attributes;
                oam_shadow[offset + 5U] = tiles[1].tile;
                oam_shadow[offset + 6U] = tiles[1].attributes;
            } else {
                oam_shadow[offset + 1U] = tiles[1].tile;
                oam_shadow[offset + 2U] = (uint8_t)(
                    tiles[1].attributes ^ NES_SPRITE_FLIP_HORIZONTAL);
                oam_shadow[offset + 5U] = tiles[0].tile;
                oam_shadow[offset + 6U] = (uint8_t)(
                    tiles[0].attributes ^ NES_SPRITE_FLIP_HORIZONTAL);
            }
            offset = (uint8_t)(offset + (2U * OAM_BYTES_PER_SPRITE));
            renderer->next_sprite = (uint8_t)(renderer->next_sprite + 2U);
        }
    }
}

#ifdef UNIT_TEST
uint8_t enemy_is_active(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES ? enemy_active[index] : 0U;
}

uint8_t enemy_x(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES ? enemy_x_positions[index] : 0U;
}

uint8_t enemy_y(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES ? enemy_y_positions[index] : 0U;
}

uint8_t enemy_is_facing_right(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES ? enemy_facing_right[index] : 0U;
}

void enemy_test_set(uint8_t index, uint8_t active, uint8_t x, uint8_t y)
{
    if (index >= MAX_ACTIVE_ENEMIES) {
        return;
    }
    enemy_x_positions[index] = x;
    enemy_y_positions[index] = y;
    enemy_active[index] = (uint8_t)(active != 0U);
    enemy_facing_right[index] = 0U;
    pending_separation = 0U;
    if (enemy_active[index] != 0U && index >= pool_high_water) {
        pool_high_water = (uint8_t)(index + 1U);
    }
    while (pool_high_water != 0U &&
           enemy_active[pool_high_water - 1U] == 0U) {
        --pool_high_water;
    }
}
#endif
