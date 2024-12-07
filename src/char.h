#ifndef CHAR_H 
#define CHAR_H

#include "Mesh.h"

typedef struct {
	Mesh *meshFrames;
	u_char start_frame;
	u_char frames;
	u_char timer;
	u_char loop;
	u_char interval;
	u_char current_frame;
} MeshAnimation;

typedef struct 
{
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	unsigned char RUN_SPEED;
	VECTOR pos, map_pos, battle_pos;
	SVECTOR rot, map_rot, battle_rot;

	MeshAnimation *meshAnimations;
	u_char animations_len;
	u_short animation_to_play;
	u_char play_animation;
} Character;

void char_animation_init(Character *c, u_short n_animations);
void char_animation_set(Character *c, u_char animation_index, u_char start_frame, u_char frames,
u_long *data[], u_short tpage, short img_size, short mesh_size);
void char_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz));
Mesh *char_getMesh(const Character *c);
u_char char_animation_is_over(Character c);
void char_play_animation(Character *c, u_char animation_index);
void char_free_animation(Character c, u_char animation_index);
void char_set_rgb(Character c, u_char r, u_char g, u_char b);
void char_set_shadeTex(Character c, u_char b);
int char_looking_at(Character *c, long x, long z);

#endif
