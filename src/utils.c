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
	u_char *vertices = "v -1.000000 0.000000 -1.000000\n
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

void init_stages(){
	Stage *s;
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
	// hallway
	s = &stages[0];
	s->tim_name = "BK1.TIM";
	s->camera_pos.vx = -185;
	s->camera_pos.vy = 969;
	s->camera_pos.vz = 3121;
	s->camera_rot.vx = 185;
	s->camera_rot.vy = -31;
	s->camera_rot.vz = 0;

	s->planes_length = 2;
	s->planes = malloc3(s->planes_length * sizeof(Mesh));

	mesh_init(&s->planes[0], (u_long*)vertices, NULL, 0, 1);
	s->planes[0].vertices[1].vx = 160;
	s->planes[0].vertices[3].vx = 160;
	s->planes[0].vertices[0].vz = -2100;
	s->planes[0].vertices[1].vz = -2100;
	s->planes[0].pos.vx = 0;
	s->planes[0].pos.vy = 0;
	s->planes[0].pos.vz = 0;

	mesh_init(&s->planes[1], (u_long*)vertices, NULL, 0, 1);
	s->planes[1].vertices[1].vx = 170;
	s->planes[1].vertices[3].vx = 170;
	s->planes[1].vertices[0].vz = -500;
	s->planes[1].vertices[1].vz = -500;
	s->planes[1].pos.vx = -170;
	s->planes[1].pos.vy = 0;
	s->planes[1].pos.vz = -1500;

	s->spawns_length = 3;
	s->spawns = malloc3(s->spawns_length * sizeof(Spawn));

	s->spawns[0].pos.vx = 100;
	s->spawns[0].pos.vy = 0; 
	s->spawns[0].pos.vz = -500;
	s->spawns[0].rot.vx = 0;
	s->spawns[0].rot.vy = 0; 
	s->spawns[0].rot.vz = 0; 

	s->spawns[1].pos.vx = 100;
	s->spawns[1].pos.vy = 0; 
	s->spawns[1].pos.vz = -1900;
	s->spawns[1].rot.vx = 0;
	s->spawns[1].rot.vy = 2048; 
	s->spawns[1].rot.vz = 0; 

	s->spawns[2].pos.vx = -40;
	s->spawns[2].pos.vy = 0; 
	s->spawns[2].pos.vz = -1500;
	s->spawns[2].rot.vx = 0;
	s->spawns[2].rot.vy = 1024*3; 
	s->spawns[2].rot.vz = 0; 
	
	// bedroom 
	s = &stages[1];
	s->tim_name = "BK2.TIM";
	s->camera_pos.vx = -461;
	s->camera_pos.vy = 942;
	s->camera_pos.vz = 2503;
	s->camera_rot.vx = 160;
	s->camera_rot.vy = 195;
	s->camera_rot.vz = 0;

	s->planes_length = 1;
	s->planes = malloc3(s->planes_length * sizeof(Mesh));

	mesh_init(&s->planes[0], (u_long*)vertices, NULL, 0, 1);
	s->planes[0].vertices[1].vx = 230;
	s->planes[0].vertices[3].vx = 230;
	s->planes[0].vertices[0].vz = -1300;
	s->planes[0].vertices[1].vz = -1300;
	s->planes[0].pos.vx = 0;
	s->planes[0].pos.vy = 0;
	s->planes[0].pos.vz = 0;

	s->spawns_length = 1;
	s->spawns = malloc3(s->spawns_length * sizeof(Spawn));

	s->spawns[0].pos.vx = 80;
	s->spawns[0].pos.vy = 0; 
	s->spawns[0].pos.vz = -1000;
	s->spawns[0].rot.vx = 0;
	s->spawns[0].rot.vy = 2048; 
	s->spawns[0].rot.vz = 0; 
	
	// bathroom 
	s = &stages[2];
	s->tim_name = "BK3.TIM";
	s->camera_pos.vx = -15;
	s->camera_pos.vy = 886;
	s->camera_pos.vz = 2542;
	s->camera_rot.vx = 159;
	s->camera_rot.vy = -73;
	s->camera_rot.vz = 0;

	s->planes_length = 1;
	s->planes = malloc3(s->planes_length * sizeof(Mesh));

	mesh_init(&s->planes[0], (u_long*)vertices, NULL, 0, 1);
	s->planes[0].vertices[1].vx = 80;
	s->planes[0].vertices[3].vx = 80;
	s->planes[0].vertices[0].vz = -600;
	s->planes[0].vertices[1].vz = -600;
	s->planes[0].pos.vx = 0;
	s->planes[0].pos.vy = 0;
	s->planes[0].pos.vz = -800;

	s->spawns_length = 1;
	s->spawns = malloc3(s->spawns_length * sizeof(Spawn));

	s->spawns[0].pos.vx = 40;
	s->spawns[0].pos.vy = 0; 
	s->spawns[0].pos.vz = -1200;
	s->spawns[0].rot.vx = 0;
	s->spawns[0].rot.vy = 2048; 
	s->spawns[0].rot.vz = 0; 
}
