#include "game.h"

#include "input.h"
#include "metasprite.h"
#include "player.h"
#include "weapon_sword.h"

static GameState current_state;
static OamRenderer oam_renderer;

void game_init(void)
{
    current_state = GAME_STATE_BOOT;
    player_init();
    weapon_sword_init();
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(),
        (uint8_t)(player_facing() == PLAYER_FACING_LEFT));
}

void game_update(void)
{
    if (current_state == GAME_STATE_BOOT) {
        current_state = GAME_STATE_RUNNING;
    }

    player_update(input_current());
    weapon_sword_update();
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
    (void)weapon_sword_render(
        &oam_renderer, player_x(), player_y(),
        (uint8_t)(player_facing() == PLAYER_FACING_LEFT));
}

GameState game_state(void)
{
    return current_state;
}
