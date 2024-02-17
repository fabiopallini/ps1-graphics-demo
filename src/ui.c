#include "ui.h"

#define SELECTOR_POSY 165

static void reset_targets();
static void mainCommandMenu();
static void closeCommandMenu();

void ui_init(u_short tpage, int screenW, int screenH){
	u_char i = 0;
	sprite_init_rgb(&command_bg, 85, 70);
	sprite_set_rgb(&command_bg, 0, 0, 255);
	command_bg.pos.vx = 15;
	command_bg.pos.vy = screenH - (command_bg.h + 22);

	sprite_init(&selector, 20, 20, tpage);
	sprite_set_uv(&selector, 0, 0, 32, 32);
	selector.pos.vy = SELECTOR_POSY;

	for(i = 0; i < 2; i++){
		sprite_init_rgb(&atb[i].bar, 0, 3);
		sprite_set_rgb(&atb[i].bar, 70, 255, 70);
		atb[i].bar.pos.vx = screenW - 60;
		atb[i].bar.pos.vy = screenH - (50 - i*17);

		sprite_init_rgb(&atb[i].border, 50, 3);
		sprite_set_rgb(&atb[i].border, 0, 0, 0);
		atb[i].border.pos.vx = atb[i].bar.pos.vx; 
		atb[i].border.pos.vy = atb[i].bar.pos.vy; 
	}

	font_init(&font);
	dmg_init(tpage, &dmg);	
}

void ui_update(u_long pad, u_long opad, Sprite *player, Camera *camera, Enemy *enemies) {
	if(atb[0].bar.w < 50){
		//atb[0].w += 0.05;
		atb[0].value += 1.0;
		atb[0].bar.w = (int)atb[0].value;
	}

	if(command_mode == CMODE_LEFT || command_mode == CMODE_RIGHT || 
		command_mode == CMODE_LEFT_ATTACK || command_mode == CMODE_RIGHT_ATTACK)
	{

		if((command_mode == CMODE_LEFT || command_mode == CMODE_LEFT_ATTACK) && camera->rot.vy < 290)
			camera->rot.vy += 8;
		if((command_mode == CMODE_RIGHT || command_mode == CMODE_RIGHT_ATTACK)&& camera->rot.vy > -290)
			camera->rot.vy -= 8;

		if(camera->rot.vy > 0 && camera->pos.vx > (player->pos.vx*-1) - 500)
			camera->pos.vx -= 24;
		if(camera->rot.vy < 0 && camera->pos.vx < (player->pos.vx*-1) + 800)
			camera->pos.vx += 24;

		if(camera->pos.vz > 2000)
			camera->pos.vz -= 8;
		if(camera->pos.vz < 2000){
			camera->pos.vz = 2000;
		}
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
			reset_targets();
			if(command_mode == CMODE_LEFT)
				command_mode = CMODE_LEFT_ATTACK;
			if(command_mode == CMODE_RIGHT)
				command_mode = CMODE_RIGHT_ATTACK;
			command_index = 0;
		}
		if(pad & PADLcircle && (opad & PADLcircle) == 0){
			closeCommandMenu();
		}

		selector.pos.vy = SELECTOR_POSY+(17*command_index);
	}

	if(command_mode == CMODE_FROM_LEFT || command_mode == CMODE_FROM_RIGHT)
	{
		if(camera->rot.vy < 0)
			camera->rot.vy += 8;
		if(camera->rot.vy > 0)
			camera->rot.vy -= 8;

		if(command_mode == CMODE_FROM_RIGHT){
			if(camera->pos.vx > camera->ox)
				camera->pos.vx -= 24;
			else
				camera->pos.vx = camera->ox;
		}

		if(command_mode == CMODE_FROM_LEFT){
			if(camera->pos.vx < camera->ox)
				camera->pos.vx += 24;
			else
				camera->pos.vx = camera->ox;
		}

		if(camera->pos.vz < 2300)
			camera->pos.vz += 8;
		else
			camera->pos.vz = 2300;

		if(camera->rot.vy == 0 && camera->pos.vx == camera->ox && camera->pos.vz == 2300){
			command_mode = 0;
		}
	}

	if(command_attack == 1)
	{
		short status = 1;
		//player->frameInterval = 10;
		status = sprite_anim(player, 41, 46, 2, 0, 6);
		if(status == 0){
			sprite_set_uv(player, 0, 46, 41, 46);
			player->hp += 1; 

			enemies[targets[target]].sprite.hp -= 8;
			enemies[targets[target]].sprite.hitted = 1;
			enemies[targets[target]].blood.pos.vx = enemies[targets[target]].sprite.pos.vx;
			enemies[targets[target]].blood.pos.vy = enemies[targets[target]].sprite.pos.vy;
			enemies[targets[target]].blood.pos.vz = enemies[targets[target]].sprite.pos.vz-5;
			enemies[targets[target]].blood.frame = 0;
			
			display_dmg(&dmg, enemies[targets[target]].sprite, "8");

			command_attack = 0;
		}
	}
}

void ui_enemies_selector(u_long pad, u_long opad, Sprite player, int n_enemies, Enemy *enemies){
	int i = 0;
	if(command_mode == CMODE_LEFT_ATTACK || command_mode == CMODE_RIGHT_ATTACK)
	{
		if(pad & PADLcross && (opad & PADLcross) == 0 && target_counter > 0)
		{
			atb[0].value = 0;
			atb[0].bar.w = 0;
			enemies[targets[target]].sprite.pos.vx -= 10;
			mainCommandMenu();
			closeCommandMenu();
			command_attack = 1;
			return;
		}

		if(pad & PADLcircle)
			mainCommandMenu();
		else
		{		
			selector.w = 60;
			selector.h = 60;
			
			if(calc_targets == 0){
				calc_targets = 1;
				for(i = 0; i < n_enemies; i++)
				{
					if(command_mode == CMODE_LEFT_ATTACK && enemies[i].sprite.pos.vx <= player.pos.vx &&
						enemies[i].sprite.hp > 0){
						targets[target_counter] = i;	
						//printf("left t %d \n", targets[target_counter]);
						target_counter++;
					}
					if(command_mode == CMODE_RIGHT_ATTACK && enemies[i].sprite.pos.vx >= player.pos.vx &&
						enemies[i].sprite.hp > 0){
						targets[target_counter] = i;	
						//printf("right t %d \n", targets[target_counter]);
						target_counter++;
					}
				}
				//printf("target %d \n", targets[0]);
				//printf("target_counter %d \n", target_counter);
			}
			if(target_counter > 0)
			{
				if(pad & PADLleft && (opad & PADLleft) == 0)
				{
					if(target == 0)
						target = target_counter-1;
					else
						target--;
				}
				if(pad & PADLright && (opad & PADLright) == 0)
				{
					target++;
					if(target >= target_counter)
						target = 0;
				}

				selector.pos.vx = enemies[targets[target]].sprite.pos.vx - (enemies[targets[target]].sprite.w + 10);
				selector.pos.vy = enemies[targets[target]].sprite.pos.vy;
				selector.pos.vz = enemies[targets[target]].sprite.pos.vz;
			}
			else
			mainCommandMenu();
		}
	}
}

void font_init(Font *font){
	int i = 0;
	for(i = 0; i < FONT_MAX_CHARS; i++){
		SetDrawMode(&font->dr_mode[i], 0, 0, GetTPage(2, 0, 640, 256), 0);
		SetSprt(&font->sprt[i]);
		font->sprt[i].w = 8; 
		font->sprt[i].h = 8;
		//setRGB0(&font->sprt[i], 100, 100, 100);
		setRGB0(&font->sprt[i], 255, 255, 255);
		setXY0(&font->sprt[i], 32+(16*i), 32);
	}
}

void dmg_init(u_short tpage, DMG *dmg){
	int i = 0;
	for(i = 0; i < 3; i++){
		sprite_init(&dmg->sprite[i], 50, 50, tpage);
		sprite_set_uv(&dmg->sprite[i], 192, 8*3, 8, 8);
		sprite_setRGB(&dmg->sprite[i], 255, 0, 0);
		sprite_shading_disable(&dmg->sprite[i], 0);
	}
}

void display_dmg(DMG *dmg, Sprite target, u_char *damage){
	u_char c, i = 0;

	dmg->damage = damage;
	dmg->display_time = 100;

	// reset sprites to space character
	sprite_set_uv(&dmg->sprite[0], 192, 0, 8, 8);
	sprite_set_uv(&dmg->sprite[1], 192, 0, 8, 8);
	sprite_set_uv(&dmg->sprite[2], 192, 0, 8, 8);

	while((c = *damage) != '\0' && i < 3){
		short row, x, y, xx, yy, zz;
		//printf("%c\n", c);
		//printf("%d\n", c);
		
		row = (c - 32) / 8;
		x = 192 + (8 * (c - (32 + (8 * row))));
		y = 8 * row;
		printf("x %d y %d\n", x, y);

		sprite_set_uv(&dmg->sprite[i], x, y, 8, 8);
		xx = target.pos.vx + (dmg->sprite[i].w/2);
		yy = target.pos.vy - (dmg->sprite[i].h/2);
		zz = target.pos.vz;
	
		dmg->sprite[i].pos.vx = xx+(dmg->sprite[i].w*i);
		dmg->sprite[i].pos.vy = yy;
		dmg->sprite[i].pos.vz = zz;

		damage++;
		i++;
	}
}

static void reset_targets(){
	u_char i;
	target = 0;
	target_counter = 0;
	calc_targets = 0;
	for(i = 0; i < MAX_TARGETS; i++)
		targets[i] = 0;
}

static void mainCommandMenu(){
	selector.pos.vx = 0; 
	selector.pos.vy = SELECTOR_POSY;
	selector.pos.vz = 0;
	selector.w = 20;
	selector.h = 20;
	if(command_mode == CMODE_LEFT_ATTACK) 
		command_mode = CMODE_LEFT;
	if(command_mode == CMODE_RIGHT_ATTACK) 
		command_mode = CMODE_RIGHT;
}

static void closeCommandMenu(){
	command_index = 0;
	if(command_mode == CMODE_LEFT)
		command_mode = CMODE_FROM_LEFT;
	if(command_mode == CMODE_RIGHT)
		command_mode = CMODE_FROM_RIGHT;
}
