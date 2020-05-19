#include "psx.h"
#include "images.h"

extern u_long bgtex[];
POLY_FT4 poly[2];
SVECTOR vec[4];
int size = 200;
long z = 1024;
short y = 0;

u_short tpage, clut;

int main() {
	psxGfxSetup(160, 120, 512);

	setVector(&vec[0], -128, -128, 0);
	setVector(&vec[1], 128, -128, 0);
	setVector(&vec[2], -128, 128, 0);
	setVector(&vec[3], 128, 128, 0);

	SetPolyFT4(&poly[0]);
	//setUVWH(&poly[0], 0, 0, 255, 255);
	setXY4(&poly[0], 0, 0, size, 0, 0, size, size, size);
	setUV4(&poly[0], 0, 0, size, 0, 0, size, size, size);
	setRGB0(&poly[0], 0xff, 0xff, 0xff);
	//SetShadeTex(&poly[0], 1);

	psxTimLoad(&tpage, &clut, img_logo);

	while(1) {
		//psxSetGtePos(0, 0, z);
		psxSetGteAng(0, y-=32 ,0);
		psxClear();

		poly[0].tpage = tpage;
		poly[0].clut = clut;
		RotTransPers(&vec[0], (long *)&poly[0].x0, 0, 0);
		RotTransPers(&vec[1], (long *)&poly[0].x1, 0, 0);
		RotTransPers(&vec[2], (long *)&poly[0].x2, 0, 0);
		RotTransPers(&vec[3], (long *)&poly[0].x3, 0, 0);
		psxAddPrimTex(&poly[0]);

		FntPrint("			psx graphics demo \n\n");
		FntPrint("y: %d", y);

		psxDisplay();
	}
}