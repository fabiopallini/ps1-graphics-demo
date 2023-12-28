#ifndef UI_H
#define UI_H

#include "sprite.h"

typedef struct {
	Sprite atb_bar, atb_border; 
	float atb_w;

} CommandPlayer;

CommandPlayer cp[2];
Sprite command_bg, selector;
u_char command_mode;
u_char command_index;

void ui_init(u_long *selector_img);
void ui_update(u_long pad, u_long opad);
void ui_draw(char fnt[][FNT_WIDTH], int player_hp, int player2_hp);

#endif
