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
#include "iVUmicro.h"
#include "iVU0micro.h"
#include "iVUops.h"
#include "VUops.h"


#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

char *recMemVU0;	   /* VU0 blocks */
char *recVU0;	   /* VU0 mem */
char *recVU0mac;
char *recVU0status;
char *recVU0Q;
char *recVU0Qstop;
char *recVU0cycles;
char *recPtrVU0;
static VURegs *VU = (VURegs*)&VU0;

static _vuopinfo _opinfo[256];
static u32 _mxcsr = 0x7F80;

static int recMacStop;
static int recStatusStop;
static int recClipStop;

static u32 vu0reccountold = -1;
static u32 recQstop;
static u32 recQpending;

//Lower/Upper instructions can use that..
#define _Ft_ ((VU0.code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((VU0.code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((VU0.code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X ((VU0.code>>24) & 0x1)
#define _Y ((VU0.code>>23) & 0x1)
#define _Z ((VU0.code>>22) & 0x1)
#define _W ((VU0.code>>21) & 0x1)

#define _Fsf_ ((VU0.code >> 21) & 0x03)
#define _Ftf_ ((VU0.code >> 23) & 0x03)


#define VU0_VFx_ADDR(x) (u32)&VU0.VF[x].UL[0]
#define VU0_VFy_ADDR(x) (u32)&VU0.VF[x].UL[1]
#define VU0_VFz_ADDR(x) (u32)&VU0.VF[x].UL[2]
#define VU0_VFw_ADDR(x) (u32)&VU0.VF[x].UL[3]

#define VU0_REGR_ADDR (u32)&VU0.VI[REG_R]
#define VU0_REGI_ADDR (u32)&VU0.VI[REG_I]
#define VU0_REGQ_ADDR (u32)&VU0.VI[REG_Q]
#define VU0_REGMAC_ADDR (u32)&VU0.VI[REG_MAC_FLAG]

#define VU0_VI_ADDR(x)	(u32)&VU0.VI[x].UL

#define VU0_ACCx_ADDR (u32)&VU0.ACC.UL[0]
#define VU0_ACCy_ADDR (u32)&VU0.ACC.UL[1]
#define VU0_ACCz_ADDR (u32)&VU0.ACC.UL[2]
#define VU0_ACCw_ADDR (u32)&VU0.ACC.UL[3]

void (*recVU0_LOWER_OPCODE[128])() = { 
	recVU0MI_LQ    , recVU0MI_SQ    , recVU0unknown , recVU0unknown,  
	recVU0MI_ILW   , recVU0MI_ISW   , recVU0unknown , recVU0unknown,  
	recVU0MI_IADDIU, recVU0MI_ISUBIU, recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0MI_FCEQ  , recVU0MI_FCSET , recVU0MI_FCAND, recVU0MI_FCOR, /* 0x10 */ 
	recVU0MI_FSEQ  , recVU0MI_FSSET , recVU0MI_FSAND, recVU0MI_FSOR, 
	recVU0MI_FMEQ  , recVU0unknown  , recVU0MI_FMAND, recVU0MI_FMOR, 
	recVU0MI_FCGET , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0MI_B     , recVU0MI_BAL   , recVU0unknown , recVU0unknown, /* 0x20 */  
	recVU0MI_JR    , recVU0MI_JALR  , recVU0unknown , recVU0unknown, 
	recVU0MI_IBEQ  , recVU0MI_IBNE  , recVU0unknown , recVU0unknown, 
	recVU0MI_IBLTZ , recVU0MI_IBGTZ , recVU0MI_IBLEZ, recVU0MI_IBGEZ, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x30 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0LowerOP  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x40*/  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x50 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x60 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x70 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
}; 
 
void (*recVU0LowerOP_T3_00_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_MOVE  , recVU0MI_LQI   , recVU0MI_DIV  , recVU0MI_MTIR,  
	recVU0MI_RNEXT , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_MFP   , recVU0unknown , recVU0unknown,  
	recVU0MI_ESADD , recVU0MI_EATANxy, recVU0MI_ESQRT, recVU0MI_ESIN,  
}; 
 
void (*recVU0LowerOP_T3_01_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_MR32  , recVU0MI_SQI   , recVU0MI_SQRT , recVU0MI_MFIR,  
	recVU0MI_RGET  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0MI_XITOP, recVU0unknown,  
	recVU0MI_ERSADD, recVU0MI_EATANxz, recVU0MI_ERSQRT, recVU0MI_EATAN, 
}; 
 
void (*recVU0LowerOP_T3_10_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_LQD   , recVU0MI_RSQRT, recVU0MI_ILWR,  
	recVU0MI_RINIT , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_ELENG , recVU0MI_ESUM  , recVU0MI_ERCPR, recVU0MI_EEXP,  
}; 
 
void (*recVU0LowerOP_T3_11_OPCODE[32])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0MI_SQD   , recVU0MI_WAITQ, recVU0MI_ISWR,  
	recVU0MI_RXOR  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_ERLENG, recVU0unknown  , recVU0MI_WAITP, recVU0unknown,  
}; 
 
void (*recVU0LowerOP_OPCODE[64])() = { 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x10 */  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown, /* 0x20 */  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0MI_IADD  , recVU0MI_ISUB  , recVU0MI_IADDI, recVU0unknown, /* 0x30 */ 
	recVU0MI_IAND  , recVU0MI_IOR   , recVU0unknown , recVU0unknown,  
	recVU0unknown  , recVU0unknown  , recVU0unknown , recVU0unknown,  
	recVU0LowerOP_T3_00, recVU0LowerOP_T3_01, recVU0LowerOP_T3_10, recVU0LowerOP_T3_11,  
}; 
 
void (*recVU0_UPPER_OPCODE[64])() = { 
	recVU0MI_ADDx  , recVU0MI_ADDy  , recVU0MI_ADDz  , recVU0MI_ADDw, 
	recVU0MI_SUBx  , recVU0MI_SUBy  , recVU0MI_SUBz  , recVU0MI_SUBw, 
	recVU0MI_MADDx , recVU0MI_MADDy , recVU0MI_MADDz , recVU0MI_MADDw, 
	recVU0MI_MSUBx , recVU0MI_MSUBy , recVU0MI_MSUBz , recVU0MI_MSUBw, 
	recVU0MI_MAXx  , recVU0MI_MAXy  , recVU0MI_MAXz  , recVU0MI_MAXw,  /* 0x10 */  
	recVU0MI_MINIx , recVU0MI_MINIy , recVU0MI_MINIz , recVU0MI_MINIw, 
	recVU0MI_MULx  , recVU0MI_MULy  , recVU0MI_MULz  , recVU0MI_MULw, 
	recVU0MI_MULq  , recVU0MI_MAXi  , recVU0MI_MULi  , recVU0MI_MINIi, 
	recVU0MI_ADDq  , recVU0MI_MADDq , recVU0MI_ADDi  , recVU0MI_MADDi, /* 0x20 */ 
	recVU0MI_SUBq  , recVU0MI_MSUBq , recVU0MI_SUBi  , recVU0MI_MSUBi, 
	recVU0MI_ADD   , recVU0MI_MADD  , recVU0MI_MUL   , recVU0MI_MAX, 
	recVU0MI_SUB   , recVU0MI_MSUB  , recVU0MI_OPMSUB, recVU0MI_MINI, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown,  /* 0x30 */ 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown, 
	recVU0_UPPER_FD_00, recVU0_UPPER_FD_01, recVU0_UPPER_FD_10, recVU0_UPPER_FD_11,  
}; 
 
void (*recVU0_UPPER_FD_00_TABLE[32])() = { 
	recVU0MI_ADDAx, recVU0MI_SUBAx , recVU0MI_MADDAx, recVU0MI_MSUBAx, 
	recVU0MI_ITOF0, recVU0MI_FTOI0, recVU0MI_MULAx , recVU0MI_MULAq , 
	recVU0MI_ADDAq, recVU0MI_SUBAq, recVU0MI_ADDA  , recVU0MI_SUBA  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
	recVU0unknown , recVU0unknown , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_01_TABLE[32])() = { 
	recVU0MI_ADDAy , recVU0MI_SUBAy  , recVU0MI_MADDAy, recVU0MI_MSUBAy, 
	recVU0MI_ITOF4 , recVU0MI_FTOI4 , recVU0MI_MULAy , recVU0MI_ABS   , 
	recVU0MI_MADDAq, recVU0MI_MSUBAq, recVU0MI_MADDA , recVU0MI_MSUBA , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_10_TABLE[32])() = { 
	recVU0MI_ADDAz , recVU0MI_SUBAz  , recVU0MI_MADDAz, recVU0MI_MSUBAz, 
	recVU0MI_ITOF12, recVU0MI_FTOI12, recVU0MI_MULAz , recVU0MI_MULAi , 
	recVU0MI_MADDAi, recVU0MI_SUBAi , recVU0MI_MULA  , recVU0MI_OPMULA, 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 
 
void (*recVU0_UPPER_FD_11_TABLE[32])() = { 
	recVU0MI_ADDAw , recVU0MI_SUBAw  , recVU0MI_MADDAw, recVU0MI_MSUBAw, 
	recVU0MI_ITOF15, recVU0MI_FTOI15, recVU0MI_MULAw , recVU0MI_CLIP  , 
	recVU0MI_MADDAi, recVU0MI_MSUBAi, recVU0unknown  , recVU0MI_NOP   , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
	recVU0unknown  , recVU0unknown  , recVU0unknown  , recVU0unknown  , 
}; 


void executeVU0( void ) {
	void (*recFunc)( void );
	char *p;

	if (VU0.VI[REG_TPC].UL >= VU0.maxmicro) {
#ifdef CPU_LOG
		SysPrintf("VU0 memory overflow!!: %x\n", VU->VI[REG_TPC].UL);
#endif
		VU0.VI[REG_VPU_STAT].UL&= ~0x1;
		VU->cycle++;
		return;
	}
//	SysPrintf("executeVU0 %x\n", VU0.VI[ REG_TPC ].UL);
	p = (char*)&recVU0[VU0.VI[ REG_TPC ].UL ];
	if (p != NULL) recFunc = (void (*)(void)) (uptr)*(u32*)p;
	else { recError(); return; }

	if (recFunc == 0) {
		recRecompileVU0();
		recFunc = (void (*)(void)) (uptr)*(u32*)p;
	}
	recFunc();
//	SysPrintf("exec ok %x %x\n", VU0.VI[REG_VPU_STAT].UL, VU0.branchpc);
}

__declspec(align(16)) static VECTOR _VF;
__declspec(align(16)) static VECTOR _VFc;
__declspec(align(16)) static REG_VI _VI;
__declspec(align(16)) static REG_VI _VIc;

static void checkvucodefn()
{
	int pctemp;

	__asm mov pctemp, eax
	SysPrintf("vu0 code changed! %x\n", pctemp);
}

void VU0Recompile(_vuopinfo *info, int i) { 
	_VURegsNum lregs;
	_VURegsNum uregs;
	u32 *ptr; 
	int vfreg;
	int vireg;
//	int i;

	ptr = (u32*)&VU0.Micro[ pc ]; 
	pc += 8;
	cinfo = info;

#ifdef _DEBUG
	CMP32ItoM((u32)ptr, ptr[0]);
	j8Ptr[0] = JNE8(0);
	CMP32ItoM((u32)(ptr+1), ptr[1]);
	j8Ptr[1] = JNE8(0);
	j8Ptr[2] = JMP8(0);
	x86SetJ8( j8Ptr[0] );
	x86SetJ8( j8Ptr[1] );
	MOV32ItoR(EAX, pc);
	CALLFunc((u32)checkvucodefn);
	x86SetJ8( j8Ptr[ 2 ] );
#endif

/*	SysPrintf("vucycle %d (%p)\n", info->cycle, info);
	SysPrintf("VU0Recompile Upper: %s\n", disVU0MicroUF( ptr[1], pc ) );
	if ((ptr[1] & 0x80000000) == 0) {
		SysPrintf("VU0Recompile Lower: %s\n", disVU0MicroLF( ptr[0], pc ) );
	}*/

	if ((info->macflag & 0x2) || (info->statusflag & 0x2) || (info->clipflag & 0x2)) {
		int j=i-1;

		while (j >= 0)
		{
			if (_opinfo[i-1].cycle - _opinfo[j].cycle >= 3)
			{
				if (info->macflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU0mac[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL,EAX);
					recMacStop = 1;
				}
				if (info->statusflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU0status[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL,EAX);
					recStatusStop = 1;
				}
				if (info->clipflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU0clip[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL,EAX);
					recClipStop = 1;
				}
				break;
			}
			j--;
		}
	}
	if (((info->macflag & 0x2) && !recMacStop) ||
		((info->statusflag & 0x2) && !recStatusStop) ||
		((info->clipflag & 0x2) && !recClipStop))
	{
		if (i==0) MOV32ItoR(ECX,3);
		else MOV32ItoR(ECX,3 - (_opinfo[i-1].cycle - (vucycleold-1)));
		MOV32MtoR(EAX,(u32)&vu0reccountold);
		DEC32R(EAX);

		j8Ptr[2] = (u8*)x86Ptr;
		CMP32ItoR(EAX,0);
		j8Ptr[0] = JL8(0);
		CMP32ItoR(ECX,0);
		j8Ptr[1] = JLE8(0);

		MOV32RtoR(EDX,EAX);
		SHL32ItoR(EDX,2);
		MOV32ItoR(ESI,(u32)recVU0cycles);
		ADD32MtoR(ESI,(u32)&vu0recpcold);
		MOV32RmtoR(ESI,ESI);
		ADD32RtoR(EDX,ESI);
		MOV32RmtoR(EDX,EDX);
		SUB32RtoR(ECX,EDX);
		DEC32R(EAX);
		JMP8((u32)j8Ptr[2] - (u32)x86Ptr - 2);

		x86SetJ8(j8Ptr[1]);
		x86SetJ8(j8Ptr[0]);

		if (3 - (_opinfo[i-1].cycle - (vucycleold-1)) > 0) INC32R(EAX);
		SHL32ItoR(EAX,3);
		ADD32MtoR(EAX,(u32)&vu0recpcold);

		if (info->macflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU0mac);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL,EDX);
		}
		if (info->statusflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU0status);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL,EDX);
		}
		if (info->clipflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU0clip);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL,EDX);
		}
	}

	if ((info->q & 0x4) && !recQstop) {
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[0] = JLE8(0);
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		MOV32ItoM((u32)&recQpending,0);
		x86SetJ8(j8Ptr[0]);
		recQstop = 1;
	}
	else if ((info->q & 0x1) && !recQstop) {
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		MOV32ItoM((u32)&recQpending,0);
		recQstop = 1;
	}

	if (ptr[1] & 0x40000000 ) { 
		AND32ItoM( (u32)&VU0.VI[ REG_VPU_STAT ].UL, ~0x1 ); /* E flag */ 
		AND32ItoM( (u32)&VU->vifRegs->stat, ~0x4 );
		branch |= 8; 
	}
	if (ptr[1] & 0x20000000) { /* M flag */ 
		OR32ItoM((u32)&VU->flags,VUFLAG_MFLAGSET);
		SysPrintf("fixme: M flag set\n");
	}

	VU->code = ptr[1];
	VU0regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	//_tmpvuTestUpperStalls(VU,&uregs);
	/* check upper flags */ 
	if (ptr[1] & 0x80000000) { /* I flag */ 
		VU0.code = ptr[1]; 
		recVU0_UPPER_OPCODE[ VU0.code & 0x3f ]( ); 

		MOV32ItoM( (u32)&VU0.VI[ REG_I ].UL, ptr[ 0 ] ); 
	} else {
		VU->code = ptr[0]; 
		VU0regs_LOWER_OPCODE[VU->code >> 25](&lregs);
		//_tmpvuTestLowerStalls(VU, &lregs);

		vfreg = 0; vireg = 0;
		if (uregs.VFwrite) {
			if (lregs.VFwrite == uregs.VFwrite) {
				SysPrintf("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle\n");
			}
			if (lregs.VFread0 == uregs.VFwrite ||
				lregs.VFread1 == uregs.VFwrite) {
				int reg = _allocVFtoXMMreg(VU, -1, uregs.VFwrite, MODE_READ);
				SSE_MOVAPS_XMM_to_M128((u32)&_VF, reg);
				vfreg = uregs.VFwrite;
			}
        }

		if (uregs.VIread & (1 << REG_CLIP_FLAG)) {
			if (lregs.VIwrite & (1 << REG_CLIP_FLAG)) {
				SysPrintf("*PCSX2*: Warning, VI write to the same reg in both lower/upper cycle\n");
			}
			if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
				SysPrintf("*PCSX2*: VUrec: fixme, save clip flag !\n");
				_VI = VU0.VI[REG_CLIP_FLAG];
				vireg = REG_CLIP_FLAG;
			}
		}

		VU0.code = ptr[1]; 
		recVU0_UPPER_OPCODE[ VU0.code & 0x3f ]( ); 

		if (vfreg) {
			int reg = _allocVFtoXMMreg(VU, -1, vfreg, MODE_READ | MODE_WRITE);
			SSE_MOVAPS_XMM_to_M128((u32)&_VFc, reg);
			SSE_MOVAPS_M128_to_XMM(reg, (u32)&_VF);
		}
		if (vireg) {
			SysPrintf("should be recompiled\n");
			_VIc = VU0.VI[vireg];
			VU0.VI[vireg] = _VI;
		}

		VU0.code = ptr[0]; 
		recVU0_LOWER_OPCODE[ VU0.code >> 25 ]( ); 

		if (vfreg) {
			int reg = _allocVFtoXMMreg(VU, -1, vfreg, MODE_WRITE);
			SSE_MOVAPS_M128_to_XMM(reg, (u32)&_VFc);
		}
		if (vireg) {
			VU0.VI[vireg] = _VIc;
		}
	}
	
	MOV32MtoR(EAX, (u32)&VU->macflag);
	MOV32RtoM((u32)&recVU0mac[pc-8],EAX);

	MOV32MtoR(EAX, (u32)&VU->statusflag);
	MOV32RtoM((u32)&recVU0status[pc-8],EAX);

	MOV32MtoR(EAX, (u32)&VU->clipflag);
	MOV32RtoM((u32)&recVU0clip[pc-8],EAX);

	if (info->q & 0x8) {
//		SysPrintf("flushing q\n");
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
	}
	if (!recQstop) {
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[1] = JLE8(0);
		SUB32ItoM((u32)&recQpending,(*(u32**)&recVU0cycles[VU0.VI[ REG_TPC ].UL])[i]);
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[0] = JG8(0);
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		x86SetJ8(j8Ptr[0]);
		x86SetJ8(j8Ptr[1]);
	}

	if (info->q & 0x1) {
//		SysPrintf("flushing q\n");
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
	}
	if (info->p & 0x1) {
//		SysPrintf("flushing p\n");
		MOV32MtoR(EAX, (u32)&VU->p);
		MOV32RtoM((u32)&VU->VI[REG_P].UL, EAX);
	}

	/*if (info->statusflag & 0x1) {
//		SysPrintf("flushing status flag %x\n", info->statusflag);
		MOV32MtoR(EAX, (u32)&VU->statusflag);
		MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL, EAX);
	}
	if (info->macflag & 0x1) {
//		SysPrintf("flushing mac flag\n");
		MOV32MtoR(EAX, (u32)&VU->macflag);
		MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL, EAX);
	}
	if (info->clipflag & 0x1) {
//		SysPrintf("flushing clip flag\n");
		MOV32MtoR(EAX, (u32)&VU->clipflag);
		MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL, EAX);
	}*/
}

static void iDumpBlock(char * ptr) {
	FILE *f;
	char command[ 256 ];
	char filename[ 256 ];
	u32 *mem;
	u32 i;

#ifdef __WIN32__
	CreateDirectory("dumps", NULL);
	sprintf( filename, "dumps\\dump%.8X.txt", VU0.VI[ REG_TPC ].UL );
#else
	mkdir("dumps", 0755);
	sprintf( filename, "dumps/dump%.8X.txt", VU0.VI[ REG_TPC ].UL );
#endif
	SysPrintf( "dump1 %x => %x (%s)\n", VU0.VI[ REG_TPC ].UL, pc, filename );
//	sprintf( filename, "dump.txt");

#ifndef __x86_64__
#if 0
	for ( i = VU0.VI[ REG_TPC ].UL; i < pc; i += 8 ) {
		mem = (u32*)&VU0.Micro[ i ]; 
		SysPrintf( "Upper: %s\n", disVU0MicroUF( mem[ 1 ], i + 4 ) );
		if ( mem[ 1 ] & 0x80000000 ) { /* I flag */ 
			SysPrintf( "Lower: Immediate %x\n", mem[ 0 ] );
		} else {
			SysPrintf( "Lower: %s\n", disVU0MicroLF( mem[ 0 ], i ) );
		}
	}
#endif
#endif

	fflush( stdout );
	f = fopen( "dump1", "wb" );
	fwrite( ptr, 1, (u32)x86Ptr - (u32)ptr, f );
	fclose( f );

#ifdef __x86_64__
	sprintf( command, "objdump -D --target=binary --architecture=i386:x86-64 dump1 > %s", filename );
#else
	sprintf( command, "objdump -D --target=binary --architecture=i386 dump1 > %s", filename );
#endif
	system( command );

#ifndef __x86_64__
	f = fopen( filename, "at" );
	fprintf( f, "\n\n" );
	for ( i = VU0.VI[REG_TPC].UL; i < pc; i += 8 ) {
		mem = (u32*)&VU0.Micro[i]; 
		fprintf(f, "Upper: %s\n", disVU0MicroUF( mem[1], i+4 ) );
		fprintf(f, "Lower: %s\n", disVU0MicroLF( mem[0], i ) );
	}
	fprintf(f, "\n---------------\n");
	fclose( f );
#endif
}

u32 _mxcsrvu0 = 0x7F80;
u32 _mxcsroldvu0;

void VU0RecompileBlock(void) {

	char *ptr;
	int i; 
 
	/* if recPtr reached the mem limit reset whole mem */ 
	if ( ( (u32)recPtrVU0 - (u32)recMemVU0 ) >= (0x800000 - 0x8000) ) { 
		recResetVU0(); 
	} 

	recPtrVU0 += 4;
	x86SetPtr(recPtrVU0); 
	recResetFlags( );

//	SysPrintf("recompile %x\n", VU0.VI[ REG_TPC ].UL );
	*(u32*)&recVU0[ VU0.VI[ REG_TPC ].UL ] = (u32)x86Ptr; 
	ptr = x86Ptr;
	pc = VU0.VI[ REG_TPC ].UL; 
	branch = 0; 
	vucycleold = vucycle;

	memset(VU->fmac,0,sizeof(VU->fmac));
	memset(&VU->fdiv,0,sizeof(VU->fdiv));
	memset(&VU->efu,0,sizeof(VU->efu));

	memset(_opinfo, 0, sizeof(_opinfo));

#ifdef _DEBUG
	// save/restore ESI since VC++ uses it for debugging
	PUSH32R(ESI);
#endif

	for (i=0; i<256; i++) { 
		_vurecAnalyzeOp(&VU0, &_opinfo[i]);
 
		if (branch) { i++; break; }
	}
	if (branch != 0) {
		_vurecAnalyzeOp(&VU0, &_opinfo[i++]);
	}

	_vurecAnalyzeBlock(&VU0, _opinfo, i);

	pc = VU0.VI[ REG_TPC ].UL; 
	branch = 0; 

	_initXMMregs();
	//SSE_STMXCSR((u32)&_mxcsrold);
	SSE_STMXCSR((u32)recPtrVU0-4);
	SSE_LDMXCSR((u32)&_mxcsr);
	MOV32ItoM((u32)&VU0.branch, 0);

	recMacStop = 0;
	recStatusStop = 0;
	recClipStop = 0;
	recQstop = 0;

	CMP32ItoM((u32)&vu0recpcold,(u32)-1);
	j8Ptr[0] = JNE8(0);

	MOV32ItoM((u32)&recQpending,0);
	j8Ptr[1] = JMP8(0);

	x86SetJ8(j8Ptr[0]);
	CMP32ItoM((u32)&recQpending,0);
	j8Ptr[2] = JG8(0);
	MOV32ItoR(EAX,(u32)recVU0Q);
	ADD32MtoR(EAX,(u32)&vu0recpcold);
	MOV32RmtoR(EAX,EAX);
	MOV32RtoM((u32)&recQpending,EAX);
	x86SetJ8(j8Ptr[2]);

	x86SetJ8(j8Ptr[1]);

	for (i=0; i<255; i++) {
		if (pc >= VU0.maxmicro) break;
		VU0Recompile(&_opinfo[i],i);

		if (branch) { i++; break; }
	}

	if( branch & 8 ) {
		
		VU0Recompile(&_opinfo[i++],i);

		// check if didn't come from a branch
		switch( (branch&7) ) {
			case 0:
				MOV32ItoM( (u32)&VU0.VI[ REG_TPC ].UL, pc ); 
				break;
			case 1:
				MOV32MtoR( EAX, (u32)&VU0.branchpc ); 
				MOV32RtoM( (u32)&VU0.VI[ REG_TPC ].UL, EAX );
				break;
			case 3:
				break;
		}
	}
	else {
		
		if (branch == 3) {
			VU0Recompile(&_opinfo[i++],i);
		} else
		if (branch == 0) { 
			MOV32ItoM( (u32)&VU0.VI[ REG_TPC ].UL, pc ); 
		} else {
			VU0Recompile(&_opinfo[i++],i);

			CMP32ItoM( (u32)&VU0.branch, 2);
			j8Ptr[0] = JNE8(0);

			MOV32MtoR( EAX, (u32)&VU0.branchpc ); 
			MOV32RtoM( (u32)&VU0.VI[ REG_TPC ].UL, EAX );
			j8Ptr[1] = JMP8(0);

			x86SetJ8( j8Ptr[ 0 ] );
			MOV32ItoM( (u32)&VU0.VI[ REG_TPC ].UL, pc ); 

			x86SetJ8( j8Ptr[ 1 ] );
		}
	}

	ADD32ItoM((u32)&VU->cycle,vucycle-vucycleold);
	MOV32ItoM((u32)&vu0recpcold,VU0.VI[ REG_TPC ].UL);
	MOV32ItoM((u32)&vu0reccountold,i);

	_freeXMMregs();

	SSE_LDMXCSR((u32)recPtrVU0-4);

#ifdef _DEBUG
	POP32R(ESI);
#endif

	RET();

	recPtrVU0 = x86Ptr; 
}


void recRecompileVU0( void )
{
	VU0RecompileBlock( );
}

extern void logtime(int);

void recExecuteVU0Block( void )
{
	//SysPrintf("executeVU0 %x\n", VU0.VI[ REG_TPC ].UL);

	if ( CHECK_VU0REC) {
		//_controlfp(_MCW_PC, _PC_24);
		executeVU0( ); 
		//_controlfp(_MCW_PC, 0);
	}
	else {
		intExecuteVU0Block();
	}
}

void recClearVU0( u32 Addr, u32 Size )
{
	if( CHECK_VU0REC) {
		memset( (void*)&recVU0[Addr], 0, Size * 4 );
	}
}

void recVU0_UPPER_FD_00( void )
{ 
	recVU0_UPPER_FD_00_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_01( void ) 
{ 
	recVU0_UPPER_FD_01_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_10( void )
{ 
	recVU0_UPPER_FD_10_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0_UPPER_FD_11( void )
{ 
	recVU0_UPPER_FD_11_TABLE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP( void )
{ 
	recVU0LowerOP_OPCODE[ VU0.code & 0x3f ]( ); 
} 
 
void recVU0LowerOP_T3_00( void )
{ 
	recVU0LowerOP_T3_00_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_01( void )
{ 
	recVU0LowerOP_T3_01_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_10( void )
{ 
	recVU0LowerOP_T3_10_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU0LowerOP_T3_11( void )
{ 
	recVU0LowerOP_T3_11_OPCODE[ ( VU0.code >> 6 ) & 0x1f ]( ); 
}


void recVU0unknown( void )
{ 
#ifdef CPU_LOG
	CPU_LOG("Unknown VU0 micromode opcode calledn"); 
#endif
}  
 
 
 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 

#ifdef RECOMPILE_VUMI_ABS
void recVU0MI_ABS()   { recVUMI_ABS(VU); } 
#else
void recVU0MI_ABS()   { rec_vuABS(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_ADD
void recVU0MI_ADD()  { rec_vuADD(VU0); /*recVUMI_ADD(VU);*/ } 
void recVU0MI_ADDi() { recVUMI_ADDi(VU); } 
void recVU0MI_ADDq() { recVUMI_ADDq(VU); } 
void recVU0MI_ADDx() { recVUMI_ADDx(VU); } 
void recVU0MI_ADDy() { recVUMI_ADDy(VU); } 
void recVU0MI_ADDz() { recVUMI_ADDz(VU); } 
void recVU0MI_ADDw() { recVUMI_ADDw(VU); } 
#else
void recVU0MI_ADD()   { rec_vuADD(VU0); } 
void recVU0MI_ADDi()  { rec_vuADDi(VU0); } 
void recVU0MI_ADDq()  { rec_vuADDq(VU0); } 
void recVU0MI_ADDx()  { rec_vuADDx(VU0); } 
void recVU0MI_ADDy()  { rec_vuADDy(VU0); } 
void recVU0MI_ADDz()  { rec_vuADDz(VU0); } 
void recVU0MI_ADDw()  { rec_vuADDw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_ADDA
void recVU0MI_ADDA()  { recVUMI_ADDA(VU); } 
void recVU0MI_ADDAi() { recVUMI_ADDAi(VU); } 
void recVU0MI_ADDAq() { recVUMI_ADDAq(VU); } 
void recVU0MI_ADDAx() { recVUMI_ADDAx(VU); } 
void recVU0MI_ADDAy() { recVUMI_ADDAy(VU); } 
void recVU0MI_ADDAz() { recVUMI_ADDAz(VU); } 
void recVU0MI_ADDAw() { recVUMI_ADDAw(VU); } 
#else
void recVU0MI_ADDA()  { rec_vuADDA(VU0); } 
void recVU0MI_ADDAi() { rec_vuADDAi(VU0); } 
void recVU0MI_ADDAq() { rec_vuADDAq(VU0); } 
void recVU0MI_ADDAx() { rec_vuADDAx(VU0); } 
void recVU0MI_ADDAy() { rec_vuADDAy(VU0); } 
void recVU0MI_ADDAz() { rec_vuADDAz(VU0); } 
void recVU0MI_ADDAw() { rec_vuADDAw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_SUB
void recVU0MI_SUB()  { recVUMI_SUB(VU); } 
void recVU0MI_SUBi() { recVUMI_SUBi(VU); } 
void recVU0MI_SUBq() { recVUMI_SUBq(VU); } 
void recVU0MI_SUBx() { recVUMI_SUBx(VU); } 
void recVU0MI_SUBy() { recVUMI_SUBy(VU); } 
void recVU0MI_SUBz() { recVUMI_SUBz(VU); } 
void recVU0MI_SUBw() { recVUMI_SUBw(VU); } 
#else
void recVU0MI_SUB()   { rec_vuSUB(VU0); } 
void recVU0MI_SUBi()  { rec_vuSUBi(VU0); } 
void recVU0MI_SUBq()  { rec_vuSUBq(VU0); } 
void recVU0MI_SUBx()  { rec_vuSUBx(VU0); } 
void recVU0MI_SUBy()  { rec_vuSUBy(VU0); } 
void recVU0MI_SUBz()  { rec_vuSUBz(VU0); } 
void recVU0MI_SUBw()  { rec_vuSUBw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_SUBA
void recVU0MI_SUBA()  { recVUMI_SUBA(VU); } 
void recVU0MI_SUBAi() { recVUMI_SUBAi(VU); } 
void recVU0MI_SUBAq() { recVUMI_SUBAq(VU); } 
void recVU0MI_SUBAx() { recVUMI_SUBAx(VU); } 
void recVU0MI_SUBAy() { recVUMI_SUBAy(VU); } 
void recVU0MI_SUBAz() { recVUMI_SUBAz(VU); } 
void recVU0MI_SUBAw() { recVUMI_SUBAw(VU); } 
#else
void recVU0MI_SUBA()  { rec_vuSUBA(VU0); } 
void recVU0MI_SUBAi() { rec_vuSUBAi(VU0); } 
void recVU0MI_SUBAq() { rec_vuSUBAq(VU0); } 
void recVU0MI_SUBAx() { rec_vuSUBAx(VU0); } 
void recVU0MI_SUBAy() { rec_vuSUBAy(VU0); } 
void recVU0MI_SUBAz() { rec_vuSUBAz(VU0); } 
void recVU0MI_SUBAw() { rec_vuSUBAw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MUL
void recVU0MI_MUL()  { recVUMI_MUL(VU); } 
void recVU0MI_MULi() { recVUMI_MULi(VU); } 
void recVU0MI_MULq() { rec_vuMULq(VU0); } //{ recVUMI_MULq(VU); }  // Causes FFX to hang
void recVU0MI_MULx() { recVUMI_MULx(VU); } 
void recVU0MI_MULy() { recVUMI_MULy(VU); } 
void recVU0MI_MULz() { recVUMI_MULz(VU); } 
void recVU0MI_MULw() { recVUMI_MULw(VU); } 
#else
void recVU0MI_MUL()   { rec_vuMUL(VU0); } 
void recVU0MI_MULi()  { rec_vuMULi(VU0); } 
void recVU0MI_MULq()  { rec_vuMULq(VU0); } 
void recVU0MI_MULx()  { rec_vuMULx(VU0); } 
void recVU0MI_MULy()  { rec_vuMULy(VU0); } 
void recVU0MI_MULz()  { rec_vuMULz(VU0); } 
void recVU0MI_MULw()  { rec_vuMULw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MULA
void recVU0MI_MULA()  { recVUMI_MULA(VU); } 
void recVU0MI_MULAi() { recVUMI_MULAi(VU); } 
void recVU0MI_MULAq() { recVUMI_MULAq(VU); } 
void recVU0MI_MULAx() { recVUMI_MULAx(VU); } 
void recVU0MI_MULAy() { recVUMI_MULAy(VU); } 
void recVU0MI_MULAz() { recVUMI_MULAz(VU); } 
void recVU0MI_MULAw() { recVUMI_MULAw(VU); } 
#else
void recVU0MI_MULA()  { rec_vuMULA(VU0); } 
void recVU0MI_MULAi() { rec_vuMULAi(VU0); } 
void recVU0MI_MULAq() { rec_vuMULAq(VU0); } 
void recVU0MI_MULAx() { rec_vuMULAx(VU0); } 
void recVU0MI_MULAy() { rec_vuMULAy(VU0); } 
void recVU0MI_MULAz() { rec_vuMULAz(VU0); } 
void recVU0MI_MULAw() { rec_vuMULAw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MADD
void recVU0MI_MADD()  { recVUMI_MADD(VU); } 
void recVU0MI_MADDi() { recVUMI_MADDi(VU); } 
void recVU0MI_MADDq() { recVUMI_MADDq(VU); } 
void recVU0MI_MADDx() { recVUMI_MADDx(VU); } 
void recVU0MI_MADDy() { recVUMI_MADDy(VU); } 
void recVU0MI_MADDz() { recVUMI_MADDz(VU); } 
void recVU0MI_MADDw() { recVUMI_MADDw(VU); } 
#else
void recVU0MI_MADD()  { rec_vuMADD(VU0); } 
void recVU0MI_MADDi() { rec_vuMADDi(VU0); } 
void recVU0MI_MADDq() { rec_vuMADDq(VU0); } 
void recVU0MI_MADDx() { rec_vuMADDx(VU0); } 
void recVU0MI_MADDy() { rec_vuMADDy(VU0); } 
void recVU0MI_MADDz() { rec_vuMADDz(VU0); } 
void recVU0MI_MADDw() { rec_vuMADDw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MADDA
void recVU0MI_MADDA()  { recVUMI_MADDA(VU); } 
void recVU0MI_MADDAi() { recVUMI_MADDAi(VU); } 
void recVU0MI_MADDAq() { recVUMI_MADDAq(VU); } 
void recVU0MI_MADDAx() { recVUMI_MADDAx(VU); } 
void recVU0MI_MADDAy() { recVUMI_MADDAy(VU); } 
void recVU0MI_MADDAz() { recVUMI_MADDAz(VU); } 
void recVU0MI_MADDAw() { recVUMI_MADDAw(VU); } 
#else
void recVU0MI_MADDA()  { rec_vuMADDA(VU0); } 
void recVU0MI_MADDAi() { rec_vuMADDAi(VU0); } 
void recVU0MI_MADDAq() { rec_vuMADDAq(VU0); } 
void recVU0MI_MADDAx() { rec_vuMADDAx(VU0); } 
void recVU0MI_MADDAy() { rec_vuMADDAy(VU0); } 
void recVU0MI_MADDAz() { rec_vuMADDAz(VU0); } 
void recVU0MI_MADDAw() { rec_vuMADDAw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MSUB
void recVU0MI_MSUB()  { recVUMI_MSUB(VU); } 
void recVU0MI_MSUBi() { recVUMI_MSUBi(VU); } 
void recVU0MI_MSUBq() { recVUMI_MSUBq(VU); } 
void recVU0MI_MSUBx() { recVUMI_MSUBx(VU); } 
void recVU0MI_MSUBy() { recVUMI_MSUBy(VU); } 
void recVU0MI_MSUBz() { recVUMI_MSUBz(VU); } 
void recVU0MI_MSUBw() { recVUMI_MSUBw(VU); } 
#else
void recVU0MI_MSUB()  { rec_vuMSUB(VU0); } 
void recVU0MI_MSUBi() { rec_vuMSUBi(VU0); } 
void recVU0MI_MSUBq() { rec_vuMSUBq(VU0); } 
void recVU0MI_MSUBx() { rec_vuMSUBx(VU0); } 
void recVU0MI_MSUBy() { rec_vuMSUBy(VU0); } 
void recVU0MI_MSUBz() { rec_vuMSUBz(VU0); } 
void recVU0MI_MSUBw() { rec_vuMSUBw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MSUBA
void recVU0MI_MSUBA()  { recVUMI_MSUBA(VU); } 
void recVU0MI_MSUBAi() { recVUMI_MSUBAi(VU); } 
void recVU0MI_MSUBAq() { recVUMI_MSUBAq(VU); } 
void recVU0MI_MSUBAx() { recVUMI_MSUBAx(VU); } 
void recVU0MI_MSUBAy() { recVUMI_MSUBAy(VU); } 
void recVU0MI_MSUBAz() { recVUMI_MSUBAz(VU); } 
void recVU0MI_MSUBAw() { recVUMI_MSUBAw(VU); } 
#else
void recVU0MI_MSUBA()  { rec_vuMSUBA(VU0); } 
void recVU0MI_MSUBAi() { rec_vuMSUBAi(VU0); } 
void recVU0MI_MSUBAq() { rec_vuMSUBAq(VU0); } 
void recVU0MI_MSUBAx() { rec_vuMSUBAx(VU0); } 
void recVU0MI_MSUBAy() { rec_vuMSUBAy(VU0); } 
void recVU0MI_MSUBAz() { rec_vuMSUBAz(VU0); } 
void recVU0MI_MSUBAw() { rec_vuMSUBAw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MAX
void recVU0MI_MAX()  { recVUMI_MAX(VU); } 
void recVU0MI_MAXi() { recVUMI_MAXi(VU); } 
void recVU0MI_MAXx() { recVUMI_MAXx(VU); } 
void recVU0MI_MAXy() { recVUMI_MAXy(VU); } 
void recVU0MI_MAXz() { recVUMI_MAXz(VU); } 
void recVU0MI_MAXw() { recVUMI_MAXw(VU); } 
#else
void recVU0MI_MAX()  { rec_vuMAX(VU0); } 
void recVU0MI_MAXi() { rec_vuMAXi(VU0); } 
void recVU0MI_MAXx() { rec_vuMAXx(VU0); } 
void recVU0MI_MAXy() { rec_vuMAXy(VU0); } 
void recVU0MI_MAXz() { rec_vuMAXz(VU0); } 
void recVU0MI_MAXw() { rec_vuMAXw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_MINI
void recVU0MI_MINI()  { recVUMI_MINI(VU); } 
void recVU0MI_MINIi() { recVUMI_MINIi(VU); } 
void recVU0MI_MINIx() { recVUMI_MINIx(VU); } 
void recVU0MI_MINIy() { recVUMI_MINIy(VU); } 
void recVU0MI_MINIz() { recVUMI_MINIz(VU); } 
void recVU0MI_MINIw() { recVUMI_MINIw(VU); }
#else
void recVU0MI_MINI()  { rec_vuMINI(VU0); } 
void recVU0MI_MINIi() { rec_vuMINIi(VU0); } 
void recVU0MI_MINIx() { rec_vuMINIx(VU0); } 
void recVU0MI_MINIy() { rec_vuMINIy(VU0); } 
void recVU0MI_MINIz() { rec_vuMINIz(VU0); } 
void recVU0MI_MINIw() { rec_vuMINIw(VU0); } 
#endif

#ifdef RECOMPILE_VUMI_FTOI
void recVU0MI_FTOI0()  { recVUMI_FTOI0(VU); }
void recVU0MI_FTOI4()  { recVUMI_FTOI4(VU); } 
void recVU0MI_FTOI12() { recVUMI_FTOI12(VU); } 
void recVU0MI_FTOI15() { recVUMI_FTOI15(VU); } 
void recVU0MI_ITOF0()  { rec_vuITOF0(VU0); } 
void recVU0MI_ITOF4()  { rec_vuITOF4(VU0); } 
void recVU0MI_ITOF12() { rec_vuITOF12(VU0); } 
void recVU0MI_ITOF15() { rec_vuITOF15(VU0); } 
#else
void recVU0MI_FTOI0()  { rec_vuFTOI0(VU0); } 
void recVU0MI_FTOI4()  { rec_vuFTOI4(VU0); } 
void recVU0MI_FTOI12() { rec_vuFTOI12(VU0); } 
void recVU0MI_FTOI15() { rec_vuFTOI15(VU0); } 
void recVU0MI_ITOF0()  { rec_vuITOF0(VU0); } 
void recVU0MI_ITOF4()  { rec_vuITOF4(VU0); } 
void recVU0MI_ITOF12() { rec_vuITOF12(VU0); } 
void recVU0MI_ITOF15() { rec_vuITOF15(VU0); } 
#endif

//void recVU0MI_OPMULA() { rec_vuOPMULA(VU0); } 
//void recVU0MI_OPMSUB() { rec_vuOPMSUB(VU0); } 
void recVU0MI_OPMULA() { recVUMI_OPMULA(VU); } 
void recVU0MI_OPMSUB() { recVUMI_OPMSUB(VU); } 
void recVU0MI_NOP()    { } 
void recVU0MI_CLIP()
{ 
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_CLIP(VU);
	else
		rec_vuCLIP(VU0);
}



/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 

#ifdef RECOMPILE_VUMI_MISC

void recVU0MI_MTIR()    { recVUMI_MTIR(VU); }
void recVU0MI_MR32()    { recVUMI_MR32(VU); } 
void recVU0MI_MFIR()    { recVUMI_MFIR(VU); }
void recVU0MI_MOVE()    { recVUMI_MOVE(VU); } 
void recVU0MI_WAITQ()   { } 
void recVU0MI_MFP()     { recVUMI_MFP(VU); } 
void recVU0MI_WAITP()   { SysPrintf("vu0 wait p?\n"); } 

#else

void recVU0MI_MOVE()    { rec_vuMOVE(VU0); } 
void recVU0MI_MFIR()    { rec_vuMFIR(VU0); } 
void recVU0MI_MTIR()    { rec_vuMTIR(VU0); } 
void recVU0MI_MR32()    { rec_vuMR32(VU0); } 
void recVU0MI_WAITQ()   { } 
void recVU0MI_MFP()     { rec_vuMFP(VU0); } 
void recVU0MI_WAITP()   { rec_vuWAITP(VU0); } 

#endif

#ifdef RECOMPILE_VUMI_MATH

void recVU0MI_SQRT()
{ 
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_SQRT(VU);
	else
		rec_vuSQRT(VU0);
}

void recVU0MI_RSQRT()
{
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_RSQRT(VU);
	else
		rec_vuRSQRT(VU0);
}

void recVU0MI_DIV()
{
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_DIV(VU);
	else
		rec_vuDIV(VU0);
}

#else

void recVU0MI_DIV()     { rec_vuDIV(VU0);} 
void recVU0MI_SQRT()    { rec_vuSQRT(VU0); } 
void recVU0MI_RSQRT()   { rec_vuRSQRT(VU0); } 

#endif

#ifdef RECOMPILE_VUMI_E

void recVU0MI_ESADD()   { rec_vuESADD(VU0); } 
void recVU0MI_ERSADD()  { rec_vuERSADD(VU0); } 
void recVU0MI_ELENG()   { recVUMI_ELENG(VU); } 
void recVU0MI_ERLENG()  { rec_vuERLENG(VU0); } 
void recVU0MI_EATANxy() { rec_vuEATANxy(VU0); } 
void recVU0MI_EATANxz() { rec_vuEATANxz(VU0); } 
void recVU0MI_ESUM()    { rec_vuESUM(VU0); } 
void recVU0MI_ERCPR()   { rec_vuERCPR(VU0); } 
void recVU0MI_ESQRT()   { rec_vuESQRT(VU0); } 
void recVU0MI_ERSQRT()  { rec_vuERSQRT(VU0); } 
void recVU0MI_ESIN()    { rec_vuESIN(VU0); } 
void recVU0MI_EATAN()   { rec_vuEATAN(VU0); } 
void recVU0MI_EEXP()    { rec_vuEEXP(VU0); } 

#else

void recVU0MI_ESADD()   { rec_vuESADD(VU0); } 
void recVU0MI_ERSADD()  { rec_vuERSADD(VU0); } 
void recVU0MI_ELENG()   { rec_vuELENG(VU0); } 
void recVU0MI_ERLENG()  { rec_vuERLENG(VU0); } 
void recVU0MI_EATANxy() { rec_vuEATANxy(VU0); } 
void recVU0MI_EATANxz() { rec_vuEATANxz(VU0); } 
void recVU0MI_ESUM()    { rec_vuESUM(VU0); } 
void recVU0MI_ERCPR()   { rec_vuERCPR(VU0); } 
void recVU0MI_ESQRT()   { rec_vuESQRT(VU0); } 
void recVU0MI_ERSQRT()  { rec_vuERSQRT(VU0); } 
void recVU0MI_ESIN()    { rec_vuESIN(VU0); } 
void recVU0MI_EATAN()   { rec_vuEATAN(VU0); } 
void recVU0MI_EEXP()    { rec_vuEEXP(VU0); } 

#endif

#ifdef RECOMPILE_VUMI_X

void recVU0MI_XITOP()   { recVUMI_XITOP(VU); }
void recVU0MI_XGKICK()  { recVUMI_XGKICK(VU); }
void recVU0MI_XTOP()    { recVUMI_XTOP(VU); }

#else

void recVU0MI_XITOP()   { rec_vuXITOP(VU0); }
void recVU0MI_XGKICK()  { REC_VUOP(VU0, XGKICK);}
void recVU0MI_XTOP()    { REC_VUOP(VU0, XTOP);}

#endif

#ifdef RECOMPILE_VUMI_RANDOM

void recVU0MI_RINIT()     { recVUMI_RINIT(VU); }
void recVU0MI_RGET()      { recVUMI_RGET(VU); }
void recVU0MI_RNEXT()     { recVUMI_RNEXT(VU); }
void recVU0MI_RXOR()      { recVUMI_RXOR(VU); }

#else

void recVU0MI_RINIT()     { rec_vuRINIT(VU0); }
void recVU0MI_RGET()      { rec_vuRGET(VU0); }
void recVU0MI_RNEXT()     { rec_vuRNEXT(VU0); }
void recVU0MI_RXOR()      { rec_vuRXOR(VU0); }

#endif

#ifdef RECOMPILE_VUMI_FLAG

void recVU0MI_FSAND()   { recVUMI_FSAND(VU); } 
void recVU0MI_FSEQ()    { recVUMI_FSEQ(VU); }
void recVU0MI_FSOR()    { recVUMI_FSOR(VU); }
void recVU0MI_FSSET()   { recVUMI_FSSET(VU); } 
void recVU0MI_FMEQ()    { recVUMI_FMEQ(VU); }
void recVU0MI_FMOR()    { recVUMI_FMOR(VU); }
void recVU0MI_FCEQ()    { recVUMI_FCEQ(VU); }
void recVU0MI_FCOR()    { recVUMI_FCOR(VU); }
void recVU0MI_FCSET()   { recVUMI_FCSET(VU); }
void recVU0MI_FCGET()   { recVUMI_FCGET(VU); }
void recVU0MI_FCAND()   { recVUMI_FCAND(VU); }
void recVU0MI_FMAND()   { recVUMI_FMAND(VU); }

#else

void recVU0MI_FSAND()   { rec_vuFSAND(VU0); } 
void recVU0MI_FSEQ()    { rec_vuFSEQ(VU0); } 
void recVU0MI_FSOR()    { rec_vuFSOR(VU0); } 
void recVU0MI_FSSET()   { rec_vuFSSET(VU0); } 
void recVU0MI_FMAND()   { rec_vuFMAND(VU0); } 
void recVU0MI_FMEQ()    { rec_vuFMEQ(VU0); } 
void recVU0MI_FMOR()    { rec_vuFMOR(VU0); } 
void recVU0MI_FCAND()   { rec_vuFCAND(VU0); } 
void recVU0MI_FCEQ()    { rec_vuFCEQ(VU0); } 
void recVU0MI_FCOR()    { rec_vuFCOR(VU0); } 
void recVU0MI_FCSET()   { rec_vuFCSET(VU0); } 
void recVU0MI_FCGET()   { rec_vuFCGET(VU0); } 

#endif

#ifdef RECOMPILE_VUMI_LOADSTORE

void recVU0MI_LQ()      { recVUMI_LQ(VU); } 
void recVU0MI_LQD()     { recVUMI_LQD(VU); } 
void recVU0MI_LQI()     { recVUMI_LQI(VU); } 
void recVU0MI_SQ()      { recVUMI_SQ(VU); } //causes some corrupt textures/2d
void recVU0MI_SQD()     { recVUMI_SQD(VU); }
void recVU0MI_SQI()     { recVUMI_SQI(VU); }
void recVU0MI_ILW()     { recVUMI_ILW(VU); }  
void recVU0MI_ISW()     { recVUMI_ISW(VU); } //causes corrupt 2d
void recVU0MI_ILWR()    { recVUMI_ILWR(VU); }
void recVU0MI_ISWR()    { recVUMI_ISWR(VU); }

#else

void recVU0MI_LQ()      { rec_vuLQ(VU0); } 
void recVU0MI_LQD()     { rec_vuLQD(VU0); } 
void recVU0MI_LQI()     { rec_vuLQI(VU0); } 
void recVU0MI_SQ()      { rec_vuSQ(VU0); } 
void recVU0MI_SQD()     { rec_vuSQD(VU0); } 
void recVU0MI_SQI()     { rec_vuSQI(VU0); } 
void recVU0MI_ILW()     { rec_vuILW(VU0); } 
void recVU0MI_ISW()     { rec_vuISW(VU0); } 
void recVU0MI_ILWR()    { rec_vuILWR(VU0); } 
void recVU0MI_ISWR()    { rec_vuISWR(VU0); } 

#endif

#ifdef RECOMPILE_VUMI_ARITHMETIC

void recVU0MI_IADD()    { recVUMI_IADD(VU); }
void recVU0MI_IADDI()   { recVUMI_IADDI(VU); }
void recVU0MI_IADDIU()  { recVUMI_IADDIU(VU); } 
void recVU0MI_IOR()     { recVUMI_IOR(VU); }
void recVU0MI_ISUB()    { recVUMI_ISUB(VU); }
void recVU0MI_IAND()    { recVUMI_IAND(VU); }
void recVU0MI_ISUBIU()  { recVUMI_ISUBIU(VU); } 

#else

void recVU0MI_IADD()    { rec_vuIADD(VU0); }
void recVU0MI_IADDI()   { rec_vuIADDI(VU0); }
void recVU0MI_IADDIU()  { rec_vuIADDIU(VU0); } 
void recVU0MI_IOR()     { rec_vuIOR(VU0); }
void recVU0MI_ISUB()    { rec_vuISUB(VU0); }
void recVU0MI_IAND()    { rec_vuIAND(VU0); }
void recVU0MI_ISUBIU()  { rec_vuISUBIU(VU0); } 

#endif                                

#ifdef RECOMPILE_VUMI_BRANCH

void recVU0MI_IBEQ()    { recVUMI_IBEQ(VU); } 
void recVU0MI_IBGEZ()   { recVUMI_IBGEZ(VU); } 
void recVU0MI_IBLTZ()   { recVUMI_IBLTZ(VU); } 
void recVU0MI_IBLEZ()   { recVUMI_IBLEZ(VU); } 
void recVU0MI_IBGTZ()   { recVUMI_IBGTZ(VU); } 
void recVU0MI_IBNE()    { recVUMI_IBNE(VU); } 
void recVU0MI_B()       { recVUMI_B(VU); } 
void recVU0MI_BAL()     { recVUMI_BAL(VU); } 
void recVU0MI_JR()      { recVUMI_JR(VU); } 
void recVU0MI_JALR()    { recVUMI_JALR(VU); } 

#else

void recVU0MI_IBEQ()    { rec_vuIBEQ(VU0); } 
void recVU0MI_IBGEZ()   { rec_vuIBGEZ(VU0); } 
void recVU0MI_IBGTZ()   { rec_vuIBGTZ(VU0); } 
void recVU0MI_IBLTZ()   { rec_vuIBLTZ(VU0); } 
void recVU0MI_IBLEZ()   { rec_vuIBLEZ(VU0); } 
void recVU0MI_IBNE()    { rec_vuIBNE(VU0); } 
void recVU0MI_B()       { rec_vuB(VU0); } 
void recVU0MI_BAL()     { rec_vuBAL(VU0); } 
void recVU0MI_JR()      { rec_vuJR(VU0); } 
void recVU0MI_JALR()    { rec_vuJALR(VU0); } 

#endif

