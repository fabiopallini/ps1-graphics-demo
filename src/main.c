#include "psx.h"
#include "sprite.h"
#include "mesh.h"

long cameraX = 0;
long cameraY = 820;
long cameraZ = 1500;

u_long *cd_data[6];
Mesh cube, cube2, plane;
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

	cube.angX += 16;
	cube.angY += 16;
	cube.angZ += 16;
	cube2.angX += 16;
	cube2.angY += 16;
	cube2.angZ += 16;
}

SVECTOR vertices[] = {
	{ 100, 100, 100, 0 },
	{ 100, 100, -100, 0 },
	{ -100, 100, -100, 0 }, 
	{ -100, 100, 100, 0 },
	{ 100, -100, 100, 0 },
	{ 100, -100, -100, 0 },
	{ -100, -100, -100, 0 },
	{ -100, -100, 100, 0 },
};

int indices[] = {
	0, 1, 2, 3,
	4, 7, 6, 5,
	0, 4, 5, 1,
	1, 5, 6, 2,
	2, 6, 7, 3,
	4, 0, 3, 7
};

int main() {
	psSetup();
	
	cd_open();
	cd_read_file("HPUP.VAG", &cd_data[0]);
	cd_read_file("CLOUD.TIM", &cd_data[1]);
	cd_read_file("GROUND.TIM", &cd_data[2]);
	cd_read_file("CUBE.OBJ", &cd_data[3]);
	cd_read_file("BOX.TIM", &cd_data[4]);
	cd_read_file("PLANE.OBJ", &cd_data[5]);
	cd_close();

	audio_init();
	audio_vag_to_spu((u_char *)cd_data[0], SECTOR * 21, SPU_00CH);
	audio_play(SPU_00CH);

	mesh_init((u_char*)cd_data[5], &plane, 500);
	mesh_setTim(&plane, (u_char*)cd_data[2]);

	//mesh_init((u_char*)cd_data[3], &cube, 100);
	cube.vertices = vertices;
	cube.indices = indices;
	cube.indicesLength = 6;
	mesh_init(NULL, &cube, 100);
	mesh_setTim(&cube, (u_char*)cd_data[4]);
	cube.posX = -350;
	cube.posZ = 1000;

	cube2.vertices = vertices;
	cube2.indices = indices;
	cube2.indicesLength = 6;
	mesh_init(NULL, &cube2, 100);
	mesh_setTim(&cube2, (u_char*)cd_data[4]);
	cube2.posX = 350;
	cube2.posZ = 1000;

	sprite_init(&player, 60, 128, (u_char *)cd_data[1]);
	sprite_setuv(&player, 0, 0, 60, 128);

	free3(cd_data);
	
	while(1) {
		psClear();
		psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
		
		update();

		mesh_draw(&plane);
		mesh_draw(&cube);
		mesh_draw(&cube2);
		sprite_draw(&player);
	
		//FntPrint("hello world %d", 123);

		psDisplay();
	}

	return 0;
}