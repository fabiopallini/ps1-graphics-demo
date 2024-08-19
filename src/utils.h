#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct PlaneNode {
    Mesh data;
    struct PlaneNode *next;
} PlaneNode;
PlaneNode *planeNode;

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

void init_stages();

typedef struct Stage {
	char *tim_name;
	VECTOR camera_pos;
	SVECTOR camera_rot;
	Mesh *planes;
	int planes_length;
	Spawn *spawns;
	int spawns_length;
	Zone *zones;
	int zones_length;
} Stage;
Stage stages[3];
Stage *stage;

void node_push(Node **node, void *data);
void node_free(Node **node);
void planeNode_push(long *_pos, short *_size, Mesh mesh);
void planeNode_free();
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);
void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id);

#endif
