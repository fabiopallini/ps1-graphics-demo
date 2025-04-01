#include "game.h"
#include "utils.h"
#include "enemy.h"
#include "battle.h"
#include "stages.h"
#define CAMERA_SPEED 5

StageData stageData;
Menu menu;
u_long *cd_data[3];
u_long *buffer_tex_c1;
u_short tpage_c1;
u_short tpage_ui;
Stage *stage;
Mesh cube;
u_char CAMERA_EDIT = 0;
Camera prevCamera;
Entity player;
Inventory inv;
Item item;
PLANE_EDIT_STATUS plane_edit_status = PLANE_NONE;

u_char loading_stage = 1;
int stage_id_to_load, spawn_id_to_load;
u_char mapChanged = 0;
Background background;

Battle *battle;
Mesh fightGround;

void camera_debug_input();
void stage_load(int stage_id, int spawn_id);
void randomBattle(Model *m);
void startBattle();
void stopBattle();
void zones_collision(const Stage *stage, const Model *m);
void add_balloon(char *text[], int Npages);

char *thoughts[] = {
	"Dove sono?",
	"Non riconosco questo posto...",
};

void menu_view_home(Window *win){
	VECTOR pos = window_get_pos(win);
	drawFont("home view", pos.vx, pos.vy, 0);
}

void menu_view_equip(Window *win){
	VECTOR pos = window_get_pos(win);
	drawFont("equip view", pos.vx, pos.vy, 0);
}

void menu_view_status(Window *win){
	VECTOR pos = window_get_pos(win);
	drawFont("status view", pos.vx, pos.vy, 0);
}

void window_view_list(Window *win){
	VECTOR pos = window_get_pos(win);
	int i;
	// Maximum number of items that can be displayed in a column of the window
	u_char maxItems = win->background.h / FONT_LINE_HEIGHT;
	u_char maxChars = 20;
	char list[maxItems][maxChars];
	int n = 0;

	//printf("win: %p, menu.win_main: %p\n", win, &menu.win_main);

	for(inv.j = inv.i; inv.j < inv.count; inv.j++){
		Item *item = inventory_get_item(&inv, inv.j);
		if(item != NULL && n < maxItems){
			int len = strlen(item->name);
			if(len > 15) len = 15;
			memcpy(list[n], item->name, len);
			// clean up the remaining chars with space character
			memset(list[n] + len, ' ', 16 - len);
			// place item count at the end
			list[n][16] = ':';
			if(item->count <= 9){
				list[n][17] = ' ';
				list[n][18] = item->count + '0';
			}
			else {
				int tens = item->count / 10;
				int ones = item->count % 10;
				list[n][17] = tens + '0';
				list[n][18] = ones + '0';
				//sprintf(&list[n][17], "%02d", item->count);
			}
			list[n][19] = '\0';
			n++;
		}
	}

	if(pad_press_delay(PADLup)){
		if(inv.n > 0)
			inv.n--;
		if(win->selector.sprite.pos.vy > pos.vy){	
			long y = win->selector.sprite.pos.vy - 10;
			win->selector.sprite.pos.vy = y;
		}
		else if(inv.i > 0) inv.i--;
	}

	if(pad_press_delay(PADLdown)){
		if(inv.n < inv.count-1){
			inv.n++;
			if(win->selector.sprite.pos.vy < (pos.vy+((maxItems-1)*10))){	
				long y = win->selector.sprite.pos.vy + 10;
				win->selector.sprite.pos.vy = y;
			}
			else if(inv.i+maxItems < inv.count) inv.i++;
		}
	}

	if(pad & PADLcross && opad == 0){
		Item *item = inventory_get_item(&inv, inv.n);
		if(item != NULL){
			inventory_remove_item(&inv, item);
			// If removing the last bottom item, move the selector up by one position
			if(inv.n > inv.count-1){
				inv.n = inv.count-1;
				if(win->selector.sprite.pos.vy > pos.vy){	
					long y = win->selector.sprite.pos.vy - 10;
					win->selector.sprite.pos.vy = y;
				}
			}
		}
	}

	drawSprite(&win->selector.sprite, 0);
	for(i = 0; i < n; i++){
		long y = pos.vy + (FONT_LINE_HEIGHT * i);
		drawFont(list[i], pos.vx + 20, y, 0);
	}
}

void game_load(){
	int i;
	u_short tpage_reg1;
	cd_read_file("CHAR1\\TEX.TIM", &buffer_tex_c1);
	cd_read_file("UI.TIM", &cd_data[0]);
	cd_read_file("REG1.TIM", &cd_data[1]);
	cd_read_file("CUBE.OBJ", &cd_data[2]);

	tpage_c1 = loadToVRAM(buffer_tex_c1);
	tpage_ui = loadToVRAM(cd_data[0]); // UI 768 0
	tpage_reg1 = loadToVRAM(cd_data[1]); // REG1 
	free3(buffer_tex_c1);
	free3(cd_data[0]);
	free3(cd_data[1]);

	mesh_init(&cube, cd_data[2], tpage_reg1, 255, 255, 30);
	free3(cd_data[2]);
	cube.pos.vx = 150;
	cube.pos.vy = -50;
	cube.pos.vz = -600;

	init_ui(tpage_ui, SCREEN_WIDTH, SCREEN_HEIGHT);
	battle = malloc3(sizeof(Battle));
	init_battle(battle, tpage_ui, SCREEN_WIDTH, SCREEN_HEIGHT);

	model_init(&player.model);
	player.HP = 80;
	player.HP_MAX = 80;
	player.MP = 20;
	player.MP_MAX = 20;
	player.SPEED = 5;

	model_animation_init(&player.model, 2);
	model_animation_set(&player.model, "CHAR1\\RUN", 0, 1, 5, tpage_c1, 128, 100);
	player.model.meshAnimations[0].interval = 7;
	model_animation_set(&player.model, "CHAR1\\ATT", 1, 0, 3, tpage_c1, 128, 150);
	player.model.meshAnimations[1].interval = 10;

	stage = malloc3(sizeof(Stage));
	if(stage == NULL){
		printf("stage malloc3 error\n");
		exit(1);
	}
	memset(stage, 0, sizeof(Stage));
	stage_load(0, 0);
	//stage_load(7, 0);
	background_init(&background);
	
	sfx_load("BINIT.VAG", SPU_1CH);
	sfx_load("SLASH.VAG", SPU_2CH);
	vag_song_play("AERITH.VAG");

	add_balloon(thoughts, sizeof(thoughts) / sizeof(char*));

	menu_init(&menu, menu_view_home, tpage_ui);

	// add some items to inventory 
	inv.count = 0; inv.i = 0; inv.j = 0;
	strcpy(item.name, "Potion");
	for(i = 1 ; i <= 6; i++)
		inventory_add_item(&inv, &item);

	strcpy(item.name, "Ether");
	inventory_add_item(&inv, &item);
	strcpy(item.name, "Potion+1");
	inventory_add_item(&inv, &item);

	for(i = 1 ; i <= 50; i++){
		strcpy(item.name, "Example");
		inventory_add_item(&inv, &item);
	}

	for(i = 1 ; i <= 50; i++){
		sprintf(item.name, "Example_%d", i);
		inventory_add_item(&inv, &item);
	}
}

void game_update()
{
	int i = 0;
	// menu is open 
	if(menu.status == MENU_ON)
	{
		if(pad & PADLcircle && (opad & PADLcircle) == 0)
			menu.status = MENU_OFF;
		if(pad & PADLup && (opad & PADLup) == 0){
			menu_selector_set_index(&menu, -1);
		}
		if(pad & PADLdown && (opad & PADLdown) == 0){
			menu_selector_set_index(&menu, +1);
		}
		if(pad & PADLcross && (opad & PADLcross) == 0){
			// set menu.status to menu.selector index + skipping MENU_ON && MENU_OFF values
			menu.status = menu.win_sidebar.selector.index + (MENU_ON+1);
			switch(menu.status){
				case MENU_VIEW_EQUIP:
					window_set_display(&menu.win_main, menu_view_equip);
					break;
				case MENU_VIEW_STATUS:
					window_set_display(&menu.win_main, menu_view_status);
					break;
				case MENU_VIEW_ITEM:
					inv.i = 0; inv.j = 0; inv.n = 0;
					pad = 1;
					window_set_display(&menu.win_main, window_view_list);
					break;
				default:
					break;
			}
		}
		return;
	}
	// OPEN MENU on Triangle button press
	else if(pad & PADLtriangle && (opad & PADLtriangle) == 0 && !balloon.display && menu.status == MENU_OFF){ 
		menu.status = MENU_ON;
		menu_selector_set_index(&menu, menu.win_sidebar.selector.index = 0);
	}

	// on sidebar object selected (Equip, Status, Item ecc)
	if(menu.status > MENU_ON && menu.status <= MENU_VIEW_ITEM)
	{
		// back to menu sidebar selection
		if(pad & PADLcircle&& (opad & PADLcircle) == 0){
			menu.status = MENU_ON;
			window_set_display(&menu.win_main, menu_view_home);
		}
	}
	// if we are inside the menu, we only run the menu loop
	if(menu.status >= MENU_ON)
		return;

#ifdef DEBUG
	if(!loading_stage && pad & PADselect && (opad & PADselect) == 0){
		CAMERA_EDIT = !CAMERA_EDIT;
		if(!CAMERA_EDIT){
			// set color green to the last plane, it may be a new plane (blue)
			Mesh *p = &stage->planes[stage->planes_length-1];
			mesh_set_rgb(p, 0, 128, 0, 1);
			// print all planes of the current stage for stages.json file
			print_planes(stage->planes, stage->planes_length);
			plane_edit_status = PLANE_NONE;
		}
	}
	if(!loading_stage && CAMERA_EDIT == 1){
		camera_debug_input();
		return;
	}
#endif

	if(loading_stage && DSR_callback_id == 0){
		stage_load(stage_id_to_load, spawn_id_to_load);
		return;
	}

	randomBattle(&player.model);

	if(battle->status == BATTLE_OFF && !battleIntro)
	{
		if(balloon.display == 1)
		{
			if((opad & PADLcross) == 0 && pad & PADLcross){
				balloon.page_index++;
				if(balloon.page_index >= balloon.pages_length){
					balloon.prev_display = 1;
					balloon.page_index = 0;
					balloon.tale[0] = NULL;
				}
				else{
					if(balloon.tale[0] == NULL)
					set_balloon(&balloon, stage->npcs[balloon.npc_id].talk_chars[balloon.page_index]);
					else
					set_balloon(&balloon, balloon.tale[balloon.page_index]);
				}
			}
			if((opad & PADLcross) == PADLcross && (pad & PADLcross) == 0 && balloon.prev_display == 1){
				balloon.display = 0;
				scene_remove(&balloon);
				scene_remove(&balloon);
			}
			return;
		}

		zones_collision(stage, &player.model);
		//model_set_rgb(player.model, 50, 50, 50);
		//model_set_shadeTex(player.model, 1);
	
		if(pad & PADLcross && ((opad & PADLcross) == 0)){
			int i = 0;
			for(i = 0; i < stage->npcs_len; i++)
			{
				Npc *npc = &stage->npcs[i];
				if(bbox_collision(player.model.pos.vx, player.model.pos.vz, npc->bbox) &&
				model_looking_at(&player.model, npc->mesh.pos.vx, npc->mesh.pos.vz) == 1)
				{
					balloon.npc_id = i;
					balloon.pages_length = npc->talk_pages;
					set_balloon(&balloon, npc->talk_chars[balloon.page_index]);
					scene_add(&balloon, GFX_BALLOON);
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
		player.model.play_animation = 0;
		if(mapChanged == 0){
			int i = 0;
			for(i = 0; i < stage->planes_length; i++){
				if(pad == (PADLup+PADLleft)){
					long z = player.model.pos.vz + player.SPEED/2;
					long x = player.model.pos.vx - player.SPEED/2;
					player.model.rot.vy = 1536;
					if(mesh_on_plane(x, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if((pad == PADLup+PADLright)){
					long z = player.model.pos.vz + player.SPEED/2;
					long x = player.model.pos.vx + player.SPEED/2;
					player.model.rot.vy = 2560;
					if(mesh_on_plane(x, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if(pad == (PADLdown+PADLleft)){
					long z = player.model.pos.vz - player.SPEED/2;
					long x = player.model.pos.vx - player.SPEED/2;
					player.model.rot.vy = 512; 
					if(mesh_on_plane(x, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if((pad == PADLdown+PADLright)){
					long z = player.model.pos.vz - player.SPEED/2;
					long x = player.model.pos.vx + player.SPEED/2;
					player.model.rot.vy = 3584;
					if(mesh_on_plane(x, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if(pad == PADLup){
					long z = player.model.pos.vz + player.SPEED;
					player.model.rot.vy = 2048;
					if(mesh_on_plane(player.model.pos.vx, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}

				if(pad == PADLdown){
					long z = player.model.pos.vz - player.SPEED;
					player.model.rot.vy = 0;
					if(mesh_on_plane(player.model.pos.vx, z, stage->planes[i])){
						player.model.pos.vz = z;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if(pad == PADLleft){
					long x = player.model.pos.vx - player.SPEED;
					player.model.rot.vy = 1024;
					if(mesh_on_plane(x, player.model.pos.vz, stage->planes[i])){
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
					}
				}
				if(pad == PADLright){
					long x = player.model.pos.vx + player.SPEED;
					player.model.rot.vy = 3072;
					if(mesh_on_plane(x, player.model.pos.vz, stage->planes[i])){
						player.model.pos.vx = x;
						player.model.play_animation = 1;
						stepsCounter++;
						break;
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
			sfx_play(SPU_1CH);
			battleIntro = 1;
			prevCamera = camera;
		}
#endif
	} // --> end battle->status == BATTLE_OFF 
	else
	{
		/* battle starting, camera transition view
		 * move the camera from left to right to show all the battle area
		*/
		if(battle->status == BATTLE_START){ 
			VECTOR a, b;
			SVECTOR a_rot, b_rot;
			// cam start pos 
			a.vx = 1555;
			a.vy = 450;
			a.vz = 2385;
			a_rot.vx = 155;
			a_rot.vy = -275; 
			a_rot.vz = 0;
			// cam target pos
			b.vx = -1140;
			b.vy = 635;
			b.vz = 1830;
			b_rot.vx = 150;
			b_rot.vy = 365;
			b_rot.vz = 0;
			battle->t += 0.006;
			camera = camera_interpolate(a, a_rot, b, b_rot, battle->t);
			if(battle->t >= 1){
				battle->t = 0;
				scene_update_billboards();
				battle->status = BATTLE_WAIT; // ATBs start to load
			}
		}
		battle_update(battle, pad, opad, &player);
		if(battle->status == BATTLE_END){
		//if(pad & PADR1 && (opad & PADR1) == 0){
			battle->status = BATTLE_OFF;
			scene_load(stopBattle);
			return;
		}

		if(enemyNode != NULL) {
			EnemyNode *node = enemyNode;
			while(node != NULL){
				Enemy *e = node->enemy;	
				enemy_update(e, *model_getMesh(&player.model), battle->status);
				if(e->attacking == 2){
					e->attacking = 3;
					player.HP -= 2;
					display_dmg(&battle->dmg, model_getMesh(&player.model)->pos, model_getMesh(&player.model)->size*1.5, 2);
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
		if(menu.status){
			menu_draw(&menu);
			return;
		}

		if(CAMERA_EDIT == 1){
			char log[100];
			sprintf(log, "x%ld y%ld z%ld rx%d ry%d rz%d\n\nx%ld y%ld z%ld\n",
			camera.pos.vx, camera.pos.vy, camera.pos.vz,
			camera.rot.vx, camera.rot.vy, camera.rot.vz,
			player.model.pos.vx, player.model.pos.vy, player.model.pos.vz);
			FntPrint(log);
		}
		if(!battle->status){
			if(CAMERA_EDIT == 1){
				for(i = 0; i < stage->zones_length; i++)
					drawMesh(&stage->zones[i].mesh, OTSIZE-1);
				for(i = 0; i < stage->planes_length; i++)
					drawMesh(&stage->planes[i], OTSIZE-1);
			}

			background_draw(&background, OTSIZE-1, drawSprite);
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
			sprintf(str_hp_mp, "HP %d/%d MP %d/%d", 
			player.HP,
			player.HP_MAX,
			player.MP,
			player.MP_MAX);
			drawFont(str_hp_mp, 105, 190, 1);
			drawSprite(&battle->atb[0].bar, 1);
			drawSprite(&battle->atb[0].border, 1);
			battle_draw(battle);
			drawSprite(&battle->command_bg, 1);

			model_draw(&player.model, 0, drawMesh);
			if(enemyNode != NULL) {
				EnemyNode *node = enemyNode;
				while(node != NULL){
					Enemy *e = node->enemy;	
					if(e->hitted == 1)
						drawSprite3D(&e->blood, 0);
					if(e->hp > 0)
						drawSprite3D(&e->sprite, 0);
					node = node->next;
				}
			}
			drawMesh(&fightGround, OTSIZE-1);
		}
	}
}

void camera_debug_input(){
	if(plane_edit_status == PLANE_NONE){
		if(pad & PADL2)
			camera.rot.vx -= CAMERA_SPEED;
		if(pad & PADR2)
			camera.rot.vx += CAMERA_SPEED;

		if(pad & PADL1)
			camera.rot.vy += CAMERA_SPEED;
		if(pad & PADR1)
			camera.rot.vy -= CAMERA_SPEED;

		if(pad & PADLleft)
			camera.pos.vx += CAMERA_SPEED;
		if(pad & PADLright)
			camera.pos.vx -= CAMERA_SPEED;

		if(pad & PADLcross){
			if(pad & PADLup)
				camera.pos.vy += CAMERA_SPEED;
			if(pad & PADLdown)
				camera.pos.vy -= CAMERA_SPEED;
		}
		else{
			if(pad & PADLup)
				camera.pos.vz -= CAMERA_SPEED;
			if(pad & PADLdown)
				camera.pos.vz += CAMERA_SPEED;
		}
	
		// add a new plane to the current stage
		if(pad & PADLsquare && (opad & PADLsquare) == 0){
			if(plane_add(stage->planes, &stage->planes_length)){
				printf("plane add %d\n", stage->planes_length);
				plane_edit_status = PLANE_POS;
			}
		}
	}
	if(plane_edit_status == PLANE_POS){
		Mesh *p = &stage->planes[stage->planes_length-1];
		if(pad & PADR1){
			// plane has a minimum width of 5
			if(pad & PADLleft && p->vertices[1].vx > 5){
				p->vertices[1].vx -= 1;
				p->vertices[3].vx -= 1;
			}
			if(pad & PADLright){
				p->vertices[1].vx += 1;
				p->vertices[3].vx += 1;
			}
			// plane has a maximum depth of 5
			// depth grows towards the camera (negative numbers)
			if(pad & PADLup && p->vertices[0].vz < 5){
				p->vertices[0].vz += 1;
				p->vertices[1].vz += 1;
			}
			// towards camera
			if(pad & PADLdown){
				p->vertices[0].vz -= 1;
				p->vertices[1].vz -= 1;
			}
		}
		else {
			if(pad & PADLleft)
				p->pos.vx -= 1;
			if(pad & PADLright)
				p->pos.vx += 1;
			if(pad & PADLup)
				p->pos.vz += 1;
			if(pad & PADLdown)
				p->pos.vz -= 1;
		}
	}
}

void stage_free(){
	if(stage != NULL){
		int i = 0;
		for(i = 0; i < stage->planes_length; i++){
			mesh_free(&stage->planes[i]);
		}
		for(i = 0; i < stage->zones_length; i++){
			mesh_free(&stage->zones[i].mesh);
		}
		for (i = 0; i < stage->npcs_len; i++) {
			Npc *npc = &stage->npcs[i];
			npc_free(npc);
		}
	}
	scene_free();
	memset(stage, 0, sizeof(Stage));
	memset(&stageData, 0, sizeof(StageData));
}

void stage_load(int stage_id, int spawn_id){
	u_long *stages_buffer;
	int stage_addr = 0;
	u_long *bk_buffer[2];
	int i,j = 0;
	size_t byte_cursor = 0;
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

	// CLEANUP PREVIOUS STAGE DATA	
	stage_free();

	// LOAD NEW STAGE DATA	
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
		mesh_init(&stage->planes[i], (u_long*)plane_vertices(), 0, 0, 0, 1);
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
	memcpy(&player.model.pos, &stage->spawns[spawn_id].pos, sizeof(stage->spawns[spawn_id].pos));
	memcpy(&player.model.rot, &stage->spawns[spawn_id].rot, sizeof(stage->spawns[spawn_id].rot));

	// LOAD SCENE OBJECTS 
	scene_add(&player.model, GFX_MODEL);
	for(i = 0; i < stage->npcs_len; i++){
		scene_add(&stage->npcs[i].mesh, GFX_MESH);
	}
	if(stage->id == 2){
		scene_add(&cube, GFX_MESH);
	}

	loading_stage = 0;
}

void randomBattle(Model *m){
	if(battle->status == BATTLE_OFF)
	{
#ifndef DEBUG
		if(stepsCounter >= 500 + battleRandom && battleIntro == 0){
			srand(m->pos.vx + m->pos.vy + m->pos.vz);
			battleRandom = randomRange(100, 1000);
			sfx_play(SPU_1CH);
			battleIntro = 1;
			prevCamera = camera;
		}
#endif
		if(battleIntro){
			float k = 0.01;
			//camera.rot.vz += 20;
			camera.pos.vx += ((m->pos.vx - 100) - camera.pos.vx) * k;
			camera.pos.vy += ((m->pos.vy + 100) - camera.pos.vy) * k;
			camera.pos.vz += (m->pos.vz - camera.pos.vz) * k;
			if(camera.pos.vz <= prevCamera.pos.vz - 500){
				stepsCounter = 0;
				battleIntro = 0;
				battle->status = BATTLE_START;
				scene_load(startBattle);
			}
		}
	}
}

void startBattle(){
	u_long *buffer_fg;
	u_long *buffer_reg;
	u_short tpage_reg;
	scene_free();
	// saving the current player position in the map view
	player.map_pos = player.model.pos;
	player.map_rot = player.model.rot;

	// place player in battle position 
	player.battle_pos.vx = 400;
	player.battle_pos.vy = 0;
	player.battle_pos.vz = 0;
	player.battle_rot.vx = 0;
	player.battle_rot.vy = 1024;
	player.battle_rot.vz = 0;
	// set the pos to battle position
	player.model.pos = player.battle_pos;
	player.model.rot = player.battle_rot;
	player.model.play_animation = 0;
	player.model.animation_to_play = 1;

	cd_read_file("FG1.OBJ", &buffer_fg);
	cd_read_file("REG1.TIM", &buffer_reg);
	tpage_reg = loadToVRAM(buffer_reg);
	mesh_init(&fightGround, buffer_fg, tpage_reg, 255, 255, 500);
	free3(buffer_fg);
	free3(buffer_reg);

	enemy_push(tpage_reg, BAT, -250, -150, 300);
	enemy_push(tpage_reg, BAT, -250, -150, 0);

	scene_add(&battle->selector, GFX_SPRITE_DRAW);
	scene_add(&battle->dmg.sprite[0], GFX_SPRITE_DRAW);
	scene_add(&battle->dmg.sprite[1], GFX_SPRITE_DRAW);
	scene_add(&battle->dmg.sprite[2], GFX_SPRITE_DRAW);
	scene_add(&battle->dmg.sprite[3], GFX_SPRITE_DRAW);

	vag_song_play("FIGHT.VAG");
}

void stopBattle(){
	enemy_free();
	if(fightGround.ft4 != NULL)
		mesh_free(&fightGround);
	stage_load(stage_id_to_load, 0);

	player.model.pos = player.map_pos;
	player.model.rot = player.map_rot;
	player.model.animation_to_play = 0;
	vag_song_play("AERITH.VAG");

	closeBattleMenu(battle);
	camera = prevCamera;
	battle->status = BATTLE_OFF;
	battle->atb[0].value = 0;
	battle->atb[0].bar.w = 0;
}

void zones_collision(const Stage *stage, const Model *m){
	int i = 0;
	if(!loading_stage)
	{
		for(i = 0; i < stage->zones_length; i++){
			const Zone *zone = &stage->zones[i];
			Mesh *mesh = model_getMesh(m);
			if(m->pos.vx <= zone->pos.vx + zone->w &&
				m->pos.vx + mesh->size >= zone->pos.vx &&
				m->pos.vz <= zone->pos.vz &&
				m->pos.vz + mesh->size >= zone->pos.vz + zone->z)
			{
				loading_stage = 1;			
				stage_id_to_load = zone->stage_id;
				spawn_id_to_load = zone->spawn_id;
				break;
			}
		}
	}
}

void add_balloon(char *text[], int Npages){
	int i = 0;
	balloon.pages_length = Npages;
	for(i = 0; i < Npages; i++)
		balloon.tale[i] = text[i];
	set_balloon(&balloon, balloon.tale[0]);
	scene_add(&balloon, GFX_BALLOON);
}
