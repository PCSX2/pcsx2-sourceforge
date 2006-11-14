#if defined(__WIN32__)
#include <windows.h>
#endif

#include <assert.h>

#include <vector>
#include <list>

using namespace std;

#include "PS2Etypes.h"
#include "zlib.h"
#include "Elfheader.h"
#include "Misc.h"
#include "System.h"
#include "R5900.h"
#include "Vif.h"
#include "VU.h"
#include "VifDma.h"
#include "Memory.h"
#include "Hw.h"

#include "ix86/ix86.h"
#include "iR5900.h"

#include "Counters.h"
#include "GS.h"

void gsConstWrite8(u32 mem, int mmreg)
{
	switch (mem&~3) {
		case 0x12001000: // GS_CSR
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			MOV32MtoR(ECX, (u32)&CSRw);
			AND32ItoR(EAX, 0xff<<(mem&3)*8);
			AND32ItoR(ECX, ~(0xff<<(mem&3)*8));
			OR32ItoR(EAX, ECX);
			PUSH32R(EAX);
			CALLFunc((u32)CSRwrite);
			ADD32ItoR(ESP, 4);
			break;
		default:
			_eeWriteConstMem8( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) {
				_recPushReg(mmreg);

				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE8);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}
			break;
	}
}

void recSetSMODE1()
{
	iFlushCall(0);
	AND32ItoR(EAX, 0x6000);
	CMP32ItoR(EAX, 0x6000);
	j8Ptr[5] = JNE8(0);

	// PAL
	OR32ItoM( (u32)&Config.PsxType, 1);
	j8Ptr[6] = JMP8(0);

	x86SetJ8( j8Ptr[5] );

	// NTSC
	AND32ItoM( (u32)&Config.PsxType, ~1 );

	x86SetJ8( j8Ptr[6] );
	CALLFunc((u32)UpdateVSyncRate);
}

void recSetSMODE2()
{
	TEST32ItoR(EAX, 1);
	j8Ptr[5] = JZ8(0);

	// Interlaced
	OR32ItoM( (u32)&Config.PsxType, 2);
	j8Ptr[6] = JMP8(0);

	x86SetJ8( j8Ptr[5] );

	// Non-Interlaced
	AND32ItoM( (u32)&Config.PsxType, ~2 );

	x86SetJ8( j8Ptr[6] );
}

void gsConstWrite16(u32 mem, int mmreg)
{	
	switch (mem&~3) {
		case 0x12000010: // GS_SMODE1
			assert( !(mem&3));
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem16( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE1();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE16);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;
			
		case 0x12000020: // GS_SMODE2
			assert( !(mem&3));
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem16( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE2();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE16);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;
			
		case 0x12001000: // GS_CSR

			assert( !(mem&2) );
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);

			MOV32MtoR(ECX, (u32)&CSRw);
			AND32ItoR(EAX, 0xffff<<(mem&2)*8);
			AND32ItoR(ECX, ~(0xffff<<(mem&2)*8));
			OR32ItoR(EAX, ECX);
			PUSH32R(EAX);
			CALLFunc((u32)CSRwrite);
			ADD32ItoR(ESP, 4);
			break;

		default:
			_eeWriteConstMem16( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) {
				_recPushReg(mmreg);

				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE16);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;
	}
}

// (value&0x1f00)|0x6000
void gsConstWriteIMR(int mmreg)
{
	const u32 mem = 0x12001010;
	if( mmreg & MEM_XMMTAG ) {
		SSE2_MOVD_XMM_to_M32((u32)PS2GS_BASE(mem), mmreg&0xf);
		AND32ItoM((u32)PS2GS_BASE(mem), 0x1f00);
		OR32ItoM((u32)PS2GS_BASE(mem), 0x6000);
	}
	else if( mmreg & MEM_MMXTAG ) {
		SetMMXstate();
		MOVDMMXtoM((u32)PS2GS_BASE(mem), mmreg&0xf);
		AND32ItoM((u32)PS2GS_BASE(mem), 0x1f00);
		OR32ItoM((u32)PS2GS_BASE(mem), 0x6000);
	}
	else if( mmreg & MEM_EECONSTTAG ) {
		MOV32ItoM( (u32)PS2GS_BASE(mem), (g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]&0x1f00)|0x6000);
	}
	else {
		AND32ItoR(mmreg, 0x1f00);
		OR32ItoR(mmreg, 0x6000);
		MOV32RtoM( (u32)PS2GS_BASE(mem), mmreg );
	}

	// IMR doesn't need to be updated in MTGS mode
}

void gsConstWrite32(u32 mem, int mmreg) {

	switch (mem) {

		case 0x12000010: // GS_SMODE1
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE1();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;

		case 0x12000020: // GS_SMODE2
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE2();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;
			
		case 0x12001000: // GS_CSR
			_recPushReg(mmreg);
			iFlushCall(0);
			CALLFunc((u32)CSRwrite);
			ADD32ItoR(ESP, 4);
			break;

		case 0x12001010: // GS_IMR
			gsConstWriteIMR(mmreg);
			break;
		default:
			_eeWriteConstMem32( (u32)PS2GS_BASE(mem), mmreg );

			if( CHECK_MULTIGS ) {
				_recPushReg(mmreg);

				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;
	}
}

void gsConstWrite64(u32 mem, int mmreg)
{
	switch (mem) {
		case 0x12000010: // GS_SMODE1
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem64((u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE1();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;

		case 0x12000020: // GS_SMODE2
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem64((u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE2();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;

		case 0x12001000: // GS_CSR
			_recPushReg(mmreg);
			iFlushCall(0);
			CALLFunc((u32)CSRwrite);
			ADD32ItoR(ESP, 4);
			break;

		case 0x12001010: // GS_IMR
			gsConstWriteIMR(mmreg);
			break;

		default:
			_eeWriteConstMem64((u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32M((u32)PS2GS_BASE(mem)+4);
				PUSH32M((u32)PS2GS_BASE(mem));
				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE64);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 16);
			}

			break;
	}
}

void gsConstWrite128(u32 mem, int mmreg)
{
	switch (mem) {
		case 0x12000010: // GS_SMODE1
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem128( (u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE1();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;

		case 0x12000020: // GS_SMODE2
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem128( (u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) PUSH32R(EAX);

			recSetSMODE2();

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE32);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 12);
			}

			break;

		case 0x12001000: // GS_CSR
			_recPushReg(mmreg);
			iFlushCall(0);
			CALLFunc((u32)CSRwrite);
			ADD32ItoR(ESP, 4);
			break;

		case 0x12001010: // GS_IMR
			// (value&0x1f00)|0x6000
			gsConstWriteIMR(mmreg);
			break;

		default:
			_eeWriteConstMem128( (u32)PS2GS_BASE(mem), mmreg);

			if( CHECK_MULTIGS ) {
				iFlushCall(0);

				PUSH32M((u32)PS2GS_BASE(mem)+4);
				PUSH32M((u32)PS2GS_BASE(mem));
				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE64);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 16);

				PUSH32M((u32)PS2GS_BASE(mem)+12);
				PUSH32M((u32)PS2GS_BASE(mem)+8);
				PUSH32I(mem&0x13ff);
				PUSH32I(GS_RINGTYPE_MEMWRITE64);
				CALLFunc((u32)GSRingBufSimplePacket);
				ADD32ItoR(ESP, 16);
			}

			break;
	}
}

int gsConstRead8(u32 x86reg, u32 mem, u32 sign)
{
#ifdef GIF_LOG
	GIF_LOG("GS read 8 %8.8lx (%8.8x), at %8.8lx\n", (u32)PS2GS_BASE(mem), mem);
#endif
	_eeReadConstMem8(x86reg, (u32)PS2GS_BASE(mem), sign);
	return 0;
}

int gsConstRead16(u32 x86reg, u32 mem, u32 sign)
{
#ifdef GIF_LOG
	GIF_LOG("GS read 16 %8.8lx (%8.8x), at %8.8lx\n", (u32)PS2GS_BASE(mem), mem);
#endif
	_eeReadConstMem16(x86reg, (u32)PS2GS_BASE(mem), sign);
	return 0;
}

int gsConstRead32(u32 x86reg, u32 mem)
{
#ifdef GIF_LOG
	GIF_LOG("GS read 32 %8.8lx (%8.8x), at %8.8lx\n", (u32)PS2GS_BASE(mem), mem);
#endif
	_eeReadConstMem32(x86reg, (u32)PS2GS_BASE(mem));
	return 0;
}

void gsConstRead64(u32 mem, int mmreg)
{
#ifdef GIF_LOG
	GIF_LOG("GS read 64 %8.8lx (%8.8x), at %8.8lx\n", (u32)PS2GS_BASE(mem), mem);
#endif
	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, (u32)PS2GS_BASE(mem));
	else {
		MOVQMtoR(mmreg, (u32)PS2GS_BASE(mem));
		SetMMXstate();
	}
}

void gsConstRead128(u32 mem, int xmmreg)
{
#ifdef GIF_LOG
	GIF_LOG("GS read 128 %8.8lx (%8.8x), at %8.8lx\n", (u32)PS2GS_BASE(mem), mem);
#endif
	_eeReadConstMem128( xmmreg, (u32)PS2GS_BASE(mem));
}

