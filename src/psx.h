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
#include <libapi.h>

#include "sprite.h"
#include "mesh.h"
#include "ui.h"

// define either PAL or NTSC
#define PAL 
//#define NTSC 

#ifdef PAL
	#define SCREEN_WIDTH 320
	#define	SCREEN_HEIGHT 256
#else
	#define SCREEN_WIDTH 320
	#define	SCREEN_HEIGHT 240
#endif

#define SECTOR 2048

#define FNT_HEIGHT 29 
#define FNT_WIDTH 39 

#define PI 3.14

typedef struct SpriteNode {
    Sprite *data;
    struct SpriteNode *next;
} SpriteNode;

typedef struct {
	SpriteNode *spriteNode;
} Scene;

Scene scene;
u_long pad, opad;
long font_id[2];

void psInit();
void psClear();
void psExit();
void psGte(VECTOR pos, SVECTOR rot);
void psDisplay();

void cd_open();
void cd_close();
void cd_read_file(unsigned char* file_path, u_long** file);
u_short loadToVRAM(u_long *image);

void audio_init();
void audio_vag_to_spu(u_char* sound_data, u_long sound_size, int voice_channel);
void audio_play(int voice_channel);
void audio_free(unsigned long spu_address);

void drawSprite(Sprite *sprite);
void drawSprite_2d(Sprite *sprite);
void drawSprite_2d_rgb(Sprite *sprite);

void drawSprt(DR_MODE *dr_mode, SPRT *sprt);

void drawFont(u_char *text, Font *font, int xx, int yy);

void mesh_draw(Mesh *mesh, int clip);
void mesh_draw_ot(Mesh *mesh, int clip, long otz);

void scene_add_sprite(Sprite *data);
void printSpriteNode(SpriteNode *head);
void scene_freeSprites();

#endif
