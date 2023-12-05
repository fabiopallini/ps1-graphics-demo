#include "game.h"
#include "xa.h"
#include "enemy.h"

#define YT 0 

#define SPEED 6 
#define BACKGROUND_BLOCK 500 
#define TOP_Z 610 
#define BOTTOM_Z -320 
#define GRAVITY 10 
#define JUMP_SPEED 45 
#define JUMP_FRICTION 0.9 
#define MAX_JUMP_HEIGHT 500 

long cameraX = 0;
long cameraZ = 2300; 
long cameraY = 1220;

u_long *cd_data[8];
Mesh cube, map[4];
short mapIndex = 0;
Sprite player, player_icon, player2, energy_bar[2];
Enemy enemies[3];
int opad = 0;
int xaChannel = 0;

void player_input(Sprite *player);
int ray_collisions(Sprite *s, Enemy enemies[]);
int ray_collision(Sprite *s1, Sprite *s2);
int sprite_collision(Sprite *s1, Sprite *s2);

void game_load(){
	int i;
	cd_open();

	cd_read_file("GUNSHOT.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("MAP.OBJ", &cd_data[2]);
	cd_read_file("GROUND.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("BOX.TIM", &cd_data[5]);
	cd_read_file("MARCO.TIM", &cd_data[6]);
	cd_read_file("BAT.TIM", &cd_data[7]);

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
	player.jump_speed = JUMP_SPEED;

	#if YT == 1
	sprite_init(&player_icon, 41, 70, (u_char *)cd_data[6]);
	sprite_setuv(&player_icon, 0, 46*4, 41, 70);
	player_icon.posX = 1;
	player_icon.posY = 8;
	#endif

	sprite_init_rgb(&energy_bar[0], 70, 10);
	sprite_init_rgb(&energy_bar[1], 70, 10);
	energy_bar[0].posY = 3;
	energy_bar[1].posY = 3;
	energy_bar[1].posX = SCREEN_WIDTH-energy_bar[1].w;

	sprite_init(&player2, 60, 128, (u_char *)cd_data[1]);
	sprite_setuv(&player2, 0, 0, 60, 128);
	player2.hp = 10;
	player2.posX -= 150;
	player2.posZ = 250;

	for(i = 0; i < 3; i++)
		enemy_load((u_char *)cd_data[7], &enemies[i].sprite, &enemies[i].blood);

	xa_play();
	//free3(cd_data);
}

void game_update()
{
	int i;
	psCamera(cameraX, cameraY, cameraZ, 250, 0, 0);
	//printf("pad %ld \n", pad);
	//printf("y %ld \n", player.posY);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
	
	if(pad >> 16 & PADLup){
		player2.posZ += 1;	
		printf("controller 2 PADLup \n");
	}
	
	player_input(&player);

	// background loop
	if(player.posX > map[mapIndex].posX + 2000){
		map[mapIndex].posX += (BACKGROUND_BLOCK*8); 
		mapIndex = (mapIndex +1) % 4;
	}

	cube.angX += 1;
	cube.angY += 16;
	cube.angZ += 16;

	for(i = 0; i < 3; i++){
		enemy_update(&enemies[i], player, cameraX, TOP_Z, BOTTOM_Z);
		if(sprite_collision(&player, &enemies[i].sprite) == 1 && player.hitted == 0 && player.hp > 0 && enemies[i].sprite.hp > 0){
			player.hp -= 1;
			energy_bar[0].w = ((player.hp * 70) / player.hp_max); 
			player.hitted = 1;
			#if YT == 1
			sprite_setuv(&player_icon, 41, 46*4, 50, 70);
			#endif
		}
	}

	opad = pad;
}

void game_draw(){
	short i = 0;
	mesh_draw(&cube, 1);
	for(i = 0; i <= 3; i++)
		mesh_draw_ot(&map[i], 0, 1023);

	sprite_draw(&player);
	sprite_draw(&player2);

	for(i = 0; i < 3; i++)
		enemy_draw(&enemies[i]);

	FntPrint("Player1						Player 2");
	sprite_draw_2d_rgb(&energy_bar[0]);
	sprite_draw_2d_rgb(&energy_bar[1]);
	#if YT == 1
	sprite_draw_2d(&player_icon);
	#endif

	//FntPrint("posX %d \n", player.posX);
	//FntPrint("cameraY %d \n", cameraY);
}

void player_input(Sprite *player)
{
	if(player->hp > 0 && player->hitted == 0)
	{
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
				cameraZ -= SPEED;
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
				cameraZ += SPEED;
				if ((pad & PADLleft) == 0 && (pad & PADLright) == 0 && player->posY >= 0){
					if(player->direction == 0)
						sprite_anim(player, 41, 46, 1, 0, 6);
					if(player->direction == 1)
						sprite_anim(player, 41, 46, 0, 0, 6);
				}
			}
			// LEFT
			if(pad & PADLleft && (pad & PADLright) == 0){
				if(player->posX > -490 && player->posX > cameraX*-1 - 800)
					player->posX -= SPEED;
				if(player->posY >= 0)
					sprite_anim(player, 41, 46, 1, 0, 6);
				player->direction = 0;
			}
			// RIGHT
			if(pad & PADLright && (pad & PADLleft) == 0){
				player->posX += SPEED;
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
				if(pad & PADLleft && player->posX > -490 && player->posX > cameraX*-1 - 800)
					player->posX -= SPEED;
				if(pad & PADLright)
					player->posX += SPEED;
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
			if(player->posX > (cameraX*-1)+400){
				cameraX -= SPEED;
				if(player->isJumping == 1)
					cameraX -= SPEED;
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
				if(ray_collisions(player, enemies))
					return;
				if(ray_collision(player, &player2)){
					energy_bar[1].posX += 5;
					energy_bar[1].w -= 5;
					return;
				}
			}
		}
	}
	else 
	{
		if(player->hp > 0)
		{
			if(player->direction == 0){
				if(sprite_anim_static(player, 41, 46, 3, 3, 5) == 0){
					player->hitted = 0;
					#if YT == 1
					sprite_setuv(&player_icon, 0, 46*4, 41, 70);
					#endif

				}
				else
					player->posX += 5;
			}
			if(player->direction == 1){
				if(sprite_anim_static(player, 41, 46, 2, 5, 5) == 0){
					player->hitted = 0;
					#if YT == 1
					sprite_setuv(&player_icon, 0, 46*4, 41, 70);
					#endif
				}
				else
					player->posX -= 5;
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

int ray_collisions(Sprite *s, Enemy enemies[])
{
	int i, distance = 10000, k, index;
	for(i = 0; i < 3; i++){
		int collision = ray_collision(s, &enemies[i].sprite);
		/*if(collision == 1){
			enemies[i].sprite.hitted = 1;
			enemies[i].sprite.hp -= 1;
			enemies[i].blood.posX = enemies[i].sprite.posX;
			enemies[i].blood.posY = enemies[i].sprite.posY;
			enemies[i].blood.posZ = enemies[i].sprite.posZ-5;
			enemies[i].blood.frame = 0;
			break;
		}*/
		if(collision == 1){
			index = i;
			for(k = 0; k < 3; k++){
				if(s->direction == 0){
					if((s->posX - enemies[k].sprite.posX) < distance){
						if(ray_collision(s, &enemies[k].sprite) == 1){
							distance = s->posX - enemies[k].sprite.posX;
							index = k;
						}
					}	
				}
				if(s->direction == 1){
					if((enemies[k].sprite.posX - s->posX) < distance){
						if(ray_collision(s, &enemies[k].sprite) == 1){
							distance = enemies[k].sprite.posX - s->posX;
							index = k;
						}
					}	
				}
			}
			enemies[index].sprite.hitted = 1;
			enemies[index].sprite.hp -= 1;
			enemies[index].blood.posX = enemies[index].sprite.posX;
			enemies[index].blood.posY = enemies[index].sprite.posY;
			enemies[index].blood.posZ = enemies[index].sprite.posZ-5;
			enemies[index].blood.frame = 0;
			return 1;
		}
	}
	return 0;
}

int ray_collision(Sprite *s1, Sprite *s2){
	if(s2->posX > (cameraX*-1) -850 && s2->posX < (cameraX*-1) +850) 
	{
		if((s1->direction == 1 && s1->posX < s2->posX) || (s1->direction == 0 && s1->posX > s2->posX))
		{
			if(s1->posZ+(s1->h) >= s2->posZ-(s2->h) && s1->posZ <= s2->posZ+(s2->h))
				return 1;
		}
	}
	return 0;
}

int sprite_collision(Sprite *s1, Sprite *s2){
	if(s1->posX+(s1->w/2) >= s2->posX-(s2->w/2) && s1->posX-(s1->w/2) <= s2->posX+(s2->w/2) && 
		s1->posY+(s1->h/2) >= s2->posY-(s2->h/2) && s1->posY-(s1->h/2) <= s2->posY+(s2->h/2) &&
		s1->posZ+(s1->h) >= s2->posZ-(s2->h) && s1->posZ <= s2->posZ+(s2->h) ) 
	{
		return 1;
	}
	return 0;
}
