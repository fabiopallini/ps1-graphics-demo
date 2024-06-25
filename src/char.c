#include "char.h"
#include "utils.h"
#include <string.h> 
#include <libmath.h>

void char_animation_init(Character *c, u_short n_animations)
{
	c->animation_to_play = 0;
	c->play_animation = 0;
	c->meshAnimations = malloc3(n_animations * sizeof(MeshAnimation));
	if (c->meshAnimations == NULL) {
		printf("Error on c->meshAnimation malloc3\n");
		return;
	}
}

void char_animation_set(Character *c, u_short animation_index, u_short frames, 
u_long *data[], u_short tpage, short img_size, short size)
{
	u_short i = 0;
	u_short n = animation_index;
	c->meshAnimations[n].current_frame = 0;
	c->meshAnimations[n].timer = 0;
	c->meshAnimations[n].frames = frames;

	c->meshAnimations[n].meshFrames = malloc3(frames * sizeof(Mesh));
	if (c->meshAnimations[n].meshFrames == NULL) {
		printf("Error on c->meshAnimation[n].meshFrames malloc3\n");
		return;
	}
	for(i = 0; i < frames; i++){
		memset(&c->meshAnimations[n].meshFrames[i], 0, sizeof(Mesh));
		mesh_init(&c->meshAnimations[n].meshFrames[i], data[i], tpage, img_size, size);
	}
}

void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	if(c->play_animation == 1)
	{
		MeshAnimation *animation = &c->meshAnimations[c->animation_to_play];
		animation->timer++;
		if(animation->timer >= 7){
			animation->timer = 0;
			animation->current_frame++;
			if(animation->current_frame >= animation->frames)
				animation->current_frame = 0;
		}
		animation->meshFrames[animation->current_frame].pos = c->pos;
		animation->meshFrames[animation->current_frame].rot = c->rot;
		drawMesh(&animation->meshFrames[animation->current_frame], _otz);
	}
	else 
	{
		MeshAnimation *animation = &c->meshAnimations[c->animation_to_play];
		animation->timer = 0;
		animation->current_frame = 0;
		animation->meshFrames[animation->current_frame].pos = c->pos;
		animation->meshFrames[animation->current_frame].rot = c->rot;
		drawMesh(&animation->meshFrames[animation->current_frame], _otz);
	}
}

Mesh *char_getMesh(Character c)
{
	return &c.meshAnimations[0].meshFrames[0];
}

int char_angle_to(Character c, long x, long z) {
	double radians = atan2(z - c.pos.vz, c.pos.vx - x);
	double angle = radians * (180 / 3.14159);
	return angle + 90;
}

int char_looking_at(Character *c, long x, long z){
	float meshAngle = 0;
	float rot = 0;
	int angle = char_angle_to(*c, x, z);
	if(c->rot.vy > 4096)
		c->rot.vy = 0;
	rot = c->rot.vy;
	meshAngle = (rot / 4096) * 360;
	if(meshAngle >= angle - 60 && meshAngle <= angle + 60)
		return 1;
	return 0;
}
