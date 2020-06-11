#include "psx.h"
#include "images.h"
#include "object.h"
#include "model.h"

long cameraX = 0;
long cameraY = 820;
long cameraZ = 1500;

OBJECT ground, player;

void update()
{
    if(pad & PADLup && player.posZ < 610){
    	player.posZ += 5;
    	cameraZ -= 5;
    }
    if(pad & PADLdown && player.posZ > -260){
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
}

int main() {
	psSetup();
	
	object_init(&ground, 500, 500, img_grass);
	object_setuv(&ground, 0, 0, 128, 128);
	ground.angX -= 1050;
	object_init(&player, 60, 128, img_player);
	object_setuv(&player, 0, 0, 60, 128);

	model_init();
	
	while(1) {
		psClear();
		psCamera(cameraX, cameraY, cameraZ, 300, 0, 0);
		
		update();
		object_draw(&ground);
		object_draw(&player);
		model_draw();
		
		psDisplay();
	}

	return 0;
}
