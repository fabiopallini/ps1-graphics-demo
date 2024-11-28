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

void npc_init(Npc *npc, u_long *cd_obj, u_short tpage, const NpcData *npcData){
	mesh_init(&npc->mesh, cd_obj, tpage, 255, 100);
	npc->mesh.pos.vx = (long)npcData->x;
	npc->mesh.pos.vy = (long)npcData->y;
	npc->mesh.pos.vz = (long)npcData->z;
	npc->mesh.rot.vx = npcData->rx;
	npc->mesh.rot.vy = npcData->ry;
	npc->mesh.rot.vz = npcData->rz;
}

void npc_free(Npc *npc){
	int i = 0;
	printf("cleaning up npc");
	mesh_free(&npc->mesh);
	if(npc->talk_chars != NULL){
		printf("cleaning up %d talk pages\n", npc->talk_pages);
		for(i = 0; i < npc->talk_pages; i++)
			free3(npc->talk_chars[i]);
		free3(npc->talk_chars);
	}
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
	mesh_set_color(&zone->mesh, 0, 0, 0, 1);
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

size_t strcpy_count(char *destination, const char *source) {
	size_t count = 0;
	while(source[count] != '\0'){
		destination[count] = source[count];
		count++;
	}
	destination[count] = '\0';
	return count;
}
