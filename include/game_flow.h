#ifndef GAME_FLOW_H
#define GAME_FLOW_H

#include <stdint.h>

typedef enum GameState {
    GAME_STATE_PRESENTED_BY = 0,
    GAME_STATE_TITLE,
    GAME_STATE_PLAYING,
    GAME_STATE_LEVEL_UP,
    GAME_STATE_GAME_OVER
} GameState;

void game_flow_init(void);
void game_flow_update(uint8_t pressed_buttons);
void game_flow_enter_game_over(void);
void game_flow_enter_level_up(void);
void game_flow_exit_level_up(void);
GameState game_flow_state(void);
uint8_t game_flow_title_prompt_visible(void);

#endif
