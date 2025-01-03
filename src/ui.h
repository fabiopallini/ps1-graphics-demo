#ifndef UI_H
#define UI_H

#include "psx.h"

#define BALLOON_MAX_CHARS 78

typedef struct {
	Sprite sprite;
	char prev_display, display;
	char *text;
	int page_index;
	int pages_length;
	int npc_id;
	char *tale[10];
} Balloon;
Balloon balloon;

void init_ui(u_short tpage, int screen_width, int screen_height);
void set_balloon(Balloon *b, char *text);

#endif
