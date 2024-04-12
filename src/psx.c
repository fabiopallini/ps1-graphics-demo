#include "psx.h"
#include "utils.h"

#define SOUND_MALLOC_MAX 3 
#define OTSIZE 1024
#define BILLBOARDS 0

DISPENV	dispenv[2];
DRAWENV	drawenv[2];
int dispid = 0;
u_long ot[OTSIZE];
u_short otIndex;

#define SUB_STACK 0x80180000 /* stack for sub-thread. update appropriately. */
unsigned long sub_th,gp;
static volatile unsigned long count1,count2; 
struct ToT *sysToT = (struct ToT *) 0x100 ; /* Table of Tabbles  */
struct TCBH *tcbh ; /* task status queue address */
struct TCB *master_thp,*sub_thp; /* start address of thread context */

static void billboard(Sprite *sprite) {
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
	//sprite->angX -= camera.rot.vx;
	//sprite->angZ = 0.0;
}

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
		SpriteNode *current = scene.spriteNode;
		while (current != NULL) {
			//printf("current-> %ld \n", current->data->pos.vx);
			if(BILLBOARDS == 1)
				billboard(current->data);
			else
				current->data->rot.vy = camera.rot.vy * -1;
			current = current->next;
		}
		/* A Vsync interrupt is received somewhere in this while loop, and control is taken away.
	        Control resumes from there at the next ChangeTh(). */
		count2 ++;
	}
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

void psInit()
{
	SetConf(16,4,0x80200000);
	tcbh = (struct TCBH *) sysToT[1].head;
	master_thp = tcbh->entry;
	gp = GetGp();
	EnterCriticalSection();
	sub_th = OpenTh(sub_func, SUB_STACK, gp);
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

	SetDefDrawEnv(&drawenv[0], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDrawEnv(&drawenv[1], 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[0], 0, SCREEN_HEIGHT,SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDefDispEnv(&dispenv[1], 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

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
	SetDispMask(1);

	drawenv[0].isbg = 1;
	setRGB0(&drawenv[0], 0,0,0);
	drawenv[1].isbg = 1;
	setRGB0(&drawenv[1], 0,0,0);

 	// load the font from the BIOS into the framebuffer
	FntLoad(960, 256);
	// screen X,Y | max text length X,Y | autmatic background clear 0,1 | max characters
	font_id[0] = FntOpen(5, 20, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 512);	
	SetDumpFnt(font_id[0]);
	font_id[1] = FntOpen(15, 170, SCREEN_WIDTH, SCREEN_HEIGHT-170, 0, 512);	
	SetDumpFnt(font_id[1]);

	//init stack 16KB heap 2 megabyte
	InitHeap3((void*)0x800F8000, 0x00200000);
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
}

void psDisplay(){
	opad = pad;
	//FntFlush(-1);
	FntFlush(font_id[0]);
	FntFlush(font_id[1]);
	DrawSync(0);
	ChangeTh(sub_th);
	//VSync(0);
	PutDispEnv(&dispenv[dispid]);
	PutDrawEnv(&drawenv[dispid]);
	//DrawOTag(ot);
	DrawOTag(ot+OTSIZE-1);
	//FntPrint(font_id[1], "free time = %d\n", count2 - count1);
	count1 = count2;
	otIndex = 0;
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
SpuCommonAttr l_c_attr;
SpuVoiceAttr  g_s_attr;
unsigned long l_vag1_spu_addr;

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

void cd_read_file(unsigned char* file_path, u_long** file) {

	u_char* file_path_raw;
	int* sectors_size;
	DslFILE* temp_file_info;
	sectors_size = malloc3(sizeof(int));
	temp_file_info = malloc3(sizeof(DslFILE));

	// Exit if libDs isn't initialized
	if(!didInitDs) {
		printf("LIBDS not initialized, run cdOpen() first\n");	
		return;
	}

	// Get raw file path
	file_path_raw = malloc3(4 + strlen(file_path));
	strcpy(file_path_raw, "\\");
	strcat(file_path_raw, file_path);
	strcat(file_path_raw, ";1");
	printf("Loading file from CD: %s\n", file_path_raw);

	// Search for file on disc
	DsSearchFile(temp_file_info, file_path_raw);

	// Read the file if it was found
	if(temp_file_info->size > 0) {
		printf("...file found\n");
		printf("...file size: %lu\n", temp_file_info->size);
		*sectors_size = temp_file_info->size + (SECTOR % temp_file_info->size);
		printf("...file buffer size needed: %d\n", *sectors_size);
		printf("...sectors needed: %d\n", (*sectors_size + SECTOR - 1) / SECTOR);
		*file = malloc3(*sectors_size + SECTOR);
		
		DsRead(&temp_file_info->pos, (*sectors_size + SECTOR - 1) / SECTOR, *file, DslModeSpeed);
		while(DsReadSync(NULL));
		printf("...file loaded!\n");
	} else {
		printf("...file not found");
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

// AUDIO PLAYER
void audio_init() {
	SpuInit();
	SpuInitMalloc (SOUND_MALLOC_MAX, (char*)(SPU_MALLOC_RECSIZ * (SOUND_MALLOC_MAX + 1)));
	l_c_attr.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);
	l_c_attr.mvol.left  = 0x3fff; // set master left volume
	l_c_attr.mvol.right = 0x3fff; // set master right volume
	SpuSetCommonAttr (&l_c_attr);
}

void audio_vag_to_spu(u_char* sound_data, u_long sound_size, int voice_channel) {
	SpuSetTransferMode (SpuTransByDMA); // set transfer mode to DMA
	l_vag1_spu_addr = SpuMalloc(sound_size); // allocate SPU memory for sound 1
	SpuSetTransferStartAddr(l_vag1_spu_addr); // set transfer starting address to malloced area
	SpuWrite(sound_data, sound_size); // perform actual transfer
	SpuIsTransferCompleted (SPU_TRANSFER_WAIT); // wait for DMA to complete
	g_s_attr.mask =
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

	g_s_attr.voice = (voice_channel);

	g_s_attr.volume.left  = 0x1fff;
	g_s_attr.volume.right = 0x1fff;

	g_s_attr.pitch        = 0x1000;
	g_s_attr.addr         = l_vag1_spu_addr;
	g_s_attr.a_mode       = SPU_VOICE_LINEARIncN;
	g_s_attr.s_mode       = SPU_VOICE_LINEARIncN;
	g_s_attr.r_mode       = SPU_VOICE_LINEARDecN;
	g_s_attr.ar           = 0x0;
	g_s_attr.dr           = 0x0;
	g_s_attr.sr           = 0x0;
	g_s_attr.rr           = 0x0;
	g_s_attr.sl           = 0xf;

	SpuSetVoiceAttr (&g_s_attr);
}

void audio_play(int voice_channel) {
	SpuSetKey(SpuOn, voice_channel);
}

void audio_free(unsigned long spu_address) {
	SpuFree(spu_address);
}

void drawSprite(Sprite *sprite){
	long otz;
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
	psGte(sprite->pos, sprite->rot);
	sprite->poly.tpage = sprite->tpage;
	RotTransPers(&sprite->vector[0], (long *)&sprite->poly.x0, 0, 0);
	RotTransPers(&sprite->vector[1], (long *)&sprite->poly.x1, 0, 0);
	RotTransPers(&sprite->vector[2], (long *)&sprite->poly.x2, 0, 0);
	otz = RotTransPers(&sprite->vector[3], (long *)&sprite->poly.x3, 0, 0);
	if(otz > 0 && otz < OTSIZE)
		AddPrim(ot+otz, &sprite->poly);
}

void drawSprite_ot(Sprite *sprite, long otz){
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
	psGte(sprite->pos, sprite->rot);
	sprite->poly.tpage = sprite->tpage;
	RotTransPers(&sprite->vector[0], (long *)&sprite->poly.x0, 0, 0);
	RotTransPers(&sprite->vector[1], (long *)&sprite->poly.x1, 0, 0);
	RotTransPers(&sprite->vector[2], (long *)&sprite->poly.x2, 0, 0);
	RotTransPers(&sprite->vector[3], (long *)&sprite->poly.x3, 0, 0);
	if(otz > 0 && otz < OTSIZE)
		AddPrim(ot+otz, &sprite->poly);
}

static void moveSprite(Sprite *sprite, long x, long y){
	sprite->poly.x0 = x;
	sprite->poly.y0 = y;
	sprite->poly.x1 = x + sprite->w;
	sprite->poly.y1 = y;
	sprite->poly.x2 = x;
	sprite->poly.y2 = y + sprite->h;
	sprite->poly.x3 = x + sprite->w;
	sprite->poly.y3 = y + sprite->h;
}

void drawSprite_2d(Sprite *sprite){
	moveSprite(sprite, sprite->pos.vx, sprite->pos.vy);
	sprite->poly.tpage = sprite->tpage;
	if(otIndex < OTSIZE)
		AddPrim(ot + otIndex, &sprite->poly);
	otIndex++;
}

void drawSprite_2d_ot(Sprite *sprite, long otz){
	moveSprite(sprite, sprite->pos.vx, sprite->pos.vy);
	sprite->poly.tpage = sprite->tpage;
	if(otz < OTSIZE)
		AddPrim(ot + otz, &sprite->poly);
}

void drawSprite_2d_rgb(Sprite *sprite){
	long x = sprite->pos.vx;
	long y = sprite->pos.vy;
	sprite->poly_rgb.x0 = x;
	sprite->poly_rgb.y0 = y;
	sprite->poly_rgb.x1 = x + sprite->w;
	sprite->poly_rgb.y1 = y;
	sprite->poly_rgb.x2 = x;
	sprite->poly_rgb.y2 = y + sprite->h;
	sprite->poly_rgb.x3 = x + sprite->w;
	sprite->poly_rgb.y3 = y + sprite->h;
	if(otIndex < OTSIZE)
		AddPrim(ot + otIndex, &sprite->poly_rgb);
	otIndex++;
}

void drawSprt(DR_MODE *dr_mode, SPRT *sprt){
	if(otIndex < OTSIZE){
		AddPrim(ot + otIndex, sprt);
		AddPrim(ot + otIndex, dr_mode);
	}
	otIndex++;
}

void drawFont(u_char *text, Font *font, int xx, int yy){
	u_char c;
	int cursor = 0;
	int i = 0;
	int line = 0;

	while((c = *text) != '\0' && i < FONT_MAX_CHARS){
		short row, x, y;
		//printf("%c\n", c);
		//printf("%d\n", c);

		if(cursor == 26 && cursor % 26 == 0){
			cursor = 0;	
			line++;
		}

		if(c == '\n'){
			cursor = 0;	
			line++;
			text++;
		}
		else{
			row = (c - 32) / 8;
			x = 192 + (font->sprt[i].w * (c - (32 + (8 * row))));
			y = (font->sprt[i].h * row);

			font->sprt[i].u0 = x;
			font->sprt[i].v0 = y; 

			setXY0(&font->sprt[i], xx+(7*(cursor++)), yy+(16*line));
		
			drawSprt(&font->dr_mode[i], &font->sprt[i]);
			text++;
			i++;
		}
	}
}

void mesh_draw(Mesh *mesh, int clip)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_FT4 *ft4 = mesh->ft4;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	int nclip;
	long otz;
	size_t n;
	psGte(mesh->pos, mesh->rot);

	for (n = 0; n < mesh->indicesLength*4; n += 4, ++ft4) {
		ft4->tpage = mesh->tpage;
		if(clip > 0){
			nclip = RotAverageNclip4(&v[i[n + 0]],
					&v[i[n + 1]],
					&v[i[n + 2]],
					&v[i[n + 3]],
					(long *)&ft4->x0, (long *)&ft4->x1,
					(long *)&ft4->x3, (long *)&ft4->x2,
					0, &otz, 0);
			if (nclip <= 0)
				continue;
		}
		else{
			RotTransPers(&v[i[n + 0]], (long *)&ft4->x0, 0, 0);
			RotTransPers(&v[i[n + 1]], (long *)&ft4->x1, 0, 0);
			RotTransPers(&v[i[n + 2]], (long *)&ft4->x3, 0, 0);
			otz = RotTransPers(&v[i[n + 3]], (long *)&ft4->x2, 0, 0);
		}
		
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, ft4);
	}
}

void mesh_draw_ot(Mesh *mesh, int clip, long otz)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_FT4 *ft4 = mesh->ft4;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	int nclip;
	size_t n;
	psGte(mesh->pos, mesh->rot);

	for (n = 0; n < mesh->indicesLength*4; n += 4, ++ft4) {
		ft4->tpage = mesh->tpage;
		if(clip > 0){
			nclip = RotAverageNclip4(&v[i[n + 0]],
					&v[i[n + 1]],
					&v[i[n + 2]],
					&v[i[n + 3]],
					(long *)&ft4->x0, (long *)&ft4->x1,
					(long *)&ft4->x3, (long *)&ft4->x2,
					0, 0, 0);
			if (nclip <= 0)
				continue;
		}
		else{
			RotTransPers(&v[i[n + 0]], (long *)&ft4->x0, 0, 0);
			RotTransPers(&v[i[n + 1]], (long *)&ft4->x1, 0, 0);
			RotTransPers(&v[i[n + 2]], (long *)&ft4->x3, 0, 0);
			RotTransPers(&v[i[n + 3]], (long *)&ft4->x2, 0, 0);
		}
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, ft4);
	}
}

void drawMesh_rgb(Mesh *mesh, int clip)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_F4 *f4 = mesh->f4;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	int nclip;
	long otz;
	size_t n;
	psGte(mesh->pos, mesh->rot);

	for (n = 0; n < mesh->indicesLength*4; n += 4, ++f4) {
		if(clip > 0){
			nclip = RotAverageNclip4(&v[i[n + 0]],
					&v[i[n + 1]],
					&v[i[n + 2]],
					&v[i[n + 3]],
					(long *)&f4->x0, (long *)&f4->x1,
					(long *)&f4->x3, (long *)&f4->x2,
					0, &otz, 0);
			if (nclip <= 0)
				continue;
		}
		else{
			RotTransPers(&v[i[n + 0]], (long *)&f4->x0, 0, 0);
			RotTransPers(&v[i[n + 1]], (long *)&f4->x1, 0, 0);
			RotTransPers(&v[i[n + 2]], (long *)&f4->x3, 0, 0);
			otz = RotTransPers(&v[i[n + 3]], (long *)&f4->x2, 0, 0);
		}
		
		if(otz > 0 && otz < OTSIZE)
			AddPrim(ot+otz, f4);
	}
}
static SpriteNode *createSprite(Sprite *data) {
	SpriteNode* newNode = malloc3(sizeof(SpriteNode));
	if (newNode == NULL) {
		printf("error on SpriteNode malloc3 \n");
		return NULL; 
	}
	newNode->data = data;
	newNode->next = NULL;
	return newNode;
}

void scene_add_sprite(Sprite *data) {
	SpriteNode *last;
	SpriteNode **head = &scene.spriteNode;
	SpriteNode *newNode = createSprite(data);
	if (*head == NULL) {
		*head = newNode;
		return;
	}
	last = *head;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = newNode;
}

void printSpriteNode(SpriteNode *head) {
	SpriteNode *current = head;
	while (current != NULL) {
		printf("SpriteNode->pos.vx %ld \n", current->data->pos.vx);
		current = current->next;
	}
}

void scene_freeSprites(){
	SpriteNode *current = scene.spriteNode;
	while (current != NULL) {
		SpriteNode *nextNode = current->next;
		free(current);
		current = nextNode;
	}
}

