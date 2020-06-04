#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>

#define SCREEN_WIDTH 320
#define	SCREEN_HEIGHT 256

void psSetup(int x, int y, int z);
void psClear();
void psGte(long x, long y, long z, short ax, short ay, short az);
void psDisplay();
void psAddPrim(POLY_F4 *poly);
void psAddPrimTex(POLY_FT4 *poly);
void psLoadTim(u_short* tpage, u_short* clut, unsigned char image[]);
void psCamera(long x, long y, long z, short rotX, short rotY, short rotZ);
void psPadInput(long *x, long *y, long *z);
