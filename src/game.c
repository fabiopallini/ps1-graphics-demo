#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"
#include "ui.h"
#include "char.h"

#define DEBUG
u_char CAMERA_DEBUG = 0;

u_long *cd_data[7];
u_long *bk_buffer[4];
u_short tpages[5];
Mesh cube;
Camera prevCamera;
Character character_1;
u_long *char1_animations[2][3];
Enemy *enemy_target;
short mapIndex = 0;
Sprite sprite_player;
unsigned int mapId = 0;
u_char mapChanged = 0;
Sprite background;
Mesh ground;
int xaChannel = 0;

int feetCounter;
u_char cameraLock;
void camera_debug_input();
void zoneTo(int id, u_char *fileName, long camX, long camY, long camZ, short camRX, short camRY, short camRZ, long posX, long posY, long posZ);
void commands(u_long pad, u_long opad, Character *character);
void zones();
void startCommandMode();
void stopCommandMode();
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);

typedef struct {
	u_char type;
	u_char total;
} Mob;

typedef struct {
	Mob mobs[5];
	u_char total;
} WAVE;
u_char wave_index = 0;
u_char cWave = 0;
u_char n_waves = 3;
WAVE waves[3];

Mesh plane1,plane2;

void game_load(){
	//int i;
	long plane_pos[] = {0, 0, 0};
	short plane_size[] = {160, 0, -2000};
	long pos2[] = {-170, 0, -1500};
	short size2[] = {170, 0, -500};

	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;

	cd_open();
	cd_read_file("MISC_1.TIM", &cd_data[0]); // 640 256
	cd_read_file("TEX1.TIM", &cd_data[1]);
	cd_read_file("CUBE.TIM", &cd_data[2]);
	cd_read_file("TEX2.TIM", &cd_data[3]);
	cd_read_file("CUBE.OBJ", &cd_data[4]);
	cd_read_file("GUNSHOT.VAG", &cd_data[5]);
	cd_read_file("GROUND.OBJ", &cd_data[6]);

	cd_read_file("CHAR10.OBJ", &char1_animations[0][0]);
	cd_read_file("CHAR11.OBJ", &char1_animations[0][1]);
	cd_read_file("CHAR12.OBJ", &char1_animations[0][2]);

	cd_read_file("CHAR1F0.OBJ", &char1_animations[1][0]);
	cd_read_file("CHAR1F1.OBJ", &char1_animations[1][1]);
	cd_read_file("CHAR1F2.OBJ", &char1_animations[1][2]);
	cd_close();

	tpages[0] = loadToVRAM(cd_data[0]); // MISC_1
	tpages[2] = loadToVRAM(cd_data[1]); // TEX1
	tpages[3] = loadToVRAM(cd_data[2]); // CUBE
	tpages[4] = loadToVRAM(cd_data[3]); // TEX2 

	free3(cd_data[0]);
	free3(cd_data[1]);
	free3(cd_data[2]);
	free3(cd_data[3]);

	audio_init();
	audio_vag_to_spu((u_char*)cd_data[5], 15200, SPU_0CH);
	//free3(cd_data[5]);
	
	mesh_init(&cube, cd_data[4], tpages[3], 32, 50);
	free3(cd_data[4]);
	cube.pos.vx = -150;

	mesh_init(&ground, cd_data[6], tpages[4], 255, 500);
	free3(cd_data[6]);

	ui_init(tpages[0], SCREEN_WIDTH, SCREEN_HEIGHT);
	scene_add_sprite(&selector);
	scene_add_sprite(&dmg.sprite[0]);
	scene_add_sprite(&dmg.sprite[1]);
	scene_add_sprite(&dmg.sprite[2]);
	scene_add_sprite(&dmg.sprite[3]);

	init_balloon(&balloon, tpages[0], SCREEN_WIDTH, SCREEN_HEIGHT);
		
	sprite_init(&sprite_player, 64, 64, tpages[2]);
	sprite_set_uv(&sprite_player, 0, 0, 16, 16);

	enemy_push(tpages[4], BAT, 250, 300);
	enemy_push(tpages[4], BAT, 250, 0);
	//scene_add_sprite(&enemy_get(i)->sprite);
	//scene_add_sprite(&enemy_get(i)->blood);

	planeNode_push(plane_pos, plane_size, plane1);
	planeNode_push(pos2, size2, plane2);
	zoneTo(0, "BK1.TIM", 
	-185, 969, 3121, 185, -31, 0, 
	100, 0, -500);

	sprite_init(&background, 255, 255, tpages[1]);
	background.w = SCREEN_WIDTH;
	background.h = SCREEN_HEIGHT;

	character_1.HP = 80;
	character_1.HP_MAX = 80;
	character_1.MP = 20;
	character_1.MP_MAX = 20;

	char_animation_init(&character_1, 2);
	char_animation_set(&character_1, 0, 1, 3, char1_animations[0], tpages[2], 255, 300);
	character_1.meshAnimations[0].interval = 10;

	free3(char1_animations[0][0]);
	free3(char1_animations[0][1]);
	free3(char1_animations[0][2]);

	char_animation_set(&character_1, 1, 1, 3, char1_animations[1], tpages[2], 255, 150);
	character_1.meshAnimations[1].interval = 10;

	free3(char1_animations[1][0]);
	free3(char1_animations[1][1]);
	free3(char1_animations[1][2]);
}

void game_update()
{
	if(command_mode == 0)
	{
	
	if(balloon.display == 1)
	{
		if((opad & PADLcross) == 0 && pad & PADLcross)
			balloon.prev_display = 1;
		if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1)
			balloon.display = 0;
		return;
	}

#ifdef DEBUG
	if(pad & PADLtriangle && (opad & PADLtriangle) == 0)
		CAMERA_DEBUG = !CAMERA_DEBUG;
#endif
	if (CAMERA_DEBUG == 0){
		zones();
		if(mapChanged == 1){
			if((pad & PADLup) == 0 &&
			(pad & PADLdown) == 0 &&
			(pad & PADLleft) == 0 &&
			(pad & PADLright) == 0){
				mapChanged = 0;
			}
		}
		// player input
		character_1.play_animation = 0;
		if(mapChanged == 0){
			PlaneNode *node = planeNode;
			while(node != NULL){
				if(pad == (PADLup+PADLleft)){
					long z = character_1.pos.vz + 5;
					long x = character_1.pos.vx - 5;
					character_1.rot.vy = 1536;
					if(mesh_on_plane(x, z, node->data)){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if((pad == PADLup+PADLright)){
					long z = character_1.pos.vz + 5;
					long x = character_1.pos.vx + 5;
					character_1.rot.vy = 2560;
					if(mesh_on_plane(x, z, node->data)){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == (PADLdown+PADLleft)){
					long z = character_1.pos.vz - 5;
					long x = character_1.pos.vx - 5;
					character_1.rot.vy = 512; 
					if(mesh_on_plane(x, z, node->data)){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if((pad == PADLdown+PADLright)){
					long z = character_1.pos.vz - 5;
					long x = character_1.pos.vx + 5;
					character_1.rot.vy = 3584;
					if(mesh_on_plane(x, z, node->data)){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLup){
					long z = character_1.pos.vz + 10;
					character_1.rot.vy = 2048;
					if(mesh_on_plane(character_1.pos.vx, z, node->data)){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
					}
				}

				if(pad == PADLdown){
					long z = character_1.pos.vz - 10;
					character_1.rot.vy = 0;
					if(mesh_on_plane(character_1.pos.vx, z, node->data)){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLleft){
					long x = character_1.pos.vx - 10;
					character_1.rot.vy = 1024;
					if(mesh_on_plane(x, character_1.pos.vz, node->data)){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLright){
					long x = character_1.pos.vx + 10;
					character_1.rot.vy = 3072;
					if(mesh_on_plane(x, character_1.pos.vz, node->data)){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				node = node->next;
			}
		}
	} // end CAMERA_DEBUG == 0
	else {
		camera_debug_input();
	}
	
	cube.rot.vx += 10;
	cube.rot.vy += 10;
	cube.rot.vz += 10;
	startCommandMode();

	} // end commands_mode == 0
	else if(command_mode > 0)
	{
		EnemyNode *enemy_node = enemyNode;
		commands(pad, opad, &character_1);
		while(enemy_node != NULL) {
			Enemy *e = enemy_node->enemy;	
			enemy_update(e, *char_getMesh(character_1), command_mode, command_attack);
			if(e->attacking == 2){
				e->attacking = 3;
				character_1.HP -= 2;
				display_dmg(&dmg, char_getMesh(character_1)->pos, char_getMesh(character_1)->h*1.5, 2);
			}
			if(e->attacking == 3){
				if(dmg.display_time <= 0)
					e->attacking = 4;
			}
			//FntPrint("atb->%d\n\n", e->atb);
			//FntPrint("pos prepos->%d %d\n\n", e->sprite.pos.vx, e->prev_pos.vx);
			enemy_node = enemy_node->next;
		}
		if(opad == 0 && pad & PADLsquare){
			xaChannel = (xaChannel+1)%NUMCHANNELS;
			xa_play(xaChannel);
		}
	}
}

void game_draw(){
	short i = 0;
	EnemyNode *enemy_node = enemyNode;
	FntPrint("command mode %d\n", command_mode);
	if(command_mode == 0){
		if(CAMERA_DEBUG == 1){
			char log[100];
			sprintf(log, "x%ld y%ld z%ld rx%d ry%d rz%d\n\nx%ld y%ld z%ld\n",
			camera.pos.vx, camera.pos.vy, camera.pos.vz,
			camera.rot.vx, camera.rot.vy, camera.rot.vz,
			character_1.pos.vx, character_1.pos.vy, character_1.pos.vz);
			FntPrint(log);
			if(planeNode != NULL){
				PlaneNode *node = planeNode;
				while(node != NULL){
					drawMesh(&node->data, 1023);
					node = node->next;
				}
			}
		}

		drawSprite_2d(&background, 1023);
		if(mapId != 2){
			drawMesh(&cube, NULL);
		}

		char_draw(&character_1, NULL, drawMesh);

		if(balloon.display == 1){
			Font font;
			drawFont(&font, balloon.text, balloon.sprite.pos.vx + 10, balloon.sprite.pos.vy + 10, 1);
			drawSprite_2d(&balloon.sprite, NULL);
		}
	}

	if(command_mode > 0)
	{
		Font font1;
		Font font2;
		char str_hp_mp[100];
		drawMesh(&ground, 1023);

		char_draw(&character_1, NULL, drawMesh);

		while(enemy_node != NULL) {
			Enemy *e = enemy_node->enemy;	
			if(e->sprite.hp > 0)
				drawSprite(&e->sprite, NULL);
			if(e->sprite.hitted == 1)
				drawSprite(&e->blood, NULL);
			enemy_node = enemy_node->next;
		}

		drawSprite_2d(&atb[0].bar, NULL);
		drawSprite_2d(&atb[0].border, NULL);

		if(command_mode == 1 && atb[0].bar.w >= 50 && ENEMY_ATTACKING == 0)
			drawSprite_2d(&selector, NULL);
		if(command_mode == 2 && atb[0].bar.w >= 50)
			drawSprite(&selector, 1);

		drawFont(&font1, "Attack\nMagic\nSkill\nItem", 20, 190, 0);
		sprintf(str_hp_mp, "HP %d/%d MP %d/%d", 
		character_1.HP,
		character_1.HP_MAX,
		character_1.MP,
		character_1.MP_MAX);
		drawFont(&font2, str_hp_mp, 105, 190, 0);
		drawSprite_2d(&command_bg, NULL);

		if(dmg.display_time > 0){
			for(i = 0; i < 4; i++){
				drawSprite(&dmg.sprite[i], NULL);
				dmg.sprite[i].pos.vy -= 3;
			}
			dmg.display_time -= 2;
		}
	}
}

void commands(u_long pad, u_long opad, Character *character) {
	int i = 0;
	if(atb[0].bar.w < 50 && ENEMY_ATTACKING == 0){
		atb[0].value += 0.2;
		atb[0].bar.w = (int)atb[0].value;
	}
	else {
		if(command_attack == 0)
		{
			if(command_mode == 1 && ENEMY_ATTACKING == 0) 
			{
				if(pad & PADLup && (opad & PADLup) == 0){
					if(command_index > 0)
						command_index--;
				}
				if(pad & PADLdown && (opad & PADLdown) == 0){
					if(command_index < 3)
						command_index++;
				}
				if(pad & PADLcross && (opad & PADLcross) == 0){
					reset_targets();
					command_mode = 2;
					command_index = 0;
				}
				if(pad & PADLcircle && (opad & PADLcircle) == 0){
					stopCommandMode();
					closeCommandMenu();
					camera = prevCamera;
					command_mode = 0;
					atb[0].value = 0;
					atb[0].bar.w = 0;
				}

				selector.pos.vy = SELECTOR_POSY+(17*command_index);
			}
		}
	}

	if(command_attack == 1 && enemy_target != NULL)
	{
		u_char moving = 0;
		int speed = 12;

		if(character->pos.vx + (char_getMesh(*character)->w/2) < enemy_target->sprite.pos.vx){
			character->pos.vx += speed;
			moving = 1;
		}
		if(character->pos.vz + (char_getMesh(*character)->w*2) > enemy_target->sprite.pos.vz){
			character->pos.vz -= speed;
			moving = 1;
		}
		if(character->pos.vz + (char_getMesh(*character)->w*2) < enemy_target->sprite.pos.vz){
			character->pos.vz += speed;
			moving = 1;
		}

		if(moving == 0){
			char_play_animation(character, 1);
			if(char_animation_is_over(*character) == 1){
				enemy_target->sprite.hp -= 8;	
				enemy_target->sprite.hitted = 1;	
				enemy_target->blood.pos.vx = enemy_target->sprite.pos.vx;
				enemy_target->blood.pos.vy = enemy_target->sprite.pos.vy;
				enemy_target->blood.pos.vz = enemy_target->sprite.pos.vz-5;
				enemy_target->blood.frame = 0;
				
				display_dmg(&dmg, enemy_target->sprite.pos, enemy_target->sprite.h, 8);

				mainCommandMenu();
				closeCommandMenu();
				command_attack = 2;
			}
		}
	}

	if(command_attack == 2 && dmg.display_time <= 0){
		u_char moving = 0;
		int speed = 12;
		if(character->pos.vx > character_1.battle_pos.vx){
			character->pos.vx -= speed;
			moving = 1;
		}
		if(character->pos.vz > character_1.battle_pos.vz){
			character->pos.vz -= speed;
			moving = 1;
		}
		if(character->pos.vz < character_1.battle_pos.vz){
			character->pos.vz += speed;
			moving = 1;
		}
		if(moving == 0)
			command_attack = 0;
	}

	// select enemy logic
	if(command_attack == 0 && (command_mode == 2))
	{
		if(pad & PADLcross && (opad & PADLcross) == 0 && target_counter > 0)
		{
			atb[0].value = 0;
			atb[0].bar.w = 0;
			command_attack = 1;
			return;
		}

		if(pad & PADLcircle)
			mainCommandMenu();
		else
		{		
			EnemyNode *enemy_node = enemyNode;
			selector.w = 60;
			selector.h = 60;
			
			if(calc_targets == 0){
				calc_targets = 1;
				while(enemy_node != NULL){
					Enemy *enemy = enemy_node->enemy;
					if(enemy->sprite.hp > 0) {
						targets[target_counter] = i;	
						//printf("right t %d \n", targets[target_counter]);
						target_counter++;
					}
					i++;
					enemy_node = enemy_node->next;
				}
				//printf("target %d \n", targets[0]);
				//printf("target_counter %d \n", target_counter);
			}
			if(target_counter > 0)
			{
				Enemy *enemy;
				if((pad & PADLleft && (opad & PADLleft) == 0) || (pad & PADLup && (opad & PADLup) == 0))
				{
					if(target == 0)
						target = target_counter-1;
					else
						target--;
				}
				if((pad & PADLright && (opad & PADLright) == 0) || (pad & PADLdown && (opad & PADLdown) == 0))
				{
					target++;
					if(target >= target_counter)
						target = 0;
				}

				enemy = enemy_get(targets[target]);
				enemy_target = enemy;
				if(enemy != NULL){
					selector.pos.vx = enemy->sprite.pos.vx - enemy->sprite.w;
					selector.pos.vy = enemy->sprite.pos.vy;
					if(command_mode == 2)
						selector.pos.vz = enemy->sprite.pos.vz;
				}
			}
			else
			mainCommandMenu();
		}
	}
}

void camera_debug_input(){
	if(pad & PADLcross){
		if(pad & PADLleft)
			camera.rot.vy -= 1;
		if(pad & PADLright)
			camera.rot.vy += 1;
		if(pad & PADLup)
			camera.rot.vx += 1;
		if(pad & PADLdown)
			camera.rot.vx -= 1;
	}
	else if(pad & PADLsquare){
		if(pad & PADLup)
			camera.pos.vz += 1;
		if(pad & PADLdown)
			camera.pos.vz -= 1;
	}
	else {
		if(pad & PADLleft)
			camera.pos.vx -= 1;
		if(pad & PADLright)
			camera.pos.vx += 1;
		if(pad & PADLup)
			camera.pos.vy += 1;
		if(pad & PADLdown)
			camera.pos.vy -= 1;
	}
}

void zoneTo(int id, u_char *fileName, long camX, long camY, long camZ, short camRX, short camRY, short camRZ, long posX, long posY, long posZ){
	cd_open();
	cd_read_file(fileName, &bk_buffer[0]);
	/*cd_read_file(fileName, &bk_buffer[1]);
	cd_read_file(fileName, &bk_buffer[2]);
	cd_read_file(fileName, &bk_buffer[3]);*/
	cd_close();
	mapChanged = 1;
	mapId = id;
	clearVRAM_at(320, 0, 256, 256);
	tpages[1] = loadToVRAM(bk_buffer[0]);
	background.tpage = tpages[1];
	camera.pos.vx = camX;
	camera.pos.vz = camZ;
	camera.pos.vy = camY;
	camera.rot.vx = camRX;
	camera.rot.vy = camRY;
	camera.rot.vz = camRZ;
	character_1.pos.vx = posX;
	character_1.pos.vy = posY;
	character_1.pos.vz = posZ;
	free3(bk_buffer[0]);
	/*free3(bk_buffer[1]);
	free3(bk_buffer[2]);
	free3(bk_buffer[3]);*/
}

void zones(){
	if(mapId == 0 && character_1.pos.vz <= -1990){
		long pos[] = {0, 0, 0};
		short size[] = {230, 0, -1200};
		planeNode_free();
		planeNode_push(pos, size, plane1);
		zoneTo(1,"BK2.TIM", 
		-461, 942, 2503, 160, 195, 0, 
		80, 0, -1000);
		character_1.rot.vy = 2048;
	}
	if(mapId == 0 && character_1.pos.vz <= -1500 && character_1.pos.vx <= -160){
		long pos[] = {0, 0, -800};
		short size[] = {80, 0, -600};
		planeNode_free();
		planeNode_push(pos, size, plane1);
		zoneTo(2,"BK3.TIM", 
		-15, 886, 2542, 159, -73, 0, 
		40, 0, -1200);
		character_1.rot.vy = 2048;
	}
	if(mapId == 1 && character_1.pos.vz <= -1190){
		long plane_pos[] = {0, 0, 0};
		short plane_size[] = {160, 0, -2000};
		long pos2[] = {-170, 0, -1500};
		short size2[] = {170, 0, -500};
		planeNode_free();
		planeNode_push(plane_pos, plane_size, plane1);
		planeNode_push(pos2, size2, plane2);
		zoneTo(0, "BK1.TIM", 
		-185, 969, 3121, 185, -31, 0, 
		100, 0, -1900);
		character_1.rot.vy = 2048;
	}
	if(mapId == 2 && character_1.pos.vz < -1350){
		long plane_pos[] = {0, 0, 0};
		short plane_size[] = {160, 0, -2000};
		long pos2[] = {-170, 0, -1500};
		short size2[] = {170, 0, -500};
		planeNode_free();
		planeNode_push(plane_pos, plane_size, plane1);
		planeNode_push(pos2, size2, plane2);
		zoneTo(0, "BK1.TIM", 
		-185, 969, 3121, 185, -31, 0, 
		-40, 0, -1500);
		character_1.rot.vy = 1024*3;
	}
	if(mesh_collision(*char_getMesh(character_1), cube) == 1){
		if(pad & PADLcross && ((opad & PADLcross) == 0) && 
		char_looking_at(&character_1, cube.pos.vx, cube.pos.vz) == 1){
			set_balloon(&balloon, "uno strano cubo...");
		}
	}
}

void startCommandMode(){
	if(pad & PADR1 && (opad & PADR1) == 0){
		command_mode = 1;
		prevCamera = camera;
		camera.pos.vx = -600;
		camera.pos.vz = 2300;
		camera.pos.vy = 900;
		camera.rot.vx = 200;
		camera.rot.vy = 200;
		camera.rot.vz = 0;

		// saving the current char position in the map view
		character_1.map_pos = character_1.pos;
		character_1.map_rot = character_1.rot;

		// place character in battle position 
		character_1.battle_pos.vx = -200;
		character_1.battle_pos.vy = 0;
		character_1.battle_pos.vz = 0;
		character_1.battle_rot.vx = 0;
		character_1.battle_rot.vy = 3072;
		character_1.battle_rot.vz = 0;
		// set the pos to battle position
		character_1.pos = character_1.battle_pos;
		character_1.rot = character_1.battle_rot;
		character_1.animation_to_play = 1;
		xaChannel = 0;
		xa_play(xaChannel);
	}
}

void stopCommandMode(){
	character_1.pos = character_1.map_pos;
	character_1.rot = character_1.map_rot;
	character_1.animation_to_play = 0;
	xa_pause();
}

Enemy* ray_collisions(Sprite *s, long cameraX)
{
	int i = 0, distance = 10000, k = 0, index = 0;
	EnemyNode *enemy_node = enemyNode;
	EnemyNode *enemy_node2 = enemyNode;
	while(enemy_node != NULL){
		int collision = 0;
		Enemy *enemy = enemy_node->enemy;
		if(enemy->sprite.hp > 0)
			collision = ray_collision(s, &enemy->sprite, cameraX);
		if(collision == 1){
			index = i;
			while(enemy_node2 != NULL){
				Enemy *enemy2 = enemy_node2->enemy;
				if(s->direction == 0){
					if(enemy2->sprite.hp > 0 && (s->pos.vx - enemy2->sprite.pos.vx) < distance){
						if(ray_collision(s, &enemy2->sprite, cameraX) == 1){
							distance = s->pos.vx - enemy2->sprite.pos.vx;
							index = k;
						}
					}	
				}
				if(s->direction == 1){
					if(enemy2->sprite.hp > 0 && (enemy2->sprite.pos.vx - s->pos.vx) < distance){
						if(ray_collision(s, &enemy2->sprite, cameraX) == 1){
							distance = enemy2->sprite.pos.vx - s->pos.vx;
							index = k;
						}
					}	
				}
				k++;
				enemy_node2 = enemy_node2->next;
			}
			enemy = enemy_get(index);
			enemy->sprite.hitted = 1;
			enemy->sprite.hp -= 1;
			enemy->blood.pos.vx = enemy->sprite.pos.vx;
			enemy->blood.pos.vy = enemy->sprite.pos.vy;
			enemy->blood.pos.vz = enemy->sprite.pos.vz-5;
			enemy->blood.frame = 0;
			return enemy; 
		}
		i++;
		enemy_node = enemy_node->next;
	}
	return NULL;
}

int ray_collision(Sprite *s1, Sprite *s2, long cameraX){
	if(s2->pos.vx > (cameraX*-1) -850 && s2->pos.vx < (cameraX*-1) +850) 
	{
		if((s1->direction == 1 && s1->pos.vx < s2->pos.vx) || (s1->direction == 0 && s1->pos.vx > s2->pos.vx))
		{
			if(s1->pos.vz+(s1->h) >= s2->pos.vz-(s2->h) && s1->pos.vz <= s2->pos.vz+(s2->h))
				return 1;
		}
	}
	return 0;
}
