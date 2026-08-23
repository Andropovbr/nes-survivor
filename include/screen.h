#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

void screen_show_presented_by(void);
void screen_show_title(void);
void screen_set_title_prompt_visible(uint8_t visible);
void screen_show_gameplay(void);
void screen_show_game_over(void);

#endif
