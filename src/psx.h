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

#define OTSIZE 2048
#define PADLsquare 128
#define PADLcircle 32 
#define PADLcross 64 
#define PADLtriangle 16 
#define true 1 
#define false 0 
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
#define FNT_WIDTH 100 
#define PI 3.14

typedef struct {
	VECTOR pos;
	SVECTOR rot;
	MATRIX mtx;
	VECTOR tmp;
} Camera;

Camera camera;

typedef struct SpriteNode {
    volatile Sprite *data;
    struct SpriteNode *next;
} SpriteNode;

typedef struct {
	SpriteNode *spriteNode;
	u_char status;
	void (*load_callback)();
} Scene;
Scene scene;

typedef struct Vag {
	u_char *name;
	u_char state;
	u_long spu_addr;
	unsigned int chunk_size;
	volatile unsigned int chunk_addr;
	volatile u_long *data;
	volatile u_long *cd_data;
	volatile u_char block;
	volatile u_char read_chunk;
} Vag;
Vag vag;

volatile int DSR_callback_id;
u_long pad, opad;

enum Game_Status {
	VAG_READ = 1,
	VAG_TRANSFER,
	VAG_TRANSFERING,
	SCENE_READY,
	SCENE_LOAD,
};
enum Game_Status game_statuses;

void clearVRAM_at(int x, int y, int w, int h);
void clearVRAM();
void psInit();
void psClear();
void psDisplay();
void psExit();
void psGte(VECTOR pos, SVECTOR rot);

void cd_open();
void cd_close();
void cd_read_file(unsigned char* file_path, u_long** file);
void cd_read_file_bytes(unsigned char* file_path, u_long** file, unsigned long start_byte, unsigned long end_byte, u_char callbackID);
u_short loadToVRAM(u_long *image); // from cd-rom
u_short loadToVRAM2(unsigned char image[]); // from bin2h.exe

void spu_init();
void vag_load(u_char* vagName, int voice_channel);
void spu_load(u_long *vag_data, u_long vag_size, int voice_channel);
void spu_play(int voice_channel);
void spu_pause(int voice_channel);
void spu_free(unsigned long spu_address);
void vag_free(Vag *vag);

void drawSprite(Sprite *sprite, long _otz);
void drawSprite_2d(Sprite *sprite, long _otz);
void drawSprt(DR_MODE *dr_mode, SPRT *sprt, long _otz);
void drawFont(Font *font, u_char *text, int xx, int yy, u_char autoReturn);
void drawMesh(Mesh *mesh, long _otz);
void drawMesh_ptr(Mesh **pmesh, long _otz);

void scene_add_sprite(Sprite *data);
void printSpriteNode(SpriteNode *head);
void scene_freeSprites();
void scene_load(void(*callback));
void enableScreen();
void disableSCreen();

#endif
