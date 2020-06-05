#include "psx.h"
#include "images.h"
#include "object.h"

long cameraX = 0;
long cameraZ = 1000;

OBJECT object1;

int main() {
    psSetup(160, 120, 512);
	
	object_init(&object1, 200, img_logo);

	while(1) {
		psClear();
		
		psPadInput(&cameraX, &cameraZ, 0);
		psCamera(cameraX, 500, cameraZ, 300, 0, 0);
		
		object1.angY+=32;
		object_draw(&object1);
		
		FntPrint("		  psx graphics demo \n\n");

		psDisplay();
	}

	return 0;
}
