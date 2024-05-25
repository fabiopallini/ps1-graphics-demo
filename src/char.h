#ifndef CHAR_H 
#define CHAR_H

#include "Mesh.h"

typedef struct 
{
	Mesh mesh;
	unsigned int HP, HP_MAX, MP, MP_MAX;
	unsigned char STR, INT, VIT, AGI, MND; 
	VECTOR battle_pos;
} Character;

#endif
