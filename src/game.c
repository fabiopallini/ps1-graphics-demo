#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"
#include "ui.h"

#define SPEED 6 
#define MAP_BLOCKS 8 
#define BACKGROUND_BLOCK 500 
#define BACKGROUND_MARGIN 5 
#define TOP_Z 450 
#define BOTTOM_Z -280 
#define GRAVITY 10 
#define JUMP_SPEED 45 
#define JUMP_FRICTION 0.9 
#define MAX_JUMP_HEIGHT 500 

u_long *cd_data[8];
u_short tpages[5];
Mesh cube, map[MAP_BLOCKS];
Mesh mesh_player;
short mapIndex = 0;
Sprite player, cloud;
Sprite background;
int xaChannel = 0;

char fntBuf[FNT_HEIGHT];
char fnt[FNT_HEIGHT][FNT_WIDTH];

void player_input(Sprite *player, u_long pad, u_long opad, u_char player_type);
int feetCounter;
u_char cameraLock;
void commands(u_long pad, u_long opad, Sprite *player);
void waves_controller();

typedef struct {
	u_char type;
	u_char total;
} Mob;

typedef struct {
	Mob mobs[5];
	u_char total;
} WAVE;
u_char wave_index = 0;
u_char cWave = 0;
u_char n_waves = 3;
WAVE waves[3];

u_char level_clear = 0;

void start_level(){
	int i;
	player.hp = 10;
	player.hp_max = 10;
	player.pos.vx = 250;

	camera.pos.vx = 0;
	cameraLock = 0;
	wave_index = 0;
	feetCounter = 0;

	mapIndex = 0;
}

void wave_set(u_char nWave, u_char mobType, u_char nMob){
	static int i = 0;
	if(nWave != cWave){
		cWave = nWave;
		i = 0;
	}
	waves[nWave].total += nMob;
	waves[nWave].mobs[i].type = mobType;
	waves[nWave].mobs[i].total = nMob;
	i++;
}

void game_load(){
	int i;
	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;
	camera.ox = 0;

	cd_open();
	i = 0;
	cd_read_file("MISC_1.TIM", &cd_data[0]); // 640 256
	cd_read_file("BK1.TIM", &cd_data[1]);
	cd_read_file("TEX1.TIM", &cd_data[2]);
	cd_read_file("CUBE.TIM", &cd_data[3]);
	cd_read_file("BAT.TIM", &cd_data[4]);
	cd_read_file("P1.OBJ", &cd_data[5]);
	cd_read_file("CUBE.OBJ", &cd_data[6]);
	cd_read_file("GUNSHOT.VAG", &cd_data[7]);
	cd_close();

	tpages[0] = loadToVRAM(cd_data[0]); // MISC_1
	tpages[1] = loadToVRAM(cd_data[1]); // BK1
	tpages[2] = loadToVRAM(cd_data[2]); // TEX1
	tpages[3] = loadToVRAM(cd_data[3]); // CUBE
	tpages[4] = loadToVRAM(cd_data[4]); // BAT 

	/*free(cd_data[0]);
	free(cd_data[1]);
	free(cd_data[2]);
	free(cd_data[3]);
	free(cd_data[5]);*/

	audio_init();
	audio_vag_to_spu((u_char*)cd_data[7], 15200, SPU_0CH);
	
	mesh_init(&cube, cd_data[6], tpages[3], 32, 50);
	cube.pos.vx -= 350;
	sprite_init(&background, 255, 255, tpages[1]);
	background.w = SCREEN_WIDTH;
	background.h = SCREEN_HEIGHT;
	mesh_init(&mesh_player, cd_data[5], tpages[2], 128, 300);

	ui_init(tpages[0], SCREEN_WIDTH, SCREEN_HEIGHT);
	scene_add_sprite(&selector);
	scene_add_sprite(&dmg.sprite[0]);
	scene_add_sprite(&dmg.sprite[1]);
	scene_add_sprite(&dmg.sprite[2]);
	scene_add_sprite(&dmg.sprite[3]);
	start_level();

	init_balloon(&balloon, tpages[0], SCREEN_WIDTH, SCREEN_HEIGHT);
	
	xa_play();
	//free3(cd_data);	
	
	for(i = 0; i < 5; i++)
		enemy_push(tpages[4], BAT);
	for(i = 0; i < 5; i++)
		enemy_push(tpages[4], BAT_GREEN);

	for(i = 0; i < 10; i++){
		scene_add_sprite(&enemy_get(i)->sprite);
		scene_add_sprite(&enemy_get(i)->blood);
	}

	wave_set(0, BAT, 2);
	wave_set(1, BAT, 2);
	wave_set(1, BAT_GREEN, 1);
	wave_set(2, BAT, 1);
	wave_set(2, BAT_GREEN, 1);

}

void game_update()
{
	//printf("pad %ld \n", pad);
	//printf("y %ld \n", player.pos.vy);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
	
	if(pad & PADLup){
		mesh_player.pos.vz += 10;
	}
	if(pad & PADLdown){
		mesh_player.pos.vz -= 10;
	}
	if(pad & PADLleft){
		mesh_player.pos.vx -= 10;
	}
	if(pad & PADLright){
		mesh_player.pos.vx += 10;
	}		
	mesh_player.rot.vy += 10;
	cube.rot.vx += 10;
	cube.rot.vy += 10;
	cube.rot.vz += 10;
}

void game_draw(){
	if(level_clear != 2){
		char log[20];
		char str[39];
		short i = 0;
		EnemyNode *enemy_node = enemyNode;

		drawSprite_2d(&background, 1023);
		drawMesh(&cube, NULL);
		drawMesh(&mesh_player, NULL);

		/*while(enemy_node != NULL) {
			Enemy *e = enemy_node->enemy;	
			if(e->sprite.hp > 0)
				drawSprite(&e->sprite, NULL);
			if(e->sprite.hitted == 1)
				drawSprite(&e->blood, NULL);
			enemy_node = enemy_node->next;
		}*/

		sprintf(log, "camera.pos.vx %ld %ld", camera.pos.vx*-1, player.pos.vx);
		strcpy(fnt[1], log);	
		//sprintf(log, "camera.rot.vy %d", camera.rot.vy);
		//strcpy(fnt[2], log);
		
		// ===================
		// 	DRAW UI
		// ===================
		
		sprintf(str, "					Player1 %d", player.hp);
		strcpy(fnt[23], str);

		drawSprite_2d(&atb[0].bar, NULL);
		drawSprite_2d(&atb[0].border, NULL);
		
		if(command_mode > 0 && atb[0].bar.w >= 50){
			FntPrint(font_id[1], "Super Shot \n\n");
			FntPrint(font_id[1], "Magic \n\n");
			FntPrint(font_id[1], "GF \n\n");
			FntPrint(font_id[1], "Items \n\n");

			if(command_mode == 1 || command_mode == 2)
				drawSprite_2d(&selector, NULL);
			if(command_mode == 5 || command_mode == 6)
				drawSprite(&selector, 1);

			drawSprite_2d(&command_bg, NULL);
		}

		if(dmg.display_time > 0){
			for(i = 0; i < 4; i++){
				drawSprite(&dmg.sprite[i], NULL);
				dmg.sprite[i].pos.vy -= 3;
			}
			dmg.display_time -= 2;
		}

		if(balloon.display == 0){
			for(i = 0; i < FNT_HEIGHT; i++){
				memcpy(fntBuf, fnt[i], sizeof(fntBuf));
				FntPrint(font_id[0], fntBuf);
				FntPrint(font_id[0], "\n");
			}
		}

		if(balloon.display == 1){
			drawFont(balloon.text, &balloon.font, balloon.sprite.pos.vx + 10, balloon.sprite.pos.vy + 10);
			drawSprite_2d(&balloon.sprite, NULL);
		}

		// ===================
		//     END DRAW UI	
		// ===================	
	}
	else{
		FntPrint("	Thank you for playing this demo\n\n");
		FntPrint("	please follow me on YouTube\n\n\n");
		FntPrint("		more to come...");
	}
}

void commands(u_long pad, u_long opad, Sprite *player) {
	int i = 0;
	if(command_attack == 0)
	{
		if(command_mode == 0 && atb[0].bar.w < 50){
			//atb[0].w += 0.05;
			atb[0].value += 1.0;
			atb[0].bar.w = (int)atb[0].value;
		}

		if(command_mode == CMODE_LEFT || command_mode == CMODE_RIGHT || 
			command_mode == CMODE_LEFT_ATTACK || command_mode == CMODE_RIGHT_ATTACK)
		{

			if(command_mode == CMODE_LEFT || command_mode == CMODE_LEFT_ATTACK){
				if(camera.rot.vy < 900){
					camera.pos.vz -= 32;
					camera.rot.vy += 16;
				}
				if((camera.pos.vx*-1) < player->pos.vx + 2300)
					camera.pos.vx -= 40;
			}
			if(command_mode == CMODE_RIGHT || command_mode == CMODE_RIGHT_ATTACK){
				if(camera.rot.vy > -900){
					camera.pos.vz -= 32;
					camera.rot.vy -= 16;
				}
				if((camera.pos.vx*-1) > player->pos.vx - 2300)
					camera.pos.vx += 40;
			}
		}

		if(command_mode == CMODE_LEFT || command_mode == CMODE_RIGHT) 
		{
			if(pad & PADLup && (opad & PADLup) == 0){
				if(command_index > 0)
					command_index--;
			}
			if(pad & PADLdown && (opad & PADLdown) == 0){
				if(command_index < 3)
					command_index++;
			}
			if(pad & PADLcross && (opad & PADLcross) == 0){
				reset_targets();
				if(command_mode == CMODE_LEFT)
					command_mode = CMODE_LEFT_ATTACK;
				if(command_mode == CMODE_RIGHT)
					command_mode = CMODE_RIGHT_ATTACK;
				command_index = 0;
			}
			if(pad & PADLcircle && (opad & PADLcircle) == 0){
				closeCommandMenu();
			}

			selector.pos.vy = SELECTOR_POSY+(17*command_index);
		}

		if(command_mode == CMODE_FROM_LEFT)
		{
			if(camera.rot.vy > 0){
				camera.pos.vz += 32;
				camera.rot.vy -= 16;
			}
			if(camera.pos.vx < camera.ox)
				camera.pos.vx += 40;

			if(camera.rot.vy <= 0 && camera.pos.vx >= camera.ox){
				camera.rot.vy = 0;
				camera.pos.vz = 2300;
				camera.pos.vx = camera.ox;
				command_mode = 0;
			}
		}
		if(command_mode == CMODE_FROM_RIGHT)
		{
			if(camera.rot.vy < 0){
				camera.pos.vz += 32;
				camera.rot.vy += 16;
			}
			if(camera.pos.vx > camera.ox)
				camera.pos.vx -= 40;

			if(camera.rot.vy >= 0 && camera.pos.vx <= camera.ox){
				camera.rot.vy = 0;
				camera.pos.vz = 2300;
				camera.pos.vx = camera.ox;
				command_mode = 0;
			}
		}
	}

	if(command_attack == 1)
	{
		short status = 1;
		//player->frameInterval = 10;
		status = sprite_anim(player, 41, 46, 2, 0, 6);
		if(status == 0){
			Enemy *enemy = enemy_get(targets[target]);
			sprite_set_uv(player, 0, 46, 41, 46);
			player->hp += 1; 

			enemy->sprite.hp -= 8;	
			enemy->sprite.hitted = 1;	
			enemy->blood.pos.vx = enemy->sprite.pos.vx;
			enemy->blood.pos.vy = enemy->sprite.pos.vy;
			enemy->blood.pos.vz = enemy->sprite.pos.vz-5;
			enemy->blood.frame = 0;
			
			display_dmg(&dmg, enemy->sprite, 8);

			mainCommandMenu();
			closeCommandMenu();
			command_attack = 2;
		}
	}

	if(command_attack == 2 && dmg.display_time <= 0)
		command_attack = 0;

	// select enemy logic
	if(command_attack == 0 && (command_mode == CMODE_LEFT_ATTACK || command_mode == CMODE_RIGHT_ATTACK))
	{
		if(pad & PADLcross && (opad & PADLcross) == 0 && target_counter > 0)
		{
			atb[0].value = 0;
			atb[0].bar.w = 0;
			command_attack = 1;
			return;
		}

		if(pad & PADLcircle)
			mainCommandMenu();
		else
		{		
			EnemyNode *enemy_node = enemyNode;
			selector.w = 60;
			selector.h = 60;
			
			if(calc_targets == 0){
				calc_targets = 1;
				while(enemy_node != NULL){
					Enemy *enemy = enemy_node->enemy;
					if(command_mode == CMODE_LEFT_ATTACK && player->direction == 0 && enemy->sprite.pos.vx <= player->pos.vx &&
						enemy->sprite.hp > 0){
						targets[target_counter] = i;	
						//printf("left t %d \n", targets[target_counter]);
						target_counter++;
					}
					if(command_mode == CMODE_RIGHT_ATTACK && player->direction == 1 && enemy->sprite.pos.vx >= player->pos.vx &&
						enemy->sprite.hp > 0){
						targets[target_counter] = i;	
						//printf("right t %d \n", targets[target_counter]);
						target_counter++;
					}
					i++;
					enemy_node = enemy_node->next;
				}
				//printf("target %d \n", targets[0]);
				//printf("target_counter %d \n", target_counter);
			}
			if(target_counter > 0)
			{
				Enemy *enemy;
				if(pad & PADLleft && (opad & PADLleft) == 0)
				{
					if(target == 0)
						target = target_counter-1;
					else
						target--;
				}
				if(pad & PADLright && (opad & PADLright) == 0)
				{
					target++;
					if(target >= target_counter)
						target = 0;
				}

				enemy = enemy_get(targets[target]);
				if(enemy != NULL){
					selector.pos.vx = enemy->sprite.pos.vx;
					selector.pos.vy = enemy->sprite.pos.vy;
					if(command_mode == CMODE_LEFT_ATTACK)
						selector.pos.vz = enemy->sprite.pos.vz - (enemy->sprite.w + 10);
					if(command_mode == CMODE_RIGHT_ATTACK)
						selector.pos.vz = enemy->sprite.pos.vz + (enemy->sprite.w + 10);
				}
			}
			else
			mainCommandMenu();
		}
	}

}

void player_input(Sprite *player, u_long pad, u_long opad, u_char player_type)
{
	if(player->hp > 0 && player->hitted == 0)
	{
		if(player->hittable > 0)
			player->hittable -= 1;

		if(level_clear == 0){
		// pad TRIANGLE 
		if(opad == 0 && pad & 16){
			xaChannel = (xaChannel+1)%NUMCHANNELS;
			xa_play_channel(xaChannel);
		}

		// STAND 
		if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
		(pad & PADLleft) == 0 && (pad & PADLright) == 0 && (pad & 64) == 0 && player->shooting == 0 && player->pos.vy >= 0)
			sprite_set_uv(player, 0, 46, 41, 46);

		if(player->shooting == 0){
			// UP
			if(pad & PADLup && player->pos.vz < TOP_Z){
				player->pos.vz += SPEED;
				//if(player_type == 1)
				//	camera.pos.vz -= SPEED;
				if ((pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->pos.vy >= 0)
					sprite_anim(player, 41, 46, 0, 0, 6);
			}
			// DOWN
			if(pad & PADLdown && player->pos.vz > BOTTOM_Z){
				player->pos.vz -= SPEED;
				//if(player_type == 1)
				//	camera.pos.vz += SPEED;
				if ((pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->pos.vy >= 0)
						sprite_anim(player, 41, 46, 0, 0, 6);
			}
			// LEFT
			if(pad & PADLleft && (pad & PADLright) == 0){
				if(player->pos.vx > -490 && player->pos.vx > cameraLeft(camera.pos.vx))
					player->pos.vx -= SPEED;
				if(player->pos.vy >= 0)
					sprite_anim(player, 41, 46, 0, 0, 6);
				player->direction = 0;
			}
			// RIGHT
			if(pad & PADLright && (pad & PADLleft) == 0){
				player->pos.vx += SPEED;
				if(player->pos.vx > cameraRight(camera.pos.vx) && cameraLock == 1)
					player->pos.vx -= SPEED;
				if(player_type == 2 && player->pos.vx > cameraRight(camera.pos.vx))
					player->pos.vx -= SPEED;
				if(player->pos.vy >= 0)
					sprite_anim(player, 41, 46, 0, 0, 6);
				player->direction = 1;
			}
			// JUMP
			if ((opad & PADLsquare) == 0 && pad & PADLsquare && player->pos.vy >= 0 && player->pos.vy > -MAX_JUMP_HEIGHT){
				player->isJumping = 1;
				player->jump_speed = JUMP_SPEED;
			}
			if (player->isJumping == 1){
				player->pos.vy -= player->jump_speed;
				player->jump_speed *= JUMP_FRICTION;
			}
			if (player->pos.vy < 0){
				player->pos.vy += GRAVITY;
				sprite_anim(player, 41, 46, 1, 1, 1);
				if(pad & PADLleft && player->pos.vx > -490 && player->pos.vx > cameraLeft(camera.pos.vx))
					player->pos.vx -= SPEED;
				if(pad & PADLright)
					player->pos.vx += SPEED;
				if(player->pos.vx > cameraRight(camera.pos.vx) && cameraLock == 1)
					player->pos.vx -= SPEED;
				if(player_type == 2 && player->pos.vx > cameraRight(camera.pos.vx))
					player->pos.vx -= SPEED;
			}
			else
				player->isJumping = 0;		
			
			if(player_type == 1 && atb[0].bar.w >= 50){
				if(camera.rot.vy == 0 && pad & PADL2){
					command_mode = CMODE_LEFT;
					camera.ox = camera.pos.vx;
				}
				if(camera.rot.vy == 0 && pad & PADR2){
					command_mode = CMODE_RIGHT;
					camera.ox = camera.pos.vx;
				}
			}

			// CAMERA
			if(player->pos.vx > (camera.pos.vx*-1)+400 && cameraLock == 0 && player_type == 1){
				camera.pos.vx -= SPEED;
				feetCounter += SPEED;
				if(player->isJumping == 1){
					camera.pos.vx -= SPEED;
					feetCounter += SPEED;
				}
			}
		}

		// can shoot only if the player is not moving && not jumping
		if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
		(pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->pos.vy >= 0){ 
			// pad X 
			if(pad & 64 && player->shooting == 0){
				player->shooting = 1;
				audio_play(SPU_0CH);
			}
		}

		if(player->shooting >= 1){
			player->shooting += 1;
			sprite_anim(player, 41, 46, 1, 2, 3);
			if(player->shooting > (1+5)*3){
				Enemy *e;
				player->shooting = 0;
				e = ray_collisions(player, camera.pos.vx);
				if(e != NULL)
					display_dmg(&dmg, e->sprite, 1);
			}
		}
		} // level_clear == 0 
		else if(level_clear == 1){
			player->pos.vx += SPEED*2;
			player->direction = 1;
			sprite_anim(player, 41, 46, 0, 0, 6);
			if(player->pos.vx >= (camera.pos.vx*-1)+3000)
				level_clear = 2;
		}
	}
	else // on player hitted
	{
		if(player->hp > 0)
		{
			if(player->direction == 0){
				if(sprite_anim_static(player, 41, 46, 1, 5, 5) == 0){
					player->hitted = 0;
					player->hittable = 50;
				}
				else{
					if(player->pos.vx < cameraRight(camera.pos.vx))
						player->pos.vx += 5;
				}
			}
			if(player->direction == 1){
				if(sprite_anim_static(player, 41, 46, 1, 5, 5) == 0){
					player->hitted = 0;
					player->hittable = 50;
				}
				else{
					if(player->pos.vx > cameraLeft(camera.pos.vx))
						player->pos.vx -= 5;
				}
			}
		}
		else{
			sprite_anim_static(player, 41, 46, 3, 3, 5);
		}
	}
}

void waves_controller(){
	if(cameraLock == 0){
		if(feetCounter >= 1500){
			feetCounter = 0;
			if(wave_index < n_waves){
				int k;
				cameraLock = 1;
				for(k = 0; k < waves[wave_index].total; k++){
					int i;
					for(i = 0; i < waves[wave_index].mobs[k].total; i++){
						Mob mobs = waves[wave_index].mobs[k];
						u_char type = mobs.type;
						EnemyNode *node = enemyNode;
						while(node != NULL){
							if(node->enemy->type == type && node->enemy->sprite.hp <= 0){
								enemy_spawn(node->enemy, camera.pos.vx, TOP_Z, BOTTOM_Z);
								break;
							}
							node = node->next;
						}
					}
				}
			}
			else {
				level_clear = 1;
			}
		}
	}
	else{
		u_char clear = 1;
		EnemyNode *node = enemyNode;
		while(node != NULL)
		{
			if(node->enemy->sprite.hp > 0)
				clear = 0;
			node = node->next;
		}
		if(clear == 1){
			cameraLock = 0;
			wave_index++;
		}
	}
}
