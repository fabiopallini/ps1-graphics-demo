#include <stdlib.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libetc.h>
#include <sys/types.h>

#define __ramsize   0x00200000
#define __stacksize 0x00004000

#define SCREEN_WIDTH 320
#define	SCREEN_HEIGHT 256

void psxGfxSetup(int x, int y, int z);
void psxClear();
void psxGte(long x, long y, long z, short ax, short ay, short az);
void psxDisplay();
void psxAddPrim(POLY_F4 *poly);
void psxAddPrimTex(POLY_FT4 *poly);
void psxLoadTIM (u_short* tpage, u_short* clut, unsigned char image8[]);