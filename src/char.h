#ifndef CHAR_H 
#define CHAR_H

#include "Mesh.h"
#include "utils.h"

typedef struct {
	Mesh *meshFrames;
	unsigned int current_frame;
	unsigned int timer;
	unsigned int frames;
} MeshAnimation;

typedef struct 
{
	Mesh mesh;
	MeshAnimation *meshAnimations;
	u_short animation_to_play;
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	VECTOR pos, battle_pos;
} Character;

void char_animation_init(Character *c, u_short n_animations);
void char_animation_set(Character *c, u_short animation_index, u_short frames,
u_long *data[], u_short tpage, short img_size, short size);
void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz));

#endif
