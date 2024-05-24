#ifndef CHAR_H 
#define CHAR_H

typedef struct {
	Mesh mesh;
	int hp, hp_max, mp, mp_max;
	VECTOR battle_pos;
} Character;

#endif
