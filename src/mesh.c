#include "mesh.h"

void mesh_setPoly(Mesh *mesh);
int* read_f(char *line);

void mesh_init(Mesh *mesh, int size, unsigned char data[])
{
	if(data != NULL)
	{
		char *token = strtok(data, "\n");
		int index = 0;
		int ii = 0;
		while(token != NULL){
			if(token[0] == 'v')
			{
				short tmp[3];
				int k = 0;
				int n;
				for(n = 0; n < strlen(token); n++){
					if(token[n] == ' '){
						char c[4];
						c[0] = token[n+1];
						c[1] = token[n+2];
						c[2] = token[n+3];
						c[3] = token[n+4];
						tmp[k++] = (short)(size * atof(c));
					}
				}
				
				if(mesh->vertices == NULL)
					mesh->vertices = malloc3(sizeof(SVECTOR));
				else
					mesh->vertices = realloc3(mesh->vertices, (index+1) * sizeof(SVECTOR));

				mesh->vertices[index].vx = tmp[0];
				mesh->vertices[index].vy = tmp[1];
				mesh->vertices[index].vz = tmp[2];
				index++;
			}
			if(token[0] == 'f'){
				int *a = read_f(token);
				int i;
				if(mesh->indices == NULL)
					mesh->indices = malloc3(4 * sizeof(int));
				else
					mesh->indices = realloc3(mesh->indices, (4 * (ii+1)) * sizeof(int));

				for(i = 0; i < 4; i++)
					mesh->indices[ii++] = a[i];

				mesh->indicesLength++;
			}
			token = strtok(NULL, "\n");
		}
	}

	mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
	mesh_setPoly(mesh);
}

void mesh_setTim(Mesh *mesh, unsigned char img[]){
	size_t i;
	psLoadTim(&mesh->tpage, img);
    for (i = 0; i < mesh->indicesLength; ++i) {
		setUV4(&mesh->ft4[i], 2, 2, 128-2, 2, 2, 128-2, 128-2, 128-2);
		SetShadeTex(&mesh->ft4[i], 1);
	}
}

void mesh_draw(Mesh *mesh)
{
	// UP = -Y
	// FORWARD = +Z
    POLY_FT4 *ft4 = mesh->ft4;
    SVECTOR *v = mesh->vertices;
    int *i = mesh->indices;
    int nclip;
	size_t n;
	psGte(mesh->posX, mesh->posY, mesh->posZ,
		mesh->angX, mesh->angY, mesh->angZ);
	for (n = 0; n < mesh->indicesLength*4; n += 4, ++ft4) {
		nclip = RotAverageNclip4(&v[i[n + 0]],
					 &v[i[n + 1]],
					 &v[i[n + 2]],
					 &v[i[n + 3]],
					 (long *)&ft4->x0, (long *)&ft4->x1,
					 (long *)&ft4->x3, (long *)&ft4->x2,
					 0, 0, 0);

		if (nclip <= 0)
			continue;

		ft4->tpage = mesh->tpage;
        psAddPrimFT4(ft4);
	}
}

void mesh_setPoly(Mesh *mesh)
{
    size_t i;
    for (i = 0; i < mesh->indicesLength; ++i)
		SetPolyFT4(&mesh->ft4[i]);
}

int * read_f(char *line){
	static int data[4];
	int k = 0;
	int i;
	for(i = 0; i < strlen(line); i++){
		if(line[i] == ' '){
			char c[1];
			c[0] = line[i+1];
			data[k++] = atoi(c)-1;
		}
	}

	return data;
}