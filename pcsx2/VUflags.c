/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2005  Pcsx2 Team
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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "Common.h"
#include "VUmicro.h"
/*****************************************/
/*          NEW FLAGS                    */ //By asadr. Thnkx F|RES :p
/*****************************************/


__inline void vuUpdateDI(VURegs * VU) {
//	u32 Flag_S = 0;
//	u32 Flag_I = 0;
//	u32 Flag_D = 0;
//
//	/*
//	FLAG D - I
//	*/
//	Flag_I = (VU->statusflag >> 4) & 0x1;
//	Flag_D = (VU->statusflag >> 5) & 0x1;
//
//	VU->statusflag|= (Flag_I | (VU0.VI[REG_STATUS_FLAG].US[0] >> 4)) << 10;
//	VU->statusflag|= (Flag_D | (VU0.VI[REG_STATUS_FLAG].US[0] >> 5)) << 11;
}

#define VU_MAC_UPDATE(name, shift) \
u32 name(VURegs * VU, float f) { \
	u32 v = *(u32*)&f; \
	int exp = (v >> 23) & 0xff; \
	u32 s = v & 0x80000000; \
	\
	if (s) VU->macflag |= 0x0010<<shift;  \
	else VU->macflag &= ~(0x0010<<shift); \
	\
	if( f == 0 ) { \
		VU->macflag = (VU->macflag & ~(0x1100<<shift)) | (0x0001<<shift); \
		return v; \
	} \
	\
	switch(exp) { \
		case 0: \
			VU->macflag = (VU->macflag&~(0x1000<<shift)) | (0x0101<<shift);  \
			return s; \
		case 255: \
			VU->macflag = (VU->macflag&~(0x0100<<shift)) | (0x1000<<shift);  \
			return s|0x7f7fffff; /* max allowed */ \
		default: \
			VU->macflag = (VU->macflag & ~(0x1101<<shift)); \
			return v; \
	} \
} \

VU_MAC_UPDATE(VU_MACx_UPDATE, 3);
VU_MAC_UPDATE(VU_MACy_UPDATE, 2);
VU_MAC_UPDATE(VU_MACz_UPDATE, 1);
VU_MAC_UPDATE(VU_MACw_UPDATE, 0);

#define VU_MAC_CLEAR(name, shift) \
void name(VURegs * VU) { \
	VU->macflag&= ~(0x1111<<shift); \
} \

VU_MAC_CLEAR(VU_MACx_CLEAR, 3);
VU_MAC_CLEAR(VU_MACy_CLEAR, 2);
VU_MAC_CLEAR(VU_MACz_CLEAR, 1);
VU_MAC_CLEAR(VU_MACw_CLEAR, 0);

void VU_STAT_UPDATE(VURegs * VU) {
	int newflag = 0 ;
	if (VU->macflag & 0x000F) newflag = 0x1;
	if (VU->macflag & 0x00F0) newflag |= 0x2;
	if (VU->macflag & 0x0F00) newflag |= 0x4;
	if (VU->macflag & 0xF000) newflag |= 0x8;
	VU->statusflag = (VU->statusflag&0xc30)|newflag|((VU->statusflag&0xf)<<6);
}
