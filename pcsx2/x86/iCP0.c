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
//REC_SYS(MTC0);
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
extern u32 s_iLastCOP0Cycle;

void recMFC0( void )
{
	int mmreg;

	if ( ! _Rt_ ) return;

	if( _Rd_ == 9 ) {
		MOV32MtoR( EAX, (u32)&cpuRegs.CP0.r[ _Rd_ ] );
		MOV32MtoR(ECX, (u32)&cpuRegs.cycle);
		ADD32RtoR(EAX, ECX);
		SUB32MtoR(EAX, (u32)&s_iLastCOP0Cycle);
		MOV32RtoM((u32)&s_iLastCOP0Cycle, ECX);
		MOV32RtoM((u32)&cpuRegs.CP0.r[ _Rd_ ], EAX);
		
		_deleteEEreg(_Rt_, 0);
		MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);

		if(EEINST_ISLIVE1(_Rt_)) {
			CDQ();
			MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
		}
		else EEINST_RESETHASLIVE1(_Rt_);
		return;
	}
	if( _Rd_ == 25 ) {		
		
		_deleteEEreg(_Rt_, 0);
		switch(_Imm_ & 0x3F){
				case 0: MOV32MtoR(EAX, (u32)&cpuRegs.PERF.n.pccr); break;
				case 1: MOV32MtoR(EAX, (u32)&cpuRegs.PERF.n.pcr0); break;
				case 3: MOV32MtoR(EAX, (u32)&cpuRegs.PERF.n.pcr1); break;
			}
			
		MOV32RtoM( (u32)&cpuRegs.CP0.r[ _Rt_ ], EAX );
		
		SysPrintf("MFC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
		return;
	}

	_eeOnWriteReg(_Rt_, 1);

	if( EEINST_ISLIVE1(_Rt_) ) {
		_deleteEEreg(_Rt_, 0);
		MOV32MtoR(EAX, (u32)&cpuRegs.CP0.r[ _Rd_ ]);
		CDQ();
		MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0], EAX);
		MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
	}
	else {
		EEINST_RESETHASLIVE1(_Rt_);

		if( (mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE)) >= 0 ) {
			MOVDMtoMMX(mmreg, (u32)&cpuRegs.CP0.r[ _Rd_ ]);
			SetMMXstate();
		}
		else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_READ)) >= 0) {

			if( EEINST_ISLIVE2(_Rt_) ) {
				if( xmmregs[mmreg].mode & MODE_WRITE ) {
					SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[_Rt_].UL[2], mmreg);
				}
				xmmregs[mmreg].inuse = 0;

				MOV32MtoR(EAX, (u32)&cpuRegs.CP0.r[ _Rd_ ]);
				MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);
			}
			else {
				SSE_MOVLPS_M64_to_XMM(mmreg, (u32)&cpuRegs.CP0.r[ _Rd_ ]);
			}
		}
		else {
			MOV32MtoR(EAX, (u32)&cpuRegs.CP0.r[ _Rd_ ]);
			if(_Rd_ == 12) AND32ItoR(EAX, 0xf0c79c1f);
			MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0],EAX);
			if(EEINST_ISLIVE1(_Rt_)) {
				CDQ();
				MOV32RtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[1], EDX);
			}
			else {
				EEINST_RESETHASLIVE1(_Rt_);
			}
		}
	}
}

void recMTC0()
{
	if( GPR_IS_CONST1(_Rt_) ) {
		switch (_Rd_) {
			case 12: 
				iFlushCall(FLUSH_NODESTROY);
				//_flushCachedRegs(); //NOTE: necessary?
				PUSH32I(g_cpuConstRegs[_Rt_].UL[0]);
				CALLFunc((u32)WriteCP0Status);
				ADD32ItoR(ESP, 4);
				break;
			case 9:
				MOV32MtoR(ECX, (u32)&cpuRegs.cycle);
				MOV32RtoM((u32)&s_iLastCOP0Cycle, ECX);
				MOV32ItoM((u32)&cpuRegs.CP0.r[9], g_cpuConstRegs[_Rt_].UL[0]);
				break;
			case 25:
				SysPrintf("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
				switch(_Imm_ & 0x3F){
					case 0: MOV32ItoM((u32)&cpuRegs.PERF.n.pccr, g_cpuConstRegs[_Rt_].UL[0]); break;
					case 1: MOV32ItoM((u32)&cpuRegs.PERF.n.pcr0, g_cpuConstRegs[_Rt_].UL[0]); break;
					case 3: MOV32ItoM((u32)&cpuRegs.PERF.n.pcr1, g_cpuConstRegs[_Rt_].UL[0]); break;
				}
				break;
			default:
				MOV32ItoM((u32)&cpuRegs.CP0.r[_Rd_], g_cpuConstRegs[_Rt_].UL[0]);
				break;
		}
	}
	else {
		switch (_Rd_) {
			case 12: 
				iFlushCall(FLUSH_NODESTROY);
				//_flushCachedRegs(); //NOTE: necessary?
				SUB32ItoR(ESP, 4);
				_eeMoveGPRtoRm(ESP, _Rt_);
				CALLFunc((u32)WriteCP0Status);
				ADD32ItoR(ESP, 4);
				break;
			case 9:
				MOV32MtoR(ECX, (u32)&cpuRegs.cycle);
				_eeMoveGPRtoM((u32)&cpuRegs.CP0.r[9], _Rt_);
				MOV32RtoM((u32)&s_iLastCOP0Cycle, ECX);
				break;
			case 25:
				SysPrintf("MTC0 PCCR = %x PCR0 = %x PCR1 = %x IMM= %x\n", 
				cpuRegs.PERF.n.pccr, cpuRegs.PERF.n.pcr0, cpuRegs.PERF.n.pcr1, _Imm_ & 0x3F);
				switch(_Imm_ & 0x3F){
					case 0: _eeMoveGPRtoM((u32)&cpuRegs.PERF.n.pccr, _Rt_); break;
					case 1: _eeMoveGPRtoM((u32)&cpuRegs.PERF.n.pcr0, _Rt_); break;
					case 3: _eeMoveGPRtoM((u32)&cpuRegs.PERF.n.pcr1, _Rt_); break;
				}
				break;
			default:
				_eeMoveGPRtoM((u32)&cpuRegs.CP0.r[_Rd_], _Rt_);
				break;
		}
	}
}

/*void rec(COP0) {
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
