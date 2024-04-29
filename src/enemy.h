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
void enemy_update(Enemy *enemy);
void enemy_spawn(Enemy *enemy);
void enemy_push(u_short tpage, u_char type);
Enemy* enemy_get(int n);
void print_enemy_node(EnemyNode *head);
void enemy_free();

#endif
