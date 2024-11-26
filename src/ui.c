#include "ui.h"

void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h);
void set_balloon(Balloon *b, char *text);

void init_ui(u_short tpage, int screen_width, int screen_height){
	init_balloon(&balloon, tpage, screen_width, screen_height);
}

void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h){
	sprite_init(&b->sprite, 200, 60, tpage);
	sprite_set_uv(&b->sprite, 0, 97, 200, 60);
	b->sprite.pos.vx = (screen_w / 2) - (b->sprite.w / 2);
	b->sprite.pos.vy = screen_h - (b->sprite.h + 10);
	b->page_index = 0;
	b->pages_length = 0;
	b->npc_id = 0;
}

void set_balloon(Balloon *b, char *text){
	b->prev_display = 0;
	b->display = 1;
	b->text = text; 
}
