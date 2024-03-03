#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"
#include "ui.h"

#define SPEED 6 
#define MAP_BLOCKS 8 
#define BACKGROUND_BLOCK 500 
#define BACKGROUND_MARGIN 5 
#define TOP_Z 700 
#define BOTTOM_Z -280 
#define GRAVITY 10 
#define JUMP_SPEED 45 
#define JUMP_FRICTION 0.9 
#define MAX_JUMP_HEIGHT 500 
#define BLOCKS 8 
#define UNIC_ENEMIES 2 
#define N_ENEMIES UNIC_ENEMIES * 3

u_long *cd_data[8];
u_short tpages[4];
Mesh cube, map[MAP_BLOCKS];
short mapIndex = 0;
Sprite player, cloud;
Enemy enemies[N_ENEMIES];
int xaChannel = 0;

char fntBuf[FNT_HEIGHT];
char fnt[FNT_HEIGHT][FNT_WIDTH];

void player_input(Sprite *player, u_long pad, u_long opad, u_char player_type);
int feetCounter;
u_char cameraLock;
void enemy_spawner();

u_char blocks[][BLOCKS * UNIC_ENEMIES] = {
	{
		//2,2,
		//0,1,
		2,2,3,2,3,2,3,3,
		0,0,0,1,1,2,2,3,
	},
};
u_char block_index = 0;
u_char level_clear = 0;
u_char stage = 0;

void start_level(){
	int i;

	player.hp = 10;
	player.hp_max = 10;
	player.pos.vx = 250;

	camera.pos.vx = 0;
	cameraLock = 0;
	block_index = 0;
	feetCounter = 0;

	mapIndex = 0;
	for(i = 0; i < MAP_BLOCKS; i++)
		map[i].pos.vx = (BACKGROUND_BLOCK*i)-BACKGROUND_MARGIN;

	for(i = 0; i < N_ENEMIES; i++)
		enemies[i].sprite.hp = 0;
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
	cd_read_file("PLAYER1.TIM", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);

	cd_read_file("BAT.TIM", &cd_data[2]);
	cd_read_file("MISC_1.TIM", &cd_data[3]); // 640 256

	cd_read_file("GROUND.OBJ", &cd_data[4]);
	cd_read_file("GROUND.TIM", &cd_data[5]);

	cd_read_file("CUBE.OBJ", &cd_data[6]);
	cd_read_file("BOX.TIM", &cd_data[7]);

	cd_read_file("GUNSHOT.VAG", &cd_data[8]);
	cd_close();

	tpages[0] = loadToVRAM(cd_data[0]);
	tpages[1] = loadToVRAM(cd_data[1]);
	tpages[2] = loadToVRAM(cd_data[2]);
	tpages[3] = loadToVRAM(cd_data[3]);

	audio_init();
	audio_vag_to_spu((u_char*)cd_data[8], 15200, SPU_0CH);
	
	for(i = 0; i < MAP_BLOCKS; i++)
		mesh_init(&map[i], cd_data[4], cd_data[5], 128, BACKGROUND_BLOCK);

	mesh_init(&cube, cd_data[6], cd_data[7], 32, 50);
	cube.pos.vx -= 350;

	sprite_init(&player, 41*2, 46*2, tpages[0]);
	sprite_set_uv(&player, 0, 0, 41, 46);

	sprite_init(&cloud, 60, 128, tpages[1]);
	sprite_set_uv(&cloud, 0, 0, 60, 128);
	cloud.pos.vx -= 150;
	cloud.pos.vz = 250;

	scene_add_sprite(&player);
	scene_add_sprite(&cloud);

	for(i = 0; i < N_ENEMIES; i++){
		if(i < 3)
			enemy_load(&enemies[i], tpages[2], BAT);
		if(i >= 3 && i < 6)
			enemy_load(&enemies[i], tpages[2], BAT_GREEN);
		scene_add_sprite(&enemies[i].sprite);
	}

	ui_init(tpages[3], SCREEN_WIDTH, SCREEN_HEIGHT);
	scene_add_sprite(&selector);
	scene_add_sprite(&dmg.sprite[0]);
	scene_add_sprite(&dmg.sprite[1]);
	scene_add_sprite(&dmg.sprite[2]);
	scene_add_sprite(&dmg.sprite[3]);
	start_level();

	init_balloon(&balloon, tpages[3], SCREEN_WIDTH, SCREEN_HEIGHT);
	
	xa_play();
	//free3(cd_data);
}

void game_update()
{
	int i;
	//printf("pad %ld \n", pad);
	//printf("y %ld \n", player.pos.vy);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
		
	if(balloon.display == 1)
	{
		if((opad & PADLcross) == 0 && pad & PADLcross)
			balloon.prev_display = 1;
		if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1)
			balloon.display = 0;
		return;
	}

	if(player.hp <= 0){
		start_level();
	}

	ui_update(pad, opad, &player, &camera, enemies);
	ui_enemies_selector(pad, opad, player, N_ENEMIES, enemies, camera);

	if((command_mode == 0 || command_mode == CMODE_FROM_LEFT || command_mode == CMODE_FROM_RIGHT) && command_attack == 0)
	{
		if(command_mode == 0)
		{
			if((opad & PADLcross) == 0 && pad & PADLcross){
				if(balloon_collision(&cloud, &player)){
					set_balloon(&balloon, "not interested");
					return;
				}
			}
			player_input(&player, pad, opad, 1);
		}

		// background loop
		if(level_clear == 0 && player.pos.vx > map[mapIndex].pos.vx + 3000){
			map[mapIndex].pos.vx += (BACKGROUND_BLOCK*9)-BACKGROUND_MARGIN; 
			mapIndex++;
			if(mapIndex == MAP_BLOCKS)
				mapIndex = 0;
		}

		cube.rot.vx += 1;
		cube.rot.vy += 16;
		cube.rot.vz += 16;

		for(i = 0; i < N_ENEMIES; i++){
			int k;
			enemy_update(&enemies[i], player, camera.pos.vx, TOP_Z, BOTTOM_Z);

			if(sprite_collision(&player, &enemies[i].sprite) == 1 && player.hittable <= 0 &&
			player.hitted == 0 && player.hp > 0 && enemies[i].sprite.hp > 0){
				player.hp -= 1;
				player.hitted = 1;
			}

			for(k = 0; k < N_ENEMIES; k++){
				if(i != k && sprite_collision(&enemies[i].sprite, &enemies[k].sprite) == 1 
				&& enemies[i].sprite.hp > 0 && enemies[k].sprite.hp > 0 && enemies[i].speed <= enemies[k].speed){
					if(enemies[i].sprite.pos.vx < enemies[k].sprite.pos.vx)
						enemies[i].sprite.pos.vx -= 32;
					if(enemies[i].sprite.pos.vx > enemies[k].sprite.pos.vx)
						enemies[i].sprite.pos.vx += 32;
					if(enemies[i].sprite.pos.vz < enemies[k].sprite.pos.vz)
						enemies[i].sprite.pos.vz -= 32;
					if(enemies[i].sprite.pos.vz > enemies[k].sprite.pos.vz)
						enemies[i].sprite.pos.vz += 32;
				}
			}
		}
		enemy_spawner();
	} // command_mode == 0
}

void game_draw(){
	if(level_clear != 2){
		char log[20];
		char str[39];
		short i = 0;
		mesh_draw(&cube, 1);
		for(i = 0; i < MAP_BLOCKS; i++)
			mesh_draw_ot(&map[i], 0, 1023);

		drawSprite(&player);
		drawSprite(&cloud);

		for(i = 0; i < N_ENEMIES; i++){
			if(enemies[i].sprite.hp > 0)
				drawSprite(&enemies[i].sprite);
			if(enemies[i].sprite.hitted == 1)
				drawSprite(&enemies[i].blood);
		}

		sprintf(log, "camera.pos.vx %ld %ld", camera.pos.vx, player.pos.vx);
		strcpy(fnt[1], log);	
		//sprintf(log, "camera.rot.vy %d", camera.rot.vy);
		//strcpy(fnt[2], log);
		
		// ===================
		// 	DRAW UI
		// ===================
		
		sprintf(str, "					Player1 %d", player.hp);
		strcpy(fnt[23], str);

		drawSprite_2d_rgb(&atb[0].bar);
		drawSprite_2d_rgb(&atb[0].border);
		
		if(command_mode > 0 && atb[0].bar.w >= 50){
			FntPrint(font_id[1], "Super Shot \n\n");
			FntPrint(font_id[1], "Magic \n\n");
			FntPrint(font_id[1], "GF \n\n");
			FntPrint(font_id[1], "Items \n\n");

			if(command_mode == 1 || command_mode == 2)
				drawSprite_2d(&selector);
			if(command_mode == 5 || command_mode == 6)
				drawSprite(&selector);

			drawSprite_2d_rgb(&command_bg);
		}

		if(dmg.display_time > 0){
			for(i = 0; i < 4; i++){
				drawSprite(&dmg.sprite[i]);
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
			drawSprite_2d(&balloon.sprite);
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
				e = ray_collisions(player, enemies, N_ENEMIES, camera.pos.vx);
				if(e != NULL)
					display_dmg(&dmg, e->sprite, 1);
				//if(ray_collisions(player, enemies, N_ENEMIES, camera.pos.vx))
					//return;
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

void enemy_spawner(){
	if(cameraLock == 0){
		if(feetCounter >= 1500){
			feetCounter = 0;
			if(block_index < BLOCKS){
				int i,u;
				cameraLock = 1;
				for(u = 0; u < UNIC_ENEMIES; u++)
				{
					//printf("pop N enemy %d\n", blocks[stage][block_index+(BLOCKS*u)]);
					for(i = 0; i < blocks[stage][block_index+(BLOCKS*u)]; i++){
						enemy_pop(&enemies[i+(3*u)], camera.pos.vx, TOP_Z, BOTTOM_Z);
						//printf("enemy index %d\n", i+(3*u));
					}
				}
			}
			else {
				// level clear
				level_clear = 1;
			}
		}
	}
	else{
		int i,u, clear = 1;
		for(u = 0; u < UNIC_ENEMIES; u++)
		{
			for(i = 0; i < blocks[stage][block_index+(BLOCKS*u)]; i++){
				if(enemies[i+(3*u)].sprite.hp > 0)
					clear = 0;	
			}
		}

		if(clear == 1){
			cameraLock = 0;
			block_index++;
		}
	}
}
