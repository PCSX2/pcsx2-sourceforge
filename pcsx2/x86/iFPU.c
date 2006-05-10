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
#include "iFPU.h"

extern void COUNT_CYCLES(u32 curpc);

#define REC_FPUBRANCH(f) \
	void f(); \
	void rec##f() { \
	COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(); \
	CALLFunc((u32)f); \
	branch = 2; \
}

#define REC_FPUFUNC(f) \
	void f(); \
	void rec##f() { \
	MOV32ItoR(EAX, 3); \
	ADD32RtoM((u32)&cpuRegs.cycle, EAX); \
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX); \
	COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&cpuRegs.code, cpuRegs.code); \
	MOV32ItoM((u32)&cpuRegs.pc, pc); \
	iFlushCall(); \
	CALLFunc((u32)f); \
}

/*********************************************************
*   COP1 opcodes                                         *
*                                                        *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

	 

////////////////////////////////////////////////////
void recMFC1(void) {
	if ( ! _Rt_ ) return;

	_freeXMMregs();
	MOV32MtoR( EAX, (u32)&fpuRegs.fpr[ _Fs_ ].UL );
	CDQ( );

	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recCFC1(void) {
	if (!_Rt_) return;

	MOV32MtoR( EAX, (u32)&fpuRegs.fprc[ _Fs_ ] );
	CDQ( );

	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32RtoM( (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recMTC1(void) {
	_freeXMMregs();
	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
	MOV32RtoM((u32)&fpuRegs.fpr[ _Fs_ ].UL, EAX);

	MOV32ItoR(EAX, 1);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);
}

////////////////////////////////////////////////////
void recCTC1( void ) {
	MOV32MtoR( EAX, (u32)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	MOV32RtoM( (u32)&fpuRegs.fprc[ _Fs_ ], EAX );
}

////////////////////////////////////////////////////
//REC_SYS(COP1_BC1);

void recCOP1_BC1() {
#ifdef CPU_LOG
	CPU_LOG( "Recompiling FPU:%s\n", disR5900Fasm( cpuRegs.code, cpuRegs.pc ) );
#endif
	recCP1BC1[_Rt_]();
}

//REC_FUNC(COP1_S);
//REC_FUNC(COP1_W);
//REC_FUNC(CVT_S);


static u32 _mxcsr = 0x7F80;
static u32 _mxcsrs;
static u32 fpucw = 0x007f;
static u32 fpucws = 0;

////////////////////////////////////////////////////
void SaveCW(int type) {
	if (iCWstate & type) return;

	if (type == 2) {
		SSE_STMXCSR((u32)&_mxcsrs);
		SSE_LDMXCSR((u32)&_mxcsr);
	} else {
		FNSTCW( (u32)&fpucws );
		FLDCW( (u32)&fpucw );
	}
	iCWstate|= type;
}

////////////////////////////////////////////////////
void LoadCW( void ) {
	if (iCWstate == 0) return;

	if (iCWstate & 2) {
		SSE_LDMXCSR((u32)&_mxcsrs);
	}
	if (iCWstate & 1) {
		FLDCW( (u32)&fpucws );
	}
	iCWstate = 0;
}

////////////////////////////////////////////////////
void recCOP1_S( void ) 
{
#ifdef CPU_LOG
	CPU_LOG( "Recompiling FPU:%s\n", disR5900Fasm( cpuRegs.code, cpuRegs.pc ) );
#endif
   recCP1S[ _Funct_ ]( );
}

////////////////////////////////////////////////////
void recCOP1_W( void ) 
{
#ifdef CPU_LOG
	CPU_LOG( "Recompiling FPU:%s\n", disR5900Fasm( cpuRegs.code, cpuRegs.pc ) );
#endif
   recCP1W[ _Funct_ ]( );
}

#ifndef FPU_RECOMPILE


REC_FPUFUNC(ADD_S);
REC_FPUFUNC(SUB_S);
REC_FPUFUNC(MUL_S);
REC_FPUFUNC(DIV_S);
REC_FPUFUNC(SQRT_S);
REC_FPUFUNC(RSQRT_S);
REC_FPUFUNC(ABS_S);
REC_FPUFUNC(MOV_S);
REC_FPUFUNC(NEG_S);
REC_FPUFUNC(ADDA_S);
REC_FPUFUNC(SUBA_S);
REC_FPUFUNC(MULA_S);
REC_FPUFUNC(MADD_S);
REC_FPUFUNC(MSUB_S);
REC_FPUFUNC(MADDA_S);
REC_FPUFUNC(MSUBA_S);
REC_FPUFUNC(CVT_S);
REC_FPUFUNC(CVT_W);
REC_FPUFUNC(MIN_S);
REC_FPUFUNC(MAX_S);
REC_FPUBRANCH(BC1F);
REC_FPUBRANCH(BC1T);
REC_FPUBRANCH(BC1FL);
REC_FPUBRANCH(BC1TL);
REC_FPUFUNC(C_F);
REC_FPUFUNC(C_EQ);
REC_FPUFUNC(C_LE);
REC_FPUFUNC(C_LT);

#else

REC_FPUFUNC(CVT_W);

////////////////////////////////////////////////////

void recC_EQ( void )
{

	_freeXMMregs();
	   SetFPUstate();

	   FLD32( (u32)&fpuRegs.fpr[_Fs_].f);
	   FCOMP32( (u32)&fpuRegs.fpr[_Ft_].f);
	   FNSTSWtoAX( );
	   TEST32ItoR( EAX, 0x00004000 );
	   j8Ptr[ 0 ] = JE8( 0 );
	   OR32ItoM( (u32)&fpuRegs.fprc[ 31 ], 0x00800000 );
	   j8Ptr[ 1 ] = JMP8( 0 );

	   x86SetJ8( j8Ptr[ 0 ] );
	   AND32ItoM( (u32)&fpuRegs.fprc[ 31 ], ~0x00800000 );

	   x86SetJ8( j8Ptr[ 1 ] );

	   //Fcr31Update()
   
}	

////////////////////////////////////////////////////
void recC_F( void ) 
{
	_freeXMMregs();
	   SetFPUstate();
	   
	   FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
	   FCOMP32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
	   FNSTSWtoAX( );
	   TEST32ItoR( EAX, 0x00004000 );
	   j8Ptr[ 0 ] = JNE8( 0 );
	   OR32ItoM( (u32)&fpuRegs.fprc[31], 0x00800000 );
	   j8Ptr[ 1 ] = JMP8( 0 );

	   x86SetJ8( j8Ptr[ 0 ] );
	   AND32ItoM( (u32)&fpuRegs.fprc[31], ~0x00800000 );

	   x86SetJ8( j8Ptr[ 1 ] );
	   //Fcr31Update()
  
}	

////////////////////////////////////////////////////
void recC_LT( void )
{
	_freeXMMregs();
	   SetFPUstate();

	   FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
	   FCOMP32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
	   FNSTSWtoAX( );
	   TEST32ItoR( EAX, 0x00000100 );
	   j8Ptr[ 0 ] = JE8( 0 );
	   OR32ItoM( (u32)&fpuRegs.fprc[ 31 ], 0x00800000 );
	   j8Ptr[ 1 ] = JMP8( 0 );

	   x86SetJ8( j8Ptr[ 0 ] );
	   AND32ItoM( (u32)&fpuRegs.fprc[ 31 ], ~0x00800000 );

	   x86SetJ8( j8Ptr[1] );
   
}

////////////////////////////////////////////////////
void recC_LE( void ) 
{
	_freeXMMregs();
	   SetFPUstate();

	   FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
	   FCOMP32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
	   FNSTSWtoAX( );
	   TEST32ItoR( EAX, 0x00004100 );
	   j8Ptr[ 0 ] = JE8( 0 );
	   OR32ItoM( (u32)&fpuRegs.fprc[ 31 ], 0x00800000 );
	   j8Ptr[ 1 ] = JMP8( 0 );

	   x86SetJ8( j8Ptr[ 0 ] );
	   AND32ItoM( (u32)&fpuRegs.fprc[ 31 ], ~0x00800000 );

	   x86SetJ8( j8Ptr[ 1 ] );
   
}

////////////////////////////////////////////////////

void recADD_S(void) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Fs_) {
			SSE_ADDSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			SSE_ADDSS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_ADDSS_XMM_to_XMM(fdreg, ftreg);
		}

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();

		SaveCW(1);
		FLD32(  (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FADD32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

////////////////////////////////////////////////////

void recSUB_S( void ) { //ok
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;
		int t0reg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Fs_) {
			SSE_SUBSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			t0reg = _allocTempXMMreg(-1);
			SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
			SSE_SUBSS_XMM_to_XMM(t0reg, fdreg);
			SSE_MOVSS_XMM_to_XMM(fdreg, t0reg);
			_freeXMMreg(t0reg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_SUBSS_XMM_to_XMM(fdreg, ftreg);
		}

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();

		SaveCW(1);
		FLD32(  (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FSUB32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

////////////////////////////////////////////////////
void recMUL_S( void ) { //ok
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Fs_) {
			SSE_MULSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			SSE_MULSS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULSS_XMM_to_XMM(fdreg, ftreg);
		}

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

////////////////////////////////////////////////////

void recDIV_S( void ) { //ok
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;
		int t0reg;

		MOV32MtoR( EAX, (u32)&fpuRegs.fpr[ _Ft_ ].f );
		OR32RtoR( EAX, EAX );
		j8Ptr[ 0 ] = JE8( 0 );

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Fs_) {
			SSE_DIVSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			t0reg = _allocTempXMMreg(-1);
			SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
			SSE_DIVSS_XMM_to_XMM(t0reg, fdreg);
			SSE_MOVSS_XMM_to_XMM(fdreg, t0reg);
			_freeXMMreg(t0reg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_DIVSS_XMM_to_XMM(fdreg, ftreg);
		}

		_clearNeededXMMregs();
		_freeXMMreg(fdreg);

		x86SetJ8( j8Ptr[ 0 ] );
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		MOV32MtoR( EAX, (u32)&fpuRegs.fpr[ _Ft_ ].f );
		OR32RtoR( EAX, EAX );
		j8Ptr[ 0 ] = JE8( 0 );
		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FDIV32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
		x86SetJ8( j8Ptr[ 0 ] );
	}
}

////////////////////////////////////////////////////

void recSQRT_S( void ) { //ok
	_freeXMMregs();

	   SetFPUstate();
	   SaveCW(1);

	   FLD32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
	   FSQRT( );
	   FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
   
}

////////////////////////////////////////////////////

void recABS_S( void ) { //ok
	_freeXMMregs();
	MOV32MtoR( EAX, (u32)&fpuRegs.fpr[ _Fs_ ].f );
	AND32ItoR( EAX, 0x7fffffff );
	MOV32RtoM( (u32)&fpuRegs.fpr[ _Fd_ ].f, EAX );
}

////////////////////////////////////////////////////

void recMOV_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int fdreg;

		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
	    MOV32MtoR( EAX, (u32)&fpuRegs.fpr[ _Fs_ ].UL );
		MOV32RtoM( (u32)&fpuRegs.fpr[ _Fd_ ].UL, EAX );
	}
}

////////////////////////////////////////////////////

static u32 _neg_s[4] = { 0x80000000, 0, 0, 0 };

void recNEG_S( void ) { //OK 
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int fdreg;

		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

		if (_Fd_ == _Fs_) {
			SSE_XORPS_M128_to_XMM(fdreg, (uptr)&_neg_s);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_XORPS_M128_to_XMM(fdreg, (uptr)&_neg_s);
		}

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		MOV32MtoR( EAX,(u32)&fpuRegs.fpr[ _Fs_ ].f );
		XOR32ItoR( EAX, 0x80000000 );
		MOV32RtoM( (u32)&fpuRegs.fpr[ _Fd_ ].f, EAX );
	}
}

////////////////////////////////////////////////////

void recRSQRT_S( void ) { //ok
	static u32 tmp;

	_freeXMMregs();
	SetFPUstate();
	SaveCW(1);

	FLD32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
	FSQRT( );
	FSTP32( (u32)&tmp );

	MOV32MtoR( EAX, (u32)&tmp );
	OR32RtoR( EAX, EAX );
	j8Ptr[ 0 ] = JE8( 0 );				
	FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
	FDIV32( (u32)&tmp );
	FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	x86SetJ8( j8Ptr[ 0 ] );
}

////////////////////////////////////////////////////


void recADDA_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int accreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		accreg = _allocFPACCtoXMMreg(-1, MODE_WRITE);

		SSE_MOVSS_XMM_to_XMM(accreg, fsreg);
		SSE_ADDSS_XMM_to_XMM(accreg, ftreg);

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FADD32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.ACC.f );
	}
}

////////////////////////////////////////////////////
void recSUBA_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int accreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		accreg = _allocFPACCtoXMMreg(-1, MODE_WRITE);

		SSE_MOVSS_XMM_to_XMM(accreg, fsreg);
		SSE_SUBSS_XMM_to_XMM(accreg, ftreg);

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FSUB32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.ACC.f );
	}
}

////////////////////////////////////////////////////
void recMULA_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int accreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		accreg = _allocFPACCtoXMMreg(-1, MODE_WRITE);

		SSE_MOVSS_XMM_to_XMM(accreg, fsreg);
		SSE_MULSS_XMM_to_XMM(accreg, ftreg);

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSTP32( (u32)&fpuRegs.ACC.f );
	}
}

////////////////////////////////////////////////////

void recMADD_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;
		int accreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);
		accreg = _allocFPACCtoXMMreg(-1, MODE_READ);

		if (_Fd_ == _Fs_) {
			SSE_MULSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			SSE_MULSS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_MULSS_XMM_to_XMM(fdreg, ftreg);
		}
		SSE_ADDSS_XMM_to_XMM(fdreg, accreg);

		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FADD32( (u32)&fpuRegs.ACC.f );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

////////////////////////////////////////////////////
void recMSUB_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;
		int t0reg;
		int accreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);
		accreg = _allocFPACCtoXMMreg(-1, MODE_READ);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULSS_XMM_to_XMM(t0reg, ftreg);
		SSE_MOVSS_XMM_to_XMM(fdreg, accreg);
		SSE_SUBSS_XMM_to_XMM(fdreg, t0reg);

		_freeXMMreg(t0reg);
		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.ACC.f );
		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSUBP( );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

////////////////////////////////////////////////////

void recMADDA_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int accreg;
		int t0reg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		accreg = _allocFPACCtoXMMreg(-1, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULSS_XMM_to_XMM(t0reg, ftreg);
		SSE_ADDSS_XMM_to_XMM(accreg, t0reg);

		_freeXMMreg(t0reg);
		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FADD32( (u32)&fpuRegs.ACC.f );
		FSTP32( (u32)&fpuRegs.ACC.f );
	}
}

////////////////////////////////////////////////////
void recMSUBA_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int accreg;
		int t0reg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPACCtoXMMreg();
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		accreg = _allocFPACCtoXMMreg(-1, MODE_READ | MODE_WRITE);
		t0reg = _allocTempXMMreg(-1);

		SSE_MOVSS_XMM_to_XMM(t0reg, fsreg);
		SSE_MULSS_XMM_to_XMM(t0reg, ftreg);
		SSE_SUBSS_XMM_to_XMM(accreg, t0reg);

		_freeXMMreg(t0reg);
		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		SetFPUstate();
		SaveCW(1);

		FLD32( (u32)&fpuRegs.ACC.f );
		FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
		FMUL32( (u32)&fpuRegs.fpr[ _Ft_ ].f );
		FSUBP( );
		FSTP32( (u32)&fpuRegs.ACC.f );
	}
}

////////////////////////////////////////////////////

void recCVT_S( void ) {
	/*if (CHECK_REGCACHING && cpucaps.hasStreamingSIMD2Extensions) {
		int fsreg;
		int fdreg;

//		_freeXMMregs();
		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);

        SSE2_CVTDQ2PS_XMM_to_XMM(fdreg, fsreg);
		_clearNeededXMMregs();
//		_freeXMMregs();
	} else*/ {
		_freeXMMregs();
		SetFPUstate();

		FILD32( (u32)&fpuRegs.fpr[ _Fs_ ].UL );
		FSTP32( (u32)&fpuRegs.fpr[ _Fd_ ].f );
	}
}

/*
////////////////////////////////////////////////////
void recCVT_W( void ) 
{
	_freeXMMregs();

	   SetFPUstate();
	   FLD32( (u32)&fpuRegs.fpr[ _Fs_ ].f );
	   FISTP32( (u32)&fpuRegs.fpr[ _Fd_ ].UL );
   
}
*/
////////////////////////////////////////////////////
void recMAX_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);
		if (_Fd_ == _Fs_) {
			SSE_MAXSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			SSE_MAXSS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_MAXSS_XMM_to_XMM(fdreg, ftreg);
		}
		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		COUNT_CYCLES(pc);
		MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
		MOV32ItoM( (u32)&cpuRegs.pc, pc );
		iFlushCall();
		CALLFunc( (u32)MAX_S );
	}
   
}

////////////////////////////////////////////////////
void recMIN_S( void ) {
	if (CHECK_REGCACHING && cpucaps.hasStreamingSIMDExtensions) {
		int fsreg;
		int ftreg;
		int fdreg;

		SaveCW(2);
		_addNeededFPtoXMMreg(_Fs_);
		_addNeededFPtoXMMreg(_Ft_);
		_addNeededFPtoXMMreg(_Fd_);
		fsreg = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
		ftreg = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
		fdreg = _allocFPtoXMMreg(-1, _Fd_, MODE_WRITE);
		if (_Fd_ == _Fs_) {
			SSE_MINSS_XMM_to_XMM(fdreg, ftreg);
		} else
		if (_Fd_ == _Ft_) {
			SSE_MINSS_XMM_to_XMM(fdreg, fsreg);
		} else {
			SSE_MOVSS_XMM_to_XMM(fdreg, fsreg);
			SSE_MINSS_XMM_to_XMM(fdreg, ftreg);
		}
		_clearNeededXMMregs();
	} else {
		_freeXMMregs();
		COUNT_CYCLES(pc);
		MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); 
		MOV32ItoM( (u32)&cpuRegs.pc, pc );
		iFlushCall();
		CALLFunc( (u32)MIN_S );
	}
}

/*REC_FPUBRANCH(BC1F);
REC_FPUBRANCH(BC1T);
REC_FPUBRANCH(BC1FL);
REC_FPUBRANCH(BC1TL);*/

////////////////////////////////////////////////////
void recBC1F( void ) {
	u32 branchTo = (s32)_Imm_ * 4 + pc;
	
	//COUNT_CYCLES(pc+4);
	MOV32MtoR(EAX, (u32)&fpuRegs.fprc[31]);
	TEST32ItoR(EAX, 0x00800000);
	j32Ptr[0] = JNZ32(0);

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
	//j32Ptr[1] = JMP32(0);

	x86SetJ32(j32Ptr[0]);

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
	//x86SetJ32(j32Ptr[1]);
}

////////////////////////////////////////////////////
void recBC1T( void ) {
	u32 branchTo = (s32)_Imm_ * 4 + pc;

	//COUNT_CYCLES(pc+4);

	MOV32MtoR(EAX, (u32)&fpuRegs.fprc[31]);
	TEST32ItoR(EAX, 0x00800000);
	j32Ptr[0] = JZ32(0);

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
	//j32Ptr[1] = JMP32(0);

	x86SetJ32(j32Ptr[0]);

	// recopy the next inst
	pc -= 4;
	recompileNextInstruction(1);
	COUNT_CYCLES(pc);

	SetBranchImm(pc);
	//x86SetJ32(j32Ptr[1]);	
}

////////////////////////////////////////////////////
void recBC1FL( void ) {
	u32 branchTo = _Imm_ * 4 + pc;

	//COUNT_CYCLES(pc+4); // off by 1 cycle

	MOV32MtoR(EAX, (u32)&fpuRegs.fprc[31]);
	TEST32ItoR(EAX, 0x00800000);
	j32Ptr[0] = JNZ32(0);

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
	//j32Ptr[1] = JMP32(0);

	x86SetJ32(j32Ptr[0]);

	// recopy the next inst
	//pc -= 4;
	//recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
	//iRet(TRUE);
	//x86SetJ32(j32Ptr[1]);
}

////////////////////////////////////////////////////
void recBC1TL( void ) {
	u32 branchTo = _Imm_ * 4 + pc;

	//COUNT_CYCLES(pc+4); // off by 1 cycle

	MOV32MtoR(EAX, (u32)&fpuRegs.fprc[31]);
	TEST32ItoR(EAX, 0x00800000);
	j32Ptr[0] = JZ32(0);

	recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(branchTo);
	//j32Ptr[1] = JMP32(0);

	x86SetJ32(j32Ptr[0]);

	// recopy the next inst
	//pc -= 4;
	//recompileNextInstruction(1);
	COUNT_CYCLES(pc);
	SetBranchImm(pc);
	//iRet(TRUE);
	//x86SetJ32(j32Ptr[1]);
}

#endif
