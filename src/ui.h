#ifndef UI_H
#define UI_H

#include "sprite.h"
#include "utils.h"
#include "enemy.h"

typedef struct {
	Sprite sprite;
	char prev_display, display;
	char *text;
} Balloon;
Balloon balloon;

void font_init(Font *font);
void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h);
void set_balloon(Balloon *b, char *text);

#endif
