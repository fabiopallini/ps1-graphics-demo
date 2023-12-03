#ifndef ENEMY_H 
#define ENEMY_H 

#include "sprite.h"

typedef struct enemy 
{
	Sprite sprite;
	Sprite blood;
} Enemy;

void enemy_load(unsigned char img[], Sprite *sprite, Sprite *blood);
void enemy_update(Sprite *player, Enemy *enemy, long cameraX, int WALL_Z, int FALL_Z);
void enemy_draw(Enemy *enemy);

#endif
