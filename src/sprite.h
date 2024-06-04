#ifndef SPRITE_H
#define SPRITE_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>

typedef struct 
{
	POLY_FT4 ft4;
	POLY_F4 f4;
	SVECTOR vector[4];
	u_short tpage;
	int w, h;
	VECTOR pos; 
	SVECTOR rot; 
	char prevFrame, prevRow, frame, frameTime, jump_speed, frameInterval;
	u_char direction, isJumping, hitted, shooting;
	int hittable;
	int hp, hp_max;
	
} Sprite;

void sprite_init(Sprite *sprite, int w, int h, u_short tpage);
void sprite_shading_disable(Sprite *sprite, int disable);
void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h);
void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b);
short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
short sprite_anim_static(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
int sprite_collision(Sprite *s1, Sprite *s2);
int sprite_collision2(Sprite *s1, Sprite *s2);
int balloon_collision(Sprite *s1, Sprite *s2);

#endif
