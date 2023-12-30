#include "sprite.h"

static void billboard(Sprite *sprite) {
    // Calcola la direzione dalla sprite alla camera
    float dirX = camera.x - sprite->posX;
    //float dirY = camera.y - sprite->posY;
    float dirZ = camera.z - sprite->posZ;

    // Applica la rotazione della telecamera
    // (modifica della direzione in base alla rotazione della telecamera)
    float cosRY = cos(camera.ry * (PI / 180.0));
    float sinRY = sin(camera.ry * (PI / 180.0));
    float tempX = dirX * cosRY + dirZ * sinRY;
    float tempZ = -dirX * sinRY + dirZ * cosRY;

    // Applica la rotazione della telecamera anche lungo l'asse X (opzionale)
    /*float cosRX = cos(camera.rx * (PI / 180.0));
    float sinRX = sin(camera.rx * (PI / 180.0));
    float tempY = dirY * cosRX - tempZ * sinRX;*/

    // Calcola l'angolo di rotazione attorno all'asse Y
    sprite->angY = atan2(tempX, tempZ) * (180.0 / PI);

    // Calcola l'angolo di rotazione attorno all'asse X
    //sprite->angX = atan2(tempY, sqrt(tempX * tempX + tempZ * tempZ)) * (180.0 / PI);

    // Applica la rotazione della sprite rispetto alla telecamera
    sprite->angY -= camera.ry;
    //sprite->angX -= camera.rx;
    //sprite->angZ = 0.0;
}

void sprite_init(Sprite *sprite, int w, int h, u_long *img){
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
	psLoadTim(&sprite->tpage, (u_char*)img);
	sprite->prevFrame = -1;
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

void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h){
	setUV4(&sprite->poly, x, y, x+w, y, x, y+h, x+w, y+h);
}

void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b) {
	setRGB0(&sprite->poly_rgb, r, g, b);
}

short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames){
	short result = 1;
	if(sprite->frame < firstFrame){
		sprite->prevFrame = -1;
		sprite->frame = firstFrame;
	}

	sprite->frameTime += 1;
	if(sprite->frameTime >= 5){
		if(sprite->frame != sprite->prevFrame){
			sprite->prevFrame = sprite->frame;
			sprite->frame += 1;
			if(sprite->frame > (firstFrame+frames)-1){
				sprite->prevFrame = -1;
				sprite->frame = firstFrame;
				result = 0;
			}
			sprite_set_uv(sprite, sprite->frame*w, row*h, w, h);
			sprite->frameTime = 0;
		}
	}
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

void sprite_billboard(Sprite *sprite){
	billboard(sprite);
}

void sprite_draw(Sprite *sprite){
	long otz;
	if(frame == 60)
		billboard(sprite);
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
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

void sprite_draw_2d(Sprite *sprite){
	sprite_moveOrtho(sprite, sprite->posX, sprite->posY);
	sprite->poly.tpage = sprite->tpage;
	psAddPrimFT4(&sprite->poly);
}

void sprite_draw_2d_rgb(Sprite *sprite){
	long x = sprite->posX;
	long y = sprite->posY;
	sprite->poly_rgb.x0 = x;
	sprite->poly_rgb.y0 = y;
	sprite->poly_rgb.x1 = x + sprite->w;
	sprite->poly_rgb.y1 = y;
	sprite->poly_rgb.x2 = x;
	sprite->poly_rgb.y2 = y + sprite->h;
	sprite->poly_rgb.x3 = x + sprite->w;
	sprite->poly_rgb.y3 = y + sprite->h;
	psAddPrimF4(&sprite->poly_rgb);
}

