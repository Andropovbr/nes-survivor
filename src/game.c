#include "game.h"

#include "input.h"
#include "metasprite.h"
#include "player.h"

static GameState current_state;
static OamRenderer oam_renderer;

void game_init(void)
{
    current_state = GAME_STATE_BOOT;
    player_init();
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
}

void game_update(void)
{
    if (current_state == GAME_STATE_BOOT) {
        current_state = GAME_STATE_RUNNING;
    }

    player_update(input_current());
    oam_renderer_begin(&oam_renderer);
    player_render(&oam_renderer);
}

GameState game_state(void)
{
    return current_state;
}
