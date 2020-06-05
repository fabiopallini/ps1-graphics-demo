#include "object.h"

void object_init(OBJECT *obj, int x, unsigned char img[]){
    setVector(&obj->vector[0], -x, -x, 0);
	setVector(&obj->vector[1], x, -x, 0);
	setVector(&obj->vector[2], -x, x, 0);
	setVector(&obj->vector[3], x, x, 0);
	
    SetPolyFT4(&obj->poly);
    setXY4(&obj->poly, 0, 0, x, 0, 0, x, x, x);
    setUV4(&obj->poly, 0, 0, x, 0, 0, x, x, x);
    setRGB0(&obj->poly, 0xff, 0xff, 0xff);
    psLoadTim(&obj->tpage, &obj->clut, img);
}

void object_draw(OBJECT *obj){
    psGte(obj->posX, obj->posY, obj->posZ,
        obj->angX, obj->angY, obj->angZ);
    obj->poly.tpage = obj->tpage;
    obj->poly.clut = obj->clut;
    RotTransPers(&obj->vector[0], (long *)&obj->poly.x0, 0, 0);
    RotTransPers(&obj->vector[1], (long *)&obj->poly.x1, 0, 0);
    RotTransPers(&obj->vector[2], (long *)&obj->poly.x2, 0, 0);
    RotTransPers(&obj->vector[3], (long *)&obj->poly.x3, 0, 0);
    psAddPrimFT4(&obj->poly);
}

void object_drawBackground(OBJECT *obj){
    obj->poly.tpage = obj->tpage;
    obj->poly.clut = obj->clut;
    psAddPrimFT4(&obj->poly);
}
