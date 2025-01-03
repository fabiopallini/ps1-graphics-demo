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
#include <rand.h>

#include "sprite.h"
#include "mesh.h"
#include "char.h"
#include "ui.h"

#define OTSIZE 1024
#define PADLsquare 128
#define PADLcircle 32 
#define PADLcross 64 
#define PADLtriangle 16 

#define true 1 
#define false 0 
#define PI 3.14

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
#define FONT_MAX_CHARS 100 

typedef struct Camera {
	VECTOR pos;
	SVECTOR rot;
	MATRIX mtx;
	VECTOR tmp;
} Camera;

Camera camera;

typedef enum DataType {
	TYPE_MESH,
	TYPE_SPRITE,
	TYPE_SPRITE2D,
	TYPE_CHARACTER,
	TYPE_UI,
	TYPE_FONT,
} DataType;

typedef struct Node {
	void *data;
	DataType type;
	struct Node *next;
} Node;

typedef struct Scene {
	Node *node;
	u_char status;
	void (*load_callback)();
} Scene;
Scene scene;

typedef struct Font {
	DR_MODE dr_mode[FONT_MAX_CHARS];
	SPRT sprt[FONT_MAX_CHARS];
	u_short index;
} Font;
Font font;

typedef struct VagSong {
	u_char *name;
	u_long size;
	u_int block_size;
	u_char state; // 0=stop 1=playing 2=end
	u_long spu_addr;
	volatile unsigned int chunk_addr;
	volatile u_long *data;
	volatile u_int data_size;
	volatile u_char block;
	volatile u_char read_chunk;
} VagSong;
VagSong vagSong;

volatile int DSR_callback_id;
u_long pad, opad;

typedef enum Game_Status {
	VAG_READ = 1,
	VAG_TRANSFER,
	VAG_TRANSFERING,
	SCENE_READY,
	SCENE_LOAD,
} Game_Status;

void clearVRAM_at(int x, int y, int w, int h);
void clearVRAM();
void psInit();
void psClear();
void psDisplay();
void psExit();
void psGte(VECTOR pos, SVECTOR rot);

void cd_open();
void cd_close();
u_long cd_read_file(unsigned char* file_path, u_long** file);
void cd_read_file_bytes(unsigned char* file_path, u_long** file, unsigned long start_byte, unsigned long end_byte, u_char callbackID);
u_short loadToVRAM(u_long *image); // from cd-rom
u_short loadToVRAM2(unsigned char image[]); // from bin2h.exe

void font_init();
void spu_init();

// always call after any sfx_load
void vag_song_play(u_char* vagName);
void vag_song_free(VagSong *vagSong);

unsigned long sfx_load(u_char *name, u_long voice_bit);
void sfx_play(u_long voice_bit);
void sfx_pause(u_long voice_bit);
void sfx_free(unsigned long spu_address);

void drawSprite(Sprite *sprite, long _otz);
void drawSprite_2d(Sprite *sprite, long _otz);
void drawSprt(DR_MODE *dr_mode, SPRT *sprt, long _otz);
void drawFont(char *text, int xx, int yy, u_char autoReturn);
void drawMesh(Mesh *mesh, long _otz);
void drawBBox(BBox *bb);

void node_push(Node **node, void *data, DataType type);
void node_free(Node **node);
void scene_add(void *data, DataType type);
void scene_remove(void *data);
void scene_free();
void scene_draw();

void scene_load(void(*callback));
void enableScreen();
void disableSCreen();
void billboard(Sprite *sprite);
u_char random(int p);
int randomRange(int min, int max);
int degrees_to_rot(int degrees);

#endif
