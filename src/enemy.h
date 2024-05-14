#ifndef ENEMY_H 
#define ENEMY_H 

#include "sprite.h"
#include "mesh.h"

enum ENEMY {
	BAT = 0,
	BAT_GREEN = 1 
};

typedef struct {
	Sprite sprite;
	Sprite blood;
	enum ENEMY type;
	int speed;	
	int atb;
	int atb_time;
} Enemy;

typedef struct EnemyNode {
    Enemy *enemy;
    struct EnemyNode *next;
} EnemyNode;
EnemyNode *enemyNode;

u_char ENEMY_ATTACKING;

void enemy_init(Enemy *enemy, u_short tpage, u_char type);
void enemy_update(Enemy *enemy, Mesh mesh);
void enemy_spawn(Enemy *enemy, long x, long z);
void enemy_push(u_short tpage, u_char type, long x, long z);
Enemy* enemy_get(int n);
void print_enemy_node(EnemyNode *head);
void enemy_free();

#endif
