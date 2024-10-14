#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"
#include "char.h"

#define MAX_TARGETS 10
#define FONT_MAX_CHARS 78
#define SELECTOR_POSY 185

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct Spawn {
	VECTOR pos;
	SVECTOR rot;
} Spawn;

typedef struct Zone {
	VECTOR pos;
	int w, h, z;
	Mesh mesh;
	int stage_id;
	int spawn_id;
} Zone;

typedef struct Stage {
	unsigned int id;
	char *tims[2];
	VECTOR camera_pos;
	SVECTOR camera_rot;
	Mesh planes[5];
	Spawn spawns[5];
	Zone zones[5];
	int planes_length;
	int spawns_length;
	int zones_length;
} Stage;

typedef struct PlaneData {
	int x, y, z;
	int w, h, d;
} PlaneData;

typedef struct SpawnData {
	int x, y, z;
	short rx, ry, rz; 
} SpawnData;

typedef struct ZoneData {
	int x, y, z;
	int w, h, d;
	int stage_id;
	int spawn_id;
} ZoneData;

typedef struct StageData {
	char tims[2][10];
	int cam_x, cam_y, cam_z;
	short cam_rx, cam_ry, cam_rz;
	PlaneData planes[5];
	SpawnData spawns[5];
	ZoneData zones[5];
	unsigned char planes_len;
	unsigned char spawns_len;
	unsigned char zones_len;
} StageData;
StageData stageData;

typedef struct Background {
	Sprite s0,s1;
	u_short tpages[2];
} Background;

typedef struct {
	Sprite sprite[4];
	int display_time;
} DMG;

typedef struct {
	DR_MODE dr_mode[FONT_MAX_CHARS];
	SPRT sprt[FONT_MAX_CHARS];
} Font;

typedef struct {
	float value;
	Sprite bar, border; 
} ATB;

typedef struct Battle {
	ATB atb[2];
	Sprite command_bg, selector;
	u_char command_mode;
	u_char command_index;
	u_char command_attack;

	u_char target;
	u_char target_counter;
	u_char targets[MAX_TARGETS];
	u_char calc_targets;

	Character chars[3];
	Enemy enemies[3];
	DMG dmg;
} Battle;

void node_push(Node **node, void *data);
void node_free(Node **node);
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);
void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id);
size_t strlen_delimiter(const u_char *ptr, u_char delimiter);
void print_bytes(u_long *buffer, size_t size);
void background_init(Background *b);
void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz));
void battle_init(Battle *battle, u_short tpage, int screenW, int screenH);
void openBattleMenu(Battle *battle);
void closeBattleMenu(Battle *battle);

#endif
