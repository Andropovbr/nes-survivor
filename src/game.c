#include "game.h"

static GameState current_state;

void game_init(void)
{
    current_state = GAME_STATE_BOOT;
}

void game_update(void)
{
    if (current_state == GAME_STATE_BOOT) {
        current_state = GAME_STATE_RUNNING;
    }
}

GameState game_state(void)
{
    return current_state;
}
