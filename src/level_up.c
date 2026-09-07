#include "level_up.h"

#include <stdint.h>

#include "input.h"
#include "tuning.h"

static uint8_t cursor;
static uint8_t confirmed;

void level_up_init(void)
{
    cursor = 0U;
    confirmed = 0U;
}

void level_up_update(uint8_t pressed_buttons)
{
    if ((pressed_buttons & BUTTON_DOWN) != 0U) {
        cursor = (uint8_t)((cursor + 1U) % LEVEL_UP_CHOICE_COUNT);
    } else if ((pressed_buttons & BUTTON_UP) != 0U) {
        cursor = (uint8_t)((cursor + LEVEL_UP_CHOICE_COUNT - 1U) % LEVEL_UP_CHOICE_COUNT);
    }

    if ((pressed_buttons & (BUTTON_A | BUTTON_START)) != 0U) {
        confirmed = 1U;
    }
}

uint8_t level_up_cursor(void)
{
    return cursor;
}

uint8_t level_up_is_confirmed(void)
{
    return confirmed;
}
