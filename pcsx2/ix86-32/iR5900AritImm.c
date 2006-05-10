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
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
#ifndef ARITHMETICIMM_RECOMPILE

REC_FUNC(ADDI);
REC_FUNC(ADDIU);
REC_FUNC(DADDI);
REC_FUNC(DADDIU);
REC_FUNC(ANDI);
REC_FUNC(ORI);
REC_FUNC(XORI);

REC_FUNC(SLTI);
REC_FUNC(SLTIU);

#else

////////////////////////////////////////////////////
void recADDI( void ) 
{
   if ( ! _Rt_ )
   {
      return;
   }

      MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }

	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
  
}

////////////////////////////////////////////////////
void recADDIU( void ) 
{
	recADDI( );
}

////////////////////////////////////////////////////
void recDADDI( void ) 
{
#ifdef __x86_64_
   if ( ! _Rt_ )
   {
      return;
   }

	MOV64MtoR( RAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	   {
		   ADD64ItoR( EAX, _Imm_ );
	   }
	MOV64RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], RAX );
#else
   if ( ! _Rt_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
		   if ( _Imm_ < 0 )
         {
			   ADC32ItoR( EDX, 0xffffffff );
         }
		   else
         {
			   ADC32ItoR( EDX, 0 );
         }
	   }
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
#endif  
}

////////////////////////////////////////////////////
void recDADDIU( void ) 
{
	recDADDI( );
}

////////////////////////////////////////////////////
void recSLTIU( void )
{
	if ( ! _Rt_ )
		return;

	MOV32ItoR(EAX, 1);
	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JB8( 0 );
	j8Ptr[2] = JA8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	MOV32ItoR(EAX, 0);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recSLTI( void )
{
	if ( ! _Rt_ )
		return;

	// test silent hill if modding
	MOV32ItoR(EAX, 1);
	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JL8( 0 );
	j8Ptr[2] = JG8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	MOV32ItoR(EAX, 0);
	
	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recANDI( void )
{
	if ( ! _Rt_ )
   {
      return;
   }
	   if ( _ImmU_ != 0 )
	   {
		   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		   AND32ItoR( EAX, _ImmU_ );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	   }
	   else
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
	   }
   
}

////////////////////////////////////////////////////
void recORI( void )
{
   if ( ! _Rt_ )
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	   if ( _ImmU_ != 0 )
	   {
		   MOV32ItoM( (int)&_imm, _ImmU_ );
		   MOVQMtoR( MM1, (int)&_imm );
		   PORRtoR( MM0, MM1 );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], MM0 );
	   SetMMXstate();
}

////////////////////////////////////////////////////
void recXORI( void )
{
   if ( ! _Rt_ )
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	   if ( _ImmU_ != 0 )
	   {
		   MOV32ItoM( (int)&_imm, _ImmU_ );
		   MOVQMtoR( MM1, (int)&_imm );
		   PXORRtoR( MM0, MM1 );
	   }
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rt_ ], MM0 );
	   SetMMXstate();

}

#endif
