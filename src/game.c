#include "game.h"
#include "xa.h"

#define BACKGROUND_BLOCK 500 
#define WALL_Z 610 
#define FALL_Z -330 

long cameraX = 0;
long cameraY = 820;
long cameraZ = 1500;

u_long *cd_data[8];
Mesh cube, plane[4];
short planeIndex = 0;
long feetCounter = 0;
Sprite player;
short player_prevDirection = 1;
int shooting = -1;
Sprite player2;
Sprite bat;
Sprite energy_bar[2];
int opad = 0;
int xaChannel = 0;
int ray_collision(Sprite *s1, Sprite *s2);

void game_load(){
	cd_open();

	cd_read_file("GUNSHOT.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	//cd_read_file("PLANE.OBJ", &cd_data[2]);
	cd_read_file("PLANE.OBJ", &cd_data[2]);
	cd_read_file("GROUND.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("BOX.TIM", &cd_data[5]);
	cd_read_file("MARCO.TIM", &cd_data[6]);
	cd_read_file("BAT.TIM", &cd_data[7]);

	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], 15200, SPU_0CH);
	
	mesh_init(&plane[0], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&plane[1], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&plane[2], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);
	mesh_init(&plane[3], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, BACKGROUND_BLOCK);

	plane[0].posX = 0;
	plane[1].posX = BACKGROUND_BLOCK*2;
	plane[2].posX = BACKGROUND_BLOCK*4;
	plane[3].posX = BACKGROUND_BLOCK*6;

	mesh_init(&cube, (u_char*)cd_data[4], (u_char*)cd_data[5], 32, 50);

	cube.posX -= 350;

	sprite_init(&player, 41*2, 46*2, (u_char *)cd_data[6]);
	sprite_setuv(&player, 0, 0, 41, 46);

	sprite_init_rgb(&energy_bar[0], 70, 10);
	sprite_init_rgb(&energy_bar[1], 70, 10);
	energy_bar[1].posX = SCREEN_WIDTH-energy_bar[1].w;

	sprite_init(&player2, 60, 128, (u_char *)cd_data[1]);
	sprite_setuv(&player2, 0, 0, 60, 128);
	player2.posX -= 150;
	player2.posZ = 250;

	sprite_init(&bat, 64, 64, (u_char *)cd_data[7]);
	sprite_setuv(&bat, 0, 0, 16, 16);
	bat.posX -= 800;
	bat.posZ = 300;

	free3(cd_data);

	xa_play();
}

void game_update()
{
	psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
	//printf("pad %ld \n", pad);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
	//controller 1 controller 2 input sample
	
	if(pad >> 16 & PADLup){
		player2.posZ += 1;	
		printf("controller 2 PADLup \n");
	}

	if(player.posX > plane[planeIndex].posX + 1200){
		plane[planeIndex].posX += (BACKGROUND_BLOCK*8); 
		planeIndex = (planeIndex +1) % 4;
	}

	// pad TRIANGLE 
	if(opad == 0 && pad & 16){
		xaChannel = (xaChannel+1)%NUMCHANNELS;
		xa_play_channel(xaChannel);
	}

	// stand pos
	if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
	(pad & PADLleft) == 0 && (pad & PADLright) == 0 && (pad & 64) == 0 && shooting == -1){
		sprite_setuv(&player, 0, 46*2, 41, 46);
		if(player_prevDirection == 0)
			sprite_setuv(&player, 41, 46*2, 41, 46);
		if(player_prevDirection == 1)
			sprite_setuv(&player, 0, 46*2, 41, 46);
	}

	if(shooting == -1){
		if(pad & PADLup && player.posZ < WALL_Z){
			player.posZ += 5;
			cameraZ -= 5;
			if ((pad & PADLleft) == 0 && (pad & PADLright) == 0){
				if(player_prevDirection == 0)
					sprite_anim(&player, 41, 46, 1, 0, 6);
				if(player_prevDirection == 1)
					sprite_anim(&player, 41, 46, 0, 0, 6);
			}
		}
		if(pad & PADLdown && player.posZ > FALL_Z){
			player.posZ -= 5;
			cameraZ += 5;
			if ((pad & PADLleft) == 0 && (pad & PADLright) == 0){
				if(player_prevDirection == 0)
					sprite_anim(&player, 41, 46, 1, 0, 6);
				if(player_prevDirection == 1)
					sprite_anim(&player, 41, 46, 0, 0, 6);
			}
		}
		if(pad & PADLleft && (pad & PADLright) == 0){
			if(player.posX > -490 && player.posX > cameraX*-1 - 500){
				feetCounter += 5;
				player.posX -= 5;
				if(feetCounter <= 500){
					cameraX += 5;
				}
			}
			sprite_anim(&player, 41, 46, 1, 0, 6);
			player_prevDirection = 0;
		}
		//if(pad & PADLright && player.posX < 500*3){
		if(pad & PADLright && (pad & PADLleft) == 0){
			player.posX += 5;
			if(feetCounter >= 500){
				feetCounter -= 5;	
			}
			else
				cameraX -= 5;
			sprite_anim(&player, 41, 46, 0, 0, 6);
			player_prevDirection = 1;
		}
	}

	// can shoot only if the player is not moving
	if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
	(pad & PADLleft) == 0 && (pad & PADLright) == 0){ 
		// pad X 
		if(pad & 64 && shooting == -1){
			shooting = 0;
			audio_play(SPU_0CH);
		}
	}

	if(shooting >= 0){
		shooting += 1;
		if(player_prevDirection == 0)
			sprite_anim(&player, 41, 46, 3, 0, 3);
		if(player_prevDirection == 1)
			sprite_anim(&player, 41, 46, 2, 2, 3);
		if(shooting > 5*3){
			shooting = -1;
			if(ray_collision(&player, &bat)){
				bat.posX = cameraX*-1 + 1000;
				bat.posZ = FALL_Z + rand()/35;
				if(bat.posZ > WALL_Z)
					bat.posZ = WALL_Z;
				if(bat.posZ < FALL_Z)
					bat.posZ = FALL_Z;
			}
			if(ray_collision(&player, &player2)){
				energy_bar[1].posX += 5;
				energy_bar[1].w -= 5;
			}
		}
	}

	cube.angX += 1;
	cube.angY += 16;
	cube.angZ += 16;

	// bat logic
	sprite_anim(&bat, 16, 16, 0, 0, 5);
	bat.posX -= 1.5f;
	if(bat.posX < player.posX - 700){
		bat.posX = cameraX*-1 + 1000;
		bat.posZ = FALL_Z + rand()/35;
		if(bat.posZ > WALL_Z)
			bat.posZ = WALL_Z;
		if(bat.posZ < FALL_Z)
			bat.posZ = FALL_Z;
	}

	opad = pad;
}

void game_draw(){
	short i = 0;
	mesh_draw(&cube, 1);
	for(i = 0; i <= 3; i++)
		mesh_draw_ot(&plane[i], 0, 1023);

	sprite_draw(&player);
	sprite_draw(&player2);
	sprite_draw(&bat);

	FntPrint("Player 1");
	FntPrint("						Player 2");
	sprite_draw_2d_rgb(&energy_bar[0]);
	sprite_draw_2d_rgb(&energy_bar[1]);
}

int ray_collision(Sprite *s1, Sprite *s2){
	if((player_prevDirection == 1 && s1->posX < s2->posX) || (player_prevDirection == 0 && s1->posX > s2->posX))
	{
		if(s1->posZ-64 <= s2->posZ+(s2->h/2) && s1->posZ+(s1->h) >= s2->posZ-(s2->h))
			return 1;
	}
	return 0;
}
