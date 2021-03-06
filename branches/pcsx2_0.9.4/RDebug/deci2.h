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

#ifndef __DECI2_H__
#define __DECI2_H__

#include "Common.h"
#include "deci2_dcmp.h"
#include "deci2_iloadp.h"
#include "deci2_dbgp.h"
#include "deci2_netmp.h"
#include "deci2_ttyp.h"

#define PROTO_DCMP		0x0001
#define PROTO_ITTYP		0x0110
#define PROTO_IDBGP		0x0130
#define PROTO_ILOADP	0x0150
#define PROTO_ETTYP		0x0220
#define PROTO_EDBGP		0x0230
#define PROTO_NETMP		0x0400


#pragma pack(1)
typedef struct tag_DECI2_HEADER{
	u16		length,		//+00
			_pad,		//+02
			protocol;	//+04
	char	source,		//+06
			destination;//+07
} DECI2_HEADER;			//=08

typedef struct tag_DECI2_DBGP_BRK{
	u32	address,			//+00
		count;				//+04
} DECI2_DBGP_BRK;			//=08
#pragma pack()

#define STOP	0
#define RUN		1

extern DECI2_DBGP_BRK	ebrk[32], ibrk[32];
extern int				ebrk_count, ibrk_count;
extern int				runStatus, runCode, runCount;

#ifdef _WIN32
extern HANDLE			runEvent;					//i don't like this;
#endif

extern int		connected;
													//when add linux code this might change

int	writeData(char *result);
void	exchangeSD(DECI2_HEADER *h);

#endif//__DECI2_H__
