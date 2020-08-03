#include "psx.h"
#include "sprite.h"
#include "mesh.h"

long cameraX = 0;
long cameraY = 820;
long cameraZ = 1500;

u_long *cd_data[7];
Mesh cube[2], plane;
Sprite player;

void update()
{
	if(pad & PADLup && player.posZ < 610){
    	player.posZ += 5;
    	cameraZ -= 5;
    }
    if(pad & PADLdown && player.posZ > -280){
    	player.posZ -= 5;
    	cameraZ += 5;
    }
    if(pad & PADLleft && player.posX > -480){
    	player.posX -= 5;
    	cameraX += 5;
    }
    if(pad & PADLright && player.posX < 480){
    	player.posX += 5;
    	cameraX -= 5;
    }

	cube[0].angX += 16;
	cube[0].angY += 16;
	cube[0].angZ += 16;
	cube[1].angX += 16;
	cube[1].angY += 16;
	cube[1].angZ += 16;
}

SVECTOR vertices[] = {
	{500, 0, -500 },
	{500, 0, 500 },
	{-500, 0, 500 },
	{-500, 0, -500 }
};

int indices[] = {
	0, 3, 2, 1
};

int main() {
	psSetup();
	
	cd_open();
	cd_read_file("HPUP.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("GROUND.TIM", &cd_data[2]);
	cd_read_file("PLANE.OBJ", &cd_data[3]);
	cd_read_file("BOX.TIM", &cd_data[4]);
	cd_read_file("CUBE.OBJ", &cd_data[5]);
	cd_read_file("CUBE.OBJ", &cd_data[6]);
	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], SECTOR * 21, SPU_00CH);
	audio_play(SPU_00CH);

	//mesh_init(&plane, 500, (u_char*)cd_data[3]);
	plane.vertices = vertices;
	plane.indices = indices;
	plane.indicesLength = 1;
	mesh_init(&plane, 500, NULL);
	mesh_setTim(&plane, (u_char*)cd_data[2]);

	mesh_init(&cube[0], 100, (u_char*)cd_data[5]);
	mesh_setTim(&cube[0], (u_char*)cd_data[4]);
	cube[0].posX = -350;
	cube[0].posZ = 1000;

	mesh_init(&cube[1], 100, (u_char*)cd_data[6]);
	mesh_setTim(&cube[1], (u_char*)cd_data[4]);
	cube[1].posX = 350;
	cube[1].posZ = 1000;

	sprite_init(&player, 60, 128, (u_char *)cd_data[1]);
	sprite_setuv(&player, 0, 0, 60, 128);

	free3(cd_data);
	
	while(1) {
		psClear();
		psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
		
		update();

		mesh_draw(&plane);
		mesh_draw(&cube[0]);
		mesh_draw(&cube[1]);
		sprite_draw(&player);
		//FntPrint("hello world %d", 123);

		psDisplay();
	}

	return 0;
}