#include "ui.h"

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

void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h){
	sprite_init(&b->sprite, 200, 60, tpage);
	sprite_set_uv(&b->sprite, 0, 97, 200, 60);
	b->sprite.pos.vx = (screen_w / 2) - (b->sprite.w / 2);
	b->sprite.pos.vy = screen_h - (b->sprite.h + 10);
}

void set_balloon(Balloon *b, char *text){
	b->prev_display = 0;
	b->display = 1;
	b->text = text; 
}
