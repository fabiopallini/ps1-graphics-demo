#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"

typedef struct PlaneNode {
    Mesh data;
    struct PlaneNode *next;
} PlaneNode;
PlaneNode *planeNode;

int mesh_on_plane(long x, long z, Mesh p);
void planeNode_push(long *_pos, short *_size, Mesh mesh);
void planeNode_free();
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
int sprite_collision(Sprite *s1, Sprite *s2);
int sprite_collision2(Sprite *s1, Sprite *s2);
int balloon_collision(Sprite *s1, Sprite *s2);
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
