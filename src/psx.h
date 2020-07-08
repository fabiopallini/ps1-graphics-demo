#ifndef PSX_H
#define PSX_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>

#define SCREEN_WIDTH 320
#define	SCREEN_HEIGHT 256

u_long pad;

void psSetup();
void psClear();
void psGte(long x, long y, long z, short ax, short ay, short az);
void psDisplay();
void psAddPrimF4(POLY_F4 *poly);
void psAddPrimFT4(POLY_FT4 *poly);
void psLoadTim(u_short* tpage, unsigned char image[]);
void psCamera(long x, long y, long z, short rotX, short rotY, short rotZ);

#endif
