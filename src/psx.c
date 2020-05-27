#include "psx.h"

#define colorR 0
#define colorG 48
#define colorB 150

DISPENV	dispenv[2];
DRAWENV	drawenv[2];
int dispid = 0;
u_long ot[256];
u_short otIndex;

typedef struct CAMERA {
	VECTOR pos;
	SVECTOR rot;
	MATRIX mtx;
	VECTOR tmp;
} CAMERA;

CAMERA camera;

SVECTOR gte_ang = {0,0,0};
VECTOR gte_pos = {0,0,0};

void clearVRAM()
{
	RECT rectTL;
	setRECT(&rectTL, 0, 0, 1024, 512);
	ClearImage2(&rectTL, 0, 0, 0);
	DrawSync(0);
}

void psSetup(int x, int y, int z)
{
	ResetCallback();
	ResetGraph(0);
	PadInit(0);
	InitGeom();
	clearVRAM();

	if(*(char *)0xbfc7ff52=='E')
		SetVideoMode(MODE_PAL);
	else
		SetVideoMode(MODE_NTSC);

	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 1);
	SetGeomOffset(x, y);
	SetGeomScreen(z);

	SetDefDrawEnv(&drawenv[0], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[1], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[0], 0, SCREEN_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[1], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDispMask(1);

	drawenv[0].isbg = 1;
	setRGB0(&drawenv[0], colorR, colorG, colorB);
	drawenv[1].isbg = 1;
	setRGB0(&drawenv[1], colorR, colorG, colorB);

 	// load the font from the BIOS into the framebuffer
	FntLoad(960, 256);
	// screen X,Y | max text length X,Y | autmatic background clear 0,1 | max characters
	SetDumpFnt(FntOpen(5, 20, 320, 240, 0, 512));
}

void psDisplay(){
	FntFlush(-1);
	DrawSync(0);
	VSync(0);
	PutDrawEnv(&drawenv[dispid]);
	PutDispEnv(&dispenv[dispid]);
	DrawOTag(ot);
	otIndex = 0;
}

void psClear(){
	dispid = (dispid + 1) %2;
	ClearOTag(ot, 256);
}

void psGte(long x, long y, long z, short ax, short ay, short az)
{
	/* PlayStation's rotation matrix values are pretty funky.
	 * 0'-360' = 0-4096*/
	MATRIX m;
	gte_pos.vx = x;
	gte_pos.vy = y;
	gte_pos.vz = z;
	gte_ang.vx = ax;
	gte_ang.vy = ay;
	gte_ang.vz = az;
	 /* Set GTE rot & trans registers for rot-trans-pers calculations: */
	RotMatrix(&gte_ang, &m); 	/* Get a rotation matrix from the vector */
	TransMatrix(&m, &gte_pos);	/* Sets the amount of parallel transfer */
	CompMatrixLV(&camera.mtx, &m, &m);
	SetRotMatrix(&m);
	SetTransMatrix(&m);
}

void psAddPrim(POLY_F4 *poly){
	AddPrim(&ot[otIndex++], poly);
}

void psAddPrimTex(POLY_FT4 *poly){
	AddPrim(&ot[otIndex++], poly);
}

void psLoadTim(u_short* tpage, u_short* clut, unsigned char image[])
{
	RECT rect;
	GsIMAGE tim;

	// skip the TIM ID and version (magic) by adding 0x4 to the pointer
	GsGetTimInfo ((u_long *)(image+4), &tim);

	// Load pattern into VRAM
	rect.x = tim.px;
   	rect.y = tim.py;
   	rect.w = tim.pw;
   	rect.h = tim.ph;
   	LoadImage(&rect, tim.pixel);

   	// Load CLUT into VRAM
   	rect.x = tim.cx;
   	rect.y = tim.cy;
   	rect.w = tim.cw;
   	rect.h = tim.ch;
   	LoadImage(&rect, tim.clut);

   	// Return TPage and CLUT IDs
   	(*tpage) = GetTPage(tim.pmode, 1, tim.px, tim.py);
   	(*clut)  = GetClut(tim.cx, tim.cy);
}

void psCamera(long x, long y, long z, short rotX, short rotY, short rotZ){
	camera.rot.vx = rotX;
	camera.rot.vy = rotY;
	camera.rot.vz = rotZ;
	camera.pos.vx = x;
	camera.pos.vy = y;
	camera.pos.vz = z;
	RotMatrix(&camera.rot, &camera.mtx);
	ApplyMatrixLV(&camera.mtx, &camera.pos, &camera.tmp);	
	TransMatrix(&camera.mtx, &camera.tmp);
	SetRotMatrix(&camera.mtx);
	SetTransMatrix(&camera.mtx);
}

void psPadInput(long *x, long *y, long *z){
	u_long padd = PadRead(1);
	if(padd & PADLup)
		*y -= 32;
	if(padd & PADLdown)
		*y += 32;
	if(padd & PADLleft)
		*x += 32;
	if(padd & PADLright)
		*x -= 32;
	if(padd & PADRup)
		*z += 32;
	if(padd & PADRdown)
		*z -= 32;
}