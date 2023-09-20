#ifndef SPRITE_H
#define SPRITE_H

#include "psx.h"

typedef struct sprite
{
    POLY_FT4 poly;
    SVECTOR vector[4];
    u_short tpage;
    int w, h;
    long posX, posY, posZ, angX, angY, angZ;
    int prevFrame, frame, frameTime;
} Sprite;

void sprite_init(Sprite *sprite, int w, int h, unsigned char img[]);
void sprite_setuv(Sprite *sprite, int x, int y, int w, int h);
void sprite_anim(Sprite *sprite, short row, int time, short lastFrame, short w, short h);
void sprite_draw(Sprite *sprite);
void sprite_drawOrtho(Sprite *sprite);

#endif
