#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/* The serial read order is A, B, Select, Start, Up, Down, Left, Right. */
#define BUTTON_A      UINT8_C(0x80)
#define BUTTON_B      UINT8_C(0x40)
#define BUTTON_SELECT UINT8_C(0x20)
#define BUTTON_START  UINT8_C(0x10)
#define BUTTON_UP     UINT8_C(0x08)
#define BUTTON_DOWN   UINT8_C(0x04)
#define BUTTON_LEFT   UINT8_C(0x02)
#define BUTTON_RIGHT  UINT8_C(0x01)

void input_update(void);
uint8_t input_current(void);
uint8_t input_pressed(void);
uint8_t input_released(void);
uint8_t input_down(uint8_t buttons);

#endif
