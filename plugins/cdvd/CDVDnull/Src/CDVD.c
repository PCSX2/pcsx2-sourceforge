#include <stdio.h>

#include "CDVD.h"


char *LibName       = "CDVDnull Driver";

const unsigned char version = PS2E_CDVD_VERSION;
const unsigned char revision = 0;
const unsigned char build = 6;


char* CALLBACK PS2EgetLibName() {
	return LibName;
}

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_CDVD;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version << 16) | (revision << 8) | build;
}



void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "CDVDnull Msg", 0);
}

s32 CALLBACK CDVDinit() {
	return 0;
}

s32 CALLBACK CDVDopen() {
	return 0;
}

void CALLBACK CDVDclose() {
}

void CALLBACK CDVDshutdown() {
}

s32 CALLBACK CDVDreadTrack(u32 lsn, int mode) {
	return -1;
}

// return can be NULL (for async modes)
u8*  CALLBACK CDVDgetBuffer() {
	return NULL;
}

s32 CALLBACK CDVDreadSubQ(u32 lsn, cdvdSubQ* subq) {
	return -1;
}

s32 CALLBACK CDVDgetTN(cdvdTN *Buffer) {
	return -1;
}

s32 CALLBACK CDVDgetTD(u8 Track, cdvdTD *Buffer) {
	return -1;
}

s32 CALLBACK CDVDgetTOC(void* toc) {
	return -1;
}

s32 CALLBACK CDVDgetDiskType() {
	return CDVD_TYPE_NODISC;
}

s32 CALLBACK CDVDgetTrayStatus() {
	return CDVD_TRAY_CLOSE;
}

s32 CALLBACK CDVDctrlTrayOpen() {
	return 0;
}

s32 CALLBACK CDVDctrlTrayClose() {
	return 0;
}

void CALLBACK CDVDconfigure() {
	SysMessage("Nothing to Configure");
}

void CALLBACK CDVDabout() {
	SysMessage("%s %d.%d", LibName, revision, build);
}

s32 CALLBACK CDVDtest() {
	return 0;
}
