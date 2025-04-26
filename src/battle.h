#ifndef BATTLE_H
#define BATTLE_H

#include "psx.h"
#include "utils.h"
#include "enemy.h"

#define ATTACK_POSY 188

typedef struct DMG {
	Sprite sprite[4]; // 9999 is the max number (4 digit)
	int display_time;
} DMG;

typedef struct ATB {
	float value;
	Sprite bar, border; 
} ATB;

typedef struct Battle {
	BATTLE_MODE status;
	ATB atb[2];
	Window window;
	Selector target_selector;
	MENU_COMMANDS command;
	float t;
	char (*action_callback)(Inventory *inv);

	u_char target;
	u_char target_counter;

	Model chars[3];
	Enemy enemies[3];
	DMG dmg;
} Battle;

int stepsCounter, battleRandom;
u_char battleIntro;
u_char battleEnd;

void init_battle(Battle *battle, u_short tpage, int screenW, int screenH, Color color[4]);
void reset_battle_targets(Battle *battle);
void display_dmg(DMG *dmg, VECTOR pos, int h, int damage);
void battle_update(Battle *battle, u_long pad, u_long opad, Entity *entity, Inventory *inv);
void openBattleMenu(Battle *battle);
void closeBattleMenu(Battle *battle);

#endif
