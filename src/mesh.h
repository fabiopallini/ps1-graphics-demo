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

void mesh_init(Mesh *mesh, u_long *obj, u_long *img, short img_size, short size);
void mesh_draw(Mesh *mesh, int clip);
void mesh_draw_ot(Mesh *mesh, int clip, long otz);

#endif
