#include "psx.h"
#include "images.h"

POLY_FT4 poly[1];
SVECTOR vec[4];
int size = 200;
long z = 1024;
short y = 0;
u_short tpage, clut;

POLY_F4 pf4;
SVECTOR v[4];
short yy = 0;

int main() {
	psxGfxSetup(160, 120, 512);

	setVector(&vec[0], -128, -128, 0);
	setVector(&vec[1], 128, -128, 0);
	setVector(&vec[2], -128, 128, 0);
	setVector(&vec[3], 128, 128, 0);

	setVector(&v[0], -50, -50, 0);
	setVector(&v[1], 50, -50, 0);
	setVector(&v[2], -50, 50, 0);
	setVector(&v[3], 50, 50, 0);

	SetPolyFT4(&poly[0]);
	//setUVWH(&poly[0], 0, 0, 255, 255);
	setXY4(&poly[0], 0, 0, size, 0, 0, size, size, size);
	setUV4(&poly[0], 0, 0, size, 0, 0, size, size, size);
	setRGB0(&poly[0], 0xff, 0xff, 0xff);
	//SetShadeTex(&poly[0], 1);

	SetPolyF4(&pf4);
	setXY4(&pf4, 0, 0, 50, 0, 0, 50, 50, 50);
	setRGB0(&pf4, 0xff, 0xff, 0xff);

	psxLoadTim(&tpage, &clut, img_logo);
	
	while(1) {
		psxClear();
		
		psCamera(0, 1000, 350, 400, 0, 0);

		psxGte(0, 0, 1024, 0, y-=32, 0);
		poly[0].tpage = tpage;
		poly[0].clut = clut;
		RotTransPers(&vec[0], (long *)&poly[0].x0, 0, 0);
		RotTransPers(&vec[1], (long *)&poly[0].x1, 0, 0);
		RotTransPers(&vec[2], (long *)&poly[0].x2, 0, 0);
		RotTransPers(&vec[3], (long *)&poly[0].x3, 0, 0);

		psxAddPrimTex(&poly[0]);

		psxGte(-200, 0, 1024, yy, 0, yy);
		yy+=32;
		RotTransPers(&v[0], (long *)&pf4.x0, 0, 0);
		RotTransPers(&v[1], (long *)&pf4.x1, 0, 0);
		RotTransPers(&v[2], (long *)&pf4.x2, 0, 0);
		RotTransPers(&v[3], (long *)&pf4.x3, 0, 0);
		psxAddPrim(&pf4);

		FntPrint("		  psx graphics demo \n\n");
		FntPrint("y: %d", y);

		psxDisplay();
	}
}