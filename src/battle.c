#include "battle.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define PADLsquare 128
#define PADLcircle 32 
#define PADLcross 64 
#define PADLtriangle 16 

Enemy *enemy_target = NULL;

void init_battle(Battle *battle, u_short tpage, int screenW, int screenH, Color color[4]){
	int i = 0;
	memset(battle, 0, sizeof(Battle));
	window_init(&battle->window, 15, screenH - 75, screenW - 30, 70, tpage, color);
	battle->window.selector.sprite.pos.vx = 0;
	battle->window.selector.sprite.pos.vy = ATTACK_POSY;

	sprite_init(&battle->target_selector.sprite, 60, 60, tpage);
	sprite_set_uv(&battle->target_selector.sprite, 0, 174, 30, 22);

	for(i = 0; i < 2; i++){
		sprite_init(&battle->atb[i].bar, 0, 8, 0);
		sprite_set_rgb(&battle->atb[i].bar, 70, 255, 70, 0);
		battle->atb[i].bar.pos.vx = screenW - 80;
		battle->atb[i].bar.pos.vy = screenH - (65 - i*17);

		sprite_init(&battle->atb[i].border, 50, 8, 0);
		sprite_set_rgb(&battle->atb[i].border, 0, 0, 0, 0);
		battle->atb[i].border.pos.vx = battle->atb[i].bar.pos.vx; 
		battle->atb[i].border.pos.vy = battle->atb[i].bar.pos.vy; 
	}

	for(i = 0; i < 4; i++){
		sprite_init(&battle->dmg.sprite[i], 50, 50, tpage);
		sprite_set_uv(&battle->dmg.sprite[i], 192, 0, 8, 8);
		sprite_set_rgb(&battle->dmg.sprite[i], 255, 255, 0, 0);
		sprite_shading_disable(&battle->dmg.sprite[i], 0);
	}

	stepsCounter = 0;
	battleRandom = 0;
	battleIntro = 0;
	battleEnd = 0;
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

void battle_update(Battle *battle, u_long pad, u_long opad, Entity *entity, Inventory *inv) {
	Model *model = &entity->model;

	// stop battle if there are no more enemies to fight
	if(battle->atb[0].bar.w > 25 && battleEnd){
		battleEnd = 0;
		battle->status = BATTLE_END;
		return;
	}
	// load the player's atb bar
	if(battle->atb[0].bar.w < 50 && ENEMY_ATTACKING == 0 && battle->status == BATTLE_WAIT){
		battle->atb[0].value += 0.2;
		battle->atb[0].bar.w = (int)battle->atb[0].value;
	}
	else {
		if(battle->status == BATTLE_WAIT && ENEMY_ATTACKING == 0) 
		{
			// select a manu option (attack, items, magic ecc)
			if(pad & PADLup && (opad & PADLup) == 0){
				if(battle->command > COMMAND_ATTACK)
					battle->command--;
			}
			if(pad & PADLdown && (opad & PADLdown) == 0){
				if(battle->command < COMMAND_ITEM)
					battle->command++;
			}

			// select a menu command: Attack, Magic, Skill, Item
			if(pad & PADLcross && (opad & PADLcross) == 0)
			{
				// reset vars to calculate available enemies to attack
				u_char i;
				battle->target = 0;
				battle->target_counter = 0;
				enemy_clean();	

				if(enemyNode != NULL){
					EnemyNode *node = enemyNode;
					while(node != NULL){
						Enemy *enemy = node->enemy;
						if(enemy->hp > 0) {
							battle->target_counter++;
						}
						i++;
						node = node->next;
					}
				}

				if(battle->command == COMMAND_ATTACK)
					battle->status = BATTLE_SELECT_TARGET;
				if(battle->command == COMMAND_ITEM)
					battle->status = BATTLE_SUBMENU;

				return;
			}

			/*if(pad & PADLcircle && (opad & PADLcircle) == 0){
				// stop battle
				battle->status = 2;
			}*/

			battle->window.selector.sprite.pos.vy = ATTACK_POSY+(17*battle->command);
		}
	}

	// attack move 
	if(battle->status == BATTLE_ATTACK && enemy_target != NULL)
	{
		u_char moving = 0;
		int speed = 50;
		model->pos.vz = enemy_target->sprite.pos.vz - model_getMesh(model)->size;
 		if(model->pos.vx - (model_getMesh(model)->size*2) > enemy_target->sprite.pos.vx)
		{
			model->pos.vx -= speed;
			moving = 1;
		}

		if(moving == 0){
			model_play_animation(model, 1);
			if(model_animation_is_over(*model) == 1){
				int i = 0;
				int damage = 5;
				EnemyNode *node = enemyNode;
				sfx_play(SPU_2CH);
				enemy_target->hp -= damage;	
				enemy_target->hitted = 1;	
				enemy_target->blood.pos.vx = enemy_target->sprite.pos.vx;
				enemy_target->blood.pos.vy = enemy_target->sprite.pos.vy;
				enemy_target->blood.pos.vz = enemy_target->sprite.pos.vz-5;
				enemy_target->blood.frame = 0;
				sprite_set_animation(&enemy_target->blood, 16, 16, 1, 0, 5, 0);
				
				display_dmg(&battle->dmg, enemy_target->sprite.pos, enemy_target->sprite.h, damage);

				openBattleMenu(battle);
				closeBattleMenu(battle);
				battle->status = BATTLE_REPOS;

				// check if is the last enemey, if so, stop the battle
				while(node != NULL){
					Enemy *enemy = node->enemy;
					if(enemy->hp > 0) {
						i++;
					}
					node = node->next;
				}
				if(i == 0)
					battleEnd = 1;
			}
		}
	}

	// moving back to original fight start pos
	if(battle->status == BATTLE_REPOS && battle->dmg.display_time <= 50){
		u_char moving = 0;
		int speed = 50;
		if(model->pos.vz > entity->battle_pos.vz)
		{
			model->pos.vz -= speed;
			moving = 1;
			if(model->pos.vz <= enemy_target->sprite.pos.vz)
				moving = 0;
		}
		if(model->pos.vz < entity->battle_pos.vz)
		{
			model->pos.vz += speed;
			moving = 1;
			if(model->pos.vz >= enemy_target->sprite.pos.vz)
				moving = 0;
		}
		if(model->pos.vx < entity->battle_pos.vx)
		{
			model->pos.vx += speed;
			moving = 1;
		}
		if(moving == 0)
			battle->status = BATTLE_WAIT;
	}

	// select an enemy to attack...
	if(battle->status == BATTLE_SELECT_TARGET)
	{
		if(pad & PADLcross && (opad & PADLcross) == 0 && battle->target_counter > 0)
		{
			if(battle->command == COMMAND_ATTACK){
				battle->atb[0].value = 0;
				battle->atb[0].bar.w = 0;
				battle->status = BATTLE_ATTACK;
			}
			if(battle->command == COMMAND_ITEM){
				battle->atb[0].value = 0;
				battle->atb[0].bar.w = 0;
				if(battle->action_callback)
					battle->action_callback(inv);
				openBattleMenu(battle);
				// after item is used, move the selector back to Attack
				battle->command = COMMAND_ATTACK; 
			}
			return;
		}

		if(pad & PADLcircle){
			if(battle->command == COMMAND_ATTACK)
				openBattleMenu(battle);
			if(battle->command == COMMAND_ITEM)
				battle->status = BATTLE_SUBMENU;	
			return;
		}
		else
		{			
			if(battle->target_counter > 0)
			{
				Enemy *enemy;
				if((pad & PADLleft && (opad & PADLleft) == 0) || (pad & PADLup && (opad & PADLup) == 0))
				{
					battle->target--;
					if(battle->target >= battle->target_counter)
						battle->target = battle->target_counter-1;
				}
				if((pad & PADLright && (opad & PADLright) == 0) || (pad & PADLdown && (opad & PADLdown) == 0))
				{
					battle->target++;
					if(battle->target >= battle->target_counter)
						battle->target = 0;
				}

				enemy = enemy_get(battle->target);
				enemy_target = enemy;
				if(enemy != NULL){
					battle->target_selector.sprite.pos.vx = enemy->sprite.pos.vx - enemy->sprite.w;
					battle->target_selector.sprite.pos.vy = enemy->sprite.pos.vy;
					battle->target_selector.sprite.pos.vz = enemy->sprite.pos.vz;
				}
			}
			else
			openBattleMenu(battle);
		}
	}

	if(battle->status == BATTLE_SUBMENU){
		if((pad & PADLcircle) && (opad == 0)){
			openBattleMenu(battle);
			return;
		}
	}
}

void openBattleMenu(Battle *battle){
	battle->window.selector.sprite.pos.vx = 0; 
	battle->window.selector.sprite.pos.vy = ATTACK_POSY;
	battle->status = BATTLE_WAIT;
}

void closeBattleMenu(Battle *battle){
	battle->command = COMMAND_ATTACK;
}
