#ifndef MESH_H
#define MESH_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>

typedef struct 
{
	POLY_FT4 *ft4;
	SVECTOR *vertices, *uv;
	int *indices, *uv_indices;
	int vericesLength, indicesLength;
	u_short tpage;
	int w, h;
	VECTOR pos; 
	SVECTOR rot;
} Mesh;

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, short img_size, short size);

#endif
