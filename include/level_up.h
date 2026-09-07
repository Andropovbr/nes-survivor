#ifndef LEVEL_UP_H
#define LEVEL_UP_H

#include <stdint.h>

void level_up_init(void);
void level_up_update(uint8_t pressed_buttons);
uint8_t level_up_cursor(void);
uint8_t level_up_is_confirmed(void);

#endif
