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
#include "VU0.h"

#ifdef __WIN32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
#ifndef LOADSTORE_RECOMPILE

REC_FUNC(LB);
REC_FUNC(LBU);
REC_FUNC(LH);
REC_FUNC(LHU);
REC_FUNC(LW);
REC_FUNC(LWU);
REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(LD);
REC_FUNC(LDR);
REC_FUNC(LDL);
REC_FUNC(LQ);
REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);
REC_FUNC(SWL);
REC_FUNC(SWR);
REC_FUNC(SD);
REC_FUNC(SDL);
REC_FUNC(SDR);
REC_FUNC(SQ);
REC_FUNC(LWC1);
REC_FUNC(SWC1);
REC_FUNC(LQC2);
REC_FUNC(SQC2);

#else

u64 retValue;
u64 dummyValue[ 4 ];

#ifdef WIN32_VIRTUAL_MEM

extern u32 recMemRead8();
extern u32 recMemRead16();
extern u32 recMemRead32();
extern u32 recMemRead64(u64 *out);
extern u32 recMemRead128(u64 *out);

extern void recMemWrite8(u8  value);
extern void recMemWrite16(u16  value);
extern void recMemWrite32(u32  value);
extern void recMemWrite64(u64  value);
extern void recMemWrite128(u64*  value);

extern void iMemRead32Check();

#define TRANSFORM_ADDR_REC() { \
	CMP32ItoR(ECX, 0x40000000); \
	j8Ptr[0] = JB8(0); \
	AND32ItoR(ECX, ~0xa0000000); \
	x86SetJ8( j8Ptr[0] ); \
} \

////////////////////////////////////////////////////
void recLB( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOVSX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead8 );
	MOVSX32R8toR(EAX, EAX);
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

////////////////////////////////////////////////////
void recLBU( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOVZX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead8 );
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
	}
}

////////////////////////////////////////////////////
void recLH( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOVSX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead16 );
	MOVSX32R16toR(EAX, EAX);

	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

////////////////////////////////////////////////////
void recLHU( void )
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}
	
	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOVZX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead16 );
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
	}
}

////////////////////////////////////////////////////
void recLW( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	iMemRead32Check();

	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead32 );
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

////////////////////////////////////////////////////
void recLWU( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}
	
	TRANSFORM_ADDR_REC();
	iMemRead32Check();
	
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead32 );
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
	}
}

////////////////////////////////////////////////////
void recLWL( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32ItoR(EDX, 0x3);
	AND32RtoR(EDX, ECX);
	TRANSFORM_ADDR_REC();
	AND32ItoR(ECX, ~3);
	SHL32ItoR(EDX, 3); // *8

	iMemRead32Check();

	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	PUSH32R(EDX);
	CALLFunc( (int)recMemRead32 );
	POP32R(EDX);
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {

		// mem << LWL_SHIFT[shift]
		MOV32ItoR(ECX, 24);
		SUB32RtoR(ECX, EDX);
		SHL32CLtoR(EAX);

		// mov temp and compute _rt_ & LWL_MASK[shift]
		MOV32RtoR(ECX, EDX);
		MOV32ItoR(EDX, 0xffffff);
		SAR32CLtoR(EDX);
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		AND32RtoR(EDX, ECX);

		// combine
		OR32RtoR(EAX, EDX);
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

////////////////////////////////////////////////////
void recLWR( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();

	iMemRead32Check();

	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RtoR(EDX, ECX);
	AND32ItoR(ECX, ~3);
	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	PUSH32R(ECX);
	AND32ItoR(ECX, ~3);
	CALLFunc( (int)recMemRead32 );
	POP32R(EDX);
	
	x86SetJ8(j8Ptr[1]);

	if ( _Rt_ ) {

		// mem << LWL_SHIFT[shift]
		MOV32RtoR(ECX, EDX);
		AND32ItoR(ECX, 3);
		SHL32ItoR(ECX, 3); // *8
		SHR32CLtoR(EAX);

		// mov temp and compute _rt_ & LWL_MASK[shift]
		MOV32RtoR(EDX, ECX);
		MOV32ItoR(ECX, 24);
		SUB32RtoR(ECX, EDX);
		MOV32ItoR(EDX, 0xffffff00);
		SHL32CLtoR(EDX);
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		AND32RtoR(ECX, EDX);

		// combine
		OR32RtoR(EAX, ECX);
		
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );

		// check if upper byte changed
		CMP32ItoR(EDX, 0);
		j8Ptr[0] = JNE8(0);

		// changed
		CDQ();
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );

		x86SetJ8( j8Ptr[0] );
	}
}

////////////////////////////////////////////////////
void recLD( void )
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	SSE_MOVLPSRmtoROffset(XMM0, ECX, PS2MEM_BASE_);

	if( _Rt_ ) {
		SSE_MOVLPS_XMM_to_M64((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], XMM0);
	}

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	if ( _Rt_ )
		PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	else
		PUSH32I( (int)&retValue );

	iFlushCall();
	CALLFunc( (int)recMemRead64 );
	ADD32ItoR(ESP, 4);
	
	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recLDL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LDL );   
}

////////////////////////////////////////////////////
void recLDR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LDR );   
}

////////////////////////////////////////////////////
void recLQ( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_);
	}

	TRANSFORM_ADDR_REC();
	AND32ItoR(ECX, ~0xf);
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	SSE_MOVAPSRmtoROffset(XMM0, ECX, PS2MEM_BASE_);

	if( _Rt_ ) {
		SSE_MOVAPS_XMM_to_M128((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], XMM0);
	}

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	if ( _Rt_ )
		PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	else
		PUSH32I( (int)&dummyValue[0] );

	iFlushCall();
	CALLFunc( (int)recMemRead128 );
	ADD32ItoR(ESP, 4);
	
	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recSB( void )
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_);
	}

	MOV8MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);

	TRANSFORM_ADDR_REC();

	PUSH32R(EAX);
	iFlushCall();
	CALLFunc( (int)recMemWrite8 );
	ADD32ItoR(ESP, 4);
}

////////////////////////////////////////////////////
void recSH( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_);
	}

	MOV16MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);

	TRANSFORM_ADDR_REC();

	PUSH32R(EAX);
	iFlushCall();
	CALLFunc( (int)recMemWrite16 );
	ADD32ItoR(ESP, 4);
}

////////////////////////////////////////////////////
void recSW( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
	TRANSFORM_ADDR_REC();

	iFlushCall();
	CALLFunc( (int)recMemWrite32 );
}

////////////////////////////////////////////////////
void recSWL( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32ItoR(EDX, 0x3);
	AND32RtoR(EDX, ECX);
	TRANSFORM_ADDR_REC();
	AND32ItoR(ECX, ~3);
	SHL32ItoR(EDX, 3); // *8

	PUSH32R(ECX);

	// read the old mem
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	PUSH32R(EDX);
	CALLFunc( (int)recMemRead32 );
	POP32R(EDX);

	x86SetJ8(j8Ptr[1]);

	//iMemRead32Check();
	MOV32MtoR(EBX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);

	// oldmem is in EAX
	// mem >> SWL_SHIFT[shift]
	MOV32ItoR(ECX, 24);
	SUB32RtoR(ECX, EDX);
	SHR32CLtoR(EBX);

	// mov temp and compute _rt_ & SWL_MASK[shift]
	MOV32RtoR(ECX, EDX);
	MOV32ItoR(EDX, 0xffffff00);
	SHL32CLtoR(EDX);
	AND32RtoR(EAX, EDX);

	// combine
	OR32RtoR(EAX, EBX);

	POP32R(ECX);

	iFlushCall();
	CALLFunc( (int)recMemWrite32 );
}

////////////////////////////////////////////////////
void recSWR( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32ItoR(EDX, 0x3);
	AND32RtoR(EDX, ECX);
	TRANSFORM_ADDR_REC();
	AND32ItoR(ECX, ~3);
	SHL32ItoR(EDX, 3); // *8

	PUSH32R(ECX);

	// read the old mem
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	PUSH32R(EDX);
	CALLFunc( (int)recMemRead32 );
	POP32R(EDX);

	x86SetJ8(j8Ptr[1]);

	//iMemRead32Check();
	MOV32MtoR(EBX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);

	// oldmem is in EAX
	// mem << SWR_SHIFT[shift]
	MOV32RtoR(ECX, EDX);
	SHL32CLtoR(EBX);

	// mov temp and compute _rt_ & SWR_MASK[shift]
	MOV32ItoR(ECX, 24);
	SUB32RtoR(ECX, EDX);
	MOV32ItoR(EDX, 0x00ffffff);
	SHR32CLtoR(EDX);
	AND32RtoR(EAX, EDX);

	// combine
	OR32RtoR(EAX, EBX);

	POP32R(ECX);

	iFlushCall();
	CALLFunc( (int)recMemWrite32 );

//	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
//	MOV32ItoM( (int)&cpuRegs.pc, pc );
//	iFlushCall();
//	CALLFunc( (int)SWR );
}

////////////////////////////////////////////////////
int tempaddr;
void recSD( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32RtoM((int)&tempaddr, ECX);
	TRANSFORM_ADDR_REC();

	MOV32ItoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	iFlushCall();
	CALLFunc( (int)recMemWrite64);
}

////////////////////////////////////////////////////
void recSDL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SDL );   
}

////////////////////////////////////////////////////
void recSDR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SDR );
}

////////////////////////////////////////////////////
void recSQ( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	AND32ItoR(ECX, ~0xf);

	MOV32ItoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	iFlushCall();
	CALLFunc( (int)recMemWrite128 );
}

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////
void recLWC1( void )
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();

	iMemRead32Check();

	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemRead32 );
	
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
}

////////////////////////////////////////////////////
void recSWC1( void )
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);

	TRANSFORM_ADDR_REC();
//	TEST32ItoR(ECX, 0x10000000);
//	j8Ptr[0] = JNZ8(0);
//
//	MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_);
//
//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);

	iFlushCall();
	CALLFunc( (int)recMemWrite32 );
	
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

void recLQC2( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_);
	}

	TRANSFORM_ADDR_REC();
	TEST32ItoR(ECX, 0x10000000);
	j8Ptr[0] = JNZ8(0);

	SSE_MOVAPSRmtoROffset(XMM0, ECX, PS2MEM_BASE_);

	if( _Rt_ ) {
		SSE_MOVAPS_XMM_to_M128((int)&VU0.VF[_Ft_].UD[0], XMM0);
	}

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	if ( _Rt_ )
		PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
    else
        PUSH32I( (int)&dummyValue[0] );

	iFlushCall();
	CALLFunc( (int)recMemRead128 );
	ADD32ItoR( ESP, 4 );
	
	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recSQC2( void ) 
{
	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( ECX, _Imm_ );
	}

	TRANSFORM_ADDR_REC();
	iFlushCall();
	MOV32ItoR(EAX, (int)&VU0.VF[_Ft_].UD[0] );
	CALLFunc( (int)recMemWrite128 );
}

#else

////////////////////////////////////////////////////
void recLB( void ) 
{


	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
      PUSH32I( (int)&retValue );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead8 );
      ADD32ItoR( ESP, 8 );
	   if ( _Rt_ ) 
      {
         u8* linkEnd;
         TEST32RtoR( EAX, EAX );
         linkEnd = JNZ8( 0 );
         MOV32MtoR( EAX, (int)&retValue );
		   MOVSX32R8toR( EAX, EAX );
		   CDQ( );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
         SETLINK8( linkEnd );
	   }
   
}

////////////////////////////////////////////////////
void recLBU( void ) 
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
      PUSH32I( (int)&retValue );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead8 );
      ADD32ItoR( ESP, 8 );
	   if ( _Rt_ ) 
      {
         u8* linkEnd;
         TEST32RtoR( EAX, EAX );
         linkEnd = JNZ8( 0 );
         MOV32MtoR( EAX, (int)&retValue );
		   MOVZX32R8toR( EAX, EAX );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
         SETLINK8( linkEnd );
	   }
   
}

////////////////////////////////////////////////////
void recLH( void ) 
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
      PUSH32I( (int)&retValue );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead16 );
      ADD32ItoR( ESP, 8 );
	   if ( _Rt_ )
      {
         u8* linkEnd;
         TEST32RtoR( EAX, EAX );
         linkEnd = JNZ8( 0 );
         MOV32MtoR( EAX, (int)&retValue );
		   MOVSX32R16toR( EAX, EAX );
		   CDQ( );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
         SETLINK8( linkEnd );
	   }
  
}

////////////////////////////////////////////////////
void recLHU( void )
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
      PUSH32I( (int)&retValue );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead16 );
      ADD32ItoR( ESP, 8 );
	   if ( _Rt_ ) 
      {
         u8* linkEnd;
         TEST32RtoR( EAX, EAX );
         linkEnd = JNZ8( 0 );
         MOV32MtoR( EAX, (int)&retValue );
		   MOVZX32R16toR( EAX, EAX );
		   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
         SETLINK8( linkEnd );
	   }
   
}

////////////////////////////////////////////////////
void recLW( void ) 
{

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32I( (int)&retValue );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );

	if ( _Rt_ ) 
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&retValue );
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
		SETLINK8( linkEnd );
	}

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	if ( _Rt_ ) 
//	{
//		MOV32RmtoR( EAX, EAX );
//		CDQ( );
//		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
//		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
//	}
//
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recLWU( void ) 
{

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32I( (int)&retValue );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );
	if ( _Rt_ )
	{
		u8* linkEnd;
		TEST32RtoR( EAX, EAX );
		linkEnd = JNZ8( 0 );
		MOV32MtoR( EAX, (int)&retValue );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		SETLINK8( linkEnd );
	}

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	if ( _Rt_ ) 
//	{
//		MOV32RmtoR( EAX, EAX );
//		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
//		MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
//	}
//
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recLWL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LWL );
}

////////////////////////////////////////////////////
void recLWR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LWR );   
}

////////////////////////////////////////////////////
extern void MOV64RmtoR( x86IntRegType to, x86IntRegType from );

void recLD( void )
{
	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	if ( _Rt_ )
	{
		PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	}
	else
	{
		PUSH32I( (int)&retValue );
	}
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memRead64 );
	ADD32ItoR( ESP, 8 );
   
//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	MOV32RmtoR( ECX, EAX );
//	ADD32ItoR( EAX, 4 );
//	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], ECX);
//	MOV32RmtoR( ECX, EAX );
//	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], ECX);
//
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recLDL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LDL );
}

////////////////////////////////////////////////////
void recLDR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)LDR );
}

////////////////////////////////////////////////////
void recLQ( void ) 
{


	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_);
	   }
	   AND32ItoR( EAX, ~0xf );

      if ( _Rt_ )
      {
         PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
      }
      else
      {
         PUSH32I( (int)&dummyValue );
      }
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead128 );
	   ADD32ItoR( ESP, 8 );
   
}

////////////////////////////////////////////////////
void recSB( void )
{
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_);
	   }
	   PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memWrite8 );
	   ADD32ItoR( ESP, 8 );
   
}

////////////////////////////////////////////////////
void recSH( void ) 
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
	   PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memWrite16 );
	   ADD32ItoR( ESP, 8 );
   
}

////////////////////////////////////////////////////
void recSW( void ) 
{
	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memWrite32 );
	ADD32ItoR( ESP, 8 );

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
//	PUSH32R(EAX);
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	MOV32RtoRm( EAX, ECX );
//
//	iFlushCall();
//	CALLFunc( (int)recClear32 );
//	ADD32ItoR(ESP, 4);
//
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recSWL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SWL );   
}

////////////////////////////////////////////////////
void recSWR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SWR );
}

////////////////////////////////////////////////////
void recSD( void ) 
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }
	   PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   PUSH32M( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memWrite64 );
	   ADD32ItoR( ESP, 12 );
   
}

////////////////////////////////////////////////////
void recSDL( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SDL );
}

////////////////////////////////////////////////////
void recSDR( void ) 
{
	COUNT_CYCLES(pc);
	MOV32ItoM( (int)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (int)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (int)SDR );
}

////////////////////////////////////////////////////
void recSQ( void ) 
{
//	int t0reg = _allocTempXMMreg(-1);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
	{
		ADD32ItoR( EAX, _Imm_ );
	}
	AND32ItoR( EAX, ~0xf );

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memWrite128 );
	ADD32ItoR( ESP, 8 );

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	SSE_MOVAPS_M128_to_XMM(t0reg, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
//	PUSH32R(EAX);
//
//	// read from psM directly
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	SSE_MOVAPSRtoRm(EAX, t0reg);
//
//	iFlushCall();
//	CALLFunc( (int)recClear128 );
//	ADD32ItoR(ESP, 4);
//
//	x86SetJ8(j8Ptr[1]);
//
//	_freeXMMreg(t0reg);
}

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////
void recLWC1( void )
{
	_freeXMMregs();

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	TEST32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32I( (int)&fpuRegs.fpr[ _Rt_ ].UL );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memRead32 );
	ADD32ItoR( ESP, 8 );

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	AND32ItoR(EAX, 0x0fffffff);
//	ADD32ItoR(EAX, (u32)psM);
//
//	MOV32RmtoR( EAX, EAX );
//	MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX);
//
//	x86SetJ8(j8Ptr[1]);
}

////////////////////////////////////////////////////
void recSWC1( void )
{
	_freeXMMregs();

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )	
	{
		ADD32ItoR( EAX, _Imm_ );
	}

//	CMP32ItoR(EAX, 0x10000000);
//	j8Ptr[0] = JZ8(0);

	PUSH32M( (int)&fpuRegs.fpr[ _Rt_ ].UL );
	PUSH32R( EAX );
	iFlushCall();
	CALLFunc( (int)memWrite32 );
	ADD32ItoR( ESP, 8 );

//	j8Ptr[1] = JMP8(0);
//	x86SetJ8(j8Ptr[0]);
//
//	// read from psM directly
//	MOV32MtoR(ECX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
//	ADD32ItoR(EAX, (u32)psM);
//
//	MOV32RtoRm( EAX, ECX );
//
//	x86SetJ8(j8Ptr[1]);
   
}

////////////////////////////////////////////////////

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

void recLQC2( void ) 
{

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_);
	   }

      if ( _Rt_ )
      {
         PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
      }
      else
      {
         PUSH32I( (int)&dummyValue );
      }
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memRead128 );
	   ADD32ItoR( ESP, 8 );
   
}

////////////////////////////////////////////////////
void recSQC2( void ) 
{


	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   if ( _Imm_ != 0 )
	   {
		   ADD32ItoR( EAX, _Imm_ );
	   }

	   PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
	   PUSH32R( EAX );
	   iFlushCall();
	   CALLFunc( (int)memWrite128 );
	   ADD32ItoR( ESP, 8 );
   
}

#endif

#endif
