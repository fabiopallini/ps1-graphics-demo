#include "utils.h"
#include <string.h>

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
	mesh_init(&zone->mesh, (u_long*)vertices, 0, 0, 0, 1);
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
