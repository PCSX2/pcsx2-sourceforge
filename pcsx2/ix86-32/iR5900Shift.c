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
#ifndef SHIFT_RECOMPILE

REC_FUNC(SLL);
REC_FUNC(SRL);
REC_FUNC(SRA);
REC_FUNC(DSLL);
REC_FUNC(DSRL);
REC_FUNC(DSRA);
REC_FUNC(DSLL32);
REC_FUNC(DSRL32);
REC_FUNC(DSRA32);

REC_FUNC(SLLV);
REC_FUNC(SRLV);
REC_FUNC(SRAV);
REC_FUNC(DSLLV);
REC_FUNC(DSRLV);
REC_FUNC(DSRAV);

#else

////////////////////////////////////////////////////
void recDSRA( void )
{
    MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	if ( _Sa_ != 0 )
	{
	  MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	  MOV32RtoR( EDX, EAX );
	  SHR32ItoR( ECX, _Sa_ );
	  SHL32ItoR( EAX, 32 - _Sa_);
	  SAR32ItoR( EDX, _Sa_ );
	  AND32ItoR( EAX, 0xffffffff & ~( ( 1 << ( 32 - _Sa_ ) ) - 1 ) );
	  OR32RtoR( EAX, ECX );
	 }
	  MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	  MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );  
}

////////////////////////////////////////////////////
void recDSRA32(void)
{
if(!_Rd_) return;
   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
   CDQ( );
   if ( _Sa_ != 0 )
   {
	 SAR32ItoR( EAX, _Sa_ );
   }
   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

  
}

////////////////////////////////////////////////////
void recSLL( void )
{
	if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Sa_ != 0 )
	   {
		   SHL32ItoR( EAX, _Sa_ );
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSRL( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Sa_ != 0 )
	   {
		   SHR32ItoR( EAX, _Sa_);
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
  
}

////////////////////////////////////////////////////
void recSRA( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Sa_ != 0 )
	   {
		   SAR32ItoR( EAX, _Sa_);
	   }
	   CDQ();
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recDSLL( void )
{
	if ( ! _Rd_ )
   {
      return;
   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Sa_ != 0 )
	   {
		   PSLLQItoR( MM0, _Sa_ );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recDSRL( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Sa_ != 0 )
	   {
		   PSRLQItoR( MM0, _Sa_ );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
}

////////////////////////////////////////////////////
void recDSLL32( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( _Sa_ == 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
		   return;
	   }
	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PSLLQItoR( MM0, _Sa_ + 32 );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recDSRL32( void ) 
{
	if ( ! _Rd_ )
   {
      return;
   }


	   if ( _Sa_ == 0 )
	   {
		   MOV32MtoR( EAX,(int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PSRLQItoR( MM0, _Sa_ + 32 );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
 
}


/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/

////////////////////////////////////////////////////


////////////////////////////////////////////////////
void recSLLV( void ) 
{

      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )	
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SHL32CLtoR( EAX );
	   }
	   CDQ();
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSRLV( void ) 
{
 
      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )	
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SHR32CLtoR( EAX );
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSRAV( void ) 
{
      if ( ! _Rd_ ) return;
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( ECX, 0x1f );
		   SAR32CLtoR( EAX );
	   }
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recDSLLV( void ) 
{
      if ( ! _Rd_ ) return;

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   AND32ItoR( EAX, 0x3f);
		   MOV32RtoM( (int)&_sa, EAX );
		   PSLLQMtoR( MM0, (int)&_sa );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
  
}

////////////////////////////////////////////////////
void recDSRLV( void ) 
{
      if ( ! _Rd_ ) return;
      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   if ( _Rs_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   AND32ItoR( EAX, 0x3f);
		   MOV32RtoM( (int)&_sa, EAX );
		   PSRLQMtoR( MM0, (int)&_sa );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();

}
////////////////////////////////////////////////////////////////
void recDSRAV( void ) 
{
	COUNT_CYCLES(pc);
	   MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	   MOV32ItoM( (int)&cpuRegs.pc, pc );
	   iFlushCall();
	   CALLFunc( (int)DSRAV );
}
#endif
