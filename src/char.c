#include "char.h"
#include <stdio.h>
#include <string.h> 
#include <libmath.h>

void cd_read_file(unsigned char* file_path, u_long** file);

void char_init(Character *c){
	memset(c, 0, sizeof(Character));
}

void char_animation_init(Character *c, u_short n_animations)
{
	c->animations_len = n_animations;
	c->animation_to_play = 0;
	c->play_animation = 0;
	c->meshAnimations = malloc3(c->animations_len * sizeof(MeshAnimation));
	if (c->meshAnimations == NULL) {
		printf("Error on c->meshAnimation malloc3\n");
		exit(1);
	}
}

void char_animation_set(Character *c, char *anim_name, u_char animation_index, u_char start_frame, u_char frames,
u_short tpage, short img_size, short mesh_size)
{
	u_short i = 0;
	u_short n = animation_index;
	u_long *obj_buffer[frames];

	c->meshAnimations[n].start_frame = start_frame;
	c->meshAnimations[n].frames = frames;
	c->meshAnimations[n].timer = 0;
	c->meshAnimations[n].loop = 0;
	c->meshAnimations[n].interval = 7;
	c->meshAnimations[n].current_frame = start_frame;

	c->meshAnimations[n].meshFrames = malloc3(frames * sizeof(Mesh));
	if (c->meshAnimations[n].meshFrames == NULL) {
		printf("Error on c->meshAnimation[n].meshFrames malloc3\n");
		exit(1);
	}
	for(i = 0; i < frames; i++){
		char name[11];
		memset(&c->meshAnimations[n].meshFrames[i], 0, sizeof(Mesh));
		strcpy(name, anim_name);
		sprintf(name + strlen(name), "%d.OBJ", i);
		cd_read_file(name, &obj_buffer[i]);
		mesh_init(&c->meshAnimations[n].meshFrames[i], obj_buffer[i], tpage, img_size, img_size, mesh_size);
		free3(obj_buffer[i]);	
	}
}

void char_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	if(c->play_animation == 1)
	{
		MeshAnimation *animation = &c->meshAnimations[c->animation_to_play];
		animation->timer++;
		if(animation->timer >= animation->interval){
			animation->timer = 0;
			animation->current_frame++;
			if(animation->current_frame >= animation->frames){
				animation->current_frame = animation->start_frame;
				if(animation->loop == 0)
					c->play_animation = 0;
			}
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

Mesh *char_getMesh(const Character *c)
{
	return &c->meshAnimations[c->animation_to_play].meshFrames[0];
}

u_char char_animation_is_over(Character c){
	if(c.meshAnimations[c.animation_to_play].current_frame >= c.meshAnimations[c.animation_to_play].frames -1)
		return 1;
	return 0;
}

void char_play_animation(Character *c, u_char animation_index)
{
	if(c->meshAnimations[c->animation_to_play].current_frame == 0){
		c->animation_to_play = animation_index;
		c->play_animation = 1;
	}
}

void char_free_animation(Character c, u_char animation_index){
	u_char i = 0;
	u_char frames = c.meshAnimations[animation_index].frames;
	for(i = 0; i < frames; i++)
		mesh_free(&c.meshAnimations[animation_index].meshFrames[i]);
}

void char_set_rgb(Character c, u_char r, u_char g, u_char b){
	int i = 0;
	for(i = 0; i < c.animations_len; i++){
		MeshAnimation *animation = &c.meshAnimations[i];
		int n = 0;
		for(n = 0; n < animation->frames; n++){
			mesh_set_rgb(&animation->meshFrames[n], r, g, b, 0);
		}
	}	
}

void char_set_shadeTex(Character c, u_char b){
	int i = 0;
	for(i = 0; i < c.animations_len; i++){
		MeshAnimation *animation = &c.meshAnimations[i];
		int n = 0;
		for(n = 0; n < animation->frames; n++){
			mesh_set_shadeTex(&animation->meshFrames[n], b);
		}
	}	
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
