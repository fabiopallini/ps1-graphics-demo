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

void planeNode_push(long *_pos, short *_size, Mesh mesh){
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
	PlaneNode *newNode = NULL;
	PlaneNode *current = planeNode;

	newNode = malloc3(sizeof(PlaneNode));
	if(newNode == NULL) {
		printf("error on PlaneNode malloc3 \n");
		return; 
	}

	newNode->data = mesh;
	mesh_init(&newNode->data, (u_long*)vertices, NULL, 0, 1);

	newNode->data.vertices[1].vx = _size[0];
	newNode->data.vertices[3].vx = _size[0];
	newNode->data.vertices[0].vz = _size[2];
	newNode->data.vertices[1].vz = _size[2];
	newNode->data.pos.vx = _pos[0];
	newNode->data.pos.vy = _pos[1];
	newNode->data.pos.vz = _pos[2];
	
	newNode->next = NULL;
	if(current == NULL) {
		planeNode = newNode;
		return;
	}
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = newNode;
}

void planeNode_free(){
	PlaneNode *node = planeNode;
	while(node != NULL) {
		PlaneNode *nextNode = node->next;
		free3(node->data.f4);
		free3(node->data.vertices);
		free3(node->data.indices);
		free3(node);
		node = nextNode;
	}
	planeNode = NULL;
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
	mesh_init(&zone->mesh, (u_long*)vertices, NULL, 0, 1);
	zone->pos.vx = posX;
	zone->pos.vy = posY;
	zone->pos.vz = posZ;
	zone->w = w;
	zone->h = h;
	zone->z = z;
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
