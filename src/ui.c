#include "ui.h"

void ui_init(u_long *selector_img){
	sprite_init_rgb(&atb_bar, 0, 3);
	sprite_set_rgb(&atb_bar, 70, 255, 70);
	atb_bar.posX = SCREEN_WIDTH - 55;
	atb_bar.posY = SCREEN_HEIGHT - 50;

	sprite_init_rgb(&atb_border, 50, 3);
	sprite_set_rgb(&atb_border, 0, 0, 0);
	atb_border.posX = atb_bar.posX; 
	atb_border.posY = atb_bar.posY; 

	sprite_init_rgb(&command_bg, 85, 70);
	sprite_set_rgb(&command_bg, 0, 0, 255);
	command_bg.posX = 15;
	command_bg.posY = SCREEN_HEIGHT - (command_bg.h + 22);

	sprite_init(&selector, 20, 20, selector_img);
	sprite_set_uv(&selector, 0, 0, 32, 32);
	selector.posY = 160;
}

void ui_update(u_long pad) {
	if(command_mode == 0){
		command_index = 0;
	}

	if(atb_bar.w < 50){
		//atb_w += 0.05;
		atb_w += 1.0;
		atb_bar.w = (int)atb_w;
	}
	else 
	{
		if(pad & PADLup && (opad & PADLup) == 0){
			if(command_index > 0)
				command_index--;
		}
		if(pad & PADLdown && (opad & PADLdown) == 0){
			if(command_index < 3)
				command_index++;
		}
		if(pad & PADLcircle && (opad & PADLcircle) == 0){
			atb_w = 0;
			atb_bar.w = 0;
			command_index = 0;
		}

		selector.posY = 160+(17*command_index);
	}
}

void ui_draw(char fnt[][FNT_WIDTH], int player_hp) {
	char str[FNT_WIDTH] = "				player1 ";
	char hp[4];
	sprintf(hp, "%d", player_hp);
	strcat(str, hp);
	strcpy(fnt[23], str);
	sprite_draw_2d_rgb(&atb_bar);
	sprite_draw_2d_rgb(&atb_border);
	
	if(command_mode > 0){
		FntPrint(font_id[1], "Super Shot \n\n");
		FntPrint(font_id[1], "Magic \n\n");
		FntPrint(font_id[1], "GF \n\n");
		FntPrint(font_id[1], "Items \n\n");
		sprite_draw_2d(&selector);
		sprite_draw_2d_rgb(&command_bg);
	}
}
