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
	POLY_F4 *f4;
	SVECTOR *vertices;
	int *indices;
	int verticesLength, indicesLength;
	u_short tpage;
	int w, h;
	VECTOR pos; 
	SVECTOR rot;
} Mesh;

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, short img_size, short size);
void mesh_init_ptr(Mesh **pmesh, u_long *obj, u_short tpage, short img_size, short size);
void mesh_free(Mesh *mesh);
void mesh_set_color(Mesh *mesh, u_char r, u_char g, u_char b);
void mesh_set_shadeTex(Mesh *mesh, u_char b);
int mesh_on_plane(long x, long z, Mesh p);
int mesh_collision(Mesh a, Mesh b);
int mesh_angle_to(Mesh mesh, long x, long z);
void mesh_point_to(Mesh *mesh, long x, long z);
int mesh_looking_at(Mesh *mesh, long x, long z);

#endif
