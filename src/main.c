#include "game.h"
#include "shooter.h"

typedef enum DemoMode {
	DEMO_MENU,
	DEMO_RPG,
	DEMO_SHOOTER
} DemoMode;

static DemoMode demo_mode = DEMO_MENU;
static int demo_selection = 0;

static void demo_menu_update(void) {
	if((pad & PADLup) && !(opad & PADLup)) demo_selection = 0;
	if((pad & PADLdown) && !(opad & PADLdown)) demo_selection = 1;
	if((pad & PADLcross) && !(opad & PADLcross)) {
		if(demo_selection == 0) {
			game_load();
			demo_mode = DEMO_RPG;
		} else {
			shooter_load();
			demo_mode = DEMO_SHOOTER;
		}
	}
}

static void demo_menu_draw(void) {
	FntPrint("\n\n\n\n\n\n");
	FntPrint("             PSY-Q DEMO DISC\n\n");
	FntPrint("               SELECT A GAME\n\n\n");
	if(demo_selection == 0) {
		FntPrint("          > CRYSTAL FANTASY\n\n");
		FntPrint("            NEON RANGER\n");
	} else {
		FntPrint("            CRYSTAL FANTASY\n\n");
		FntPrint("          > NEON RANGER\n");
	}
	FntPrint("\n\n\n       D-PAD: SELECT     X: START\n");
}

int main() {
	psInit();

	while(1) {
		psClear();
		if(demo_mode == DEMO_MENU) {
			demo_menu_update();
			demo_menu_draw();
		} else if(demo_mode == DEMO_RPG) {
			if(scene.status == SCENE_LOAD && DSR_callback_id == 0){
				scene.load_callback();
				scene.status = SCENE_READY;
			}
			if(scene.status == SCENE_READY){
				game_update();
				game_draw();
			}
		} else {
			shooter_update();
			shooter_draw();
		}

		psDisplay();
	}

	return 0;
}
