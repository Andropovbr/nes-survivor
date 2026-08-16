#include <stdint.h>

#include "enemy.h"
#include "input.h"
#include "metasprite.h"
#include "nes.h"
#include "player.h"
#include "rng.h"
#include "soldier_animation_data.h"
#include "tuning.h"
#include "weapon_sword.h"

#if MAX_EQUIPPED_WEAPONS != 4U
#error "unexpected initial weapon capacity"
#endif

#if LEVEL_UP_CHOICE_COUNT != 3U
#error "unexpected initial choice count"
#endif

#if UINT8_MAX != 255U || UINT16_MAX != 65535U
#error "unexpected fixed-width integer representation"
#endif

static uint8_t failures;

/* Page alignment matters only in the NES linker target, not pure logic tests. */
uint8_t oam_shadow[256];

#define CHECK(condition)       \
    do {                       \
        if (!(condition)) {    \
            ++failures;        \
        }                      \
    } while (0)

/* Test-only entry point compiled by input.c when UNIT_TEST is defined. */
void input_test_apply(uint8_t sample);

static uint8_t active_enemy_count(void)
{
    uint8_t index;
    uint8_t count = 0U;

    for (index = 0U; index < MAX_ACTIVE_ENEMIES; ++index) {
        count = (uint8_t)(count + enemy_is_active(index));
    }
    return count;
}

/* Satisfies input.c without NES hardware in the sim6502 test target. */
uint8_t nes_read_controller(void)
{
    return 0U;
}

static void test_rng(void)
{
    rng_seed(UINT16_C(0xACE1));
    CHECK(rng_next_u16() == UINT16_C(0xD30F));
    CHECK(rng_next_u16() == UINT16_C(0xF1A5));
    CHECK(rng_next_u8() == UINT8_C(0x34));

    rng_seed(UINT16_C(0xACE1));
    CHECK(rng_next_u16() == UINT16_C(0xD30F));

    rng_seed(UINT16_C(0));
    CHECK(rng_next_u16() == UINT16_C(0x8181));
}

static void test_input_edges(void)
{
    input_test_apply(0U);
    input_test_apply((uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_current() == (uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_pressed() == (uint8_t)(BUTTON_A | BUTTON_RIGHT));
    CHECK(input_released() == 0U);
    CHECK(input_down(BUTTON_A) == 1U);
    CHECK(input_down(BUTTON_LEFT) == 0U);

    input_test_apply((uint8_t)(BUTTON_A | BUTTON_LEFT));
    CHECK(input_pressed() == BUTTON_LEFT);
    CHECK(input_released() == BUTTON_RIGHT);

    input_test_apply(0U);
    CHECK(input_pressed() == 0U);
    CHECK(input_released() == (uint8_t)(BUTTON_A | BUTTON_LEFT));
}

static void test_player_direction_and_animation_selection(void)
{
    player_init();
    CHECK(player_x() == PLAYER_INITIAL_X);
    CHECK(player_y() == PLAYER_INITIAL_Y);
    CHECK(player_facing() == PLAYER_FACING_RIGHT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_IDLE);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 12U);

    player_update(BUTTON_RIGHT);
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X + 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);
    CHECK(player_is_moving() == 1U);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 12U);

    player_update(BUTTON_UP);
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);
    CHECK(player_frame_timer() == 11U);

    player_update(BUTTON_LEFT);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 10U);

    player_update(BUTTON_DOWN);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);

    player_update(0U);
    CHECK(player_is_moving() == 0U);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_IDLE);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 12U);
}

static void test_player_diagonal_and_bounds(void)
{
    uint16_t updates;

    player_init();
    player_update((uint8_t)(BUTTON_UP | BUTTON_LEFT));
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X - 1U));
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);

    player_init();
    player_update((uint8_t)(BUTTON_DOWN | BUTTON_RIGHT));
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X + 1U));
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y + 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);

    player_init();
    player_update((uint8_t)(BUTTON_UP | BUTTON_RIGHT));
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X + 1U));
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);

    player_init();
    player_update((uint8_t)(BUTTON_DOWN | BUTTON_LEFT));
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X - 1U));
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y + 1U));
    CHECK(player_facing() == PLAYER_FACING_LEFT);

    for (updates = 0U; updates < 255U; ++updates) {
        player_update((uint8_t)(BUTTON_UP | BUTTON_LEFT));
    }
    CHECK(player_x() == PLAYER_MIN_X);
    CHECK(player_y() == PLAYER_MIN_Y);

    for (updates = 0U; updates < 255U; ++updates) {
        player_update((uint8_t)(BUTTON_DOWN | BUTTON_RIGHT));
    }
    CHECK(player_x() == PLAYER_MAX_X);
    CHECK(player_y() == PLAYER_MAX_Y);

    player_update((uint8_t)(BUTTON_LEFT | BUTTON_RIGHT));
    CHECK(player_x() == PLAYER_MAX_X);
    CHECK(player_is_moving() == 0U);
}

static void test_animation_duration_and_loop(void)
{
    uint16_t updates;

    player_init();
    player_update(BUTTON_RIGHT);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 12U);

    for (updates = 0U; updates < 11U; ++updates) {
        player_update(BUTTON_RIGHT);
        CHECK(player_current_frame() == 0U);
    }
    CHECK(player_frame_timer() == 1U);

    player_update(BUTTON_RIGHT);
    CHECK(player_current_frame() == 1U);
    CHECK(player_frame_timer() == 12U);

    for (updates = 0U; updates < 12U; ++updates) {
        player_update(BUTTON_RIGHT);
    }

    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 12U);
}

static void test_metasprite_rendering_and_idle_flip(void)
{
    OamRenderer renderer;

    player_init();
    player_update(BUTTON_LEFT);
    oam_renderer_init(&renderer);
    player_render(&renderer);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_MOVEMENT);
    CHECK(renderer.next_sprite == 7U);
    CHECK(oam_shadow[0] == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(oam_shadow[1] == 0x00U);
    CHECK(oam_shadow[2] == NES_SPRITE_FLIP_HORIZONTAL);
    CHECK(oam_shadow[3] == (uint8_t)(player_x() + 8U));
    CHECK(oam_shadow[6] == 0x00U);
    CHECK(oam_shadow[7] == player_x());
    player_update(0U);
    oam_renderer_begin(&renderer);
    CHECK(renderer.next_sprite == 0U);
    CHECK(oam_shadow[0] == UINT8_C(0xFF));
    CHECK(oam_shadow[24] == UINT8_C(0xFF));
    player_render(&renderer);
    CHECK(player_current_animation() == SOLDIER_ANIMATION_IDLE);
    CHECK(oam_shadow[2] == NES_SPRITE_FLIP_HORIZONTAL);
    CHECK(oam_shadow[3] == (uint8_t)(player_x() + 8U));
    CHECK(oam_shadow[6] == 0x00U);
    CHECK(oam_shadow[7] == player_x());
}

static void test_automatic_sword_attack_and_rendering(void)
{
    OamRenderer renderer;
    WeaponSwordHitbox hitbox;
    uint8_t updates;

    player_init();
    weapon_sword_init();
    CHECK(weapon_sword_is_attacking() == 0U);
    CHECK(weapon_sword_hitbox(&hitbox, player_x(), player_y(), 0U) == 0U);

    weapon_sword_update();
    CHECK(weapon_sword_is_attacking() == 1U);
    CHECK(weapon_sword_active_frames() == SWORD_ATTACK_ACTIVE_FRAMES);
    CHECK(weapon_sword_frames_until_attack() ==
          (uint8_t)(SWORD_ATTACK_COOLDOWN_FRAMES - 1U));
    CHECK(weapon_sword_hitbox(&hitbox, player_x(), player_y(), 0U) == 1U);
    CHECK(hitbox.x == (int16_t)(PLAYER_INITIAL_X + PLAYER_WIDTH_PIXELS));
    CHECK(hitbox.y ==
          (int16_t)(PLAYER_INITIAL_Y + SWORD_VERTICAL_OFFSET_PIXELS));
    CHECK(hitbox.width == SWORD_WIDTH_PIXELS);
    CHECK(hitbox.height == 16U);

    oam_renderer_init(&renderer);
    player_render(&renderer);
    CHECK(weapon_sword_render(&renderer, player_x(), player_y(), 0U) == 2U);
    CHECK(renderer.next_sprite == 9U);
    CHECK(oam_shadow[28] ==
          (uint8_t)(PLAYER_INITIAL_Y + SWORD_VERTICAL_OFFSET_PIXELS - 1U));
    CHECK(oam_shadow[29] == 0x08U);
    CHECK(oam_shadow[30] == 0x00U);
    CHECK(oam_shadow[31] == (uint8_t)(PLAYER_INITIAL_X + PLAYER_WIDTH_PIXELS));
    CHECK(oam_shadow[33] == 0x09U);

    for (updates = 0U; updates < (SWORD_ATTACK_ACTIVE_FRAMES - 1U);
         ++updates) {
        weapon_sword_update();
    }
    CHECK(weapon_sword_active_frames() == 1U);
    weapon_sword_update();
    CHECK(weapon_sword_is_attacking() == 0U);
    CHECK(weapon_sword_hitbox(&hitbox, player_x(), player_y(), 0U) == 0U);

    for (updates = SWORD_ATTACK_ACTIVE_FRAMES;
         updates < SWORD_ATTACK_COOLDOWN_FRAMES; ++updates) {
        weapon_sword_update();
    }
    CHECK(weapon_sword_is_attacking() == 1U);
    CHECK(weapon_sword_active_frames() == SWORD_ATTACK_ACTIVE_FRAMES);

    player_update(BUTTON_LEFT);
    oam_renderer_begin(&renderer);
    player_render(&renderer);
    CHECK(weapon_sword_render(&renderer, player_x(), player_y(), 1U) == 2U);
    CHECK(oam_shadow[30] == NES_SPRITE_FLIP_HORIZONTAL);
    CHECK(oam_shadow[31] == (uint8_t)(player_x() - SWORD_WIDTH_PIXELS));
}

static void test_enemy_spawn_movement_collision_and_saturation(void)
{
    OamRenderer renderer;
    uint16_t update;
    uint8_t before_x;
    uint8_t before_y;
    uint8_t target_x;
    uint8_t target_y;
    uint8_t movement_remainder;
    uint8_t expected_movement;
    WeaponSwordHitbox hitbox;

    rng_seed(INITIAL_RNG_SEED);
    enemy_init();
    for (update = 0U; update < BAT_INITIAL_SPAWN_DELAY_FRAMES - 1U;
         ++update) {
        enemy_update(120U, 100U);
    }
    CHECK(active_enemy_count() == 0U);
    enemy_update(120U, 100U);
    CHECK(active_enemy_count() == 1U);
    CHECK(enemy_is_active(0U) != 0U);

    oam_renderer_init(&renderer);
    enemy_render(&renderer);
    CHECK(renderer.next_sprite == 2U);
    CHECK(oam_shadow[0] == (uint8_t)(enemy_y(0U) - 1U));
    CHECK(oam_shadow[1] == UINT8_C(0x0A));
    CHECK(oam_shadow[3] == enemy_x(0U));
    CHECK(oam_shadow[5] == UINT8_C(0x0B));
    CHECK(oam_shadow[7] == (uint8_t)(enemy_x(0U) + 8U));

    for (update = 0U; update < BAT_SPAWN_INTERVAL_FRAMES - 1U; ++update) {
        enemy_update(120U, 100U);
    }
    CHECK(active_enemy_count() == 1U);
    enemy_update(120U, 100U);
    CHECK(active_enemy_count() == 2U);

    before_x = enemy_x(0U);
    before_y = enemy_y(0U);
    target_x = before_x < 120U ? BAT_MAX_X : BAT_MIN_X;
    target_y = before_y < 116U ? BAT_MAX_Y : BAT_MIN_Y;
    movement_remainder = (uint8_t)(
        ((BAT_INITIAL_SPAWN_DELAY_FRAMES + BAT_SPAWN_INTERVAL_FRAMES) *
         BAT_MOVEMENT_SPEED_SUBPIXELS) %
        BAT_POSITION_SUBPIXELS_PER_PIXEL);
    expected_movement = (uint8_t)(
        (movement_remainder + 16U * BAT_MOVEMENT_SPEED_SUBPIXELS) /
        BAT_POSITION_SUBPIXELS_PER_PIXEL);
    for (update = 0U; update < 16U; ++update) {
        enemy_update(target_x, target_y);
    }
    CHECK(enemy_x(0U) == (uint8_t)(before_x < target_x
                                        ? before_x + expected_movement
                                        : before_x - expected_movement));
    CHECK(enemy_y(0U) == (uint8_t)(before_y < target_y
                                        ? before_y + expected_movement
                                        : before_y - expected_movement));

    hitbox.x = enemy_x(0U);
    hitbox.y = enemy_y(0U);
    hitbox.width = BAT_WIDTH_PIXELS;
    hitbox.height = BAT_HEIGHT_PIXELS;
    enemy_apply_sword_hitbox(&hitbox);
    CHECK(enemy_is_active(0U) == 0U);
    CHECK(active_enemy_count() == 1U);

    for (update = 0U;
         update < (uint16_t)(BAT_SPAWN_INTERVAL_FRAMES *
                             (MAX_ACTIVE_ENEMIES + 2U));
         ++update) {
        enemy_update(120U, 100U);
    }
    CHECK(active_enemy_count() == MAX_ACTIVE_ENEMIES);
    CHECK(enemy_is_active(MAX_ACTIVE_ENEMIES) == 0U);
    CHECK(enemy_x(MAX_ACTIVE_ENEMIES) == 0U);
    CHECK(enemy_y(MAX_ACTIVE_ENEMIES) == 0U);
}

static void test_sword_screen_edges_and_oam_saturation(void)
{
    OamRenderer renderer;

    weapon_sword_init();
    weapon_sword_update();

    oam_renderer_init(&renderer);
    CHECK(weapon_sword_render(&renderer, 0U, PLAYER_INITIAL_Y, 1U) == 0U);
    CHECK(renderer.next_sprite == 0U);
    CHECK(weapon_sword_render(&renderer, PLAYER_MAX_X, PLAYER_INITIAL_Y, 0U) ==
          0U);
    CHECK(renderer.next_sprite == 0U);

    renderer.next_sprite = (uint8_t)(NES_OAM_SPRITE_CAPACITY - 1U);
    CHECK(weapon_sword_render(&renderer, PLAYER_INITIAL_X, PLAYER_INITIAL_Y,
                              0U) == 1U);
    CHECK(renderer.next_sprite == NES_OAM_SPRITE_CAPACITY);
}

int main(void)
{
    test_rng();
    test_input_edges();
    test_player_direction_and_animation_selection();
    test_player_diagonal_and_bounds();
    test_animation_duration_and_loop();
    test_metasprite_rendering_and_idle_flip();
    test_automatic_sword_attack_and_rendering();
    test_sword_screen_edges_and_oam_saturation();
    test_enemy_spawn_movement_collision_and_saturation();
    return failures;
}
