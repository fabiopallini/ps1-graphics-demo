#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"
#include "char.h"
#include "data.h"

typedef struct Npc {
	Mesh mesh;
	BBox bbox;
	int talk_pages;
	char **talk_chars;
} Npc;

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
	unsigned char planes_length;
	unsigned char spawns_length;
	unsigned char zones_length;
	unsigned char npcs_len;
	Npc npcs[5];
} Stage;
StageData stageData;

typedef struct Background {
	Sprite s0,s1;
	u_short tpages[2];
} Background;

int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);
void npc_init(Npc *npc, u_long *cd_obj, u_short tpage, const NpcData *npcData);
void npc_free(Npc *npc);
void npc_update(Npc *npc);
void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id);
size_t strlen_delimiter(const u_char *ptr, u_char delimiter);
void print_bytes(u_long *buffer, size_t size);
void background_init(Background *b);
void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz));
size_t strcpy_count(char *destination, const char *source);

#endif
