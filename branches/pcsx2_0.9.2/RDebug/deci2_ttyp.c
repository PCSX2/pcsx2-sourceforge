/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2003  Pcsx2 Team
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

#include "Common.h"
#include "deci2.h"

typedef struct tag_DECI2_TTYP_HEADER{
	DECI2_HEADER	h;		//+00
	u32		flushreq;	//+08
	u8		data[0];	//+0C
} DECI2_TTYP_HEADER;			//=0C

void sendTTYP(u16 protocol, u8 source, char *data){
	static char tmp[2048];
	((DECI2_TTYP_HEADER*)tmp)->h.length		=sizeof(DECI2_TTYP_HEADER)+strlen(data);
	((DECI2_TTYP_HEADER*)tmp)->h._pad		=0;
	((DECI2_TTYP_HEADER*)tmp)->h.protocol	=protocol +(source=='E' ? PROTO_ETTYP : PROTO_ITTYP);
	((DECI2_TTYP_HEADER*)tmp)->h.source		=source;
	((DECI2_TTYP_HEADER*)tmp)->h.destination='H';
	((DECI2_TTYP_HEADER*)tmp)->flushreq		=0;
	if (((DECI2_TTYP_HEADER*)tmp)->h.length>2048)
		SysMessage("TTYP: Buffer overflow");
	else
		memcpy(&tmp[sizeof(DECI2_TTYP_HEADER)], data, strlen(data));
	//writeData(tmp);
}