#include "sprite.h"

void sprite_init(Sprite *sprite, int w, int h, unsigned char img[]){
	sprite->w = w;
	sprite->h = h;
	setVector(&sprite->vector[0], -w, -h, 0);
	setVector(&sprite->vector[1], w, -h, 0);
	setVector(&sprite->vector[2], -w, h, 0);
	setVector(&sprite->vector[3], w, h, 0);
	
	SetPolyFT4(&sprite->poly);
	setXY4(&sprite->poly, 0, 0, w, 0, 0, h, w, h);
	setUV4(&sprite->poly, 0, 0, w, 0, 0, h, w, h);
	SetShadeTex(&sprite->poly, 1);
	//setRGB0(&sprite->poly, 255, 0, 0);
	psLoadTim(&sprite->tpage, img);
	sprite->prevFrame = -1;
}

void sprite_setuv(Sprite *sprite, int x, int y, int w, int h){
	setUV4(&sprite->poly, x, y, x+w, y, x, y+h, x+w, y+h);
}

void sprite_anim(Sprite *sprite, short row, int time, short lastFrame, short w, short h){
	sprite->frameTime += 1;
	if(sprite->frameTime >= time){
		if(sprite->frame != sprite->prevFrame){
			sprite->prevFrame = sprite->frame;
			sprite->frame += 1;
			if(sprite->frame > lastFrame-1)
				sprite->frame = 0;
			sprite_setuv(sprite, sprite->frame*w, row*h, w, h);
			sprite->frameTime = 0;
		}
	}
}

void sprite_draw(Sprite *sprite){
	long otz;
	psGte(sprite->posX, sprite->posY, sprite->posZ,
	sprite->angX, sprite->angY, sprite->angZ);
	sprite->poly.tpage = sprite->tpage;
	RotTransPers(&sprite->vector[0], (long *)&sprite->poly.x0, 0, 0);
	RotTransPers(&sprite->vector[1], (long *)&sprite->poly.x1, 0, 0);
	RotTransPers(&sprite->vector[2], (long *)&sprite->poly.x2, 0, 0);
	otz = RotTransPers(&sprite->vector[3], (long *)&sprite->poly.x3, 0, 0);
	psAddPrimFT4otz(&sprite->poly, otz);
}

void sprite_moveOrtho(Sprite *sprite, long x, long y){
	sprite->poly.x0 = x;
	sprite->poly.y0 = y;
	sprite->poly.x1 = x + sprite->w;
	sprite->poly.y1 = y;
	sprite->poly.x2 = x;
	sprite->poly.y2 = y + sprite->h;
	sprite->poly.x3 = x + sprite->w;
	sprite->poly.y3 = y + sprite->h;
}

void sprite_drawOrtho(Sprite *sprite){
	sprite_moveOrtho(sprite, sprite->posX, sprite->posY);
	sprite->poly.tpage = sprite->tpage;
	psAddPrimFT4(&sprite->poly);
}
