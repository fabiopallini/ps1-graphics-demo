#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "mesh.h"
#include "enemy.h"

int mesh_on_plane(long x, long z, Mesh p);
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
int sprite_collision(Sprite *s1, Sprite *s2);
int sprite_collision2(Sprite *s1, Sprite *s2);
int balloon_collision(Sprite *s1, Sprite *s2);
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
