#include "enemy.h"

#include "animation.h"
#include "bat_animation_data.h"
#include "rng.h"
#include "tuning.h"

#define POSITION_FRACTION_BITS 4U

#if BAT_POSITION_SUBPIXELS_PER_PIXEL != (1U << POSITION_FRACTION_BITS)
#error "Bat position scale must remain Q12.4"
#endif

#if BAT_MOVEMENT_SPEED_SUBPIXELS >= \
    (PLAYER_MOVEMENT_SPEED * BAT_POSITION_SUBPIXELS_PER_PIXEL)
#error "Bat must remain slower than the player"
#endif

#if BAT_INITIAL_SPAWN_DELAY_FRAMES == 0U || BAT_SPAWN_INTERVAL_FRAMES == 0U
#error "Bat spawn timers must be nonzero"
#endif

#if 7U + SWORD_ATTACK_SPRITE_COUNT + (MAX_ACTIVE_ENEMIES * 2U) > NES_OAM_SPRITE_CAPACITY
#error "Player, sword and Bat pool exceed OAM capacity"
#endif

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t active;
    uint8_t frame;
    uint8_t frame_timer;
} EnemyState;

static EnemyState enemies[MAX_ACTIVE_ENEMIES];
static uint16_t spawn_timer;
static uint8_t pool_high_water;

static uint16_t pixels_to_position(uint8_t pixels)
{
    return (uint16_t)((uint16_t)pixels << POSITION_FRACTION_BITS);
}

static uint8_t position_to_pixels(uint16_t position)
{
    return (uint8_t)(position >> POSITION_FRACTION_BITS);
}

static uint16_t move_toward(uint16_t position, uint16_t target)
{
    if (position < target) {
        uint16_t distance = (uint16_t)(target - position);
        return distance <= BAT_MOVEMENT_SPEED_SUBPIXELS
                   ? target
                   : (uint16_t)(position + BAT_MOVEMENT_SPEED_SUBPIXELS);
    }
    if (position > target) {
        uint16_t distance = (uint16_t)(position - target);
        return distance <= BAT_MOVEMENT_SPEED_SUBPIXELS
                   ? target
                   : (uint16_t)(position - BAT_MOVEMENT_SPEED_SUBPIXELS);
    }
    return position;
}

static uint8_t scale_random_to_range(uint8_t random, uint8_t range)
{
    return (uint8_t)(((uint16_t)random * (uint16_t)range) >> 8);
}

static uint8_t spawn_bat(void)
{
    uint8_t index;
    uint8_t edge;
    uint8_t coordinate;

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemies[index].active == 0U) {
            break;
        }
    }
    if (index >= MAX_ACTIVE_ENEMIES) {
        return 0U;
    }
    if (index == pool_high_water) {
        ++pool_high_water;
    }

    edge = (uint8_t)(rng_next_u8() & 3U);
    if ((edge & 1U) == 0U) {
        coordinate = scale_random_to_range(
            rng_next_u8(), (uint8_t)(BAT_MAX_Y - BAT_MIN_Y + 1U));
        enemies[index].x = pixels_to_position(
            edge == 0U ? BAT_MIN_X : BAT_MAX_X);
        enemies[index].y = pixels_to_position(
            (uint8_t)(BAT_MIN_Y + coordinate));
    } else {
        coordinate = scale_random_to_range(
            rng_next_u8(), (uint8_t)(BAT_MAX_X - BAT_MIN_X + 1U));
        enemies[index].x = pixels_to_position(
            (uint8_t)(BAT_MIN_X + coordinate));
        enemies[index].y = pixels_to_position(
            edge == 1U ? BAT_MIN_Y : BAT_MAX_Y);
    }
    enemies[index].active = 1U;
    enemies[index].frame = 0U;
    enemies[index].frame_timer = bat_animation_data.frames[0].duration;
    return 1U;
}

void enemy_init(void)
{
    uint8_t index;

    for (index = 0U; index < MAX_ACTIVE_ENEMIES; ++index) {
        enemies[index].active = 0U;
    }
    pool_high_water = 0U;
    spawn_timer = BAT_INITIAL_SPAWN_DELAY_FRAMES;
}

void enemy_update(uint8_t target_x, uint8_t target_y)
{
    uint8_t index;
    uint16_t target_position_x = pixels_to_position(target_x);
    uint16_t target_position_y = pixels_to_position(target_y);

    if (spawn_timer > 1U) {
        --spawn_timer;
    } else if (spawn_bat() != 0U) {
        spawn_timer = BAT_SPAWN_INTERVAL_FRAMES;
    }

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemies[index].active != 0U) {
            enemies[index].x = move_toward(enemies[index].x,
                                           target_position_x);
            enemies[index].y = move_toward(enemies[index].y,
                                           target_position_y);
            if (enemies[index].frame_timer > 1U) {
                --enemies[index].frame_timer;
            } else {
                enemies[index].frame =
                    (uint8_t)(enemies[index].frame ^ 1U);
                enemies[index].frame_timer =
                    bat_animation_data.frames[enemies[index].frame].duration;
            }
        }
    }
}

void enemy_apply_sword_hitbox(const WeaponSwordHitbox *hitbox)
{
    uint8_t index;
    int16_t sword_right = (int16_t)(hitbox->x + hitbox->width);
    int16_t sword_bottom = (int16_t)(hitbox->y + hitbox->height);

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemies[index].active != 0U) {
            int16_t bat_x = position_to_pixels(enemies[index].x);
            int16_t bat_y = position_to_pixels(enemies[index].y);
            int16_t bat_right = (int16_t)(bat_x + BAT_WIDTH_PIXELS);
            int16_t bat_bottom = (int16_t)(bat_y + BAT_HEIGHT_PIXELS);

            if (hitbox->x < bat_right && sword_right > bat_x &&
                hitbox->y < bat_bottom && sword_bottom > bat_y) {
                enemies[index].active = 0U;
            }
        }
    }

    while (pool_high_water != 0U &&
           enemies[pool_high_water - 1U].active == 0U) {
        --pool_high_water;
    }
}

void enemy_render(OamRenderer *renderer)
{
    uint8_t index;

    for (index = 0U; index < pool_high_water; ++index) {
        if (enemies[index].active != 0U) {
            const AnimationFrame *frame =
                &bat_animation_data.frames[enemies[index].frame];
            const MetaspriteTile *tiles =
                &bat_animation_data.sprites[frame->sprite_offset];
            uint8_t x = position_to_pixels(enemies[index].x);
            uint8_t y = position_to_pixels(enemies[index].y);

            (void)oam_renderer_draw_sprite(renderer, x, y,
                                           tiles[0].tile,
                                           tiles[0].attributes);
            (void)oam_renderer_draw_sprite(renderer, (uint8_t)(x + 8U), y,
                                           tiles[1].tile,
                                           tiles[1].attributes);
        }
    }
}

#ifdef UNIT_TEST
uint8_t enemy_is_active(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES ? enemies[index].active : 0U;
}

uint8_t enemy_x(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES
               ? position_to_pixels(enemies[index].x)
               : 0U;
}

uint8_t enemy_y(uint8_t index)
{
    return index < MAX_ACTIVE_ENEMIES
               ? position_to_pixels(enemies[index].y)
               : 0U;
}
#endif
