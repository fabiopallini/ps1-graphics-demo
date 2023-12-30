#ifndef PSX_H
#define PSX_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <stdio.h>
#include <libspu.h>
#include <libds.h>
#include <strings.h>
#include <libmath.h>

#define SCREEN_WIDTH 320
#define	SCREEN_HEIGHT 256
#define SECTOR 2048

#define FNT_HEIGHT 29 
#define FNT_WIDTH 39 

#define PADLsquare 128
#define PADLcircle 32 
#define PADLcross 64 
#define PADLtriangle 16 

#define PI 3.14159265358979323846

/*
	#define PADLup     (1<<12)
	#define PADLdown   (1<<14)
	#define PADLleft   (1<<15)
	#define PADLright  (1<<13)
	#define PADRup     (1<< 4)
	#define PADRdown   (1<< 6)
	#define PADRleft   (1<< 7)
	#define PADRright  (1<< 5)
	#define PADi       (1<< 9)
	#define PADj       (1<<10)
	#define PADk       (1<< 8)
	#define PADl       (1<< 3)
	#define PADm       (1<< 1)
	#define PADn       (1<< 2)
	#define PADo       (1<< 0)
	#define PADh       (1<<11)
	#define PADL1      PADn
	#define PADL2      PADo
	#define PADR1      PADl
	#define PADR2      PADm
	#define PADstart   PADh
	#define PADselect  PADk
*/

typedef struct {
	long x,z,y,rx,ry,rz,ox;
} GameCamera;

GameCamera camera;
u_long pad, opad;
u_char frame;
long font_id[2];

void psSetup();
void psClear();
void psGte(long x, long y, long z, short ax, short ay, short az);
void psDisplay();
void psAddPrimF4(POLY_F4 *poly);
void psAddPrimFT4(POLY_FT4 *poly);
void psAddPrimFT4otz(POLY_FT4 *poly, long otz);
void psLoadTim(u_short* tpage, unsigned char image[]);
void psCamera(GameCamera camera);
void cd_open();
void cd_close();
void cd_read_file(unsigned char* file_path, u_long** file);
void audio_init();
void audio_vag_to_spu(u_char* sound_data, u_long sound_size, int voice_channel);
void audio_play(int voice_channel);
void audio_free(unsigned long spu_address);

#endif
