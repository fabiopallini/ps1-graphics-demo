#ifndef SPRITE_H
#define SPRITE_H

#include "psx.h"

typedef struct sprite
{
	POLY_FT4 poly;
	POLY_F4 poly_rgb;
	SVECTOR vector[4];
	u_short tpage;
	int w, h;
	long posX, posY, posZ, angX, angY, angZ;
	int prevFrame, frame, frameTime, direction, isJumping, hitted, shooting, jump_speed;
	int hp, hp_max;
	
} Sprite;

void sprite_init(Sprite *sprite, int w, int h, unsigned char img[]);
void sprite_init_rgb(Sprite *sprite, int w, int h);
void sprite_setuv(Sprite *sprite, int x, int y, int w, int h);
short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
short sprite_anim_static(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
void sprite_draw(Sprite *sprite);
void sprite_draw_2d(Sprite *sprite);
void sprite_draw_2d_rgb(Sprite *sprite);

#endif
