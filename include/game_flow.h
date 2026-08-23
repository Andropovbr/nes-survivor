#ifndef GAME_FLOW_H
#define GAME_FLOW_H

#include <stdint.h>

typedef enum GameState {
    GAME_STATE_PRESENTED_BY = 0,
    GAME_STATE_TITLE,
    GAME_STATE_PLAYING
} GameState;

void game_flow_init(void);
void game_flow_update(uint8_t pressed_buttons);
GameState game_flow_state(void);
uint8_t game_flow_title_prompt_visible(void);

#endif
