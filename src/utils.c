#include "utils.h"

void node_push(Node **node, void *data) {
	Node *newNode = malloc3(sizeof(Node));
	if (newNode == NULL) {
		printf("error on Node malloc3\n");
		return;
	}

	newNode->data = data;
	newNode->next = NULL;

	if (*node == NULL) {
		*node = newNode;
	} 
	else 
	{
		Node *current = *node;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = newNode;
	}
}

void node_free(Node **node) {
	Node *current = *node;
	Node *nextNode;
	while (current != NULL) {
		nextNode = current->next;
		free3(current);
		current = nextNode;
	}
	*node = NULL;
}

int inCameraView(Sprite s, long cameraX){
	//printf("sprite pos x %ld \n", s.pos.vx);
	//printf("cameraX %ld \n", cameraX*-1);
	if(s.pos.vx > (cameraX*-1) - 1000 && s.pos.vx < (cameraX*-1) + 1000) 
		return 1;
	return 0;
}

int cameraLeft(long cameraX){
	return (cameraX*-1) - 800;
}

int cameraRight(long cameraX){
	return (cameraX*-1) + 800;
}

void zone_init(Zone *zone, long posX, long posY, long posZ, int w, int h, int z, int stage_id, int spawn_id){
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
	zone->pos.vx = posX;
	zone->pos.vy = posY;
	zone->pos.vz = posZ;
	zone->w = w;
	zone->h = h;
	zone->z = z;
	mesh_init(&zone->mesh, (u_long*)vertices, NULL, 0, 1);
	mesh_set_color(&zone->mesh, 255, 0, 0);
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

size_t strlen_delimiter(const u_char *ptr, u_char delimiter) {
	const u_char *c = ptr;
	while (*c && *c != delimiter) {
		c++;
	}
	return c - ptr;
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
	b->s0.pos.vx = 0;
	b->s0.pos.vy = 0;
	sprite_init(&b->s1, 64, 255, b->tpages[1]);
	b->s1.pos.vx = 255;
	b->s1.pos.vy = 0;
}

void background_draw(Background *b, long otz, void(*draw)(Sprite *sprite, long otz)){
	draw(&b->s0, otz);
	draw(&b->s1, otz);
}

void battle_init(Battle *battle, u_short tpage, int screenW, int screenH){
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
