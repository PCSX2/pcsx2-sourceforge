                                                                     
                                                                     
                                                                     
                                             
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

#include <stdlib.h>
#include <string.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"
#include "VU0.h"
#include "VUmicro.h"
#include "VUflags.h"
#include "iVUmicro.h"
#include "iVU0micro.h"
#include "iVU1micro.h"
#include "iVUops.h"

#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

int vucycle;
int vucycleold;

//Lower/Upper instructions can use that..
#define _Ft_ (( VU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ (( VU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ (( VU->code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X (( VU->code>>24) & 0x1)
#define _Y (( VU->code>>23) & 0x1)
#define _Z (( VU->code>>22) & 0x1)
#define _W (( VU->code>>21) & 0x1)

#define _Fsf_ (( VU->code >> 21) & 0x03)
#define _Ftf_ (( VU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff)
#define _UImm11_	(s32)(VU->code & 0x7ff)

#define VU_VFx_ADDR(x)  (int)&VU->VF[x].UL[0]
#define VU_VFy_ADDR(x)  (int)&VU->VF[x].UL[1]
#define VU_VFz_ADDR(x)  (int)&VU->VF[x].UL[2]
#define VU_VFw_ADDR(x)  (int)&VU->VF[x].UL[3]

#define VU_REGR_ADDR    (int)&VU->VI[REG_R]
#define VU_REGI_ADDR    (int)&VU->VI[REG_I]
#define VU_REGQ_ADDR    (int)&VU->VI[REG_Q]
#define VU_REGMAC_ADDR  (int)&VU0.VI[REG_MAC_FLAG]

#define VU_VI_ADDR(x)   (int)&VU->VI[x].UL

#define VU_ACCx_ADDR    (int)&VU->ACC.UL[0]
#define VU_ACCy_ADDR    (int)&VU->ACC.UL[1]
#define VU_ACCz_ADDR    (int)&VU->ACC.UL[2]
#define VU_ACCw_ADDR    (int)&VU->ACC.UL[3]


/********************************/
/*     HELP FUNCTIONS           */
/********************************/

#define _X_OR_Y   ((( VU->code >> 24 ) & 0x1 ) || (( VU->code >> 23 ) & 0x1 ) )
#define _Z_OR_W   ((( VU->code >> 22 ) & 0x1 ) || (( VU->code >> 21 ) & 0x1 ) )
#define _X_AND_Y  ((( VU->code >> 24 ) & 0x1 ) && (( VU->code >> 23 ) & 0x1 ) )
#define _Z_AND_W  ((( VU->code >> 22 ) & 0x1 ) && (( VU->code >> 21 ) & 0x1 ) )
#define _X_AND_Z  ((( VU->code >> 24 ) & 0x1 ) && (( VU->code >> 22 ) & 0x1 ) )
#define _Y_AND_Z  ((( VU->code >> 23 ) & 0x1 ) && (( VU->code >> 22 ) & 0x1 ) )
#define _Y_AND_W  ((( VU->code >> 23 ) & 0x1 ) && (( VU->code >> 21 ) & 0x1 ) )
#define _X_Y_Z_W  ((( VU->code >> 21 ) & 0xF ) )


__declspec(align(16)) float recMult_float_to_int4[4] = { 16.0, 16.0, 16.0, 16.0 };
__declspec(align(16)) float recMult_float_to_int12[4] = { 4096.0, 4096.0, 4096.0, 4096.0 };
__declspec(align(16)) float recMult_float_to_int15[4] = { 32768.0, 32768.0, 32768.0, 32768.0 };

__declspec(align(16)) float recMult_int_to_float4[4] = { 0.0625f, 0.0625f, 0.0625f, 0.0625f };
__declspec(align(16)) float recMult_int_to_float12[4] = { 0.000244140625, 0.000244140625, 0.000244140625, 0.000244140625 };
__declspec(align(16)) float recMult_int_to_float15[4] = { 0.000030517578125, 0.000030517578125, 0.000030517578125, 0.000030517578125 };
static s32 bpc;

extern void _flushVF_XMMReg(VURegs* VU, int vfreg);

#ifdef __MSCW32__
__declspec(align(16)) 
#endif

static int SSEmovMask[ 16 ][ 4 ] =
{
{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
{ 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF },
{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 },
{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 },
{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 },
{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000 },
{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF }
};

#define VU_SWAPSRC 0xf090 // don't touch

void VU_MERGE0(int dest, int src) { // 0000
}
void VU_MERGE1(int dest, int src) { // 1000
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc4);
}
void VU_MERGE2(int dest, int src) { // 0100
	SSE_MOVHLPS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0x64);
}
void VU_MERGE3(int dest, int src) { // 1100
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE4(int dest, int src) { // 0010s
	SSE_MOVSS_XMM_to_XMM(src, dest);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xe4);
}
void VU_MERGE5(int dest, int src) { // 1010
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0x8d);
	if(cpucaps.hasStreamingSIMD2Extensions)
		SSE2_PSHUFD_XMM_to_XMM(dest, src, 0x72);
	else
		SSE2EMU_PSHUFD_XMM_to_XMM(dest, src, 0x72);
}
void VU_MERGE6(int dest, int src) { // 0110
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xc9);
	if(cpucaps.hasStreamingSIMD2Extensions)
		SSE2_PSHUFD_XMM_to_XMM(dest, src, 0xd2);
	else
		SSE2EMU_PSHUFD_XMM_to_XMM(dest, src, 0xd2);
}
void VU_MERGE7(int dest, int src) { // 1110s
	SSE_MOVSS_XMM_to_XMM(src, dest);
}
void VU_MERGE8(int dest, int src) { // 0001
	SSE_MOVSS_XMM_to_XMM(dest, src);
}
void VU_MERGE9(int dest, int src) { // 1001
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0x9c);
	if(cpucaps.hasStreamingSIMD2Extensions)
		SSE2_PSHUFD_XMM_to_XMM(dest, src, 0x78);
	else
		SSE2EMU_PSHUFD_XMM_to_XMM(dest, src, 0x78);
}
void VU_MERGE10(int dest, int src) { // 0101
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xd8);
	if(cpucaps.hasStreamingSIMD2Extensions)
		SSE2_PSHUFD_XMM_to_XMM(dest, src, 0xd8);
	else
		SSE2EMU_PSHUFD_XMM_to_XMM(dest, src, 0xd8);
}
void VU_MERGE11(int dest, int src) { // 1101
	SSE_MOVSS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
}
void VU_MERGE12(int dest, int src) { // 0011s
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xe4);
}
void VU_MERGE13(int dest, int src) { // 1011s
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0x64);
}
void VU_MERGE14(int dest, int src) { // 0111s
	SSE_MOVHLPS_XMM_to_XMM(dest, src);
	SSE_SHUFPS_XMM_to_XMM(src, dest, 0xc4);
}
void VU_MERGE15(int dest, int src) { // 1111s
}

typedef void (*VUMERGEFN)(int dest, int src);
static VUMERGEFN s_VuMerge[16] = {
	VU_MERGE0, VU_MERGE1, VU_MERGE2, VU_MERGE3,
	VU_MERGE4, VU_MERGE5, VU_MERGE6, VU_MERGE7,
	VU_MERGE8, VU_MERGE9, VU_MERGE10, VU_MERGE11,
	VU_MERGE12, VU_MERGE13, VU_MERGE14, VU_MERGE15 };

#define SWAP(x, y) *(u32*)&y ^= *(u32*)&x ^= *(u32*)&y ^= *(u32*)&x;

#define VU_MERGE_REGS(dest, src) { \
	assert( xmmregs[src].type == XMMTYPE_TEMP ); \
	s_VuMerge[_X_Y_Z_W](dest, src); \
	if( VU_SWAPSRC & (1<<_X_Y_Z_W) ) { \
		/* swap */ \
		xmmregs[src] = xmmregs[dest]; \
		xmmregs[dest].inuse = 0; \
		SWAP(dest, src); \
	} \
	else { \
		_freeXMMreg(src); \
	} \
} \

#define VU_MERGE_REGS_CUSTOM(dest, src, xyzw) { \
	assert( xmmregs[src].type == XMMTYPE_TEMP ); \
	s_VuMerge[xyzw](dest, src); \
	if( VU_SWAPSRC & (1<<xyzw) ) { \
		/* swap */ \
		xmmregs[src] = xmmregs[dest]; \
		xmmregs[dest].inuse = 0; \
		SWAP(dest, src); \
	} \
	else { \
		_freeXMMreg(src); \
	} \
} \

void _unpackVF_xyzw(int dstreg, int srcreg, int xyzw) {
	if(cpucaps.hasStreamingSIMD2Extensions) 
	{
		switch (xyzw) {
			case 0: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x00); break;
			case 1: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x55); break;
			case 2: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xaa); break;
			case 3: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xff); break;
		}
	} else {
		switch (xyzw) {
			case 0: SSE2EMU_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x00); break;
			case 1: SSE2EMU_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x55); break;
			case 2: SSE2EMU_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xaa); break;
			case 3: SSE2EMU_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xff); break;
		}
	}
}

void _recvuFMACflush(VURegs * VU) {
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;

		if ((vucycle - VU->fmac[i].sCycle) >= VU->fmac[i].Cycle) {
#ifdef VUM_LOG
//			if (Log) { VUM_LOG("flushing FMAC pipe[%d]\n", i); }
#endif
			VU->fmac[i].enable = 0;
		}
	}
}

void _recvuFDIVflush(VURegs * VU) {
	if (VU->fdiv.enable == 0) return;

	if ((vucycle - VU->fdiv.sCycle) >= VU->fdiv.Cycle) {
//		SysPrintf("flushing FDIV pipe\n");
		VU->fdiv.enable = 0;
	}
}

void _recvuEFUflush(VURegs * VU) {
	if (VU->efu.enable == 0) return;

	if ((vucycle - VU->efu.sCycle) >= VU->efu.Cycle) {
//		SysPrintf("flushing FDIV pipe\n");
		VU->efu.enable = 0;
	}
}

void _recvuTestPipes(VURegs * VU) {
	_recvuFMACflush(VU);
	_recvuFDIVflush(VU);
	_recvuEFUflush(VU);
}

void _recvuFMACTestStall(VURegs * VU, int reg, int xyzw) {
	int cycle;
	int i;

	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 0) continue;
		if (VU->fmac[i].reg == reg &&
			VU->fmac[i].xyzw & xyzw) break;
	}

	if (i == 8) return;

	cycle = VU->fmac[i].Cycle - (vucycle - VU->fmac[i].sCycle);
	VU->fmac[i].enable = 0;

//	SysPrintf("FMAC stall %d\n", cycle);
	vucycle+= cycle;
	_recvuTestPipes(VU);
}

void _recvuFMACAdd(VURegs * VU, int reg, int xyzw) {
	int i;

	/* find a free fmac pipe */
	for (i=0; i<8; i++) {
		if (VU->fmac[i].enable == 1) continue;
		break;
	}
	if (i==8) {
		SysPrintf("*PCSX2*: error , out of fmacs\n");
	}

#ifdef VUM_LOG
//	if (Log) { VUM_LOG("adding FMAC pipe[%d]; reg %d\n", i, reg); }
#endif
	VU->fmac[i].enable = 1;
	VU->fmac[i].sCycle = vucycle;
	VU->fmac[i].Cycle = 3;
	VU->fmac[i].xyzw = xyzw; 
	VU->fmac[i].reg = reg; 
}

void _recvuFDIVAdd(VURegs * VU, int cycles) {
//	SysPrintf("adding FDIV pipe\n");
	VU->fdiv.enable = 1;
	VU->fdiv.sCycle = vucycle;
	VU->fdiv.Cycle  = cycles;
}

void _recvuEFUAdd(VURegs * VU, int cycles) {
//	SysPrintf("adding EFU pipe\n");
	VU->efu.enable = 1;
	VU->efu.sCycle = vucycle;
	VU->efu.Cycle  = cycles;
}

void _recvuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VFread0) {
		_recvuFMACTestStall(VU, VUregsn->VFread0, VUregsn->VFr0xyzw);
	}
	if (VUregsn->VFread1) {
		_recvuFMACTestStall(VU, VUregsn->VFread1, VUregsn->VFr1xyzw);
	}
}

void _recvuAddFMACStalls(VURegs * VU, _VURegsNum *VUregsn) {
	if (VUregsn->VFwrite) {
		_recvuFMACAdd(VU, VUregsn->VFwrite, VUregsn->VFwxyzw);
	} else
	if (VUregsn->VIwrite & (1 << REG_CLIP_FLAG)) {
//		SysPrintf("REG_CLIP_FLAG pipe\n");
		_recvuFMACAdd(VU, -REG_CLIP_FLAG, 0);
	} else {
		_recvuFMACAdd(VU, 0, 0);
	}
}

void _recvuFlushFDIV(VURegs * VU) {
	int cycle;

	if (VU->fdiv.enable == 0) return;

	cycle = VU->fdiv.Cycle - (vucycle - VU->fdiv.sCycle);
//	SysPrintf("waiting FDIV pipe %d\n", cycle);
	VU->fdiv.enable = 0;
	vucycle+= cycle;
}

void _recvuFlushEFU(VURegs * VU) {
	int cycle;

	if (VU->efu.enable == 0) return;

	cycle = VU->efu.Cycle - (vucycle - VU->efu.sCycle);
//	SysPrintf("waiting FDIV pipe %d\n", cycle);
	VU->efu.enable = 0;
	vucycle+= cycle;
}

void _recvuTestFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	_recvuFlushFDIV(VU);
}

void _recvuTestEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	_recvuFlushEFU(VU);
}

void _recvuAddFDIVStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	if (VUregsn->VIwrite & (1 << REG_Q)) {
		_recvuFDIVAdd(VU, VUregsn->cycles);
	}
}

void _recvuAddEFUStalls(VURegs * VU, _VURegsNum *VUregsn) {
//	_vuTestFMACStalls(VURegs * VU, _VURegsNum *VUregsn);
	if (VUregsn->VIwrite & (1 << REG_P)) {
		_recvuEFUAdd(VU, VUregsn->cycles);
	}
}

void _recvuTestUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn); break;
	}
}

void _recvuTestLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuTestFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _recvuTestFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuTestEFUStalls(VU, VUregsn); break;
	}
}

void _recvuAddUpperStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn); break;
	}
}

void _recvuAddLowerStalls(VURegs * VU, _VURegsNum *VUregsn) {
	switch (VUregsn->pipe) {
		case VUPIPE_FMAC: _recvuAddFMACStalls(VU, VUregsn); break;
		case VUPIPE_FDIV: _recvuAddFDIVStalls(VU, VUregsn); break;
		case VUPIPE_EFU:  _recvuAddEFUStalls(VU, VUregsn); break;
	}
}


void SuperVU1AnalyzeOp(VURegs *VU, _vuopinfo *info, _VURegsNum* pCodeRegs)
{
	_VURegsNum* lregs;
	_VURegsNum* uregs;
	int *ptr; 

	lregs = pCodeRegs;
	uregs = pCodeRegs+1;

	ptr = (int*)&VU->Micro[pc]; 
	pc += 8; 

	if (ptr[1] & 0x40000000) { // EOP
		branch |= 8; 
	} 
 
	VU->code = ptr[1];
	if (VU == &VU1) {
		VU1regs_UPPER_OPCODE[VU->code & 0x3f](uregs);
	} else {
		VU0regs_UPPER_OPCODE[VU->code & 0x3f](uregs);
	}

	_recvuTestUpperStalls(VU, uregs);
	switch(VU->code & 0x3f) {
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x1d: case 0x1f:
		case 0x2b: case 0x2f:
			break;

		case 0x3c:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3d:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3e:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;
		case 0x3f:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7: case 0xb:
					break;
				default:
					info->statusflag = 4;
					info->macflag = 4;
					break;
			}
			break;

		default:
			info->statusflag = 4;
			info->macflag = 4;
			break;
	}

	if (uregs->VIwrite & (1 << REG_CLIP_FLAG)) {
		info->clipflag = 4;
		uregs->VIwrite &= ~(1 << REG_CLIP_FLAG);
	}

	if (uregs->VIread & (1 << REG_Q)) {
		info->q |= 2;
	}

	if (uregs->VIread & (1 << REG_P)) {
		assert( VU == &VU1 );
		info->p |= 2;
	}

	// check upper flags
	if (ptr[1] & 0x80000000) { // I flag
		info->cycle = vucycle;

	} else {

		VU->code = ptr[0]; 
		if (VU == &VU1) {
			VU1regs_LOWER_OPCODE[VU->code >> 25](lregs);
		} else {
			VU0regs_LOWER_OPCODE[VU->code >> 25](lregs);
		}

		_recvuTestLowerStalls(VU, lregs);
		info->cycle = vucycle;

		if (lregs->pipe == VUPIPE_BRANCH) {
			branch |= 1;
		}

		if (lregs->VIwrite & (1 << REG_Q)) {
			info->q |= 4;
			info->cycles = lregs->cycles;
			//info->pqinst = (1 << ((VU->code&3)==2)); // rsqrt is 2
		}
		else if (lregs->pipe == VUPIPE_FDIV) {
			info->q |= 8|1;
			info->pqinst = 0;
		}

		if (lregs->VIwrite & (1 << REG_P)) {
			assert( VU == &VU1 );
			info->p |= 4;
			info->cycles = lregs->cycles;

//			switch( VU->code & 0xff ) {
//				case 0xfd: info->pqinst = 0; break; //eatan
//				case 0x7c: info->pqinst = 0; break; //eatanxy
//				case 0x7d: info->pqinst = 0; break; //eatanzy
//				case 0xfe: info->pqinst = 1; break; //eexp
//				case 0xfc: info->pqinst = 2; break; //esin
//				case 0x3f: info->pqinst = 3; break; //erleng
//				case 0x3e: info->pqinst = 4; break; //eleng
//				case 0x3d: info->pqinst = 4; break; //ersadd
//				case 0xbd: info->pqinst = 4; break; //ersqrt
//				case 0xbe: info->pqinst = 5; break; //ercpr
//				case 0xbc: info->pqinst = 5; break; //esqrt
//				case 0x7e: info->pqinst = 5; break; //esum
//				case 0x3c: info->pqinst = 6; break; //esadd
//				default: assert(0);
//			}
		}
		else if (lregs->pipe == VUPIPE_EFU) {
			info->p |= 8|1;
		}

//		if (lregs->VIread & (1 << REG_CLIP_FLAG)) {
//			info->clipflag|= 2;
//		}
//
//		if (lregs->VIread & (1 << REG_STATUS_FLAG)) {
//			info->statusflag|= 2;
//		}
//
//		if (lregs->VIread & (1 << REG_MAC_FLAG)) {
//			info->macflag|= 2;
//		}

		_recvuAddLowerStalls(VU, lregs);
	}
	_recvuAddUpperStalls(VU, uregs);

	_recvuTestPipes(VU);

	vucycle++;
}

// Analyze an op - first pass
void _vurecAnalyzeOp(VURegs *VU, _vuopinfo *info) {
	_VURegsNum lregs;
	_VURegsNum uregs;
	int *ptr; 

//	SysPrintf("_vurecAnalyzeOp %x; %p\n", pc, info);
	ptr = (int*)&VU->Micro[pc]; 
	pc += 8; 

/*	SysPrintf("_vurecAnalyzeOp Upper: %s\n", disVU1MicroUF( ptr[1], pc ) );
	if ((ptr[1] & 0x80000000) == 0) {
		SysPrintf("_vurecAnalyzeOp Lower: %s\n", disVU1MicroLF( ptr[0], pc ) );
	}*/
	if (ptr[1] & 0x40000000) {
		branch |= 8; 
	} 
 
	VU->code = ptr[1];
	if (VU == &VU1) {
		VU1regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	} else {
		VU0regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	}

	_recvuTestUpperStalls(VU, &uregs);
	switch(VU->code & 0x3f) {
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x1d: case 0x1f:
		case 0x2b: case 0x2f:
			break;

		case 0x3c:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag|= 4;
					info->macflag|= 4;
					break;
			}
			break;
		case 0x3d:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7:
					break;
				default:
					info->statusflag|= 4;
					info->macflag|= 4;
					break;
			}
			break;
		case 0x3e:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					info->statusflag|= 4;
					info->macflag|= 4;
					break;
			}
			break;
		case 0x3f:
			switch ((VU->code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7: case 0xb:
					break;
				default:
					info->statusflag|= 4;
					info->macflag|= 4;
					break;
			}
			break;

		default:
			info->statusflag|= 4;
			info->macflag|= 4;
			break;
	}
//SysPrintf("%x\n", info->macflag);

	if (uregs.VIwrite & (1 << REG_CLIP_FLAG)) {
		info->clipflag |= 4;
	}

	if (uregs.VIread & (1 << REG_Q)) {
		info->q |= 2;
	}

	if (uregs.VIread & (1 << REG_P)) {
		assert( VU == &VU1 );
		info->p |= 2;
	}

	/* check upper flags */ 
	if (ptr[1] & 0x80000000) { /* I flag */ 
		info->cycle = vucycle;

	} else {

		VU->code = ptr[0]; 
		if (VU == &VU1) {
			VU1regs_LOWER_OPCODE[VU->code >> 25](&lregs);
		} else {
			VU0regs_LOWER_OPCODE[VU->code >> 25](&lregs);
		}

		_recvuTestLowerStalls(VU, &lregs);
		info->cycle = vucycle;

		if (lregs.pipe == VUPIPE_BRANCH) {
			branch |= 1;
		}

		if (lregs.VIwrite & (1 << REG_Q)) {
//			SysPrintf("write to Q\n");
			info->q |= 4;
			info->cycles = lregs.cycles;
		}
		else if (lregs.pipe == VUPIPE_FDIV) {
			info->q |= 8|1;
		}

		if (lregs.VIwrite & (1 << REG_P)) {
//			SysPrintf("write to P\n");
			info->p |= 4;
			info->cycles = lregs.cycles;
		}
		else if (lregs.pipe == VUPIPE_EFU) {
			assert( VU == &VU1 );
			info->p |= 8|1;
		}

		if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
			info->clipflag|= 2;
		}

		if (lregs.VIread & (1 << REG_STATUS_FLAG)) {
			info->statusflag|= 2;
		}

		if (lregs.VIread & (1 << REG_MAC_FLAG)) {
			info->macflag|= 2;
		}

		_recvuAddLowerStalls(VU, &lregs);
	}
	_recvuAddUpperStalls(VU, &uregs);

	_recvuTestPipes(VU);

	vucycle++;
}

/* Analyze a block - second pass */
void _vurecAnalyzeBlock(VURegs *VU, _vuopinfo *info, int count) {
	int i, j;
	int qcycle=-1;
	int pcycle=-1;
	int** cyclearray;

	if (VU == &VU0) cyclearray = (int**)&recVU0cycles[VU0.VI[ REG_TPC ].UL];
	else cyclearray = (int**)&recVU1cycles[VU1.VI[ REG_TPC ].UL];
	
	if (*cyclearray != 0) free((void*)*cyclearray);
	*cyclearray = (int*)malloc(count*sizeof(int));
	assert(*cyclearray);

	for (i=0; i<count; i++) {
//		SysPrintf("_vurecAnalyzeBlock %x: cycle=%d\n", pc, info[i].cycle);

		/*if (info[i].statusflag & 2) {
			for (j=i-1; j>=0; j--) {
				if (info[j].statusflag & 4 &&
					info[j].cycle <= (info[i-1].cycle-3)) break;
			}
			if (j>=0) info[j].statusflag|= 1;
		}*/

		if (info[i].macflag & 2) {
			for (j=i-1; j>=0; j--) {
				if (info[j].macflag & 4 &&
					info[j].cycle <= (info[i-1].cycle-3)) break;
			}
			if (j>=0) info[j].macflag|= 1;
		}

		if (qcycle != -1 &&
			info[i].q == 1) {
			qcycle = -1;
		}

		if (info[i].q &4) {
//			SysPrintf("write at q\n");
			if (qcycle != -1) {
				info[i-1].q |= 8;
			}
			qcycle = info[i].cycle + info[i].cycles;
		}

		if (qcycle != -1 &&
			info[i].cycle >= qcycle) {
///			SysPrintf("flush q\n");
			qcycle = -1;
			info[i-1].q |= 8;
		}

		if (pcycle != -1 &&
			info[i].p == 1) {
			assert( VU == &VU1 );
			pcycle = -1;
		}

		if (info[i].p & 4) {
			if (pcycle != -1) {
				assert( VU == &VU1 );
				info[i-1].p |= 8;
			}
			pcycle = info[i].cycle + info[i].cycles;
		}

		if (pcycle != -1 && info[i].cycle >= pcycle) {
			assert( VU == &VU1 );
			pcycle = -1;
			info[i-1].p |= 8;
		}

		if (i==0)
			(*cyclearray)[i] = info[i].cycle - ( vucycleold - 1 );
		else
			(*cyclearray)[i] = info[i].cycle - info[i-1].cycle;
	}

	for (i=count-1; i>=0; i--) {
		if (info[i].cycle >= (info[count-1].cycle-3)) {
			if (info[i].macflag & 4) info[i].macflag |= 1;
		} else {
			if (info[i].macflag & 4) {
				info[i].macflag |= 1;
				break;
			}
		}
	}

	if (qcycle != -1 && info[count-1].cycle+1 >= qcycle) {
		info[count-1].q |= 8;
		qcycle = -1;
	}

	if (VU == &VU0) {
		if (qcycle != -1)
			*(int*)&recVU0Q[VU0.VI[ REG_TPC ].UL] = qcycle - info[count-1].cycle;
		else *(int*)&recVU0Q[VU0.VI[ REG_TPC ].UL] = 0;
	}
	else {
		if (qcycle != -1)
			*(int*)&recVU1Q[VU1.VI[ REG_TPC ].UL] = qcycle - info[count-1].cycle;
		else *(int*)&recVU1Q[VU1.VI[ REG_TPC ].UL] = 0;
	}

	if (pcycle != -1 && info[count-1].cycle+1 >= pcycle) {
		assert( VU == &VU1 );
		info[count-1].p |= 8;
		pcycle = -1;
	}

	if( VU == &VU1 ) {
		if (pcycle != -1) {
			*(int*)&recVU1P[VU1.VI[ REG_TPC ].UL] = pcycle - info[count-1].cycle;
		}
		else 
			*(int*)&recVU1P[VU1.VI[ REG_TPC ].UL] = 0;
	}
}

/******************************/
/*        VU Flags            */
/******************************/

void recResetFlags( void ) {
}

static __declspec(align(16)) u32 s_FloatMinMax[] = {
	0x007fffff, 0x007fffff, 0x007fffff, 0x007fffff,
	0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff };

void recVU_MAC_UPDATE(VURegs * VU, int reg, int xyzw) {
	int t0reg = _allocTempXMMreg(-1);
	int t1reg;

	if (xyzw != 0) t1reg = _allocTempXMMreg(-1);

	if(cpucaps.hasStreamingSIMD2Extensions) 
	{
		switch(xyzw) {
			case 0:
				SSE2_MOVD_XMM_to_R32(EAX, reg);
				break;
			case 1:
				SSE2_PSHUFD_XMM_to_XMM(t0reg, reg, 0x55);
				SSE2_MOVD_XMM_to_R32(EAX, t0reg);
				break;
			case 2:
				SSE2_PSHUFD_XMM_to_XMM(t0reg, reg, 0xaa);
				SSE2_MOVD_XMM_to_R32(EAX, t0reg);
				break;
			case 3:
				SSE2_PSHUFD_XMM_to_XMM(t0reg, reg, 0xff);
				SSE2_MOVD_XMM_to_R32(EAX, t0reg);
				break;
		}
	} else {
		switch(xyzw) {
			case 0:
				SSE2EMU_MOVD_XMM_to_R32(EAX, reg);
				break;
			case 1:
				SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, reg, 0x55);
				SSE2EMU_MOVD_XMM_to_R32(EAX, t0reg);
				break;
			case 2:
				SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, reg, 0xaa);
				SSE2EMU_MOVD_XMM_to_R32(EAX, t0reg);
				break;
			case 3:
				SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, reg, 0xff);
				SSE2EMU_MOVD_XMM_to_R32(EAX, t0reg);
				break;
		}
	}

	MOV32RtoR(ECX, EAX);
	AND32ItoR(ECX, 0x7f800000);

	// overflow
	CMP32ItoR(ECX, 0x7f800000);
	j8Ptr[ 0 ] = JNE8( 0 );

	// if exp==256, set overflow
	OR32ItoR(EDX, 1<<(15-xyzw));

	// sign
	MOV32RtoR(ECX, EAX);
	SHR32ItoR(EAX, 31);
	SHL32ItoR(EAX, 4+(3-xyzw));
	OR32RtoR(EDX, EAX);

	j8Ptr[ 1 ] = JMP8( 0 );

	// else
	x86SetJ8( j8Ptr[ 0 ] );

	// underflow
	CMP32ItoR(ECX, 0);
	j8Ptr[ 2 ] = JNE8( 0 );

	// set zero, underflow and sign
	OR32ItoR(EDX, 1<<(3-xyzw));

	// sign
	MOV32RtoR(ECX, EAX);
	SHR32ItoR(EAX, 31);
	SHL32ItoR(EAX, 4+(3-xyzw));
	OR32RtoR(EDX, EAX);

	AND32RtoR(ECX, 0x7fffffff);
	CMP32ItoR(ECX, 0);

	j8Ptr[ 4 ] = JE8( 0 );
	OR32ItoR(EDX, 1<<(11-xyzw));
	j8Ptr[ 3 ] = JMP8( 0 );

	// else
	x86SetJ8( j8Ptr[ 2 ] );

	// sign
	SHR32ItoR(EAX, 31);
	SHL32ItoR(EAX, 4+((3-xyzw)));
	OR32RtoR(EDX, EAX);

	// zero
	if (xyzw == 0) {
		SSE_XORPS_XMM_to_XMM(t0reg, t0reg);
		SSE_CMPEQSS_XMM_to_XMM(t0reg, reg);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_MOVD_XMM_to_R32(EAX, t0reg);
		else SSE2EMU_MOVD_XMM_to_R32(EAX, t0reg);
	} else {
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg);
		SSE_CMPEQSS_XMM_to_XMM(t1reg, t0reg);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_MOVD_XMM_to_R32(EAX, t1reg);
		else SSE2EMU_MOVD_XMM_to_R32(EAX, t1reg);
	}

	AND32ItoR(EAX, 0x1);
	SHL32ItoR(EAX, 0+(3-xyzw));
	OR32RtoR(EDX, EAX);

	x86SetJ8( j8Ptr[ 1 ] );
	x86SetJ8( j8Ptr[ 3 ] );
	x86SetJ8( j8Ptr[ 4 ] );

	_freeXMMreg(t0reg);
	if (xyzw != 0) _freeXMMreg(t1reg);
}

void recVU_MAC_UPDATExyzw(VURegs * VU, int reg) {
	if (_X) { recVU_MAC_UPDATE(VU, reg, 0); }
	if (_Y) { recVU_MAC_UPDATE(VU, reg, 1); }
	if (_Z) { recVU_MAC_UPDATE(VU, reg, 2); }
	if (_W) { recVU_MAC_UPDATE(VU, reg, 3); }
}

// if macmode == 0, read from _X_Y_Z_W,
// if macmode == 1, only update xyz (regardless of _X_Y_Z_W)
// if macmode == 2, only update status flags
void recVUMI_STAT_REG_UPDATE(VURegs *VU, int reg, int macmode) {
	XOR32RtoR(EDX, EDX);

	switch(macmode) {
		case 0:
			if (_X_Y_Z_W == 0xf) {
				recVU_MAC_UPDATExyzw(VU, reg);
			} else {
				if (_X) { recVU_MAC_UPDATE(VU, reg, 0); }
				if (_Y) { recVU_MAC_UPDATE(VU, reg, 1); }
				if (_Z) { recVU_MAC_UPDATE(VU, reg, 2); }
				if (_W) { recVU_MAC_UPDATE(VU, reg, 3); }
			}

			MOV32RtoM((uptr)&VU->macflag, EDX);
			break;

		case 1:
			recVU_MAC_UPDATE(VU, reg, 0);
			recVU_MAC_UPDATE(VU, reg, 1);
			recVU_MAC_UPDATE(VU, reg, 2);

			MOV32RtoM((uptr)&VU->macflag, EDX);
			break;

		case 2:
			MOV32MtoR(EDX, (uptr)&VU->macflag);
			break;

		default: __assume(0);
	}

	SSE_MINPS_M128_to_XMM(reg, (int)(s_FloatMinMax+4));

	MOV32MtoR(ECX, (uptr)&VU->statusflag);
	MOV32RtoR(EAX, ECX);
	AND32ItoR(ECX, 0xC30);
	AND32ItoR(EAX, 0xF);
	SHL32ItoR(EAX, 6);
	OR32RtoR(ECX, EAX);

	TEST32ItoR(EDX, 0x000F);
	j8Ptr[0] = JZ8(0);
	OR32ItoR(ECX, (1<<0));
	x86SetJ8(j8Ptr[0]); 

	TEST32ItoR(EDX, 0x00F0);
	j8Ptr[0] = JZ8(0);
	OR32ItoR(ECX, (1<<1));
	x86SetJ8(j8Ptr[0]); 

	TEST32ItoR(EDX, 0x0F00);
	j8Ptr[0] = JZ8(0);
	OR32ItoR(ECX, (1<<2));
	x86SetJ8(j8Ptr[0]); 

	TEST32ItoR(EDX, 0xF000);
	j8Ptr[0] = JZ8(0);
	OR32ItoR(ECX, (1<<3));
	x86SetJ8(j8Ptr[0]); 

	MOV32RtoM((uptr)&VU->statusflag, ECX);
}


/******************************/
/*   VU Upper instructions    */
/******************************/

static __declspec(align(16)) int const_abs_table[16][4] = 
{
   { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
   { 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff },
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0xffffffff },
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0x7fffffff },
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0xffffffff },
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0x7fffffff },
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0xffffffff },
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff },
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff },
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0x7fffffff },
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff },
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0x7fffffff },
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0xffffffff },
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0x7fffffff },
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0xffffffff },
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff },
};


void _vurecABS(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);

	SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
	
	if(cpucaps.hasStreamingSIMD2Extensions) {
		// this is the cheapest way to do abs... but it's fast
		SSE2_PSLLD_I8_to_XMM(t0reg, 1);
		SSE2_PSRLD_I8_to_XMM(t0reg, 1);
	}
	else {
		SSE_ANDPS_M128_to_XMM(t0reg, (int)&const_abs_table[ _X_Y_Z_W ][ 0 ] );
	}

	VU_MERGE_REGS(ftreg, t0reg);
}

void _vurecABSxyzw(VURegs *VU) {
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);

	if(cpucaps.hasStreamingSIMD2Extensions) {
		// this is the cheapest way to do abs... but it's fast
		if (_Ft_ == _Fs_) {
			SSE2_PSLLD_I8_to_XMM(ftreg, 1);
			SSE2_PSRLD_I8_to_XMM(ftreg, 1);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(ftreg, fsreg);
			SSE2_PSLLD_I8_to_XMM(ftreg, 1);
			SSE2_PSRLD_I8_to_XMM(ftreg, 1);
		}
	}
	else {
		if( _Ft_ == _Fs_ ) {
			SSE_ANDPS_M128_to_XMM(ftreg, (int)&const_abs_table[ _X_Y_Z_W ][ 0 ] );
		} else {
			SSE_MOVAPS_XMM_to_XMM(ftreg, fsreg);
			SSE_ANDPS_M128_to_XMM(ftreg, (int)&const_abs_table[ _X_Y_Z_W ][ 0 ] );
		}
	}
}

void recVUMI_ABS(VURegs *VU) {
    if ( _Ft_ == 0 ) return;

	if (_X_Y_Z_W != 0xf) {
		_vurecABS(VU);
	} else {
		_vurecABSxyzw(VU);
	}

	_clearNeededXMMregs();
}

void recVUMI_ADD(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_ADDPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_ADDPS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (fdreg == ftreg) {
			SSE_ADDPS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_ADDPS_XMM_to_XMM(fdreg, ftreg);
		}
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_ADD_iq(VURegs *VU, int addr) {
	int t0reg;
	int fsreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_ADDPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_ADDPS_XMM_to_XMM(fdreg, t0reg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_ADDPS_XMM_to_XMM(fdreg, t0reg);
		}

		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_ADD_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_ADDPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_ADDPS_XMM_to_XMM(fdreg, t0reg);

		_freeXMMreg(t0reg);
	}
	
	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_ADDi(VURegs *VU) { recVUMI_ADD_iq(VU, VU_REGI_ADDR); }
void recVUMI_ADDq(VURegs *VU) { recVUMI_ADD_iq(VU, VU_REGQ_ADDR); }
void recVUMI_ADDx(VURegs *VU) { recVUMI_ADD_xyzw(VU, 0); }
void recVUMI_ADDy(VURegs *VU) { recVUMI_ADD_xyzw(VU, 1); }
void recVUMI_ADDz(VURegs *VU) { recVUMI_ADD_xyzw(VU, 2); }
void recVUMI_ADDw(VURegs *VU) { recVUMI_ADD_xyzw(VU, 3); }

void recVUMI_ADDA(VURegs *VU) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_ADDPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);
		SSE_MOVAPS_XMM_to_XMM(accreg, fsreg);
		SSE_ADDPS_XMM_to_XMM(accreg, ftreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_ADDA_iq(VURegs *VU, int addr) {
	int t0reg;
	int accreg;
	int fsreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);

		SSE_MOVSS_M32_to_XMM(t0reg, addr); 
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
		else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

		SSE_ADDPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);

		SSE_MOVSS_M32_to_XMM(accreg, addr); 
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(accreg, accreg, 0x00);
		else SSE2EMU_PSHUFD_XMM_to_XMM(accreg, accreg, 0x00);

		SSE_ADDPS_XMM_to_XMM(accreg, fsreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_ADDAi(VURegs *VU) { recVUMI_ADDA_iq(VU, VU_REGI_ADDR); }
void recVUMI_ADDAq(VURegs *VU) { recVUMI_ADDA_iq(VU, VU_REGQ_ADDR); }

void recVUMI_ADDA_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);
	SSE_ADDPS_XMM_to_XMM(t0reg, fsreg);

	if (_X_Y_Z_W != 0xf) {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);
		SSE_MOVAPS_XMM_to_XMM(accreg, t0reg);
		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_ADDAx(VURegs *VU) { recVUMI_ADDA_xyzw(VU, 0); }
void recVUMI_ADDAy(VURegs *VU) { recVUMI_ADDA_xyzw(VU, 1); }
void recVUMI_ADDAz(VURegs *VU) { recVUMI_ADDA_xyzw(VU, 2); }
void recVUMI_ADDAw(VURegs *VU) { recVUMI_ADDA_xyzw(VU, 3); }

void recVUMI_SUB(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_SUBPS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (fdreg == ftreg) {
			t0reg = _allocTempXMMreg(-1);

			SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
			SSE_SUBPS_XMM_to_XMM(t0reg, fdreg);
			SSE_MOVAPS_XMM_to_XMM(fdreg, t0reg);

			_freeXMMreg(t0reg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_SUBPS_XMM_to_XMM(fdreg, ftreg);
		}
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);

	_clearNeededXMMregs();
}

void recVUMI_SUB_iq(VURegs *VU, int addr) {
	int t0reg;
	int t1reg;
	int fsreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(fdreg, t1reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_SUBPS_XMM_to_XMM(fdreg, t0reg);
	}

	_freeXMMreg(t0reg);

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_SUB_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int t1reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(fdreg, t1reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_SUBPS_XMM_to_XMM(fdreg, t0reg);
	}

	_freeXMMreg(t0reg);

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_SUBi(VURegs *VU) { recVUMI_SUB_iq(VU, VU_REGI_ADDR); }
void recVUMI_SUBq(VURegs *VU) { recVUMI_SUB_iq(VU, VU_REGQ_ADDR); }
void recVUMI_SUBx(VURegs *VU) { recVUMI_SUB_xyzw(VU, 0); }
void recVUMI_SUBy(VURegs *VU) { recVUMI_SUB_xyzw(VU, 1); }
void recVUMI_SUBz(VURegs *VU) { recVUMI_SUB_xyzw(VU, 2); }
void recVUMI_SUBw(VURegs *VU) { recVUMI_SUB_xyzw(VU, 3); }

void recVUMI_SUBA(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int accreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(accreg, fsreg);
		SSE_SUBPS_XMM_to_XMM(accreg, ftreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_SUBA_iq(VURegs *VU, int addr) {
	int t0reg;
	int t1reg;
	int fsreg;
	int accreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(accreg, t1reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(accreg, fsreg);
		SSE_SUBPS_XMM_to_XMM(accreg, t0reg);
	}

	_freeXMMreg(t0reg);

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_SUBA_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int t1reg;
	int fsreg;
	int ftreg;
	int accreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, fsreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(accreg, t1reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(accreg, fsreg);
		SSE_SUBPS_XMM_to_XMM(accreg, t0reg);
	}

	_freeXMMreg(t0reg);

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_SUBAi(VURegs *VU) { recVUMI_SUBA_iq(VU, VU_REGI_ADDR); }
void recVUMI_SUBAq(VURegs *VU) { recVUMI_SUBA_iq(VU, VU_REGQ_ADDR); }
void recVUMI_SUBAx(VURegs *VU) { recVUMI_SUBA_xyzw(VU, 0); }
void recVUMI_SUBAy(VURegs *VU) { recVUMI_SUBA_xyzw(VU, 1); }
void recVUMI_SUBAz(VURegs *VU) { recVUMI_SUBA_xyzw(VU, 2); }
void recVUMI_SUBAw(VURegs *VU) { recVUMI_SUBA_xyzw(VU, 3); }

void recVUMI_MUL(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_MULPS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (fdreg == ftreg) {
			SSE_MULPS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULPS_XMM_to_XMM(fdreg, ftreg);
		}
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MUL_iq(VURegs *VU, int addr) {
	int t0reg;
	int fsreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		}

		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MUL_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == fsreg) {
			SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		} else {
			SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		}

		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MULi(VURegs *VU) { recVUMI_MUL_iq(VU, VU_REGI_ADDR); }
void recVUMI_MULq(VURegs *VU) { recVUMI_MUL_iq(VU, VU_REGQ_ADDR); }
void recVUMI_MULx(VURegs *VU) { recVUMI_MUL_xyzw(VU, 0); }
void recVUMI_MULy(VURegs *VU) { recVUMI_MUL_xyzw(VU, 1); }
void recVUMI_MULz(VURegs *VU) { recVUMI_MUL_xyzw(VU, 2); }
void recVUMI_MULw(VURegs *VU) { recVUMI_MUL_xyzw(VU, 3); }

void recVUMI_MULA( VURegs *VU ) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);
		SSE_MOVAPS_XMM_to_XMM(accreg, fsreg);
		SSE_MULPS_XMM_to_XMM(accreg, ftreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MULA_iq(VURegs *VU, int addr) {
	int t0reg;
	int accreg;
	int fsreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);

		SSE_MOVSS_M32_to_XMM(t0reg, addr); 
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
		else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);

		SSE_MOVSS_M32_to_XMM(accreg, addr); 
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(accreg, accreg, 0x00);
		else SSE2EMU_PSHUFD_XMM_to_XMM(accreg, accreg, 0x00);

		SSE_MULPS_XMM_to_XMM(accreg, fsreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MULAi(VURegs *VU) { recVUMI_MULA_iq(VU, VU_REGI_ADDR); }
void recVUMI_MULAq(VURegs *VU) { recVUMI_MULA_iq(VU, VU_REGQ_ADDR); }

void recVUMI_MULA_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		_unpackVF_xyzw(t0reg, ftreg, xyzw);
		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE);
		_unpackVF_xyzw(accreg, ftreg, xyzw);
		SSE_MULPS_XMM_to_XMM(accreg, fsreg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	
	_clearNeededXMMregs();
}

void recVUMI_MULAx(VURegs *VU) { recVUMI_MULA_xyzw(VU, 0); }
void recVUMI_MULAy(VURegs *VU) { recVUMI_MULA_xyzw(VU, 1); }
void recVUMI_MULAz(VURegs *VU) { recVUMI_MULA_xyzw(VU, 2); }
void recVUMI_MULAw(VURegs *VU) { recVUMI_MULA_xyzw(VU, 3); }

void recVUMI_MADD(VURegs *VU) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_XMM_to_XMM(t0reg, ftreg);
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (fdreg == ftreg) {
			t0reg = _allocTempXMMreg(-1);

			SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
			SSE_MULPS_XMM_to_XMM(t0reg, ftreg);
			SSE_ADDPS_XMM_to_XMM(t0reg, accreg);
			SSE_MOVAPS_XMM_to_XMM(fdreg, t0reg);

			_freeXMMreg(t0reg);
		}
		else {
			if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULPS_XMM_to_XMM(fdreg, ftreg);
			SSE_ADDPS_XMM_to_XMM(fdreg, accreg);
		}
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MADD_iq(VURegs *VU, int addr) {
	int t0reg;
	int accreg;
	int fsreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		SSE_ADDPS_XMM_to_XMM(fdreg, accreg);

		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MADD_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		if (_Fd_ == 0 || _Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MULPS_XMM_to_XMM(fdreg, t0reg);
		SSE_ADDPS_XMM_to_XMM(fdreg, accreg);
		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MADDi(VURegs *VU) { recVUMI_MADD_iq(VU, VU_REGI_ADDR); }
void recVUMI_MADDq(VURegs *VU) { recVUMI_MADD_iq(VU, VU_REGQ_ADDR); }
void recVUMI_MADDx(VURegs *VU) { recVUMI_MADD_xyzw(VU, 0); }
void recVUMI_MADDy(VURegs *VU) { recVUMI_MADD_xyzw(VU, 1); }
void recVUMI_MADDz(VURegs *VU) { recVUMI_MADD_xyzw(VU, 2); }
void recVUMI_MADDw(VURegs *VU) { recVUMI_MADD_xyzw(VU, 3); }

void recVUMI_MADDA( VURegs *VU ) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);

	SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
	SSE_MULPS_XMM_to_XMM(t0reg, ftreg);

	if (_X_Y_Z_W != 0xf) {
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);
		VU_MERGE_REGS(accreg, t0reg);
	} else {
		SSE_ADDPS_XMM_to_XMM(accreg, t0reg);
		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MADDA_iq( VURegs *VU, int addr) {
	int t0reg;
	int accreg;
	int fsreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

	if (_X_Y_Z_W != 0xf) {
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);

		VU_MERGE_REGS(accreg, t0reg);
	} else {
		SSE_ADDPS_XMM_to_XMM(accreg, t0reg);
		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MADDAi( VURegs *VU ) { recVUMI_MADDA_iq( VU, VU_REGI_ADDR ); }
void recVUMI_MADDAq( VURegs *VU ) { recVUMI_MADDA_iq( VU, VU_REGQ_ADDR ); }

void recVUMI_MADDA_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);
	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

	if (_X_Y_Z_W != 0xf) {
		SSE_ADDPS_XMM_to_XMM(t0reg, accreg);
		VU_MERGE_REGS(accreg, t0reg);
	} else {
		SSE_ADDPS_XMM_to_XMM(accreg, t0reg);
		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MADDAx( VURegs *VU ) { recVUMI_MADDA_xyzw(VU, 0); }
void recVUMI_MADDAy( VURegs *VU ) { recVUMI_MADDA_xyzw(VU, 1); }
void recVUMI_MADDAz( VURegs *VU ) { recVUMI_MADDA_xyzw(VU, 2); }
void recVUMI_MADDAw( VURegs *VU ) { recVUMI_MADDA_xyzw(VU, 3); }

void recVUMI_MSUB(VURegs *VU) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_XMM_to_XMM(t0reg, ftreg);
		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(fdreg, t1reg);
		_freeXMMreg(t0reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_XMM_to_XMM(t0reg, ftreg);
		SSE_MOVAPS_XMM_to_XMM(fdreg, accreg);
		SSE_SUBPS_XMM_to_XMM(fdreg, t0reg);

		_freeXMMreg(t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MSUB_iq(VURegs *VU, int addr) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(fdreg, t1reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MOVAPS_XMM_to_XMM(fdreg, accreg);
		SSE_SUBPS_XMM_to_XMM(fdreg, t0reg);
	}

	_freeXMMreg(t0reg);
	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MSUB_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;
	int ftreg;
	int fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	if (_Fd_) _addNeededVFtoXMMreg(_Fd_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(fdreg, t1reg);
	} else {
		if (_Fd_) fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);
		else fdreg = _allocTempXMMreg(-1);

		SSE_MULPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MOVAPS_XMM_to_XMM(fdreg, accreg);
		SSE_SUBPS_XMM_to_XMM(fdreg, t0reg);
	}

	_freeXMMreg(t0reg);
	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 0);
	}
	if (_Fd_ == 0) _freeXMMreg(fdreg);
	_clearNeededXMMregs();
}

void recVUMI_MSUBi(VURegs *VU) { recVUMI_MSUB_iq(VU, VU_REGI_ADDR); }
void recVUMI_MSUBq(VURegs *VU) { recVUMI_MSUB_iq(VU, VU_REGQ_ADDR); }
void recVUMI_MSUBx(VURegs *VU) { recVUMI_MSUB_xyzw(VU, 0); }
void recVUMI_MSUBy(VURegs *VU) { recVUMI_MSUB_xyzw(VU, 1); }
void recVUMI_MSUBz(VURegs *VU) { recVUMI_MSUB_xyzw(VU, 2); }
void recVUMI_MSUBw(VURegs *VU) { recVUMI_MSUB_xyzw(VU, 3); }

void recVUMI_MSUBA( VURegs *VU ) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);

	SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
	SSE_MULPS_XMM_to_XMM(t0reg, ftreg);

	if (_X_Y_Z_W != 0xf) {
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(accreg, t1reg);
	} else {
		SSE_SUBPS_XMM_to_XMM(accreg, t0reg);
	}

	_freeXMMreg(t0reg);
	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MSUBA_iq( VURegs *VU, int addr) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

	if (_X_Y_Z_W != 0xf) {
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(accreg, t1reg);
	} else {
		SSE_SUBPS_XMM_to_XMM(accreg, t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MSUBA_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int t1reg;
	int accreg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ | MODE_WRITE);
	t0reg = _allocTempXMMreg(-1);
	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	SSE_MULPS_XMM_to_XMM(t0reg, fsreg);

	if (_X_Y_Z_W != 0xf) {
		t1reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
		SSE_SUBPS_XMM_to_XMM(t1reg, t0reg);

		VU_MERGE_REGS(accreg, t1reg);
	} else {
		SSE_SUBPS_XMM_to_XMM(accreg, t0reg);
	}

	if (cinfo->statusflag & 0x1 ||
		cinfo->macflag & 0x1) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 0);
	}
	_clearNeededXMMregs();
}

void recVUMI_MSUBAi( VURegs *VU ) { recVUMI_MSUBA_iq( VU, VU_REGI_ADDR ); }
void recVUMI_MSUBAq( VURegs *VU ) { recVUMI_MSUBA_iq( VU, VU_REGQ_ADDR ); }
void recVUMI_MSUBAx( VURegs *VU ) { recVUMI_MSUBA_xyzw(VU, 0); }
void recVUMI_MSUBAy( VURegs *VU ) { recVUMI_MSUBA_xyzw(VU, 1); }
void recVUMI_MSUBAz( VURegs *VU ) { recVUMI_MSUBA_xyzw(VU, 2); }
void recVUMI_MSUBAw( VURegs *VU ) { recVUMI_MSUBA_xyzw(VU, 3); }

void recVUMI_MAX(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededVFtoXMMreg(_Fd_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MAXPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Ft_) {
			t0reg = _allocTempXMMreg(-1);
			SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
			SSE_MAXPS_XMM_to_XMM(t0reg, ftreg);
			SSE_MOVAPS_XMM_to_XMM(fdreg, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MAXPS_XMM_to_XMM(fdreg, ftreg);
		}
	}

	_clearNeededXMMregs();
}

void recVUMI_MAX_iq(VURegs *VU, int addr) {
	int t0reg;
	int fsreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);

		SSE_MAXPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MAXPS_XMM_to_XMM(fdreg, t0reg);
		_freeXMMreg(t0reg);
	}

	_clearNeededXMMregs();
}

void recVUMI_MAX_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);

		SSE_MAXPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MAXPS_XMM_to_XMM(fdreg, t0reg);
		_freeXMMreg(t0reg);
	}

	_clearNeededXMMregs();
}

void recVUMI_MAXi(VURegs *VU) { recVUMI_MAX_iq(VU, VU_REGI_ADDR); }
void recVUMI_MAXx(VURegs *VU) { recVUMI_MAX_xyzw(VU, 0); }
void recVUMI_MAXy(VURegs *VU) { recVUMI_MAX_xyzw(VU, 1); }
void recVUMI_MAXz(VURegs *VU) { recVUMI_MAX_xyzw(VU, 2); }
void recVUMI_MAXw(VURegs *VU) { recVUMI_MAX_xyzw(VU, 3); }

void recVUMI_MINI(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededVFtoXMMreg(_Fd_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MINPS_XMM_to_XMM(t0reg, ftreg);

		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Ft_) {
			t0reg = _allocTempXMMreg(-1);
			SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
			SSE_MINPS_XMM_to_XMM(t0reg, ftreg);
			SSE_MOVAPS_XMM_to_XMM(fdreg, t0reg);
			_freeXMMreg(t0reg);
		}
		else {
			if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
			SSE_MINPS_XMM_to_XMM(fdreg, ftreg);
		}
	}

	_clearNeededXMMregs();
}

void recVUMI_MINI_iq(VURegs *VU, int addr) {
	int t0reg;
	int fsreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVSS_M32_to_XMM(t0reg, addr); 
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);

		SSE_MINPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MINPS_XMM_to_XMM(fdreg, t0reg);
		_freeXMMreg(t0reg);
	}

	_clearNeededXMMregs();
}

void recVUMI_MINI_xyzw(VURegs *VU, int xyzw) {
	int t0reg;
	int fsreg;
	int ftreg;
	int fdreg;

	if ( _Fd_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededVFtoXMMreg(_Fd_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	_unpackVF_xyzw(t0reg, ftreg, xyzw);

	if (_X_Y_Z_W != 0xf) {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_READ | MODE_WRITE);

		SSE_MINPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(fdreg, t0reg);
	} else {
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE);

		if (_Fd_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(fdreg, fsreg);
		SSE_MINPS_XMM_to_XMM(fdreg, t0reg);
		_freeXMMreg(t0reg);
	}

	_clearNeededXMMregs();
}

void recVUMI_MINIi(VURegs *VU) { recVUMI_MINI_iq(VU, VU_REGI_ADDR); }
void recVUMI_MINIx(VURegs *VU) { recVUMI_MINI_xyzw(VU, 0); }
void recVUMI_MINIy(VURegs *VU) { recVUMI_MINI_xyzw(VU, 1); }
void recVUMI_MINIz(VURegs *VU) { recVUMI_MINI_xyzw(VU, 2); }
void recVUMI_MINIw(VURegs *VU) { recVUMI_MINI_xyzw(VU, 3); }

void recVUMI_OPMULA( VURegs *VU )
{
	int t0reg, t1reg, accreg, fsreg, ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);
	t0reg = _allocTempXMMreg(-1);
	t1reg = _allocTempXMMreg(-1);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_WRITE|MODE_READ);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if(cpucaps.hasStreamingSIMD2Extensions) {
		SSE2_PSHUFD_XMM_to_XMM( t0reg, fsreg, 0xC9 );           // XMM0 = WXZY
		SSE2_PSHUFD_XMM_to_XMM( t1reg, ftreg, 0xD2 );           // XMM1 = WYXZ
	}
	else {
		SSE2EMU_PSHUFD_XMM_to_XMM( t0reg, fsreg, 0xC9 );           // XMM0 = WXZY
		SSE2EMU_PSHUFD_XMM_to_XMM( t1reg, ftreg, 0xD2 );           // XMM1 = WYXZ
	}

	SSE_MULPS_XMM_to_XMM( t0reg, t1reg );
	VU_MERGE_REGS_CUSTOM(accreg, t0reg, 14);

	_freeXMMreg(t1reg);
	if ((cinfo->statusflag & 0x1) || (cinfo->macflag & 0x1) ) {
		recVUMI_STAT_REG_UPDATE(VU, accreg, 1);
	}

	_clearNeededXMMregs();
}

void recVUMI_OPMSUB( VURegs *VU )
{
	int t0reg, t1reg, accreg, fsreg, ftreg, fdreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	_addNeededACCtoXMMreg(VU);

	if (_Fd_) {
		_addNeededVFtoXMMreg(_Fd_);
		fdreg = _allocVFtoXMMreg(VU, -1, _Fd_, MODE_WRITE|MODE_READ);
	}
	else
		fdreg = _allocTempXMMreg(-1);

	t0reg = _allocTempXMMreg(-1);
	t1reg = _allocTempXMMreg(-1);
	accreg = _allocACCtoXMMreg(VU, -1, MODE_READ);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if(cpucaps.hasStreamingSIMD2Extensions) {
		SSE2_PSHUFD_XMM_to_XMM( t0reg, fsreg, 0xC9 );           // XMM0 = WXZY
		SSE2_PSHUFD_XMM_to_XMM( t1reg, ftreg, 0xD2 );           // XMM1 = WYXZ
	}
	else {
		SSE2EMU_PSHUFD_XMM_to_XMM( t0reg, fsreg, 0xC9 );           // XMM0 = WXZY
		SSE2EMU_PSHUFD_XMM_to_XMM( t1reg, ftreg, 0xD2 );           // XMM1 = WYXZ
	}

	SSE_MULPS_XMM_to_XMM( t0reg, t1reg);
	SSE_MOVAPS_XMM_to_XMM(t1reg, accreg);
	SSE_SUBPS_XMM_to_XMM( t1reg, t0reg);
	VU_MERGE_REGS_CUSTOM(fdreg, t1reg, 14);

	_freeXMMreg(t0reg);
	if ((cinfo->statusflag & 0x1) || (cinfo->macflag & 0x1)) {
		recVUMI_STAT_REG_UPDATE(VU, fdreg, 1);
	}

	if( _Fd_ == 0 ) _freeXMMreg(fdreg);

	_clearNeededXMMregs();
}

void recVUMI_NOP( VURegs *VU ) 
{
}

void recVUMI_FTOI0(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;

	if ( _Ft_ == 0 ) return; 

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

        if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTPS2DQ_XMM_to_XMM(t0reg, fsreg);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
        if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTPS2DQ_XMM_to_XMM(ftreg, fsreg);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(ftreg, fsreg);
	}

	_clearNeededXMMregs();
}

void recVUMI_FTOIX(VURegs *VU, int addr)
{
	int t0reg;
	int fsreg;
	int ftreg;

	if ( _Ft_ == 0 ) return; 

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULPS_M128_to_XMM(t0reg, addr);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTPS2DQ_XMM_to_XMM(t0reg, t0reg);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(t0reg, t0reg);

		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
		if (_Ft_ != _Fs_) SSE_MOVAPS_XMM_to_XMM(ftreg, fsreg);
		SSE_MULPS_M128_to_XMM(ftreg, addr);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTPS2DQ_XMM_to_XMM(ftreg, ftreg);
		else SSE2EMU_CVTPS2DQ_XMM_to_XMM(ftreg, ftreg);
	}

	_clearNeededXMMregs();
}

void recVUMI_FTOI4( VURegs *VU ) {
	recVUMI_FTOIX(VU, (int)&recMult_float_to_int4[0]);
}

void recVUMI_FTOI12( VURegs *VU ) {
	recVUMI_FTOIX(VU, (int)&recMult_float_to_int12[0]);
}

void recVUMI_FTOI15( VURegs *VU ) {
	recVUMI_FTOIX(VU, (int)&recMult_float_to_int15[0]);
}

void recVUMI_ITOF0( VURegs *VU )
{
	int t0reg;
	int ftreg;
	int fsreg;
	
	if ( _Ft_ == 0 ) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);

	if (_X_Y_Z_W != 0xf) {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		fsreg = _checkVFtoXMMreg(VU, _Fs_);
		t0reg = _allocTempXMMreg(-1);

		if( fsreg < 0 ) {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
			else SSE2EMU_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
		}
		else {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(t0reg, fsreg);
			else {
				_freeXMMreg(fsreg);
				SSE2EMU_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
			}
		}

		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
		fsreg = _checkVFtoXMMreg(VU, _Fs_);

		if( fsreg < 0 ) {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
			else SSE2EMU_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
		}
		else {
			if( cpucaps.hasStreamingSIMD2Extensions ) SSE2_CVTDQ2PS_XMM_to_XMM(ftreg, fsreg);
			else {
				_freeXMMreg(fsreg);
				SSE2EMU_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
			}
		}
	}

	_clearNeededXMMregs();
}

void recVUMI_ITOFX(VURegs *VU, int addr)
{
	int t0reg, fsreg, ftreg;

	if ( _Ft_ == 0 ) return; 

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);

	if (_X_Y_Z_W != 0xf) {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		fsreg = _checkVFtoXMMreg(VU, _Fs_);
		t0reg = _allocTempXMMreg(-1);

		if( fsreg < 0 ) {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
			else SSE2EMU_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
		}
		else {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(t0reg, fsreg);
			else {
				_freeXMMreg(fsreg);
				SSE2EMU_CVTDQ2PS_M128_to_XMM(t0reg, VU_VFx_ADDR( _Fs_ ));
			}
		}

		SSE_MULPS_M128_to_XMM(t0reg, addr);
		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
		fsreg = _checkVFtoXMMreg(VU, _Fs_);

		if( fsreg < 0 ) {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
			else SSE2EMU_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
		}
		else {
			if(cpucaps.hasStreamingSIMD2Extensions) SSE2_CVTDQ2PS_XMM_to_XMM(ftreg, fsreg);
			else {
				_freeXMMreg(fsreg);
				SSE2EMU_CVTDQ2PS_M128_to_XMM(ftreg, VU_VFx_ADDR( _Fs_ ));
			}
		}

		SSE_MULPS_M128_to_XMM(ftreg, addr);
	}

	_clearNeededXMMregs();
}

void recVUMI_ITOF4( VURegs *VU )
{
	recVUMI_ITOFX(VU, (int)&recMult_int_to_float4[0]);
}

void recVUMI_ITOF12( VURegs *VU )
{
   recVUMI_ITOFX(VU, (int)&recMult_int_to_float12[0]);
}

void recVUMI_ITOF15( VURegs *VU )
{
   recVUMI_ITOFX(VU, (int)&recMult_int_to_float15[0]);
}

static __declspec(align(16)) int const_clip[] = {
	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff,
	0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000 };

void recVUMI_CLIP(VURegs *VU) {
	int t0reg, t1reg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	t0reg = _allocTempXMMreg(-1);
	t1reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);
	
	MOV32MtoR(EAX, (uptr)&VU->clipflag);
	SHL32ItoR(EAX, 6);

	_unpackVF_xyzw(t0reg, ftreg, 3);
	SSE_MOVAPS_XMM_to_XMM(t1reg, t0reg);
	SSE_ANDPS_M128_to_XMM(t0reg, (int)const_clip);
	SSE_ORPS_M128_to_XMM(t1reg, (int)(const_clip+4));
	SSE_CMPLTPS_XMM_to_XMM(t0reg, fsreg);
	SSE_CMPNLEPS_XMM_to_XMM(t1reg, fsreg);

	// shuffle so arranged in -w,+w,-z,+z,-y,+y,-x,+x
	SSE_ANDPS_M128_to_XMM(t0reg, (int)(const_clip+8));
	SSE_ANDPS_M128_to_XMM(t1reg, (int)(const_clip+12));
	SSE_ORPS_XMM_to_XMM(t0reg, t1reg);

	// switch happens in ivu1micro.c
//	if(cpucaps.hasStreamingSIMD2Extensions) {
		SSE2_PACKSSWB_XMM_to_XMM(t0reg, t0reg);

		// extract bit mask
		SSE2_PMOVMSKB_XMM_to_R32(EDX, t0reg);
//	} else {
//		// doesn't work
//		SSE_MOVHLPS_XMM_to_XMM(t1reg, t0reg);
//		PACKSSWBMMXtoMMX(t0reg, t1reg);
//
//		// extract bit mask
//		PMOVMSKBMMXtoR(EDX, t0reg);
//	}

	AND32ItoR(EDX, 0x3f);
	OR32RtoR(EAX, EDX);
	AND32ItoR(EAX, 0xffffff);
	MOV32RtoM((uptr)&VU->clipflag, EAX);
	MOV32RtoM((uptr)&VU0.VI[REG_CLIP_FLAG].UL, EAX);

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_clearNeededXMMregs();
}

/******************************/
/*   VU Lower instructions    */
/******************************/

void recVUMI_DIV(VURegs *VU) {
	int t0reg;
	int fsreg;
	int ftreg;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	t0reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
	ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

	if (_Fsf_ == 0 && _Ftf_ == 0) {
		SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
		SSE_DIVSS_XMM_to_XMM(t0reg, ftreg);
		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_MOVD_XMM_to_M32((int)&VU->q.F, t0reg);
		else SSE2EMU_MOVD_XMM_to_M32((int)&VU->q.F, t0reg);
	} else {
		_unpackVF_xyzw(t0reg, fsreg, _Fsf_);

		SSE2_PSHUFD_XMM_to_XMM(ftreg, ftreg, (0xe4e4>>(2*_Ftf_))&0xff);
		SSE_DIVSS_XMM_to_XMM(t0reg, ftreg);

		// mov back
		SSE2_PSHUFD_XMM_to_XMM(ftreg, ftreg, (0xe4e4>>(8-2*_Ftf_))&0xff);

		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_MOVD_XMM_to_M32((int)&VU->q.F, t0reg);
		else SSE2EMU_MOVD_XMM_to_M32((int)&VU->q.F, t0reg);
	}

	_freeXMMreg(t0reg);
	_clearNeededXMMregs();
}

void recVUMI_SQRT( VURegs *VU ) {
	//_freeXMMregs(VU);
	int ftreg = _checkVFtoXMMreg(VU, _Ft_);

	if( ftreg < 0 ) {
		FLD32((int)&VU->VF[ _Ft_ ].F[ _Ftf_ ]);
		FSQRT();
		FSTP32( (int)&VU->q.F );
	}
	else {
		int t0reg;

		_addNeededVFtoXMMreg(_Ft_);
		t0reg = _allocTempXMMreg(-1);

		// load 0x7fffffff for abs value
		switch( _Ftf_ ) {
			case 0:
				SSE_MOVSS_XMM_to_XMM(t0reg, ftreg);
				break;
			default:
				SSE2_PSHUFD_XMM_to_XMM(t0reg, ftreg, 0x55 * _Ftf_);
				break;
		}

		SSE2_PSLLD_I8_to_XMM(t0reg, 1);
		SSE2_PSRLD_I8_to_XMM(t0reg, 1);

		SSE_SQRTSS_XMM_to_XMM(t0reg, t0reg);
		SSE_MOVSS_XMM_to_M32((int)&VU->q.F, t0reg);

		_freeXMMreg(t0reg);
		_clearNeededXMMregs();
	}
}

static int rsqrttmp;

void recVUMI_RSQRT(VURegs *VU) {
	//_freeXMMregs(VU);
	int fsreg, ftreg;

	fsreg = _checkVFtoXMMreg(VU, _Fs_);
	ftreg = _checkVFtoXMMreg(VU, _Ft_);

	if( ftreg < 0 && fsreg < 0 ) {
		// not loaded, so just do regular assembly
		FLD32((int)&VU->VF[ _Ft_ ].F[ _Ftf_ ]);
		FSQRT( );
		FSTP32( (int)&rsqrttmp );

		MOV32MtoR( EAX, (int)&rsqrttmp );
		OR32RtoR( EAX, EAX );
		j8Ptr[ 0 ] = JE8( 0 );				
		FLD32( (int)&VU->VF[ _Fs_ ].F[ _Fsf_ ] );
		FDIV32( (int)&rsqrttmp );
		FSTP32( (int)&VU->q.F );
		x86SetJ8( j8Ptr[ 0 ] );
	}
	else {
		int t0reg;

		_addNeededVFtoXMMreg(_Ft_);
		_addNeededVFtoXMMreg(_Fs_);

		if( fsreg < 0 ) fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);
		if( ftreg < 0 ) ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ);

		t0reg = _allocTempXMMreg(-1);

		// load 0x7fffffff for abs value
		switch( _Ftf_ ) {
			case 0:
				SSE_MOVSS_XMM_to_XMM(t0reg, ftreg);
				break;
			default:
				SSE2_PSHUFD_XMM_to_XMM(t0reg, ftreg, 0x55 * _Ftf_);
				break;
		}

		SSE2_PSLLD_I8_to_XMM(t0reg, 1);
		SSE2_PSRLD_I8_to_XMM(t0reg, 1);
		SSE_RSQRTSS_XMM_to_XMM(t0reg, t0reg);

		if( _Fsf_ == 0 ) {
			SSE_MULSS_XMM_to_XMM(t0reg, fsreg);
		}
		else {
			// mov _Fsf_ to lower word
			SSE2_PSHUFD_XMM_to_XMM(fsreg, fsreg, (0xe4e4>>(2*_Fsf_))&0xff);
			SSE_MULSS_XMM_to_XMM(t0reg, fsreg);

			// mov back
			SSE2_PSHUFD_XMM_to_XMM(fsreg, fsreg, (0xe4e4>>(8-2*_Fsf_))&0xff);
		}

		// store
		SSE_MOVSS_XMM_to_M32((int)&VU->q.F, t0reg);

		_freeXMMreg(t0reg);
		_clearNeededXMMregs();
	}
}

void _addISIMMtoIT(VURegs *VU, s16 imm) {
	if (_Ft_ == 0) return;

	if (_Ft_ == _Fs_) {
		if (imm != 0 ) {
			ADD16ItoM(VU_VI_ADDR(_Ft_ ), imm);
		}
	} else {
		if (_Fs_ == 0) {
			MOV16ItoM(VU_VI_ADDR(_Ft_), imm);
		} else {
			MOV16MtoR(EAX, VU_VI_ADDR( _Fs_ ) );
			if (imm != 0) {
				ADD16ItoR( EAX, imm );
			}
			MOV16RtoM(VU_VI_ADDR( _Ft_ ), EAX );
		}
	}
}

void recVUMI_IADDI(VURegs *VU) {
	s16 imm;

	if ( _Ft_ == 0 ) return;

	imm = ( VU->code >> 6 ) & 0x1f;
	imm = ( imm & 0x10 ? 0xfff0 : 0) | ( imm & 0xf );
	_addISIMMtoIT(VU, imm);
}

void recVUMI_IADDIU(VURegs *VU) {
	int imm;

	if ( _Ft_ == 0 ) return;

	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	_addISIMMtoIT(VU, imm);
}

void recVUMI_IADD( VURegs *VU )
{
   if ( _Fd_ == 0 ) return;

   if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) )
   {
      MOV16ItoM( VU_VI_ADDR( _Fd_ ), 0 );
   } 
   else if ( _Fs_ == 0 )
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else if ( _Ft_ == 0 )
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
      ADD16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
}

void recVUMI_IAND( VURegs *VU )
{
   if ( _Fd_ == 0 ) return;

   if ( ( _Fs_ == 0 ) || ( _Ft_ == 0 ) )
   {
      XOR16RtoR( EAX, EAX );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else
   {
      if ( _Fs_ == _Ft_ )
      {
         MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
         MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
      }
      else if ( _Fd_ == _Fs_ )
      {
         MOV16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
         AND16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
      }
      else if ( _Fd_ == _Ft_ )
      {
         MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
         AND16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
      }
      else
      {
         MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
         AND16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
         MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
      }
   }
}

void recVUMI_IOR( VURegs *VU )
{
   if ( _Fd_ == 0 ) return;

   if ( _Ft_ == _Fs_ || _Fs_ == 0 )
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
      if ( _Ft_ != 0 )
      {
         OR16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      }
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
}

void recVUMI_ISUB( VURegs *VU ) {
   if ( _Fd_ == 0 ) return;

   if ( _Fs_ == _Ft_ )
   {
	   MOV16ItoM( VU_VI_ADDR( _Fd_ ),0);
   }
   else if ( _Fs_ == 0 )
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      NEG16R( EAX );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else if ( _Ft_ == 0 )
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
   else
   {
      MOV16MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
      SUB16MtoR( EAX, VU_VI_ADDR( _Ft_ ) );
      MOV16RtoM( VU_VI_ADDR( _Fd_ ), EAX );
   }
}

void recVUMI_ISUBIU( VURegs *VU ) {
	int imm;

	if ( _Ft_ == 0 ) return;

	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	if (_Ft_ == _Fs_) {
		if (imm != 0 ) {
			SUB16ItoM(VU_VI_ADDR(_Ft_ ), imm);
		}
	} else {
		if (_Fs_ == 0) {
			MOV16ItoM(VU_VI_ADDR(_Ft_), 0 - imm);
		} else {
			MOV16MtoR(EAX, VU_VI_ADDR( _Fs_ ) );
			if (imm != 0) {
				SUB16ItoR( EAX, imm );
			}
			MOV16RtoM(VU_VI_ADDR( _Ft_ ), EAX );
		}
	}
}

void recVUMI_MOVE( VURegs *VU ) {
	int t0reg;
	int fsreg;
	int ftreg;

	if (_Ft_ == 0) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);

		SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		if (_Ft_ != _Fs_) {
			ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
			SSE_MOVAPS_XMM_to_XMM(ftreg, fsreg);
		}
	}

	_clearNeededXMMregs();
}

void recVUMI_MFIR( VURegs *VU ) {
	int t0reg;
	int ftreg;

	if ( _Ft_ == 0 ) return;

	_addNeededVFtoXMMreg(_Ft_);
	t0reg = _allocTempXMMreg(-1);

	MOVSX32M16toR(EAX, VU_VI_ADDR(_Fs_));
	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_MOVD_R32_to_XMM(t0reg, EAX);
	else SSE2EMU_MOVD_R32_to_XMM(t0reg, EAX);
	SSE_UNPCKLPS_XMM_to_XMM(t0reg, t0reg);
	SSE_MOVLHPS_XMM_to_XMM(t0reg, t0reg);

    if (_X_Y_Z_W != 0xf) {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		VU_MERGE_REGS(ftreg, t0reg);
    } else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
		SSE_MOVAPS_XMM_to_XMM(ftreg, t0reg);
		_freeXMMreg(t0reg);
	}

	_clearNeededXMMregs();
}

void recVUMI_MTIR( VURegs *VU )
{
	if ( _Ft_ == 0 ) return;

	_flushVF_XMMReg(VU, _Fs_);

	MOV32MtoR( EAX, (int)&VU->VF[ _Fs_ ].F[ _Fsf_ ] );
	MOV16RtoM( VU_VI_ADDR( _Ft_ ), EAX );
} 

void recVUMI_MR32( VURegs *VU ) {
	int t0reg;
	int fsreg;
	int ftreg;

	if (_Ft_ == 0) return;

	_addNeededVFtoXMMreg(_Fs_);
	_addNeededVFtoXMMreg(_Ft_);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	if (_X_Y_Z_W != 0xf) {
		t0reg = _allocTempXMMreg(-1);
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);

		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, fsreg, 0x39);
		else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, fsreg, 0x39);

		VU_MERGE_REGS(ftreg, t0reg);
	} else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);

		if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(ftreg, fsreg, 0x39);
		else SSE2EMU_PSHUFD_XMM_to_XMM(ftreg, fsreg, 0x39);
	}

	_clearNeededXMMregs();
}

void _loadEAX(VURegs *VU) {
	int t0reg;
	int ftreg;

    if (_X_Y_Z_W != 0xf) {

		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

	    SSE_MOVAPSRmtoR(t0reg, EAX);
		VU_MERGE_REGS(ftreg, t0reg);
    } else {
		ftreg = _allocVFtoXMMreg(VU, -1, _Ft_, MODE_WRITE);
	    SSE_MOVAPSRmtoR(ftreg, EAX);
	}

	_clearNeededXMMregs();
}

void recVUMI_LQ(VURegs *VU) {
	s16 imm;

	if ( _Ft_ == 0 ) return;

	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff); 
	if (_Fs_ == 0) {
		MOV32ItoR(EAX, (int)VU->Mem + ((imm * 16) & 0x3fff));
	} else {
		MOVZX32M16toR(EAX, VU_VI_ADDR(_Fs_));
		if (imm) ADD32ItoR(EAX, imm);
		SHL32ItoR(EAX, 4);
		AND32ItoR(EAX, 0x3fff);
		ADD32ItoR(EAX, (int)VU->Mem);
	}

	_loadEAX(VU);
}

void recVUMI_LQD( VURegs *VU ) {
	if ( _Fs_ != 0 ) {
		DEC16M( VU_VI_ADDR( _Fs_ ) );
	}

	if ( _Ft_ == 0 ) return;

	if ( _Fs_ == 0 ) {
		MOV32ItoR( EAX, (int)VU->Mem );
	} else {
		MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
	}

	_loadEAX(VU);
}

void recVUMI_LQI(VURegs *VU) {
	if ( _Ft_ == 0 ) {
		if( _Fs_ != 0 ) ADD32ItoM( VU_VI_ADDR( _Fs_ ), 1 );
		return;
	}

    if (_Fs_ == 0) {
		MOV32ItoR( EAX, (int)VU->Mem );
    } else {
		//MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		MOV32MtoR( EAX, VU_VI_ADDR( _Fs_ ) );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
		ADD32ItoM( VU_VI_ADDR( _Fs_ ), 1 );
    }

	_loadEAX(VU);
}

#include <assert.h>

void _saveEAX(VURegs *VU, u32 offset) {

	int t0reg, t1reg;
	int fsreg;

	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

    if (_X_Y_Z_W != 0xf) {

		t0reg = _allocTempXMMreg(-1);
		t1reg = _allocTempXMMreg(-1);

		if( offset == 0 ) SSE_MOVAPSRmtoR(t0reg, EDI);
		else SSE_MOVAPS_M128_to_XMM(t0reg, offset);

		SSE_MOVAPS_XMM_to_XMM(t1reg, fsreg);
		VU_MERGE_REGS(t0reg, t1reg);

		if( offset == 0 ) SSE_MOVAPSRtoRm(EDI, t0reg);
		else SSE_MOVAPS_XMM_to_M128(offset, t0reg);

		// slower by 10x!
//		if( offset != 0 ) MOV32ItoR(EDI, offset);
//		SSE_MOVAPS_M128_to_XMM(t0reg, (u32)&SSEmovMask[ _X_Y_Z_W ][0] );
//		SSE_MASKMOVDQU_XMM_to_XMM(fsreg, t0reg);

		_freeXMMreg(t0reg);
		_freeXMMreg(t1reg);
    } else {

		if( offset == 0 ) SSE_MOVAPSRtoRm(EDI, fsreg);
		else SSE_MOVAPS_XMM_to_M128(offset, fsreg);
	}

	_clearNeededXMMregs();
}

void recVUMI_SQ(VURegs *VU) {
	s16 imm;
 
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if ( _Ft_ == 0 ) {
		_saveEAX(VU, (u32)VU->Mem + imm * 16);
	} else {
		MOV32MtoR(EDI, VU_VI_ADDR( _Ft_ ));
		ADD32ItoR( EDI, imm );
		SHL32ItoR( EDI, 4 );
		AND32ItoR( EDI, 0x3fff);
		ADD32ItoR( EDI, (int)VU->Mem );
		_saveEAX(VU, 0);
	}
}

void recVUMI_SQD(VURegs *VU) {
	if (_Ft_ == 0) {
		_saveEAX(VU, (int)VU->Mem);
	} else {
		SUB32ItoM( VU_VI_ADDR( _Ft_ ), 1 );
		MOVZX32M16toR( EDI, VU_VI_ADDR( _Ft_ ) );
		SHL32ItoR( EDI, 4 );
		AND32ItoR( EDI, 0x3fff);
		ADD32ItoR( EDI, (int)VU->Mem );
		_saveEAX(VU, 0);
	}
}

void recVUMI_SQI(VURegs *VU) {
	if (_Ft_ == 0) {
		_saveEAX(VU, (int)VU->Mem);
	} else {
		MOVZX32M16toR( EDI, VU_VI_ADDR( _Ft_ ) );
		SHL32ItoR( EDI, 4 );
		AND32ItoR( EDI, 0x3fff);
		ADD32ItoR( EDI, (int)VU->Mem );
		ADD32ItoM( VU_VI_ADDR( _Ft_ ), 1 );
		_saveEAX(VU, 0);
	}
}

void recVUMI_ILW(VURegs *VU) {
	s16 imm;
 
	if ( _Ft_ == 0 ) return;

	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if ( _Fs_ == 0 ) {
		MOV32ItoR( EAX, (int)VU->Mem + imm * 16 );
	} else {
		MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		ADD32ItoR( EAX, imm );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
	}

	if (_X) XOR32RtoR(EDX, EDX);
	else if (_Y) MOV32ItoR(EDX, 1);
	else if (_Z) MOV32ItoR(EDX, 2);
	else if (_W) MOV32ItoR(EDX, 3);
	MOV32RmStoR( ECX, EAX, EDX, 2 );
	MOV16RtoM( (int)&VU->VI[ _Ft_ ].US[ 0 ], ECX );
}

void recVUMI_ISW( VURegs *VU ) {
	s16 imm;
	//int t0reg;
 

	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if (_Fs_ == 0) {
		MOV32ItoR( EAX, (int)VU->Mem + imm * 16 );
	} else {
		MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		ADD32ItoR( EAX, imm );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
	}

//	t0reg = _allocTempXMMreg(-1);
//	SSE_MOVSS_M32_to_XMM(t0reg, (int)&VU->VI[ _Ft_ ].UL);
//	if(cpucaps.hasStreamingSIMD2Extensions) SSE2_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
//	else SSE2EMU_PSHUFD_XMM_to_XMM(t0reg, t0reg, 0x00);
//	SSE_MOVAPSRtoRm(EAX,  t0reg);
//	_freeXMMreg(t0reg);

	MOV32MtoR( ECX, (int)&VU->VI[ _Ft_ ].US[ 0 ] );

	// if anyone figures out how to do these with offsets
	if (_X) { MOV32RtoRm(EAX, ECX); }
	if( _Y || _Z || _W ) { ADD32ItoR( EAX, 4); }
	if (_Y) { MOV32RtoRm(EAX, ECX); }
	if( _Z || _W ) { ADD32ItoR( EAX, 4); }
	if (_Z) { MOV32RtoRm(EAX, ECX); }
	if (_W) {
		ADD32ItoR( EAX, 4);
		MOV32RtoRm(EAX, ECX);
	}
}

void recVUMI_ILWR( VURegs *VU ) {
	if ( _Ft_ == 0 ) return;

	if ( _Fs_ == 0 ) {
		MOV32ItoR( EAX, (int)VU->Mem );
	} else {
		MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
	}

	if (_X) XOR32RtoR(EDX, EDX);
	else if (_Y) MOV32ItoR(EDX, 1);
	else if (_Z) MOV32ItoR(EDX, 2);
	else if (_W) MOV32ItoR(EDX, 3);
	MOV32RmStoR( ECX, EAX, EDX, 2 );
	MOV16RtoM( (int)&VU->VI[ _Ft_ ].US[ 0 ], ECX );
}

void recVUMI_ISWR( VURegs *VU ) {	

	if ( _Fs_ == 0 ) {
		MOV32ItoR( EAX, (int)VU->Mem );
	} else {
		MOVZX32M16toR( EAX, VU_VI_ADDR( _Fs_ ) );
		SHL32ItoR( EAX, 4 );
		AND32ItoR( EAX, 0x3fff);
		ADD32ItoR( EAX, (int)VU->Mem );
	}

	MOV32MtoR( ECX, (int)&VU->VI[ _Ft_ ].US[ 0 ] );
	if (_X) XOR32RtoR(EDX, EDX);
	else if (_Y) MOV32ItoR(EDX, 1);
	else if (_Z) MOV32ItoR(EDX, 2);
	else if (_W) MOV32ItoR(EDX, 3);
	MOV16RtoRmS( ECX, EAX, EDX, 2 );
}

void recVUMI_RINIT(VURegs *VU) {
	_flushVF_XMMReg(VU, _Fs_);
	MOV32MtoR( EAX, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );
	AND32ItoR( EAX, 0x7fffff );
	OR32ItoR( EAX, 0x7f << 23 );
	MOV32RtoM( VU_REGR_ADDR, EAX );
}

void recVUMI_RGET(VURegs *VU) {
   if ( _Ft_ == 0 ) return;

   _flushVF_XMMReg(VU, _Ft_);

   MOV32MtoR( EAX, VU_REGR_ADDR );
   if ( _X )   MOV32RtoM( VU_VFx_ADDR( _Ft_ ), EAX );
   if ( _Y )   MOV32RtoM( VU_VFy_ADDR( _Ft_ ), EAX );
   if ( _Z )   MOV32RtoM( VU_VFz_ADDR( _Ft_ ), EAX );
   if ( _W )   MOV32RtoM( VU_VFw_ADDR( _Ft_ ), EAX );
}

void recVUMI_RNEXT( VURegs *VU )
{
   if ( _Ft_ == 0) return;

   _flushVF_XMMReg(VU, _Ft_);

   MOV32MtoR( EAX, VU_REGR_ADDR );
   MOV32ItoR( ECX, 0x15a4e35 );
   IMUL32R( ECX );
   ADD32ItoR( EAX, 1 );
   SHR32ItoR( EAX, 9 );
   MOV32RtoM( VU_REGR_ADDR, EAX );
   OR32ItoR( EAX, 0x7f << 23 );
   if ( _X )   MOV32RtoM( VU_VFx_ADDR( _Ft_ ), EAX );
   if ( _Y )   MOV32RtoM( VU_VFy_ADDR( _Ft_ ), EAX );
   if ( _Z )   MOV32RtoM( VU_VFz_ADDR( _Ft_ ), EAX );
   if ( _W )   MOV32RtoM( VU_VFw_ADDR( _Ft_ ), EAX );
}

void recVUMI_RXOR( VURegs *VU )
{
	_flushVF_XMMReg(VU, _Fs_);

   MOV32MtoR( EAX, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );
   XOR32MtoR( EAX, VU_REGR_ADDR );
   AND32ItoR( EAX, 0x7fffff );
   OR32ItoR ( EAX, 0x7f << 23 );
   MOV32RtoM( VU_REGR_ADDR, EAX );
} 

void recVUMI_WAITQ( VURegs *VU )
{
}

void recVUMI_FSAND( VURegs *VU ) {
	u16 imm;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_Ft_ == 0) return; 

	MOV32MtoR(EAX, (int)&VU0.VI[REG_STATUS_FLAG].US[0]);
	AND32ItoR( EAX, 0xFFF );
	AND32ItoR(EAX, imm);
	MOV32RtoM((int)&VU->VI[_Ft_].US[0], EAX);
}

void recVUMI_FSEQ( VURegs *VU )
{
	u32 imm;
	if ( _Ft_ == 0 ) return;

	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	MOV32MtoR( EAX, (int)&VU0.VI[ REG_STATUS_FLAG ].UL );
	AND32ItoR( EAX, 0xFFF );

	MOV32ItoM((int)&VU->VI[_Ft_].US[0], 0);
	CMP32ItoR(EAX, imm);

	// if eax!=0xffffff, set to 0
	j8Ptr[ 0 ] = JNE8( 0 );
	MOV32ItoM((int)&VU->VI[_Ft_].US[0], 1);
	x86SetJ8( j8Ptr[ 0 ] );
}

void recVUMI_FSOR( VURegs *VU )
{
	u32 imm;
	if(_Ft_ == 0) return; 
	
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	MOV32MtoR( EAX, (int)&VU0.VI[ REG_STATUS_FLAG ].UL );
	OR32ItoR( EAX, imm );
	MOV32MtoR( (int)&VU->VI[_Ft_], EAX);
}

void recVUMI_FSSET(VURegs *VU) {
	u16 imm = 0;
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7FF);

	MOV32MtoR(EAX, (int)&VU0.VI[REG_STATUS_FLAG].US[0]);
	if (imm == 0) {
		MOV32RtoM((int)&VU->statusflag, EAX);
		AND32ItoM((int)&VU->statusflag, 0x3F);
	} else {		
		AND32ItoR(EAX, 0x3F);
		OR32ItoR(EAX, imm & 0xFC0);
		MOV32RtoM((int)&VU->statusflag, EAX);
	}
}

void recVUMI_FMAND( VURegs *VU ) {
	if ( _Ft_ == 0 ) return;

	MOV32MtoR( EAX, (int)&VU->VI[ _Fs_ ].US[ 0 ] );
	AND32MtoR( EAX, (int)&VU0.VI[ REG_MAC_FLAG ].US[ 0 ] );
	AND32ItoR( EAX, 0xFFFF );
	MOV32RtoM( (int)&VU->VI[ _Ft_ ].US[ 0 ], EAX );
}

void recVUMI_FMEQ( VURegs *VU )
{
	if ( _Ft_ == 0 ) return;

	MOV32MtoR( EAX, (int)&VU0.VI[ REG_MAC_FLAG ].UL );
	AND32ItoR( EAX, 0xFFFF );

	MOV32ItoM((int)&VU->VI[_Ft_].US[0], 0);
	CMP32MtoR(EAX, (int)&VU->VI[_Fs_].US[0]);

	// if eax!=0xffffff, set to 0
	j8Ptr[ 0 ] = JNE8( 0 );
	MOV32ItoM((int)&VU->VI[_Ft_].US[0], 1);
	x86SetJ8( j8Ptr[ 0 ] );
}

void recVUMI_FMOR( VURegs *VU )
{
	if ( _Ft_ == 0 ) return;

	MOV32MtoR( EAX, (int)&VU0.VI[ REG_MAC_FLAG ].UL );
	AND32ItoR( EAX, 0xFFFF );
	OR16MtoR( EAX, (int)&VU->VI[_Fs_].US[0]);
	MOV32RtoM((int)&VU->VI[_Ft_].US[0], EAX);
}

void recVUMI_FCAND( VURegs *VU ) {
	MOV32MtoR( EAX, (int)&VU0.VI[ REG_CLIP_FLAG ].UL );
	AND32ItoR( EAX, VU->code & 0xFFFFFF );
	NEG32R( EAX );
	SBB32RtoR( EAX, EAX );
	NEG32R( EAX );
	MOV32RtoM( (int)&VU->VI[ 1 ].UL, EAX );
}

void recVUMI_FCEQ( VURegs *VU )
{
	MOV32MtoR( EAX, (int)&VU0.VI[ REG_CLIP_FLAG ].UL );
	MOV32ItoM((int)&VU->VI[1].US[0], 0);
	AND32ItoR( EAX, 0xffffff);

	CMP32ItoR( EAX, VU->code&0xffffff );

	// if eax!=0xffffff, set to 0
	j8Ptr[ 0 ] = JNE8( 0 );
	MOV32ItoM((int)&VU->VI[1].US[0], 1);
	x86SetJ8( j8Ptr[ 0 ] );
}

void recVUMI_FCOR( VURegs *VU )
{
	MOV32MtoR( EAX, (int)&VU0.VI[ REG_CLIP_FLAG ].UL );
	AND32ItoR( EAX, 0xffffff);
	OR32ItoR( EAX, VU->code & 0xFFFFFF );

	MOV32ItoM((int)&VU->VI[1].US[0], 0);
	CMP32ItoR( EAX, 0xffffff );

	// if eax==0xffffff, set to 1
	j8Ptr[ 0 ] = JNE8( 0 );
	MOV32ItoM((int)&VU->VI[1].US[0], 1);
	x86SetJ8( j8Ptr[ 0 ] );
}

void recVUMI_FCSET( VURegs *VU )
{
	MOV32ItoM( (int)&VU->clipflag, VU->code&0xffffff );
	MOV32ItoM( (int)&VU0.VI[ REG_CLIP_FLAG ].UL, VU->code&0xffffff );
}

void recVUMI_FCGET( VURegs *VU )
{
	if(_Ft_ == 0) return;

	MOV32MtoR(EAX, (int)&VU0.VI[REG_CLIP_FLAG].UL);
	AND32ItoR(EAX, 0x0fff);
	MOV32RtoM((int)&VU->VI[_Ft_].US[0], EAX);
}

static s32 _recbranchAddr(VURegs * VU) {
	bpc = pc + (_Imm11_ << 3); 
	if (bpc < 0) {
		bpc = pc + (_UImm11_ << 3); 
	}
	if (VU == &VU1) {
		bpc&= 0x3fff;
	} else {
		bpc&= 0x0fff;
	}

	return bpc;
}

void recVUMI_IBEQ(VURegs *VU) {
	bpc = _recbranchAddr(VU); 

	MOV16MtoR( EAX, (int)&VU->VI[ _Ft_ ].US[ 0 ] );
	CMP16MtoR( EAX, (int)&VU->VI[ _Fs_ ].US[ 0 ] );

	j8Ptr[ 0 ] = JNE8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBGEZ( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( (int)&VU->VI[ REG_TPC ].UL, (int)pc );
	CMP16ItoM( (int)&VU->VI[ _Fs_ ].US[ 0 ], 0x0 );

	j8Ptr[ 0 ] = JL8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBGTZ( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( (int)&VU->VI[ REG_TPC ].UL, (int)pc );
	CMP16ItoM( (int)&VU->VI[ _Fs_ ].US[ 0 ], 0x0 );

	j8Ptr[ 0 ] = JLE8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBLEZ( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( (int)&VU->VI[ REG_TPC ].UL, (int)pc );
	CMP16ItoM( (int)&VU->VI[ _Fs_ ].US[ 0 ], 0x0 );

	j8Ptr[ 0 ] = JG8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

void recVUMI_IBLTZ( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( (int)&VU->VI[ REG_TPC ].UL, (int)pc );
	CMP16ItoM( (int)&VU->VI[ _Fs_ ].US[ 0 ], 0x0 );

	j8Ptr[ 0 ] = JGE8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );
    
	branch |= 1;
}

void recVUMI_IBNE( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV16ItoM( (int)&VU->VI[ REG_TPC ].UL, (int)pc );
	MOV16MtoR( EAX, (int)&VU->VI[ _Ft_ ].US[ 0 ] );
	CMP16MtoR( EAX, (int)&VU->VI[ _Fs_ ].US[ 0 ] );

	j8Ptr[ 0 ] = JE8( 0 );
	MOV16ItoM((uptr)&VU->branch, 2);
	MOV32ItoM((uptr)&VU->branchpc, bpc);

	// only jump when not E bit
	x86SetJ8( j8Ptr[ 0 ] );
    
	branch |= 1;
}

void recVUMI_B(VURegs *VU) {
	bpc = _recbranchAddr(VU); 

	MOV32ItoM((int)&VU->VI[ REG_TPC ].UL, bpc);

	branch |= 3;
}

void recVUMI_BAL( VURegs *VU ) {
	bpc = _recbranchAddr(VU); 

	MOV32ItoM((int)&VU->VI[ REG_TPC ].UL, bpc);

	if ( _Ft_ ) {
		MOV32ItoR(EAX, pc);
		ADD32ItoR(EAX, 8);
		SHR32ItoR(EAX, 3);
		MOV16RtoM( (int)&VU->VI[ _Ft_ ].US[ 0 ], EAX );
	}

	branch |= 3;
}

void recVUMI_JR(VURegs *VU) {
	MOV32MtoR(EAX, (int)&VU->VI[ _Fs_ ].US[ 0 ] );
	SHL32ItoR(EAX, 3);
	MOV32RtoM((int)&VU->VI[ REG_TPC ].UL, EAX);

	branch |= 3;
}

void recVUMI_JALR(VURegs *VU) {
	MOV32MtoR(EAX, (int)&VU->VI[ _Fs_ ].US[ 0 ] );
	SHL32ItoR(EAX, 3);
	MOV32RtoM((int)&VU->VI[ REG_TPC ].UL, EAX);

	if ( _Ft_ ) {
		MOV32ItoR(EAX, pc);
		ADD32ItoR(EAX, 8);
		SHR32ItoR(EAX, 3);
		MOV16RtoM( (int)&VU->VI[ _Ft_ ].US[ 0 ], EAX );
	}

	branch |= 3;
}

void recVUMI_MFP(VURegs *VU) {
	if (_Ft_ == 0) return; 

	MOV32MtoR(EAX, (int)&VU->VI[REG_P].UL );

	if (_X) MOV32RtoM((int)&VU->VF[_Ft_].i.x, EAX);
	if (_Y) MOV32RtoM((int)&VU->VF[_Ft_].i.y, EAX);
	if (_Z) MOV32RtoM((int)&VU->VF[_Ft_].i.z, EAX);
	if (_W) MOV32RtoM((int)&VU->VF[_Ft_].i.w, EAX);
}

void recVUMI_WAITP(VURegs *VU) {
}

void recVUMI_ESADD( VURegs *VU )
{
}

void recVUMI_ERSADD( VURegs *VU )
{
}

static __declspec(align(16)) float tempmem[4];

void recVUMI_ELENG( VURegs *VU )
{
	int t0reg, t1reg;
	int fsreg;

	_addNeededVFtoXMMreg(_Fs_);
	t0reg = _allocTempXMMreg(-1);
	t1reg = _allocTempXMMreg(-1);
	fsreg = _allocVFtoXMMreg(VU, -1, _Fs_, MODE_READ);

	SSE_MOVAPS_XMM_to_XMM(t0reg, fsreg);
	SSE_MULPS_XMM_to_XMM(t0reg, t0reg);

	if(cpucaps.hasStreamingSIMD2Extensions) {
		SSE2_PSHUFD_XMM_to_XMM(t1reg, t0reg, 0x55);
		SSE_ADDSS_XMM_to_XMM(t0reg, t1reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, t0reg, 0xaa);
		SSE_ADDSS_XMM_to_XMM(t0reg, t1reg);
	}
	else {
		SSE2EMU_PSHUFD_XMM_to_XMM(t1reg, t0reg, 0x55);
		SSE_ADDSS_XMM_to_XMM(t0reg, t1reg);
		SSE2EMU_PSHUFD_XMM_to_XMM(t1reg, t0reg, 0xaa);
		SSE_ADDSS_XMM_to_XMM(t0reg, t1reg);
	}

	SSE_SQRTSS_XMM_to_XMM(t1reg, t0reg);
	SSE_MOVSS_XMM_to_M32((int)&VU->p, t1reg);

	SSE_MOVSS_M32_to_XMM(t0reg, (int)&VU->p); 

	_freeXMMreg(t0reg);
	_freeXMMreg(t1reg);
	_clearNeededXMMregs();
}

void recVUMI_ERLENG( VURegs *VU )
{
}

void recVUMI_EATANxy( VURegs *VU )
{
}

void recVUMI_EATANxz( VURegs *VU )
{
}

void recVUMI_ESUM( VURegs *VU )
{
}

void recVUMI_ERCPR( VURegs *VU )
{
}

void recVUMI_ESQRT( VURegs *VU )
{
}

void recVUMI_ERSQRT( VURegs *VU )
{
}

void recVUMI_ESIN( VURegs *VU )
{
}

void recVUMI_EATAN( VURegs *VU )
{
}

void recVUMI_EEXP( VURegs *VU )
{
}

void recVUMI_XITOP( VURegs *VU )
{
	if (_Ft_ == 0) return;
	MOV32MtoR( EAX, (int)&VU->vifRegs->itop );
	MOV16RtoM( VU_VI_ADDR( _Ft_ ), EAX );
}

void recVUMI_XTOP( VURegs *VU ) {
	if ( _Ft_ == 0 ) return;

	MOV32MtoR( EAX, (int)&VU->vifRegs->top );
	MOV16RtoM( VU_VI_ADDR( _Ft_ ), EAX );
}

extern HANDLE g_hGsEvent;

void gstrans(u32 *pMem, u32 addr)
{
	u32 size;
	u8* pmem;
	u32* data = (u32*)((u8*)pMem + (addr&0x3fff));

	size = GSgifTransferDummy(0, data, 0x4000>>4);

	size = 0x4000-(size<<4);
	pmem = GSRingBufCopy(NULL, size, GS_RINGTYPE_P1);
	assert( pmem != NULL );

	memcpy_amd(pmem, (u8*)pMem+addr, size);
	GSRINGBUF_DONECOPY(pmem, size);

	if( !CHECK_DUALCORE ) {
		SetEvent(g_hGsEvent);
	}
}

void recVUMI_XGKICK( VURegs *VU ) {
#ifdef __x86_64__
	MOV32MtoR(RSI, (uptr)&VU->VI[_Fs_].US[0]);
	SHL32ItoR(RSI, 4);
	AND32ItoR(RSI, 0x3fff);

	MOV64MtoR(RDI, (uptr)VU->Mem );
	MOV64MtoR(RDX, (uptr)GSgifTransfer1);
	JMP64R(RDX);
#else
	MOVZX32M16toR(EAX, (uptr)&VU->VI[_Fs_].US[0]);
	SHL32ItoR(EAX, 4);
	AND32ItoR(EAX, 0x3fff);

	PUSH32R(EAX);
	PUSH32I((int)VU->Mem);
	iFlushCall();

	if( CHECK_MULTIGS ) {
		CALLFunc((int)gstrans);
		ADD32ItoR(ESP, 8);
	}
	else {
		CALLFunc((int)GSgifTransfer1);
		//OR32ItoM((u32)&GSCSRr, 2);
	}

#endif
}

