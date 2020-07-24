#ifndef MESH_H
#define MESH_H

#include "psx.h"

typedef struct mesh
{
	POLY_FT4 *ft4;
	SVECTOR *vertices, *uv;
	int *indices, *uv_indices;
	int vericesLength, indicesLength;
	u_short tpage;
    int w, h;
    long posX, posY, posZ, angX, angY, angZ;
} Mesh;

void mesh_init(unsigned char data[], Mesh *mesh, int size);
void mesh_setTim(Mesh *mesh, unsigned char img[]);
void mesh_draw(Mesh *mesh);

#endif