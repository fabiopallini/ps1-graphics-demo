#ifndef UI_H
#define UI_H

#include "sprite.h"
#include "utils.h"
#include "enemy.h"

enum CMODE {
	CMODE_LEFT = 1,
	CMODE_RIGHT,
	CMODE_FROM_LEFT,
	CMODE_FROM_RIGHT,
	CMODE_BACK,
	
};

typedef struct {
	float value;
	Sprite bar, border; 
} ATB;

ATB atb[2];
Sprite command_bg, selector;
u_char command_mode;
u_char command_index;

void ui_init(u_long *selector_img, int screenW, int screenH);
void ui_update(u_long pad, u_long opad, Sprite *player, Camera *camera);
void ui_enemies_selector(u_long pad, u_long opad, int size, Enemy *enemies);

#endif
