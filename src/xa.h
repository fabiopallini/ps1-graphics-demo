#include <sys/types.h>
#include <rand.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libcd.h>
#include <libsnd.h>
#include <libsn.h>
#include <stdio.h>

#define NUMCHANNELS 8 

void xa_init();
void xa_play(int channel);
void xa_pause();
void PlayXAChannel(int channel, int startPos, int endpos);
void cbready(int intr, u_char *result);
void UnprepareXA(CdlCB oldCallback);
