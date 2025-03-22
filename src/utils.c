#include "utils.h"
#include <string.h>
#include <ctype.h>

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

void menu_draw_list(Window *win, char *list[], int len){
	VECTOR pos = window_get_pos(win);
	int i;
	for(i = 0; i < len; i++){
		long y = pos.vy + (10*i);
		long top = pos.vy - 10;
		long bottom = pos.vy + win->background.h - 10;
		if(y > top && y < bottom)
			drawFont(list[i], pos.vx + 20, y, 0);
	}
}

void menu_view_sidebar(Window *win){
	VECTOR pos = window_get_pos(win);
	drawFont("Equip", pos.vx, pos.vy, 0);
	drawFont("Status", pos.vx, pos.vy + 20, 0);
	drawFont("Item", pos.vx, pos.vy + 40, 0);
}

void menu_init(Menu *menu, void (*win_view)(Window *win), u_short tpage_ui){
	Color color[4] = { 
		{0, 0, 200}, // top left
		{0, 0, 60}, // top right
		{0, 0, 80}, // bottom left
		{0, 0, 40} // bottom right
	};
	memset(menu, 0, sizeof(Menu));
	window_init(&menu->win_main, 5, 5, 230, SCREEN_HEIGHT-10, tpage_ui, color);
	window_init(&menu->win_sidebar, 240, 5, 70, SCREEN_HEIGHT-10, tpage_ui, color);
	window_set_display(&menu->win_main, win_view);
	window_set_display(&menu->win_sidebar, menu_view_sidebar);
	// init menu selector sprite
	sprite_init(&menu->selector.sprite, 20, 22, tpage_ui);
	sprite_set_uv(&menu->selector.sprite, 0, 174, 30, 22);
	menu_selector_set_index(menu, 0);
}

void menu_draw(Menu menu){
	drawSprite(&menu.selector.sprite, 0);
	window_draw(&menu.win_main);
	window_draw(&menu.win_sidebar);
}

// move selector sprite up and down on menu sidebar
void menu_selector_set_index(Menu *menu, u_char index){
	//VECTOR backgroundPos = menu.win_main.background.pos;
	//VECTOR sidebarPos = menu->win_sidebar.background.pos;
	VECTOR sidebarPos = window_get_pos(&menu->win_sidebar);
	int w = menu->selector.sprite.w;
	if(index == 255)
		index = 0;
	if(index > 2)
		index = 2;
	menu->selector.index = index;
	switch(menu->selector.index){
		case 0:
			menu->selector.sprite.pos.vx = sidebarPos.vx - w; 
			menu->selector.sprite.pos.vy = sidebarPos.vy; 	
			break;
		case 1:
			menu->selector.sprite.pos.vx = sidebarPos.vx - w; 	
			menu->selector.sprite.pos.vy = sidebarPos.vy + 20; 	
			break;
		case 2:
			menu->selector.sprite.pos.vx = sidebarPos.vx - w; 	
			menu->selector.sprite.pos.vy = sidebarPos.vy + 40; 	
			break;
		default:
			break;
	}
}

void menu_selector_set_pos(Menu *menu, long x, long y){
	menu->selector.sprite.pos.vx = x; 
	menu->selector.sprite.pos.vy = y; 	
}

void inventory_add_item(Inventory *inv, Item *item){
	Item *i = inventory_get_item_name(inv, item->name);
	if(i == NULL){
		//printf("item %s not found\n", item->name);
		item->count = 1;
		node_push(&inv->node, item, sizeof(Item), GFX_SPRITE);
		inv->count++;
	}
	else {
		//printf("item %s found!\n", item->name);
		// if item with same name is within inventory
		// rename Item to Item 2 -> Item 3 ecc...
		i->count++;
		if(i->count > 1)
			sprintf(i->name, "%s %d", item->name, i->count);
	}
}

void inventory_remove_item(Inventory *inv, Item *item){
	Item *i = inventory_get_item_name(inv, item->name);
	if(i == NULL){
		node_remove(&inv->node, item, 1);
		inv->count--;
	}
	else {
		i->count--;
		if(i->count > 1)
			sprintf(i->name, "%s %d", item->name, i->count);
	}
}

void inventory_all(Inventory inv){
	if(inv.node != NULL){
		Node *node = inv.node;
		while(node != NULL){
			Item *item = node->data;
			printf("inv name %s\n", item->name);
			node = node->next;
		}
	}
}

void inventory_iterator_start(Inventory *inv) {
	inv->current_node = inv->node;
}

Item *inventory_iterator_next(Inventory *inv) {
	Item *item;
	if (inv->current_node == NULL) {
		return NULL;
	}
	item = inv->current_node->data;
	inv->current_node = inv->current_node->next;
	return item;
}

Item *inventory_get_item(Inventory *inv, int n){
	Item *item;
	int i = 0;
	inventory_iterator_start(inv);	
	while((item = inventory_iterator_next(inv)) != NULL){
		if(i == n)
			return item;
		i++;
	}
	return NULL;
}

u_char item_name_cmp(Item *item, char *name){
	char *s1 = item->name;
	char *s2 = name;
	while (*s1 && *s2 && *s1 == *s2) {
		// If we encounter a space followed by a number in both strings, stop
		if (*s1 == ' ' && isdigit(*(s1 + 1))) {
			return 0; // Strings are equal up to the first space and number 
		}
		s1++;
		s2++;
	}
	// Check if both strings stopped at a space followed by a number or at the end
	if ( ((*s1 == ' ' && isdigit(*(s1 + 1))) || *s1 == '\0') && ((*s2 == ' ' && isdigit(*(s2 + 1))) || *s2 == '\0') ) {
		return 0; // Strings are equal up to the first space and number
	}
	return 1;
}

Item *inventory_get_item_name(Inventory *inv, char *name){
	Item *item;
	inventory_iterator_start(inv);	
	while((item = inventory_iterator_next(inv)) != NULL){
		if(item_name_cmp(item, name) == 0)
			return item;
	}
	return NULL;
}
