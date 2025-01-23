#include "utils.h"
#include <string.h>

void npc_init(Npc *npc, u_long *cd_obj, u_short tpage, const NpcData *npcData){
	memset(npc, 0, sizeof(Npc));
	mesh_init(&npc->mesh, cd_obj, tpage, 255, 128, 45);
	npc->mesh.pos.vx = (long)npcData->x;
	npc->mesh.pos.vy = (long)npcData->y;
	npc->mesh.pos.vz = (long)npcData->z;
	npc->mesh.rot.vx = npcData->rx;
	npc->mesh.rot.vy = npcData->ry;
	npc->mesh.rot.vz = npcData->rz;
}

void npc_free(Npc *npc){
	int i = 0;
	//printf("clean npc");
	mesh_free(&npc->mesh);
	if(npc->talk_chars != NULL){
		printf("clean %d talk pages\n", npc->talk_pages);
		for(i = 0; i < npc->talk_pages; i++)
			free3(npc->talk_chars[i]);
		free3(npc->talk_chars);
	}
	if(npc->bbox.poly_f4 != NULL){
		printf("clean npc bbox\n");
		free3(npc->bbox.poly_f4);
	}
}

void npc_update(Npc *npc){
	//npc->mesh.rot.vy += 10;
}

void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id){
	zone->pos.vx = posX;
	zone->pos.vy = posY;
	zone->pos.vz = posZ;
	zone->w = w;
	zone->h = h;
	zone->z = z;
	mesh_init(&zone->mesh, (u_long*)plane_vertices(), 0, 0, 0, 1);
	mesh_set_rgb(&zone->mesh, 255, 0, 0, 1);
	zone->mesh.vertices[1].vx = zone->w;
	zone->mesh.vertices[3].vx = zone->w;
	zone->mesh.vertices[0].vz = zone->z;
	zone->mesh.vertices[1].vz = zone->z;

	zone->mesh.vertices[0].vy = h;
	zone->mesh.vertices[1].vy = h;
	zone->mesh.vertices[2].vy = h;
	zone->mesh.vertices[3].vy = h;

	zone->mesh.pos.vx = zone->pos.vx;
	zone->mesh.pos.vy = zone->pos.vy;
	zone->mesh.pos.vz = zone->pos.vz;
	zone->stage_id = stage_id;
	zone->spawn_id = spawn_id;
}

u_char plane_add(Mesh planes[], unsigned char *planes_len){
	unsigned char i = *planes_len;
	if(i < PLANES_LEN-1){
		int w = 250;
		int d = -200;
		mesh_init(&planes[i], (u_long*)plane_vertices(), 0, 0, 0, 1);
		mesh_set_rgb(&planes[i], 0, 0, 128, 1);
		planes[i].vertices[1].vx = w;
		planes[i].vertices[3].vx = w;
		planes[i].vertices[0].vz = d;
		planes[i].vertices[1].vz = d;
		planes[i].pos.vx = 0;
		planes[i].pos.vy = 0;
		planes[i].pos.vz = 0;
		*planes_len = *planes_len+1;
		return 1;
	}
	return 0;
}

void print_planes(Mesh planes[], unsigned char planes_len){
	unsigned char i = 0;
	printf("======================\n");
	for(i = 0; i < planes_len; i++){
		Mesh p = planes[i];
		printf("{x: %ld y: %ld z: %ld w: %d h: %d d: %d} \n",
		p.pos.vx, p.pos.vy, p.pos.vz,
		p.vertices[1].vx, 0, p.vertices[0].vz);
	}
}

void print_bytes(u_long *buffer, size_t size){
	unsigned char *bytes = (unsigned char *)buffer;
	size_t i = 0;
	printf("bytes: ");
	//for (i = 0; i < sizeof(int)*2; i++) {
	for (i = 0; i < size; i++) {
		printf("%02x ", bytes[i]);
	}
	printf("\n");
}

void background_init(Background *b){
	sprite_init(&b->s0, 255, 255, b->tpages[0]);
	sprite_init(&b->s1, 64, 255, b->tpages[1]);
	b->s1.pos.vx = 255;
}

void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz)){
	draw(&b->s0, otz);
	draw(&b->s1, otz);
}

size_t strcpy_count(char *destination, const char *source) {
	size_t count = 0;
	while(source[count] != '\0'){
		destination[count] = source[count];
		count++;
	}
	destination[count] = '\0';
	return count;
}

const u_char *plane_vertices(){
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
	return vertices;
}

unsigned int nextLevel(unsigned int current_lv){
	return 1000 + ((current_lv+1) * 1000) * 0.8;
}

void window_init(Window *win, long x, long y, int w, int h, u_short tpage_ui){
	memset(win, 0, sizeof(Window));
	sprite_init(&win->background, w, h, NULL);
	sprite_set_rgb(&win->background, 0, 0, 31, 0);
	win->background.pos.vx = x;
	win->background.pos.vy = y;
	//sprite_set_uv(&b->sprite, 0, 195, 200, 60);
	
	sprite_init(&win->borderTopL, w/2, 8, tpage_ui);
	sprite_set_uv(&win->borderTopL, 0, 195, w/2, 8);
	win->borderTopL.pos.vx = x;

	sprite_init(&win->borderTopR, w/2, 8, tpage_ui);
	win->borderTopR.direction = LEFT;
	sprite_set_uv(&win->borderTopR, 0, 195, w/2, 8);
	win->borderTopR.pos.vx = (w/2)+x;
}

void menu_init(Menu *menu, u_short tpage_ui){
	memset(menu, 0, sizeof(Menu));
	window_init(&menu->win_main, 5, 5, 230, SCREEN_HEIGHT-10, tpage_ui);
	window_init(&menu->win_sidebar, 240, 5, 70, SCREEN_HEIGHT-10, tpage_ui);

	memset(&menu->selector, 0, sizeof(Selector));
	sprite_init(&menu->selector.sprite, 20, 22, tpage_ui);
	sprite_set_uv(&menu->selector.sprite, 0, 174, 30, 22);
	menu_set_selector_index(menu, 0);
	
	//sprite_init(&menu.sprite_background, SCREEN_WIDTH, SCREEN_HEIGHT, NULL);
	//sprite_set_rgb(&menu.sprite_background, 0, 0, 31, 0);
	//sprite_set_uv(&b->sprite, 0, 195, 200, 60);
}

void menu_set_selector_index(Menu *menu, u_char index){
	//VECTOR backgroundPos = menu.win_main.background.pos;
	VECTOR sidebarPos = menu->win_sidebar.background.pos;
	if(index == 255)
		index = 0;
	if(index > 2)
		index = 2;
	menu->selector.index = index;
	switch (menu->selector.index){
		case 0:
			menu->selector.sprite.pos.vx = sidebarPos.vx - menu->selector.sprite.w; 	
			menu->selector.sprite.pos.vy = sidebarPos.vy + 5; 	
			break;
		case 1:
			menu->selector.sprite.pos.vx = sidebarPos.vx - menu->selector.sprite.w; 	
			menu->selector.sprite.pos.vy = sidebarPos.vy + 25; 	
			break;
		case 2:
			menu->selector.sprite.pos.vx = sidebarPos.vx - menu->selector.sprite.w; 	
			menu->selector.sprite.pos.vy = sidebarPos.vy + 45; 	
			break;
		default:
			break;
	}
}
