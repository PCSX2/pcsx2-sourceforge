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


#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iCP0.h"

/*********************************************************
*   COP0 opcodes                                         *
*                                                        *
*********************************************************/

#ifndef CP0_RECOMPILE

REC_SYS(MFC0);
REC_SYS(MTC0);
REC_SYS(BC0F);
REC_SYS(BC0T);
REC_SYS(BC0FL);
REC_SYS(BC0TL);
REC_SYS(TLBR);
REC_SYS(TLBWI);
REC_SYS(TLBWR);
REC_SYS(TLBP);
REC_SYS(ERET);
REC_SYS(DI);
REC_SYS(EI);

#else

////////////////////////////////////////////////////
REC_SYS(MTC0);
////////////////////////////////////////////////////
REC_SYS(BC0F);
////////////////////////////////////////////////////
REC_SYS(BC0T);
////////////////////////////////////////////////////
REC_SYS(BC0FL);
////////////////////////////////////////////////////
REC_SYS(BC0TL);
////////////////////////////////////////////////////
REC_SYS(TLBR);
////////////////////////////////////////////////////
REC_SYS(TLBWI);
////////////////////////////////////////////////////
REC_SYS(TLBWR);
////////////////////////////////////////////////////
REC_SYS(TLBP);
////////////////////////////////////////////////////
REC_SYS(ERET);
////////////////////////////////////////////////////
REC_SYS(DI);
////////////////////////////////////////////////////
REC_SYS(EI);

////////////////////////////////////////////////////
void recMFC0( void )
{
      if ( ! _Rt_ ) return;


	   MOV32MtoR( EAX, (u32)&cpuRegs.CP0.r[ _Rd_ ] );
	   CDQ( );

	   MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
   
}

/*
void rec(MTC0) {
}

void rec(COP0) {
}

void rec(BC0F) {
}

void rec(BC0T) {
}

void rec(BC0FL) {
}

void rec(BC0TL) {
}

void rec(TLBR) {
}

void rec(TLBWI) {
}

void rec(TLBWR) {
}

void rec(TLBP) {
}

void rec(ERET) {
}
*/
/*
////////////////////////////////////////////////////
void recDI( void ) 
{
      MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status);
      AND32ItoR(EAX, ~0x10000);
	  PUSH32R(EAX);
	  CALLFunc((u32)WriteCP0Status);
	  ADD32ItoR(ESP, 4);
   
}

////////////////////////////////////////////////////
void recEI( void ) 
{
      MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status);
      OR32ItoR(EAX, 0x10000);
	  PUSH32R(EAX);
	  CALLFunc((u32)WriteCP0Status);
	  ADD32ItoR(ESP, 4);

}
*/

#endif
