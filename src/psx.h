#ifndef PSX_H
#define PSX_H

#include <stdlib.h>
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include <stdio.h>
#include <libspu.h>
#include <libds.h>
#include <strings.h>
#include <libmath.h>

#define SCREEN_WIDTH 320
#define	SCREEN_HEIGHT 256
#define SECTOR 2048

u_long pad;

void psSetup();
void psClear();
void psGte(long x, long y, long z, short ax, short ay, short az);
void psDisplay();
void psAddPrimF4(POLY_F4 *poly);
void psAddPrimFT4(POLY_FT4 *poly);
void psLoadTim(u_short* tpage, unsigned char image[]);
void psCamera(long x, long y, long z, short rotX, short rotY, short rotZ);
void cd_open();
void cd_close();
void cd_read_file(unsigned char* file_path, u_long** file);
void audio_init();
void audio_vag_to_spu(char* sound, int sound_size, int voice_channel);
void audio_play(int voice_channel);
void audio_free(unsigned long spu_address);

#endif
