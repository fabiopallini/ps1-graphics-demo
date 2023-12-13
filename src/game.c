#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"

#define YT 0 
#define SPEED 6 
#define BACKGROUND_BLOCK 500 
#define TOP_Z 700 
#define BOTTOM_Z -280 
#define GRAVITY 10 
#define JUMP_SPEED 45 
#define JUMP_FRICTION 0.9 
#define MAX_JUMP_HEIGHT 500 
#define BLOCKS 6 
#define UNIC_ENEMIES 2 
//#define N_ENEMIES BLOCKS * UNIC_ENEMIES 
#define N_ENEMIES 6 

#define FNT_HEIGHT 29 
#define FNT_WIDTH 39 

long cameraX = 0;
long cameraZ = 2300; 
long cameraY = 900;

u_long *cd_data[9];
Mesh cube, map[4];
short mapIndex = 0;
Sprite player, player_icon, player2, player2_icon, cloud, energy_bar[2];
Enemy enemies[N_ENEMIES];
int xaChannel = 0;

char fntBuf[FNT_HEIGHT];
char fnt[FNT_HEIGHT][FNT_WIDTH];

void player_input(Sprite *player, u_long pad, u_long opad, u_char player_type);
int feetCounter;
u_char cameraLock;
void enemy_spawner();

enum ENEMY {
	BAT = 0,
	ZOMBIE = 5
};
u_char blocks[][BLOCKS * UNIC_ENEMIES] = {
	{
		2,3,3,3,3,3,
		1,0,1,2,3,3,

		//2,3,3, 
		//0,1,2,
	},
	//{3,2,1, 0,0,1},
};
u_char block_index = 0;
u_char level_clear = 0;
u_char stage = 0;

void game_load(){
	int i;

	cd_open();
	cd_read_file("GUNSHOT.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("MAP.OBJ", &cd_data[2]);
	cd_read_file("GROUND.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("BOX.TIM", &cd_data[5]);
	cd_read_file("PLAYER1.TIM", &cd_data[6]);
	cd_read_file("BAT.TIM", &cd_data[7]);
	cd_read_file("PLAYER2.TIM", &cd_data[8]);

	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], 15200, SPU_0CH);
	
	mesh_init(&map[0], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&map[1], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&map[2], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&map[3], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);

	map[0].posX = 0;
	map[1].posX = BACKGROUND_BLOCK*2;
	map[2].posX = BACKGROUND_BLOCK*4;
	map[3].posX = BACKGROUND_BLOCK*6;

	mesh_init(&cube, (u_char*)cd_data[4], (u_char*)cd_data[5], 32, 50);

	cube.posX -= 350;

	sprite_init(&player, 41*2, 46*2, (u_char *)cd_data[6]);
	sprite_setuv(&player, 0, 0, 41, 46);
	player.hp = 10;
	player.hp_max = 10;
	player.direction = 1;
	player.posX = 250;

	sprite_init(&player2, 41*2, 46*2, (u_char *)cd_data[8]);
	sprite_setuv(&player2, 0, 0, 41, 46);
	player2.hp = 10;
	player2.hp_max = 10;
	player2.direction = 1;

	#if YT == 1
	sprite_init(&player_icon, 41, 70, (u_char *)cd_data[6]);
	sprite_setuv(&player_icon, 0, 46*4, 41, 70);
	player_icon.posX = 1;
	player_icon.posY = 8;

	sprite_init(&player2_icon, 51, 70, (u_char *)cd_data[8]);
	sprite_setuv(&player2_icon, 0, 46*4, 51, 70);
	player2_icon.posX = SCREEN_WIDTH - (51+1);
	player2_icon.posY = 8;
	#endif

	sprite_init(&cloud, 60, 128, (u_char *)cd_data[1]);
	sprite_setuv(&cloud, 0, 0, 60, 128);
	cloud.posX -= 150;
	cloud.posZ = 250;

	sprite_init_rgb(&energy_bar[0], 70, 10);
	sprite_init_rgb(&energy_bar[1], 70, 10);
	energy_bar[0].posY = 3;
	energy_bar[1].posY = 3;
	energy_bar[1].posX = SCREEN_WIDTH-energy_bar[1].w;

	for(i = 0; i < N_ENEMIES; i++){
		if(i < 3)
			enemy_load(&enemies[i], (u_char *)cd_data[7], 0);
		else
			enemy_load(&enemies[i], (u_char *)cd_data[7], 1);
	}

	//xa_play();
	//free3(cd_data);
}

void game_update()
{
	int i;
	psCamera(cameraX, cameraY, cameraZ, 200, 0, 0);
	//printf("pad %ld \n", pad);
	//printf("y %ld \n", player.posY);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
	
	player_input(&player, pad, opad, 1);
	player_input(&player2, pad >> 16, opad >> 16, 2);

	// background loop
	if(level_clear == 0 && player.posX > map[mapIndex].posX + 2000){
		map[mapIndex].posX += (BACKGROUND_BLOCK*8); 
		mapIndex = (mapIndex +1) % 4;
	}

	cube.angX += 1;
	cube.angY += 16;
	cube.angZ += 16;

	for(i = 0; i < N_ENEMIES; i++){
		enemy_update(&enemies[i], player, cameraX, TOP_Z, BOTTOM_Z);
		if(sprite_collision(&player, &enemies[i].sprite) == 1 && player.hittable <= 0 && player.hitted == 0 && player.hp > 0 && enemies[i].sprite.hp > 0){
			player.hp -= 1;
			energy_bar[0].w = ((player.hp * 70) / player.hp_max); 
			player.hitted = 1;
			#if YT == 1
			sprite_setuv(&player_icon, 41, 46*4, 50, 70);
			#endif
		}
		if(sprite_collision(&player2, &enemies[i].sprite) == 1 && player2.hittable <= 0 && player2.hitted == 0 
		&& player2.hp > 0 && enemies[i].sprite.hp > 0){
			player2.hp -= 1;
			energy_bar[1].w = ((player2.hp * 70) / player2.hp_max); 
			player2.hitted = 1;
			#if YT == 1
			sprite_setuv(&player2_icon, 82, 46*4, 44, 70);
			#endif
		}
	}
	enemy_spawner();
}

void game_draw(){
	if(level_clear != 2){
		short i = 0;
		mesh_draw(&cube, 1);
		for(i = 0; i <= 3; i++)
			mesh_draw_ot(&map[i], 0, 1023);

		sprite_draw(&player);
		sprite_draw(&player2);
		sprite_draw(&cloud);

		for(i = 0; i < N_ENEMIES; i++)
			enemy_draw(&enemies[i]);

		strcpy(fnt[0], "Player1						Player 2");
		//strcpy(fnt[1], "hello");
		
		for(i = 0; i < FNT_HEIGHT; i++){
			memcpy(fntBuf, fnt[i], sizeof(fntBuf));
			FntPrint(fntBuf);
			FntPrint("\n");
		}

		sprite_draw_2d_rgb(&energy_bar[0]);
		sprite_draw_2d_rgb(&energy_bar[1]);
		#if YT == 1
		sprite_draw_2d(&player_icon);
		sprite_draw_2d(&player2_icon);
		#endif

		//FntPrint("posX %d \n", player.posX);
		//FntPrint("cameraY %d \n", cameraY);
	}
	else{
		FntPrint("\n\n\n\n\n\n\n");
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
		(pad & PADLleft) == 0 && (pad & PADLright) == 0 && (pad & 64) == 0 && player->shooting == 0 && player->posY >= 0){
			sprite_setuv(player, 0, 46*2, 41, 46);
			if(player->direction == 0)
				sprite_setuv(player, 41, 46*2, 41, 46);
			if(player->direction == 1)
				sprite_setuv(player, 0, 46*2, 41, 46);
		}

		if(player->shooting == 0){
			// UP
			if(pad & PADLup && player->posZ < TOP_Z){
				player->posZ += SPEED;
				//if(player_type == 1)
				//	cameraZ -= SPEED;
				if ((pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->posY >= 0){
					if(player->direction == 0)
						sprite_anim(player, 41, 46, 1, 0, 6);
					if(player->direction == 1)
						sprite_anim(player, 41, 46, 0, 0, 6);
				}
			}
			// DOWN
			if(pad & PADLdown && player->posZ > BOTTOM_Z){
				player->posZ -= SPEED;
				//if(player_type == 1)
				//	cameraZ += SPEED;
				if ((pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->posY >= 0){
					if(player->direction == 0)
						sprite_anim(player, 41, 46, 1, 0, 6);
					if(player->direction == 1)
						sprite_anim(player, 41, 46, 0, 0, 6);
				}
			}
			// LEFT
			if(pad & PADLleft && (pad & PADLright) == 0){
				if(player->posX > -490 && player->posX > cameraLeft(cameraX))
					player->posX -= SPEED;
				if(player->posY >= 0)
					sprite_anim(player, 41, 46, 1, 0, 6);
				player->direction = 0;
			}
			// RIGHT
			if(pad & PADLright && (pad & PADLleft) == 0){
				player->posX += SPEED;
				if(player->posX > cameraRight(cameraX) && cameraLock == 1)
					player->posX -= SPEED;
				if(player_type == 2 && player->posX > cameraRight(cameraX))
					player->posX -= SPEED;
				if(player->posY >= 0)
					sprite_anim(player, 41, 46, 0, 0, 6);
				player->direction = 1;
			}
			// JUMP
			if ((opad & PADLsquare) == 0 && pad & PADLsquare && player->posY >= 0 && player->posY > -MAX_JUMP_HEIGHT){
				player->isJumping = 1;
				player->jump_speed = JUMP_SPEED;
			}
			if (player->isJumping == 1){
				player->posY -= player->jump_speed;
				player->jump_speed *= JUMP_FRICTION;
			}
			if (player->posY < 0){
				player->posY += GRAVITY;
				if(player->direction == 1)
					sprite_anim(player, 41, 46, 3, 4, 1);
				if(player->direction == 0)
					sprite_anim(player, 41, 46, 3, 5, 1);
				if(pad & PADLleft && player->posX > -490 && player->posX > cameraLeft(cameraX))
					player->posX -= SPEED;
				if(pad & PADLright)
					player->posX += SPEED;
				if(player->posX > cameraRight(cameraX) && cameraLock == 1)
					player->posX -= SPEED;
				if(player_type == 2 && player->posX > cameraRight(cameraX))
					player->posX -= SPEED;
			}
			else
				player->isJumping = 0;		
			
			// L1
			/*if(pad & PADL1){
				cameraZ += 5;
				cameraY += 3;
			}
			// R1
			if(pad & PADR1){
				cameraZ -= 5;
				cameraY -= 3;
			}*/

			// CAMERA
			if(player->posX > (cameraX*-1)+400 && cameraLock == 0 && player_type == 1){
				cameraX -= SPEED;
				feetCounter += SPEED;
				if(player->isJumping == 1){
					cameraX -= SPEED;
					feetCounter += SPEED;
				}
			}
		}

		// can shoot only if the player is not moving && not jumping
		if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
		(pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->posY >= 0){ 
			// pad X 
			if(pad & 64 && player->shooting == 0){
				player->shooting = 1;
				audio_play(SPU_0CH);
			}
		}

		if(player->shooting >= 1){
			player->shooting += 1;
			if(player->direction == 0)
				sprite_anim(player, 41, 46, 3, 0, 3);
			if(player->direction == 1)
				sprite_anim(player, 41, 46, 2, 2, 3);
			if(player->shooting > (1+5)*3){
				player->shooting = 0;
				ray_collisions(player, enemies, N_ENEMIES, cameraX);
				//if(ray_collisions(player, enemies, N_ENEMIES, cameraX))
					//return;
			}
		}
		} // level_clear == 0 
		else if(level_clear == 1){
			player->posX += SPEED*2;
			player->direction = 1;
			sprite_anim(player, 41, 46, 0, 0, 6);
			if(player->posX >= (cameraX*-1)+3000)
				level_clear = 2;
		}
	}
	else 
	{
		if(player->hp > 0)
		{
			if(player->direction == 0){
				if(sprite_anim_static(player, 41, 46, 3, 3, 5) == 0){
					player->hitted = 0;
					player->hittable = 50;
					#if YT == 1
					if(player_type == 1)
						sprite_setuv(&player_icon, 0, 46*4, 41, 70);
					if(player_type == 2)
						sprite_setuv(&player2_icon, 0, 46*4, 51, 70);
					#endif

				}
				else{
					if(player->posX < cameraRight(cameraX))
						player->posX += 5;
				}
			}
			if(player->direction == 1){
				if(sprite_anim_static(player, 41, 46, 2, 5, 5) == 0){
					player->hitted = 0;
					player->hittable = 50;
					#if YT == 1
					if(player_type == 1)
						sprite_setuv(&player_icon, 0, 46*4, 41, 70);
					if(player_type == 2)
						sprite_setuv(&player2_icon, 0, 46*4, 51, 70);
					#endif
				}
				else{
					if(player->posX > cameraLeft(cameraX))
						player->posX -= 5;
				}
			}
		}
		else{
			if(player->direction == 0)
				sprite_anim_static(player, 41, 46, 3, 3, 5);
			if(player->direction == 1)
				sprite_anim_static(player, 41, 46, 2, 5, 5);
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
						enemy_pop(&enemies[i+(3*u)], cameraX, TOP_Z, BOTTOM_Z);
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
