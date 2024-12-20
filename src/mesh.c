#include "mesh.h"
#include <stdio.h>
#include <string.h>
#include <libmath.h>

float _atof(const char *s);
int isdigit(char c);

/*static const u_char *read_str_delimiter(const u_char* ptr, u_char delimiter) {
    while (*ptr && *ptr != delimiter) {
        ptr++;
    }
    return ptr;
}*/

size_t strlen_delimiter(const u_char *ptr, u_char delimiter);

static void obj_reader(u_long *buffer_data){
	u_char *data = (u_char*)buffer_data;
	if(data != NULL)
	{
		const u_char *ptr = data; 
		while (*ptr) 
		{ 
			//size_t line_length;
			u_char line[100];
			const u_char *line_ptr;
			/*const u_char *end = ptr;
			while (*end && *end != '\n')
				end++;*/
			//const u_char *end = read_str_delimiter(ptr, '\n');	
			//line_length = end - ptr;
			size_t line_length = strlen_delimiter(ptr, '\n');
			strncpy(line, ptr, line_length);
			printf("line: %s\n", line);
			line_ptr = line;
			while (*line_ptr && *line_ptr != '\n')
			{
				int value_length;
				u_char value[100];
				/*u_char *c = line_ptr;
				while (*c != ';' && *c != '\0') {
					c++;
				}
				const u_char *c = read_str_delimiter(line_ptr, ';');
				value_length = c - line_ptr;*/
				value_length = strlen_delimiter(line_ptr, ';');
				printf("value_length: %d\n", value_length);

				if (value_length < sizeof(value)) {
					memcpy(value, line_ptr, value_length);
					value[value_length] = '\0';
					printf("value: %s\n", value);
				} else {
					printf("value is too long to be stored in the buffer\n");
				}

				//line_ptr = (*c == ';') ? c + 1 : c;
				line_ptr += value_length + 1;
			}
			//ptr = (*end == '\n') ? end + 1 : end;
			ptr += line_length + 1;
		}
	}
} 

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, short tex_size, short mesh_size) {
	u_char *data = (u_char*) obj;
	memset(mesh, 0, sizeof(Mesh));
	mesh->size = mesh_size;
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
						//printf("v %s\n", t);
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
						//printf("vt t %s\n", t);
						vt[i_vt][i++] = _atof(t);
						//printf2("vt %f\n", vt[i_vt][i-1]);
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
					//printf("%d\n", numbers[i]);
					f[i_f++] = numbers[i];
				}
			}
			// Skip the newline character or point to the end of the string 
			ptr = (*end == '\n') ? end + 1 : end;
		}
		
		mesh->verticesLength = i_v;
		if(mesh->vertices == NULL){
			mesh->vertices = malloc3((i_v+1) * sizeof(SVECTOR));
			if (mesh->vertices == NULL) {
				printf("error on mesh->vertices malloc3 \n");
				exit(1);
			}
		}
		for (i = 0; i < i_v; i++){
			mesh->vertices[i].vx = v[i][0] * mesh->size;
			mesh->vertices[i].vy = v[i][1] * mesh->size;
			mesh->vertices[i].vz = v[i][2] * mesh->size;
		}

		kk = 0;
		n = 0;
		if(mesh->indices == NULL) {
			mesh->indices = malloc3((4 * (mesh->indicesLength+1)) * sizeof(int));
			if (mesh->indices == NULL) {
				printf("error on mesh->indices malloc3 \n");
				exit(1);
			}
		}
		for (i = 0; i < mesh->indicesLength; ++i)
		{
			int k;
			for(k = 0; k < 4; k++){
				mesh->indices[n] = f[kk]-1;
				n++;
				kk += 2;
			}	
		}

		if(tpage != 0){
			mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
			if (mesh->ft4 == NULL) {
				printf("error on mesh->ft4 malloc3 \n");
				exit(1);
			}
			mesh->tpage = tpage;
		}
		else {
			mesh->f4 = malloc3(mesh->indicesLength * sizeof(POLY_F4));
			if (mesh->f4 == NULL) {
				printf("indices length %d\n", mesh->indicesLength);
				printf("sizeof poly %d\n", sizeof(POLY_F4));
				printf("error on mesh->ft4 malloc3 \n");
				exit(1);
			}
		}
		//printf("\n\nallocated %d\n\n", mesh->indicesLength * sizeof(POLY_FT4));
		kk = 1;
		for (i = 0; i < mesh->indicesLength; ++i) {
			if(mesh->tpage != 0)
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
					vt[ff[0]][0]*tex_size, 
					tex_size - vt[ff[0]][1]*tex_size,
		 
					vt[ff[1]][0]*tex_size, 
					tex_size - vt[ff[1]][1]*tex_size,
		 
					vt[ff[3]][0]*tex_size, 
					tex_size - vt[ff[3]][1]*tex_size,

					vt[ff[2]][0]*tex_size, 
					tex_size - vt[ff[2]][1]*tex_size
				);
				SetShadeTex(&mesh->ft4[i], 1);
			}
			else {
				SetPolyF4(&mesh->f4[i]);
				/*mesh->f4[i].r0 = 0;
				mesh->f4[i].g0 = 0;
				mesh->f4[i].b0 = 255;
				SetShadeTex(&mesh->f4[i], 1);*/
			}
		}
	} // data read
}

void mesh_free(Mesh *mesh){
	if(mesh->ft4 != NULL)
		free3(mesh->ft4);
	if(mesh->f4 != NULL)
		free3(mesh->f4);
	if(mesh->vertices != NULL)
		free3(mesh->vertices);
	if(mesh->indices != NULL)
		free3(mesh->indices);
}

void mesh_set_rgb(Mesh *mesh, u_char r, u_char g, u_char b, int semitransparent){
	int i = 0;
	for (i = 0; i < mesh->indicesLength; ++i) {
		if(mesh->ft4 != NULL){
			mesh->ft4[i].r0 = r;
			mesh->ft4[i].g0 = g;
			mesh->ft4[i].b0 = b;
			//setRGB0(&mesh->ft4[i], r, g, b);
			SetShadeTex(&mesh->ft4[i], 0);
			SetSemiTrans(&mesh->ft4[i], semitransparent);
		}
		if(mesh->f4 != NULL){
			mesh->f4[i].r0 = r;
			mesh->f4[i].g0 = g;
			mesh->f4[i].b0 = b;
			SetShadeTex(&mesh->f4[i], 0);
			SetSemiTrans(&mesh->f4[i], semitransparent);
		}
	}
}

void mesh_set_resize(Mesh *mesh, float size){
	int i = 0;
	if(mesh->vertices != NULL){
		for (i = 0; i < mesh->verticesLength; i++){
			mesh->vertices[i].vx = mesh->vertices[i].vx * size;
			mesh->vertices[i].vy = mesh->vertices[i].vy * size;
			mesh->vertices[i].vz = mesh->vertices[i].vz * size;
		}
	}
}

void mesh_set_shadeTex(Mesh *mesh, u_char b){
	int i = 0;
	for (i = 0; i < mesh->indicesLength; ++i) {
		if(mesh->ft4 != NULL)
			SetShadeTex(&mesh->ft4[i], b);
		if(mesh->f4 != NULL)
			SetShadeTex(&mesh->f4[i], b);
	}
}

int mesh_on_plane(long x, long z, Mesh p){
	if(z < p.pos.vz + p.vertices[3].vz && z > p.pos.vz + p.vertices[0].vz &&
	x < p.pos.vx + p.vertices[1].vx && x > p.pos.vx + p.vertices[0].vx){
		return 1;
	}
	return 0;
}

int mesh_collision(Mesh a, Mesh b){
	short m = 50;
	if(a.pos.vz <= b.pos.vz + (b.size+m) && a.pos.vz + m*2 >= b.pos.vz &&
		a.pos.vy <= b.pos.vy + (b.size+m) && a.pos.vy + m >= b.pos.vy &&
		a.pos.vx <= b.pos.vx + (b.size+m) && a.pos.vx + m >= b.pos.vx){
		return 1;
	}
	return 0;
}

int mesh_angle_to(Mesh mesh, long x, long z) {
	double radians = atan2(z - mesh.pos.vz, mesh.pos.vx - x);
	double angle = radians * (180 / 3.14159);
	return angle + 90;
}

void mesh_point_to(Mesh *mesh, long x, long z) {
	if(mesh->rot.vy > 4096)
		mesh->rot.vy = mesh->rot.vy / 4096;
	mesh->rot.vy = (mesh_angle_to(*mesh, x, z) * 4096) / 360;
}

int mesh_looking_at(Mesh *mesh, long x, long z){
	float meshAngle = 0;
	float rot = 0;
	int angle = mesh_angle_to(*mesh, x, z);
	if(mesh->rot.vy > 4096)
		mesh->rot.vy = 0;
	rot = mesh->rot.vy;
	meshAngle = (rot / 4096) * 360;
	printf("angle %d\n", angle);
	printf2("meshAngle %f\n", meshAngle);
	if(meshAngle >= angle - 60 && meshAngle <= angle + 60)
		return 1;
	return 0;
}

void bbox_init(BBox *bb, Mesh *mesh){
	int i = 0;
	memset(bb, 0, sizeof(BBox));
	bb->poly_f4 = malloc3(1 * sizeof(POLY_F4));
	if(bb->poly_f4 == NULL){
		printf("bb->poly_f4 malloc3 failed");
		exit(1);
	}
	for(i = 0; i < 1; i++){
		SetPolyF4(&bb->poly_f4[i]);
		bb->poly_f4[i].r0 = 255;
		bb->poly_f4[i].g0 = 0;
		bb->poly_f4[i].b0 = 255;
		SetSemiTrans(&bb->poly_f4[i], 1);
	}

	/*
 		v -1.000000 0.000000 -1.000000
		v 1.000000 0.000000 -1.000000
		v -1.000000 0.000000 1.000000
		v 1.000000 0.000000 1.000000
		f 1 2 4 3
	*/
	bb->vertices[0].vx = -1;    bb->vertices[1].vx = 1;	
	bb->vertices[0].vy = 0;     bb->vertices[1].vy = 0;	
	bb->vertices[0].vz = -1;    bb->vertices[1].vz = -1;	
	bb->vertices[2].vx = 1;     bb->vertices[3].vx = -1;	
	bb->vertices[2].vy = 0;     bb->vertices[3].vy = 0;	
	bb->vertices[2].vz = 1;     bb->vertices[3].vz = 1;
	
	for(i = 0; i < 4; i++){
		bb->vertices[i].vx *= mesh->size;
		bb->vertices[i].vy *= mesh->size;
		bb->vertices[i].vz *= mesh->size;
	}
	
	bb->pos.vx = mesh->pos.vx;	bb->rot.vx = mesh->rot.vx;
	bb->pos.vy = mesh->pos.vy;	bb->rot.vy = mesh->rot.vy;
	bb->pos.vz = mesh->pos.vz;	bb->rot.vz = mesh->rot.vz;
}

int bbox_collision(long x, long z, BBox bbox){
	if(z < bbox.pos.vz + bbox.vertices[3].vz && z > bbox.pos.vz + bbox.vertices[0].vz &&
	x < bbox.pos.vx + bbox.vertices[1].vx && x > bbox.pos.vx + bbox.vertices[0].vx){
		return 1;
	}
	return 0;
}

/*
===========================================================
			PRIVATE
===========================================================
*/

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
