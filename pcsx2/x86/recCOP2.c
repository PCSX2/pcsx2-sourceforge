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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "Common.h"
#include "Debug.h"
#include "R5900.h"
#include "InterTables.h"
#include "VU0.h"
#include "VUmicro.h"
#include "VU0.h"
#include "iVU0micro.h"
#include "iVUmicro.h"


#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

extern u32 pc;			/* recompiler pc */
static _vuopinfo info = {0, 0, 1, 0, 0, 0, 0};

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define _Fsf_ ((cpuRegs.code >> 21) & 0x03)
#define _Ftf_ ((cpuRegs.code >> 23) & 0x03)
#define _Cc_ (cpuRegs.code & 0x03)



#define REC_COP2_FUNC(f) \
	void f(); \
	void rec##f() { \
	/*CPU_LOG("Uncompiled COP2:%s\n", disR5900F(cpuRegs.code, cpuRegs.pc));*/\
	COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(); \
	CALLFunc((u32)f); \
}

#define INTERPRETATE_COP2_FUNC(f) \
void recV##f() { \
	/*CPU_LOG("Uncompiled COP2:%s\n", disR5900F(cpuRegs.code, cpuRegs.pc));*/\
	COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(); \
	CALLFunc((u32)V##f); \
}

#define REC_COP2_VU0(f) \
void recV##f() { \
	cinfo = &info; \
	VU0.code = cpuRegs.code; recVU0MI_##f(); \
	_freeXMMregs(&VU0); \
}

#define REC_COP2_VU0MS(f) \
void recV##f() { \
	cinfo = &info; \
	VU0.code = cpuRegs.code; recVU0MI_##f(); \
	_freeXMMregs(&VU0); \
	MOV32MtoR(EAX,(u32)&VU0.macflag); \
	MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL,EAX); \
	MOV32MtoR(EAX,(u32)&VU0.statusflag); \
	MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL,EAX); \
}

#define REC_COP2_VU0Q(f) \
void recV##f() { \
	cinfo = &info; \
	VU0.code = cpuRegs.code; recVU0MI_##f(); \
	_freeXMMregs(&VU0); \
	MOV32MtoR(EAX, (u32)&VU0.q); \
	MOV32RtoM((u32)&VU0.VI[REG_Q].UL, EAX); \
}

#define REC_COP2_VU0CLIP(f) \
void recV##f() { \
	cinfo = &info; \
	VU0.code = cpuRegs.code; recVU0MI_##f(); \
	_freeXMMregs(&VU0); \
	MOV32MtoR(EAX, (u32)&VU0.clipflag); \
	MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL, EAX); \
}


void rec_C2UNK()
{
	SysPrintf("Cop2 bad opcode:%x",cpuRegs.code);
}

void (*recCOP2t[32])();
void (*recCOP2_BC2t[32])();
void (*recCOP2SPECIAL1t[64])();
void (*recCOP2SPECIAL2t[128])();

void recCOP2();
void recCOP2_SPECIAL();
void recCOP2_BC2();
void recCOP2_SPECIAL2();



//////////////////////////////////////////////////////////////////////////
//COP2 
//////////////////////////////////////////////////////////////////////////

//REC_COP2_FUNC(QMFC2);
//REC_COP2_FUNC(QMTC2);
//REC_COP2_FUNC(CFC2);
//REC_COP2_FUNC(CTC2);

static void _recvu0WaitMicro() {
	MOV32MtoR(EAX,(u32)&VU0.VI[REG_VPU_STAT].UL);
	TEST32ItoR(EAX,0x1);
	j8Ptr[0] = JZ8(0);

	OR32ItoM((u32)&VU0.flags,VUFLAG_BREAKONMFLAG);
	AND32ItoM((u32)&VU0.flags,~VUFLAG_MFLAGSET);

	j8Ptr[3] = (u8*)x86Ptr;

	MOV32MtoR(EAX,(u32)&VU0.VI[REG_VPU_STAT].UL);
	TEST32ItoR(EAX,0x1);
	j8Ptr[1] = JZ8(0);
	MOV32MtoR(EAX,(u32)&VU0.flags);
	TEST32ItoR(EAX,VUFLAG_MFLAGSET);
	j8Ptr[2] = JNZ8(0);

	iFlushCall();
	CALLFunc((u32)Cpu->ExecuteVU0Block);
	JMP8((u32)j8Ptr[3] - (u32)x86Ptr - 2);

	x86SetJ8(j8Ptr[2]);
	x86SetJ8(j8Ptr[1]);

	AND32ItoM((u32)&VU0.flags,~VUFLAG_BREAKONMFLAG);

	x86SetJ8(j8Ptr[0]);
}

static void recCFC2()
{
	if (cpuRegs.code & 1) {
		_recvu0WaitMicro();
	}

	if(!_Rt_) return;

	MOV32MtoR(EAX,(u32)&VU0.VI[_Fs_].UL);
	MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);
	if(VU0.VI[_Fs_].UL & 0x80000000)
	MOV32ItoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1], 0xffffffff);
}

static void recCTC2()
{
	if (cpuRegs.code & 1) {
		_recvu0WaitMicro();
	}

	if(!_Fs_) return;

	switch(_Fs_) {
		case REG_MAC_FLAG: // read-only
		case REG_TPC:      // read-only
		case REG_VPU_STAT: // read-only
			break;
		case REG_FBRST:
			MOV32MtoR(EAX,(u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
			AND32ItoR(EAX,0x0C0C);
			MOV32RtoM((u32)&VU0.VI[REG_FBRST].UL,EAX);

			MOV32MtoR(EAX,(u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
			TEST32ItoR(EAX,0x2);
			j8Ptr[0] = JZ8(0);
			iFlushCall();
			CALLFunc((u32)vu0ResetRegs);
			x86SetJ8(j8Ptr[0]);

			MOV32MtoR(EAX,(u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
			TEST32ItoR(EAX,0x200);
			j8Ptr[0] = JZ8(0);
			iFlushCall();
			CALLFunc((u32)vu1ResetRegs);
			x86SetJ8(j8Ptr[0]);
			break;
		case REG_CMSAR1: // REG_CMSAR1
			MOVZX32M16toR(EAX,(u32)&cpuRegs.GPR.r[_Rt_].US[0]);
			MOV32RtoM((u32)&VU1.VI[REG_TPC].UL,EAX);

			PUSH32R(EAX);
			iFlushCall();
			CALLFunc((u32)vu1ExecMicro);	// Execute VU1 Micro SubRoutine
			ADD32ItoR(ESP,4);
			break;
		default:
			MOV32MtoR(EAX,(u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
			MOV32RtoM((u32)&VU0.VI[_Fs_].UL,EAX);
			break;
	}
}

static void recQMFC2(void)
{
	if (cpuRegs.code & 1) {
		_recvu0WaitMicro();
	}

	if(!_Rt_) return;

	CPU_SSE_START;
	SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&VU0.VF[_Fs_].UD[0]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[_Rt_].UD[0],XMM0);
    CPU_SSE_END;

	MOVQMtoR(MM0,(u32)&VU0.VF[_Fs_].UD[0]);
	MOVQMtoR(MM1,(u32)&VU0.VF[_Fs_].UD[1]);
	MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UD[0],MM0);
	MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UD[1],MM1);
	SetMMXstate();
}

static void recQMTC2()
{
	if (cpuRegs.code & 1) {
		_recvu0WaitMicro();
	}

	if(!_Fs_) return;

	CPU_SSE_START;
	SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[_Rt_].UD[0]);
	SSE_MOVAPS_XMM_to_M128((u32)&VU0.VF[_Fs_].UD[0],XMM0);
	CPU_SSE_END;

	MOVQMtoR(MM0,(u32)&cpuRegs.GPR.r[_Rt_].UD[0]);
	MOVQMtoR(MM1,(u32)&cpuRegs.GPR.r[_Rt_].UD[1]);
	MOVQRtoM((u32)&VU0.VF[_Fs_].UD[0],MM0);
	MOVQRtoM((u32)&VU0.VF[_Fs_].UD[1],MM1);
	SetMMXstate();
}


//////////////////////////////////////////////////////////////////////////
//    BC2: Instructions 
//////////////////////////////////////////////////////////////////////////
REC_COP2_FUNC(BC2F);
REC_COP2_FUNC(BC2T);
REC_COP2_FUNC(BC2FL);
REC_COP2_FUNC(BC2TL);

//////////////////////////////////////////////////////////////////////////
//    Special1 instructions 
//////////////////////////////////////////////////////////////////////////
//TODO: redirect all the opcodes to the ivu0micro same functions
REC_COP2_VU0(IADD);
REC_COP2_VU0(IADDI);
REC_COP2_VU0(ISUB);
REC_COP2_VU0(IOR);
REC_COP2_VU0(IAND);
REC_COP2_VU0MS(OPMSUB);
REC_COP2_VU0MS(MADDq);
REC_COP2_VU0MS(MADDi);
REC_COP2_VU0MS(MSUBq);
REC_COP2_VU0MS(MSUBi);
REC_COP2_VU0MS(SUBi);
REC_COP2_VU0MS(SUBq);
REC_COP2_VU0MS(MULi);
REC_COP2_VU0MS(MULq);
REC_COP2_VU0(MAXi);
REC_COP2_VU0(MINIi);
REC_COP2_VU0MS(MUL);
REC_COP2_VU0(MAX);
REC_COP2_VU0MS(MADD);
REC_COP2_VU0MS(MSUB);

REC_COP2_VU0(MINIx);
REC_COP2_VU0(MINIy);
REC_COP2_VU0(MINIz);
REC_COP2_VU0(MINIw);

REC_COP2_VU0(MAXx);
REC_COP2_VU0(MAXy);
REC_COP2_VU0(MAXz);
REC_COP2_VU0(MAXw);

REC_COP2_VU0MS(MULx);
REC_COP2_VU0MS(MULy);
REC_COP2_VU0MS(MULz);
REC_COP2_VU0MS(MULw);

REC_COP2_VU0MS(ADD);
REC_COP2_VU0MS(ADDi);
REC_COP2_VU0MS(ADDq);
REC_COP2_VU0MS(ADDx);
REC_COP2_VU0MS(ADDy);
REC_COP2_VU0MS(ADDz);
REC_COP2_VU0MS(ADDw);

REC_COP2_VU0MS(SUBx);
REC_COP2_VU0MS(SUBy);
REC_COP2_VU0MS(SUBz);
REC_COP2_VU0MS(SUBw);

REC_COP2_VU0MS(MADDx);
REC_COP2_VU0MS(MADDy);
REC_COP2_VU0MS(MADDz);
REC_COP2_VU0MS(MADDw);

REC_COP2_VU0MS(MSUBx);
REC_COP2_VU0MS(MSUBy);
REC_COP2_VU0MS(MSUBz);
REC_COP2_VU0MS(MSUBw);

REC_COP2_VU0MS(SUB);
REC_COP2_VU0(MINI);

//////////////////////////////////////////////////////////////////////////
//  Special2 instructions 
//////////////////////////////////////////////////////////////////////////

REC_COP2_VU0CLIP(CLIP);
REC_COP2_VU0(LQI);
REC_COP2_VU0(SQI);
REC_COP2_VU0(LQD);
REC_COP2_VU0(SQD);
REC_COP2_VU0(MTIR);
REC_COP2_VU0(MFIR);
REC_COP2_VU0(ILWR);
REC_COP2_VU0(ISWR);
REC_COP2_VU0(RINIT);
REC_COP2_VU0(RXOR);
REC_COP2_VU0(RNEXT);
REC_COP2_VU0(RGET);

REC_COP2_VU0(ITOF0);
REC_COP2_VU0(ITOF4);
REC_COP2_VU0(ITOF12);
REC_COP2_VU0(ITOF15);
REC_COP2_VU0(FTOI0);
REC_COP2_VU0(FTOI4);
REC_COP2_VU0(FTOI12);
REC_COP2_VU0(FTOI15);
REC_COP2_VU0MS(MADDA);
REC_COP2_VU0MS(MSUBA);
REC_COP2_VU0MS(MADDAi);
REC_COP2_VU0MS(MADDAq);
REC_COP2_VU0MS(MADDAx);
REC_COP2_VU0MS(MADDAy);
REC_COP2_VU0MS(MADDAz);
REC_COP2_VU0MS(MADDAw);
REC_COP2_VU0MS(MSUBAi);
REC_COP2_VU0MS(MSUBAq);
REC_COP2_VU0MS(MSUBAx);
REC_COP2_VU0MS(MSUBAy);
REC_COP2_VU0MS(MSUBAz);
REC_COP2_VU0MS(MSUBAw);
REC_COP2_VU0MS(ADDAi);
REC_COP2_VU0MS(ADDA);
REC_COP2_VU0MS(SUBA);
REC_COP2_VU0MS(MULA);
REC_COP2_VU0MS(ADDAq);
REC_COP2_VU0MS(ADDAx);
REC_COP2_VU0MS(ADDAy);
REC_COP2_VU0MS(ADDAz);
REC_COP2_VU0MS(ADDAw);
REC_COP2_VU0MS(SUBAi);
REC_COP2_VU0MS(SUBAq);
REC_COP2_VU0MS(SUBAx);
REC_COP2_VU0MS(SUBAy);
REC_COP2_VU0MS(SUBAz);
REC_COP2_VU0MS(SUBAw);
REC_COP2_VU0MS(MULAi);
REC_COP2_VU0MS(MULAq);
REC_COP2_VU0MS(MULAx);
REC_COP2_VU0MS(MULAy);
REC_COP2_VU0MS(MULAz);
REC_COP2_VU0MS(MULAw);
REC_COP2_VU0MS(OPMULA);
REC_COP2_VU0(MOVE);
REC_COP2_VU0Q(DIV);
REC_COP2_VU0Q(SQRT);
REC_COP2_VU0Q(RSQRT);
REC_COP2_VU0(MR32);
REC_COP2_VU0(ABS);

void recVNOP(){}
void recVWAITQ(){}
INTERPRETATE_COP2_FUNC(CALLMS);
INTERPRETATE_COP2_FUNC(CALLMSR);


void (*recCOP2t[32])() = {
    rec_C2UNK,		recQMFC2,       recCFC2,        rec_C2UNK,		rec_C2UNK,		recQMTC2,       recCTC2,        rec_C2UNK,
    recCOP2_BC2,    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,
	recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,recCOP2_SPECIAL,
};

void (*recCOP2_BC2t[32])() = {
    recBC2F,        recBC2T,        recBC2FL,       recBC2TL,       rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
    rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,		rec_C2UNK,
}; 

void (*recCOP2SPECIAL1t[64])() = { 
 recVADDx,       recVADDy,       recVADDz,       recVADDw,       recVSUBx,        recVSUBy,        recVSUBz,        recVSUBw,  
 recVMADDx,      recVMADDy,      recVMADDz,      recVMADDw,      recVMSUBx,       recVMSUBy,       recVMSUBz,       recVMSUBw, 
 recVMAXx,       recVMAXy,       recVMAXz,       recVMAXw,       recVMINIx,       recVMINIy,       recVMINIz,       recVMINIw, 
 recVMULx,       recVMULy,       recVMULz,       recVMULw,       recVMULq,        recVMAXi,        recVMULi,        recVMINIi,
 recVADDq,       recVMADDq,      recVADDi,       recVMADDi,      recVSUBq,        recVMSUBq,       recVSUBi,        recVMSUBi, 
 recVADD,        recVMADD,       recVMUL,        recVMAX,        recVSUB,         recVMSUB,        recVOPMSUB,      recVMINI,  
 recVIADD,       recVISUB,       recVIADDI,      rec_C2UNK,      recVIAND,        recVIOR,         rec_C2UNK,       rec_C2UNK,
 recVCALLMS,     recVCALLMSR,    rec_C2UNK,      rec_C2UNK,      recCOP2_SPECIAL2,recCOP2_SPECIAL2,recCOP2_SPECIAL2,recCOP2_SPECIAL2,  
};

void (*recCOP2SPECIAL2t[128])() = {
 recVADDAx      ,recVADDAy      ,recVADDAz      ,recVADDAw      ,recVSUBAx      ,recVSUBAy      ,recVSUBAz      ,recVSUBAw,
 recVMADDAx     ,recVMADDAy     ,recVMADDAz     ,recVMADDAw     ,recVMSUBAx     ,recVMSUBAy     ,recVMSUBAz     ,recVMSUBAw,
 recVITOF0      ,recVITOF4      ,recVITOF12     ,recVITOF15     ,recVFTOI0      ,recVFTOI4      ,recVFTOI12     ,recVFTOI15,
 recVMULAx      ,recVMULAy      ,recVMULAz      ,recVMULAw      ,recVMULAq      ,recVABS        ,recVMULAi      ,recVCLIP,
 recVADDAq      ,recVMADDAq     ,recVADDAi      ,recVMADDAi     ,recVSUBAq      ,recVMSUBAq     ,recVSUBAi      ,recVMSUBAi,
 recVADDA       ,recVMADDA      ,recVMULA       ,rec_C2UNK      ,recVSUBA       ,recVMSUBA      ,recVOPMULA     ,recVNOP,   
 recVMOVE       ,recVMR32       ,rec_C2UNK      ,rec_C2UNK      ,recVLQI        ,recVSQI        ,recVLQD        ,recVSQD,   
 recVDIV        ,recVSQRT       ,recVRSQRT      ,recVWAITQ      ,recVMTIR       ,recVMFIR       ,recVILWR       ,recVISWR,  
 recVRNEXT      ,recVRGET       ,recVRINIT      ,recVRXOR       ,rec_C2UNK      ,rec_C2UNK      ,rec_C2UNK      ,rec_C2UNK, 
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
 rec_C2UNK      ,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,rec_C2UNK,
};
int opc;

void recCOP22()
{
	recCOP2t[_Rs_]();
}

void recCOP2_SPECIAL()
{
	recCOP2SPECIAL1t[_Funct_]();
}

void recCOP2_BC2()
{
	recCOP2_BC2t[_Rt_]();
}

void recCOP2_SPECIAL2()
{
	int opc=(cpuRegs.code & 0x3) | ((cpuRegs.code >> 4) & 0x7c);
	recCOP2SPECIAL2t[opc]();
}
