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

extern void COUNT_CYCLES(u32 curpc);

#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
#ifndef JUMP_RECOMPILE

REC_SYS(J);
REC_SYS(JAL);
REC_SYS(JR);
REC_SYS(JALR);

#else

////////////////////////////////////////////////////
void recJ( void ) 
{	
	// SET_FPUSTATE;
	u32 newpc = (_Target_ << 2) + ( pc & 0xf0000000 );
	COUNT_CYCLES(pc+4);
	recompileNextInstruction(1);
	SetBranchImm(newpc);
}

////////////////////////////////////////////////////
void recJAL( void ) 
{
	u32 newpc = (_Target_ << 2) + ( pc & 0xf0000000 );
	MOV32ItoM( (int)&cpuRegs.GPR.r[31].UL[ 0 ], pc + 4 );
	MOV32ItoM( (int)&cpuRegs.GPR.r[31].UL[ 1 ], 0 );

	COUNT_CYCLES(pc+4);

	recompileNextInstruction(1);
	SetBranchImm(newpc);
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/

////////////////////////////////////////////////////
void recJR( void ) 
{
	COUNT_CYCLES(pc+4);
	SetBranchReg( _Rs_);
}

////////////////////////////////////////////////////
void recJALR( void ) 
{
	MOV32MtoR( ESI, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	//MOV32RtoM( (int)&target, EAX);

	if ( _Rd_ ) 
	{

		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], pc + 4 );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
	}

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	MOV32RtoM( (u32)&cpuRegs.pc, ESI);

	SetBranchReg(0xffffffff);
}

#endif
