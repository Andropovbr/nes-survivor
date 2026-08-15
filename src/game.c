#include "game.h"

#include "enemy.h"
#include "input.h"
#include "metasprite.h"
#include "player.h"
#include "weapon_sword.h"
#include "tuning.h"

static GameState current_state;
static OamRenderer oam_renderer;

void game_init(void)
{
    current_state = GAME_STATE_BOOT;
    player_init();
    weapon_sword_init();
    enemy_init();
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(),
        (uint8_t)(player_facing() == PLAYER_FACING_LEFT));
}

void game_update(void)
{
    WeaponSwordHitbox sword_hitbox;
    uint8_t facing_left;

    if (current_state == GAME_STATE_BOOT) {
        current_state = GAME_STATE_RUNNING;
    }

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
    return current_state;
}
