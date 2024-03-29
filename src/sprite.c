#include "sprite.h"

void sprite_init(Sprite *sprite, int w, int h, u_short tpage){
	sprite->w = w;
	sprite->h = h;
	setVector(&sprite->vector[0], -w, -h, 0);
	setVector(&sprite->vector[1], w, -h, 0);
	setVector(&sprite->vector[2], -w, h, 0);
	setVector(&sprite->vector[3], w, h, 0);
	
	SetPolyFT4(&sprite->poly);
	setXY4(&sprite->poly, 0, 0, w, 0, 0, h, w, h);
	//setUV4(&sprite->poly, 0, 0, w, 0, 0, h, w, h);
	sprite_set_uv(sprite, 0, 0, w, h);
	SetShadeTex(&sprite->poly, 1); // turn shading OFF
	//sprite_setRGB(sprite, 255, 255, 255);
	sprite_setRGB(sprite, 255, 255, 200);
	sprite->tpage = tpage;

	sprite->prevFrame = -1;
	sprite->frameInterval = 5;
	sprite->direction = 1;
}
void sprite_init_rgb(Sprite *sprite, int w, int h){
	sprite->w = w;
	sprite->h = h;
	setVector(&sprite->vector[0], -w, -h, 0);
	setVector(&sprite->vector[1], w, -h, 0);
	setVector(&sprite->vector[2], -w, h, 0);
	setVector(&sprite->vector[3], w, h, 0);

	SetPolyF4(&sprite->poly_rgb);
	setXY4(&sprite->poly_rgb, 0, 0, w, 0, 0, h, w, h);
	setRGB0(&sprite->poly_rgb, 255, 255, 255);
}

void sprite_shading_disable(Sprite *sprite, int disable){
	// disable == 1 to to turn shading OFF
	// disable == 0 to to turn shading ON 
	SetShadeTex(&sprite->poly, disable);
}

void sprite_setRGB(Sprite *sprite, u_char r, u_char g, u_char b){
	sprite->poly.r0 = r;
	sprite->poly.g0 = g;
	sprite->poly.b0 = b;
}

void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h){
	if(sprite->direction == 1){
		setUV4(
			&sprite->poly, 
			x, y, 
			x+w, y, 
			x, y+h, 
			x+w, y+h
		);
	}
	else {
		setUV4(
			&sprite->poly, 
			x+w, y, 
			x, y, 
			x+w, y+h, 
			x, y+h
		);
	}
}

void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b) {
	setRGB0(&sprite->poly_rgb, r, g, b);
}

short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames){
	short result = 1;
	if(sprite->frame < firstFrame || sprite->prevRow != row){
		sprite->prevFrame = -1;
		sprite->prevRow = row;
		sprite->frame = firstFrame;
	}

	sprite->frameTime += 1;
	if(sprite->frameTime >= sprite->frameInterval){
		sprite->prevFrame = sprite->frame;
		sprite->frame += 1;
		if(sprite->frame > (firstFrame+frames)-1){
			sprite->prevFrame = -1;
			sprite->frame = firstFrame;
			sprite->frameInterval = 5;
			result = 0;
		}
		sprite->frameTime = 0;
	}

	if(sprite->frame != sprite->prevFrame)
		sprite_set_uv(sprite, sprite->frame*w, row*h, w, h);

	return result;
}

short sprite_anim_static(Sprite *sprite, short w, short h, short row, short firstFrame, short frames){
	short result = 1;

	sprite->frame = firstFrame;
	sprite_set_uv(sprite, sprite->frame*w, row*h, w, h);

	sprite->frameTime += 1;
	if(sprite->frameTime >= frames*5){
		sprite->frameTime = 0;
		result = 0;
	}
	return result;
}
