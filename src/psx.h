#ifndef PSX_H
#define PSX_H

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <libspu.h>
#include <libds.h>
#include <libmath.h>
#include <libapi.h>
#include <libcd.h>
#include <libsnd.h>
#include <libsn.h>
#include <rand.h>

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
#define BALLOON_MAX_CHARS 78

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

typedef struct 
{
	POLY_FT4 ft4;
	POLY_F4 f4;
	SVECTOR vector[4];
	u_short tpage;
	int w, h;
	VECTOR pos; 
	SVECTOR rot; 
	char prevFrame, prevRow, frame, frameTime, jump_speed, frameInterval;
	u_char direction, isJumping, hitted, shooting;
	int hittable;
	int hp, hp_max;
	
} Sprite;

typedef enum {
	QUADS = 1,
	TRIANGLES
} MeshType;

typedef struct 
{
	MeshType type;
	POLY_FT4 *ft4;
	POLY_F4 *f4;
	POLY_FT3 *ft3;
	POLY_F3 *f3;
	SVECTOR *vertices;
	int *indices;
	int verticesLength, indicesLength;
	u_short tpage;
	int size;
	VECTOR pos; 
	SVECTOR rot;
} Mesh;

typedef struct BBox {
	POLY_F4 *poly_f4;
	SVECTOR vertices[4];
	VECTOR pos;
	SVECTOR rot;
} BBox;

typedef struct {
	Sprite sprite;
	char prev_display, display;
	char *text;
	int page_index;
	int pages_length;
	int npc_id;
	char *tale[10];
} Balloon;
Balloon balloon;

typedef struct {
	Mesh *meshFrames;
	u_char start_frame;
	u_char frames;
	u_char timer;
	u_char loop;
	u_char interval;
	u_char current_frame;
} MeshAnimation;

typedef struct 
{
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	unsigned char RUN_SPEED;
	VECTOR pos, map_pos, battle_pos;
	SVECTOR rot, map_rot, battle_rot;

	MeshAnimation *meshAnimations;
	u_char animations_len;
	u_short animation_to_play;
	u_char play_animation;
	u_char animation_name[6];
} Character;

void sprite_init(Sprite *sprite, int w, int h, u_short tpage);
void sprite_shading_disable(Sprite *sprite, int disable);
void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h);
void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b, int semitrans);
short sprite_anim(Sprite *sprite, short w, short h, short row, short firstFrame, short frames);
short sprite_set_frame(Sprite *sprite, short w, short h, short row, short frame, short time);
int sprite_collision(Sprite *s1, Sprite *s2);
int sprite_collision2(Sprite *s1, Sprite *s2);
int balloon_collision(Sprite *s1, Sprite *s2);

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, u_short w, u_short h, short mesh_size);
void mesh_load(Mesh *mesh);
void mesh_free(Mesh *mesh);
void mesh_set_rgb(Mesh *mesh, u_char r, u_char g, u_char b, int semitransparent);
void mesh_set_resize(Mesh *mesh, float size);
void mesh_set_shadeTex(Mesh *mesh, u_char b);
int mesh_on_plane(long x, long z, Mesh p);
int mesh_collision(Mesh a, Mesh b);
int mesh_angle_to(Mesh mesh, long x, long z);
void mesh_point_to(Mesh *mesh, long x, long z);
int mesh_looking_at(Mesh *mesh, long x, long z);
void bbox_init(BBox *bb, Mesh *mesh, u_short size);
int bbox_collision(long x, long z, BBox bbox);

void init_ui(u_short tpage, int screen_width, int screen_height);
void set_balloon(Balloon *b, char *text);

void char_init(Character *c);
void char_animation_init(Character *c, u_short n_animations);
void char_animation_set(Character *c, char *anim_name, u_char animation_index, u_char start_frame, u_char frames,
u_short tpage, short img_size, short mesh_size);
void char_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz));
Mesh *char_getMesh(const Character *c);
u_char char_animation_is_over(Character c);
u_char char_get_frame(Character c);
void char_play_animation(Character *c, u_char animation_index);
void char_free_animation(Character c, u_char animation_index);
void char_set_rgb(Character c, u_char r, u_char g, u_char b);
void char_set_shadeTex(Character c, u_char b);
int char_looking_at(Character *c, long x, long z);

void xa_play(int *channel);
void xa_stop();
void PlayXAChannel(int channel, int startPos, int endpos);
void cbready(int intr, u_char *result);
void UnprepareXA(CdlCB oldCallback);
int xa_currentSecond();

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
