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

#ifndef __VU_H__
#define __VU_H__

#define REG_STATUS_FLAG	16
#define REG_MAC_FLAG	17
#define REG_CLIP_FLAG	18
#define REG_R			20
#define REG_I			21
#define REG_Q			22
#define REG_P           23 //only exists in micromode 
#define REG_TPC			26
#define REG_CMSAR0		27
#define REG_FBRST		28
#define REG_VPU_STAT	29
#define REG_CMSAR1		31

enum VUStatus {
	VU_Ready = 0,
	VU_Run   = 1,
	VU_Stop  = 2,
};

typedef union {
	float F;
	s32   SL;
	u32	  UL;
	s16   SS[2];
	u16   US[2];		
	s8    SC[4];
	u8    UC[4];
} REG_VI;

#define VUFLAG_BREAKONMFLAG		0x00000001
#define VUFLAG_MFLAGSET			0x00000002

typedef struct {
	int enable;
	REG_VI reg;
	u32 sCycle;
	u32 Cycle;
	u32 statusflag;
} fdivPipe;

typedef struct {
	int enable;
	REG_VI reg;
	u32 sCycle;
	u32 Cycle;
} efuPipe;

typedef struct {
	int enable;
	int reg;
	int xyzw;
	u32 sCycle;
	u32 Cycle;
	u32 macflag;
	u32 statusflag;
	u32 clipflag;
} fmacPipe;

typedef struct {
	VECTOR	ACC;
	VECTOR	*VF;
	REG_VI	*VI;

	fmacPipe fmac[8];
	fdivPipe fdiv;
	efuPipe efu;

	u32 code;
	u32 maxmem;
	u32 maxmicro;
	u32 flags;
	u32 cycle;

	u16 branch;
	u16 ebit;
	u32 branchpc;

	REG_VI q;
	REG_VI p;
	u32 macflag;
	u32 statusflag;
	u32 clipflag;

	void (*vuExec)(void*);
	VIFregisters *vifRegs;

	u8 *Mem;
	u8 *Micro;
} VURegs;

#define VUPIPE_NONE		0
#define VUPIPE_FMAC		1
#define VUPIPE_FDIV		2
#define VUPIPE_EFU		3
#define VUPIPE_IALU		4
#define VUPIPE_BRANCH	5
#define VUPIPE_XGKICK	6

#define VUREG_READ		0x1
#define VUREG_WRITE		0x2

typedef struct {
	u8 pipe;
	u8 VFwrite;
	u8 VFwxyzw;
	u8 VFr0xyzw;
	u8 VFr1xyzw;
	u8 VFread0;
	u8 VFread1;
	u32 VIwrite;
	u32 VIread;
	int cycles;
} _VURegsNum;

#endif /* __VU_H__ */
