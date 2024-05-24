#ifndef CHAR_H 
#define CHAR_H

typedef struct {
	Mesh mesh;
	int HP, HP_MAX, MP, MP_MAX;
	int STR, INT, VIT, AGI, MND; 
	VECTOR battle_pos;
} Character;

#endif
