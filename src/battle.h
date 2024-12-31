#ifndef BATTLE_H
#define BATTLE_H

#include "sprite.h"
#include "char.h"
#include "enemy.h"

#define MAX_TARGETS 10
#define SELECTOR_POSY 185

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
	u_char status;
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

int stepsCounter, battleRandom;
u_char battleIntro;
u_char battleEnd;

void init_battle(Battle *battle, u_short tpage, int screenW, int screenH);
void reset_battle_targets(Battle *battle);
void display_dmg(DMG *dmg, VECTOR pos, int h, int damage);
void battle_update(Battle *battle, u_long pad, u_long opad, Character *character);
void battle_draw(Battle *battle, void(*drawSprite)(Sprite *sprite, long _otz), 
	void(*drawSprite_2d)(Sprite *sprite, long _otz), long otz);
void openBattleMenu(Battle *battle);
void closeBattleMenu(Battle *battle);

#endif
