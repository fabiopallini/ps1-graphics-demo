#ifndef ENEMY_H 
#define ENEMY_H 

#include "sprite.h"

typedef struct enemy 
{
	Sprite sprite;
	Sprite blood;
} Enemy;

void enemy_load(unsigned char img[], Sprite *sprite, Sprite *blood);
void enemy_update(Enemy *enemy, Sprite playe, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_draw(Enemy *enemy);

#endif
