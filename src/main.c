#include "game.h"

int main() {
	psInit();
	game_load();	

	while(1) {
		psClear();

		if(scene.status == SCENE_LOAD && DSR_callback_id == 0){
			scene.load_callback();		
			scene.status = SCENE_READY;
		}
		if(scene.status == SCENE_READY){
			game_update();
			game_draw();
		}

		psDisplay();
	}

	return 0;
}

