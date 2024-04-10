#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "enemy.h"

//Enemy* ray_collisions(Sprite *s, Enemy enemies[], int n_enemies, long cameraX);
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
int sprite_collision(Sprite *s1, Sprite *s2);
int sprite_collision2(Sprite *s1, Sprite *s2);
int balloon_collision(Sprite *s1, Sprite *s2);
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
