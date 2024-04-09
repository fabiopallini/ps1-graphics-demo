#ifndef ENEMY_H 
#define ENEMY_H 

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

typedef struct EnemyNode {
    Enemy *enemy;
    struct EnemyNode *next;
} EnemyNode;
EnemyNode *enemyNode;

void enemy_init(Enemy *enemy, u_short tpage, u_char type);
void enemy_update(Enemy *enemy, Sprite playe, long cameraX, int TOP_Z, int BOTTOM_Z);
void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z);
//void enemy_push(u_short tpage);
void enemy_push(Enemy *enemy, u_short tpage);
Enemy* enemy_get(int n);
void print_enemy_node(EnemyNode *head);
void enemy_free_all();

#endif
