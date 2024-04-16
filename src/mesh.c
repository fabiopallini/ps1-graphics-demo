#include "mesh.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "libmath.h"

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

int isdigit(char c) {
	if (c >= 48 && c <= 57) {
		return 1;
	} else {
		return 0;
	}
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

		const u_char *ptr = data; 
		while (*ptr) { 
			size_t line_length;
			u_char line[100];
			const u_char *end = ptr;
			// find the end of the line or the end of the string 
			while (*end && *end != '\n')
				end++;

			line_length = end - ptr;
			memcpy(line, ptr, line_length+1);
			line[line_length+1] = '\0';
			//printf("line: %s\n", line);
			
			if (strncmp(line, "v ", 2) == 0) {
				unsigned char *c = line;
				unsigned i = 0;
				while(*c != '\0'){
					unsigned char t[10];
					if(*c != 'v' && *c != ' '){
						memcpy(t, c, 10);
						t[10] = '\0';
						printf("v %s\n", t);
						if(*(t) >= 45 && *(t) <= 57){
							v[i_v][i++] = _atof(t); 
						}
						while(*(c) != ' ' && *(c) != '\0')
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
					unsigned char t[10];
					if(*c != 'v' && *c != 't' && *c != ' ' && *c != '/'){
						memcpy(t, c, 10);
						t[10] = '\0';
						printf("vt t %s\n", t);
						vt[i_vt][i++] = _atof(t);
						printf2("vt %f\n", vt[i_vt][i-1]);
						while(*(c) != ' ' && *(c) != '\0')
							c++;
					}
					c++;
					if(c == NULL || i >= 3)
						break;
				}
				i_vt++;
			}
			if (strncmp(line, "f", 1) == 0) {
				unsigned char *str = line;
				int numbers[8]; // 8 total numbers per line
				int num_index = 0;
				int i = 0;
				mesh->indicesLength++;
				while (str[i] != '\0' && num_index < 8) {
					if (isdigit(str[i]) == 1) { // check if the character is a number 
						int number = 0;
						// number may be > than 9, so read all numbers
						while (isdigit(str[i]) == 1) {
							number = number * 10 + (str[i] - '0');
							i++;
						}
						numbers[num_index] = number;
						num_index++;
					} else {
						i++;
					}
				}

				// print all numbers from f line
				for (i = 0; i < num_index; i++) {
					printf("%d\n", numbers[i]);
					f[i_f++] = numbers[i];
				}
			}
			// Skip the newline character or point to the end of the string 
			ptr = (*end == '\n') ? end + 1 : end;
		}

		for (i = 0; i < i_v; i++){
			//tmp[k++] = (short)(size * atof(c));
			if(mesh->vertices == NULL){
				mesh->vertices = malloc3(sizeof(SVECTOR));
				if (mesh->vertices == NULL) {
					printf("error on mesh->vertices malloc3 \n");
					return; 
				}
			}
			else {
				mesh->vertices = realloc3(mesh->vertices, (i+1) * sizeof(SVECTOR));
				if (mesh->vertices == NULL) {
					printf("error on mesh->vertices realloc3 \n");
					return; 
				}
			}
			
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
				if(mesh->indices == NULL) {
					mesh->indices = malloc3(4 * sizeof(int));
					if (mesh->indices == NULL) {
						printf("error on mesh->indices malloc3 \n");
						return; 
					}
				}
				else {
					mesh->indices = realloc3(mesh->indices, (4 * (n+1)) * sizeof(int));
					if (mesh->indices == NULL) {
						printf("error on mesh->indices realloc3 \n");
						return; 
					}
				}
				mesh->indices[n] = f[kk]-1;
				n++;
				kk += 2;
			}	
		}

		if(tpage != NULL){
			mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
			if (mesh->ft4 == NULL) {
				printf("error on mesh->ft4 malloc3 \n");
				return; 
			}
			mesh->tpage = tpage;
		}
		else {
			mesh->f4 = malloc3(mesh->indicesLength * sizeof(POLY_F4));
			if (mesh->f4 == NULL) {
				printf("indices length %d\n", mesh->indicesLength);
				printf("sizeof poly %d\n", sizeof(POLY_F4));
				printf("error on mesh->ft4 malloc3 \n");
				return; 
			}
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
				SetPolyFT4(&mesh->ft4[i]);
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
				SetPolyF4(&mesh->f4[i]);
				mesh->f4[i].r0 = 0;
				mesh->f4[i].g0 = 0;
				mesh->f4[i].b0 = 255;
				SetShadeTex(&mesh->f4[i], 1);
			}
		}
	} // data read
}
