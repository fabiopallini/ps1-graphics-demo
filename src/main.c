#include "game.h"

void balloon(){
	FntPrint("balloon test");
}

int main() {
	psInit();
	game_load();	

	while(1) {
		psClear();
		
		game_update();
		game_draw();
		//FntPrint(cd_data[3]);
		//balloon();

		psDisplay();
	}

	return 0;
}

