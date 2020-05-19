#include "psx.h"

#define colorR 0
#define colorG 48
#define colorB 150

DISPENV	dispenv[2];
DRAWENV	drawenv[2];
int dispid = 0;
u_long ot[256];
u_short otIndex;

SVECTOR gte_ang = {0,0,0};
VECTOR gte_pos = {0,0,1024};

void clearVRAM()
{
	RECT rectTL;
	setRECT(&rectTL, 0, 0, 1024, 512);
	ClearImage2(&rectTL, 0, 0, 0);
	DrawSync(0);
}

void psxGfxSetup(int x, int y, int z)
{
	ResetCallback();
	ResetGraph(0);
	//PadInit(0);
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

void psxDisplay(){
	FntFlush(-1);
	DrawSync(0);
	VSync(0);
	PutDrawEnv(&drawenv[dispid]);
	PutDispEnv(&dispenv[dispid]);
	DrawOTag(ot);
	otIndex = 0;
}

void psxClear(){
	psxUpdate_gte();
	dispid = (dispid + 1) %2;
	ClearOTag(ot, 256);
}

void psxUpdate_gte()
{
	/* PlayStation's rotation matrix values are pretty funky.
	 * 0'-360' = 0-4096*/
	 MATRIX m;
	 /* Set GTE rot & trans registers for rot-trans-pers calculations: */
	 RotMatrix(&gte_ang, &m); 	/* Get a rotation matrix from the vector */
	 TransMatrix(&m, &gte_pos);	/* Sets the amount of parallel transfer */
	 SetRotMatrix(&m);
	 SetTransMatrix(&m);
}

void psxAddPrim(POLY_F4 *poly){
	AddPrim(&ot[otIndex++], poly);
}

void psxAddPrimTex(POLY_FT4 *poly){
	AddPrim(&ot[otIndex++], poly);
}

void psxSetGtePos(long x, long y, long z){
	gte_pos.vx = x;
	gte_pos.vy = y;
	gte_pos.vz = z;
}

void psxSetGteAng(short x, short y, short z){
	gte_ang.vx = x;
	gte_ang.vy = y;
	gte_ang.vz = z;
}

void psxTimLoad(u_short* tpage, u_short* clut, unsigned char image[])
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