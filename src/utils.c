#include "utils.h"

int ray_collisions(Sprite *s, Enemy enemies[], long cameraX)
{
	int i, distance = 10000, k, index;
	for(i = 0; i < 3; i++){
		int collision = ray_collision(s, &enemies[i].sprite, cameraX);
		/*if(collision == 1){
			enemies[i].sprite.hitted = 1;
			enemies[i].sprite.hp -= 1;
			enemies[i].blood.posX = enemies[i].sprite.posX;
			enemies[i].blood.posY = enemies[i].sprite.posY;
			enemies[i].blood.posZ = enemies[i].sprite.posZ-5;
			enemies[i].blood.frame = 0;
			break;
		}*/
		if(collision == 1){
			index = i;
			for(k = 0; k < 3; k++){
				if(s->direction == 0){
					if((s->posX - enemies[k].sprite.posX) < distance){
						if(ray_collision(s, &enemies[k].sprite, cameraX) == 1){
							distance = s->posX - enemies[k].sprite.posX;
							index = k;
						}
					}	
				}
				if(s->direction == 1){
					if((enemies[k].sprite.posX - s->posX) < distance){
						if(ray_collision(s, &enemies[k].sprite, cameraX) == 1){
							distance = enemies[k].sprite.posX - s->posX;
							index = k;
						}
					}	
				}
			}
			enemies[index].sprite.hitted = 1;
			enemies[index].sprite.hp -= 1;
			enemies[index].blood.posX = enemies[index].sprite.posX;
			enemies[index].blood.posY = enemies[index].sprite.posY;
			enemies[index].blood.posZ = enemies[index].sprite.posZ-5;
			enemies[index].blood.frame = 0;
			return 1;
		}
	}
	return 0;
}

int ray_collision(Sprite *s1, Sprite *s2, long cameraX){
	if(s2->posX > (cameraX*-1) -850 && s2->posX < (cameraX*-1) +850) 
	{
		if((s1->direction == 1 && s1->posX < s2->posX) || (s1->direction == 0 && s1->posX > s2->posX))
		{
			if(s1->posZ+(s1->h) >= s2->posZ-(s2->h) && s1->posZ <= s2->posZ+(s2->h))
				return 1;
		}
	}
	return 0;
}

int sprite_collision(Sprite *s1, Sprite *s2){
	if(s1->posX+(s1->w/2) >= s2->posX-(s2->w/2) && s1->posX-(s1->w/2) <= s2->posX+(s2->w/2) && 
		s1->posY+(s1->h/2) >= s2->posY-(s2->h/2) && s1->posY-(s1->h/2) <= s2->posY+(s2->h/2) &&
		s1->posZ+(s1->h) >= s2->posZ-(s2->h) && s1->posZ <= s2->posZ+(s2->h) ) 
	{
		return 1;
	}
	return 0;
}

int inCameraView(Sprite s, long cameraX){
	if(s.posX > (cameraX*-1) - 1000 && s.posX < (cameraX*-1) + 1000) 
		return 1;
	return 0;
}
