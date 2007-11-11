/*  ZeroPAD
 *  Copyright (C) 2006-2007
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PAD_H__
#define __PAD_H__

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#endif

#include <vector>
#include <map>
#include <string>
using namespace std;

#define PADdefs
extern "C" {
#include "PS2Edefs.h"
}

extern char *libraryName;

#define PAD_GETKEY(key) (key&0xffff)
#define IS_KEYBOARD(key) (key<0x10000)
#define IS_JOYBUTTONS(key) (key>=0x10000 && key<0x20000)
#define IS_JOYSTICK(key) (key>=0x20000&&key<0x30000)
#define IS_POV(key) (key>=0x30000&&key<0x40000)
#define IS_MOUSE(key) (key>=0x40000&&key<0x50000)

#define PADKEYS 20

typedef struct {
	unsigned long keys[2][PADKEYS];
	int log;
} PADconf;

typedef struct {
	u8 x,y;
	u8 button;
} Analog;

extern PADconf conf;

extern Analog lanalog[2], ranalog[2];
extern FILE *padLog;
#define PAD_LOG __Log

#define PAD_LEFT      0x8000
#define PAD_DOWN      0x4000
#define PAD_RIGHT     0x2000
#define PAD_UP        0x1000
#define PAD_START     0x0800
#define PAD_R3        0x0400
#define PAD_L3        0x0200
#define PAD_SELECT    0x0100
#define PAD_SQUARE    0x0080
#define PAD_CROSS     0x0040
#define PAD_CIRCLE    0x0020
#define PAD_TRIANGLE  0x0010
#define PAD_R1        0x0008
#define PAD_L1        0x0004
#define PAD_R2        0x0002
#define PAD_L2        0x0001

/* end of pad.h */

extern keyEvent event;

extern u16 status[2];
extern u32 pads;

int POV(u32 direction, u32 angle);
s32  _PADopen(void *pDsp);
void _PADclose();
void _KeyPress(int pad, u32 key);
void _KeyRelease(int pad, u32 key);
void PADsetMode(int pad, int mode);
void _PADupdate(int pad);

void __Log(char *fmt, ...);
void LoadConfig();
void SaveConfig();

void SysMessage(char *fmt, ...);

#endif


