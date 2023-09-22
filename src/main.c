#include "psx.h"
#include "sprite.h"
#include "mesh.h"

long cameraX = 0;
long cameraY = 820;
long cameraZ = 1500;

u_long *cd_data[7];
Mesh cube, plane;
Sprite player;
short player_prevDirection = 1;
Sprite player2;

void update()
{
	//printf("%ld \n", pad);

	if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
	(pad & PADLleft) == 0 && (pad & PADLright) == 0 && (pad & 64) == 0){
		sprite_setuv(&player, 0, 46*2, 41, 46);
		if(player_prevDirection == 0)
			sprite_setuv(&player, 41, 46*2, 41, 46);
		if(player_prevDirection == 1)
			sprite_setuv(&player, 0, 46*2, 41, 46);
	}
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
	if(pad & PADLleft && player.posX > -480){
		player.posX -= 5;
		cameraX += 5;
		sprite_anim(&player, 41, 46, 1, 0, 6);
		player_prevDirection = 0;
	}
	if(pad & PADLright && player.posX < 480){
		player.posX += 5;
		cameraX -= 5;
		sprite_anim(&player, 41, 46, 0, 0, 6);
		player_prevDirection = 1;
	}

	// can shoot only if the player is not moving
	if((pad & PADLup) == 0 && (pad & PADLdown) == 0 && 
	(pad & PADLleft) == 0 && (pad & PADLright) == 0){ 
		// pad X 
		if(pad & 64){
			if(player_prevDirection == 0)
				sprite_anim(&player, 41, 46, 3, 0, 3);
			if(player_prevDirection == 1)
				sprite_anim(&player, 41, 46, 2, 2, 3);
		} 
	}

	cube.angX += 1;
	cube.angY += 16;
	cube.angZ += 16;
}


void balloon(){
	FntPrint("balloon test");
}

int main() {
	psSetup();
	
	cd_open();

	cd_read_file("HPUP.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("PLANE.OBJ", &cd_data[2]);
	cd_read_file("GROUND.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("BOX.TIM", &cd_data[5]);
	cd_read_file("MARCO.TIM", &cd_data[6]);

	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], SECTOR * 21, SPU_00CH);
	audio_play(SPU_00CH);
	
	mesh_init(&plane, (u_char*)cd_data[2], (u_char*)cd_data[3], 128, 500);
	mesh_init(&cube, (u_char*)cd_data[4], (u_char*)cd_data[5], 32, 50);

	cube.posX -= 350;

	sprite_init(&player, 41*2, 46*2, (u_char *)cd_data[6]);
	sprite_setuv(&player, 0, 0, 41, 46);

	sprite_init(&player2, 60*0.8, 128*0.8, (u_char *)cd_data[1]);
	sprite_setuv(&player2, 0, 0, 60, 128);
	player2.posX -= 150;
	player2.posZ -= 250;

	free3(cd_data);

	while(1) {
		psClear();
		psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
		
		update();

		mesh_draw(&cube);
		mesh_draw_ot(&plane, 1023);
		sprite_draw(&player);
		sprite_draw(&player2);
		//FntPrint(cd_data[3]);
		//balloon();

		psDisplay();
	}

	return 0;
}
