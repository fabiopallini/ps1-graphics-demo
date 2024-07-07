#include "xa.h"

// file name on the CD-ROM
// file start position filled in by CdSearchFile
// file end position filled in by CdSearchFile
// channel description (not needed for playing the .XA)
typedef struct {
	char* filename; 
	int startpos;
	int endpos;
	char* channelName[NUMCHANNELS];
} XAFILE;
XAFILE xafile = {"\\FINAL.XA;1", 0, 0};
// CD buffer - don't really need this as we are not interested in the data itself coming off of the CD-ROM
u_char buffer[2340];
CdlCB Oldcallback; // keep any old CdCallback
CdlCB PrepareXA(void);
int currentPos = 0; // current position of the CD-ROM

void xa_init()
{
	CdlFILE fp; // CD file details
	CdInit(); // init the CD-ROM
	//CdSetDebug(3); // CD-ROM debugging is on
	if(CdSearchFile(&fp,xafile.filename) == 0)
	{
	  printf("%s: not found!\nWaiting for the file to be loaded...\n", xafile.filename);
	}
   	// get the XA file start position
	xafile.startpos = CdPosToInt(&fp.pos);
	// get the XA file end position, start pos + number of sectors -1
	xafile.endpos = xafile.startpos + (fp.size/2048) -1;
	// set up for XA playing
	Oldcallback = PrepareXA();
	//PlayXAChannel(0,xafile.startpos,xafile.endpos);
	//stop
	//CdControlF(CdlPause,0);
	//VSync(3); //maybe useless

	// stop the CD-ROM when quitting
	//UnprepareXA(Oldcallback);
	// shut down
	//CdSync(0, 0);
	//StopCallback();
} 

void xa_play(int channel){
	xa_init();
	PlayXAChannel(channel, xafile.startpos, xafile.endpos);
}

void xa_pause(){
	CdControlF(CdlPause, 0);
	VSync(3);
	UnprepareXA(Oldcallback);
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
	theFilter.file=1;
	theFilter.chan=channelNum;
	CdControlF(CdlSetfilter, (u_char *)&theFilter);
	// starting position on the CD
	CdIntToPos(startPos, &loc);
	currentPos=startPos;
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
		CdGetSector((u_long*)buffer,585);
		currentPos = CdPosToInt((CdlLOC *)buffer);
		if(currentPos >= xafile.endpos){
			SsSetSerialVol(SS_SERIAL_A,0,0);
			CdControlF(CdlPause,0);
		}
	}
	else
		printf("UnHandled Callback Occured\n");	
}
