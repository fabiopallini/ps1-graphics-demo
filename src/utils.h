#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "sprite.h"
#include "enemy.h"

#define PADLsquare 128
#define PADLcircle 32 
#define PADLcross 64 
#define PADLtriangle 16 

typedef struct {
	VECTOR pos;
	SVECTOR rot;
	MATRIX mtx;
	VECTOR tmp;
	long ox;
} Camera;

Camera camera;

int ray_collisions(Sprite *s, Enemy enemies[], int n_enemies, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
int sprite_collision(Sprite *s1, Sprite *s2);
int inCameraView(Sprite s, long cameraX);
int cameraLeft(long cameraX);
int cameraRight(long cameraX);

#endif
