#include "game.h"

#include "enemy.h"
#include "game_flow.h"
#include "hud.h"
#include "input.h"
#include "level_up.h"
#include "metasprite.h"
#include "nes.h"
#include "player.h"
#include "screen.h"
#include "weapon_sword.h"
#include "tuning.h"
#include "xp_gem.h"

static OamRenderer oam_renderer;
static uint8_t collision_phase;

static void gameplay_init(void)
{
    player_init();
    weapon_sword_init();
    enemy_init();
    xp_gem_init();
    hud_init();
    collision_phase = 0U;
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
    uint8_t player_vulnerable;
    uint8_t sword_active;
    uint16_t collected_xp;

    if (game_flow_state() == GAME_STATE_LEVEL_UP) {
        uint8_t prev_cursor = level_up_cursor();
        level_up_update(input_pressed());
        if (level_up_cursor() != prev_cursor) {
            screen_update_level_up_cursor(level_up_cursor());
        }
        if (level_up_is_confirmed() != 0U) {
            player_apply_level_up(level_up_cursor());
            screen_hide_level_up_modal();
            hud_update();
            game_flow_exit_level_up();
        }
        return;
    }

    if (game_flow_state() != GAME_STATE_PLAYING) {
        initial_screens_update();
        return;
    }

    player_vulnerable = player_update(input_current());
    weapon_sword_update();
    facing_left = (uint8_t)(player_facing() == PLAYER_FACING_LEFT);
    enemy_update((uint8_t)(player_x() +
                           (PLAYER_WIDTH_PIXELS - BAT_WIDTH_PIXELS) / 2U),
                 (uint8_t)(player_y() +
                           (PLAYER_HEIGHT_PIXELS - BAT_HEIGHT_PIXELS) / 2U));
    sword_active = weapon_sword_hitbox(
        &sword_hitbox, player_x(), player_y(), facing_left);
    if (player_vulnerable != 0U &&
        (sword_active == 0U || collision_phase == 0U) &&
        enemy_overlaps_player(player_hitbox_x(), player_hitbox_y()) != 0U &&
        player_take_contact_damage() != 0U) {
        nes_play_player_hit_sfx();
        if (player_hp() == 0U) {
            game_flow_enter_game_over();
            screen_show_game_over();
            return;
        }
    }
    if (sword_active != 0U &&
        (player_vulnerable == 0U || collision_phase != 0U)) {
        enemy_apply_sword_hitbox(&sword_hitbox);
    }
    collision_phase ^= 1U;
    collected_xp = xp_gem_update(player_x(), player_y());
    if (collected_xp != 0U) {
        player_add_xp(collected_xp);
    }
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(), facing_left);
    enemy_render(&oam_renderer);
    xp_gem_render(&oam_renderer);
    hud_update();
    if (player_level_up_pending() != 0U) {
        level_up_init();
        screen_show_level_up_modal(level_up_cursor());
        game_flow_enter_level_up();
        return;
    }
}

GameState game_state(void)
{
    return game_flow_state();
}
