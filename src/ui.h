#ifndef UI_H
#define UI_H

#include "sprite.h"

Sprite atb_bar, atb_border, command_bg, selector;
float atb_w;
u_char command_mode;
u_char command_index;

void ui_init(u_long *selector_img);
void ui_update(u_long pad);
void ui_draw(char fnt[][FNT_WIDTH], int player_hp);

#endif
