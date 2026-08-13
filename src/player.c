#include "player.h"

#include "animation.h"
#include "input.h"
#include "soldier_animation_data.h"
#include "tuning.h"

typedef struct {
    uint8_t x;
    uint8_t y;
    PlayerFacing facing;
    uint8_t moving;
    AnimationPlayer animation;
} PlayerState;

static PlayerState player;

static uint8_t selected_animation(void)
{
    if (player.moving == 0U) {
        return SOLDIER_ANIMATION_IDLE;
    }
    return SOLDIER_ANIMATION_MOVEMENT;
}

void player_init(void)
{
    player.x = PLAYER_INITIAL_X;
    player.y = PLAYER_INITIAL_Y;
    player.facing = PLAYER_FACING_RIGHT;
    player.moving = 0U;
    animation_player_init(&player.animation, &soldier_animation_data,
                          SOLDIER_ANIMATION_IDLE);
}

void player_update(uint8_t buttons)
{
    uint8_t move_left = (uint8_t)((buttons & BUTTON_LEFT) != 0U);
    uint8_t move_right = (uint8_t)((buttons & BUTTON_RIGHT) != 0U);
    uint8_t move_up = (uint8_t)((buttons & BUTTON_UP) != 0U);
    uint8_t move_down = (uint8_t)((buttons & BUTTON_DOWN) != 0U);
    uint8_t changed_animation;

    if (move_left != 0U && move_right == 0U) {
        player.facing = PLAYER_FACING_LEFT;
        if (player.x > PLAYER_MIN_X) {
            if ((uint8_t)(player.x - PLAYER_MIN_X) <= PLAYER_MOVEMENT_SPEED) {
                player.x = PLAYER_MIN_X;
            } else {
                player.x = (uint8_t)(player.x - PLAYER_MOVEMENT_SPEED);
            }
        }
    } else if (move_right != 0U && move_left == 0U) {
        player.facing = PLAYER_FACING_RIGHT;
        if (player.x < PLAYER_MAX_X) {
            if ((uint8_t)(PLAYER_MAX_X - player.x) <= PLAYER_MOVEMENT_SPEED) {
                player.x = PLAYER_MAX_X;
            } else {
                player.x = (uint8_t)(player.x + PLAYER_MOVEMENT_SPEED);
            }
        }
    }

    if (move_up != 0U && move_down == 0U) {
        if (player.y > PLAYER_MIN_Y) {
            if ((uint8_t)(player.y - PLAYER_MIN_Y) <= PLAYER_MOVEMENT_SPEED) {
                player.y = PLAYER_MIN_Y;
            } else {
                player.y = (uint8_t)(player.y - PLAYER_MOVEMENT_SPEED);
            }
        }
    } else if (move_down != 0U && move_up == 0U) {
        if (player.y < PLAYER_MAX_Y) {
            if ((uint8_t)(PLAYER_MAX_Y - player.y) <= PLAYER_MOVEMENT_SPEED) {
                player.y = PLAYER_MAX_Y;
            } else {
                player.y = (uint8_t)(player.y + PLAYER_MOVEMENT_SPEED);
            }
        }
    }

    player.moving = (uint8_t)(((move_left ^ move_right) |
                               (move_up ^ move_down)) != 0U);
    changed_animation = animation_player_select(
        &player.animation, &soldier_animation_data, selected_animation());
    if (changed_animation == 0U) {
        animation_player_update(&player.animation, &soldier_animation_data);
    }
}

void player_render(OamRenderer *renderer)
{
    const AnimationDefinition *animation = animation_player_definition(
        &player.animation, &soldier_animation_data);
    const AnimationFrame *frame = animation_player_frame(
        &player.animation, &soldier_animation_data);
    int16_t anchor_x = player.x;
    uint8_t horizontal_flip =
        (uint8_t)(player.facing == PLAYER_FACING_LEFT);

    (void)oam_renderer_draw_metasprite(
        renderer, anchor_x, player.y,
        &soldier_animation_data.sprites[frame->sprite_offset],
        frame->sprite_count,
        (uint8_t)(animation->width_tiles * NES_SPRITE_WIDTH_PIXELS),
        horizontal_flip);
}

uint8_t player_x(void) { return player.x; }
uint8_t player_y(void) { return player.y; }
PlayerFacing player_facing(void) { return player.facing; }
uint8_t player_is_moving(void) { return player.moving; }
uint8_t player_current_animation(void) { return player.animation.animation; }
uint8_t player_current_frame(void) { return player.animation.frame; }
uint8_t player_frame_timer(void) { return player.animation.frame_timer; }
