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

// recompiler reworked to add dynamic linking zerofrog(@gmail.com) Jan06

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"


#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

extern void COUNT_CYCLES(u32 curpc);

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#ifndef BRANCH_RECOMPILE

REC_SYS(BEQ);
REC_SYS(BEQL);
REC_SYS(BNE);
REC_SYS(BNEL);
REC_SYS(BLTZ);
REC_SYS(BGTZ);
REC_SYS(BLEZ);
REC_SYS(BGEZ);
REC_SYS(BGTZL);
REC_SYS(BLTZL);
REC_SYS(BLTZAL);
REC_SYS(BLTZALL);
REC_SYS(BLEZL);
REC_SYS(BGEZL);
REC_SYS(BGEZAL);
REC_SYS(BGEZALL);

#else


////////////////////////////////////////////////////
void recBEQ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

//	COUNT_CYCLES(pc+4);

	if ( _Rs_ == _Rt_ )
	{
		recompileNextInstruction(1);
		COUNT_CYCLES(pc);
		SetBranchImm( branchTo );
	}
	else
	{
		//	SetFPUstate();
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		j32Ptr[ 0 ] = JNE32( 0 );

		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		j32Ptr[ 1 ] = JNE32( 0 );
		
		recompileNextInstruction(1);
		COUNT_CYCLES(pc);
		SetBranchImm(branchTo);

		x86SetJ32( j32Ptr[ 0 ] ); 
		x86SetJ32( j32Ptr[ 1 ] );

		// recopy the next inst
		pc -= 4;
		recompileNextInstruction(1);
		COUNT_CYCLES(pc);
		SetBranchImm(pc);
	}
}

////////////////////////////////////////////////////
void recBNE( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	if ( _Rs_ == _Rt_ )
	{
		return;
	}

	//COUNT_CYCLES(pc+4);

	//	SetFPUstate();
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j32Ptr[ 1 ] = JE32( 0 );

	x86SetJ32( j32Ptr[ 0 ] ); 

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
	//j32Ptr[ 2 ] = JMP32( 0 );

	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
	//x86SetJ32( j32Ptr[ 2 ] );
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
void recBLTZAL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)BLTZAL );
	branch = 2;    
}

////////////////////////////////////////////////////
void recBGEZAL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)BGEZAL );
	branch = 2; 
}

////////////////////////////////////////////////////
void recBLEZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JL32( 0 );
	j32Ptr[ 1 ] = JG32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JNZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGTZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JG32( 0 );
	j32Ptr[ 1 ] = JL32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JGE32( 0 );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);

	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGEZ( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JL32( 0 );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);

	SetBranchImm(pc);
}

/*********************************************************
* Register branch logic  Likely                          *
* Format:  OP rs, offset                                 *
*********************************************************/

////////////////////////////////////////////////////
void recBLEZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4); // off by 1 cycle

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JL32( 0 );
	j32Ptr[ 1 ] = JG32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JNZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBGTZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4); // off by 1 cycle

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JG32( 0 );
	j32Ptr[ 1 ] = JL32( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0 );
	j32Ptr[ 2 ] = JZ32( 0 );

	x86SetJ32( j32Ptr[ 0 ] );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 1 ] );
	x86SetJ32( j32Ptr[ 2 ] );
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	//COUNT_CYCLES(pc+4); // off by 1 cycle

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JGE32( 0 );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBLTZALL( void ) 
{
	SysPrintf("BLTZALL\n");
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)BLTZALL );
	branch = 2; 
}

////////////////////////////////////////////////////
void recBGEZALL( void ) 
{
	SysPrintf("BGEZALL\n");
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)BGEZALL );
	branch = 2; 
}

////////////////////////////////////////////////////
void recBEQL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 1 ] = JNE32( 0 );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

////////////////////////////////////////////////////
void recBNEL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;

	

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 0 ] = JNE32( 0 );

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	CMP32RtoR( ECX, EDX );
	j32Ptr[ 1 ] = JNE32( 0 );
	COUNT_CYCLES(pc);
	SetBranchImm(pc+4);

	x86SetJ32( j32Ptr[ 0 ] ); 
	x86SetJ32( j32Ptr[ 1 ] );

	// recopy the next inst
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
}

////////////////////////////////////////////////////
void recBGEZL( void ) 
{
	u32 branchTo = ((s32)_Imm_ * 4) + pc;


	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0 );
	j32Ptr[ 0 ] = JL32( 0 );

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);

	x86SetJ32( j32Ptr[ 0 ] );
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
}

#endif
