#ifndef CHAR_H 
#define CHAR_H

#include "Mesh.h"
#include "utils.h"

typedef struct {
	Mesh *meshFrames;
	u_char start_frame;
	u_char frames;
	u_char timer;
	u_char loop;
	u_char speed;

	u_char current_frame;
} MeshAnimation;

typedef struct 
{
	Mesh mesh;
	MeshAnimation *meshAnimations;
	u_short animation_to_play;
	u_char play_animation;
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	VECTOR pos, map_pos, battle_pos;
	SVECTOR rot, map_rot, battle_rot;
} Character;

void char_animation_init(Character *c, u_short n_animations);
void char_animation_set(Character *c, u_char animation_index, u_char start_frame, u_char frames,
u_long *data[], u_short tpage, short img_size, short size);
void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz));
Mesh *char_getMesh(Character c);
int char_looking_at(Character *c, long x, long z);

#endif
