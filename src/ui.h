#ifndef UI_H
#define UI_H

#include "sprite.h"
#include "utils.h"
#include "enemy.h"

#define MAX_TARGETS 10
#define BALLOON_MAX_CHARS 78
#define SELECTOR_POSY 185

typedef struct {
	DR_MODE dr_mode[BALLOON_MAX_CHARS];
	SPRT sprt[BALLOON_MAX_CHARS];
} Font;

typedef struct {
	Sprite sprite;
	char prev_display, display;
	char *text;
	int page_index;
	int pages_length;
} Balloon;
Balloon balloon;

typedef struct {
	Sprite sprite[4]; // 9999 is the max number (4 digit)
	int display_time;
} DMG;

typedef struct {
	float value;
	Sprite bar, border; 
} ATB;

typedef struct Battle {
	ATB atb[2];
	Sprite command_bg, selector;
	u_char command_mode;
	u_char command_index;
	u_char command_attack;

	u_char target;
	u_char target_counter;
	u_char targets[MAX_TARGETS];
	u_char calc_targets;

	Character chars[3];
	Enemy enemies[3];
	DMG dmg;
} Battle;

void font_init(Font *font);
void init_balloon(Balloon *b, u_short tpage, int screen_w, int screen_h);
void set_balloon(Balloon *b, char *text);
void battle_init(Battle *battle, u_short tpage, int screenW, int screenH);
void reset_battle_targets(Battle *battle);
void display_dmg(DMG *dmg, VECTOR pos, int h, int damage);
void openBattleMenu(Battle *battle);
void closeBattleMenu(Battle *battle);

#endif
