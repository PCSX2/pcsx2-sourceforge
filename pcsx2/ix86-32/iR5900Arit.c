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
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

#ifndef ARITHMETIC_RECOMPILE

REC_FUNC(ADD);
REC_FUNC(ADDU);
REC_FUNC(DADD);
REC_FUNC(DADDU);
REC_FUNC(SUB);
REC_FUNC(SUBU);
REC_FUNC(DSUB);
REC_FUNC(DSUBU);
REC_FUNC(AND);
REC_FUNC(OR);
REC_FUNC(XOR);
REC_FUNC(NOR);
REC_FUNC(SLT);
REC_FUNC(SLTU);

#else

////////////////////////////////////////////////////
void recADD( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

}

////////////////////////////////////////////////////
void recADDU( void ) 
{
	recADD( );
}

////////////////////////////////////////////////////
void recDADD( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   ADC32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );  
}

////////////////////////////////////////////////////
void recDADDU( void )
{
	recDADD( );
}

////////////////////////////////////////////////////
void recSUB( void ) 
{
       if ( ! _Rd_ ) return;
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSUBU( void ) 
{
	recSUB( );
}

////////////////////////////////////////////////////
void recDSUB( void ) 
{
      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   SBB32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

}

////////////////////////////////////////////////////
void recDSUBU( void ) 
{
	recDSUB( );
}

////////////////////////////////////////////////////
void recAND( void )
{
   if ( ! _Rd_ )
   {
      return;
   }
	   if ( ( _Rt_ == 0 ) || ( _Rs_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0 );	
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );	
	   }
      else
      {
	      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	      MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	      PANDRtoR( MM0, MM1);
	      MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	      SetMMXstate();
      }

}

////////////////////////////////////////////////////
void recOR( void )
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0  ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0x0 );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0x0 );
	   }
      else if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
	   } 
      else if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
	   }
      else
      {
	      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	      MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	      PORRtoR( MM0, MM1 );
	      MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	      SetMMXstate();
      }
}

////////////////////////////////////////////////////
void recXOR( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ],0x0);
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ],0x0);
		   return;
	   }

	   if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
		   return;
	   }

	   if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	   MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PXORRtoR( MM0, MM1);
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
}

////////////////////////////////////////////////////
void recNOR( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ],0xffffffff);
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ],0xffffffff);
		   return;
	   }

	   if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   PCMPEQDRtoR( MM1,MM1);
		   PXORRtoR( MM0,MM1);
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
		   SetMMXstate();
		   return;
	   }

	   if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   PCMPEQDRtoR( MM1,MM1);
		   PXORRtoR( MM0,MM1);
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
		   SetMMXstate();
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	   PCMPEQDRtoR( MM1,MM1);
	   PORMtoR( MM0,(int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PXORRtoR( MM0,MM1);
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
	   SetMMXstate();
}

////////////////////////////////////////////////////
// test with silent hill, lemans
void recSLT( void )
{
	if ( ! _Rd_ )
		return;
	
	MOV32ItoR(EAX, 1);

	if( _Rs_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0);
		j8Ptr[0] = JG8( 0 );
		j8Ptr[2] = JL8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
		j8Ptr[1] = JA8(0);
		
		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}
	else if( _Rt_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0);
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0);
		j8Ptr[1] = JB8(0);

		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}
	else {
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );

		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recSLTU( void )
{
   MOV32ItoR(EAX, 1);

	if( _Rs_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0);
		j8Ptr[0] = JA8( 0 );
		j8Ptr[2] = JB8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
		j8Ptr[1] = JA8(0);
		
		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}
	else if( _Rt_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0);
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}
	else {
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );

		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		MOV32ItoR(EAX, 0);
	}

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
}

#endif
