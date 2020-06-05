#include "psx.h"
#include "images.h"
#include "object.h"

long cameraX = 0;
long cameraZ = 1000;

OBJECT sprite;
OBJECT model;

int main() {
	psSetup(160, 120, 512);
	
	object_init(&sprite, 50, 50, img_logo);
	object_setuv(&sprite, 0, 0, 200, 200);
	object_init(&model, 150, 150, img_logo);
	object_setuv(&model, 0, 0, 200, 200);

	while(1) {
		psClear();
		
		psPadInput(&cameraX, &cameraZ, 0);
		psCamera(cameraX, 500, cameraZ, 300, 0, 0);
		
		
		sprite.posX = 20;
		sprite.posY = 20;
		object_draw2d(&sprite);

		model.angY+=32;
		object_draw(&model);
		
		FntPrint("		  psx graphics demo \n\n");

		psDisplay();
	}

	return 0;
}
