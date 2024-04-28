#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"
#include "ui.h"

#define DEBUG
u_char CAMERA_DEBUG = 0;

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

u_long *cd_data[9];
u_short tpages[5];
Mesh cube, map[MAP_BLOCKS];
Mesh mesh_player;
short mapIndex = 0;
Sprite player, cloud;
unsigned int mapId = 0;
u_char mapChanged = 0;
Sprite background;
int xaChannel = 0;

int feetCounter;
u_char cameraLock;
void camera_debug_input();
void zoneTo(int id, int dataIndex, long camX, long camY, long camZ, short camRX, short camRY, short camRZ, long posX, long posY, long posZ);
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

Mesh plane1, plane2;

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
	long plane_pos[] = {0, 0, 0};
	short plane_size[] = {160, 0, -2000};

	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;
	camera.ox = 0;

	cd_open();
	cd_read_file("MISC_1.TIM", &cd_data[0]); // 640 256
	cd_read_file("BK1.TIM", &cd_data[1]);
	cd_read_file("TEX1.TIM", &cd_data[2]);
	cd_read_file("CUBE.TIM", &cd_data[3]);
	cd_read_file("BAT.TIM", &cd_data[4]);
	cd_read_file("P1.OBJ", &cd_data[5]);
	cd_read_file("CUBE.OBJ", &cd_data[6]);
	cd_read_file("GUNSHOT.VAG", &cd_data[7]);
	cd_read_file("BK2.TIM", &cd_data[8]);
	cd_close();

	tpages[0] = loadToVRAM(cd_data[0]); // MISC_1
	tpages[1] = loadToVRAM(cd_data[1]); // BK1
	tpages[2] = loadToVRAM(cd_data[2]); // TEX1
	tpages[3] = loadToVRAM(cd_data[3]); // CUBE
	tpages[4] = loadToVRAM(cd_data[4]); // BAT 

	/*free3(cd_data[0]);
	free3(cd_data[1]);
	free3(cd_data[2]);
	free3(cd_data[3]);
	free3(cd_data[5]);*/

	audio_init();
	audio_vag_to_spu((u_char*)cd_data[7], 15200, SPU_0CH);
	
	sprite_init(&background, 255, 255, tpages[1]);
	background.w = SCREEN_WIDTH;
	background.h = SCREEN_HEIGHT;

	mesh_init(&mesh_player, cd_data[5], tpages[2], 255, 300);

	mesh_init(&cube, cd_data[6], tpages[3], 32, 50);
	cube.pos.vx = -150;

	ui_init(tpages[0], SCREEN_WIDTH, SCREEN_HEIGHT);
	scene_add_sprite(&selector);
	scene_add_sprite(&dmg.sprite[0]);
	scene_add_sprite(&dmg.sprite[1]);
	scene_add_sprite(&dmg.sprite[2]);
	scene_add_sprite(&dmg.sprite[3]);

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

	planeNode_push(plane_pos, plane_size, plane1);
	zoneTo(0, 1, 
	-185, 969, 3121, 185, -31, 0, 
	100, 0, -500);
}

void game_update()
{
	if(balloon.display == 1)
	{
		if((opad & PADLcross) == 0 && pad & PADLcross)
			balloon.prev_display = 1;
		if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1)
			balloon.display = 0;
		return;
	}

#ifdef DEBUG
	if(pad & PADLtriangle && (opad & PADLtriangle) == 0)
		CAMERA_DEBUG = !CAMERA_DEBUG;
#endif
	if (CAMERA_DEBUG == 0){
		if(mapChanged == 1){
			if((pad & PADLup) == 0 &&
			(pad & PADLdown) == 0 &&
			(pad & PADLleft) == 0 &&
			(pad & PADLright) == 0){
				mapChanged = 0;
			}
		}
		// player input
		if(mapChanged == 0){
			PlaneNode *plane_node = planeNode;
			while(plane_node != NULL){
				if(pad & PADLup){
					long z = mesh_player.pos.vz + 10;
					if(mesh_on_plane(mesh_player.pos.vx, z, plane_node->data))
						mesh_player.pos.vz = z;
				}
				if(pad & PADLdown){
					long z = mesh_player.pos.vz - 10;
					if(mesh_on_plane(mesh_player.pos.vx, z, plane_node->data))
						mesh_player.pos.vz = z;
				}
				if(pad & PADLleft){
					long x = mesh_player.pos.vx - 10;
					if(mesh_on_plane(x, mesh_player.pos.vz, plane_node->data))
						mesh_player.pos.vx = x;
				}
				if(pad & PADLright){
					long x = mesh_player.pos.vx + 10;
					if(mesh_on_plane(x, mesh_player.pos.vz, plane_node->data))
						mesh_player.pos.vx = x;
				}
				plane_node = plane_node->next;
			}
		}
		if(mapId == 0 && mesh_player.pos.vz <= -1990){
			long pos[] = {0, 0, 0};
			short size[] = {230, 0, -1200};
			planeNode_free();
			planeNode_push(pos, size, plane1);
			zoneTo(1,8, 
			-461, 942, 2503, 160, 195, 0, 
			80, 0, -1000);
		}
		if(mapId == 1 && mesh_player.pos.vz <= -1190){
			long plane_pos[] = {0, 0, 0};
			short plane_size[] = {160, 0, -2000};
			planeNode_free();
			planeNode_push(plane_pos, plane_size, plane1);
			zoneTo(0, 1, 
			-185, 969, 3121, 185, -31, 0, 
			100, 0, -1900);
		}
		if(mesh_collision(mesh_player, cube) == 1){
			if(pad & PADLcross && ((opad & PADLcross) == 0)){
				set_balloon(&balloon, "uno strano cubo...");
			}
		}
	} // end CAMERA_DEBUG == 0
	else
		camera_debug_input();
	mesh_player.rot.vy += 10;
	cube.rot.vx += 10;
	cube.rot.vy += 10;
	cube.rot.vz += 10;
}

void game_draw(){
	if(level_clear != 2){
		short i = 0;
		//EnemyNode *enemy_node = enemyNode;
		PlaneNode *plane_node = planeNode;

		if(CAMERA_DEBUG == 1){
			char log[100];
			sprintf(log, "x%ld y%ld z%ld rx%d ry%d rz%d\n\nx%ld y%ld z%ld",
			camera.pos.vx, camera.pos.vy, camera.pos.vz,
			camera.rot.vx, camera.rot.vy, camera.rot.vz,
			mesh_player.pos.vx, mesh_player.pos.vy, mesh_player.pos.vz);
			FntPrint(log);
			while(plane_node != NULL){
				drawMesh(&plane_node->data, 1023);
				plane_node = plane_node->next;
			}
		}

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
	
		// ===================
		// 	DRAW UI
		// ===================

		//drawSprite_2d(&atb[0].bar, NULL);
		//drawSprite_2d(&atb[0].border, NULL);
		
		if(command_mode > 0 && atb[0].bar.w >= 50){
			FntPrint("Super Shot \n\n");
			FntPrint("Magic \n\n");
			FntPrint("GF \n\n");
			FntPrint("Items \n\n");

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

void camera_debug_input(){
	if(pad & PADLcross){
		if(pad & PADLleft)
			camera.rot.vy -= 1;
		if(pad & PADLright)
			camera.rot.vy += 1;
		if(pad & PADLup)
			camera.rot.vx += 1;
		if(pad & PADLdown)
			camera.rot.vx -= 1;
	}
	else if(pad & PADLsquare){
		if(pad & PADLup)
			camera.pos.vz += 1;
		if(pad & PADLdown)
			camera.pos.vz -= 1;
	}
	else {
		if(pad & PADLleft)
			camera.pos.vx -= 1;
		if(pad & PADLright)
			camera.pos.vx += 1;
		if(pad & PADLup)
			camera.pos.vy += 1;
		if(pad & PADLdown)
			camera.pos.vy -= 1;
	}
}

void zoneTo(int id, int dataIndex, long camX, long camY, long camZ, short camRX, short camRY, short camRZ, long posX, long posY, long posZ){
	mapChanged = 1;
	mapId = id;
	clearVRAM_at(320,0, 256, 256);
	tpages[1] = loadToVRAM(cd_data[dataIndex]);
	background.tpage = tpages[1];
	camera.pos.vx = camX;
	camera.pos.vz = camZ;
	camera.pos.vy = camY;
	camera.rot.vx = camRX;
	camera.rot.vy = camRY;
	camera.rot.vz = camRZ;
	mesh_player.pos.vx = posX;
	mesh_player.pos.vy = posY;
	mesh_player.pos.vz = posZ;
}
