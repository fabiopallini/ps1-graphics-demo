#include "psx.h"

typedef struct object
{
    POLY_FT4 poly;
    SVECTOR vector[4];
    u_short tpage, clut;
    long posX, posY, posZ, angX, angY, angZ;
} OBJECT;

void object_init(OBJECT *obj, int x, unsigned char img[]);
void object_draw(OBJECT *obj);
void object_drawBackground(OBJECT *obj);
