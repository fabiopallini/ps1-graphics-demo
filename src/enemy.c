#include "enemy.h"
#include "utils.h"

void enemy_load(Enemy *enemy, u_long *img, u_char type){
	sprite_init(&enemy->sprite, 64, 64, img);
	sprite_set_uv(&enemy->sprite, 0, 0, 16, 16);
	sprite_init(&enemy->blood, 64, 64, img);
	sprite_set_uv(&enemy->blood, 16, 16, 16, 16);
	enemy->type = type;
	enemy->speed = 2;
	if(enemy->type > BAT){
		enemy->speed = 4;
	}
}

void enemy_update(Enemy *enemy, Sprite player, long cameraX, int TOP_Z, int BOTTOM_Z){
	if(enemy->sprite.hitted == 1)
		enemy->sprite.hitted = sprite_anim(&enemy->blood, 16, 16, 1, 0, 5);

	if(enemy->sprite.hp > 0){
		if(enemy->type == 0)
			sprite_anim(&enemy->sprite, 16, 16, 0, 0, 5);
		if(enemy->type == 1)
			sprite_anim(&enemy->sprite, 16, 16, 2, 0, 5);

		// catch the player
		if(inCameraView(enemy->sprite, cameraX))
		{
			if(enemy->sprite.pos.vz < player.pos.vz){
				enemy->sprite.pos.vz += enemy->speed/2;
			}
			if(enemy->sprite.pos.vz > player.pos.vz){
				enemy->sprite.pos.vz -= enemy->speed/2;
			}
		}
		if(enemy->sprite.pos.vx < player.pos.vx){
			enemy->sprite.pos.vx += enemy->speed;
		}
		if(enemy->sprite.pos.vx > player.pos.vx){
			enemy->sprite.pos.vx -= enemy->speed;
		}
	}
}

void enemy_pop(Enemy *enemy, long cameraX, int TOP_Z, int BOTTOM_Z){
	int randomX = cameraX*-1 + 1000 + rand() % 500;
	int randomZ = BOTTOM_Z + rand() % (TOP_Z+(BOTTOM_Z*-1));
	enemy->sprite.pos.vz = randomZ;
	enemy->sprite.pos.vx = randomX; 
	enemy->sprite.hp = 3;
	if(enemy->type == BAT_GREEN)
		enemy->sprite.hp = 6;
}
