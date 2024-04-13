#include "mesh.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

void mesh_setPoly(Mesh *mesh)
{
	size_t i;
	for (i = 0; i < mesh->indicesLength; ++i){
		if(&mesh->ft4 != NULL)
			SetPolyFT4(&mesh->ft4[i]);
		if(&mesh->f4 != NULL)
			SetPolyF4(&mesh->f4[i]);
	}
}

float _atof(const char *s) {
	float result = 0.0f;
	int sign = 1;
	int hasDecimal = 0;
	float decimalMultiplier = 0.1f;

	if (*s == '-') {
		sign = -1;
		s++;
	} else if (*s == '+') {
		s++;
	}

	while (*s != '\0') {
		if (*s >= '0' && *s <= '9') {
			if (!hasDecimal) {
				result = result * 10 + (*s - '0');
			} else {
				result = result + (*s - '0') * decimalMultiplier;
				decimalMultiplier *= 0.1f;
			}
		} else if (*s == '.') {
			hasDecimal = 1;
		} else {
			// Invalid character
			break;
		}
		s++;
	}

	return result * sign;
}

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, short img_size, short size) {
	u_char *data = (u_char*) obj;
	if(data != NULL)
	{
		float v[2048][3];
		int i_v = 0;
		float vt[2048][2];
		int i_vt = 0;
		int f[2048];
		int i_f = 0;
		size_t i;
		int kk = 1;
		int n = 0;

		int start = 0;
		int end = 0;
		int len = strlen(data);
		int line_length = 0;

		while (end < len) {
			unsigned char line[100];

			while (end < len && data[end] != '\n') {
				end++;
			}

			line_length = end - start;
			memcpy(line, data + start, line_length);
			line[line_length] = '\0';

			if (strncmp(line, "v ", 2) == 0) {
				unsigned char *c = line;
				unsigned i = 0;
				while(*c != '\0'){
					unsigned char t[8];
					if(*c != 'v' &&  *c != ' '){
						memcpy(t, c, 8);
						t[8] = '\0';
						if(*(t) >= 45 && *(t) <= 57){
							v[i_v][i++] = _atof(t); 
						}
						while(*(c) != ' ' && *(c) != '\0' && *(c) != NULL)
							c++;
					}
					c++;
					if(c == NULL || i >= 3)
						break;
				}
				i_v++;

			} 
			if (strncmp(line, "vt", 2) == 0) {
				unsigned char *c = line;
				unsigned int i = 0;
				while(*c != '\0'){
					unsigned char t[8];
					if(*c != 'v' && *c != 't' && *c != ' ' && *c != '/'){
						memcpy(t, c, 8);
						t[7] = '\0';
						vt[i_vt][i++] = _atof(t);
						while(*(c) != ' ' && *(c) != '\0' && *(c) != NULL)
							c++;
					}
					c++;
					if(c == NULL || i >= 3)
						break;
				}
				i_vt++;
			}
			if (strncmp(line, "f", 1) == 0) {
				unsigned char *c = line;
				mesh->indicesLength++;
				while(*c != '\0'){
					size_t count = 1; 
					if(*c != 'f' && *c != ' ' && *c != '/'){
						unsigned char s[10];
						// count number length, checking if c+count value is a number from 0 to 9 (48-57 ASCII)
						while(*(c+count) >= 48 && *(c+count) <= 57){
							count++;
							if(count >=10)
								count = 9;
						}
						memcpy(s, c, count);
						s[count] = '\0';	
						f[i_f++] = atoi(s);
					}
					if(c == NULL)
						break;
					c+=count;
				}
			}
			start = end + 1;
			end = start;
		}

		for (i = 0; i < i_v; i++){
			//tmp[k++] = (short)(size * atof(c));
			if(mesh->vertices == NULL)
				mesh->vertices = malloc3(sizeof(SVECTOR));
			else
				mesh->vertices = realloc3(mesh->vertices, (i+1) * sizeof(SVECTOR));
			
			mesh->vertices[i].vx = v[i][0] * size;
			mesh->vertices[i].vy = v[i][1] * size;
			mesh->vertices[i].vz = v[i][2] * size;
		}

		kk = 0;
		n = 0;
		for (i = 0; i < mesh->indicesLength; ++i)
		{
			int k;
			for(k = 0; k < 4; k++){
				if(mesh->indices == NULL)
					mesh->indices = malloc3(4 * sizeof(int));
				else
					mesh->indices = realloc3(mesh->indices, (4 * (n+1)) * sizeof(int));
				mesh->indices[n] = f[kk]-1;
				n++;
				kk += 2;
			}	

		}

		if(tpage != NULL){
			mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
			mesh_setPoly(mesh);
			mesh->tpage = tpage;
		}
		else {
			mesh->f4 = malloc3(mesh->indicesLength * sizeof(POLY_F4));
			mesh_setPoly(mesh);
		}
		kk = 1;
		for (i = 0; i < mesh->indicesLength; ++i) {
			if(mesh->tpage != NULL)
			{
				int k;
				int ff[4];
				int ii = 0;
				for(k = 0; k < 4; k++){
					ff[ii++] = f[kk] - 1;
					kk += 2;
				}
				//setUV4(&mesh->ft4[i], 0, 0, 128, 0, 0, 128, 128, 128);
				//setUV4(&sprite->poly, x, y, x+w, y, x, y+h, x+w, y+h);
				setUV4(&mesh->ft4[i], 
					vt[ff[0]][0]*img_size, 
					img_size - vt[ff[0]][1]*img_size,
		 
					vt[ff[1]][0]*img_size, 
					img_size - vt[ff[1]][1]*img_size,
		 
					vt[ff[3]][0]*img_size, 
					img_size - vt[ff[3]][1]*img_size,

					vt[ff[2]][0]*img_size, 
					img_size - vt[ff[2]][1]*img_size
				);
				SetShadeTex(&mesh->ft4[i], 1);
			}
			else {
				mesh->f4[i].r0 = 0;
				mesh->f4[i].g0 = 255;
				mesh->f4[i].b0 = 0;
			}
		}
	}
}

void mesh_set_rgb(Mesh *mesh, u_char r, u_char g, u_char b) {
	int i;
	for (i = 0; i < mesh->indicesLength; ++i) {
		mesh->f4[i].r0 = r;
		mesh->f4[i].g0 = g;
		mesh->f4[i].b0 = b;
	}
}
