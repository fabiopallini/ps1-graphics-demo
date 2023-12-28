#include "ui.h"

void ui_init(u_long *selector_img){
	u_char i = 0;
	sprite_init_rgb(&command_bg, 85, 70);
	sprite_set_rgb(&command_bg, 0, 0, 255);
	command_bg.posX = 15;
	command_bg.posY = SCREEN_HEIGHT - (command_bg.h + 22);

	sprite_init(&selector, 20, 20, selector_img);
	sprite_set_uv(&selector, 0, 0, 32, 32);
	selector.posY = 160;

	for(i = 0; i < 2; i++){
		sprite_init_rgb(&cp[i].atb_bar, 0, 3);
		sprite_set_rgb(&cp[i].atb_bar, 70, 255, 70);
		cp[i].atb_bar.posX = SCREEN_WIDTH - 60;
		cp[i].atb_bar.posY = SCREEN_HEIGHT - (50 - i*17);

		sprite_init_rgb(&cp[i].atb_border, 50, 3);
		sprite_set_rgb(&cp[i].atb_border, 0, 0, 0);
		cp[i].atb_border.posX = cp[i].atb_bar.posX; 
		cp[i].atb_border.posY = cp[i].atb_bar.posY; 

	}
}

void ui_update(u_long pad, u_long opad) {
	if(command_mode == 0){
		command_index = 0;
	}

	if(cp[0].atb_bar.w < 50){
		//cp[0].atb_w += 0.05;
		cp[0].atb_w += 1.0;
		cp[0].atb_bar.w = (int)cp[0].atb_w;
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
			cp[0].atb_w = 0;
			cp[0].atb_bar.w = 0;
			command_index = 0;
		}

		selector.posY = 160+(17*command_index);
	}
}

void ui_draw(char fnt[][FNT_WIDTH], int player_hp, int player2_hp) {
	char str[FNT_WIDTH];
	char str2[FNT_WIDTH];

	sprintf(str, "					Player1 %d", player_hp);
	strcpy(fnt[23], str);
	sprintf(str2, "					Player2 %d", player2_hp);
	strcpy(fnt[25], str2);

	sprite_draw_2d_rgb(&cp[0].atb_bar);
	sprite_draw_2d_rgb(&cp[0].atb_border);
	sprite_draw_2d_rgb(&cp[1].atb_bar);
	sprite_draw_2d_rgb(&cp[1].atb_border);
	
	if(command_mode > 0){
		FntPrint(font_id[1], "Super Shot \n\n");
		FntPrint(font_id[1], "Magic \n\n");
		FntPrint(font_id[1], "GF \n\n");
		FntPrint(font_id[1], "Items \n\n");
		sprite_draw_2d(&selector);
		sprite_draw_2d_rgb(&command_bg);
	}
}
