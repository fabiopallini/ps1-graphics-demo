#ifndef UI_H
#define UI_H

#include "sprite.h"
#include "enemy.h"

typedef struct {
	float value;
	Sprite bar, border; 
} ATB;

ATB atb[2];
Sprite command_bg, selector;
u_char command_mode;
u_char command_index;

void ui_init(u_long *selector_img);
void ui_update(u_long pad, u_long opad, Sprite *player);
void ui_enemies_selector(u_long pad, u_long opad, int size, Enemy *enemies);
void ui_draw(char fnt[][FNT_WIDTH], int player_hp, int player2_hp);

#endif
