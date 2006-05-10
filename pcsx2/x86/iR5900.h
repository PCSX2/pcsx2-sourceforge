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

#ifndef __IR5900_H__
#define __IR5900_H__

#include "VU.h"

//#define EMMS_TRACE				//If defined EMMS tracing is ON.

#define ARITHMETICIMM_RECOMPILE  //
#define ARITHMETIC_RECOMPILE		//
#define MULTDIV_RECOMPILE			//
#define SHIFT_RECOMPILE				//
#define BRANCH_RECOMPILE			//
#define JUMP_RECOMPILE           //
#define LOADSTORE_RECOMPILE      //
#define MOVE_RECOMPILE           //
#define MMI_RECOMPILE				//
#define MMI0_RECOMPILE				//
#define MMI1_RECOMPILE				//
#define MMI2_RECOMPILE				//
#define MMI3_RECOMPILE				//
#define FPU_RECOMPILE				//
#define CP0_RECOMPILE				//
#define CP2_RECOMPILE				//

#ifdef __x86_64__
#undef FPU_RECOMPILE
#undef CP2_RECOMPILE
#endif


/*
#undef ARITHMETICIMM_RECOMPILE
#undef ARITHMETIC_RECOMPILE
#undef MULTDIV_RECOMPILE 
#undef SHIFT_RECOMPILE
#undef BRANCH_RECOMPILE
#undef JUMP_RECOMPILE
#undef LOADSTORE_RECOMPILE
#undef MOVE_RECOMPILE
#undef MMI_RECOMPILE
#undef MMI0_RECOMPILE
#undef MMI1_RECOMPILE				
#undef MMI2_RECOMPILE				
#undef MMI3_RECOMPILE				
#undef FPU_RECOMPILE

*/

#define FPU_STATE 0
#define MMX_STATE 1

extern u32 pc;
extern int branch;

extern u32 pc;			         /* recompiler pc */
extern int count;		         /* recompiler intruction count */
extern int branch;		         /* set for branch */
extern u32 target;		         /* branch target */
extern u64 _imm;		         /* temp immediate */
extern u64 _sa;			         /* temp sa */
int x86FpuState;
int iCWstate;

void COUNT_CYCLES(u32 curpc);

#define REC_FUNC( f ) \
   void f( void ); \
   void rec##f( void ) \
   { \
		COUNT_CYCLES(pc); \
	   MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); \
	   MOV32ItoM( (u32)&cpuRegs.pc, pc ); \
	   iFlushCall(); \
	   CALLFunc( (u32)f ); \
   }

#define REC_SYS( f ) \
   void f( void ); \
   void rec##f( void ) \
   { \
	COUNT_CYCLES(pc); \
	   MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); \
	   MOV32ItoM( (u32)&cpuRegs.pc, pc ); \
	   iFlushCall(); \
	   CALLFunc( (u32)f ); \
	   branch = 2; \
   }

#define CPU_3DNOW_START \
	if (cpucaps.has3DNOWInstructionExtensions) \
    { 

#define CPU_3DNOW_END \
	return; \
	} 

#define CPU_SSE_START \
	if (cpucaps.hasStreamingSIMDExtensions) \
    { \

#define CPU_SSE_END \
	return; \
	} 

#define CPU_SSE2_START \
	if (cpucaps.hasStreamingSIMD2Extensions) \
    { \

#define CPU_SSE2_END \
	return; \
	} 
#define EMUREC_SIGN_EXTEND( dest, src )   \
   {                                      \
      MOV32RtoR( dest, src );             \
      SAR32ItoR( dest, 31 );              \
   }

#define SETLINK8( link )	*link = (u32)x86Ptr - (u32)link - 1

#define SETLINK32( link )  *link = (u32)x86Ptr - (u32)link - 4

typedef void (*RECFUNC)( void );

void COMPAREINTREGS32( int r1, int r2, int i1, int i2, int idx );
void recompileNextInstruction(int delayslot);
void SetBranchReg( u32 reg );
void SetBranchImm( u32 imm );
void SetFPUstate();
void SetMMXstate();
void iFlushCall();
void SaveCW();
void LoadCW();
void iRet( BOOL freeRegs );

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP0BC0[32])();
extern void (*recCP0C0[64])();
extern void (*recCP1[32])();
extern void (*recCP1BC1[32])();
extern void (*recCP1S[64])();
extern void (*recCP1W[64])();
extern void (*recMMIt[64])();
extern void (*recMMI0t[32])();
extern void (*recMMI1t[32])();
extern void (*recMMI2t[32])();
extern void (*recMMI3t[32])();

#define MODE_READ   0x1
#define MODE_WRITE  0x2

void _initX86regs();
int  _getFreeX86reg();
int  _allocTempX86reg(int x86reg);
int  _allocGPRtoX86reg(int x86reg, int gprreg, int mode);
void _addNeededGPRtoX86reg(int gprreg);
void _clearNeededX86regs();
void _freeX86reg(int x86reg);
void _freeX86regs();

void _initXMMregs();
int  _getFreeXMMreg();
int  _allocTempXMMreg(int xmmreg);
int  _allocVFtoXMMreg(VURegs *VU, int xmmreg, int vfreg, int mode);
int  _allocFPtoXMMreg(int xmmreg, int fpreg, int mode);
int  _allocACCtoXMMreg(VURegs *VU, int xmmreg, int mode);
int  _allocFPACCtoXMMreg(int xmmreg, int mode);
int  _checkVFtoXMMreg(VURegs* VU, int vfreg);
void _addNeededVFtoXMMreg(int vfreg);
void _addNeededACCtoXMMreg();
void _addNeededFPtoXMMreg(int fpreg);
void _addNeededFPACCtoXMMreg();
void _clearNeededXMMregs();
void _freeXMMreg(int xmmreg);
void _freeXMMregs();


void recError();
void recReset();

typedef struct {
	int inuse;
	int type;
	VURegs *VU;
	int reg;
	int mode;
	int needed;
} _xmmregs;

#define XMMTYPE_TEMP	0
#define XMMTYPE_VFREG	1
#define XMMTYPE_ACC		2
#define XMMTYPE_FPREG	3
#define XMMTYPE_FPACC	4

#ifdef __x86_64__
#define XMMREGS 16
#else
#define XMMREGS 8
#endif

extern _xmmregs xmmregs[XMMREGS];

#endif /* __IR5900_H__ */
