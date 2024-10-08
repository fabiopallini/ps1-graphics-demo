#include "psx.h"
#include "utils.h"

#define SOUND_MALLOC_MAX 3 
#define SPU_SONG_SIZE 300000

DISPENV	dispenv[2];
DRAWENV	drawenv[2];
int dispid = 0;
u_long ot[OTSIZE];
u_short otIndex;
u_char screenWait = 0;
u_long current_vag_song_size;
DslCB cd_read_callback();

void billboards_updated()
{
	SpriteNode *current = scene.spriteNode;
	while (current != NULL) {
		current->data->rot.vy = camera.rot.vy * -1;
		current = current->next;
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
	//init heap 2 megabyte
	//InitHeap3((void*)0x801F8000, 0x00200000); // 16KB stack
	InitHeap3((void*)0x801EFFF0, 0x00200000); // 64KB stack

	ResetCallback();
	ResetGraph(0);
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
	DS_callback_flag = 0;
	DsReadCallback((DslCB)cd_read_callback);
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
	VSync(0);
	PutDispEnv(&dispenv[dispid]);
	PutDrawEnv(&drawenv[dispid]);
	//DrawOTag(ot);
	DrawOTag(ot+OTSIZE-1);
	//FntPrint(font_id, "free time = %d\n", count2 - count1);
	otIndex = 0;

	if(screenWait < 1)
		screenWait++;
	if(screenWait == 1){
		enableScreen();
		screenWait++;
	}
}

void psExit(){
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
		printf("loading %s, size: %lu\n", file_path_raw, temp_file_info->size);
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
		printf("file loaded!\n");
	} else {
		printf("file not found");
	}

	// Clean up
	free3(file_path_raw);
	free3(sectors_size);
	free3(temp_file_info);
}

DslCB cd_read_callback(){
	if(DS_callback_flag == 1){
		if(!vagSong.state)
			return 0;
		//printf("cd read callback\n");
		memcpy(vagSong.data, 0, SPU_SONG_SIZE);
		memcpy(vagSong.data, vagSong.cd_data, vagSong.chunk_size);
		free3(vagSong.cd_data);
		//printf("index %d, transfer address %d\n", vagSong.index, (vagSong.index-1) * vagSong.chunk_size);
		SpuSetTransferStartAddr(vagSong.spu_addr + (vagSong.index-1) * vagSong.chunk_size);
		SpuWrite((u_char *)vagSong.data, vagSong.chunk_size);
		if(vagSong.index == 3)
			vagSong.index = 4;
		DS_callback_flag = 0;
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
	if(callbackID)
		DS_callback_flag = callbackID;
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

		printf("loading file %s from byte %lu to byte %lu\n", file_path_raw, start_byte, end_byte);
		bytes_to_read = end_byte - start_byte;

		*sectors_size = bytes_to_read + (SECTOR % bytes_to_read);
		*file = malloc3(*sectors_size + SECTOR);
		if (*file == NULL) {
			printf("file %s malloc3 failed\n", file_path);
			DS_callback_flag  = 0;
			// Clean up
			free3(file_path_raw);
			free3(sectors_size);
			free3(temp_file_info);
			return;
		}    
		current_vag_song_size = temp_file_info->size;
		DsRead(&start_loc, (*sectors_size + SECTOR -1) / SECTOR, *file, DslModeSpeed);
		if(!callbackID)
			while(DsReadSync(NULL));
	} else {
		printf("file not found\n");
		DS_callback_flag  = 0;
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
	//printf("spu_handler\n");
	SpuSetIRQ(SPU_OFF);
	if(!vagSong.state)
		return 0;
	if(!vagSong.chunk_addr)
		vagSong.chunk_addr = SPU_SONG_SIZE;

	if(vagSong.index == 3){
		SpuSetKey(SpuOn, SPU_0CH); // play again from begin (data is changed, so it will starts to play the next block
	}
	cd_read_file_bytes(vagSong.name, &vagSong.cd_data, vagSong.chunk_addr, vagSong.chunk_addr + vagSong.chunk_size, 1);
	vagSong.chunk_addr += vagSong.chunk_size;
	if(vagSong.chunk_addr >= current_vag_song_size){
		vagSong.chunk_addr = NULL;
	}
	return 0;
}

SpuTransferCallbackProc spu_transfer_callback(){
	//printf("transfer callback\n");
	if(!vagSong.state){
		return 0;
	}
	if(vagSong.index == 0){
		vagSong.index = 1;
		SpuSetIRQ(SPU_ON);
		SpuSetIRQAddr(vagSong.spu_addr + vagSong.chunk_size);
		SpuSetKey(SpuOn, SPU_0CH); // start play the song from begin
	}
	else if(vagSong.index == 1){
		SpuSetIRQ(SPU_ON);
		SpuSetIRQAddr(vagSong.spu_addr + (vagSong.chunk_size*2));
		vagSong.index = 2;
	}
	else if(vagSong.index == 2){
		vagSong.index = 3;
		SpuSetIRQ(SPU_ON);
		SpuSetIRQAddr(vagSong.spu_addr + SPU_SONG_SIZE);
	}
	else if(vagSong.index == 4){
		vagSong.index = 1;
		SpuSetIRQ(SPU_ON);
		SpuSetIRQAddr(vagSong.spu_addr + vagSong.chunk_size);
	}
	return 0;
}

void vag_song_load(u_char* vagName, int voice_channel){
	vagSong.name = malloc3(strlen(vagName));
	strcpy(vagSong.name, vagName);
	vagSong.data = malloc3(SPU_SONG_SIZE);
	vagSong.chunk_size = SPU_SONG_SIZE / 3; // size in bytes, triple buffer 
	vagSong.chunk_addr = 0;
	vagSong.index = 0;
	vagSong.state = 1;
	vagSong.spu_addr = SpuMalloc(SPU_SONG_SIZE);

	cd_read_file_bytes(vagName, &vagSong.cd_data, 0, SPU_SONG_SIZE, 0);
	memcpy(vagSong.data, vagSong.cd_data, SPU_SONG_SIZE);
	free3(vagSong.cd_data);

	SpuSetTransferStartAddr(vagSong.spu_addr);
	SpuWrite((u_char *)vagSong.data, SPU_SONG_SIZE);
	spu_set_voice_attr(SPU_0CH, vagSong.spu_addr);
	SpuSetTransferCallback((SpuTransferCallbackProc)spu_transfer_callback);
	SpuSetIRQCallback((SpuIRQCallbackProc)spu_handler);
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

void vag_song_free(VagSong *vagSong) {
	vagSong->state = 0;
	SpuSetIRQ(SPU_OFF);
	SpuSetKey(SpuOff, SPU_0CH);
	DS_callback_flag = 0;
	SpuFree(vagSong->spu_addr);
	free3(vagSong->name);
	free3(vagSong->data);
}

void drawSprite(Sprite *sprite, long _otz){
	long otz;
	setVector(&sprite->vector[0], -sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[1], sprite->w, -sprite->h, 0);
	setVector(&sprite->vector[2], -sprite->w, sprite->h, 0);
	setVector(&sprite->vector[3], sprite->w, sprite->h, 0);
	psGte(sprite->pos, sprite->rot);
	if(sprite->tpage != NULL){
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
		if(_otz != NULL)
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
		if(_otz != NULL)
			otz = _otz;
		AddPrim(ot+otz, &sprite->f4);
	}
}

static void moveSprite(Sprite *sprite, long x, long y){
	if(sprite->tpage != NULL){
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
	if(_otz != NULL)
		otz = _otz;
	if(otIndex < OTSIZE){
		if(sprite->tpage != NULL) {
			sprite->ft4.tpage = sprite->tpage;
			AddPrim(ot + otz, &sprite->ft4);
		}
		else
			AddPrim(ot + otz, &sprite->f4);
	}
	if(_otz == NULL)
		otIndex++;
}

void drawSprt(DR_MODE *dr_mode, SPRT *sprt){
	if(otIndex < OTSIZE){
		AddPrim(ot + otIndex, sprt);
		AddPrim(ot + otIndex, dr_mode);
	}
	otIndex++;
}

void drawFont(Font *font, u_char *text, int xx, int yy, u_char autoReturn){
	u_char c;
	int cursor = 0;
	int i = 0;
	int line = 0;
	font_init(font);

	while((c = *text) != '\0' && i < FONT_MAX_CHARS){
		short row, x, y;
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

void drawMesh(Mesh *mesh, long _otz)
{
	// UP = -Y
	// FORWARD = +Z
	POLY_FT4 *ft4 = mesh->ft4;
	POLY_F4 *f4 = mesh->f4;
	SVECTOR *v = mesh->vertices;
	int *i = mesh->indices;
	long otz;
	size_t n;
	psGte(mesh->pos, mesh->rot);

	for (n = 0; n < mesh->indicesLength*4; n += 4) {
		if(mesh->tpage != NULL){
			ft4->tpage = mesh->tpage;
			otz = RotAverage4(&v[i[n + 0]],
					&v[i[n + 1]],
					&v[i[n + 2]],
					&v[i[n + 3]],
					(long *)&ft4->x0, (long *)&ft4->x1,
					(long *)&ft4->x3, (long *)&ft4->x2,
					0, 0);
			if(_otz != NULL)
				otz = _otz;
			if(otz > 0 && otz < OTSIZE)
				AddPrim(ot+otz, ft4);
			ft4++;
		}
		else {
			otz = RotAverage4(&v[i[n + 0]],
					&v[i[n + 1]],
					&v[i[n + 2]],
					&v[i[n + 3]],
					(long *)&f4->x0, (long *)&f4->x1,
					(long *)&f4->x3, (long *)&f4->x2,
					0, 0);
			if(_otz != NULL)
				otz = _otz;
			if(otz > 0 && otz < OTSIZE)
				AddPrim(ot+otz, f4);
			f4++;
		}	
	}
}

void drawMesh_ptr(Mesh **pmesh, long _otz){
	drawMesh(*pmesh, _otz);
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
		free3(current);
		current = nextNode;
	}
	scene.spriteNode = NULL;
}

void enableScreen(){
	SetDispMask(1);
}

void disableScreen(){
	ResetGraph(0);
}
