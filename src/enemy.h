#ifndef ENEMY_H 
#define ENEMY_H 

#include "sprite.h"

typedef struct enemy 
{
	Sprite sprite;
	Sprite blood;
	u_char type, speed;
} Enemy;

void enemy_load(Enemy *enemy, unsigned char img[], u_char type);
void enemy_update(Enemy *enemy, Sprite playe, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_draw(Enemy *enemy);

#endif
