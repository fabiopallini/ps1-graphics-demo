#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"
#include "char.h"
#include "data.h"

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
	Npc npc;
} Stage;

StageData stageData;

typedef struct Background {
	Sprite s0,s1;
	u_short tpages[2];
} Background;

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

#endif
