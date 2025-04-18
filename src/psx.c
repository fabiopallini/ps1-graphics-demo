#include "psx.h"

//#define SUB_STACK 0x80180000 /* stack for sub-thread. update appropriately. */
#define SUB_STACK_SIZE 0x4000 // 8KB
//#define SUB_STACK_SIZE 0x8000 // 16KB

#define SOUND_MALLOC_MAX 5 
#define VAG_BLOCK_SIZE SECTOR*32
#define SPU_BLOCKS_SIZE VAG_BLOCK_SIZE*3
//#define DEBUG_VAG 

DISPENV	dispenv[2];
DRAWENV	drawenv[2];
int dispid = 0;
u_long ot[OTSIZE];
u_short otIndex = 1;
u_char screenWait = 0;
DslCB cd_read_callback();

unsigned long sub_th,gp;
static volatile unsigned long count1,count2; 
struct ToT *sysToT = (struct ToT *) 0x100 ; /* Table of Tabbles  */
struct TCBH *tcbh ; /* task status queue address */
struct TCB *master_thp,*sub_thp; /* start address of thread context */

static unsigned char sub_stack_area[SUB_STACK_SIZE];
static unsigned char ramAddr[0x19F0A0]; // 1.7MB heap 300k stack
//static unsigned char ramAddr[0x16E360]; // 1.5MB heap 500k stack

#define NUMCHANNELS 8 
// file name on the CD-ROM
// file start position filled in by CdSearchFile
// file end position filled in by CdSearchFile
// channel description (not needed for playing the .XA)
typedef struct XAFILE {
	char* filename; 
	int startpos;
	int endpos;
	char* channelName[NUMCHANNELS];
} XAFILE;
XAFILE xafile = {"\\TRACKS1.XA;1", 0, 0};
u_char xa_buffer[2340];
CdlCB xa_Oldcallback;
CdlCB PrepareXA(void);
int xa_currentPos = 0;
int xa_currentChannel;

static void cbvsync(void)
{
	/* 
	return from interrupt set to main thread. if this is not done, control will
        return to sub thread (in sub_func()).  
	*/
	tcbh->entry = master_thp;
}

/*
program to loop and count up during idle time.
note that functions called from ChangeTh() will not have anywhere to return to.  
*/
static long sub_func()
{
	count1 = 0;
	count2 = 0;
	while(1){
		if(scene.update_billboards){
			Node *node = scene.node;
			while(node != NULL){
				if(node->type == GFX_SPRITE || node->type == GFX_SPRITE_DRAW){
					sprite_billboard((Sprite*)node->data);
					//printf("billboard updated \n");	
				}
				node = node->next;
			}
			scene.update_billboards = 0;
		}
		// pcsxr fix, sometimes misses the Spu IRQ
		if(vagSong.block > 0 && vagSong.state > 0 && SpuGetKeyStatus(SPU_0CH) != 1){
			SpuSetKey(SPU_ON, SPU_0CH); 
			//printf("spu key status %ld\n", SpuGetKeyStatus(SPU_0CH));
		}

		if(vagSong.read_chunk){
#ifdef DEBUG_VAG
			printf("read_chunk true \n");	
#endif
			vagSong.read_chunk = false;
			if(vagSong.data != NULL){
				free3((void*)vagSong.data);
				vagSong.data = NULL;
			}
			// play reached the end of the song, restart reading from begin (0KB - SPU_BOCKS_SIZE KB)
			if(vagSong.state == 2){
				cd_read_file_bytes(vagSong.name, (void*)&vagSong.data, vagSong.chunk_addr, 
					vagSong.chunk_addr + (SPU_BLOCKS_SIZE), VAG_READ);
			}
			else {
				// read next chunk
				cd_read_file_bytes(vagSong.name, (void*)&vagSong.data, vagSong.chunk_addr, 
					vagSong.chunk_addr + vagSong.block_size, VAG_READ);
			}
			vagSong.chunk_addr += vagSong.data_size;
		}
		if(DSR_callback_id == VAG_TRANSFER){
#ifdef DEBUG_VAG
			printf("dsr_callback_id == VAG_TRANSFER\n");	
#endif
			if(!vagSong.state){
				DSR_callback_id = 0;
				return 0;
			}
			// copy the first SPU_BOCKS_SIZE KB on song restart (loop)
			if(vagSong.state == 2){
				vagSong.state = 1;
				SpuSetTransferStartAddr(vagSong.spu_addr);
				SpuWrite((u_char *)vagSong.data, SPU_BLOCKS_SIZE);
			}
			else {
				// copy next chunk in sound memory
				SpuSetTransferStartAddr(vagSong.spu_addr + ((vagSong.block-1) * vagSong.data_size));
				SpuWrite((u_char *)vagSong.data, vagSong.data_size);
			}
			DSR_callback_id = VAG_TRANSFERING;
		}
		/* A Vsync interrupt is received somewhere in this while loop, and control is taken away.
	        Control resumes from there at the next ChangeTh(). */
		count2++;
	}
}

void sprite_init(Sprite *sprite, int w, int h, u_short tpage){
	memset(sprite, 0, sizeof(Sprite));
	sprite->tpage = tpage;
	sprite->w = w;
	sprite->h = h;
	setVector(&sprite->vector[0], -w, -h, 0);
	setVector(&sprite->vector[1], w, -h, 0);
	setVector(&sprite->vector[2], -w, h, 0);
	setVector(&sprite->vector[3], w, h, 0);
	
	if(tpage != 0){
		SetPolyFT4(&sprite->poly.ft4);
		setXY4(&sprite->poly.ft4, 0, 0, w, 0, 0, h, w, h);
		sprite_set_uv(sprite, 0, 0, w, h);
		SetShadeTex(&sprite->poly.ft4, 1); // turn shading OFF 
		/*
		// to apply rgb, shading must be ON
		SetShadeTex(&sprite->ft4, 0); // turn shading ON 
		// 128 set the texture's default brithness
		sprite_set_rgb(sprite, 128, 128, 128, 0);
		*/
	}
	else {
		SetPolyF4(&sprite->poly.f4);
		setXY4(&sprite->poly.f4, 0, 0, w, 0, 0, h, w, h);
		setRGB0(&sprite->poly.f4, 255, 255, 255);
	}
	sprite->prevFrame = -1;
	sprite->frameInterval = 5;
}

void sprite_init_g4(Sprite *sprite, int w, int h, Color color[4]){
	memset(sprite, 0, sizeof(Sprite));
	sprite->w = w;
	sprite->h = h;
	setVector(&sprite->vector[0], -w, -h, 0);
	setVector(&sprite->vector[1], w, -h, 0);
	setVector(&sprite->vector[2], -w, h, 0);
	setVector(&sprite->vector[3], w, h, 0);

	SetPolyG4(&sprite->poly.g4);
	setXY4(&sprite->poly.g4, 0, 0, w, 0, 0, h, w, h);
	setRGB0(&sprite->poly.g4, color[0].r, color[0].g, color[0].b);
	setRGB1(&sprite->poly.g4, color[1].r, color[1].g, color[1].b);
	setRGB2(&sprite->poly.g4, color[2].r, color[2].g, color[2].b);
	setRGB3(&sprite->poly.g4, color[3].r, color[3].g, color[3].b);
}

void sprite_load(Sprite *sprite, char *tim_name){
	u_long *buff;
	u_short tpage;
	GsIMAGE tim;
	cd_read_file(tim_name, &buff);
	GsGetTimInfo(buff+1, &tim);
	tpage = loadToVRAM(buff);
	sprite_init(sprite, tim.pw-1, tim.ph-1, tpage);
	free3(buff);
}

void sprite_shading_disable(Sprite *sprite, int disable){
	// disable == 1 to to turn shading OFF
	// disable == 0 to to turn shading ON 
	SetShadeTex(&sprite->poly.ft4, disable);
}

void sprite_set_uv(Sprite *sprite, int x, int y, int w, int h){
	int x0, y0, x1, y1;
	x0 = x;
	y0 = y;
	x1 = x + w;
	y1 = y + h;
	w = w;
	y = y;
	if(sprite->mirror_h){
		x0 = x + w-1;
		x1 = x;
	}
	if(sprite->mirror_v){
		y0 = y + h-1;
		y1 = y;
	}

	setUV4(&sprite->poly.ft4, 
		x0, y0, 
		x1, y0, 
		x0, y1, 
		x1, y1
	);
}

void sprite_set_rgb(Sprite *sprite, u_char r, u_char g, u_char b, int semitrans) {
	if(sprite->tpage != 0){
		setRGB0(&sprite->poly.ft4, r, g, b);
		if(semitrans){
			SetShadeTex(&sprite->poly.ft4, !semitrans);
			SetSemiTrans(&sprite->poly.ft4, semitrans);
		}
	}
	else{
		setRGB0(&sprite->poly.f4, r, g, b);
		if(semitrans){
			SetShadeTex(&sprite->poly.f4, !semitrans);
			SetSemiTrans(&sprite->poly.f4, semitrans);
		}
	}
}

void sprite_set_animation(Sprite *sprite, int frameW, int frameH, short row, short firstFrame, short frames, u_char loop){
	sprite->animation_loop = loop;
	sprite->frameW = frameW;
	sprite->frameH = frameH;
	sprite->row = row;
	sprite->firstFrame = firstFrame;
	sprite->frames = frames;
	sprite->animation_playing = 1;
}

void sprite_animation(Sprite *sprite){
	if(sprite->animation_playing)
	{
		if(sprite->frame < sprite->firstFrame){
			sprite->prevFrame = -1;
			sprite->frame = sprite->firstFrame;
		}

		sprite->frameTime += 1;
		if(sprite->frameTime >= sprite->frameInterval){
			sprite->prevFrame = sprite->frame;
			sprite->frame += 1;
			if(sprite->frame > (sprite->firstFrame+sprite->frames)-1){
				sprite->prevFrame = -1;
				if(sprite->animation_loop)
					sprite->frame = sprite->firstFrame;
				else 
					sprite->animation_playing = 0;
			}
			sprite->frameTime = 0;
		}

		if(sprite->frame != sprite->prevFrame)
			sprite_set_uv(sprite, 
				sprite->frame * sprite->frameW, 
				sprite->row * sprite->frameH, 
				sprite->frameW, sprite->frameH
			);
	}
}

u_char sprite_animation_over(const Sprite *sprite){
	if(sprite->frame > (sprite->firstFrame+sprite->frames)-1)
		return 1;
	return 0;
}

// display a single frame for X time
short sprite_set_frame(Sprite *sprite, short w, short h, short row, short frame, short time){
	short result = 1;

	sprite->frame = frame;
	sprite_set_uv(sprite, sprite->frame*w, row*h, w, h);

	sprite->frameTime += 1;
	if(sprite->frameTime >= time){
		sprite->frameTime = 0;
		result = 0;
	}
	return result;
}

void sprite_billboard(Sprite *sprite){
	// sprite direction from camera pos
	float dirX = camera.pos.vx - sprite->pos.vx;
	//float dirY = camera.pos.vy - sprite->pos.vy;
	float dirZ = camera.pos.vz - sprite->pos.vz;

	// modify rotation based on camera rotation (Y axis)
	float cosRY = cos(camera.rot.vy * (PI / 180.0));
	float sinRY = sin(camera.rot.vy * (PI / 180.0));

	//float tempX = dirX * cosRY + dirZ * sinRY;
	//float tempZ = -dirX * sinRY + dirZ * cosRY;

	// modify rotation based on camera rotation (X axis)
	//float cosRX = cos(camera.rot.vx * (PI / 180.0));
	//float sinRX = sin(camera.rot.vx * (PI / 180.0));
	//float tempY = dirY * cosRX - tempZ * sinRX;

	// rotation angle Y
	sprite->rot.vy = atan2(dirX * cosRY + dirZ * sinRY, -dirX * sinRY + dirZ * cosRY) * (180.0 / PI);

	// rotation angle X
	//sprite->angX = atan2(tempY, sqrt(tempX * tempX + tempZ * tempZ)) * (180.0 / PI);

	// sprite rotation angle based on camera rotation
	sprite->rot.vy -= camera.rot.vy;
}

int sprite_collision(Sprite *s1, Sprite *s2){
	if(s1->pos.vx+(s1->w/2) >= s2->pos.vx-(s2->w/2) && s1->pos.vx-(s1->w/2) <= s2->pos.vx+(s2->w/2) && 
		s1->pos.vy+(s1->h/2) >= s2->pos.vy-(s2->h/2) && s1->pos.vy-(s1->h/2) <= s2->pos.vy+(s2->h/2) &&
		s1->pos.vz+(s1->h) >= s2->pos.vz-(s2->h) && s1->pos.vz <= s2->pos.vz+(s2->h) ) 
	{
		return 1;
	}
	return 0;
}

int sprite_collision2(Sprite *s1, Sprite *s2){
	if(s1->pos.vx+s1->w >= s2->pos.vx && s1->pos.vx <= s2->pos.vx+s2->w && 
		s1->pos.vy+s1->h >= s2->pos.vy && s1->pos.vy <= s2->pos.vy+s2->h && 
		s1->pos.vz+s1->h >= s2->pos.vz && s1->pos.vz <= s2->pos.vz+s2->h) 
	{
		return 1;
	}
	return 0;
}

int balloon_collision(Sprite *s1, Sprite *s2){
	int m = 50;
	if(s1->pos.vx+s1->w+m >= s2->pos.vx && s1->pos.vx <= s2->pos.vx+s2->w+m && 
		s1->pos.vy+s1->h+m >= s2->pos.vy && s1->pos.vy <= s2->pos.vy+s2->h+m && 
		s1->pos.vz+s1->h+m >= s2->pos.vz && s1->pos.vz <= s2->pos.vz+s2->h+m) 
	{
		return 1;
	}
	return 0;
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

int isdigit(char c) {
	if (c >= 48 && c <= 57) {
		return 1;
	} else {
		return 0;
	}
}

size_t strlen_delimiter(const u_char *ptr, u_char delimiter) {
	const u_char *c = ptr;
	while (*c && *c != delimiter) {
		c++;
	}
	return c - ptr;
}

void mesh_init(Mesh *mesh, u_long *obj, u_short tpage, u_short w, u_short h, short mesh_size) {
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
			u_char line[100];
			size_t line_length = strlen_delimiter(ptr, '\n');
			memcpy(line, ptr, line_length+1);
			line[line_length+1] = '\0';
			//printf("line: %s\n", line);
			
			if (strncmp(line, "v ", 2) == 0) {
				unsigned char *c = line;
				unsigned i = 0;
				while(*c != '\0'){
					// we have three coordinates for vertex: x y z
					//      x         y         z 
					// v -0.190964 -1.359672 -0.396550
					unsigned char coord[10];
					if(*c != 'v' && *c != ' '){
						memcpy(coord, c, 9);
						coord[9] = '\0';
						v[i_v][i++] = _atof(coord); 
						//printf("v %s\n", coord);
						// go to the next coordinate 
						while(*(c) != ' ' && *(c) != '\0')
							c++;
					}
					else { 
						c++; 
					}
					// once added all the three coords we are done
					if(c == NULL || i >= 3)
						break;
				}
				i_v++;
			} 
			if (strncmp(line, "vt", 2) == 0) {
				unsigned char *c = line;
				unsigned int i = 0;
				while(*c != '\0'){
					// we have two coords for vt 
					// vt 0.106954 0.965927
					unsigned char coord[10];
					if(*c != 'v' && *c != 't' && *c != ' '){
						memcpy(coord, c, 9);
						coord[9] = '\0';
						vt[i_vt][i++] = _atof(coord);
						//printf("vt t %s\n", coord);
						//printf2("vt %f\n", vt[i_vt][i-1]);
						// go to the next coordinate 
						while(*(c) != ' ' && *(c) != '\0')
							c++;
					}
					else {
						c++;
					}
					// once added all the two coords we are done
					if(c == NULL || i >= 2)
						break;
				}
				i_vt++;
			}
			if (strncmp(line, "f", 1) == 0) {
				unsigned char *str = line;
				/*
				6 or 8 values per line based on face type
				f 1/2 2/2 4/5 Triangle face
				f 1/2 2/2 4/5 2/1 Quad face
				*/
				int numbers_len = 6;
				int numbers[8]; // max size
				int num_index = 0;
				int i = 0;

				// count how many slashes we have at firt f line
				// 3 slashes == 3 vertices == TRIANGLES
				// 4 slashes == 4 vertices == QUADS 
				int slashCount = 0;
				if(mesh->type == 0){
					while (str[i] != '\n' && slashCount < 4) {
						if(str[i] == '/'){
							slashCount++;
						}
						i++;
					}
					//printf("slashCount %d\n", slashCount);
					mesh->type = QUADS;
					if(slashCount == 3)
						mesh->type = TRIANGLES;
					i = 0;
				}

				if(mesh->type == QUADS)
					numbers_len = 8;
				mesh->indicesLength++;
				while (str[i] != '\0' && num_index < numbers_len) {
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
			ptr += line_length + 1;
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
			switch(mesh->type){
				case QUADS:
				mesh->ft4 = malloc3(mesh->indicesLength * sizeof(POLY_FT4));
				if (mesh->ft4 == NULL) {
					printf("error on mesh->ft4 malloc3 \n");
					exit(1);
				}
				break;
				case TRIANGLES:
				mesh->ft3 = malloc3(mesh->indicesLength * sizeof(POLY_FT3));
				if (mesh->ft3 == NULL) {
					printf("error on mesh->ft3 malloc3 \n");
					exit(1);
				}
				break;
			}
			mesh->tpage = tpage;
		}
		else {
			switch(mesh->type){
				case QUADS:
				mesh->f4 = malloc3(mesh->indicesLength * sizeof(POLY_F4));
				if (mesh->f4 == NULL) {
					printf("error on mesh->ft4 malloc3 \n");
					exit(1);
				}
				break;
				case TRIANGLES:
				mesh->f3 = malloc3(mesh->indicesLength * sizeof(POLY_F3));
				if (mesh->f3 == NULL) {
					printf("error on mesh->f4 malloc3 \n");
					exit(1);
				}
				break;
			}
		}
		//printf("\n\nallocated %d\n\n", mesh->indicesLength * sizeof(POLY_FT4));
		kk = 1;
		for (i = 0; i < mesh->indicesLength; ++i) {
			if(mesh->tpage != 0)
			{
				int k;
				/* 
 				3 or 4 values for uv face, based on face type
				first value is for geometry, second value is for uv
				f 1/2 2/2 4/5 Triangle face
				f 1/2 2/2 4/5 2/1 Quad face
				*/
				int ff_len = 3;
				int ff[4]; // max len
				int ii = 0;
				if(mesh->type == QUADS)
					ff_len = 4;
				for(k = 0; k < ff_len; k++){
					ff[ii++] = f[kk] - 1;
					kk += 2;
				}
				switch(mesh->type){
					case QUADS:
					/*setUV4(&mesh->ft4[i], 0, 0, 128, 0, 0, 128, 128, 128);
					setUV4(&mesh->ft4[i], 
						x, y, 
						x+w, y, 
						x, y+h);
					*/
					SetPolyFT4(&mesh->ft4[i]);
					setUV4(&mesh->ft4[i], 
						vt[ff[0]][0]*w, h - (vt[ff[0]][1]*h),
						vt[ff[1]][0]*w, h - (vt[ff[1]][1]*h),
						vt[ff[3]][0]*w, h - (vt[ff[3]][1]*h),
						vt[ff[2]][0]*w, h - (vt[ff[2]][1]*h)
					);
					SetShadeTex(&mesh->ft4[i], 1);
					break;
					case TRIANGLES:
					SetPolyFT3(&mesh->ft3[i]);
					setUV3(&mesh->ft3[i], 
						vt[ff[0]][0]*w, h - (vt[ff[0]][1]*h),
						vt[ff[2]][0]*w, h - (vt[ff[2]][1]*h),
						vt[ff[1]][0]*w, h - (vt[ff[1]][1]*h)
					);
					SetShadeTex(&mesh->ft3[i], 1);
					break;
				}
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

void mesh_load(Mesh *mesh, char *obj_name, char *tim_name, short mesh_size){
	u_long *buff_obj, *buff_tim;
	GsIMAGE tim;
	cd_read_file(obj_name, &buff_obj);
	cd_read_file(tim_name, &buff_tim);
	GsGetTimInfo(buff_tim+1, &tim);
	//printf("tim x:%d y:%d w:%d h:%d\n", tim.px, tim.py, tim.pw, tim.ph);
	mesh_init(mesh, buff_obj, loadToVRAM(buff_tim), tim.pw-1, tim.ph-1, mesh_size);
	free3(buff_obj);
	free3(buff_tim);
}

void mesh_free(Mesh *mesh){
	if(mesh == NULL) return;
	if(mesh->ft4 != NULL){
		free3(mesh->ft4);
		mesh->ft4 = NULL;
	}
	if(mesh->f4 != NULL){
		free3(mesh->f4);
		mesh->f4 = NULL;
	}
	if(mesh->ft3 != NULL){
		free3(mesh->ft3);
		mesh->ft3 = NULL;
	}
	if(mesh->f3 != NULL){
		free3(mesh->f3);
		mesh->f3 = NULL;
	}
	if(mesh->vertices != NULL){
		free3(mesh->vertices);
		mesh->vertices = NULL;
	}
	if(mesh->indices != NULL){
		free3(mesh->indices);
		mesh->indices = NULL;
	}
	mesh = NULL;
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

void bbox_init(BBox *bb, Mesh *mesh, u_short size){
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
		bb->vertices[i].vx *= size;
		bb->vertices[i].vy *= size; 
		bb->vertices[i].vz *= size;
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

void balloon_init(Balloon *b, u_short tpage, int screen_w, int screen_h){
	int w = 200;
	int h = 50;
	Color color[4] = { 
		{0, 0, 200}, // top left
		{0, 0, 60}, // top right
		{0, 0, 80}, // bottom left
		{0, 0, 40} // bottom right
	};
	memset(b, 0, sizeof(Balloon));
	window_init(&b->window, 
		(screen_w / 2) - (w / 2),
		screen_h -( h + 10),
		w, h, tpage, color
	);
	b->page_index = 0;
	b->pages_length = 0;
	b->npc_id = 0;
}

void init_ui(u_short tpage, int screen_width, int screen_height){
	balloon_init(&balloon, tpage, screen_width, screen_height);
}

void set_balloon(Balloon *b, char *text){
	b->prev_display = 0;
	b->display = 1;
	b->text = text; 
}

void model_init(Model *m){
	memset(m, 0, sizeof(Model));
}

void model_animation_init(Model *m, u_short n_animations){
	m->animations_len = n_animations;
	m->animation_to_play = 0;
	m->play_animation = 0;
	m->meshAnimations = malloc3(m->animations_len * sizeof(MeshAnimation));
	if (m->meshAnimations == NULL) {
		printf("Error on m->meshAnimation malloc3\n");
		exit(1);
	}
}

void model_animation_set(Model *m, char *anim_name, u_char animation_index, u_char start_frame, u_char frames,
u_short tpage, short img_size, short mesh_size)
{
	u_short i = 0;
	u_short n = animation_index;
	u_long *obj_buffer[frames];

	m->meshAnimations[n].start_frame = start_frame;
	m->meshAnimations[n].frames = frames;
	m->meshAnimations[n].timer = 0;
	m->meshAnimations[n].loop = 0;
	m->meshAnimations[n].interval = 7;
	m->meshAnimations[n].current_frame = start_frame;

	m->meshAnimations[n].meshFrames = malloc3(frames * sizeof(Mesh));
	if (m->meshAnimations[n].meshFrames == NULL) {
		printf("Error on m->meshAnimation[n].meshFrames malloc3\n");
		exit(1);
	}
	for(i = 0; i < frames; i++){
		char name[11];
		memset(&m->meshAnimations[n].meshFrames[i], 0, sizeof(Mesh));
		strcpy(name, anim_name);
		sprintf(name + strlen(name), "%d.OBJ", i);
		cd_read_file(name, &obj_buffer[i]);
		mesh_init(&m->meshAnimations[n].meshFrames[i], obj_buffer[i], tpage, img_size, img_size, mesh_size);
		free3(obj_buffer[i]);	
	}
}

void model_draw(Model *m, long _otz, void(*drawMesh)(Mesh *mesh, long _otz))
{
	if(m->play_animation == 1)
	{
		MeshAnimation *animation = &m->meshAnimations[m->animation_to_play];
		animation->timer++;
		if(animation->timer >= animation->interval){
			animation->timer = 0;
			animation->current_frame++;
			if(animation->current_frame >= animation->frames){
				animation->current_frame = animation->start_frame;
				if(animation->loop == 0)
					m->play_animation = 0;
			}
		}
		animation->meshFrames[animation->current_frame].pos = m->pos;
		animation->meshFrames[animation->current_frame].rot = m->rot;
		drawMesh(&animation->meshFrames[animation->current_frame], _otz);
	}
	else 
	{
		MeshAnimation *animation = &m->meshAnimations[m->animation_to_play];
		animation->timer = 0;
		animation->current_frame = 0;
		animation->meshFrames[animation->current_frame].pos = m->pos;
		animation->meshFrames[animation->current_frame].rot = m->rot;
		drawMesh(&animation->meshFrames[animation->current_frame], _otz);
	}
}

Mesh *model_getMesh(const Model *m)
{
	return &m->meshAnimations[m->animation_to_play].meshFrames[0];
}

u_char model_animation_is_over(Model m){
	if(m.meshAnimations[m.animation_to_play].current_frame >= m.meshAnimations[m.animation_to_play].frames -1)
		return 1;
	return 0;
}

u_char model_get_frame(Model m){
	return m.meshAnimations[m.animation_to_play].current_frame;
}

void model_play_animation(Model *m, u_char animation_index)
{
	if(m->meshAnimations[m->animation_to_play].current_frame == 0){
		m->animation_to_play = animation_index;
		m->play_animation = 1;
	}
}

void model_free_animation(Model m, u_char animation_index){
	u_char i = 0;
	u_char frames = m.meshAnimations[animation_index].frames;
	for(i = 0; i < frames; i++)
		mesh_free(&m.meshAnimations[animation_index].meshFrames[i]);
}

void model_set_rgb(Model m, u_char r, u_char g, u_char b){
	int i = 0;
	for(i = 0; i < m.animations_len; i++){
		MeshAnimation *animation = &m.meshAnimations[i];
		int n = 0;
		for(n = 0; n < animation->frames; n++){
			mesh_set_rgb(&animation->meshFrames[n], r, g, b, 0);
		}
	}	
}

void model_set_shadeTex(Model m, u_char b){
	int i = 0;
	for(i = 0; i < m.animations_len; i++){
		MeshAnimation *animation = &m.meshAnimations[i];
		int n = 0;
		for(n = 0; n < animation->frames; n++){
			mesh_set_shadeTex(&animation->meshFrames[n], b);
		}
	}	
}

int model_angle_to(Model m, long x, long z) {
	double radians = atan2(z - m.pos.vz, m.pos.vx - x);
	double angle = radians * (180 / 3.14159);
	return angle + 90;
}

int model_looking_at(Model *m, long x, long z){
	float meshAngle = 0;
	float rot = 0;
	int angle = model_angle_to(*m, x, z);
	if(m->rot.vy > 4096)
		m->rot.vy = 0;
	rot = m->rot.vy;
	meshAngle = (rot / 4096) * 360;
	if(meshAngle >= angle - 60 && meshAngle <= angle + 60)
		return 1;
	return 0;
}

void xa_play(int *channel){
	CdlFILE fp; // CD file details
	CdInit(); // init the CD-ROM
	//CdSetDebug(3); // CD-ROM debugging is on
	if(CdSearchFile(&fp, xafile.filename) == 0){
		printf("%s not found!\n", xafile.filename);
	}
	else {
		xafile.startpos = CdPosToInt(&fp.pos);
		// get the XA file end position, start pos + number of sectors -1
		xafile.endpos = xafile.startpos + (fp.size/2048) -1;
		xa_Oldcallback = PrepareXA();
		//PlayXAChannel(0,xafile.startpos,xafile.endpos);

		*channel = *channel % NUMCHANNELS; 
		xa_currentChannel = *channel;	
		PlayXAChannel(*channel, xafile.startpos, xafile.endpos);
	}
}

void xa_stop(){
	CdControlF(CdlPause, 0);
	VSync(3);
	UnprepareXA(xa_Oldcallback);
	CdSync(0, 0);
}

// plays channel zero of an .XA file
void PlayXAChannel(int channelNum, int startPos, int endpos)
{
	CdlLOC  loc;
	CdlFILTER theFilter;
	// set the volume to max
	SsSetSerialVol(SS_SERIAL_A,127,127);
	// set up the XA filter
	theFilter.file = 1;
	theFilter.chan = channelNum;
	CdControlF(CdlSetfilter, (u_char *)&theFilter);
	// starting position on the CD
	CdIntToPos(startPos, &loc);
	xa_currentPos = startPos;
	// begin playing
	CdControlF(CdlReadS, (u_char *)&loc);
	return;
}

// set the CD mode and hook in the callback for XA playing
CdlCB PrepareXA(void)
{
	u_char param[4];
	// setup the XA playback - adjust the speed as needed by your XA samples
	param[0] = CdlModeSpeed|CdlModeRT|CdlModeSF|CdlModeSize1; // 2X speed
  	//param[0] = CdlModeRT|CdlModeSF|CdlModeSize1; // 1X speed
	CdControlB(CdlSetmode, param, 0);
	CdControlF(CdlPause,0);
	return CdReadyCallback((CdlCB)cbready);
}

// sets the CD back to double speed mode, ready for reading data
// returns the current callback if required
void UnprepareXA(CdlCB oldCallback)
{
	u_char param[4];
	// reset any callback that we replaced
	CdControlF(CdlPause,0);
	CdReadyCallback((void *)oldCallback);
	// clear XA mode
	param[0] = CdlModeSpeed;
	CdControlB(CdlSetmode, param, 0);
	return;
}

// callback used to monitor when to stop playing the XA channel
// with one channel XA files, the data callback will occur at least 7 times for every one audio sector
void cbready(int intr, u_char *result)
{
	if (intr == CdlDataReady)
	{
		/*CdGetSector((u_long*)xa_buffer,585);
		xa_currentPos = CdPosToInt((CdlLOC *)xa_buffer);
		printf("\n\ncurrentPos %d endpos %d\n\n", xa_currentPos, xafile.endpos);
		if(xa_currentPos >= xafile.endpos){
			SsSetSerialVol(SS_SERIAL_A,0,0);
			CdControlF(CdlPause,0);
		}*/
		xa_play(&xa_currentChannel);
	}
	else
		printf("UnHandled Callback Occured\n");	
}

int xa_get_currentSecond(){
	CdlLOC *loc;
	u_char bcd; // binary-coded decimal
	unsigned char tens, units;
	CdGetSector((u_long*)xa_buffer,585);
	xa_currentPos = CdPosToInt((CdlLOC *)xa_buffer);
	loc = CdIntToPos(xa_currentPos, (CdlLOC *)xa_buffer);
	/*
	bcd >> 4 shifts the 4 most significant bits of the BCD byte into 
	the positions of the 4 least significant bits. This gives you the tens digit.
	& 0x0F masks the 4 most significant bits, leaving only the 4 least 
	significant bits, which represent the units digit.
	*/
	bcd = loc->second;
	//tens = (bcd >> 4) & 0x0F;
	tens = (bcd >> 4);
	units = bcd & 0x0F;
	return tens * 10 + units; 
}

void clearVRAM_at(int x, int y, int w, int h)
{
	RECT rectTL;
	setRECT(&rectTL, x, y, w, h);
	ClearImage2(&rectTL, 0, 0, 0);
	DrawSync(0);
	//while(DrawSync(1));
}

void clearVRAM()
{
	RECT rectTL;
	setRECT(&rectTL, 0, 0, 1024, 512);
	ClearImage2(&rectTL, 0, 0, 0);
	DrawSync(0);
	//while(DrawSync(1));
}

void fntColor()
{
	int FONTX = 960;
	int FONTY = 256;
	CVECTOR fntColor = { 255, 255, 255 };
	CVECTOR fntColorBG = { 0, 0, 255};
	// The debug font clut is at tx, ty + 128
	// tx = bg color
	// tx + 1 = fg color
	// We can override the color by drawing a rect at these coordinates
	// 
	// Define 1 pixel at 960,128 (background color) and 1 pixel at 961, 128 (foreground color)
	RECT fg = { FONTX+1, FONTY + 128, 1, 1 };
	RECT bg = { FONTX, FONTY + 128, 1, 1 };
	// Set colors
	ClearImage(&fg, fntColor.r, fntColor.g, fntColor.b);
	ClearImage(&bg, fntColorBG.r, fntColorBG.g, fntColorBG.b);
}

void psInit()
{
	//InitHeap3((void*)0x800F8000, 0x00100000);
	InitHeap3((u_long*)ramAddr, sizeof(ramAddr));
	//printf("\n\n ramAddr %p \n\n", (u_long*)ramAddr);

	SetConf(16,4,0x80200000);
	tcbh = (struct TCBH *) sysToT[1].head;
	master_thp = tcbh->entry;
	gp = GetGp();
	EnterCriticalSection();
	//sub_th = OpenTh(sub_func, SUB_STACK, gp);
	sub_th = OpenTh(sub_func, (u_long)(sub_stack_area + SUB_STACK_SIZE), gp);
	//printf("\n\n sub_stack %lx \n\n", (u_long)(sub_stack_area + SUB_STACK_SIZE));
	ExitCriticalSection();
	sub_thp = (struct TCB *) sysToT[2].head + (sub_th & 0xffff);
	sub_thp->reg[R_SR] = 0x404;

	ResetCallback();
	ResetGraph(0);
	VSyncCallback(cbvsync);	
	PadInit(0);

	#ifdef PAL
		SetVideoMode(MODE_PAL);
	#else
		SetVideoMode(MODE_NTSC);
	#endif

	GsInitGraph(SCREEN_WIDTH, SCREEN_HEIGHT, GsINTER|GsOFSGPU, 1, 1);
	SetGeomOffset(160, 120);
	SetGeomScreen(512);

	SetDefDispEnv(&dispenv[0], 0, SCREEN_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[1], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[0], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[1], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

	#ifdef PAL
		dispenv[0].screen.x = 0;
		dispenv[0].screen.y = 16;
		dispenv[0].screen.w = 256;
		dispenv[0].screen.h = 256;

		dispenv[1].screen.x = 0;
		dispenv[1].screen.y = 16;
		dispenv[1].screen.w = 256;
		dispenv[1].screen.h = 256;
	#endif

	clearVRAM();

 	// load the font from the BIOS into the framebuffer
	FntLoad(960, 256);
	// screen X,Y | max text length X,Y | autmatic background clear 0,1 | max characters
	FntOpen(5, 5, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 512);	
	SetDumpFnt(0);
	fntColor();

	drawenv[0].isbg = 1;
	setRGB0(&drawenv[0], 0,0,0);
	drawenv[1].isbg = 1;
	setRGB0(&drawenv[1], 0,0,0);
	
	memset(&scene, 0, sizeof(Scene));
	scene.status = SCENE_READY;
	DSR_callback_id = 0;
	DsReadCallback((DslCB)cd_read_callback);
	font_init();
	cd_open();
	spu_init();

	camera.pos.vx = 0;
	camera.pos.vz = 2300;
	camera.pos.vy = 900;
	camera.rot.vx = 200;
	camera.rot.vy = 0;
	camera.rot.vz = 0;
}

void psClear(){
	pad = PadRead(0);
	dispid = (dispid + 1) %2;
	//ClearOTag(ot, OTSIZE);
	ClearOTagR(ot, OTSIZE);
	RotMatrix(&camera.rot, &camera.mtx);
	ApplyMatrixLV(&camera.mtx, &camera.pos, &camera.tmp);	
	TransMatrix(&camera.mtx, &camera.tmp);
	SetRotMatrix(&camera.mtx);
	SetTransMatrix(&camera.mtx);
	font->index = 0;
}

void psDisplay(){
	opad = pad;
	FntFlush(0);
	DrawSync(0);
	// comment VSync(0) if ChangeTh(sub_th) is running
	ChangeTh(sub_th);
	//VSync(0);
	PutDispEnv(&dispenv[dispid]);
	PutDrawEnv(&drawenv[dispid]);
	//DrawOTag(ot);
	DrawOTag(ot+OTSIZE-1);
	//FntPrint(font_id, "free time = %d\n", count2 - count1);
	count1 = count2;
	otIndex = 1;

	if(!screenWait)
		screenWait = 1;
	if(screenWait == 1){
		enableScreen();
		screenWait++;
	}
}

void psExit(){
	EnterCriticalSection();
	CloseTh(sub_th);
	ExitCriticalSection();
	StopCallback();
	PadStop();
}

void psGte(VECTOR pos, SVECTOR rot){
	//0'-360' = 0-4096
	MATRIX m;
	RotMatrix(&rot, &m);
	TransMatrix(&m, &pos);
	CompMatrixLV(&camera.mtx, &m, &m);
	SetRotMatrix(&m);
	SetTransMatrix(&m);
}

// LOAD DATA FROM CD-ROM
int didInitDs = 0;

void cd_open() {
	if(!didInitDs) {
		didInitDs = 1;
		DsInit();
	}
}

void cd_close() {
	if(didInitDs) {
		didInitDs = 0;
		DsClose();
	}
}

u_long cd_read_file(unsigned char* file_path, u_long** file) {
	u_char* file_path_raw;
	int* sectors_size;
	DslFILE* temp_file_info;
	sectors_size = malloc3(sizeof(int));
	temp_file_info = malloc3(sizeof(DslFILE));

	// Exit if libDs isn't initialized
	if(!didInitDs) {
		printf("LIBDS not initialized, run cdOpen() first\n");	
		return 0;
	}

	// Get raw file path
	file_path_raw = malloc3(4 + strlen(file_path));
	strcpy(file_path_raw, "\\");
	strcat(file_path_raw, file_path);
	strcat(file_path_raw, ";1");
	while(DsReadSync(NULL));
	DsSearchFile(temp_file_info, file_path_raw);
	// Read the file if it was found
	if(temp_file_info->size > 0) {
		//printf("loading %s, size: %lu\n", file_path_raw, temp_file_info->size);
		*sectors_size = temp_file_info->size + (SECTOR % temp_file_info->size);
		/*printf("...file buffer size needed: %d\n", *sectors_size);
		printf("...sectors needed: %d\n", (*sectors_size + SECTOR - 1) / SECTOR);*/
		*file = malloc3(*sectors_size + SECTOR);
		if(*file == NULL){
			printf("file %s malloc3 failed\n", file_path);
			return 0;
		}	
		
		if(!DsRead(&temp_file_info->pos, (*sectors_size + SECTOR - 1) / SECTOR, *file, DslModeSpeed)){
			printf("cd_read_file DsRead failed\n");
		}
		while(DsReadSync(NULL));
#ifdef DEBUG
		printf("file %s loaded!\n", file_path_raw);
#endif
	} else {
		printf("file %s not found\n", file_path_raw);
		exit(1);
	}

	// Clean up
	free3(file_path_raw);
	free3(sectors_size);
	free3(temp_file_info);
	return temp_file_info->size;
}

DslCB cd_read_callback(){
#ifdef DEBUG_VAG
	printf("cd_read_callback \n");	
#endif
	if(DSR_callback_id == VAG_READ){
		DSR_callback_id = VAG_TRANSFER;
	}
	return 0;
}

void cd_read_file_bytes(unsigned char* file_path, u_long** file, unsigned long start_byte, unsigned long end_byte, u_char callbackID){
	u_char* file_path_raw;
	int* sectors_size;
	DslFILE* temp_file_info;
	DslLOC start_loc;
	unsigned long bytes_to_read;
	int file_sector;
	sectors_size = malloc3(sizeof(int));
	temp_file_info = malloc3(sizeof(DslFILE));

	if(!didInitDs) {
		printf("LIBDS not initialized, run cdOpen() first\n");    
		return;
	}

	// Get raw file path
	file_path_raw = malloc3(4 + strlen(file_path));
	strcpy(file_path_raw, "\\");
	strcat(file_path_raw, file_path);
	strcat(file_path_raw, ";1");
	while(DsReadSync(NULL));
	if(callbackID != 0)
		DSR_callback_id = callbackID;
	DsSearchFile(temp_file_info, file_path_raw);
	// Read the file if it was found
	if(temp_file_info->size) {
		//printf("loading %s, size: %lu\n", file_path_raw, temp_file_info->size);
		if (end_byte > temp_file_info->size) {
			printf("...end_byte exceeds file size, adjusting to file size\n");
			end_byte = temp_file_info->size;
		}

		file_sector = DsPosToInt(&temp_file_info->pos);
		file_sector += start_byte / SECTOR;
		DsIntToPos(file_sector, &start_loc);

#ifdef DEBUG_VAG
		printf("loading file %s from byte %lu to byte %lu\n", file_path_raw, start_byte, end_byte);
#endif
		bytes_to_read = end_byte - start_byte;

		*sectors_size = bytes_to_read + (SECTOR % bytes_to_read);
		*file = malloc3(*sectors_size + SECTOR);

		//printf("cd_read_file_bytes sectors_size + sector %d\n", *sectors_size + SECTOR);
		if (*file == NULL) {
			printf("file %s malloc3 failed\n", file_path);
			DSR_callback_id = 0;
			// Clean up
			free3(file_path_raw);
			free3(sectors_size);
			free3(temp_file_info);
			return;
		}    

		if(DSR_callback_id == VAG_READ){
			vagSong.data_size = bytes_to_read;
			if(vagSong.size == 0)
				vagSong.size = temp_file_info->size;
		}

		if(!DsRead(&start_loc, (*sectors_size + SECTOR -1) / SECTOR, *file, DslModeSpeed)){
			printf("cd_read_file_bytes DsRead failed\n");
		}
		if(callbackID == 0){
			while(DsReadSync(NULL));
		}
	} else {
		DSR_callback_id = 0;
		printf("file %s not found\n", file_path_raw);
		exit(1);
	}

	// Clean up
	free3(file_path_raw);
	free3(sectors_size);
	free3(temp_file_info);
}

u_short loadToVRAM(u_long *image){
	RECT rect;
	GsIMAGE tim;
	// skip the TIM ID and version (magic) by adding 0x4 to the pointer
	/* 
 		image+4 if passing u_char*, image+1 if passing u_long*
		u_char == 1 byte, u_long == 4 byte
	*/
	GsGetTimInfo(image+1, &tim);
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
	return GetTPage(tim.pmode, 1, tim.px, tim.py);
}

u_short loadToVRAM2(unsigned char image[]){
	RECT rect;
	GsIMAGE tim;
	GsGetTimInfo ((u_long *)(image+4), &tim);
	rect.x = tim.px;
	rect.y = tim.py;
	rect.w = tim.pw;
	rect.h = tim.ph;
	LoadImage(&rect, tim.pixel);
	rect.x = tim.cx;
	rect.y = tim.cy;
	rect.w = tim.cw;
	rect.h = tim.ch;
	LoadImage(&rect, tim.clut);
	return GetTPage(tim.pmode, 1, tim.px, tim.py);
}

void font_init(){
	int i = 0;
	font = malloc3(sizeof(Font));
	memset(font, 0, sizeof(Font));
	//printf("\n\nsize of Font %d\n\n", sizeof(Font));
	for(i = 0; i < FONT_MAX_CHARS; i++){
		SetDrawMode(&font->dr_mode[i], 0, 0, GetTPage(2, 0, 768, 0), 0);
		SetSprt(&font->sprt[i]);
		font->sprt[i].w = 8; 
		font->sprt[i].h = 8;
		setRGB0(&font->sprt[i], 128, 128, 128);
	}
}

// AUDIO PLAYER
void spu_init() {
	SpuCommonAttr l_c_attr;
	SpuInit();
	SpuInitMalloc(SOUND_MALLOC_MAX, (char*)(SPU_MALLOC_RECSIZ * (SOUND_MALLOC_MAX + 1)));
	l_c_attr.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);
	l_c_attr.mvol.left  = 0x3fff;
	l_c_attr.mvol.right = 0x3fff;
	SpuSetCommonAttr (&l_c_attr);
	SpuSetTransferMode(SpuTransByDMA);
}

void spu_set_voice_attr(u_long voice_bit, unsigned long addr){
	SpuVoiceAttr attr;
	attr.mask =
		(
		 SPU_VOICE_VOLL |
		 SPU_VOICE_VOLR |
		 SPU_VOICE_PITCH |
		 SPU_VOICE_WDSA |
		 SPU_VOICE_ADSR_AMODE |
		 SPU_VOICE_ADSR_SMODE |
		 SPU_VOICE_ADSR_RMODE |
		 SPU_VOICE_ADSR_AR |
		 SPU_VOICE_ADSR_DR |
		 SPU_VOICE_ADSR_SR |
		 SPU_VOICE_ADSR_RR |
		 SPU_VOICE_ADSR_SL
		);
	attr.voice = (voice_bit);
	attr.volume.left  = 0x3fff;
	attr.volume.right = 0x3fff;
	attr.pitch        = 0x1000;
	attr.addr         = addr;
	attr.a_mode       = SPU_VOICE_LINEARIncN;
	attr.s_mode       = SPU_VOICE_LINEARIncN;
	attr.r_mode       = SPU_VOICE_LINEARDecN;
	attr.ar           = 0x1f;
	attr.dr           = 0x0;
	attr.sr           = 0x0;
	attr.rr           = 0x0;
	attr.sl           = 0xf;
	SpuSetVoiceAttr(&attr);
}

SpuIRQCallbackProc spu_handler(){
#ifdef DEBUG_VAG
	printf("spu_handler %ld %lu \n", SpuGetIRQ(), SpuGetIRQAddr());
#endif
	SpuSetIRQ(SPU_OFF);

	if(!vagSong.state)
		return 0;

	if(vagSong.size && vagSong.chunk_addr >= vagSong.size){
	//if(vagSong.chunk_addr >= 600000){
		SpuSetKey(SpuOff, SPU_0CH);
		vagSong.chunk_addr = 0;
		vagSong.block = 0;
		vagSong.state = 2;
		vagSong.read_chunk = true;
		return 0;
	}

	// play again from begin (data is changed, so it will starts to play the next block
	if(vagSong.block == 3){
		SpuSetKey(SpuOn, SPU_0CH); 
	}

	vagSong.read_chunk = true;
	return 0;
}

SpuTransferCallbackProc spu_transfer_callback(){
	//printf("spu transfer callback \n");
#ifdef DEBUG_VAG
	printf("spu transfer callback - vagSong.block %d \n", vagSong.block);
#endif
	DSR_callback_id = 0;

	if(!vagSong.state)
		return 0;

	// start play the song from begin on first transfer 0-300k
	if(vagSong.block == 0)
		SpuSetKey(SPU_ON, SPU_0CH);

	vagSong.block++;
	if(vagSong.block > 3)
		vagSong.block = 1;

	SpuSetIRQAddr(vagSong.spu_addr + (vagSong.block_size * vagSong.block));
	SpuSetIRQ(SPU_ON);
	return 0;
}

// always call after any sfx_load
void vag_song_play(u_char* vagName){
	if(vagSong.name != NULL)
		vag_song_free(&vagSong);
	memset(&vagSong, 0, sizeof(VagSong));
	vagSong.name = malloc3(strlen(vagName));
	strcpy(vagSong.name, vagName);
	vagSong.block_size = VAG_BLOCK_SIZE;
	vagSong.chunk_addr = SPU_BLOCKS_SIZE;
	vagSong.block = 0;
	vagSong.state = 1;
	vagSong.spu_addr = SpuMalloc(SPU_BLOCKS_SIZE);
	//printf("vagSong.spu_addr %ld\n", vagSong.spu_addr);
	
	// load atleast a 240k+ vag file
	cd_read_file_bytes(vagName, (void*)&vagSong.data, 0, SPU_BLOCKS_SIZE, 0);
	SpuSetTransferCallback((SpuTransferCallbackProc)spu_transfer_callback);
	SpuSetIRQCallback((SpuIRQCallbackProc)spu_handler);

	SpuSetTransferStartAddr(vagSong.spu_addr);
	SpuWrite((u_char *)vagSong.data, SPU_BLOCKS_SIZE);
	spu_set_voice_attr(SPU_0CH, vagSong.spu_addr);
}

void vag_song_free(VagSong *vagSong) {
	EnterCriticalSection();
#ifdef DEBUG_VAG
	printf("vag_free\n");
#endif
	SpuSetKey(SpuOff, SPU_0CH);
	SpuSetIRQ(SPU_OFF);
	SpuSetTransferCallback(NULL);
	SpuSetIRQCallback(NULL);
	SpuFree(vagSong->spu_addr);
	free3(vagSong->name);
	vagSong->name = NULL;
	if(vagSong->data != NULL){
		free3((void*)vagSong->data);
		vagSong->data = NULL;
	}
	vagSong->state = 0;
	vagSong->block_size = 0;
	vagSong->chunk_addr = 0;
	vagSong->block = 0;
	vagSong->read_chunk = false;
	DSR_callback_id = 0;
	ExitCriticalSection();
}

unsigned long sfx_load(u_char *name, u_long voice_bit){
	u_long *buffer;
	unsigned long spu_addr;
	u_long vag_size = cd_read_file(name, &buffer);
	spu_addr = SpuMalloc(vag_size);
	//printf("spu_addr %ld\n", spu_addr);
	SpuSetTransferStartAddr(spu_addr);
	SpuWrite((u_char *)buffer, vag_size);
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
	//while(!SpuIsTransferCompleted(SPU_TRANSFER_PEEK)){
		//printf("tranferring...\n");
	//};
	free3(buffer);
	spu_set_voice_attr(voice_bit, spu_addr);
	return spu_addr;
}

void sfx_play(u_long voice_bit) {
	SpuSetKey(SPU_ON, voice_bit);
}

void sfx_pause(u_long voice_bit) {
	SpuSetKey(SPU_OFF, voice_bit);
}

void sfx_free(unsigned long spu_address) {
	SpuFree(spu_address);
}

void drawSprite3D(Sprite *sprite, long _otz){
	long otz;
	sprite_animation(sprite);
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
	psGte(sprite->pos, sprite->rot);
	if(sprite->tpage != 0){
		sprite->poly.ft4.tpage = sprite->tpage;
		/*RotTransPers(&sprite->vector[0], (long *)&sprite->ft4.x0, 0, 0);
		RotTransPers(&sprite->vector[1], (long *)&sprite->ft4.x1, 0, 0);
		RotTransPers(&sprite->vector[2], (long *)&sprite->ft4.x2, 0, 0);
		otz = RotTransPers(&sprite->vector[3], (long *)&sprite->ft4.x3, 0, 0);*/
		otz = RotTransPers4(
			&sprite->vector[0], &sprite->vector[1],
			&sprite->vector[2], &sprite->vector[3],
			(long *)&sprite->poly.ft4.x0,
			(long *)&sprite->poly.ft4.x1,
			(long *)&sprite->poly.ft4.x2,
			(long *)&sprite->poly.ft4.x3, 0,0
		);
		if(_otz != 0)
			otz = _otz;
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, &sprite->poly.ft4);
	}
	else {
		/*RotTransPers(&sprite->vector[0], (long *)&sprite->poly.f4.x0, 0, 0);
		RotTransPers(&sprite->vector[1], (long *)&sprite->poly.f4.x1, 0, 0);
		RotTransPers(&sprite->vector[2], (long *)&sprite->poly.f4.x2, 0, 0);
		otz = RotTransPers(&sprite->vector[3], (long *)&sprite->poly.f4.x3, 0, 0);*/
		otz = RotTransPers4(
			&sprite->vector[0], &sprite->vector[1],
			&sprite->vector[2], &sprite->vector[3],
			(long *)&sprite->poly.f4.x0,
			(long *)&sprite->poly.f4.x1,
			(long *)&sprite->poly.f4.x2,
			(long *)&sprite->poly.f4.x3, 0,0
		);
		if(_otz != 0)
			otz = _otz;
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, &sprite->poly.f4);
	}
}

static void moveSprite(Sprite *sprite, long x, long y){
	if(sprite->tpage != 0){
		sprite->poly.ft4.x0 = x;
		sprite->poly.ft4.y0 = y;
		sprite->poly.ft4.x1 = x + sprite->w;
		sprite->poly.ft4.y1 = y;
		sprite->poly.ft4.x2 = x;
		sprite->poly.ft4.y2 = y + sprite->h;
		sprite->poly.ft4.x3 = x + sprite->w;
		sprite->poly.ft4.y3 = y + sprite->h;
	}
	else {	
		sprite->poly.f4.x0 = x;
		sprite->poly.f4.y0 = y;
		sprite->poly.f4.x1 = x + sprite->w;
		sprite->poly.f4.y1 = y;
		sprite->poly.f4.x2 = x;
		sprite->poly.f4.y2 = y + sprite->h;
		sprite->poly.f4.x3 = x + sprite->w;
		sprite->poly.f4.y3 = y + sprite->h;
	}
}

void drawSprite(Sprite *sprite, long _otz){
	long otz = otIndex;
	sprite_animation(sprite);
	moveSprite(sprite, sprite->pos.vx, sprite->pos.vy);
	if(_otz != 0)
		otz = _otz;
	if(otIndex > 0 && otIndex < OTSIZE){
		if(sprite->tpage != 0) {
			sprite->poly.ft4.tpage = sprite->tpage;
			AddPrim(ot + otz, &sprite->poly.ft4);
		}
		else
			AddPrim(ot + otz, &sprite->poly.f4);
	}
	if(_otz == 0)
		otIndex++;
}

void drawSprite_g4(Sprite *sprite, long _otz){
	long otz = otIndex;
	sprite->poly.g4.x0 = sprite->pos.vx;
	sprite->poly.g4.y0 = sprite->pos.vy;
	sprite->poly.g4.x1 = sprite->pos.vx + sprite->w;
	sprite->poly.g4.y1 = sprite->pos.vy;
	sprite->poly.g4.x2 = sprite->pos.vx;
	sprite->poly.g4.y2 = sprite->pos.vy + sprite->h;
	sprite->poly.g4.x3 = sprite->pos.vx + sprite->w;
	sprite->poly.g4.y3 = sprite->pos.vy + sprite->h;
	if(_otz != 0)
		otz = _otz;
	if(otIndex > 0 && otIndex < OTSIZE){
		AddPrim(ot + otz, &sprite->poly.g4);
	}
	if(_otz == 0)
		otIndex++;
}

void drawSprt(DR_MODE *dr_mode, SPRT *sprt, long _otz){
	if(_otz != 0 && _otz < OTSIZE){
		AddPrim(ot + _otz, sprt);
		AddPrim(ot + _otz, dr_mode);
	}
	else if(otIndex > 0 && otIndex < OTSIZE){
		AddPrim(ot + otIndex, sprt);
		AddPrim(ot + otIndex, dr_mode);
		otIndex++;
	}
}

void drawFont(char *text, int xx, int yy, u_char autoReturn){
	char c;
	int cursor = 0;
	int i = 0;
	int line = 0;
	u_short textLen = strlen(text);
	
	while(i < textLen && font->index < FONT_MAX_CHARS){
		short row, x, y;
		c = *text;
	
		// if the character (c) is a word
 		// find the end of the word to check if the word fits the balloon box
		if(autoReturn == 1 && c != ' ' && c != '\n')
		{
			char wordEnd = 0;
			int count = 1;
			while(!wordEnd){
				if(*(text+count) == ' ' || *(text+count) == '\n' || *(text+count) == '\0')
					wordEnd = 1;
				else
					count++;
				// if the word doesn't fit the balloon box, write the word at new line
				if(wordEnd && cursor+(count-1) >= BALLOON_MAX_LINE_CHARS){
					cursor = 0;
					line++;
				}
			}
		}

		/*if(autoReturn == 1 && cursor == 26 && cursor % 26 == 0){
			cursor = 0;	
			line++;
		}*/

		if(c == '\n'){
			cursor = 0;	
			line++;
			text++;
		}
		else {
			row = (c - 32) / 8;
			x = 192 + (font->sprt[font->index].w * (c - (32 + (8 * row))));
			y = (font->sprt[font->index].h * row);

			font->sprt[font->index].u0 = x;
			font->sprt[font->index].v0 = y; 

			setXY0(&font->sprt[font->index], xx+(7*(cursor++)), yy+(16*line));
		
			drawSprt(&font->dr_mode[font->index], &font->sprt[font->index], 1);
			text++;
			font->index++;
		}
		i++;
	}
}

void drawMesh(Mesh *mesh, long _otz)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_FT4 *ft4 = mesh->ft4;
	POLY_F4 *f4 = mesh->f4;
	POLY_FT3 *ft3 = mesh->ft3;
	POLY_F3 *f3 = mesh->f3;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	int indices = 3;
	long otz = 0;
	size_t n = 0;

	if(mesh == NULL) return;

	if(v == NULL || i == NULL)return;

	psGte(mesh->pos, mesh->rot);
	
	if(mesh->type == QUADS)
		indices = 4;
	for (n = 0; n < mesh->indicesLength*indices; n += indices) {
		if(mesh->tpage != 0){
			switch(mesh->type){
				case QUADS:
				if(ft4 == NULL) return;
				ft4->tpage = mesh->tpage;
				otz = RotAverage4(&v[i[n]],
						&v[i[n + 1]],
						&v[i[n + 2]],
						&v[i[n + 3]],
						(long *)&ft4->x0, (long *)&ft4->x1,
						(long *)&ft4->x3, (long *)&ft4->x2,
						0, 0);
				if(_otz != 0)
					otz = _otz;
				if(otz > 0 && otz < OTSIZE)
					AddPrim(ot+otz, ft4);
				ft4++;
				break;
				case TRIANGLES:
				if(ft3 == NULL) return;
				ft3->tpage = mesh->tpage;
				otz = RotAverage3(&v[i[n]],
						&v[i[n + 1]],
						&v[i[n + 2]],
						(long *)&ft3->x0, 
						(long *)&ft3->x2,
						(long *)&ft3->x1,
						0, 0);
				if(_otz != 0)
					otz = _otz;
				if(otz > 0 && otz < OTSIZE)
					AddPrim(ot+otz, ft3);
				ft3++;
				break;
			}
		}
		else {
			switch(mesh->type){
				case QUADS:
				if(f4 == NULL) return;
				otz = RotAverage4(&v[i[n]],
						&v[i[n + 1]],
						&v[i[n + 2]],
						&v[i[n + 3]],
						(long *)&f4->x0, (long *)&f4->x1,
						(long *)&f4->x3, (long *)&f4->x2,
						0, 0);
				if(_otz != 0)
					otz = _otz;
				if(otz > 0 && otz < OTSIZE)
					AddPrim(ot+otz, f4);
				f4++;
				break;
				case TRIANGLES:
				if(f3 == NULL) return;
				otz = RotAverage3(&v[i[n]],
						&v[i[n + 1]],
						&v[i[n + 2]],
						(long *)&f3->x0, (long *)&f3->x1,
						(long *)&f3->x2,
						0, 0);
				if(_otz != 0)
					otz = _otz;
				if(otz > 0 && otz < OTSIZE)
					AddPrim(ot+otz, f3);
				f3++;
				break;
			}
		}	
	}
}

void drawBBox(BBox *bb){
	long otz = 0;
	int i = 0;
	psGte(bb->pos, bb->rot);
	for (i = 0; i < 1; i++) {
		otz = RotAverage4(
			&bb->vertices[i], &bb->vertices[i+1], 
			&bb->vertices[i+2], &bb->vertices[i+3],
			(long *)&bb->poly_f4[i].x0, (long *)&bb->poly_f4[i].x1,
			(long *)&bb->poly_f4[i].x3, (long *)&bb->poly_f4[i].x2,
			0, 0
		);
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, &bb->poly_f4[i]);
	}
}

void node_push(Node **node, void *data, size_t data_size, GfxType type) {
	Node *newNode = malloc3(sizeof(Node));
	if (newNode == NULL) {
		printf("error on Node malloc3\n");
		return;
	}

	if(data_size){
		newNode->data = malloc3(data_size);
		if(newNode->data == NULL){
			printf("newNode->data malloc3 failed");
			exit(1);
		}
		memcpy(newNode->data, data, data_size);
	}
	else
		newNode->data = data;

	newNode->type = type;
	newNode->next = NULL;

	if (*node == NULL) {
		*node = newNode;
	} 
	else 
	{
		Node *current = *node;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = newNode;
	}
}

void node_remove(Node **node, void *data, u_char freeData) {
	Node *current = *node;
	Node *prev = NULL;
	while (current != NULL) {
		if (current->data == data) {
			// if first node
			if (prev == NULL) {
				*node = current->next;
			} else {
				prev->next = current->next;
			}
			if(freeData)
				free3(current->data);
			free3(current);
			return;
		}
		prev = current;
		current = current->next;
	}
}

void node_free(Node **node) {
	Node *current = *node;
	Node *nextNode;
	while (current != NULL) {
		nextNode = current->next;
		free3(current);
		current = nextNode;
	}
	*node = NULL;
}

void scene_add(void *data, GfxType type) {
	node_push(&scene.node, data, 0, type);
}

void scene_remove(void *data) {
	node_remove(&scene.node, data, 0);
}

void scene_free() {
	node_free(&scene.node);
}

void scene_draw(){
	Node *current = scene.node;
	Balloon *b;
	while(current != NULL) {
		switch(current->type) {
			case GFX_SPRITE:
				drawSprite((Sprite*)current->data, 0);
				break;
			case GFX_MESH:
				drawMesh((Mesh*)current->data, 0);
				break;
			case GFX_MODEL:
				model_draw((Model*)current->data, 0, drawMesh);
				break;
			case GFX_SPRITE3D:
				drawSprite3D((Sprite*)current->data, 0);
				break;
			case GFX_WINDOW:
				window_draw((Window*)current->data);
				break;
			case GFX_BALLOON:
				b = (Balloon*)current->data;
				drawFont(b->text, balloon.window.background.pos.vx + 5, balloon.window.background.pos.vy + 5, 1);
				window_draw(&b->window);
				break;
			default:
				break;
		}
		current = current->next;
	}
}

void scene_load(void(*callback)){
	scene.status = SCENE_LOAD;
	scene.load_callback = callback;
}

void scene_update_billboards(){
	scene.update_billboards = 1;
}

void enableScreen(){
	SetDispMask(1);
}

void disableScreen(){
	ResetGraph(0);
}

u_char random(int p) {
	//return rand() % (max+1);
	//if (rand() < (RAND_MAX + 1) * 0.01) // 1%
	if (rand() < (RAND_MAX + 1) * p){
		return 1;
	}
	return 0;
}

int randomRange(int min, int max){
	return rand() % (max - min +1) + min;
}

int degrees_to_rot(int degrees) {
	return (degrees * 2048) / 180;
}

VECTOR interpolate(VECTOR A, VECTOR B, float t) {
	// p(t) = (1−t) * a + t*b
	VECTOR result;
	// t value is between 0 and 1
	if(t < 0) t = 0;
	if(t > 1) t = 1;
	result.vx = (1 - t) * A.vx + t * B.vx;
	result.vy = (1 - t) * A.vy + t * B.vy;
	result.vz = (1 - t) * A.vz + t * B.vz;
	return result;
}

SVECTOR interpolateRot(SVECTOR A, SVECTOR B, float t) {
	// p(t) = (1−t) * a + t*b
	SVECTOR result;
	// t value is between 0 and 1
	if(t < 0) t = 0;
	if(t > 1) t = 1;
	result.vx = (1 - t) * A.vx + t * B.vx;
	result.vy = (1 - t) * A.vy + t * B.vy;
	result.vz = (1 - t) * A.vz + t * B.vz;
	return result;
}

Camera camera_interpolate(VECTOR startPos, SVECTOR startRot, VECTOR targetPos, SVECTOR targetRot, float t){
	Camera cam;
	cam.mtx = camera.mtx;
	cam.tmp = camera.tmp;
	cam.pos = interpolate(startPos, targetPos, t);
	cam.rot = interpolateRot(startRot, targetRot, t);
	return cam;
}

void window_init(Window *win, long x, long y, int w, int h, u_short tpage_ui, Color color[4]){
	/*Color color[4] = { 
		{0, 0, 200}, // top left
		{0, 0, 60}, // top right
		{0, 0, 80}, // bottom left
		{0, 0, 40} // bottom right
	};*/
	int i = 0;
	u_char size = 3; // border size
	memset(win, 0, sizeof(Window));
	sprite_init_g4(&win->background, w, h, color);
	//sprite_set_rgb(&win->background, 0, 0, 31, 0);
	win->background.pos.vx = x;
	win->background.pos.vy = y;

	// init window's borders
	// TOP EDGES 
	for(i = 0; i <= 1; i++){
		Sprite *b = &win->borders[i];
		sprite_init(b, size, size, tpage_ui);
		b->pos.vx = x-1;
		b->pos.vy = y - size/2;
		// right 
		if(i == 1){
			b->mirror_h = 1;
			b->pos.vx = x + w - size+1;
			b->pos.vy = y - size/2;
		}
		sprite_set_uv(b, size, 0, size, size);

	}
	// LEFT && RIGHT
	for(i = 2; i <= 3; i++){
		Sprite *b = &win->borders[i];
		sprite_init(b, size, h-size, tpage_ui);
		b->pos.vx = x-1;
		b->pos.vy = y + size -1;
		// right 
		if(i == 3){
			b->mirror_h = 1;
			b->pos.vx = x + w - size+1;
			b->pos.vy = y + size -1;
		}
		sprite_set_uv(b, 0, 0, size, size);
	}
	// BOTTOM EDGES 
	for(i = 4; i <= 5; i++){
		Sprite *b = &win->borders[i];
		sprite_init(b, size, size, tpage_ui);
		b->pos.vx = x-1;
		b->pos.vy = y + h - size/2;
		// right 
		if(i == 5){
			b->mirror_h = 1;
			b->pos.vx = x + w - size+1;
			b->pos.vy = y + h - size/2;
		}
		b->mirror_v = 1;
		sprite_set_uv(b, size, 0, size, size);
	}
	// TOP && BOTTOM 
	// top border's width = window width - both borders' size + 2  
	// (left corner -1, right corner +1, so we add 2)
	for(i = 6; i <= 7; i++){
		Sprite *b = &win->borders[i];
		sprite_init(b, w-(size*2)+2, size, tpage_ui);
		b->pos.vx = x + size-1;
		b->pos.vy = y - size/2;
		// bottom
		if(i == 7){
			b->mirror_v = 1;
			b->pos.vx = x + size-1;
			b->pos.vy = y + h - size/2;
		}
		sprite_set_uv(b, size*2, 0, size, size);
	}

	sprite_init(&win->selector.sprite, 20, 22, tpage_ui);
	sprite_set_uv(&win->selector.sprite, 0, 174, 30, 22);
}

void window_draw(Window *win){
	int i = 0;
	if(win->display)
		win->display(win);
	for(i = 0; i < 8; i++)
		drawSprite(&win->borders[i], 0);
	drawSprite_g4(&win->background, 0);
}

VECTOR window_get_pos(Window *win){
	VECTOR pos;
	long x = win->background.pos.vx + win->borders[0].w;
	long y = win->background.pos.vy + win->borders[0].h;
	pos.vx = x;
	pos.vy = y;
	return pos;
}

void window_set_display(Window *win, void (*func)(Window *win)){
	VECTOR pos = window_get_pos(win);
	win->selector.sprite.pos.vx = pos.vx;
	win->selector.sprite.pos.vy = pos.vy;
	win->selector.index = 0;
	win->display = func;
}

int pad_press(u_long key){
	if(pad & key)
		return 1;
	return 0;
}

int pad_press_once(u_long key){
	if(pad & key && (opad & key) == 0)
		return 1;
	return 0;
}

int pad_press_delay(u_long key){
	static int time;
	static float speed;
	if(pad & key && (opad & key) == 0){
		time = 0;
		speed = 1;
		return 1;
	}
	else if(pad & key){
		time += floor(speed);
		if(speed < 5)
			speed += 0.1f;
	}
	if(time >= 20){
		time = 0;
		return 1;
	}
	return 0;
}
