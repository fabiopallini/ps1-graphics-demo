#include "object.h"

void object_init(OBJECT *obj, int w, int h, unsigned char img[]){
	obj->w = w;
	obj->h = h;
	setVector(&obj->vector[0], -w, -h, 0);
	setVector(&obj->vector[1], w, -h, 0);
	setVector(&obj->vector[2], -w, h, 0);
	setVector(&obj->vector[3], w, h, 0);
	
	SetPolyFT4(&obj->poly);
	setXY4(&obj->poly, 0, 0, w, 0, 0, h, w, h);
	setUV4(&obj->poly, 0, 0, w, 0, 0, h, w, h);
	SetShadeTex(&obj->poly, 1);
	//setRGB0(&obj->poly, 255, 0, 0);
	psLoadTim(&obj->tpage, img);
}

void object_setuv(OBJECT *obj, int x, int y, int w, int h){
	setUV4(&obj->poly, x, y, x+w, y, x, y+h, x+w, y+h);
}

void object_draw(OBJECT *obj){
	psGte(obj->posX, obj->posY, obj->posZ,
	obj->angX, obj->angY, obj->angZ);
	obj->poly.tpage = obj->tpage;
	RotTransPers(&obj->vector[0], (long *)&obj->poly.x0, 0, 0);
	RotTransPers(&obj->vector[1], (long *)&obj->poly.x1, 0, 0);
	RotTransPers(&obj->vector[2], (long *)&obj->poly.x2, 0, 0);
	RotTransPers(&obj->vector[3], (long *)&obj->poly.x3, 0, 0);
	psAddPrimFT4(&obj->poly);
}

void object_move2d(OBJECT *obj, long x, long y){
	obj->poly.x0 = x;
	obj->poly.y0 = y;
	obj->poly.x1 = x + obj->w;
	obj->poly.y1 = y;
	obj->poly.x2 = x;
	obj->poly.y2 = y + obj->h;
	obj->poly.x3 = x + obj->w;
	obj->poly.y3 = y + obj->h;
}

void object_draw2d(OBJECT *obj){
	object_move2d(obj, obj->posX, obj->posY);
	obj->poly.tpage = obj->tpage;
	psAddPrimFT4(&obj->poly);
}
