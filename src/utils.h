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
Node *currentNode;

typedef struct PlaneNode {
    Mesh data;
    struct PlaneNode *next;
} PlaneNode;
PlaneNode *planeNode;

void node_push(void *data);
void node_free();
void planeNode_push(long *_pos, short *_size, Mesh mesh);
void planeNode_free();
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
