#include <stdint.h>

#include "input.h"
#include "metasprite.h"
#include "nes.h"
#include "player.h"
#include "player_animation_data.h"
#include "rng.h"
#include "tuning.h"

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
    CHECK(player_current_animation() == PLAYER_ANIMATION_IDLE);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 6U);

    player_update(BUTTON_RIGHT);
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X + 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);
    CHECK(player_is_moving() == 1U);
    CHECK(player_current_animation() == PLAYER_ANIMATION_MOVEMENT_RIGHT);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 6U);

    player_update(BUTTON_UP);
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(player_facing() == PLAYER_FACING_RIGHT);
    CHECK(player_current_animation() == PLAYER_ANIMATION_MOVEMENT_RIGHT);
    CHECK(player_frame_timer() == 5U);

    player_update(BUTTON_LEFT);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == PLAYER_ANIMATION_MOVEMENT_LEFT);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 6U);

    player_update(BUTTON_DOWN);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == PLAYER_ANIMATION_MOVEMENT_LEFT);

    player_update(0U);
    CHECK(player_is_moving() == 0U);
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == PLAYER_ANIMATION_IDLE);
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 6U);
}

static void test_player_diagonal_and_bounds(void)
{
    uint16_t updates;

    player_init();
    player_update((uint8_t)(BUTTON_UP | BUTTON_LEFT));
    CHECK(player_x() == (uint8_t)(PLAYER_INITIAL_X - 1U));
    CHECK(player_y() == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(player_facing() == PLAYER_FACING_LEFT);
    CHECK(player_current_animation() == PLAYER_ANIMATION_MOVEMENT_LEFT);

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
    for (updates = 0U; updates < 5U; ++updates) {
        player_update(0U);
        CHECK(player_current_frame() == 0U);
    }
    player_update(0U);
    CHECK(player_current_frame() == 1U);
    CHECK(player_frame_timer() == 6U);

    for (updates = 0U; updates < 30U; ++updates) {
        player_update(0U);
    }
    CHECK(player_current_frame() == 0U);
    CHECK(player_frame_timer() == 6U);
}

static void test_metasprite_rendering_and_idle_flip(void)
{
    OamRenderer renderer;

    player_init();
    oam_renderer_begin(&renderer);
    player_render(&renderer);
    CHECK(renderer.next_sprite == 7U);
    CHECK(oam_shadow[0] == (uint8_t)(PLAYER_INITIAL_Y - 1U));
    CHECK(oam_shadow[1] == 0x00U);
    CHECK(oam_shadow[2] == 0x00U);
    CHECK(oam_shadow[3] == (uint8_t)(PLAYER_INITIAL_X + 8U));
    CHECK(oam_shadow[7] == (uint8_t)(PLAYER_INITIAL_X + 16U));
    CHECK(oam_shadow[6] == NES_SPRITE_FLIP_HORIZONTAL);
    CHECK(oam_shadow[7U * 4U] == 0xFFU);

    player_update(BUTTON_LEFT);
    player_update(0U);
    oam_renderer_begin(&renderer);
    player_render(&renderer);
    CHECK(player_current_animation() == PLAYER_ANIMATION_IDLE);
    CHECK(oam_shadow[2] == NES_SPRITE_FLIP_HORIZONTAL);
    CHECK(oam_shadow[3] == (uint8_t)(player_x() + 8U));
    CHECK(oam_shadow[6] == 0x00U);
    CHECK(oam_shadow[7] == player_x());
}

int main(void)
{
    test_rng();
    test_input_edges();
    test_player_direction_and_animation_selection();
    test_player_diagonal_and_bounds();
    test_animation_duration_and_loop();
    test_metasprite_rendering_and_idle_flip();
    return failures;
}
