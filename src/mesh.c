#include "mesh.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

void mesh_setPoly(Mesh *mesh);

static void loadTim(u_short* tpage, unsigned char image[])
{
	RECT rect;
	GsIMAGE tim;

	// skip the TIM ID and version (magic) by adding 0x4 to the pointer
	GsGetTimInfo ((u_long *)(image+4), &tim);

	// Load pattern into VRAM
	rect.x = tim.px;
   	rect.y = tim.py;
   	rect.w = tim.pw;
   	rect.h = tim.ph;
   	LoadImage(&rect, tim.pixel);

   	// Load CLUT into VRAM
   	rect.x = tim.cx;
   	rect.y = tim.cy;
   	rect.w = tim.cw;
   	rect.h = tim.ch;
   	LoadImage(&rect, tim.clut);

   	// Return TPage
   	(*tpage) = GetTPage(tim.pmode, 1, tim.px, tim.py);
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

void mesh_init(Mesh *mesh, u_long *obj, u_long *img, short img_size, short size) {
	u_char *data = (u_char*) obj;
	if(data != NULL)
	{
		float v[1024][3];
		int i_v = 0;
		float vt[1024][2];
		int i_vt = 0;
		int f[1024];
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

		mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
		mesh_setPoly(mesh);

		loadTim(&mesh->tpage, (u_char*)img);
		kk = 1;
		for (i = 0; i < mesh->indicesLength; ++i) {
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
	}
}
