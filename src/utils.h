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

void init_stages();

typedef struct Stage {
	char *tim_name;
	VECTOR camera_pos;
	SVECTOR camera_rot;
	Mesh *planes;
	int planes_length;
	Spawn *spawns;
	int spawns_length;
	VECTOR *zones;
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

#endif
