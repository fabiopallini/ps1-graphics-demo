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

void enemy_update(Sprite *player, Enemy *enemy, long cameraX, int WALL_Z, int FALL_Z){
	sprite_anim(&enemy->sprite, 16, 16, 0, 0, 5);
	enemy->sprite.posX -= 1;
	if(enemy->sprite.posX < (cameraX*-1) - 1500){
		enemy->sprite.posX = cameraX*-1 + 2000;
		enemy->sprite.posZ = FALL_Z + rand()/35;
		if(enemy->sprite.posZ > WALL_Z)
			enemy->sprite.posZ = WALL_Z;
		if(enemy->sprite.posZ < FALL_Z)
			enemy->sprite.posZ = FALL_Z;
	}

	if(enemy->sprite.hitted == 1){
		enemy->sprite.hitted = sprite_anim(&enemy->blood, 16, 16, 1, 0, 5);
	}
	if(enemy->sprite.hp <= 0){
		enemy->sprite.posX = cameraX*-1 + 2000;
		enemy->sprite.posZ = FALL_Z + rand()/35;
		enemy->sprite.hp = 3;
	}

	/*if(sprite_collision(player, enemy) == 1 && player->hitted == 0 && player->hp > 0){
		player->hp -= 1;
		energy_bar[0].w = ((player->hp * 70) / player->hp_max); 
		player->hitted = 1;*/
}

void enemy_draw(Enemy *enemy){
	sprite_draw(&enemy->sprite);
	if(enemy->sprite.hitted == 1)
		sprite_draw(&enemy->blood);
}
