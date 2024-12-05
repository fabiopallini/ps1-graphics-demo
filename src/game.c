#include "game.h"
#include "xa.h"
#include "utils.h"
#include "enemy.h"
#include "ui.h"
#include "battle.h"
#include "char.h"
#include "stages.h"

#define CAMERA_DEBUG_SPEED 5
#define DEBUG
u_char CAMERA_DEBUG = 0;

u_long *cd_data[4];
u_short tpages[1];
u_short tpage_misc1;
Stage *stage;
Mesh cube;
Camera prevCamera;
Character character_1;
u_long *char1_animations[2][5];

u_char loading_stage = 0;
int stage_id_to_load, spawn_id_to_load;
u_char mapChanged = 0;
Background background;

Battle *battle;
Mesh ground;
//int xaChannel = 0;

void camera_debug_input();
void load_stage(int stage_id, int spawn_id);
void startBattle();
void stopBattle();
void stopBattle();
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
void zones_collision(const Stage *stage, const Character *c);

typedef struct {
	u_char type;
	u_char total;
} Mob;

typedef struct {
	Mob mobs[5];
	u_char total;
} WAVE;

void game_load(){
	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;

	cd_open();
	cd_read_file("MISC_1.TIM", &cd_data[0]);
	cd_read_file("TEX2.TIM", &cd_data[1]);
	cd_read_file("CUBE.OBJ", &cd_data[2]);
	cd_read_file("GROUND.OBJ", &cd_data[3]);

	cd_read_file("C1RUN0.OBJ", &char1_animations[0][0]);
	cd_read_file("C1RUN1.OBJ", &char1_animations[0][1]);
	cd_read_file("C1RUN2.OBJ", &char1_animations[0][2]);
	cd_read_file("C1RUN3.OBJ", &char1_animations[0][3]);
	cd_read_file("C1RUN4.OBJ", &char1_animations[0][4]);

	cd_read_file("C1ATT0.OBJ", &char1_animations[1][0]);
	cd_read_file("C1ATT1.OBJ", &char1_animations[1][1]);
	cd_read_file("C1ATT2.OBJ", &char1_animations[1][2]);

	tpage_misc1 = loadToVRAM(cd_data[0]); // MISC_1
	tpages[0] = loadToVRAM(cd_data[1]); // TEX2 

	free3(cd_data[0]);
	free3(cd_data[1]);

	spu_init();
	vag_load("AERITH.VAG", SPU_0CH);
	//spu_load(cd_data[5], 15200, SPU_0CH);
	//free3(cd_data[5]);
	
	mesh_init(&cube, cd_data[2], tpage_misc1, 255, 30);
	free3(cd_data[2]);
	cube.pos.vx = 150;
	cube.pos.vy = -50;
	cube.pos.vz = -600;

	mesh_init(&ground, cd_data[3], tpages[0], 255, 500);
	free3(cd_data[3]);

	init_ui(tpage_misc1, SCREEN_WIDTH, SCREEN_HEIGHT);
	battle = malloc3(sizeof(Battle));
	init_battle(battle, tpage_misc1, SCREEN_WIDTH, SCREEN_HEIGHT);
	scene_add_sprite(&battle->selector);
	scene_add_sprite(&battle->dmg.sprite[0]);
	scene_add_sprite(&battle->dmg.sprite[1]);
	scene_add_sprite(&battle->dmg.sprite[2]);
	scene_add_sprite(&battle->dmg.sprite[3]);
		
	//enemy_push(tpages[3], BAT, 250, 300);
	//enemy_push(tpages[3], BAT, 250, 0);

	//Enemy enemy[2];
	//enemy_init(enemy[0], tpages[3], BAT);
	//node_push(enemyNode, enemy[0]);
	//scene_add_sprite(&enemy_get(i)->sprite);
	//scene_add_sprite(&enemy_get(i)->blood);
	
	stage = malloc3(sizeof(Stage));
	load_stage(2, 1);

	background_init(&background);
	/*sprite_init(&background.sprite_effect, 64, 255, tpages[0]);
	sprite_set_uv(&background.sprite_effect, 128, 128, 127, 127);
	sprite_set_rgb(&background.sprite_effect, 20, 20, 20, 1);*/

	character_1.HP = 80;
	character_1.HP_MAX = 80;
	character_1.MP = 20;
	character_1.MP_MAX = 20;
	character_1.RUN_SPEED = 5;

	char_animation_init(&character_1, 2);
	char_animation_set(&character_1, 0, 1, 5, char1_animations[0], tpage_misc1, 255, 100);
	character_1.meshAnimations[0].interval = 7;

	free3(char1_animations[0][0]);
	free3(char1_animations[0][1]);
	free3(char1_animations[0][2]);
	free3(char1_animations[0][3]);

	char_animation_set(&character_1, 1, 1, 3, char1_animations[1], tpage_misc1, 255, 100);
	character_1.meshAnimations[1].interval = 10;

	free3(char1_animations[1][0]);
	free3(char1_animations[1][1]);
	free3(char1_animations[1][2]);	
}

void game_update()
{
	if(battle->command_mode == 0 && !loading_stage)
	{
	
	if(balloon.display == 1)
	{
		if((opad & PADLcross) == 0 && pad & PADLcross){
			balloon.page_index++;
			if(balloon.page_index >= balloon.pages_length){
				balloon.prev_display = 1;
				balloon.page_index = 0;
			}
			else{
				set_balloon(&balloon, stage->npcs[balloon.npc_id].talk_chars[balloon.page_index]);
				//set_balloon(&balloon, stage->npcs[1].talk_chars[balloon.page_index]);
			}
		}
		if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1)
			balloon.display = 0;
		return;
	}

#ifdef DEBUG
	if(pad & PADLtriangle && (opad & PADLtriangle) == 0){
		CAMERA_DEBUG = !CAMERA_DEBUG;
	}
#endif
	if (CAMERA_DEBUG == 0)
	{
		zones_collision(stage, &character_1);
		//char_set_color(character_1, 50, 50, 50);
		//char_set_shadeTex(character_1, 1);
	
		if(pad & PADLcross && ((opad & PADLcross) == 0)){
			int i = 0;
			for(i = 0; i < stage->npcs_len; i++)
			{
				Npc *npc = &stage->npcs[i];
				if(bbox_collision(character_1.pos.vx, character_1.pos.vz, npc->bbox) &&
				char_looking_at(&character_1, npc->mesh.pos.vx, npc->mesh.pos.vz) == 1)
				{
					balloon.npc_id = i;
					balloon.pages_length = npc->talk_pages;
					set_balloon(&balloon, npc->talk_chars[balloon.page_index]);
					break;
				}
			}
		}

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
			int i = 0;
			for(i = 0; i < stage->planes_length; i++){
				if(pad == (PADLup+PADLleft)){
					long z = character_1.pos.vz + character_1.RUN_SPEED/2;
					long x = character_1.pos.vx - character_1.RUN_SPEED/2;
					character_1.rot.vy = 1536;
					if(mesh_on_plane(x, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if((pad == PADLup+PADLright)){
					long z = character_1.pos.vz + character_1.RUN_SPEED/2;
					long x = character_1.pos.vx + character_1.RUN_SPEED/2;
					character_1.rot.vy = 2560;
					if(mesh_on_plane(x, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == (PADLdown+PADLleft)){
					long z = character_1.pos.vz - character_1.RUN_SPEED/2;
					long x = character_1.pos.vx - character_1.RUN_SPEED/2;
					character_1.rot.vy = 512; 
					if(mesh_on_plane(x, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if((pad == PADLdown+PADLright)){
					long z = character_1.pos.vz - character_1.RUN_SPEED/2;
					long x = character_1.pos.vx + character_1.RUN_SPEED/2;
					character_1.rot.vy = 3584;
					if(mesh_on_plane(x, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLup){
					long z = character_1.pos.vz + character_1.RUN_SPEED;
					character_1.rot.vy = 2048;
					if(mesh_on_plane(character_1.pos.vx, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
					}
				}

				if(pad == PADLdown){
					long z = character_1.pos.vz - character_1.RUN_SPEED;
					character_1.rot.vy = 0;
					if(mesh_on_plane(character_1.pos.vx, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLleft){
					long x = character_1.pos.vx - character_1.RUN_SPEED;
					character_1.rot.vy = 1024;
					if(mesh_on_plane(x, character_1.pos.vz, stage->planes[i])){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
				if(pad == PADLright){
					long x = character_1.pos.vx + character_1.RUN_SPEED;
					character_1.rot.vy = 3072;
					if(mesh_on_plane(x, character_1.pos.vz, stage->planes[i])){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
					}
				}
			}
		}
	} // end CAMERA_DEBUG == 0
	else if(!loading_stage){
		camera_debug_input();
	}
	
	if(stage->id == 2){
		cube.rot.vx += 10;
		cube.rot.vy += 10;
		cube.rot.vz += 10;
	}
	if(pad & PADR1 && (opad & PADR1) == 0){
		battle->status = 1;
		scene_load(startBattle);
	}

	} // end commands_mode == 0
	else if(battle->command_mode > 0 && !loading_stage)
	{
		battle_update(battle, pad, opad, &character_1);
		//if(battle->status == 2){
		if(pad & PADR1 && (opad & PADR1) == 0){
			battle->status = 0;
			scene_load(stopBattle);
		}

		if(enemyNode != NULL) {
			EnemyNode *node = enemyNode;
			while(node != NULL){
				Enemy *e = node->enemy;	
				enemy_update(e, *char_getMesh(&character_1), battle->command_mode, battle->command_attack);
				if(e->attacking == 2){
					e->attacking = 3;
					character_1.HP -= 2;
					display_dmg(&battle->dmg, char_getMesh(&character_1)->pos, char_getMesh(&character_1)->size*1.5, 2);
				}
				if(e->attacking == 3){
					if(battle->dmg.display_time <= 0)
						e->attacking = 4;
				}
				//FntPrint("atb->%d\n\n", e->atb);
				//FntPrint("pos prepos->%d %d\n\n", e->sprite.pos.vx, e->prev_pos.vx);
				node = node->next;
			}
		}
	}
	
	if(loading_stage)
		load_stage(stage_id_to_load, spawn_id_to_load);
}

void game_draw(){
	short i = 0;
	if(!battle->command_mode){
		if(CAMERA_DEBUG == 1){
			char log[100];
			sprintf(log, "x%ld y%ld z%ld rx%d ry%d rz%d\n\nx%ld y%ld z%ld\n",
			camera.pos.vx, camera.pos.vy, camera.pos.vz,
			camera.rot.vx, camera.rot.vy, camera.rot.vz,
			character_1.pos.vx, character_1.pos.vy, character_1.pos.vz);
			FntPrint(log);
			for(i = 0; i < stage->zones_length; i++)
				drawMesh(&stage->zones[i].mesh, OTSIZE-1);
			for(i = 0; i < stage->planes_length; i++)
				drawMesh(&stage->planes[i], OTSIZE-1);
		}

		background_draw(&background, OTSIZE-1, drawSprite_2d);
		//drawSprite_2d(&background.sprite_effect, 0);

		if(stage->id == 2){
			drawMesh(&cube, NULL);
		}

		for(i = 0; i < stage->npcs_len; i++){
			drawMesh(&stage->npcs[i].mesh, NULL);
#ifdef DEBUG
			if(stage->npcs[i].bbox.poly_f4 != NULL)
				add_bbox_prims(&stage->npcs[i].bbox);
#endif
		}

		char_draw(&character_1, NULL, drawMesh);

		if(balloon.display == 1){
			Font font;
			drawFont(&font, balloon.text, balloon.sprite.pos.vx + 10, balloon.sprite.pos.vy + 10, 1);
			drawSprite_2d(&balloon.sprite, 0);
		}
	}
	else {
		Font font1;
		Font font2;
		char str_hp_mp[100];

		battle_draw(battle, drawSprite, drawSprite_2d, OTSIZE);
		drawSprite_2d(&battle->atb[0].bar, OTSIZE-1);
		drawSprite_2d(&battle->atb[0].border, OTSIZE-1);

		drawFont(&font1, "Attack\nMagic\nSkill\nItem", 20, 190, 0);
		sprintf(str_hp_mp, "HP %d/%d MP %d/%d", 
		character_1.HP,
		character_1.HP_MAX,
		character_1.MP,
		character_1.MP_MAX);
		drawFont(&font2, str_hp_mp, 105, 190, 0);
		drawSprite_2d(&battle->command_bg, OTSIZE-1);
		
		char_draw(&character_1, OTSIZE-1, drawMesh);
		if(enemyNode != NULL) {
			EnemyNode *node = enemyNode;
			while(node != NULL){
				Enemy *e = node->enemy;	
				if(e->sprite.hitted == 1)
					drawSprite(&e->blood, OTSIZE-1);
				if(e->sprite.hp > 0)
					drawSprite(&e->sprite, OTSIZE-1);
				node = node->next;
			}
		}

		drawMesh(&ground, OTSIZE-1);
	}
}

void camera_debug_input(){
	if(pad & PADLcross){
		if(pad & PADLleft)
			camera.rot.vy -= CAMERA_DEBUG_SPEED;
		if(pad & PADLright)
			camera.rot.vy += CAMERA_DEBUG_SPEED;
		if(pad & PADLup)
			camera.rot.vx += CAMERA_DEBUG_SPEED;
		if(pad & PADLdown)
			camera.rot.vx -= CAMERA_DEBUG_SPEED;
	}
	else if(pad & PADLsquare){
		if(pad & PADLup)
			camera.pos.vz += CAMERA_DEBUG_SPEED;
		if(pad & PADLdown)
			camera.pos.vz -= CAMERA_DEBUG_SPEED;
	}
	else {
		if(pad & PADLleft)
			camera.pos.vx -= CAMERA_DEBUG_SPEED;
		if(pad & PADLright)
			camera.pos.vx += CAMERA_DEBUG_SPEED;
		if(pad & PADLup)
			camera.pos.vy += CAMERA_DEBUG_SPEED;
		if(pad & PADLdown)
			camera.pos.vy -= CAMERA_DEBUG_SPEED;
	}
}

const u_char *read_str_delimiter(const u_char* ptr, u_char delimiter) {
    while (*ptr && *ptr != delimiter) {
        ptr++;
    }
    return ptr;
}

void read_stage_data(int stage_id){
	u_long *stages_data;
	u_char *data;
	int line_n = 0;
	cd_read_file("STAGES.DAT", &stages_data);
	data = (u_char*)stages_data;
	if(data != NULL)
	{
		const u_char *ptr = data; 
		while (*ptr) 
		{ 
			//size_t line_length;
			u_char line[100];
			/*const u_char *end = ptr;
			while (*end && *end != '\n')
				end++;*/
			//const u_char *end = read_str_delimiter(ptr, '\n');	
			//line_length = end - ptr;
			size_t line_length = strlen_delimiter(ptr, '\n');
			strncpy(line, ptr, line_length);
			printf("line: %s\n", line);
			if(line_n == stage_id)
			{
				const u_char *line_ptr = line;
				while (*line_ptr && *line_ptr != '\n')
				{
					int value_length;
					u_char value[100];
					/*u_char *c = line_ptr;
					while (*c != ';' && *c != '\0') {
						c++;
					}
					const u_char *c = read_str_delimiter(line_ptr, ';');
					value_length = c - line_ptr;*/
					value_length = strlen_delimiter(line_ptr, ';');
					printf("value_length: %d\n", value_length);

					if (value_length < sizeof(value)) {
						memcpy(value, line_ptr, value_length);
						value[value_length] = '\0';
						printf("value: %s\n", value);
					} else {
						printf("value is too long to be stored in the buffer\n");
					}

					//line_ptr = (*c == ';') ? c + 1 : c;
					line_ptr += value_length + 1;
				}
			}
			line_n++;
			//ptr = (*end == '\n') ? end + 1 : end;
			ptr += line_length + 1;
		}
	}
} 

void load_stage(int stage_id, int spawn_id){
	u_long *stages_buffer;
	int stage_addr = 0;
	u_long *bk_buffer[2];
	int i,j = 0;
	size_t byte_cursor = 0;
	/*
 	mesh vertices order
 		3----4 
		|    |
		|    |
 		1----2 
	*/
	const u_char *vertices = "v -1.000000 0.000000 -1.000000\n
v 1.000000 0.000000 -1.000000\n
v -1.000000 0.000000 1.000000\n
v 1.000000 0.000000 1.000000\n
vt 0.000000 0.000000\n
vt 1.000000 0.000000\n
vt 1.000000 1.000000\n
vt 0.000000 1.000000\n
s 0\n
f 1/1 2/2 4/3 3/4\n
"; 

	unsigned long start_sector, end_sector;
	unsigned long start_byte, end_byte;
	unsigned long bytes_to_read, offset, size_to_copy;

	if(stage_id > 0)
		stage_addr = stages_byte_addr[stage_id-1];

	start_sector = stage_addr / SECTOR;
	end_sector = (stages_byte_addr[stage_id] + SECTOR - 1) / SECTOR;

	start_byte = start_sector * SECTOR;
	end_byte = end_sector * SECTOR;
	bytes_to_read = end_byte - start_byte;

	cd_read_file_bytes("STAGES.BIN", &stages_buffer, start_byte, end_byte, NULL);
	offset = stage_addr - start_byte;
	size_to_copy = stages_byte_addr[stage_id] - stage_addr;
/*
===========================================================================================
				CLEANUP PREVIOUS STAGE DATA	
===========================================================================================
*/
	if(stage != NULL){
		for(i = 0; i < stage->planes_length; i++){
			mesh_free(&stage->planes[i]);
		}
		for(i = 0; i < stage->zones_length; i++){
			mesh_free(&stage->zones[i].mesh);
		}
		for (j = 0; j < stage->npcs_len; j++) {
			Npc *npc = &stage->npcs[j];
			npc_free(npc);
		}
	}
	memset(stage, 0, sizeof(Stage));
	memset(&stageData, 0, sizeof(StageData));
/*
===========================================================================================
					LOAD NEW STAGE DATA	
===========================================================================================
*/
	memcpy(&stageData, (u_char *)stages_buffer + offset, sizeof(StageData));
	//memcpy(&stageData, (u_char *)stages_buffer + offset, size_to_copy);

	stage->id = stage_id;
	//printf("tim 0 %s\n", stageData.tims[0]);
	//printf("tim 1 %s\n", stageData.tims[1]);
	stage->tims[0] = stageData.tims[0];
	stage->tims[1] = stageData.tims[1];
	stage->camera_pos.vx = stageData.cam_x;
	stage->camera_pos.vy = stageData.cam_y;
	stage->camera_pos.vz = stageData.cam_z;
	stage->camera_rot.vx = stageData.cam_rx;
	stage->camera_rot.vy = stageData.cam_ry;
	stage->camera_rot.vz = stageData.cam_rz;

	stage->planes_length = stageData.planesData_len;
	for(i = 0; i < stage->planes_length; i++){
		PlaneData *p = &stageData.planesData[i];
		mesh_init(&stage->planes[i], (u_long*)vertices, NULL, 0, 1);
		mesh_set_color(&stage->planes[i], 0, 128, 0, 1);
		stage->planes[i].vertices[1].vx = p->w;
		stage->planes[i].vertices[3].vx = p->w;
		stage->planes[i].vertices[0].vz = p->d;
		stage->planes[i].vertices[1].vz = p->d;
		stage->planes[i].pos.vx = p->x;
		stage->planes[i].pos.vy = p->y;
		stage->planes[i].pos.vz = p->z;
	}

	stage->spawns_length = stageData.spawnsData_len;
	for(i = 0; i < stage->spawns_length; i++){
		SpawnData *sp = &stageData.spawnsData[i];
		stage->spawns[i].pos.vx = sp->x;
		stage->spawns[i].pos.vy = sp->y; 
		stage->spawns[i].pos.vz = sp->z;
		stage->spawns[i].rot.vx = sp->rx; 
		stage->spawns[i].rot.vy = sp->ry; 
		stage->spawns[i].rot.vz = sp->rz; 
	}

	stage->zones_length = stageData.zonesData_len;
	for(i = 0; i < stage->zones_length; i++){
		ZoneData *z = &stageData.zonesData[i];
		zone_init(&stage->zones[i], 
			z->x, z->y, z->z, 
			z->w, z->h, z->d,
			z->stage_id, z->spawn_id
		);
	}

	stage->npcs_len = stageData.npcsData_len;
	for (j = 0; j < stage->npcs_len; j++) {
		Npc *npc = &stage->npcs[j];
		u_long *cd_obj;
		cd_read_file("OSVALDO.OBJ", &cd_obj);
		npc_init(npc, cd_obj, tpage_misc1, &stageData.npcData[j]);
		bbox_init(&npc->bbox, &npc->mesh);
		free3(cd_obj);
		npc->talk_pages = stageData.npcData[j].talk_pages;
		npc->talk_chars = malloc3(npc->talk_pages * sizeof(char*));
		if (!npc->talk_chars) {
			printf("Error on malloc3 npc->talk_chars\n");
			return;
		}
		for (i = 0; i < npc->talk_pages; i++) {
			size_t len = strlen((u_char *)stages_buffer + offset + sizeof(StageData) + byte_cursor);
			npc->talk_chars[i] = malloc3((len+1) * sizeof(char));
			if (!npc->talk_chars[i]) {
				printf("Error on malloc3 npc.talk_chars[x]\n");
				return;
			}
			strcpy(npc->talk_chars[i], (u_char *)stages_buffer + offset + sizeof(StageData) + byte_cursor);
			byte_cursor += (len+1) * sizeof(char);
		}
		// TEST 
		/*for (i = 0; i < npc->talk_pages; i++) {
			printf("--->npc->talk_char[%d] --> %s\n", i, npc->talk_chars[i]);
		}*/
	}
/*
===========================================================================================
					LOAD STAGE	
===========================================================================================
*/
	cd_read_file(stage->tims[0], &bk_buffer[0]);
	cd_read_file(stage->tims[1], &bk_buffer[1]);
	mapChanged = 1;
	clearVRAM_at(320, 0, 256, 256);
	background.tpages[0] = loadToVRAM(bk_buffer[0]);
	background.tpages[1] = loadToVRAM(bk_buffer[1]);
	memcpy(&camera.pos, &stage->camera_pos, sizeof(stage->camera_pos));
	memcpy(&camera.rot, &stage->camera_rot, sizeof(stage->camera_rot));
	memcpy(&character_1.pos, &stage->spawns[spawn_id].pos, sizeof(stage->spawns[spawn_id].pos));
	memcpy(&character_1.rot, &stage->spawns[spawn_id].rot, sizeof(stage->spawns[spawn_id].rot));

	//background.sprite_effect.pos = character_1.pos;
	//background.sprite_effect.pos.vz = character_1.pos.vz + 800;
	background.sprite_effect.pos.vy -= 100; 
	background.sprite_effect.pos.vx = 130; 
	background.sprite_effect.pos.vz = 0; 

	free3(stages_buffer);
	free3(bk_buffer[0]);
	free3(bk_buffer[1]);
	loading_stage = 0;
}

void startBattle(){
	battle->command_mode = 1;
	prevCamera = camera;
	camera.pos.vx = 0;
	camera.pos.vy = 700;
	camera.pos.vz = 1700;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
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
	/*spu_pause(SPU_0CH);
	xaChannel = 1;
	xa_play(&xaChannel);*/
	vag_free(&vag);
	vag_load("FIGHT.VAG", SPU_0CH);
	enemy_push(tpages[0], BAT, 250, 300);
	enemy_push(tpages[0], BAT, 250, 0);
}

void stopBattle(){
	character_1.pos = character_1.map_pos;
	character_1.rot = character_1.map_rot;
	character_1.animation_to_play = 0;
	//xa_stop();
	//spu_play(SPU_0CH);
	vag_free(&vag);
	vag_load("AERITH.VAG", SPU_0CH);
	enemy_free();

	closeBattleMenu(battle);
	camera = prevCamera;
	battle->command_mode = 0;
	battle->atb[0].value = 0;
	battle->atb[0].bar.w = 0;
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

void zones_collision(const Stage *stage, const Character *c){
	int i = 0;
	if(!loading_stage)
	{
		for(i = 0; i < stage->zones_length; i++){
			const Zone *zone = &stage->zones[i];
			Mesh *mesh = char_getMesh(c);
			if(c->pos.vx <= zone->pos.vx + zone->w &&
				c->pos.vx + mesh->size >= zone->pos.vx &&
				c->pos.vz <= zone->pos.vz &&
				c->pos.vz + mesh->size >= zone->pos.vz + zone->z)
			{
				loading_stage = 1;			
				stage_id_to_load = zone->stage_id;
				spawn_id_to_load = zone->spawn_id;
				break;
			}
		}
	}
}
