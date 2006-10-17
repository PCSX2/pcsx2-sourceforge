/******************************************************************************
    GStaris - GS plugin for PS2 emulators < gstaris.ngemu.com        >
    Copyright (C) 2003 Absolute0          < absolutezero@ifrance.com >

    This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/
// 1/8/2003 : replace PS2EgetLibVersion with PS2EgetLibVersion2 (shadow)

#include "GStaris.h"
#include <stdio.h>
#include <time.h>

// vertex information

s32 primCount[] = { 1,2,2,3,3,3,2,0 };
void (*primTable[])(gsvertex *v) = {
	primPoint,
	primLine, primLine,
	primTriangle, primTriangle, primTriangle,
	primSprite,
	primNothing
};

// PS2E funcs
// First of all, plugin infos for emulators

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_GS;
}

char* CALLBACK PS2EgetLibName() {
	return NAME;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (PLUGIN_VERSION<<16)|(VERSION<<8)|(BUILD);
}

// Logging funcs

FILE *logfile;

void GSlog(char *string, ...)
{
	va_list list;

	if (config.log.logon) {
		va_start(list,string);
		vfprintf(logfile,string,list);
		va_end(list);
		fflush(logfile);
	}
}

// GSreset > reset the gs

void GSreset()
{
	csr.reset = 1;
	dispfb = &_dispfb[1]; display = &_display[1];
	_dispfb[0].fbp = _dispfb[1].fbp = 0;
	_dispfb[0].fbw = _dispfb[1].fbw = 640;
	_dispfb[0].psm = _dispfb[1].psm = 0;
	_dispfb[0].dbx = _dispfb[1].dbx = _dispfb[0].dby = _dispfb[1].dby = 0;
	_display[0].dx = _display[1].dx = _display[0].dy = _display[1].dy = 0;
	_display[0].w  = _display[1].w  = 640;
	_display[0].h  = _display[1].h  = 200;
	pmode.en1      = 0; pmode.en2   = 1;
	smode2.intm    = 0;

	prmodecont = 1;
	prim = &_prim[1];
	_prim[1].ctxt = 1;
	_frame[0].fbw = _frame[1].fbw = 640;
//	_clamp[0].wms = _clamp[0].wmt = _clamp[1].wms = _clamp[1].wmt = 0;
	GSupdateContext();

	texa.aem = 1;
	texa.ta0 = 0xffffffff;
	texa.ta1 = 0xffffffff;
}

// Init/Shutdown functions
// Start using the plugin and shutdown it

s32 CALLBACK GSinit()
{
	vRam = NULL;
	vRam = (unsigned char*)malloc(4194304*2); // 4*1024*1024   * 2 prevent j0mommaz crash (ok it's big but temp)
	if (!vRam) return -1;
	vRamUL = (unsigned long*)vRam;

	// ADD HERE > reset registers

	GSreset();

	gifmode = 0;

	image = NULL;

	framec=0;
	fps=0;
	fpsc=0;
	to=0;

	snap_frame = 0;
	snap_path = (char*)malloc(MAX_PATH);

	already_opened = 0;
	config_changed = 0;

	return 0;
}

void CALLBACK GSshutdown()
{
	if (snap_path) free(snap_path);
	if (image) free(image);
	if (vRam)  free(vRam);

	if (tCount > -1) {
		FreeTextures();
		tCount = -1;
	}

	GSdestroyGL();
	GSdestroyWindow();
}

// Open/Close functions
// Start/Stop a demo

s32 CALLBACK GSopen(void *pDsp, char *Title)
{
	emulator_title = NULL;
	emulator_title = (char*)malloc(strlen(Title));
	strcpy(emulator_title,Title);
	if (!emulator_title) sprintf(emulator_title,"(unknow PS2 emulator)");

	if (config_load()) {
		config_default();
	}
	config.itf.texture_view = -1;
	config.itf.texture_info = 0;

	*(HWND*)pDsp = GScreateMsgWindow();
	if (!pDsp)                          return -1;

	if (already_opened) {
		if (config_changed) {
			if (tCount > -1) {
				FreeTextures();
				tCount = -1;
			}
			GSdestroyGL();
			GSdestroyWindow();
			config_changed = 0;
		} else return -GSshowWindow();
	}

	if (GScreateWindow(emulator_title)) return -1;
	if (GScreateGL())                   return -1;
	if (config.video.fullscreen) {
		if (GSfullscreenGL())           return -1;
	}

	textures = NULL;
	tCount   = -1;

	logfile = NULL;
	logfile = fopen("GStaris.log","wt");

	if (config.log.isoc) {
		char dtbuf[128];

		GSlog("GStaris OpenGL GS plugin on %s\n",emulator_title);
		GSlog("GStaris project © Absolute0\n");

		_strdate(dtbuf); GSlog("Date: %s - ",dtbuf);
		_strtime(dtbuf); GSlog("Time: %s",dtbuf);

		GSlog("\n\n... log on\n\n\n");
	}

	GSsetAA1();

	already_opened = 1;

	return 0;
}

void CALLBACK GSclose()
{
	if (config.log.isoc) {
		char dtbuf[128];

		GSlog("\n\n... log off\n\n");

		_strdate(dtbuf); GSlog("Date: %s - ",dtbuf);
		_strtime(dtbuf); GSlog("Time: %s",dtbuf);
	}

	fflush(logfile);
	fclose(logfile);

	GShideWindow();

	GSdestroyMsgWindow();
	if (emulator_title) free(emulator_title);
}

// VSync func
// Render stuff

void CALLBACK GSvsync()
{
	framec++; fpsc++;
	if (time(NULL)>to) {
		time(&to);
		fps   = fpsc;
		fpsc  = 0;
		fps2  = fpsc2;
		fpsc2 = 0;
	}

	GSupdateGL((config.fixes.fbfix == 0));
}
// Misc

void CALLBACK GSmakeSnapshot(char *path)
{
	snap_frame = 1;
	strcpy(snap_path,path);
}

s32 CALLBACK GStest()
{
	if (already_opened && config_changed)
		return -1;
	return 0;
}

