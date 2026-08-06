#include "input.h"
#include "nes.h"

static uint8_t current_buttons;
static uint8_t pressed_buttons;
static uint8_t released_buttons;

static void input_apply_sample(uint8_t sample)
{
    uint8_t previous = current_buttons;

    current_buttons = sample;
    pressed_buttons = (uint8_t)(sample & (uint8_t)~previous);
    released_buttons = (uint8_t)(previous & (uint8_t)~sample);
}

void input_update(void)
{
    input_apply_sample(nes_read_controller());
}

uint8_t input_current(void)
{
    return current_buttons;
}

uint8_t input_pressed(void)
{
    return pressed_buttons;
}

uint8_t input_released(void)
{
    return released_buttons;
}

uint8_t input_down(uint8_t buttons)
{
    return (uint8_t)((current_buttons & buttons) != 0U);
}

#ifdef UNIT_TEST
void input_test_apply(uint8_t sample)
{
    input_apply_sample(sample);
}
#endif
