#ifndef ENEMY_H 
#define ENEMY_H 

#include "psx.h"
#include "sprite.h"

enum ENEMY {
	BAT = 0,
	BAT_GREEN = 1 
};

typedef struct {
	Sprite sprite;
	Sprite blood;
	enum ENEMY type;
	u_char speed;
} Enemy;

void enemy_load(Enemy *enemy, u_long *img, u_char type);
void enemy_update(Enemy *enemy, Sprite playe, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_draw(Enemy *enemy);

#endif
