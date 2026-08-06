#ifndef GAME_H
#define GAME_H

typedef enum GameState {
    GAME_STATE_BOOT = 0,
    GAME_STATE_RUNNING
} GameState;

void game_init(void);
void game_update(void);
GameState game_state(void);

#endif
