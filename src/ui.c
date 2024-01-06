#include "ui.h"

#define SELECTOR_POSY 165

void ui_init(u_long *selector_img){
	u_char i = 0;
	sprite_init_rgb(&command_bg, 85, 70);
	sprite_set_rgb(&command_bg, 0, 0, 255);
	command_bg.posX = 15;
	command_bg.posY = SCREEN_HEIGHT - (command_bg.h + 22);

	sprite_init(&selector, 20, 20, selector_img);
	sprite_set_uv(&selector, 0, 0, 32, 32);
	selector.posY = SELECTOR_POSY;

	for(i = 0; i < 2; i++){
		sprite_init_rgb(&atb[i].bar, 0, 3);
		sprite_set_rgb(&atb[i].bar, 70, 255, 70);
		atb[i].bar.posX = SCREEN_WIDTH - 60;
		atb[i].bar.posY = SCREEN_HEIGHT - (50 - i*17);

		sprite_init_rgb(&atb[i].border, 50, 3);
		sprite_set_rgb(&atb[i].border, 0, 0, 0);
		atb[i].border.posX = atb[i].bar.posX; 
		atb[i].border.posY = atb[i].bar.posY; 
	}
}

void ui_update(u_long pad, u_long opad, Sprite *player) {
	if(atb[0].bar.w < 50){
		//atb[0].w += 0.05;
		atb[0].value += 1.0;
		atb[0].bar.w = (int)atb[0].value;
	}

	if(command_mode == CMODE_LEFT || command_mode == CMODE_RIGHT) 
	{
		if(pad & PADLup && (opad & PADLup) == 0){
			if(command_index > 0)
				command_index--;
		}
		if(pad & PADLdown && (opad & PADLdown) == 0){
			if(command_index < 3)
				command_index++;
		}
		if(pad & PADLcross && (opad & PADLcross) == 0){
			atb[0].value = 0;
			atb[0].bar.w = 0;
			if(command_mode == CMODE_LEFT)
				command_mode = 5;
			if(command_mode == CMODE_RIGHT)
				command_mode = 6;
			command_index = 0;
		}
		if(pad & PADLcircle && (opad & PADLcircle) == 0){
			command_index = 0;
			if(command_mode == CMODE_LEFT)
				command_mode = CMODE_FROM_LEFT;
			if(command_mode == CMODE_RIGHT)
				command_mode = CMODE_FROM_RIGHT;
		}

		selector.posY = SELECTOR_POSY+(17*command_index);

		if(command_mode == CMODE_LEFT && camera.rot.vy < 290)
			camera.rot.vy += 8;
		if(command_mode == CMODE_RIGHT && camera.rot.vy > -290)
			camera.rot.vy -= 8;

		if(camera.rot.vy > 0 && camera.pos.vx > (player->posX*-1) - 500)
			camera.pos.vx -= 24;
		if(camera.rot.vy < 0 && camera.pos.vx < (player->posX*-1) + 800)
			camera.pos.vx += 24;

		if(camera.pos.vz > 2000)
			camera.pos.vz -= 8;
		if(camera.pos.vz < 2000){
			camera.pos.vz = 2000;
		}

	}

	if(command_mode == CMODE_FROM_LEFT || command_mode == CMODE_FROM_RIGHT){

		if(camera.rot.vy < 0)
			camera.rot.vy += 8;
		if(camera.rot.vy > 0)
			camera.rot.vy -= 8;

		if(command_mode == CMODE_FROM_RIGHT){
			if(camera.pos.vx > camera.ox)
				camera.pos.vx -= 24;
			else
				camera.pos.vx = camera.ox;
		}

		if(command_mode == CMODE_FROM_LEFT){
			if(camera.pos.vx < camera.ox)
				camera.pos.vx += 24;
			else
				camera.pos.vx = camera.ox;
		}

		if(camera.pos.vz < 2300)
			camera.pos.vz += 8;
		else
			camera.pos.vz = 2300;

		if(camera.rot.vy == 0 && camera.pos.vx == camera.ox && camera.pos.vz == 2300){
			command_mode = 0;
		}
	}
}

void ui_enemies_selector(u_long pad, u_long opad, int size, Enemy *enemies){
	int i = 0;
	if(command_mode == 5 || command_mode == 6){
		if(pad & PADLcircle){
			selector.posX = 0; 
			selector.posY = SELECTOR_POSY;
			selector.posZ = 0;
			selector.w = 20;
			selector.h = 20;
			if(command_mode == 5)
				command_mode = 1;
			if(command_mode == 6)
				command_mode = 2;
		}
		else
		{		
			selector.w = 60;
			selector.h = 60;
			selector.posX = enemies[0].sprite.posX - (enemies[0].sprite.w + 10);
			selector.posY = enemies[0].sprite.posY;
			selector.posZ = enemies[0].sprite.posZ;

			for(i = 0; i < size; i++)
			{
				printf("%ld \n", enemies[i].sprite.posX);
			}
		}
	}
}

void ui_draw(char fnt[][FNT_WIDTH], int player_hp, int player2_hp) {
	char str[FNT_WIDTH];
	char str2[FNT_WIDTH];

	sprintf(str, "					Player1 %d", player_hp);
	strcpy(fnt[23], str);
	sprintf(str2, "					Player2 %d", player2_hp);
	strcpy(fnt[25], str2);

	drawSprite_2d_rgb(&atb[0].bar);
	drawSprite_2d_rgb(&atb[0].border);
	drawSprite_2d_rgb(&atb[1].bar);
	drawSprite_2d_rgb(&atb[1].border);
	
	if(command_mode > 0){
		FntPrint(font_id[1], "Super Shot \n\n");
		FntPrint(font_id[1], "Magic \n\n");
		FntPrint(font_id[1], "GF \n\n");
		FntPrint(font_id[1], "Items \n\n");

		if(command_mode == 1 || command_mode == 2)
			drawSprite_2d(&selector);
		if(command_mode == 5 || command_mode == 6)
			drawSprite(&selector);

		drawSprite_2d_rgb(&command_bg);
	}
}
