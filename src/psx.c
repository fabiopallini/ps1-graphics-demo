#include "psx.h"
#include "utils.h"

//#define SUB_STACK 0x80180000 /* stack for sub-thread. update appropriately. */
#define SUB_STACK_SIZE 0x4000 // 8KB
//#define SUB_STACK_SIZE 0x8000 // 16KB

#define SOUND_MALLOC_MAX 3 
#define SPU_BLOCKS_SIZE 240000
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
		if(vag.read_chunk){
#ifdef DEBUG_VAG
			printf("cd_read_file_bytes\n");	
#endif
			vag.read_chunk = false;
			if(vag.data != NULL){
				free3((void*)vag.data);
				vag.data = NULL;
			}
			// play reached the end of the song, restart reading from begin (0-240k)
			if(vag.state == 2){
				cd_read_file_bytes(vag.name, (void*)&vag.data, vag.chunk_addr, 
					vag.chunk_addr + (SPU_BLOCKS_SIZE), VAG_READ);
			}
			else {
				// read next chunk
				cd_read_file_bytes(vag.name, (void*)&vag.data, vag.chunk_addr, 
					vag.chunk_addr + vag.block_size, VAG_READ);
			}
			vag.chunk_addr += vag.data_size;
		}
		if(DSR_callback_id == VAG_TRANSFER){
#ifdef DEBUG_VAG
			printf("dsr_callback_id VAG_TRANSFER\n");	
#endif
			if(!vag.state){
				DSR_callback_id = 0;
				return 0;
			}
			// copy the first 0-300k on song restart (loop)
			if(vag.state == 2){
				vag.state = 1;
				SpuSetTransferStartAddr(vag.spu_addr);
				SpuWrite((u_char *)vag.data, SPU_BLOCKS_SIZE);
			}
			else {
				// copy next chunk in sound memory
				SpuSetTransferStartAddr(vag.spu_addr + ((vag.block-1) * vag.data_size));
				SpuWrite((u_char *)vag.data, vag.data_size);
			}
			DSR_callback_id = VAG_TRANSFERING;
		}
		/* A Vsync interrupt is received somewhere in this while loop, and control is taken away.
	        Control resumes from there at the next ChangeTh(). */
		count2++;
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
	
	scene.status = SCENE_READY;
	DSR_callback_id = 0;
	DsReadCallback((DslCB)cd_read_callback);
	font_init();
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

	if(screenWait < 1)
		screenWait++;
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
SpuCommonAttr l_c_attr;
SpuVoiceAttr g_s_attr;
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
			return;
		}	
		
		DsRead(&temp_file_info->pos, (*sectors_size + SECTOR - 1) / SECTOR, *file, DslModeSpeed);
		while(DsReadSync(NULL));
		//printf("file loaded!\n");
	} else {
		printf("file %s not found\n", file_path_raw);
		exit(1);
	}

	// Clean up
	free3(file_path_raw);
	free3(sectors_size);
	free3(temp_file_info);
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
			vag.data_size = bytes_to_read;
			if(vag.size == 0)
				vag.size = temp_file_info->size;
		}

		DsRead(&start_loc, (*sectors_size + SECTOR -1) / SECTOR, *file, DslModeSpeed);
		if(callbackID == 0)
			while(DsReadSync(NULL));
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
	memset(&font, 0, sizeof(Font));
	for(i = 0; i < FONT_MAX_CHARS; i++){
		SetDrawMode(&font.dr_mode[i], 0, 0, GetTPage(2, 0, 640, 0), 0);
		SetSprt(&font.sprt[i]);
		font.sprt[i].w = 8; 
		font.sprt[i].h = 8;
		setRGB0(&font.sprt[i], 128, 128, 128);
	}
}

// AUDIO PLAYER
void spu_init() {
	SpuInit();
	SpuInitMalloc(SOUND_MALLOC_MAX, (char*)(SPU_MALLOC_RECSIZ * (SOUND_MALLOC_MAX + 1)));
	l_c_attr.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR);
	l_c_attr.mvol.left  = 0x3fff;
	l_c_attr.mvol.right = 0x3fff;
	SpuSetCommonAttr (&l_c_attr);
	SpuSetTransferMode(SpuTransByDMA);
}

void spu_set_voice_attr(int channel, unsigned long addr){
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
	g_s_attr.voice = (SPU_0CH);
	g_s_attr.volume.left  = 0x3fff;
	g_s_attr.volume.right = 0x3fff;
	g_s_attr.pitch        = 0x1000;
	g_s_attr.addr         = addr;
	g_s_attr.a_mode       = SPU_VOICE_LINEARIncN;
	g_s_attr.s_mode       = SPU_VOICE_LINEARIncN;
	g_s_attr.r_mode       = SPU_VOICE_LINEARDecN;
	g_s_attr.ar           = 0x1f;
	g_s_attr.dr           = 0x0;
	g_s_attr.sr           = 0x0;
	g_s_attr.rr           = 0x0;
	g_s_attr.sl           = 0xf;
	SpuSetVoiceAttr(&g_s_attr);
}

SpuIRQCallbackProc spu_handler(){
#ifdef DEBUG_VAG
	printf("spu_handler\n");
#endif
	SpuSetIRQ(SPU_OFF);

	if(!vag.state)
		return 0;

	if(vag.size && vag.chunk_addr >= vag.size){
	//if(vag.chunk_addr >= 600000){
		SpuSetKey(SpuOff, SPU_0CH);
		vag.chunk_addr = 0;
		vag.block = 0;
		vag.state = 2;
		vag.read_chunk = true;
		return 0;
	}

	// play again from begin (data is changed, so it will starts to play the next block
	if(vag.block == 3){
		SpuSetKey(SpuOn, SPU_0CH); 
	}

	vag.read_chunk = true;
	return 0;
}

SpuTransferCallbackProc spu_transfer_callback(){
#ifdef DEBUG_VAG
	printf("spu transfer callback\n");
#endif
	DSR_callback_id = 0;

	if(!vag.state)
		return 0;

	SpuSetIRQ(SPU_ON);

	// start play the song from begin on first transfer 0-300k
	if(vag.block == 0)
		SpuSetKey(SpuOn, SPU_0CH);

	vag.block++;
	if(vag.block > 3)
		vag.block = 1;

	SpuSetIRQAddr(vag.spu_addr + (vag.block_size * vag.block));
	return 0;
}

void vag_load(u_char* vagName, int voice_channel){
	memset(&vag, 0, sizeof(Vag));
	vag.name = malloc3(strlen(vagName));
	strcpy(vag.name, vagName);
	vag.block_size = 80000;
	vag.chunk_addr = SPU_BLOCKS_SIZE;
	vag.block = 0;
	vag.state = 1;
	vag.spu_addr = SpuMalloc(SPU_BLOCKS_SIZE);

	// load atleast a 240k+ vag file
	cd_read_file_bytes(vagName, (void*)&vag.data, 0, SPU_BLOCKS_SIZE, 0);
	SpuSetTransferStartAddr(vag.spu_addr);
	SpuWrite((u_char *)vag.data, SPU_BLOCKS_SIZE);
	spu_set_voice_attr(SPU_0CH, vag.spu_addr);
	SpuSetTransferCallback((SpuTransferCallbackProc)spu_transfer_callback);
	SpuSetIRQCallback((SpuIRQCallbackProc)spu_handler);
}

void vag_free(Vag *vag) {
	EnterCriticalSection();
#ifdef DEBUG_VAG
	printf("vag_free\n");
#endif
	SpuSetKey(SpuOff, SPU_0CH);
	SpuSetIRQ(SPU_OFF);
	SpuSetTransferCallback(NULL);
	SpuSetIRQCallback(NULL);
	SpuFree(vag->spu_addr);
	free3(vag->name);
	vag->name = NULL;
	if(vag->data != NULL){
		free3((void*)vag->data);
		vag->data = NULL;
	}
	vag->state = 0;
	vag->block_size = 0;
	vag->chunk_addr = 0;
	vag->block = 0;
	vag->read_chunk = false;
	DSR_callback_id = 0;
	ExitCriticalSection();
}

void spu_load(u_long *vag_data, u_long vag_size, int voice_channel){
	SpuSetTransferMode(SpuTransByDMA);
	l_vag1_spu_addr = SpuMalloc(vag_size);
	SpuSetTransferStartAddr(l_vag1_spu_addr);
	SpuWrite((u_char *)vag_data, vag_size);
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT);
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
	g_s_attr.ar           = 0x1F;
	g_s_attr.dr           = 0x0;
	g_s_attr.sr           = 0x0;
	g_s_attr.rr           = 0x0;
	g_s_attr.sl           = 0xf;
	SpuSetVoiceAttr(&g_s_attr);
}

void spu_play(int voice_channel) {
	SpuSetKey(SpuOn, voice_channel);
}

void spu_pause(int voice_channel) {
	SpuSetKey(SpuOff, voice_channel);
}

void spu_free(unsigned long spu_address) {
	SpuFree(spu_address);
}

void drawSprite(Sprite *sprite, long _otz){
	long otz;
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
	psGte(sprite->pos, sprite->rot);
	if(sprite->tpage != 0){
		sprite->ft4.tpage = sprite->tpage;
		/*RotTransPers(&sprite->vector[0], (long *)&sprite->ft4.x0, 0, 0);
		RotTransPers(&sprite->vector[1], (long *)&sprite->ft4.x1, 0, 0);
		RotTransPers(&sprite->vector[2], (long *)&sprite->ft4.x2, 0, 0);
		otz = RotTransPers(&sprite->vector[3], (long *)&sprite->ft4.x3, 0, 0);*/
		otz = RotTransPers4(
			&sprite->vector[0], &sprite->vector[1],
			&sprite->vector[2], &sprite->vector[3],
			(long *)&sprite->ft4.x0,
			(long *)&sprite->ft4.x1,
			(long *)&sprite->ft4.x2,
			(long *)&sprite->ft4.x3, 0,0
		);
		if(_otz != 0)
			otz = _otz;
		AddPrim(ot+otz, &sprite->ft4);
	}
	else {
		/*RotTransPers(&sprite->vector[0], (long *)&sprite->f4.x0, 0, 0);
		RotTransPers(&sprite->vector[1], (long *)&sprite->f4.x1, 0, 0);
		RotTransPers(&sprite->vector[2], (long *)&sprite->f4.x2, 0, 0);
		otz = RotTransPers(&sprite->vector[3], (long *)&sprite->f4.x3, 0, 0);*/
		otz = RotTransPers4(
			&sprite->vector[0], &sprite->vector[1],
			&sprite->vector[2], &sprite->vector[3],
			(long *)&sprite->f4.x0,
			(long *)&sprite->f4.x1,
			(long *)&sprite->f4.x2,
			(long *)&sprite->f4.x3, 0,0
		);
		if(_otz != 0)
			otz = _otz;
		AddPrim(ot+otz, &sprite->f4);
	}
}

static void moveSprite(Sprite *sprite, long x, long y){
	if(sprite->tpage != 0){
		sprite->ft4.x0 = x;
		sprite->ft4.y0 = y;
		sprite->ft4.x1 = x + sprite->w;
		sprite->ft4.y1 = y;
		sprite->ft4.x2 = x;
		sprite->ft4.y2 = y + sprite->h;
		sprite->ft4.x3 = x + sprite->w;
		sprite->ft4.y3 = y + sprite->h;
	}
	else {	
		sprite->f4.x0 = x;
		sprite->f4.y0 = y;
		sprite->f4.x1 = x + sprite->w;
		sprite->f4.y1 = y;
		sprite->f4.x2 = x;
		sprite->f4.y2 = y + sprite->h;
		sprite->f4.x3 = x + sprite->w;
		sprite->f4.y3 = y + sprite->h;
	}
}

void drawSprite_2d(Sprite *sprite, long _otz){
	long otz = otIndex;
	moveSprite(sprite, sprite->pos.vx, sprite->pos.vy);
	if(_otz != 0)
		otz = _otz;
	if(otIndex > 0 && otIndex < OTSIZE){
		if(sprite->tpage != 0) {
			sprite->ft4.tpage = sprite->tpage;
			AddPrim(ot + otz, &sprite->ft4);
		}
		else
			AddPrim(ot + otz, &sprite->f4);
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

void drawFont(u_char *text, int xx, int yy, u_char autoReturn){
	u_char c;
	int cursor = 0;
	int i = 0;
	int line = 0;
	u_short textLen = strlen(text);

	while(i < textLen){
		short row, x, y;
		c = *text;
		//printf("%c\n", c);
		//printf("%d\n", c);

		if(autoReturn == 1 && cursor == 26 && cursor % 26 == 0){
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
			x = 192 + (font.sprt[font.index].w * (c - (32 + (8 * row))));
			y = (font.sprt[font.index].h * row);

			font.sprt[font.index].u0 = x;
			font.sprt[font.index].v0 = y; 

			setXY0(&font.sprt[font.index], xx+(7*(cursor++)), yy+(16*line));
		
			drawSprt(&font.dr_mode[font.index], &font.sprt[font.index], 1);
			text++;
			i++;
			font.index++;
			if(font.index >= FONT_MAX_CHARS-1)
				font.index = 0;
		}
	}
}

void drawMesh(Mesh *mesh, long _otz)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_FT4 *ft4 = mesh->ft4;
	POLY_F4 *f4 = mesh->f4;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	long otz = 0;
	size_t n = 0;
	psGte(mesh->pos, mesh->rot);

	for (n = 0; n < mesh->indicesLength*4; n += 4) {
		if(mesh->tpage != 0){
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
		}
		else {
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

void node_push(Node **node, void *data, DataType type) {
	Node *newNode = malloc3(sizeof(Node));
	if (newNode == NULL) {
		printf("error on Node malloc3\n");
		return;
	}

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

void node_remove(Node **node, void *data) {
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

void scene_add(void *data, DataType type) {
	node_push(&scene.node, data, type);
}

void scene_remove(void *data) {
	node_remove(&scene.node, data);
}

void scene_free() {
	node_free(&scene.node);
}

void scene_draw(){
	Node *current = scene.node;
	Balloon *b;
	while(current != NULL) {
		switch(current->type) {
			case TYPE_MESH:
				drawMesh((Mesh*)current->data, 0);
				break;
			case TYPE_SPRITE:
				drawSprite((Sprite*)current->data, 0);
				break;
			case TYPE_SPRITE2D:
				drawSprite_2d((Sprite*)current->data, 0);
				break;
			case TYPE_CHARACTER:
				char_draw((Character*)current->data, 0, drawMesh);
				break;
			case TYPE_UI:
				drawSprite_2d((Sprite*)current->data, 1);
				break;
			case TYPE_FONT:
				b = (Balloon*)current->data;
				drawFont(b->text, balloon.sprite.pos.vx + 10, balloon.sprite.pos.vy + 10, 1);
				break;
		}
		current = current->next;
	}
}

void scene_load(void(*callback)){
	scene.status = SCENE_LOAD;
	scene.load_callback = callback;
}

void enableScreen(){
	SetDispMask(1);
}

void disableScreen(){
	ResetGraph(0);
}

void billboard(Sprite *sprite){
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
