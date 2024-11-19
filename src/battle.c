#include "battle.h"
#include <stdio.h>

void init_battle(Battle *battle, u_short tpage, int screenW, int screenH){
	int i = 0;
	sprite_init(&battle->command_bg, screenW - 30, 70, NULL);
	sprite_set_rgb(&battle->command_bg, 0, 0, 79);
	battle->command_bg.pos.vx = 15;
	battle->command_bg.pos.vy = screenH - (battle->command_bg.h + 5);

	sprite_init(&battle->selector, 20, 20, tpage);
	sprite_set_uv(&battle->selector, 0, 0, 32, 32);
	battle->selector.pos.vy = SELECTOR_POSY;

	for(i = 0; i < 2; i++){
		sprite_init(&battle->atb[i].bar, 0, 8, NULL);
		sprite_set_rgb(&battle->atb[i].bar, 70, 255, 70);
		battle->atb[i].bar.pos.vx = screenW - 80;
		battle->atb[i].bar.pos.vy = screenH - (65 - i*17);

		sprite_init(&battle->atb[i].border, 50, 8, NULL);
		sprite_set_rgb(&battle->atb[i].border, 0, 0, 0);
		battle->atb[i].border.pos.vx = battle->atb[i].bar.pos.vx; 
		battle->atb[i].border.pos.vy = battle->atb[i].bar.pos.vy; 
	}

	for(i = 0; i < 4; i++){
		sprite_init(&battle->dmg.sprite[i], 50, 50, tpage);
		sprite_set_uv(&battle->dmg.sprite[i], 192, 0, 8, 8);
		sprite_set_rgb(&battle->dmg.sprite[i], 255, 255, 0);
		sprite_shading_disable(&battle->dmg.sprite[i], 0);
	}
}

void reset_battle_targets(Battle *battle){
	u_char i;
	battle->target = 0;
	battle->target_counter = 0;
	battle->calc_targets = 0;
	for(i = 0; i < MAX_TARGETS; i++)
		battle->targets[i] = 0;
}

void display_dmg(DMG *dmg, VECTOR pos, int h, int damage){
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
	
		xx = pos.vx + 32;
		yy = pos.vy - h;
		zz = pos.vz;

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

void battle_draw(Battle *battle, void(*drawSprite)(Sprite *sprite, long _otz), 
	void(*drawSprite_2d)(Sprite *sprite, long _otz), long otsize){
	int i = 0;
	if(battle->dmg.display_time > 0){
		for(i = 0; i < 4; i++){
			drawSprite(&battle->dmg.sprite[i], otsize-1);
			battle->dmg.sprite[i].pos.vy -= 3;
		}
		battle->dmg.display_time -= 2;
	}
	if(battle->command_mode == 1 && battle->atb[0].bar.w >= 50 && ENEMY_ATTACKING == 0)
		drawSprite_2d(&battle->selector, otsize-1);
	if(battle->command_mode == 2 && battle->atb[0].bar.w >= 50)
		drawSprite(&battle->selector, otsize-1);
}

void openBattleMenu(Battle *battle){
	battle->selector.pos.vx = 0; 
	battle->selector.pos.vy = SELECTOR_POSY;
	battle->selector.pos.vz = 0;
	battle->selector.w = 20;
	battle->selector.h = 20;
	battle->command_mode = 1;
}

void closeBattleMenu(Battle *battle){
	battle->command_index = 0;
}
