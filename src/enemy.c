#include "enemy.h"
#include "utils.h"

void enemy_load(unsigned char img[], Sprite *sprite, Sprite *blood){
	sprite_init(sprite, 64, 64, img);
	sprite_setuv(sprite, 0, 0, 16, 16);
	sprite_init(blood, 64, 64, img);
	sprite_setuv(blood, 16, 16, 16, 16);
}

void enemy_update(Enemy *enemy, Sprite player, long cameraX, int TOP_Z, int BOTTOM_Z){
	sprite_anim(&enemy->sprite, 16, 16, 0, 0, 5);
	if(enemy->sprite.posX < (cameraX*-1) - 1500 || enemy->sprite.hp <= 0)
		enemy_pop(enemy, cameraX, TOP_Z, BOTTOM_Z);

	if(enemy->sprite.hitted == 1)
		enemy->sprite.hitted = sprite_anim(&enemy->blood, 16, 16, 1, 0, 5);

	// catch the player
	if(inCameraView(enemy->sprite, cameraX)){
		if(enemy->sprite.posZ < player.posZ){
			enemy->sprite.posZ++;
		}
		if(enemy->sprite.posZ > player.posZ){
			enemy->sprite.posZ--;
		}
		if(enemy->sprite.posX < player.posX){
			enemy->sprite.posX++;
		}
		if(enemy->sprite.posX > player.posX){
			enemy->sprite.posX--;
		}
	}
}

void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z){
	int randomX = cameraX*-1 + 1500 + rand() % 500;
	int randomZ = BOTTOM_Z + rand() % (TOP_Z+(BOTTOM_Z*-1));
	enemy->sprite.posZ = randomZ;
	enemy->sprite.posX = randomX; 
	enemy->sprite.hp = 3;
}

void enemy_draw(Enemy *enemy){
	if(enemy->sprite.hp > 0)
		sprite_draw(&enemy->sprite);
	if(enemy->sprite.hitted == 1)
		sprite_draw(&enemy->blood);
}
