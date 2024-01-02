#ifndef SPRITE_H
#define SPRITE_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <stdio.h>
#include <libspu.h>
#include <libds.h>
#include <strings.h>
#include <libmath.h>
#include <libapi.h>

typedef struct 
{
	POLY_FT4 poly;
	POLY_F4 poly_rgb;
	SVECTOR vector[4];
	u_short tpage;
	int w, h;
	long posX, posY, posZ, angX, angY, angZ;
	int prevFrame, frame, frameTime, jump_speed;
	u_char direction, isJumping, hitted, shooting;
	int hittable;
	int hp, hp_max;
	
} Sprite;

void sprite_init(Sprite *sprite, int w, int h, u_long *img);
void sprite_init_rgb(Sprite *sprite, int w, int h);
void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h);
void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b);
short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
short sprite_anim_static(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);

#endif
