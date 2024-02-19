#include "game.h"

int main() {
	psInit();
	game_load();	

	while(1) {
		psClear();
		
		game_update();
		game_draw();

		psDisplay();
	}

	return 0;
}

