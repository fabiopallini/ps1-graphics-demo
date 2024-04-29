#include "ui.h"

void ui_init(u_short tpage, int screenW, int screenH){
	u_char i = 0;
	sprite_init(&command_bg, 85, 70, NULL);
	sprite_set_rgb(&command_bg, 0, 0, 255);
	command_bg.pos.vx = 15;
	command_bg.pos.vy = screenH - (command_bg.h + 22);

	sprite_init(&selector, 20, 20, tpage);
	sprite_set_uv(&selector, 0, 0, 32, 32);
	selector.pos.vy = SELECTOR_POSY;

	for(i = 0; i < 2; i++){
		sprite_init(&atb[i].bar, 0, 3, NULL);
		sprite_set_rgb(&atb[i].bar, 70, 255, 70);
		atb[i].bar.pos.vx = screenW - 60;
		atb[i].bar.pos.vy = screenH - (50 - i*17);

		sprite_init(&atb[i].border, 50, 3, NULL);
		sprite_set_rgb(&atb[i].border, 0, 0, 0);
		atb[i].border.pos.vx = atb[i].bar.pos.vx; 
		atb[i].border.pos.vy = atb[i].bar.pos.vy; 
	}

	font_init(&font);
	dmg_init(tpage, &dmg);	
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
	}
}

void dmg_init(u_short tpage, DMG *dmg){
	int i = 0;
	for(i = 0; i < 4; i++){
		sprite_init(&dmg->sprite[i], 50, 50, tpage);
		sprite_set_uv(&dmg->sprite[i], 192, 0, 8, 8);
		sprite_setRGB(&dmg->sprite[i], 255, 255, 0);
		sprite_shading_disable(&dmg->sprite[i], 0);
	}
}

void display_dmg(DMG *dmg, Sprite target, int damage){
	u_char c, i = 0;
	char dmg_str[4];
	sprintf(dmg_str, "%d", damage);
	//printf("\n damage \n %s \n", dmg_str);

	dmg->display_time = 100;

	// reset sprites to space character
	sprite_set_uv(&dmg->sprite[0], 192, 0, 8, 8);
	sprite_set_uv(&dmg->sprite[1], 192, 0, 8, 8);
	sprite_set_uv(&dmg->sprite[2], 192, 0, 8, 8);
	sprite_set_uv(&dmg->sprite[3], 192, 0, 8, 8);

	while((c = dmg_str[i]) != '\0' && i < 4){
		short row, x, y, xx, yy, zz;
		//printf("%c\n", c);
		//printf("%d\n", c);
		
		row = (c - 32) / 8;
		x = 192 + (8 * (c - (32 + (8 * row))));
		y = 8 * row;
		//printf("x %d y %d\n", x, y);

		sprite_set_uv(&dmg->sprite[i], x, y, 8, 8);
	
		xx = target.pos.vx + (target.w/2);
		yy = target.pos.vy - (target.h/2);
		zz = target.pos.vz;

		dmg->sprite[i].pos.vx = xx+(dmg->sprite[i].w*i);
		dmg->sprite[i].pos.vy = yy;
		dmg->sprite[i].pos.vz = zz;

		if(i == 1){
			dmg->sprite[i-1].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i].pos.vx -= dmg->sprite[i].w;	
		}
		if(i == 2){
			dmg->sprite[i-2].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i-1].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i].pos.vx -= dmg->sprite[i].w * 2;	
		}
		if(i == 3){
			dmg->sprite[i-3].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i-2].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i-1].pos.vx -= dmg->sprite[i].w;	
			dmg->sprite[i].pos.vx -= dmg->sprite[i].w * 3;
		}

		i++;
	}
}

void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h){
	sprite_init(&b->sprite, 200, 60, tpage);
	sprite_set_uv(&b->sprite, 0, 97, 200, 60);
	b->sprite.pos.vx = (screen_w / 2) - (b->sprite.w / 2);
	b->sprite.pos.vy = screen_h - (b->sprite.h + 10);
	font_init(&b->font);
}

void set_balloon(Balloon *b, char *text){
	b->prev_display = 0;
	b->display = 1;
	b->text = text; 
}

void reset_targets(){
	u_char i;
	target = 0;
	target_counter = 0;
	calc_targets = 0;
	for(i = 0; i < MAX_TARGETS; i++)
		targets[i] = 0;
}

void mainCommandMenu(){
	selector.pos.vx = 0; 
	selector.pos.vy = SELECTOR_POSY;
	selector.pos.vz = 0;
	selector.w = 20;
	selector.h = 20;
	command_mode = CMODE_RIGHT;
}

void closeCommandMenu(){
	command_index = 0;
}
