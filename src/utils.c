#include "utils.h"

int ray_collisions(Sprite *s, Enemy enemies[], int n_enemies, long cameraX)
{
	int i, distance = 10000, k, index;
	for(i = 0; i < n_enemies; i++){
		int collision = 0;
		if(enemies[i].sprite.hp > 0)
			collision = ray_collision(s, &enemies[i].sprite, cameraX);
		if(collision == 1){
			index = i;
			for(k = 0; k < n_enemies; k++){
				if(s->direction == 0){
					if(enemies[k].sprite.hp > 0 && (s->pos.vx - enemies[k].sprite.pos.vx) < distance){
						if(ray_collision(s, &enemies[k].sprite, cameraX) == 1){
							distance = s->pos.vx - enemies[k].sprite.pos.vx;
							index = k;
						}
					}	
				}
				if(s->direction == 1){
					if(enemies[k].sprite.hp > 0 && (enemies[k].sprite.pos.vx - s->pos.vx) < distance){
						if(ray_collision(s, &enemies[k].sprite, cameraX) == 1){
							distance = enemies[k].sprite.pos.vx - s->pos.vx;
							index = k;
						}
					}	
				}
			}
			enemies[index].sprite.hitted = 1;
			enemies[index].sprite.hp -= 1;
			enemies[index].blood.pos.vx = enemies[index].sprite.pos.vx;
			enemies[index].blood.pos.vy = enemies[index].sprite.pos.vy;
			enemies[index].blood.pos.vz = enemies[index].sprite.pos.vz-5;
			enemies[index].blood.frame = 0;
			return 1;
		}
	}
	return 0;
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

int inCameraView(Sprite s, long cameraX){
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
