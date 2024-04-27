#include "utils.h"

int mesh_on_plane(long x, long z, Mesh p){
	if(z < p.pos.vz + p.vertices[3].vz && z > p.pos.vz + p.vertices[0].vz &&
	x < p.pos.vx + p.vertices[1].vx && x > p.pos.vx + p.vertices[0].vx){
		return 1;
	}
	return 0;
}

PlaneNode *plane_create() {
	PlaneNode* newNode = malloc3(sizeof(PlaneNode));
	if(newNode == NULL) {
		printf("error on PlaneNode malloc3 \n");
		return NULL; 
	}
	newNode->next = NULL;
	return newNode;
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

	//newNode->data = malloc3(sizeof(Mesh));
	newNode->data = mesh;
	mesh_init(&newNode->data, (u_long*)vertices, NULL, 0, 1);

	newNode->data.vertices[1].vx = _size[0];
	newNode->data.vertices[3].vx = _size[0];
	newNode->data.vertices[0].vz = _size[2];
	newNode->data.vertices[1].vz = _size[2];
	newNode->data.pos.vx = _pos[0];
	newNode->data.pos.vy = _pos[1];
	newNode->data.pos.vz = _pos[2];
	//newNode->data = data;
	
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
		free(node->data.f4);
		free(node->data.vertices);
		free(node->data.indices);
		free(node);
		node = nextNode;
	}
	planeNode = NULL;
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

int sprite_collision(Sprite *s1, Sprite *s2){
	if(s1->pos.vx+(s1->w/2) >= s2->pos.vx-(s2->w/2) && s1->pos.vx-(s1->w/2) <= s2->pos.vx+(s2->w/2) && 
		s1->pos.vy+(s1->h/2) >= s2->pos.vy-(s2->h/2) && s1->pos.vy-(s1->h/2) <= s2->pos.vy+(s2->h/2) &&
		s1->pos.vz+(s1->h) >= s2->pos.vz-(s2->h) && s1->pos.vz <= s2->pos.vz+(s2->h) ) 
	{
		return 1;
	}
	return 0;
}

int sprite_collision2(Sprite *s1, Sprite *s2){
	if(s1->pos.vx+s1->w >= s2->pos.vx && s1->pos.vx <= s2->pos.vx+s2->w && 
		s1->pos.vy+s1->h >= s2->pos.vy && s1->pos.vy <= s2->pos.vy+s2->h && 
		s1->pos.vz+s1->h >= s2->pos.vz && s1->pos.vz <= s2->pos.vz+s2->h) 
	{
		return 1;
	}
	return 0;
}

int balloon_collision(Sprite *s1, Sprite *s2){
	int m = 50;
	if(s1->pos.vx+s1->w+m >= s2->pos.vx && s1->pos.vx <= s2->pos.vx+s2->w+m && 
		s1->pos.vy+s1->h+m >= s2->pos.vy && s1->pos.vy <= s2->pos.vy+s2->h+m && 
		s1->pos.vz+s1->h+m >= s2->pos.vz && s1->pos.vz <= s2->pos.vz+s2->h+m) 
	{
		return 1;
	}
	return 0;
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
