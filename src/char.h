#ifndef CHAR_H 
#define CHAR_H

#include "Mesh.h"
#include "utils.h"

typedef struct {
	Node *meshNode;
	unsigned int current_frame;
	unsigned int timer;
	unsigned int frames;
} MeshAnimation;

typedef struct 
{
	Mesh mesh;
	Node *animationNode;
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	VECTOR battle_pos;
} Character;

void char_animation_set(Mesh meshFrames[], MeshAnimation *meshAnim, Character *c, u_long *cd_data[], u_short tpages[]);
void char_animation_draw(Character *c, long _otz, void(*drawMesh)(Mesh *mesh, long _otz));

#endif
