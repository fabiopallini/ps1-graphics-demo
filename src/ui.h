#ifndef UI_H
#define UI_H

#include "sprite.h"

Sprite atb_bar, atb_border, command_bg;
float atb_w;

void ui_init();
void ui_update();
void ui_draw(u_char rpgAttack, char fnt[][FNT_WIDTH], int player_hp);

#endif
