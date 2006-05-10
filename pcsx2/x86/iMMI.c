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

/*********************************************************
*   MMI opcodes                                          *
*                                                        *
*********************************************************/
#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"

__declspec(align(16)) static u32 s_UnpackMasks[] = {
	0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff,
	0xffffffff, 0x00000000, 0xffffffff, 0x00000000,
	0x00000000, 0xffffffff, 0x00000000, 0xffffffff };

#define NEW_MMI //this will remove as soon as we figure out that the new code is working without bugz

#ifndef MMI_RECOMPILE

REC_FUNC( MADD );
REC_FUNC( MADDU );
REC_FUNC( MADD1 );
REC_FUNC( MADDU1 );
REC_FUNC( PLZCW );

#ifndef MMI0_RECOMPILE

REC_FUNC( MMI0 );

#endif

#ifndef MMI1_RECOMPILE

REC_FUNC( MMI1 );

#endif

#ifndef MMI2_RECOMPILE

REC_FUNC( MMI2 );

#endif

#ifndef MMI3_RECOMPILE

REC_FUNC( MMI3 );

#endif

REC_FUNC( MFHI1 );
REC_FUNC( MFLO1 );
REC_FUNC( MTHI1 );
REC_FUNC( MTLO1 );
REC_FUNC( MULT1 );
REC_FUNC( MULTU1 );
REC_FUNC( DIV1 );
REC_FUNC( DIVU1 );
REC_FUNC( PMFHL );
REC_FUNC( PMTHL );

REC_FUNC( PSRLW );
REC_FUNC( PSRLH );

REC_FUNC( PSRAH );
REC_FUNC( PSRAW );

REC_FUNC( PSLLH );
REC_FUNC( PSLLW );

#else

REC_FUNC( MADD );

__declspec(align(16)) static u32 s_MaddMask[] = { 0x80000000, 0, 0x80000000, 0 };

void recMADDU()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START

	SSE_MOVSS_M32_to_XMM(XMM0, (u32)&cpuRegs.LO.UL[0]);
	SSE_MOVSS_M32_to_XMM(XMM2, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE2_PUNPCKLDQ_M128_to_XMM(XMM0, (u32)&cpuRegs.HI.UL[0]);

	SSE2_PMULUDQ_M128_to_XMM(XMM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE_ADDPS_XMM_to_XMM(XMM2, XMM0);

	// hi hi lo lo
	SSE2_PUNPCKLDQ_XMM_to_XMM(XMM2, XMM2);

	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&s_MaddMask[0]);
	SSE2_PCMPGTD_XMM_to_XMM(XMM1, XMM2);

	// xmm1 is 1 whenever bits need to be extended
	SSE_MOVAPS_XMM_to_XMM(XMM0, XMM1);
	SSE_ANDNPS_XMM_to_XMM(XMM0, XMM2);
	SSE_ORPS_XMM_to_XMM(XMM0, XMM1);

	SSE_MOVLPS_XMM_to_M64((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
	SSE_MOVLPS_XMM_to_M64((u32)&cpuRegs.LO.UL[0], XMM0);
	SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.HI.UL[0], XMM0);

CPU_SSE2_END

COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)MADDU );
}

REC_FUNC( MADD1 );
REC_FUNC( MADDU1 );

void recPLZCW()
{
	if ( ! _Rd_ ) return;

	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);

	// first word
	TEST32RtoR(EAX, EAX);
	j8Ptr[0] = JNZ8(0);

	// zero, so put 31
	MOV32ItoM((u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 31);
	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	TEST32ItoR(EAX, 0x80000000);
	j8Ptr[0] = JZ8(0);
	NOT32R(EAX);
	x86SetJ8(j8Ptr[0]);

	// not zero
	x86SetJ8(j8Ptr[0]);
	BSRRtoR(EAX, EAX);
	MOV32ItoR(ECX, 30);
	SUB32RtoR(ECX, EAX);
	MOV32RtoM((u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX);

	x86SetJ8(j8Ptr[1]);

	// second word
	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);

	TEST32RtoR(EAX, EAX);
	j8Ptr[0] = JNZ8(0);

	// zero, so put 31
	MOV32ItoM((u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 31);
	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	TEST32ItoR(EAX, 0x80000000);
	j8Ptr[0] = JZ8(0);
	NOT32R(EAX);
	x86SetJ8(j8Ptr[0]);

	// not zero
	x86SetJ8(j8Ptr[0]);
	BSRRtoR(EAX, EAX);
	MOV32ItoR(ECX, 30);
	SUB32RtoR(ECX, EAX);
	MOV32RtoM((u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], ECX);
	x86SetJ8(j8Ptr[1]);
}

__declspec(align(16)) static u32 s_CmpMasks[] = {
	0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff };

void recPMFHL()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START

	switch (_Sa_) {
		case 0x00: // LW

			SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.LO.UL[0] );
			SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.HI.UL[0] );
			SSE_ANDPS_M128_to_XMM(XMM0, (int)&s_UnpackMasks[4]);
			SSE2_PSLLQ_I8_to_XMM(XMM1, 32);
			SSE_ORPS_XMM_to_XMM(XMM0, XMM1);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
			return;

		case 0x01: // UW
			SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.HI.UL[0] );
			SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.LO.UL[0] );
			SSE_ANDPS_M128_to_XMM(XMM0, (int)&s_UnpackMasks[8]);
			SSE2_PSRLQ_I8_to_XMM(XMM1, 32);
			SSE_ORPS_XMM_to_XMM(XMM0, XMM1);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
			return;

		case 0x02: // SLW
			// fall to interp
			COUNT_CYCLES(pc);
			MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
			MOV32ItoM( (u32)&cpuRegs.pc, pc );
			iFlushCall();
			CALLFunc( (u32)PMFHL );
			return;

		case 0x03: // LH
			SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.LO.UL[0] );
			SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.HI.UL[0] );

			// and so it doesn't sign saturate
			SSE_ANDPS_M128_to_XMM(XMM0, (u32)&s_CmpMasks[0]);
			SSE_ANDPS_M128_to_XMM(XMM1, (u32)&s_CmpMasks[0]);

			SSE2_PACKSSDW_XMM_to_XMM(XMM1, XMM0);

			// shuffle so a1a0b1b0->a1b1a0b0
			SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM1, 0xd8);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
			return;

		case 0x04: // SH
			SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.LO.UL[0] );
			SSE2_PACKSSDW_M128_to_XMM(XMM1, (u32)&cpuRegs.HI.UL[0]);
			
			// shuffle so a1a0b1b0->a1b1a0b0
			SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM1, 0xd8);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
			return;
	}

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PMFHL );
}

REC_FUNC( PMTHL );

#ifndef MMI0_RECOMPILE

REC_FUNC( MMI0 );

#endif

#ifndef MMI1_RECOMPILE

REC_FUNC( MMI1 );

#endif

#ifndef MMI2_RECOMPILE

REC_FUNC( MMI2 );

#endif

#ifndef MMI3_RECOMPILE

REC_FUNC( MMI3 );

#endif

////////////////////////////////////////////////////
void recPSRLH( void )
{


	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSRLH\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}

    CPU_SSE2_START //24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSRLW_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
    CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSRLWItoR( MM0, _Sa_ );
	PSRLWItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSRLW( void )
{


	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSRLW\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}

   CPU_SSE2_START //24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSRLD_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
   CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSRLDItoR( MM0, _Sa_ );
	PSRLDItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSRAH( void )
{

	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSRAH\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}

   CPU_SSE2_START//24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSRAW_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
   CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSRAWItoR( MM0, _Sa_ );
	PSRAWItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSRAW( void )
{

	if ( _Rd_ == 0 )return;

	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSRAW\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}

   CPU_SSE2_START//24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSRAD_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
   CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSRADItoR( MM0, _Sa_ );
	PSRADItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSLLH( void )
{

	if ( _Rd_ == 0 )return;

	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSLLH\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}

   CPU_SSE2_START//24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSLLW_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
   CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSLLWItoR( MM0, _Sa_ );
	PSLLWItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSLLW( void )
{


	if ( _Rd_ == 0 )return;

	if ( _Sa_ == 0 ) {
		SysPrintf("SA == 0 on PSLLW\n");
	    CPU_SSE2_START //09/04/06 Why weren't we copying it if sa == 0??! (Refraction)
			SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
		CPU_SSE2_END
		
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	}
   CPU_SSE2_START//24/05/2005 (shadow)
	   SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
       SSE2_PSLLD_I8_to_XMM(XMM0,_Sa_ );
	   SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
   CPU_SSE2_END

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PSLLDItoR( MM0, _Sa_ );
	PSLLDItoR( MM1, _Sa_ );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

/*
void recMADD( void ) 
{
}

void recMADDU( void ) 
{
}

void recPLZCW( void ) 
{
}
*/  

#ifdef MMI0_RECOMPILE

void recMMI0( void )
{
	recMMI0t[ _Sa_ ]( );
}

#endif

#ifdef MMI1_RECOMPILE

void recMMI1( void )
{
	recMMI1t[ _Sa_ ]( );
}

#endif

#ifdef MMI2_RECOMPILE

void recMMI2( void )
{
	recMMI2t[ _Sa_ ]( );
}

#endif

#ifdef MMI3_RECOMPILE

////////////////////////////////////////////////////
void recMMI3( void )
{
	recMMI3t[ _Sa_ ]( );
}

#endif

////////////////////////////////////////////////////
void recMTHI1( void )
{
//#ifdef NEW_MMI
//    CPU_SSE_START
//	SSE_MOVLPS_M64_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
//	SSE_MOVLPS_XMM_to_M64((u32)&cpuRegs.HI.UD[ 1 ], XMM0);
//	CPU_SSE_END
//#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQRtoM( (u32)&cpuRegs.HI.UD[ 1 ], MM0 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recMTLO1( void )
{
#ifdef NEW_MMI
/*	24/05/05 shadow hmm is that faster? at least it doesn't EMMS() :P
    CPU_SSE_START
	MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.LO.UD[ 0 ]);
    MOVHPS_M64_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	MOVAPS_XMM_to_M128((u32)&cpuRegs.LO.UD[ 0 ], XMM0 );
	CPU_SSE_END*/
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQRtoM( (u32)&cpuRegs.LO.UD[ 1 ], MM0 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recMFHI1( void )
{


	if ( ! _Rd_ ) return;
#ifdef NEW_MMI
/*	24/05/05 shadow hmm is that faster? at least it doesn't EMMS() :P
    CPU_SSE_START
	MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ]);
    MOVLPS_M64_to_XMM(XMM0, (u32)&cpuRegs.HI.UD[ 1 ]);
	MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
	CPU_SSE_END*/
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.HI.UD[ 1 ] );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recMFLO1( void )
{


	if ( ! _Rd_ ) return;
#ifdef NEW_MMI
/*	24/05/05 shadow hmm is that faster? at least it doesn't EMMS() :P
    CPU_SSE_START
	MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ]);
    MOVLPS_M64_to_XMM(XMM0, (u32)&cpuRegs.LO.UD[ 1 ]);
	MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
	CPU_SSE_END*/
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.LO.UD[ 1 ] );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recMULT1( void ) 
{
	//INC32M( (int)&cpuRegs.cycle);
	//INC32M( (int)&cpuRegs.CP0.n.Count);

	   MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
   //	XOR32RtoR( EDX, EDX );
   //	CDQ();
	   IMUL32M( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

	   MOV32RtoR( ECX, EDX );
	   MOV32RtoM( (u32)&cpuRegs.LO.UL[ 2 ], EAX );
	   CDQ( );
	   MOV32RtoM( (u32)&cpuRegs.LO.UL[ 3 ], EDX );
	   if ( _Rd_ != 0 )
	   {
		   MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		   MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
	   }

	   MOV32RtoR( EAX, ECX );
	   CDQ( );
	   MOV32RtoM( (u32)&cpuRegs.HI.UL[ 2 ], EAX );
	   MOV32RtoM( (u32)&cpuRegs.HI.UL[ 3 ], EDX );

	   MOV32ItoR(EAX, 3);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);

   
}

////////////////////////////////////////////////////
void recMULTU1( void ) 
{

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
//	XOR32RtoR( EDX, EDX );
	MUL32M  ( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

	MOV32RtoR( ECX, EDX );
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 2 ], EAX );
	CDQ();
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 3 ], EDX );
	if ( _Rd_ != 0 )
	{
		MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
	}

	MOV32RtoR( EAX, ECX );
	CDQ();
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 2 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 3 ], EDX );

	MOV32ItoR(EAX, 3);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);

}

////////////////////////////////////////////////////
void recDIV1( void )
{

	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32ItoR( ECX, 0 );
	j8Ptr[ 0 ] = JE8(0 );

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
//	XOR32RtoR( EDX, EDX );
	CDQ( );
	IDIV32R  ( ECX );

	MOV32RtoR( ECX, EDX );
	CDQ( );
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 2 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 3 ], EDX );

	MOV32RtoR( EAX, ECX );
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 2 ], ECX );
	CDQ( );
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 3 ], EDX );
	x86SetJ8( j8Ptr[ 0 ] );

	MOV32ItoR(EAX, 36);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);
}

////////////////////////////////////////////////////
void recDIVU1( void ) 
{

	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CMP32ItoR( ECX, 0 );
	j8Ptr[ 0 ] = JE8(0 );

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

	XOR32RtoR( EDX, EDX );
	DIV32R  ( ECX );

	MOV32RtoR( ECX, EDX );
	CDQ();
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 2 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.LO.UL[ 3 ], EDX );

	MOV32RtoR( EAX, ECX );
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 2 ], ECX );
	CDQ();
	MOV32RtoM( (u32)&cpuRegs.HI.UL[ 3 ], EDX );
	x86SetJ8(j8Ptr[ 0 ] );

	MOV32ItoR(EAX, 36);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);
}

#endif

/*

void recMADD1( void ) 
{
}

void recMADDU1( void ) 
{
}

void recPMFHL( void ) 
{
}

void recPMTHL( void ) 
{
}

void recPSLLH( void ) 
{
}

void recPSRLH( void ) 
{
}

void recPSRAH( void ) 
{
}

void recPSLLW( void ) 
{
}

void recPSRLW( void ) 
{
}

void recPSRAW( void ) 
{
}*/

/*********************************************************
*   MMI0 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI0_RECOMPILE

REC_FUNC( PADDB );
REC_FUNC( PADDH );
REC_FUNC( PADDW );
REC_FUNC( PADDSB );
REC_FUNC( PADDSH );
REC_FUNC( PADDSW );
REC_FUNC( PSUBB );
REC_FUNC( PSUBH );
REC_FUNC( PSUBW );
REC_FUNC( PSUBSB );
REC_FUNC( PSUBSH );
REC_FUNC( PSUBSW );

REC_FUNC( PMAXW ); //x86 dont have PMAXSD :(  we can use PCMPGTD
REC_FUNC( PMAXH );        

REC_FUNC( PCGTW );
REC_FUNC( PCGTH );
REC_FUNC( PCGTB );

REC_FUNC( PEXTLW );

REC_FUNC( PPACW );        
REC_FUNC( PEXTLH );
REC_FUNC( PPACH );        
REC_FUNC( PEXTLB );
REC_FUNC( PPACB );
REC_FUNC( PEXT5 );
REC_FUNC( PPAC5 );

#else

REC_FUNC( PADDSW );
REC_FUNC( PSUBSW );
////////////////////////////////////////////////////
void recPMAXW()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);

	SSE_MOVAPS_XMM_to_XMM(XMM2, XMM0);
	SSE2_PCMPGTD_XMM_to_XMM(XMM2, XMM1);

	SSE_ANDPS_XMM_to_XMM(XMM0, XMM2);
	SSE_ANDNPS_XMM_to_XMM(XMM2, XMM1);
	
	SSE_ORPS_XMM_to_XMM(XMM0, XMM2);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PMAXW );
}

////////////////////////////////////////////////////

__declspec(align(16)) static s_PackMasks[] = {
	0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff, // byte
	0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff }; // word

void recPPACW()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_SHUFPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ], 0x88 );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END

	//Done - Refraction - Crude but quicker than int
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].UL[2]); //Copy this one cos it could get overwritten

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[2]);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], ECX); //This is where we bring it back
	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
}

void recPPACH( void )
{
	if (!_Rd_) return;

CPU_SSE2_START

	SSE2_PSHUFLW_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ], 0x88);
	SSE2_PSHUFLW_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ], 0x88);
	SSE2_PSHUFHW_XMM_to_XMM(XMM0, XMM0, 0x88);
	SSE2_PSHUFHW_XMM_to_XMM(XMM1, XMM1, 0x88);
	SSE_SHUFPS_XMM_to_XMM(XMM0, XMM1, 0x88);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END

	//Done - Refraction - Crude but quicker than int
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[6]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[7], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[6]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[2]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[5], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[4]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[6], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[4]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[2], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[0]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[4], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[0]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[0], EAX);
	
}

////////////////////////////////////////////////////

void recPPACB()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	SSE_ANDPS_M128_to_XMM(XMM0, (u32)&s_PackMasks[0]);
	SSE_ANDPS_M128_to_XMM(XMM1, (u32)&s_PackMasks[0]);
	SSE2_PACKUSWB_XMM_to_XMM(XMM0, XMM1);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PPACB );
}

////////////////////////////////////////////////////
REC_FUNC( PEXT5 );
////////////////////////////////////////////////////
REC_FUNC( PPAC5 );

////////////////////////////////////////////////////
void recPMAXH( void ) 
{
	
	if ( ! _Rd_ ) return;
CPU_SSE2_START//24/05/2005 (shadow)
      if ( _Rs_ == 0 )
	 {
		if ( _Rt_ != 0 )
		{
           SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
           SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		   return;
		}
		else //RS and RT ==0
		{
          SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
          SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		  return;
		}
	 }
	 else
	 {
          SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	 }
     if ( _Rt_ == 0 )
	 {
           SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
           SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		   return;
	 }
	 else
	 {
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	 }
     SSE2_PMAXSW_XMM_to_XMM(XMM0,XMM1);
	 SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
CPU_SSE2_END


//TODO:optimize RS | RT== 0 PXOR MMx, MMx,no second load.
   CPU_SSE_START
	 MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	 MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	 SSE_PMAXSW_MM_to_MM( MM0, MM1 );
	 MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	 MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	 SSE_PMAXSW_MM_to_MM( MM2, MM3);
	 MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	 MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM2);
	 SetMMXstate();
   CPU_SSE_END
	COUNT_CYCLES(pc);
	   MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); 
	   MOV32ItoM( (u32)&cpuRegs.pc, pc ); 
	   CALLFunc( (u32)PMAXH ); 
   

}

////////////////////////////////////////////////////
void recPCGTB( void )
{
	//TODO:optimize RS | RT== 0
	if ( ! _Rd_ ) return;
#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    //SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	//SSE2_PCMPGTB_XMM_to_XMM(XMM0,XMM1);
    SSE2_PCMPGTB_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	PCMPGTBRtoR( MM0, MM1 );
	
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PCMPGTBRtoR( MM2, MM3);
	
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM2);
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPCGTH( void )
{
	//TODO:optimize RS | RT== 0
	if ( ! _Rd_ ) return;

#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested!
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    //SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	//SSE2_PCMPGTW_XMM_to_XMM(XMM0,XMM1);
    SSE2_PCMPGTW_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif

   MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

	PCMPGTWRtoR( MM0, MM1 );
	PCMPGTWRtoR( MM2, MM3);
	
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM2);
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPCGTW( void )
{
	//TODO:optimize RS | RT== 0
	if ( ! _Rd_ ) return;

#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested!
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    //SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	//SSE2_PCMPGTD_XMM_to_XMM(XMM0,XMM1);
    SSE2_PCMPGTD_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif
   MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

	PCMPGTDRtoR( MM0, MM1 );
	PCMPGTDRtoR( MM2, MM3);
	
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM2);
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDSB( void ) 
{
	if ( ! _Rd_ ) return;

  //TO be checked...
   CPU_SSE2_START//24/05/2005 (shadow)
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDSB_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 

    CPU_SSE2_END

   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		//MOVQMtoR( (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ], MM0 );
		//MOVQMtoR( (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ], MM2);//BUG!
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );//FIXED
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDSBRtoR( MM0, MM2);
	PADDSBRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDSH( void ) 
{
	if ( ! _Rd_ ) return;

    CPU_SSE2_START//24/05/2005 (shadow)
    //TO be checked...
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDSW_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDSWRtoR( MM0, MM2);
	PADDSWRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
/*void recPADDSW( void ) 
{


	if ( ! _Rd_ ) return;

   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDSDRtoR( MM0, MM2); //DOESN'T EXIST!!
	PADDSDRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}*/

////////////////////////////////////////////////////
void recPSUBSB( void ) 
{
   if ( ! _Rd_ ) return;
   CPU_SSE2_START//24/05/2005 (shadow)
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PSUBSB_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 

    CPU_SSE2_END
	



   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBSBRtoR( MM0, MM2);
	PSUBSBRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSUBSH( void ) 
{

	if ( ! _Rd_ ) return;
   CPU_SSE2_START//24/05/2005 (shadow)
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PSUBSW_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 

    CPU_SSE2_END


   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBSWRtoR( MM0, MM2);
	PSUBSWRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
/*void recPSUBSW( void ) 
{
	if ( ! _Rd_ ) return;



	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBSDRtoR( MM0, MM2 );  //DOESN'T EXIST!!
	PSUBSDRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}*/

////////////////////////////////////////////////////
void recPADDB( void )
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDB_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDBRtoR( MM0, MM2 );
	PADDBRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDH( void )
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDW_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDWRtoR( MM0, MM2 );
	PADDWRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDW( void ) 
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDD_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDDRtoR( MM0, MM2 );
	PADDDRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSUBB( void ) 
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PSUBB_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBBRtoR( MM0, MM2 );
	PSUBBRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSUBH( void ) 
{
	if ( ! _Rd_ ) return;


       CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PSUBW_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END
	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBWRtoR( MM0, MM2 );
	PSUBWRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPSUBW( void ) 
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PSUBD_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}

	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PSUBDRtoR( MM0, MM2);
	PSUBDRtoR( MM1, MM3);
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPEXTLW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKLDQ_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], ECX );

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX );

	
}

void recPEXTLB( void ) 
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKLBW_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	//Done - Refraction - Crude but quicker than int
	//SysPrintf("PEXTLB\n");
	//Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[7]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[15], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[7]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[14], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[6]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[13], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[6]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[12], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[5]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[11], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[5]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[10], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[4]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[9], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[4]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[8], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[3]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[7], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[3]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[6], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[2]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[5], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[2]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[4], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[1]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[3], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[1]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[2], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[0]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[1], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[0]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[0], EAX);	
}

void recPEXTLH( void )
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKLWD_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	//Done - Refraction - Crude but quicker than int
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[3]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[7], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[3]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[6], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[2]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[5], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[4], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[1]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[1]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[2], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[0]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].US[0]);
	MOV16RtoM((u32)&cpuRegs.GPR.r[_Rd_].US[0], EAX);
}
/*
void recPMAXW( void ) 
{
}

void recPEXT5( void ) 
{
}

void recPPAC5( void ) 
{
}
*/

#endif

/*********************************************************
*   MMI1 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI1_RECOMPILE

REC_FUNC( PABSW );
REC_FUNC( PABSH );

REC_FUNC( PMINW ); 
REC_FUNC( PADSBH );
REC_FUNC( PMINH );
REC_FUNC( PCEQB );   
REC_FUNC( PCEQH );
REC_FUNC( PCEQW );

REC_FUNC( PADDUB );
REC_FUNC( PADDUH );
REC_FUNC( PADDUW );

REC_FUNC( PSUBUB );
REC_FUNC( PSUBUH );
REC_FUNC( PSUBUW );

REC_FUNC( PEXTUW );   
REC_FUNC( PEXTUH );
REC_FUNC( PEXTUB );
REC_FUNC( QFSRV ); 

#else

////////////////////////////////////////////////////
REC_FUNC( PABSW );
////////////////////////////////////////////////////
REC_FUNC( PABSH );

////////////////////////////////////////////////////
void recPMINW()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);

	SSE_MOVAPS_XMM_to_XMM(XMM2, XMM0);
	SSE2_PCMPGTD_XMM_to_XMM(XMM2, XMM1);

	SSE_ANDPS_XMM_to_XMM(XMM1, XMM2);
	SSE_ANDNPS_XMM_to_XMM(XMM2, XMM0);
	
	SSE_ORPS_XMM_to_XMM(XMM1, XMM2);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM1);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PMINW );
}

////////////////////////////////////////////////////
REC_FUNC(PADSBH);

////////////////////////////////////////////////////
REC_FUNC( PADDUW );

////////////////////////////////////////////////////
void recPSUBUB()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);

	// if rs > rt, can do the sub, else put 0s
	// have to do unsigned compare
	SSE_XORPS_XMM_to_XMM(XMM4, XMM4);
	
	SSE_MOVAPS_XMM_to_XMM(XMM2, XMM0);
	SSE_MOVAPS_XMM_to_XMM(XMM3, XMM1);
	SSE2_PUNPCKLBW_XMM_to_XMM(XMM2, XMM4);
	SSE2_PUNPCKLBW_XMM_to_XMM(XMM3, XMM4);
	SSE2_PCMPGTW_XMM_to_XMM(XMM2, XMM3);

	SSE_MOVAPS_XMM_to_XMM(XMM5, XMM0);
	SSE_MOVAPS_XMM_to_XMM(XMM6, XMM1);
	SSE2_PUNPCKHBW_XMM_to_XMM(XMM5, XMM4);
	SSE2_PUNPCKHBW_XMM_to_XMM(XMM6, XMM4);
	SSE2_PCMPGTW_XMM_to_XMM(XMM5, XMM6);
	
	SSE2_PACKSSWB_XMM_to_XMM(XMM2, XMM5);
	SSE2_PSUBB_XMM_to_XMM(XMM0, XMM1);
	SSE_ANDPS_XMM_to_XMM(XMM0, XMM2);

	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PSUBUB );
}

////////////////////////////////////////////////////
void recPSUBUH()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);

	// if rs > rt, can do the sub, else put 0s
	// have to do unsigned compare
	SSE_XORPS_XMM_to_XMM(XMM4, XMM4);
	
	SSE_MOVAPS_XMM_to_XMM(XMM2, XMM0);
	SSE_MOVAPS_XMM_to_XMM(XMM3, XMM1);
	SSE2_PUNPCKLWD_XMM_to_XMM(XMM2, XMM4);
	SSE2_PUNPCKLWD_XMM_to_XMM(XMM3, XMM4);
	SSE2_PCMPGTD_XMM_to_XMM(XMM2, XMM3);

	SSE_MOVAPS_XMM_to_XMM(XMM5, XMM0);
	SSE_MOVAPS_XMM_to_XMM(XMM6, XMM1);
	SSE2_PUNPCKHWD_XMM_to_XMM(XMM5, XMM4);
	SSE2_PUNPCKHWD_XMM_to_XMM(XMM6, XMM4);
	SSE2_PCMPGTD_XMM_to_XMM(XMM5, XMM6);
	
	SSE2_PACKSSDW_XMM_to_XMM(XMM2, XMM5);
	SSE2_PSUBW_XMM_to_XMM(XMM0, XMM1);
	SSE_ANDPS_XMM_to_XMM(XMM0, XMM2);

	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PSUBUH );
}

////////////////////////////////////////////////////
REC_FUNC( PSUBUW );

////////////////////////////////////////////////////
void recPEXTUH()
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKHWD_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PEXTUH );
}

////////////////////////////////////////////////////
__declspec(align(16)) static s_64 = 64;

void recQFSRV()
{
	u8* pshift1, *pshift2, *poldptr, *pnewptr;

	if ( ! _Rd_ ) return;

CPU_SSE2_START

	MOV32MtoR(EAX, (u32)&cpuRegs.sa);
	SHR32ItoR(EAX, 3);
	MOV32ItoR(ECX, 16);
	SUB32RtoR(ECX, EAX);
	
	poldptr = x86Ptr;
	x86Ptr += 12;

	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);

	SSE2_PSRLDQ_I8_to_XMM(XMM0, 0);
	pshift1 = x86Ptr-1;
	SSE2_PSLLDQ_I8_to_XMM(XMM1, 0);
	pshift2 = x86Ptr-1;

	pnewptr = x86Ptr;
	x86Ptr = poldptr;

	MOV8RtoM((u32)pshift1, EAX);
	MOV8RtoM((u32)pshift2, ECX);

	x86Ptr = pnewptr;

	SSE_ORPS_XMM_to_XMM(XMM0, XMM1);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END
COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)QFSRV );
}


void recPEXTUB( void )
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKHBW_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	//Done - Refraction - Crude but faster than int
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[8]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[0], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[8]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[1], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[9]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[2], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[9]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[3], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[10]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[4], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[10]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[5], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[11]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[6], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[11]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[7], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[12]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[8], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[12]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[9], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[13]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[10], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[13]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[11], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[14]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[12], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[14]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[13], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rt_].UC[15]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[14], EAX);
	MOV8MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UC[15]);
	MOV8RtoM((u32)&cpuRegs.GPR.r[_Rd_].UC[15], EAX);

}
/*
void recPABSW( void ) 
{
}

void recPABSH( void ) 
{
}

void recPMINH( void ) 
{
}

void recPMINW( void ) 
{
}

void recPADSBH( void ) 
{
}

void recPADDUH( void ) 
{
}

void recPADDUW( void ) 
{
}

void recPSUBUW( void ) 
{
}

void recPSUBUH( void ) 
{
}

void recPEXTUH( void ) 
{
}
*/

////////////////////////////////////////////////////
void recPEXTUW( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PUNPCKHDQ_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 2 ] );
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 2 ] );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX );
	
	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 3 ] );
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 3 ] );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], ECX );
}

////////////////////////////////////////////////////
void recPMINH( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START//24/05/2005 (shadow)
      if ( _Rs_ == 0 )
	 {
		if ( _Rt_ != 0 )
		{
           SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
           SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		   return;
		}
		else //RS and RT ==0
		{
          SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
          SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		  return;
		}
	 }
	 else
	 {
          SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
	 }
     if ( _Rt_ == 0 )
	 {
           SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
           SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
		   return;
	 }
	 else
	 {
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	 }
     SSE2_PMINSW_XMM_to_XMM(XMM0,XMM1);
	 SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);  
CPU_SSE2_END

  CPU_SSE_START
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	SSE_PMINSW_MM_to_MM( MM0, MM2 );
	SSE_PMINSW_MM_to_MM( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
  CPU_SSE_END

	  COUNT_CYCLES(pc);
	   MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); 
	   MOV32ItoM( (u32)&cpuRegs.pc, pc ); 
	   CALLFunc( (u32)PMINH ); 
}

////////////////////////////////////////////////////
void recPCEQB( void )
{
	if ( ! _Rd_ ) return;

#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    //SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	//SSE2_PCMPEQB_XMM_to_XMM(XMM0,XMM1);
	SSE2_PCMPEQB_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PCMPEQBRtoR( MM0, MM2 );
	PCMPEQBRtoR( MM1, MM3 );

	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPCEQH( void )
{
	if ( ! _Rd_ ) return;

#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    SSE2_PCMPEQW_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif
	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PCMPEQWRtoR( MM0, MM2 );
	PCMPEQWRtoR( MM1, MM3 );
	
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPCEQW( void )
{
	if ( ! _Rd_ ) return;

#ifdef NEW_MMI
    CPU_SSE2_START //25/05/2005 shadow NOT tested
    SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
    //SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	//SSE2_PCMPEQD_XMM_to_XMM(XMM0,XMM1);
	SSE2_PCMPEQD_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0] ,XMM0);
    CPU_SSE2_END
#endif

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PCMPEQDRtoR( MM0, MM2 );
	PCMPEQDRtoR( MM1, MM3 );
	
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDUB( void ) 
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDUSB_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}
	
	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDUSBRtoR( MM0, MM2 );
	PADDUSBRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPADDUH( void ) 
{
	if ( ! _Rd_ ) return;

   CPU_SSE2_START //25/05/2005 shadow
   if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
	        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
		else
		{
			SSE2_PXOR_XMM_to_XMM(XMM0,XMM0);
            SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
			return;
		}
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);   
	}
	if ( _Rt_ == 0 )
	{
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
		return;
	}
	else
	{
        SSE_MOVAPS_M128_to_XMM(XMM1,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]); 
	}
	SSE2_PADDUSW_XMM_to_XMM(XMM0,XMM1);
    SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0); 
	CPU_SSE2_END

	if ( _Rs_ == 0 )
	{
		if ( _Rt_ != 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}
		else
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}
	}
	else
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	}
	
	if ( _Rt_ == 0 )
	{
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	else
	{
		MOVQMtoR( MM2, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM3, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	}
	PADDUSWRtoR( MM0, MM2 );
	PADDUSWRtoR( MM1, MM3 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

#endif
/*********************************************************
*   MMI2 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI2_RECOMPILE

REC_FUNC( PMFHI );
REC_FUNC( PMFLO );
REC_FUNC( PCPYLD );
REC_FUNC( PAND );
REC_FUNC( PXOR ); 

REC_FUNC( PMADDW );
REC_FUNC( PSLLVW );
REC_FUNC( PSRLVW ); 
REC_FUNC( PMSUBW );
REC_FUNC( PINTH );
REC_FUNC( PMULTW );
REC_FUNC( PDIVW );
REC_FUNC( PMADDH );
REC_FUNC( PHMADH );
REC_FUNC( PMSUBH );
REC_FUNC( PHMSBH );
REC_FUNC( PEXEH );
REC_FUNC( PREVH ); 
REC_FUNC( PMULTH );
REC_FUNC( PDIVBW );
REC_FUNC( PEXEW );
REC_FUNC( PROT3W ); 

#else

////////////////////////////////////////////////////
REC_FUNC( PMADDW );
////////////////////////////////////////////////////
REC_FUNC( PSLLVW );
////////////////////////////////////////////////////
REC_FUNC( PSRLVW ); 
////////////////////////////////////////////////////
REC_FUNC( PMSUBW );
////////////////////////////////////////////////////
//REC_FUNC( PINTH ); //-----
////////////////////////////////////////////////////
REC_FUNC( PMULTW );
////////////////////////////////////////////////////
REC_FUNC( PDIVW );
////////////////////////////////////////////////////
//REC_FUNC( PMADDH ); //-----
////////////////////////////////////////////////////
REC_FUNC( PHMADH ); //-----
////////////////////////////////////////////////////
REC_FUNC( PMSUBH ); //---
////////////////////////////////////////////////////
REC_FUNC( PHMSBH ); //---
////////////////////////////////////////////////////
REC_FUNC( PEXEH ); //----
////////////////////////////////////////////////////
REC_FUNC( PREVH ); //--- 
////////////////////////////////////////////////////
//REC_FUNC( PMULTH );
////////////////////////////////////////////////////
REC_FUNC( PDIVBW ); //--
////////////////////////////////////////////////////
//REC_FUNC( PEXEW ); //--
////////////////////////////////////////////////////
//REC_FUNC( PROT3W ); //--

////////////////////////////////////////////////////

void recPINTH( void ) //Done - Refraction
{
	if (!_Rd_) return;

	
	MOV16MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[4]);
	MOV16MtoR( EBX, (u32)&cpuRegs.GPR.r[_Rt_].US[1]);
	MOV16MtoR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].US[2]);
	MOV16MtoR( EDX, (u32)&cpuRegs.GPR.r[_Rt_].US[0]);

	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[1], EAX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[2], EBX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[4], ECX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[0], EDX);

	MOV16MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].US[5]);
	MOV16MtoR( EBX, (u32)&cpuRegs.GPR.r[_Rs_].US[6]);
	MOV16MtoR( ECX, (u32)&cpuRegs.GPR.r[_Rs_].US[7]);
	MOV16MtoR( EDX, (u32)&cpuRegs.GPR.r[_Rt_].US[3]);

	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[3], EAX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[5], EBX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[7], ECX);
	MOV16RtoM( (u32)&cpuRegs.GPR.r[_Rd_].US[6], EDX);
}

void recPEXEW( void ) //Done - Refraction
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM0, 0xc6);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rt_].UL[2]);
	MOV32MtoR( EBX, (u32)&cpuRegs.GPR.r[_Rt_].UL[1]);
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32MtoR( EDX, (u32)&cpuRegs.GPR.r[_Rt_].UL[3]);

	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], EBX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[2], ECX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[3], EDX);
}

void recPROT3W( void )  //Done - Refraction
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM0, 0xc9);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[_Rt_].UL[1]);
	MOV32MtoR( EBX, (u32)&cpuRegs.GPR.r[_Rt_].UL[2]);
	MOV32MtoR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].UL[0]);
	MOV32MtoR( EDX, (u32)&cpuRegs.GPR.r[_Rt_].UL[3]);

	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], EBX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[2], ECX);
	MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[3], EDX);
}
void recPMULTH( void ) //Done - Refraction
{
	if(!_Rt_ || !_Rs_) {
		MOV32ItoM( (u32)&cpuRegs.LO.UL[0], 0);
		MOV32ItoM( (u32)&cpuRegs.LO.UL[1], 0);
		MOV32ItoM( (u32)&cpuRegs.LO.UL[2], 0);
		MOV32ItoM( (u32)&cpuRegs.LO.UL[3], 0);
		MOV32ItoM( (u32)&cpuRegs.HI.UL[0], 0);
		MOV32ItoM( (u32)&cpuRegs.HI.UL[1], 0);
		MOV32ItoM( (u32)&cpuRegs.HI.UL[2], 0);
		MOV32ItoM( (u32)&cpuRegs.HI.UL[3], 0);
		return;
		
	}

//CPU_SSE2_START
//	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
//	SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM0, 0xc9);
//	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
//
//	PMULLW
//	PMULHW
//
//CPU_SSE2_END

	MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[0]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[0]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.LO.UL[0], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[1]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[1]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.LO.UL[1], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[2]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[2]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.HI.UL[0], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[3]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[3]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.HI.UL[1], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[4]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[4]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.LO.UL[2], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[5]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[5]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.LO.UL[3], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[6]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[6]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.HI.UL[2], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[7]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[7]);
	IMUL32RtoR( EAX, ECX);
	MOV32RtoM( (u32)&cpuRegs.HI.UL[3], EAX);

	if (_Rd_) {
		MOV32MtoR( EAX, (u32)&cpuRegs.LO.UL[0]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.HI.UL[0]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.LO.UL[2]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.HI.UL[2]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	}
}

void recPMFHI( void )
{
	if ( ! _Rd_ ) return;



    CPU_SSE_START
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.HI.UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
    CPU_SSE_END

		MOVQMtoR( MM0, (u32)&cpuRegs.HI.UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.HI.UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	
}

////////////////////////////////////////////////////
void recPMFLO( void )
{
	if ( ! _Rd_ ) return;


    CPU_SSE_START
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.LO.UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
    CPU_SSE_END

		MOVQMtoR( MM0, (u32)&cpuRegs.LO.UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.LO.UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();

}

////////////////////////////////////////////////////
void recPAND( void ) 
{
	if ( ! _Rd_ ) return;


    CPU_SSE_START
		if ( _Rt_ == 0 || _Rs_ == 0 )
		{
			SSE_XORPS_XMM_to_XMM( XMM0,XMM0 );
			SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
			return;
		}

		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		SSE_ANDPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
    CPU_SSE_END

		if ( _Rt_ == 0 || _Rs_ == 0 ) 
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}

		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PANDMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PANDMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	
}

////////////////////////////////////////////////////
void recPXOR( void ) 
{
	if ( ! _Rd_ ) return;


    CPU_SSE_START
	if ( _Rt_ == 0 && _Rs_ == 0 )
		{
			SSE_XORPS_XMM_to_XMM( XMM0,XMM0 );
			SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
			return;
		}

		if ( _Rt_ == 0 )
		{
			SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
			SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
			return;
		}
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		SSE_XORPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
     CPU_SSE_END

		if ( _Rt_ == 0 && _Rs_ == 0 )
		{
			PXORRtoR( MM0, MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
			SetMMXstate();
			return;
		}

		if ( _Rs_ == 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}

		if ( _Rt_ == 0 )
		{
			MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
			MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );

			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
			MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
			SetMMXstate();
			return;
		}

		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PXORMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		PXORMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );

		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
	
}

/*
void recPMADDW( void ) 
{
}

void recPSLLVW( void ) 
{
}

void recPSRLVW( void ) 
{
} 

void recPMSUBW( void ) 
{
}

void recPMFLO( void ) 
{
}



void recPMULTW( void ) 
{
}

void recPDIVW( void ) 
{
}
*/

////////////////////////////////////////////////////
void recPCPYLD( void ) 
{
	if ( ! _Rd_ ) return;

	CPU_SSE_START
		SSE_MOVLPS_M64_to_XMM( XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		SSE_MOVHPS_M64_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[0]);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
	CPU_SSE_END

	 MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	 MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	 MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
	 MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM1 );
	 SetMMXstate();

}


void recPMADDH( void ) 
{
	if(_Rt_ && _Rs_){

	MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[0]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[0]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.LO.UL[0], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[1]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[1]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.LO.UL[1], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[2]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[2]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.HI.UL[0], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[3]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[3]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.HI.UL[1], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[4]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[4]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.LO.UL[2], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[5]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[5]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.LO.UL[3], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[6]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[6]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.HI.UL[2], EAX);

    MOVSX32M16toR( EAX, (u32)&cpuRegs.GPR.r[_Rs_].SS[7]);
	MOVSX32M16toR( ECX, (u32)&cpuRegs.GPR.r[_Rt_].SS[7]);
	IMUL32RtoR( EAX, ECX);
	ADD32RtoM( (u32)&cpuRegs.HI.UL[3], EAX);

	}

	if (_Rd_) {
		MOV32MtoR( EAX, (u32)&cpuRegs.LO.UL[0]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.HI.UL[0]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.LO.UL[2]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[2], EAX);
		MOV32MtoR( EAX, (u32)&cpuRegs.HI.UL[2]);
		MOV32RtoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[3], EAX);
	}
}
/*
void recPHMADH( void ) 
{
}

void recPAND( void ) 
{
}

void recPXOR( void ) 
{
} 

void recPMSUBH( void ) 
{
}

void recPHMSBH( void ) 
{
}

void recPEXEH( void ) 
{
}

void recPREVH( void ) 
{
} 

void recPDIVBW( void ) 
{
}
*/



#endif
/*********************************************************
*   MMI3 opcodes                                         *
*                                                        *
*********************************************************/
#ifndef MMI3_RECOMPILE

REC_FUNC( PMADDUW );
REC_FUNC( PSRAVW ); 
REC_FUNC( PMTHI );
REC_FUNC( PMTLO );
REC_FUNC( PINTEH );
REC_FUNC( PMULTUW );
REC_FUNC( PDIVUW );
REC_FUNC( PCPYUD );
REC_FUNC( POR );
REC_FUNC( PNOR );  
REC_FUNC( PCPYH ); 
REC_FUNC( PEXCW );
REC_FUNC( PEXCH );

#else

////////////////////////////////////////////////////
REC_FUNC( PMADDUW );
////////////////////////////////////////////////////
REC_FUNC( PSRAVW ); 

////////////////////////////////////////////////////
void recPINTEH()
{

	if ( ! _Rd_ ) return;

CPU_SSE2_START
	if ( _Rs_ == 0 && _Rt_ == 0 )
	{
		SSE_XORPS_XMM_to_XMM(XMM0, XMM0);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
		return;
	}

	if ( _Rs_ == 0 )
	{
		SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		SSE_ANDPS_M128_to_XMM(XMM0, (int)&s_UnpackMasks[0]);
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
		return;
	}
	
	if ( _Rt_ == 0 )
	{
		SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		SSE2_PSLLD_I8_to_XMM(XMM0, 16);
        SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
		return;
	}

	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE_MOVAPS_M128_to_XMM(XMM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	SSE_ANDPS_M128_to_XMM(XMM0, (int)&s_UnpackMasks[0]);
	SSE2_PSLLD_I8_to_XMM(XMM1, 16);
	SSE_ORPS_XMM_to_XMM(XMM0, XMM1);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);

CPU_SSE2_END

COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PINTEH );
}

////////////////////////////////////////////////////
REC_FUNC( PMULTUW );
////////////////////////////////////////////////////
REC_FUNC( PDIVUW );

////////////////////////////////////////////////////
void recPEXCW()
{
	if (!_Rd_) return;

CPU_SSE2_START
	SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
	SSE2_PSHUFD_XMM_to_XMM(XMM0, XMM0, 0xd8);
	SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0);
CPU_SSE2_END

COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	CALLFunc( (u32)PEXCW );
}

////////////////////////////////////////////////////
REC_FUNC( PEXCH );

////////////////////////////////////////////////////
void recPNOR( void ) 
{
	if ( ! _Rd_ ) return;

CPU_SSE2_START
	if ( _Rs_ == 0 && _Rt_ == 0 )
	{
        SSE2_PCMPEQD_XMM_to_XMM(XMM0,XMM0);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
		return;
	}

	if ( _Rs_ == 0 )
	{
		SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
        SSE2_PCMPEQD_XMM_to_XMM(XMM1,XMM1);
		SSE_ANDNPS_XMM_to_XMM(XMM0,XMM1);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
		return;
	}
	
	if ( _Rt_ == 0 )
	{
		SSE_MOVAPS_M128_to_XMM(XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
        SSE2_PCMPEQD_XMM_to_XMM(XMM1,XMM1);
		SSE_ANDNPS_XMM_to_XMM(XMM0,XMM1);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
		return;
	}
	SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	SSE2_POR_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	SSE2_PCMPEQD_XMM_to_XMM(XMM1,XMM1);
	SSE_ANDNPS_XMM_to_XMM(XMM0,XMM1);
	SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], XMM0 );
CPU_SSE2_END

	if ( _Rs_ == 0 && _Rt_ == 0 )
	{
		PCMPEQDRtoR( MM0, MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM0 );
		SetMMXstate();
		return;
	}

	if ( _Rs_ == 0 )
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		PCMPEQDRtoR( MM2, MM2);
		PXORRtoR( MM0, MM2);
		PXORRtoR( MM1, MM2);
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}
	
	if ( _Rt_ == 0 )
	{
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		PCMPEQDRtoR( MM2, MM2);
		PXORRtoR( MM0, MM2);
		PXORRtoR( MM1, MM2);
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();
		return;
	}

	MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
	PORMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	PORMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	PCMPEQDRtoR( MM2, MM2 );
	PXORRtoR( MM0, MM2 );
	PXORRtoR( MM1, MM2 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
	SetMMXstate();
}

////////////////////////////////////////////////////
void recPMTHI( void ) 
{

     CPU_SSE_START
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.HI.UD[ 0 ],XMM0 );
     CPU_SSE_END
		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.HI.UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.HI.UD[ 1 ], MM1 );
		SetMMXstate();

}

////////////////////////////////////////////////////
void recPMTLO( void ) 
{


     CPU_SSE_START
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.LO.UD[ 0 ],XMM0 );
     CPU_SSE_END

		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		MOVQRtoM( (u32)&cpuRegs.LO.UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.LO.UD[ 1 ], MM1 );
		SetMMXstate();
	
}

////////////////////////////////////////////////////
void recPCPYUD( void ) 
{
   if ( ! _Rd_ ) return;

	CPU_SSE_START
		SSE_MOVHPS_M64_to_XMM( XMM0,(u32)&cpuRegs.GPR.r[ _Rt_ ].UD[1 ] );
		SSE_MOVLPS_M64_to_XMM(XMM0,(u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ]);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0);
	CPU_SSE_END

   MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
   MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
   MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
   MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
   SetMMXstate();
}

////////////////////////////////////////////////////
void recPOR( void ) 
{
	if ( ! _Rd_ ) return;



     CPU_SSE_START
		SSE_MOVAPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		if ( _Rt_ != 0 )
		{
			SSE_ORPS_M128_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		}
		SSE_MOVAPS_XMM_to_M128( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ],XMM0 );
     CPU_SSE_END

		MOVQMtoR( MM0, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
		MOVQMtoR( MM1, (u32)&cpuRegs.GPR.r[ _Rs_ ].UD[ 1 ] );
		if ( _Rt_ != 0 )
		{
			PORMtoR ( MM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
			PORMtoR ( MM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		}
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
		MOVQRtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UD[ 1 ], MM1 );
		SetMMXstate();

}

////////////////////////////////////////////////////
void recPCPYH( void ) 
{
	if ( ! _Rd_ ) return;

	CPU_SSE_START
		SSE_MOVSS_M32_to_XMM( XMM0, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
		SSE_MOVSS_M32_to_XMM( XMM1, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
		SSE2_PSHUFLW_XMM_to_XMM(XMM0, XMM0, 0);
		SSE2_PSHUFLW_XMM_to_XMM(XMM1, XMM1, 0);
		SSE_MOVLHPS_XMM_to_XMM(XMM0, XMM1);
		SSE_MOVAPS_XMM_to_M128((u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], XMM0);
	CPU_SSE_END

	//PUSH32R( EBX );
	MOVZX32M16toR( EAX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
	MOV32RtoR( ECX, EAX );
	SHL32ItoR( ECX, 16 );
	OR32RtoR( EAX, ECX );
	MOVZX32M16toR( EDX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UD[ 1 ] );
	MOV32RtoR( ECX, EDX );
	SHL32ItoR( ECX, 16 );
	OR32RtoR( EDX, ECX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], EDX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rd_ ].UL[ 3 ], EDX );
	//POP32R( EBX );
} 


#endif


