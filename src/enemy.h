#ifndef ENEMY_H 
#define ENEMY_H 

#include "psx.h"

typedef enum ENEMY_TYPE {
	BAT = 0,
	BAT_GREEN = 1 
} ENEMY_TYPE;

typedef struct Enemy {
	Sprite sprite;
	Sprite blood;
	ENEMY_TYPE type;
	int speed;	
	int atb;
	int atb_time;
	int atb_speed;
	u_char attacking, hitted;
	VECTOR prev_pos; 
	int hp, hp_max;
} Enemy;

typedef struct EnemyNode {
    Enemy *enemy;
    struct EnemyNode *next;
} EnemyNode;
EnemyNode *enemyNode;

u_char ENEMY_ATTACKING;

void enemy_init(Enemy *enemy, u_short tpage, ENEMY_TYPE type);
void enemy_update(Enemy *enemy, Mesh mesh, int battle_status);
void enemy_push(u_short tpage, ENEMY_TYPE type, long x, long y, long z);
Enemy* enemy_get(int n);
void print_enemy_node(EnemyNode *head);
void enemy_clean();
void enemy_free();

#endif
