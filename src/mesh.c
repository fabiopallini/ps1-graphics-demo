#include "mesh.h"

void mesh_setPoly(Mesh *mesh);
float* read_v(char *line);
int* read_f(char *line);

void mesh_init(unsigned char data[], Mesh *mesh, int size)
{
	if(data != NULL)
	{
		char *token = strtok(data, "\n");
		int index = 0;
		int ii = 0;
		printf("mesh INIT \n");
		while(token != NULL){
			printf("token %s \n", token);
			if(token[0] == 'v')
			{
				float *a = read_v(token);
				if(mesh->vertices == NULL)
					mesh->vertices = malloc3(3 * sizeof(SVECTOR));
				else
					mesh->vertices = realloc3(mesh->vertices, sizeof(SVECTOR));

				mesh->vertices[index].vx = size * a[0];
				mesh->vertices[index].vy = size * a[1];
				mesh->vertices[index].vz = size * a[2];
				index++;
			}
			if(token[0] == 'f'){
				int *a = read_f(token);
				int i;
				if(mesh->indices == NULL)
					mesh->indices = malloc3(4 * sizeof(int));
				else
					mesh->indices = realloc3(mesh->indices, 4 * sizeof(int));

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

float* read_v(char *line){
	float data[3];
	float *r;
	int index = 0;
	int length = strlen(line);
	int i;
	for(i = 0; i < length; i++){
		//printf("%c \n", line[i]);
		if(line[i] == ' '){
			char *value;
			float f;
			char c[4];
			c[0] = line[i+1];
			c[1] = line[i+2];
			c[2] = line[i+3];
			c[3] = line[i+4];
			value = c;
			f = atof(c);
			//printf2(":: %.2f \n", f);
			data[index] = f;
			//printf("%d\n", data[index]);
			index++;
		}
	}

	r = data;
	return r;
}

int* read_f(char *line){
	int data[4];
 	int *r;
	int index = 0;
	int length = strlen(line);
	int i;
	for(i = 0; i < length; i++){
		if(line[i] == ' '){
			char *value;
			int n;
			char c[1];
			c[0] = line[i+1];
			value = c;
			n = atoi(c);
			data[index] = n-1;
			//printf("%d\n", data[index]);
			index++;
		}
	}

	r = data;
	return r;
}