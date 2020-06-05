#include "psx.h"

typedef struct object
{
    POLY_FT4 poly;
    SVECTOR vector[4];
    u_short tpage, clut;
    int w, h;
    long posX, posY, posZ, angX, angY, angZ;
} OBJECT;

void object_init(OBJECT *obj, int w, int h, unsigned char img[]);
void object_setuv(OBJECT *obj, int x, int y, int w, int h);
void object_draw(OBJECT *obj);
void object_draw2d(OBJECT *obj);
