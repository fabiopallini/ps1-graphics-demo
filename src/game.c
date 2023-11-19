#include "game.h"
#include "xa.h"

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
Sprite energy_bar_1;
int opad = 0;
int xaChannel = 0;

void game_load(){
	cd_open();

	cd_read_file("GUNSHOT.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("PLANE.OBJ", &cd_data[2]);
	cd_read_file("GROUND.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("BOX.TIM", &cd_data[5]);
	cd_read_file("MARCO.TIM", &cd_data[6]);
	cd_read_file("BAT.TIM", &cd_data[7]);

	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], 15200, SPU_0CH);
	
	mesh_init(&plane[0], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, 500);
	mesh_init(&plane[1], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, 500);
	mesh_init(&plane[2], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, 500);
	mesh_init(&plane[3], (u_char*)cd_data[2], (u_char*)cd_data[3], 128, 500);
	
	plane[1].posX += 500;
	plane[2].posX += 500*2;
	plane[3].posX += 500*3;

	mesh_init(&cube, (u_char*)cd_data[4], (u_char*)cd_data[5], 32, 50);

	cube.posX -= 350;

	sprite_init(&player, 41*2, 46*2, (u_char *)cd_data[6]);
	sprite_setuv(&player, 0, 0, 41, 46);

	sprite_init_rgb(&energy_bar_1, 16, 16);

	sprite_init(&player2, 60*0.8, 128*0.8, (u_char *)cd_data[1]);
	sprite_setuv(&player2, 0, 0, 60, 128);
	player2.posX -= 150;
	player2.posZ -= 250;

	sprite_init(&bat, 64, 64, (u_char *)cd_data[7]);
	sprite_setuv(&bat, 0, 0, 16, 16);
	bat.posX -= 800;
	bat.posZ += 100;

	free3(cd_data);

	xa_play();
}

void game_update()
{
	psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
	//printf("pad %ld \n", pad);
	//printf("%ld %d %d \n", pad >> 16, _PAD(0, PADLup),_PAD(1, PADLup));
	/* controller 1 controller 2 input sample
	if(pad & PADLup)
		printf("player 1 up \n");
	if(pad >> 16 & PADLup)
		printf("player 2 up \n");
	if(pad == _PAD(0, PADLup))
		printf("player 1 up \n");
	if(pad == _PAD(1, PADLup))
		printf("player 2 up \n");
	*/
	
	if(player.posX > plane[planeIndex].posX + 1200){
		plane[planeIndex].posX += (500*5)-5; 
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
		if(pad & PADLup && player.posZ < 610){
			player.posZ += 5;
			cameraZ -= 5;
			if ((pad & PADLleft) == 0 && (pad & PADLright) == 0){
				if(player_prevDirection == 0)
					sprite_anim(&player, 41, 46, 1, 0, 6);
				if(player_prevDirection == 1)
					sprite_anim(&player, 41, 46, 0, 0, 6);
			}
		}
		if(pad & PADLdown && player.posZ > -280){
			player.posZ -= 5;
			cameraZ += 5;
			if ((pad & PADLleft) == 0 && (pad & PADLright) == 0){
				if(player_prevDirection == 0)
					sprite_anim(&player, 41, 46, 1, 0, 6);
				if(player_prevDirection == 1)
					sprite_anim(&player, 41, 46, 0, 0, 6);
			}
		}
		if(pad & PADLleft){
			if(player.posX > cameraX*-1 - 500){
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
		if(pad & PADLright){
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
			if(player_prevDirection == 1 && player.posX < bat.posX){
				if((player.posZ == bat.posZ) ||
				((player.posZ > bat.posZ) && (player.posZ < bat.posZ + 64)) ||
				((player.posZ < bat.posZ) && (player.posZ + 160 > bat.posZ))){
					bat.posX = cameraX*-1 + 800;
				}
			}
			if(player_prevDirection == 0 && player.posX > bat.posX){
				if((player.posZ == bat.posZ) ||
				((player.posZ > bat.posZ) && (player.posZ < bat.posZ + 64)) ||
				((player.posZ < bat.posZ) && (player.posZ + 160 > bat.posZ))){
					bat.posX = cameraX*-1 + 800;
				}
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
		bat.posX = player.posX + 800;
	}

	opad = pad;
}

void game_draw(){
	short i = 0;
	mesh_draw(&cube);
	for(i = 0; i <= 3; i++)
		mesh_draw_ot(&plane[i], 1023);

	sprite_draw(&player);
	sprite_draw(&player2);
	sprite_draw(&bat);

	sprite_draw_2d_rgb(&energy_bar_1);
	energy_bar_1.w += 1;

	FntPrint("pos x %d \n", player.posX);
	FntPrint("bat x %d \n", bat.posX);
	FntPrint("camX %d\n", cameraX*-1);
}
