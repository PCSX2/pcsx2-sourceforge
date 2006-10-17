/*  SPU2null
 *  Copyright (C) 2002-2005  SPU2null Team
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

#ifndef __SPU2_H__
#define __SPU2_H__

#include <stdio.h>

#ifdef __LINUX__
#include <gtk/gtk.h>
#else
#include <windows.h>
#include <windowsx.h>
#endif

#define SPU2defs
#include "PS2Edefs.h"

FILE *spu2Log;
#define SPU2_LOG __Log  //debug mode


typedef struct {
  int Log;
} Config;

Config conf;

void __Log(char *fmt, ...);
void SaveConfig();
void LoadConfig();
void SysMessage(char *fmt, ...);

#endif /* __SPU2_H__ */
