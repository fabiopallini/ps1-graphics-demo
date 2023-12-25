#include "ui.h"

void ui_init() {
	sprite_init_rgb(&atb_bar, 0, 3);
	sprite_set_rgb(&atb_bar, 70, 255, 70);
	atb_bar.posX = SCREEN_WIDTH - 60;
	atb_bar.posY = SCREEN_HEIGHT - 50;

	sprite_init_rgb(&atb_border, 50, 3);
	sprite_set_rgb(&atb_border, 0, 0, 0);
	atb_border.posX = atb_bar.posX; 
	atb_border.posY = atb_bar.posY; 
}

void ui_update() {
	if(atb_bar.w < 50){
		atb_w += 0.05;
		atb_bar.w = (int)atb_w;
	}
}

void ui_draw() {
	sprite_draw_2d_rgb(&atb_bar);
	sprite_draw_2d_rgb(&atb_border);
}
