#include "game.h"

#include "enemy.h"
#include "game_flow.h"
#include "input.h"
#include "metasprite.h"
#include "nes.h"
#include "player.h"
#include "screen.h"
#include "weapon_sword.h"
#include "tuning.h"

static OamRenderer oam_renderer;

static void gameplay_init(void)
{
    player_init();
    weapon_sword_init();
    enemy_init();
    oam_renderer_init(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(),
        (uint8_t)(player_facing() == PLAYER_FACING_LEFT));
}

static void initial_screens_update(void)
{
    GameState previous_state = game_flow_state();
    uint8_t previous_prompt_visible = game_flow_title_prompt_visible();

    game_flow_update(input_pressed());

    if (game_flow_state() != previous_state) {
        if (game_flow_state() == GAME_STATE_TITLE) {
            screen_show_title();
        } else if (game_flow_state() == GAME_STATE_PLAYING) {
            screen_show_gameplay();
            gameplay_init();
        }
    } else if (game_flow_state() == GAME_STATE_TITLE &&
               game_flow_title_prompt_visible() != previous_prompt_visible) {
        screen_set_title_prompt_visible(game_flow_title_prompt_visible());
    }
}

void game_init(void)
{
    game_flow_init();
    oam_renderer_init(&oam_renderer);
    screen_show_presented_by();

    while (game_flow_state() != GAME_STATE_PLAYING) {
        nes_wait_frame();
        input_update();
        initial_screens_update();
    }
}

void game_update(void)
{
    WeaponSwordHitbox sword_hitbox;
    uint8_t facing_left;

    player_update(input_current());
    weapon_sword_update();
    facing_left = (uint8_t)(player_facing() == PLAYER_FACING_LEFT);
    enemy_update((uint8_t)(player_x() +
                           (PLAYER_WIDTH_PIXELS - BAT_WIDTH_PIXELS) / 2U),
                 (uint8_t)(player_y() +
                           (PLAYER_HEIGHT_PIXELS - BAT_HEIGHT_PIXELS) / 2U));
    if (weapon_sword_hitbox(&sword_hitbox, player_x(), player_y(),
                            facing_left) != 0U) {
        enemy_apply_sword_hitbox(&sword_hitbox);
    }
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(), facing_left);
    enemy_render(&oam_renderer);
}

GameState game_state(void)
{
    return game_flow_state();
}
