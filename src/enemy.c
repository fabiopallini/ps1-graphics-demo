#include "enemy.h"
#include "psx.h"
#include "utils.h"

static void enemy_spawn(Enemy *enemy, long x, long y, long z){
	enemy->atb = 0;
	enemy->attacking = 0;
	enemy->sprite.pos.vx = x; 
	enemy->sprite.pos.vy = y; 
	enemy->sprite.pos.vz = z;
	enemy->sprite.rot.vx = 0; 
	enemy->sprite.rot.vy = 0; 
	enemy->sprite.rot.vz = 0;
	enemy->hp = 3;
	enemy->hitted = 0;
	enemy->prev_pos = enemy->sprite.pos; 
	if(enemy->type == BAT_GREEN)
		enemy->hp = 6;
}

void enemy_init(Enemy *enemy, u_short tpage, ENEMY_TYPE type){
	memset(enemy, 0, sizeof(Enemy));
	sprite_init(&enemy->sprite, 64, 64, tpage);
	sprite_set_uv(&enemy->sprite, 0, 0, 16, 16);
	sprite_init(&enemy->blood, 64, 64, tpage);
	sprite_set_uv(&enemy->blood, 16, 16, 16, 16);
	enemy->type = type;
	enemy->speed = 20;
	enemy->atb = 0;
	enemy->atb_time = 500;
	enemy->atb_speed = 1;
	enemy->attacking = 0;
	if(enemy->type > BAT){
		enemy->atb_speed = 2;
	}
	enemy->prev_pos.vx = 0;
	enemy->prev_pos.vy = 0;
	enemy->prev_pos.vz = 0;

	if(enemy->type == 0)
		sprite_set_animation(&enemy->sprite, 16, 16, 0, 0, 5, 1);
	if(enemy->type == 1)
		sprite_set_animation(&enemy->sprite, 16, 16, 2, 0, 5, 1);
}

void enemy_update(Enemy *enemy, Mesh mesh, int battle_status){
	if(sprite_animation_over(&enemy->blood))
		enemy->hitted = 0;
	if(enemy->hp > 0)
	{
		if(enemy->atb < enemy->atb_time && ENEMY_ATTACKING == 0 && battle_status == BATTLE_WAIT){
			enemy->atb += enemy->atb_speed;
		}

		if(enemy->atb >= enemy->atb_time && ENEMY_ATTACKING == 0){
			enemy->atb = 0;
			enemy->attacking = 1;
			ENEMY_ATTACKING = 1;
		}

		if(enemy->attacking == 1){
			if(enemy->sprite.pos.vx < mesh.pos.vx - (mesh.size)){
				enemy->sprite.pos.vx += enemy->speed;
			}
			if(enemy->sprite.pos.vz > mesh.pos.vz + (mesh.size)){
				enemy->sprite.pos.vz -= enemy->speed;
			}
			if(enemy->sprite.pos.vz < mesh.pos.vz + (mesh.size)){
				enemy->sprite.pos.vz += enemy->speed;
			}
			if(enemy->sprite.pos.vx >= mesh.pos.vx - (mesh.size))
				enemy->attacking = 2;
		}

		if(enemy->attacking == 4){
			u_char moving = 0;
			if(enemy->sprite.pos.vx > enemy->prev_pos.vx){
				enemy->sprite.pos.vx -= enemy->speed;
				moving = 1;
			}
			if(enemy->sprite.pos.vz > enemy->prev_pos.vz){
				enemy->sprite.pos.vz -= enemy->speed;
				moving = 1;
			}
			if(enemy->sprite.pos.vz < enemy->prev_pos.vz){
				enemy->sprite.pos.vz += enemy->speed;
				moving = 1;
			}
			if(moving == 0){
				enemy->attacking = 0;
				ENEMY_ATTACKING = 0;
			}
		}
	}
}

EnemyNode *enemy_create(Enemy *enemy) {
	EnemyNode* newNode = malloc3(sizeof(EnemyNode));
	if(newNode == NULL) {
		printf("error on EnemyNode malloc3 \n");
		return NULL; 
	}
	newNode->enemy = enemy;
	newNode->next = NULL;
	return newNode;
}

void enemy_push(u_short tpage, ENEMY_TYPE type, long x, long y, long z) {
	EnemyNode *newNode = NULL;
	EnemyNode *current = enemyNode;

	Enemy *e = malloc3(sizeof(Enemy));
	enemy_init(e, tpage, type);
	scene_add(&e->sprite, GFX_SPRITE_DRAW);
	enemy_spawn(e, x, y, z);

	newNode = enemy_create(e);
	if(current == NULL) {
		enemyNode = newNode;
		return;
	}
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = newNode;
}

Enemy* enemy_get(int n){
	int i = 0;
	EnemyNode *node = enemyNode;
	while(node != NULL) {
		if(i == n)
			return node->enemy;	
		i++;
		node = node->next;
	}
	return NULL;
}

void enemy_clean() { // clean all enemies with hp <= 0
	EnemyNode *node = enemyNode;
	EnemyNode *prev = NULL;

	while (node != NULL) {
		if (node->enemy->hp <= 0) {
			if (prev == NULL) {  // first node 
				enemyNode = node->next;
			} else {
				prev->next = node->next;
			}
			free3(node->enemy);
			free3(node);
			node = prev ? prev->next : enemyNode;
		} else {
			prev = node;
			node = node->next;
		}
	}
}

void enemy_free(){
	EnemyNode *node = enemyNode;
	while(node != NULL) {
		EnemyNode *nextNode = node->next;
		free3(node->enemy);
		free3(node);
		node = nextNode;
	}
	enemyNode = NULL;
}

void print_enemy_node(EnemyNode *head) {
	EnemyNode *node = head;
	while(node != NULL) {
		printf("EnemyNode pos.vx %ld \n", node->enemy->sprite.pos.vx);
		node = node->next;
	}
}
