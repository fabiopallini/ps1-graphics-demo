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

//Node *enemyNode;

void node_push(Node **node, void *data);
void node_free(Node **node);
void planeNode_push(long *_pos, short *_size, Mesh mesh);
void planeNode_free();
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
