#ifndef UI_H
#define UI_H

#include "sprite.h"
#include "utils.h"
#include "enemy.h"

#define MAX_TARGETS 10
#define FONT_MAX_CHARS 78
#define SELECTOR_POSY 185

typedef struct {
	float value;
	Sprite bar, border; 
} ATB;

ATB atb[2];
Sprite command_bg, selector;
u_char command_mode;
u_char command_index;
u_char command_attack;

u_char target;
u_char target_counter;
u_char targets[MAX_TARGETS];
u_char calc_targets;

typedef struct {
	Sprite sprite[4];
	int display_time;
} DMG;
DMG dmg;

typedef struct {
	DR_MODE dr_mode[FONT_MAX_CHARS];
	SPRT sprt[FONT_MAX_CHARS];
} Font;

typedef struct {
	Sprite sprite;
	char prev_display, display;
	char *text;
} Balloon;
Balloon balloon;

void ui_init(u_short tpage, int screenW, int screenH);
void font_init(Font *font);
void dmg_init(u_short tpage, DMG *dmg);
void display_dmg(DMG *dmg, VECTOR pos, int h, int damage);
void reset_targets();
void mainCommandMenu();
void closeCommandMenu();
void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h);
void set_balloon(Balloon *b, char *text);

#endif
