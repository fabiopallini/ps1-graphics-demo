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
u_long *buffer_tex_c1;
u_short tpage_c1;
u_short tpage_ui;
u_short tpage_reg1;
Stage *stage;
Mesh cube;
Camera prevCamera;
Character character_1;

u_char loading_stage = 1;
int stage_id_to_load, spawn_id_to_load;
u_char mapChanged = 0;
Background background;

Battle *battle;
Mesh fightGround;
//int xaChannel = 0;

void camera_debug_input();
void load_stage(int stage_id, int spawn_id);
void randomBattle(Character *c);
void startBattle();
void stopBattle();
void stopBattle();
Enemy* ray_collisions(Sprite *s, long cameraX);
int ray_collision(Sprite *s1, Sprite *s2, long cameraX);
void zones_collision(const Stage *stage, const Character *c);

void game_load(){
	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;

	cd_open();
	cd_read_file("CHAR1\\TEX.TIM", &buffer_tex_c1);
	cd_read_file("UI.TIM", &cd_data[0]);
	cd_read_file("REG1.TIM", &cd_data[1]);
	cd_read_file("CUBE.OBJ", &cd_data[2]);
	cd_read_file("FG1.OBJ", &cd_data[3]);

	tpage_c1 = loadToVRAM(buffer_tex_c1);
	tpage_ui = loadToVRAM(cd_data[0]); // UI
	tpage_reg1 = loadToVRAM(cd_data[1]); // REG1 
	free3(buffer_tex_c1);
	free3(cd_data[0]);
	free3(cd_data[1]);

	mesh_init(&cube, cd_data[2], tpage_reg1, 255, 255, 30);
	free3(cd_data[2]);
	cube.pos.vx = 150;
	cube.pos.vy = -50;
	cube.pos.vz = -600;

	mesh_init(&fightGround, cd_data[3], tpage_reg1, 255, 255, 500);
	free3(cd_data[3]);

	init_ui(tpage_ui, SCREEN_WIDTH, SCREEN_HEIGHT);
	battle = malloc3(sizeof(Battle));
	init_battle(battle, tpage_ui, SCREEN_WIDTH, SCREEN_HEIGHT);

	char_init(&character_1);
	character_1.HP = 80;
	character_1.HP_MAX = 80;
	character_1.MP = 20;
	character_1.MP_MAX = 20;
	character_1.RUN_SPEED = 5;

	char_animation_init(&character_1, 2);
	char_animation_set(&character_1, "CHAR1\\RUN", 0, 1, 5, tpage_c1, 128, 100);
	character_1.meshAnimations[0].interval = 7;
	char_animation_set(&character_1, "CHAR1\\ATT", 1, 0, 3, tpage_c1, 128, 150);
	character_1.meshAnimations[1].interval = 10;

	stage = malloc3(sizeof(Stage));
	if(stage == NULL){
		printf("stage malloc3 error\n");
		exit(1);
	}
	memset(stage, 0, sizeof(Stage));
	load_stage(0, 0);
	background_init(&background);
	//load_stage(3, 1);
	
	spu_init();
	sfx_load("SLASH.VAG", 6656, SPU_1CH);
	vag_song_play("AERITH.VAG", SPU_0CH);
	//sfx_load("GUNSHOT.VAG", 15200, SPU_1CH);
	//spu_load(cd_data[5], 15200, SPU_0CH);
	//free3(cd_data[5]);
}

void game_update()
{
	int i = 0;
#ifdef DEBUG
	if(!loading_stage && pad & PADLtriangle && (opad & PADLtriangle) == 0){
		CAMERA_DEBUG = !CAMERA_DEBUG;
	}
	if(!loading_stage && CAMERA_DEBUG == 1){
		camera_debug_input();
		return;
	}
#endif

	if(loading_stage && DSR_callback_id == 0){
		load_stage(stage_id_to_load, spawn_id_to_load);
		return;
	}

	randomBattle(&character_1);

	if(battle->command_mode == 0 && !battleIntro)
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
				}
			}
			if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1){
				balloon.display = 0;
				scene_remove(&balloon);
				scene_remove(&balloon);
			}
			return;
		}

		zones_collision(stage, &character_1);
		//char_set_rgb(character_1, 50, 50, 50);
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
					scene_add(&balloon, TYPE_FONT);
					scene_add(&balloon, TYPE_UI);
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
						stepsCounter++;
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
						stepsCounter++;
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
						stepsCounter++;
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
						stepsCounter++;
					}
				}
				if(pad == PADLup){
					long z = character_1.pos.vz + character_1.RUN_SPEED;
					character_1.rot.vy = 2048;
					if(mesh_on_plane(character_1.pos.vx, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
						stepsCounter++;
					}
				}

				if(pad == PADLdown){
					long z = character_1.pos.vz - character_1.RUN_SPEED;
					character_1.rot.vy = 0;
					if(mesh_on_plane(character_1.pos.vx, z, stage->planes[i])){
						character_1.pos.vz = z;
						character_1.play_animation = 1;
						stepsCounter++;
					}
				}
				if(pad == PADLleft){
					long x = character_1.pos.vx - character_1.RUN_SPEED;
					character_1.rot.vy = 1024;
					if(mesh_on_plane(x, character_1.pos.vz, stage->planes[i])){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
						stepsCounter++;
					}
				}
				if(pad == PADLright){
					long x = character_1.pos.vx + character_1.RUN_SPEED;
					character_1.rot.vy = 3072;
					if(mesh_on_plane(x, character_1.pos.vz, stage->planes[i])){
						character_1.pos.vx = x;
						character_1.play_animation = 1;
						stepsCounter++;
					}
				}
			}
		}
	
		if(stage->id == 2){
			cube.rot.vx += 10;
			cube.rot.vy += 10;
			cube.rot.vz += 10;
		}
		for(i = 0; i < stage->npcs_len; i++){
			npc_update(&stage->npcs[i]);
		}
#ifdef DEBUG
		if(pad & PADR1 && (opad & PADR1) == 0){
			battleIntro = 1;
			prevCamera = camera;
			/*battle->status = 1;
			scene_load(startBattle);*/
		}
#endif
	}
	// end battle->command_mode == 0
	else
	{
		battle_update(battle, pad, opad, &character_1);
		if(battle->status == 2){
		//if(pad & PADR1 && (opad & PADR1) == 0){
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
}

void game_draw(){
	short i = 0;
	if(!loading_stage)
	{
		if(CAMERA_DEBUG == 1){
			char log[100];
			sprintf(log, "x%ld y%ld z%ld rx%d ry%d rz%d\n\nx%ld y%ld z%ld\n",
			camera.pos.vx, camera.pos.vy, camera.pos.vz,
			camera.rot.vx, camera.rot.vy, camera.rot.vz,
			character_1.pos.vx, character_1.pos.vy, character_1.pos.vz);
			FntPrint(log);
		}
		if(!battle->command_mode){
			if(CAMERA_DEBUG == 1){
				for(i = 0; i < stage->zones_length; i++)
					drawMesh(&stage->zones[i].mesh, OTSIZE-1);
				for(i = 0; i < stage->planes_length; i++)
					drawMesh(&stage->planes[i], OTSIZE-1);
			}

			background_draw(&background, OTSIZE-1, drawSprite_2d);
			scene_draw();
#ifdef DEBUG
			for(i = 0; i < stage->npcs_len; i++){
				if(stage->npcs[i].bbox.poly_f4 != NULL)
					drawBBox(&stage->npcs[i].bbox);
			}
#endif
		}
		else {
			char str_hp_mp[30];
			drawFont("Attack\nMagic\nSkill\nItem\n", 20, 190, 0);
			sprintf(str_hp_mp, "HP %d/%d MP %d/%d", 
			character_1.HP,
			character_1.HP_MAX,
			character_1.MP,
			character_1.MP_MAX);
			drawFont(str_hp_mp, 105, 190, 1);
			drawSprite_2d(&battle->atb[0].bar, 1);
			drawSprite_2d(&battle->atb[0].border, 1);
			battle_draw(battle, drawSprite, drawSprite_2d, OTSIZE-1);
			drawSprite_2d(&battle->command_bg, 1);

			char_draw(&character_1, 0, drawMesh);
			if(enemyNode != NULL) {
				EnemyNode *node = enemyNode;
				while(node != NULL){
					Enemy *e = node->enemy;	
					if(e->sprite.hitted == 1)
						drawSprite(&e->blood, 0);
					if(e->sprite.hp > 0)
						drawSprite(&e->sprite, 0);
					node = node->next;
				}
			}
			drawMesh(&fightGround, OTSIZE-1);
		}
	}
}

void camera_debug_input(){
	if(pad & PADL2)
		camera.rot.vx -= CAMERA_DEBUG_SPEED;
	if(pad & PADR2)
		camera.rot.vx += CAMERA_DEBUG_SPEED;

	if(pad & PADL1)
		camera.rot.vy += CAMERA_DEBUG_SPEED;
	if(pad & PADR1)
		camera.rot.vy -= CAMERA_DEBUG_SPEED;

	if(pad & PADLleft)
		camera.pos.vx += CAMERA_DEBUG_SPEED;
	if(pad & PADLright)
		camera.pos.vx -= CAMERA_DEBUG_SPEED;

	if(pad & PADLcross){
		if(pad & PADLup)
			camera.pos.vy += CAMERA_DEBUG_SPEED;
		if(pad & PADLdown)
			camera.pos.vy -= CAMERA_DEBUG_SPEED;
	}
	else{
		if(pad & PADLup)
			camera.pos.vz -= CAMERA_DEBUG_SPEED;
		if(pad & PADLdown)
			camera.pos.vz += CAMERA_DEBUG_SPEED;
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

	cd_read_file_bytes("STAGES.BIN", &stages_buffer, start_byte, end_byte, 0);
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

	scene_free();
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
		mesh_init(&stage->planes[i], (u_long*)vertices, 0, 0, 0, 1);
		mesh_set_rgb(&stage->planes[i], 0, 128, 0, 1);
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
		u_long *buffer_obj, *buffer_tim;
		u_short tpage;
		cd_read_file("NPCS\\NPC1.OBJ", &buffer_obj);
		cd_read_file("NPCS\\NPC1.TIM", &buffer_tim);
		tpage = loadToVRAM(buffer_tim);	
		npc_init(npc, buffer_obj, tpage, &stageData.npcData[j]);
		bbox_init(&npc->bbox, &npc->mesh, 100);
		free3(buffer_obj);
		free3(buffer_tim);
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
		//for (i = 0; i < npc->talk_pages; i++) {
			//printf("--->npc->talk_char[%d] --> %s\n", i, npc->talk_chars[i]);
		//}
	}
	free3(stages_buffer);
/*
===========================================================================================
					LOAD STAGE	
===========================================================================================
*/
	// PRE-RENDERED BACKGROUND
	cd_read_file(stage->tims[0], &bk_buffer[0]);
	cd_read_file(stage->tims[1], &bk_buffer[1]);
	mapChanged = 1;
	clearVRAM_at(320, 0, 256, 256);
	background.tpages[0] = loadToVRAM(bk_buffer[0]);
	background.tpages[1] = loadToVRAM(bk_buffer[1]);
	free3(bk_buffer[0]);
	free3(bk_buffer[1]);
	// SET CAMERA POS
	memcpy(&camera.pos, &stage->camera_pos, sizeof(stage->camera_pos));
	memcpy(&camera.rot, &stage->camera_rot, sizeof(stage->camera_rot));
	// SET CHARACTER POS
	memcpy(&character_1.pos, &stage->spawns[spawn_id].pos, sizeof(stage->spawns[spawn_id].pos));
	memcpy(&character_1.rot, &stage->spawns[spawn_id].rot, sizeof(stage->spawns[spawn_id].rot));

	// LOAD SCENE OBJECTS 
	scene_add(&character_1, TYPE_CHARACTER);
	for(i = 0; i < stage->npcs_len; i++){
		scene_add(&stage->npcs[i].mesh, TYPE_MESH);
	}
	if(stage->id == 2){
		scene_add(&cube, TYPE_MESH);
	}

	loading_stage = 0;
}

void randomBattle(Character *c){
	if(battle->command_mode == 0)
	{
		if(stepsCounter >= 500 && battleIntro == 0){
			int r = 0;
			srand(c->pos.vx + c->pos.vy + c->pos.vz);
			r = random(200);
			if(r < 3){
				battleIntro = 1;
				prevCamera = camera;
			}	
		}
		if(battleIntro){
			float k = 0.01;
			//camera.rot.vz += 20;
			camera.pos.vx += ((c->pos.vx - 100) - camera.pos.vx) * k;
			camera.pos.vy += ((c->pos.vy + 100) - camera.pos.vy) * k;
			camera.pos.vz += (c->pos.vz - camera.pos.vz) * k;
			if(camera.pos.vz <= prevCamera.pos.vz - 600){
				stepsCounter = 0;
				battleIntro = 0;
				battle->status = 1;
				scene_load(startBattle);
			}
		}
	}
}

void startBattle(){
	battle->command_mode = 1;
	// front view
	/*
	camera.pos.vx = 0;
	camera.pos.vy = 700;
	camera.pos.vz = 1700;
	camera.rot.vx = 200;
	camera.rot.vy = 0; 
	*/
	// right lateral view
	camera.pos.vx = -1140;
	camera.pos.vy = 635;
	camera.pos.vz = 1830;
	camera.rot.vx = 150;
	camera.rot.vy = 365;
	camera.rot.vz = 0;

	// saving the current char position in the map view
	character_1.map_pos = character_1.pos;
	character_1.map_rot = character_1.rot;

	// place character in battle position 
	character_1.battle_pos.vx = 400;
	character_1.battle_pos.vy = 0;
	character_1.battle_pos.vz = 0;
	character_1.battle_rot.vx = 0;
	character_1.battle_rot.vy = 1024;
	character_1.battle_rot.vz = 0;
	// set the pos to battle position
	character_1.pos = character_1.battle_pos;
	character_1.rot = character_1.battle_rot;
	character_1.play_animation = 0;
	character_1.animation_to_play = 1;
	/*spu_pause(SPU_0CH);
	xaChannel = 1;
	xa_play(&xaChannel);*/
	vag_song_play("FIGHT.VAG", SPU_0CH);
	enemy_push(tpage_reg1, BAT, -250, -150, 300);
	enemy_push(tpage_reg1, BAT, -250, -150, 0);
	if(enemyNode != NULL) {
		EnemyNode *node = enemyNode;
		while(node != NULL){
			Enemy *e = node->enemy;	
			billboard(&e->sprite);
			billboard(&e->blood);
			node = node->next;
		}
	}
	billboard(&battle->selector);
	billboard(&battle->dmg.sprite[0]);
	billboard(&battle->dmg.sprite[1]);
	billboard(&battle->dmg.sprite[2]);
	billboard(&battle->dmg.sprite[3]);
}

void stopBattle(){
	character_1.pos = character_1.map_pos;
	character_1.rot = character_1.map_rot;
	character_1.animation_to_play = 0;
	//xa_stop();
	//spu_play(SPU_0CH);
	vag_song_play("AERITH.VAG", SPU_0CH);
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
