#include "enemy.h"

void enemy_load(unsigned char img[], Sprite *sprite, Sprite *blood){
	sprite_init(sprite, 64, 64, img);
	sprite_setuv(sprite, 0, 0, 16, 16);
	sprite->hp = 3;
	sprite->posX = 250;
	sprite->posZ = 300;

	sprite_init(blood, 64, 64, img);
	sprite_setuv(blood, 16, 16, 16, 16);
	blood->posX -= 350;
	blood->posZ = 250;
}

void enemy_update(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z){
	sprite_anim(&enemy->sprite, 16, 16, 0, 0, 5);
	enemy->sprite.posX -= 1;
	if(enemy->sprite.posX < (cameraX*-1) - 1500 || enemy->sprite.hp <= 0)
		enemy_reset(enemy, cameraX, TOP_Z, BOTTOM_Z);

	if(enemy->sprite.hitted == 1)
		enemy->sprite.hitted = sprite_anim(&enemy->blood, 16, 16, 1, 0, 5);
}

void enemy_reset(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z){
	enemy->sprite.posX = cameraX*-1 + 2000;
	enemy->sprite.posZ = BOTTOM_Z + rand()/35;
	if(enemy->sprite.posZ > TOP_Z)
		enemy->sprite.posZ = TOP_Z;
	if(enemy->sprite.posZ < BOTTOM_Z)
		enemy->sprite.posZ = BOTTOM_Z;
	enemy->sprite.hp = 3;
}

void enemy_draw(Enemy *enemy){
	sprite_draw(&enemy->sprite);
	if(enemy->sprite.hitted == 1)
		sprite_draw(&enemy->blood);
}
