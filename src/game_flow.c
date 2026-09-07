#include "game_flow.h"

#include "input.h"
#include "tuning.h"

static GameState current_state;
static uint8_t state_timer;
static uint8_t title_prompt_visible;

static void enter_title(void)
{
    current_state = GAME_STATE_TITLE;
    state_timer = TITLE_BLINK_HALF_PERIOD_FRAMES;
    title_prompt_visible = 1U;
}

void game_flow_init(void)
{
    current_state = GAME_STATE_PRESENTED_BY;
    state_timer = PRESENTED_BY_DURATION_FRAMES;
    title_prompt_visible = 0U;
}

void game_flow_update(uint8_t pressed_buttons)
{
    switch (current_state) {
    case GAME_STATE_PRESENTED_BY:
        if ((pressed_buttons & BUTTON_START) != 0U) {
            enter_title();
        } else if (state_timer > 1U) {
            --state_timer;
        } else {
            enter_title();
        }
        break;

    case GAME_STATE_TITLE:
        if ((pressed_buttons & BUTTON_START) != 0U) {
            current_state = GAME_STATE_PLAYING;
            title_prompt_visible = 0U;
        } else if (state_timer > 1U) {
            --state_timer;
        } else {
            title_prompt_visible = (uint8_t)(title_prompt_visible == 0U);
            state_timer = TITLE_BLINK_HALF_PERIOD_FRAMES;
        }
        break;

    case GAME_STATE_PLAYING:
        break;

    case GAME_STATE_LEVEL_UP:
        break;

    case GAME_STATE_GAME_OVER:
        if ((pressed_buttons & BUTTON_START) != 0U) {
            enter_title();
        }
        break;
    }
}

void game_flow_enter_level_up(void)
{
    current_state = GAME_STATE_LEVEL_UP;
}

void game_flow_exit_level_up(void)
{
    current_state = GAME_STATE_PLAYING;
}

void game_flow_enter_game_over(void)
{
    current_state = GAME_STATE_GAME_OVER;
    state_timer = 0U;
    title_prompt_visible = 0U;
}

GameState game_flow_state(void)
{
    return current_state;
}

uint8_t game_flow_title_prompt_visible(void)
{
    return title_prompt_visible;
}
