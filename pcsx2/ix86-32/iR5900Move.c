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
#include <assert.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"


#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
#ifndef MOVE_RECOMPILE

REC_FUNC(LUI);
REC_FUNC(MFLO);
REC_FUNC(MFHI);
REC_FUNC(MTLO);
REC_FUNC(MTHI);
REC_FUNC(MOVZ);
REC_FUNC(MOVN);

#else

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/

////////////////////////////////////////////////////
void recLUI( void )
{
	if(!_Rt_) return;
      if ( _Imm_ < 0 )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], (u32)_Imm_ << 16 );	//U
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0xffffffff );	//V
	   }
	   else
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], (u32)_Imm_ << 16 );	//U
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );			//V
	   }
 
}

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/

////////////////////////////////////////////////////
void recMFHI( void ) 
{

   if ( ! _Rd_ )
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.HI.UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recMFLO( void ) 
{

   if ( ! _Rd_ ) 
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.LO.UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	   SetMMXstate();
 
}

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/

////////////////////////////////////////////////////
void recMTHI( void ) 
{

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.HI.UD[ 0 ], MM0 );
	   SetMMXstate();
 
}

////////////////////////////////////////////////////
void recMTLO( void )
{

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.LO.UD[ 0 ], MM0 );
	   SetMMXstate();
 
}


/*********************************************************
* Conditional Move                                       *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

////////////////////////////////////////////////////
void recMOVZ( void )
{
   if ( ! _Rd_ ) 
   {
      return;
   }
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   j8Ptr[ 0 ] = JNZ8( 0 );

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

	   x86SetJ8( j8Ptr[ 0 ] ); 
 
}

////////////////////////////////////////////////////
void recMOVN( void ) 
{
   if ( ! _Rd_ ) 
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   j8Ptr[ 0 ] = JZ8( 0 );

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], ECX );

	   x86SetJ8( j8Ptr[ 0 ] );

}

#endif
