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
#include "VUmicro.h"
#include "iVU0micro.h"
#include "iVUmicro.h"


#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

// cycle cycles statusflag macflag clipflag
_vuopinfo g_cop2info = {0, 0, 1, 1, 1, 0, 0};

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define _Fsf_ ((cpuRegs.code >> 21) & 0x03)
#define _Ftf_ ((cpuRegs.code >> 23) & 0x03)
#define _Cc_ (cpuRegs.code & 0x03)

#define REC_COP2_FUNC(f, delreg) \
	void f(); \
	void rec##f() { \
	SysPrintf("cop2 "#f" called\n"); \
	SetFPUstate(); \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(FLUSH_NOCONST); \
	CALLFunc((u32)f); \
	_freeX86regs(); \
	branch = 2; \
}

#define INTERPRETATE_COP2_FUNC(f) \
void recV##f() { \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(FLUSH_EVERYTHING); \
	CALLFunc((u32)V##f); \
	_freeX86regs(); \
}

#define REC_COP2_VU0(f) \
void recV##f() { \
	recVU0MI_##f(); \
	_freeX86regs(); \
}

#define REC_COP2_VU0MS(f) \
void recV##f() { \
	recVU0MI_##f(); \
	_freeX86regs(); \
}

#define REC_COP2_VU0CLIP(f) \
void recV##f() { \
	recVU0MI_##f(); \
	_freeX86regs(); \
}


void rec_C2UNK()
{
	SysPrintf("Cop2 bad opcode:%x\n",cpuRegs.code);
}

void _vuRegs_C2UNK(VURegs * VU, _VURegsNum *VUregsn)
{
	SysPrintf("Cop2 bad _vuRegs code:%x\n",cpuRegs.code);
}

void _vuRegsCOP22(VURegs * VU, _VURegsNum *VUregsn);

void (*recCOP2t[32])();
void (*recCOP2_BC2t[32])();
void (*recCOP2SPECIAL1t[64])();
void (*recCOP2SPECIAL2t[128])();

void recCOP2();
void recCOP2_SPECIAL();
void recCOP2_BC2();
void recCOP2_SPECIAL2();

extern void _vu0WaitMicro();

static void recCFC2()
{
	int mmreg;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((u32)_vu0WaitMicro);
	}

	if(!_Rt_) return;

	_deleteGPRtoXMMreg(_Rt_, 2);
	
	if( (mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_WRITE)) >= 0 ) {
		
		if( _Fs_ >= 16 ) {
			MOVDMtoMMX(mmreg, (u32)&VU0.VI[ _Fs_ ].UL);

			if( EEINST_ISLIVE1(_Rt_) ) {
				_signExtendGPRtoMMX(mmreg, _Rt_, 0);
			}
			else {
				EEINST_RESETHASLIVE1(_Rt_);
			}
		}
		else {
			MOVDMtoMMX(mmreg, (u32)&VU0.VI[ _Fs_ ].UL);
		}
		SetMMXstate();
	}
	else {
		MOV32MtoR(EAX, (u32)&VU0.VI[ _Fs_ ].UL);
		MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);

		if(EEINST_ISLIVE1(_Rt_)) {
			if( _Fs_ < 16 ) {
				// no sign extending
				MOV32ItoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1],0);
			}
			else {
				CDQ();
				MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
			}
		}
		else {
			EEINST_RESETHASLIVE1(_Rt_);
		}
	}

	_eeOnWriteReg(_Rt_, 1);
}

static void recCTC2()
{
	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((u32)_vu0WaitMicro);
	}

	if(!_Fs_) return;

	if( GPR_IS_CONST1(_Rt_) ) {
		switch(_Fs_) {
			case REG_MAC_FLAG: // read-only
			case REG_TPC:      // read-only
			case REG_VPU_STAT: // read-only
				break;
			case REG_FBRST:
				if( g_cpuConstRegs[_Rt_].UL[0] & 2 ) {
					CALLFunc((u32)vu0ResetRegs);
				}

				if( g_cpuConstRegs[_Rt_].UL[0] & 0x200 ) {
					CALLFunc((u32)vu1ResetRegs);
				}

				MOV16ItoM((u32)&VU0.VI[REG_FBRST].UL,g_cpuConstRegs[_Rt_].UL[0]&0x0c0c);
				break;

			case REG_CMSAR1: // REG_CMSAR1

				iFlushCall(FLUSH_NOCONST); // since CALLFunc

				// ignore if VU1 is operating
				TEST32ItoM((u32)&VU0.VI[REG_VPU_STAT].UL, 0x100);
				j8Ptr[0] = JNZ8(0);

				MOV32ItoM((u32)&VU1.VI[REG_TPC].UL, g_cpuConstRegs[_Rt_].UL[0]&0xffff);
				PUSH32I(g_cpuConstRegs[_Rt_].UL[0]&0xffff);
				CALLFunc((u32)vu1ExecMicro);	// Execute VU1 Micro SubRoutine
				ADD32ItoR(ESP,4);

				x86SetJ8( j8Ptr[0] );
				break;
			default:
				MOV32ItoM((u32)&VU0.VI[_Fs_].UL,g_cpuConstRegs[_Rt_].UL[0]);
				break;
		}
	}
	else {
		switch(_Fs_) {
			case REG_MAC_FLAG: // read-only
			case REG_TPC:      // read-only
			case REG_VPU_STAT: // read-only
				break;
			case REG_FBRST:
				_eeMoveGPRtoR(EAX, _Rt_);

				TEST32ItoR(EAX,0x2);
				j8Ptr[0] = JZ8(0);
				CALLFunc((u32)vu0ResetRegs);
				_eeMoveGPRtoR(EAX, _Rt_);
				x86SetJ8(j8Ptr[0]);

				TEST32ItoR(EAX,0x200);
				j8Ptr[0] = JZ8(0);
				CALLFunc((u32)vu1ResetRegs);
				_eeMoveGPRtoR(EAX, _Rt_);
				x86SetJ8(j8Ptr[0]);

				AND32ItoR(EAX,0x0C0C);
				MOV16RtoM((u32)&VU0.VI[REG_FBRST].UL,EAX);
				break;
			case REG_CMSAR1: // REG_CMSAR1

				iFlushCall(FLUSH_NOCONST); // since CALLFunc

				// ignore if VU1 is operating
				TEST32ItoM((u32)&VU0.VI[REG_VPU_STAT].UL, 0x100);
				j8Ptr[0] = JNZ8(0);

				_eeMoveGPRtoR(EAX, _Rt_);
				MOV16RtoM((u32)&VU1.VI[REG_TPC].UL,EAX);

				PUSH32R(EAX);
				CALLFunc((u32)vu1ExecMicro);	// Execute VU1 Micro SubRoutine
				ADD32ItoR(ESP,4);

				x86SetJ8( j8Ptr[0] );
				break;
			default:
				_eeMoveGPRtoM((u32)&VU0.VI[_Fs_].UL,_Rt_);
				break;
		}
	}
}

static void recQMFC2(void)
{
	int t0reg, fsreg;
	_xmmregs temp;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((u32)_vu0WaitMicro);
	}

	if(!_Rt_) return;

	_deleteMMXreg(MMX_GPR+_Rt_, 2);
	_eeOnWriteReg(_Rt_, 0);
	
	// could 'borrow' the reg
	fsreg = _checkXMMreg(XMMTYPE_VFREG, _Fs_, MODE_READ);

	if( fsreg >= 0 ) {
		if( xmmregs[fsreg].mode & MODE_WRITE ) {
			t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
			SSEX_MOVDQA_XMM_to_XMM(t0reg, fsreg);

			// change regs
			temp = xmmregs[t0reg];
			xmmregs[t0reg] = xmmregs[fsreg];
			xmmregs[fsreg] = temp;
		}
		else {
			// swap regs
			t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);

			xmmregs[fsreg] = xmmregs[t0reg];
			xmmregs[t0reg].inuse = 0;
		}
	}
	else {
		t0reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
		SSE_MOVAPS_M128_to_XMM( t0reg, (u32)&VU0.VF[_Fs_].UD[0]);
	}

	_clearNeededXMMregs();
}

static void recQMTC2()
{
	int mmreg, fsreg;

	if (cpuRegs.code & 1) {
		iFlushCall(FLUSH_NOCONST);
		CALLFunc((u32)_vu0WaitMicro);
	}

	if(!_Fs_) return;

	if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ)) >= 0) {
		fsreg = _checkXMMreg(XMMTYPE_VFREG, _Fs_, MODE_WRITE);

		if( fsreg >= 0 ) {

			if( (xmmregs[mmreg].mode&MODE_WRITE) && (g_pCurInstInfo->regs[_Rt_]&(EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)) ) {
				SSE_MOVAPS_XMM_to_XMM(fsreg, mmreg);
			}
			else {
				// swap regs
				if( (xmmregs[mmreg].mode&MODE_WRITE) && (g_pCurInstInfo->regs[_Rt_]&(EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)) )
					SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[_Rt_], mmreg);

				xmmregs[mmreg] = xmmregs[fsreg];
				xmmregs[mmreg].mode = MODE_WRITE;
				xmmregs[fsreg].inuse = 0;
				g_xmmtypes[mmreg] = XMMT_FPS;
			}
		}
		else {

			if( (xmmregs[mmreg].mode&MODE_WRITE) && (g_pCurInstInfo->regs[_Rt_]&(EEINST_LIVE0|EEINST_LIVE1|EEINST_LIVE2)) )
				SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[_Rt_], mmreg);

			// swap regs
			xmmregs[mmreg].type = XMMTYPE_VFREG;
			xmmregs[mmreg].VU = 0;
			xmmregs[mmreg].reg = _Fs_;
			xmmregs[mmreg].mode = MODE_WRITE;
			g_xmmtypes[mmreg] = XMMT_FPS;
		}
	}
	else {
		fsreg = _allocVFtoXMMreg(&VU0, -1, _Fs_, MODE_WRITE);

		if( (mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ)) >= 0) {
			SetMMXstate();
			SSE2_MOVQ2DQ_MM_to_XMM(fsreg, mmreg);
			SSE_MOVHPS_M64_to_XMM(fsreg, (u32)&cpuRegs.GPR.r[_Rt_].UL[2]);
		}
		else {
			
			if( GPR_IS_CONST1( _Rt_ ) ) {
				assert( _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ) == -1 );
				_flushConstReg(_Rt_);	
			}

			SSE_MOVAPS_M128_to_XMM(fsreg, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		}
	}

	_clearNeededXMMregs();
}

void _cop2AnalyzeOp(EEINST* pinst, int dostalls)
{
	_vuRegsCOP22(&VU0, &pinst->vuregs);
	if( !dostalls ) return;

	_recvuTestLowerStalls(&VU0, &pinst->vuregs);
	
	switch(VU0.code & 0x3f) {
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x1d: case 0x1f:
		case 0x2b: case 0x2f:
			break;

		case 0x3c:
			switch ((VU0.code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					pinst->vuregs.VIwrite |= (1<<REG_MAC_FLAG);
					break;
			}
			break;
		case 0x3d:
			switch ((VU0.code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7:
					break;
				default:
					pinst->vuregs.VIwrite |= (1<<REG_MAC_FLAG);
					break;
			}
			break;
		case 0x3e:
			switch ((VU0.code >> 6) & 0x1f) {
				case 0x4: case 0x5:
					break;
				default:
					pinst->vuregs.VIwrite |= (1<<REG_MAC_FLAG);
					break;
			}
			break;
		case 0x3f:
			switch ((VU0.code >> 6) & 0x1f) {
				case 0x4: case 0x5: case 0x7: case 0xb:
					break;
				default:
					pinst->vuregs.VIwrite |= (1<<REG_MAC_FLAG);
					break;
			}
			break;

		default:
			pinst->vuregs.VIwrite |= (1<<REG_MAC_FLAG);
			break;
	}

	if (pinst->vuregs.VIwrite & (1 << REG_CLIP_FLAG)) {
		_recAddWriteBack(vucycle+4, 1<<REG_CLIP_FLAG, pinst);
	}

	if (pinst->vuregs.VIwrite & (1 << REG_Q)) {
		_recAddWriteBack(vucycle+pinst->vuregs.cycles, 1<<REG_Q, pinst);
	}

	pinst->cycle = vucycle;
	_recvuAddLowerStalls(&VU0, &pinst->vuregs);
	_recvuTestPipes(&VU0);

	vucycle++;
}

//////////////////////////////////////////////////////////////////////////
//    BC2: Instructions 
//////////////////////////////////////////////////////////////////////////
REC_COP2_FUNC(BC2F, 0);
REC_COP2_FUNC(BC2T, 0);
REC_COP2_FUNC(BC2FL, 0);
REC_COP2_FUNC(BC2TL, 0);

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
REC_COP2_VU0MS(DIV);
REC_COP2_VU0MS(SQRT);
REC_COP2_VU0MS(RSQRT);
REC_COP2_VU0(MR32);
REC_COP2_VU0(ABS);

void recVNOP(){}
void recVWAITQ(){}
INTERPRETATE_COP2_FUNC(CALLMS);
INTERPRETATE_COP2_FUNC(CALLMSR);

void _vuRegsCOP2_SPECIAL(VURegs * VU, _VURegsNum *VUregsn);
void _vuRegsCOP2_SPECIAL2(VURegs * VU, _VURegsNum *VUregsn);

// information
void _vuRegsQMFC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VFread0 = _Fs_;
	VUregsn->VFr0xyzw= 0xf;
}

void _vuRegsCFC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VIread = 1<<_Fs_;
}

void _vuRegsQMTC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VFwrite = _Fs_;
	VUregsn->VFwxyzw= 0xf;
}

void _vuRegsCTC2(VURegs * VU, _VURegsNum *VUregsn) {
	VUregsn->VIwrite = 1<<_Fs_;
}

void (*_vuRegsCOP2t[32])(VURegs * VU, _VURegsNum *VUregsn) = {
    _vuRegs_C2UNK,		_vuRegsQMFC2,       _vuRegsCFC2,        _vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegsQMTC2,       _vuRegsCTC2,        _vuRegs_C2UNK,
    _vuRegsNOP,    _vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegs_C2UNK,		_vuRegs_C2UNK,
    _vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,
	_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,_vuRegsCOP2_SPECIAL,
};

void (*_vuRegsCOP2SPECIAL1t[64])(VURegs * VU, _VURegsNum *VUregsn) = { 
 _vuRegsADDx,       _vuRegsADDy,       _vuRegsADDz,       _vuRegsADDw,       _vuRegsSUBx,        _vuRegsSUBy,        _vuRegsSUBz,        _vuRegsSUBw,  
 _vuRegsMADDx,      _vuRegsMADDy,      _vuRegsMADDz,      _vuRegsMADDw,      _vuRegsMSUBx,       _vuRegsMSUBy,       _vuRegsMSUBz,       _vuRegsMSUBw, 
 _vuRegsMAXx,       _vuRegsMAXy,       _vuRegsMAXz,       _vuRegsMAXw,       _vuRegsMINIx,       _vuRegsMINIy,       _vuRegsMINIz,       _vuRegsMINIw, 
 _vuRegsMULx,       _vuRegsMULy,       _vuRegsMULz,       _vuRegsMULw,       _vuRegsMULq,        _vuRegsMAXi,        _vuRegsMULi,        _vuRegsMINIi,
 _vuRegsADDq,       _vuRegsMADDq,      _vuRegsADDi,       _vuRegsMADDi,      _vuRegsSUBq,        _vuRegsMSUBq,       _vuRegsSUBi,        _vuRegsMSUBi, 
 _vuRegsADD,        _vuRegsMADD,       _vuRegsMUL,        _vuRegsMAX,        _vuRegsSUB,         _vuRegsMSUB,        _vuRegsOPMSUB,      _vuRegsMINI,  
 _vuRegsIADD,       _vuRegsISUB,       _vuRegsIADDI,      _vuRegs_C2UNK,      _vuRegsIAND,        _vuRegsIOR,         _vuRegs_C2UNK,       _vuRegs_C2UNK,
 _vuRegsNOP,		_vuRegsNOP,    _vuRegs_C2UNK,      _vuRegs_C2UNK,      _vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,_vuRegsCOP2_SPECIAL2,  
};

void (*_vuRegsCOP2SPECIAL2t[128])(VURegs * VU, _VURegsNum *VUregsn) = {
 _vuRegsADDAx      ,_vuRegsADDAy      ,_vuRegsADDAz      ,_vuRegsADDAw      ,_vuRegsSUBAx      ,_vuRegsSUBAy      ,_vuRegsSUBAz      ,_vuRegsSUBAw,
 _vuRegsMADDAx     ,_vuRegsMADDAy     ,_vuRegsMADDAz     ,_vuRegsMADDAw     ,_vuRegsMSUBAx     ,_vuRegsMSUBAy     ,_vuRegsMSUBAz     ,_vuRegsMSUBAw,
 _vuRegsITOF0      ,_vuRegsITOF4      ,_vuRegsITOF12     ,_vuRegsITOF15     ,_vuRegsFTOI0      ,_vuRegsFTOI4      ,_vuRegsFTOI12     ,_vuRegsFTOI15,
 _vuRegsMULAx      ,_vuRegsMULAy      ,_vuRegsMULAz      ,_vuRegsMULAw      ,_vuRegsMULAq      ,_vuRegsABS        ,_vuRegsMULAi      ,_vuRegsCLIP,
 _vuRegsADDAq      ,_vuRegsMADDAq     ,_vuRegsADDAi      ,_vuRegsMADDAi     ,_vuRegsSUBAq      ,_vuRegsMSUBAq     ,_vuRegsSUBAi      ,_vuRegsMSUBAi,
 _vuRegsADDA       ,_vuRegsMADDA      ,_vuRegsMULA       ,_vuRegs_C2UNK      ,_vuRegsSUBA       ,_vuRegsMSUBA      ,_vuRegsOPMULA     ,_vuRegsNOP,   
 _vuRegsMOVE       ,_vuRegsMR32       ,_vuRegs_C2UNK      ,_vuRegs_C2UNK      ,_vuRegsLQI        ,_vuRegsSQI        ,_vuRegsLQD        ,_vuRegsSQD,   
 _vuRegsDIV        ,_vuRegsSQRT       ,_vuRegsRSQRT      ,_vuRegsWAITQ      ,_vuRegsMTIR       ,_vuRegsMFIR       ,_vuRegsILWR       ,_vuRegsISWR,  
 _vuRegsRNEXT      ,_vuRegsRGET       ,_vuRegsRINIT      ,_vuRegsRXOR       ,_vuRegs_C2UNK      ,_vuRegs_C2UNK      ,_vuRegs_C2UNK      ,_vuRegs_C2UNK, 
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
 _vuRegs_C2UNK      ,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,_vuRegs_C2UNK,
};

void _vuRegsCOP22(VURegs * VU, _VURegsNum *VUregsn)
{
	_vuRegsCOP2t[_Rs_](VU, VUregsn);
}

void _vuRegsCOP2_SPECIAL(VURegs * VU, _VURegsNum *VUregsn)
{
	_vuRegsCOP2SPECIAL1t[_Funct_](VU, VUregsn);
}

void _vuRegsCOP2_SPECIAL2(VURegs * VU, _VURegsNum *VUregsn)
{
	int opc=(cpuRegs.code & 0x3) | ((cpuRegs.code >> 4) & 0x7c);
	_vuRegsCOP2SPECIAL2t[opc](VU, VUregsn);
}

// recompilation
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

void recCOP22()
{
	cinfo = &g_cop2info;
	g_VUregs = &g_pCurInstInfo->vuregs;
	VU0.code = cpuRegs.code;
	g_pCurInstInfo->vuregs.pipe = 0xff; // to notify eeVURecompileCode that COP2
	recCOP2t[_Rs_]();
	_freeX86regs();
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
