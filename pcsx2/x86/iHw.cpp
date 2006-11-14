#include <string.h>
#include <malloc.h>

#include "Common.h"
#include "iR5900.h"
#include "VUmicro.h"
#include "PsxMem.h"
#include "IPU/IPU.h"
#include "GS.h"

#include <assert.h>

#ifndef WIN32_VIRTUAL_MEM
extern u8  *psH; // hw mem
extern u16 *psHW;
extern u32 *psHL;
extern u64 *psHD;
#endif

extern int b440;
extern BOOL bExecBIOS;
extern int b440table[];

int hwConstRead8(u32 x86reg, u32 mem, u32 sign)
{
#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10000000 && mem < 0x10008000 )
		SysPrintf("hwRead8 to %x\n", mem);
#endif

	if ((mem & 0xffffff0f) == 0x1000f200) {
		if(mem == 0x1000f260) {
			if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
			else XOR32RtoR(x86reg, x86reg);
			return 0;
		}
		else if(mem == 0x1000F240) {

			_eeReadConstMem8(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff], sign);
			//psHu32(mem) &= ~0x4000;
			return 0;
		}
	}
	
	if (mem < 0x10010000)
	{
		_eeReadConstMem8(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff], sign);
	}
	else {
		if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
		else XOR32RtoR(x86reg, x86reg);
	}

	return 0;
}

#define CONSTREAD16_CALL(name) { \
	iFlushCall(0); \
	CALLFunc((u32)name); \
	if( sign ) MOVSX32R16toR(EAX, EAX); \
	else MOVZX32R16toR(EAX, EAX); \
} \

static u32 s_regreads[3] = {0x010200000, 0xbfff0000, 0xF0000102};
int hwConstRead16(u32 x86reg, u32 mem, u32 sign)
{
#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10002000 && mem < 0x10008000 )
		SysPrintf("hwRead16 to %x\n", mem);
#endif
#ifdef PCSX2_DEVBUILD
	if( mem >= 0x10000000 && mem < 0x10002000 ){
#ifdef EECNT_LOG
	EECNT_LOG("cnt read to %x\n", mem);
#endif
	}
#endif

	switch (mem) {
		case 0x10000000:
			PUSH32I(0);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000010:
			_eeReadConstMem16(x86reg, (u32)&counters[0].mode, sign);
			return 0;
		case 0x10000020:
			_eeReadConstMem16(x86reg, (u32)&counters[0].mode, sign);
			return 0;
		case 0x10000030:
			_eeReadConstMem16(x86reg, (u32)&counters[0].hold, sign);
			return 0;

		case 0x10000800:
			PUSH32I(1);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10000810:
			_eeReadConstMem16(x86reg, (u32)&counters[1].mode, sign);
			return 0;

		case 0x10000820:
			_eeReadConstMem16(x86reg, (u32)&counters[1].target, sign);
			return 0;

		case 0x10000830:
			_eeReadConstMem16(x86reg, (u32)&counters[1].hold, sign);
			return 0;

		case 0x10001000:
			PUSH32I(2);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10001010:
			_eeReadConstMem16(x86reg, (u32)&counters[2].mode, sign);
			return 0;

		case 0x10001020:
			_eeReadConstMem16(x86reg, (u32)&counters[2].target, sign);
			return 0;

		case 0x10001800:
			PUSH32I(3);
			CONSTREAD16_CALL(rcntRcount);
			ADD32ItoR(ESP, 4);
			return 1;

		case 0x10001810:
			_eeReadConstMem16(x86reg, (u32)&counters[3].mode, sign);
			return 0;

		case 0x10001820:
			_eeReadConstMem16(x86reg, (u32)&counters[3].target, sign);
			return 0;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) {
					if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
					else XOR32RtoR(x86reg, x86reg);
					return 0;
				}
				else if(mem == 0x1000F240) {

					if( IS_MMXREG(x86reg) ) {
						MOVDMtoMMX(x86reg&0xf, (u32)&PS2MEM_HW[(mem) & 0xffff] - 2);
						PORMtoR(x86reg&0xf, (u32)&s_regreads[0]);
						PANDMtoR(x86reg&0xf, (u32)&s_regreads[1]);
					}
					else {
						if( sign ) MOVSX32M16toR(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff]);
						else MOVZX32M16toR(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff]);

						OR32ItoR(x86reg, 0x0102);
						AND32ItoR(x86reg, ~0x4000);
					}
					return 0;
				}
			}
			if (mem < 0x10010000) {
				_eeReadConstMem16(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff], sign);
			}
			else {
				if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
				else XOR32RtoR(x86reg, x86reg);
			}
			
			return 0;
	}
}

#ifdef WIN32_VIRTUAL_MEM
__declspec(naked) void recCheckF440()
{
	__asm {
		add b440, 1
		mov eax, b440
		sub eax, 3
		mov edx, 31

		cmp eax, 27
		ja WriteVal
		shl eax, 2
		mov edx, dword ptr [eax+b440table]

WriteVal:
		mov eax, PS2MEM_BASE_+0x1000f440
		mov dword ptr [eax], edx
		ret
	}
}

void iMemRead32Check()
{
	// test if 0xf440
	if( bExecBIOS ) {
		u8* ptempptr[2];
		CMP32ItoR(ECX, 0x1000f440);
		ptempptr[0] = JNE8(0);

//		// increment and test
//		INC32M((int)&b440);
//		MOV32MtoR(EAX, (int)&b440);
//		SUB32ItoR(EAX, 3);
//		MOV32ItoR(EDX, 31);
//
//		CMP32ItoR(EAX, 27);
//
//		// look up table
//		ptempptr[1] = JA8(0);
//		SHL32ItoR(EAX, 2);
//		ADD32ItoR(EAX, (int)b440table);
//		MOV32RmtoR(EDX, EAX);
//
//		x86SetJ8( ptempptr[1] );
//
//		MOV32RtoM( (int)PS2MEM_HW+0xf440, EDX);
		CALLFunc((u32)recCheckF440);

		x86SetJ8( ptempptr[0] );
	}
}

#endif

int hwConstRead32(u32 x86reg, u32 mem)
{
	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
		return ipuConstRead32(x86reg, mem);
	}

	switch (mem) {
		case 0x10000000:
			iFlushCall(0);
			PUSH32I(0);
			CALLFunc((u32)rcntRcount);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 0 count read = %x\n", rcntRcount(0));
#endif
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000010:
			_eeReadConstMem32(x86reg, (u32)&counters[0].mode);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 0 mode read = %x\n", counters[0].mode);
#endif
			return 0;
		case 0x10000020:
			_eeReadConstMem32(x86reg, (u32)&counters[0].target);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 0 target read = %x\n", counters[0].target);
#endif
			return 0;
		case 0x10000030:
			_eeReadConstMem32(x86reg, (u32)&counters[0].hold);
			return 0;

		case 0x10000800:
			iFlushCall(0);
			PUSH32I(1);
			CALLFunc((u32)rcntRcount);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 1 count read = %x\n", rcntRcount(1));
#endif
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10000810:
			_eeReadConstMem32(x86reg, (u32)&counters[1].mode);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 1 mode read = %x\n", counters[1].mode);
#endif
			return 0;
		case 0x10000820:
			_eeReadConstMem32(x86reg, (u32)&counters[1].target);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 1 target read = %x\n", counters[1].target);
#endif
			return 0;
		case 0x10000830:
			_eeReadConstMem32(x86reg, (u32)&counters[1].hold);
			return 0;

		case 0x10001000:
			iFlushCall(0);
			PUSH32I(2);
			CALLFunc((u32)rcntRcount);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 2 count read = %x\n", rcntRcount(2));
#endif
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10001010:
			_eeReadConstMem32(x86reg, (u32)&counters[2].mode);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 2 mode read = %x\n", counters[2].mode);
#endif
			return 0;
		case 0x10001020:
			_eeReadConstMem32(x86reg, (u32)&counters[2].target);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 2 target read = %x\n", counters[2].target);
#endif
			return 0;
		case 0x10001030:
			_eeReadConstMem32(x86reg, (u32)&counters[2].hold);
			return 0;

		case 0x10001800:
			iFlushCall(0);
			PUSH32I(3);
			CALLFunc((u32)rcntRcount);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 3 count read = %x\n", rcntRcount(3));
#endif
			ADD32ItoR(ESP, 4);
			return 1;
		case 0x10001810:
			_eeReadConstMem32(x86reg, (u32)&counters[3].mode);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 3 mode read = %x\n", counters[3].mode);
#endif
			return 0;
		case 0x10001820:
			_eeReadConstMem32(x86reg, (u32)&counters[3].target);
#ifdef EECNT_LOG
			EECNT_LOG("Counter 3 target read = %x\n", counters[3].target);
#endif
			return 0;
		case 0x10001830:
			_eeReadConstMem32(x86reg, (u32)&counters[3].hold);
			return 0;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
			else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
			else XOR32RtoR(x86reg, x86reg);
			return 0;

		case 0x1000f440:
			//iMemRead32Check();
			// increment and test
			INC32M((int)&b440);
			MOV32MtoR(EAX, (int)&b440);
			SUB32ItoR(EAX, 3);
			MOV32ItoR(EDX, 31);

			CMP32ItoR(EAX, 27);

			// look up table
			j8Ptr[8] = JA8(0);
			SHL32ItoR(EAX, 2);
			ADD32ItoR(EAX, (int)b440table);
			MOV32RmtoR(EDX, EAX);

			x86SetJ8( j8Ptr[8] );

			MOV32RtoM( (int)PS2MEM_HW+0xf440, EDX);
			
			if( IS_XMMREG(x86reg) ) SSE2_MOVD_R_to_XMM(x86reg&0xf, EDX);
			else if( IS_MMXREG(x86reg) ) MOVD32RtoMMX(x86reg&0xf, EDX);
			else MOV32RtoR(x86reg, EDX);

			return 0;

		case 0x1000f520: // DMAC_ENABLER
			_eeReadConstMem32(x86reg, (u32)&PS2MEM_HW[0xf590]);
			return 0;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				if(mem == 0x1000f260) {
					if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
					else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
					else XOR32RtoR(x86reg, x86reg);
					return 0;
				}
				else if(mem == 0x1000F240) {

					if( IS_XMMREG(x86reg) ) {
						SSEX_MOVD_M32_to_XMM(x86reg&0xf, (u32)&PS2MEM_HW[(mem) & 0xffff]);
						SSEX_POR_M128_to_XMM(x86reg&0xf, (u32)&s_regreads[2]);
					}
					else if( IS_MMXREG(x86reg) ) {
						MOVDMtoMMX(x86reg&0xf, (u32)&PS2MEM_HW[(mem) & 0xffff]);
						PORMtoR(x86reg&0xf, (u32)&s_regreads[2]);
					}
					else {
						MOV32MtoR(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff]);
						OR32ItoR(x86reg, 0xF0000102);
					}
					return 0;
				}
			}
			
			if (mem < 0x10010000) {
				_eeReadConstMem32(x86reg, (u32)&PS2MEM_HW[(mem) & 0xffff]);
			}
			else {
				if( IS_XMMREG(x86reg) ) SSEX_PXOR_XMM_to_XMM(x86reg&0xf, x86reg&0xf);
				else if( IS_MMXREG(x86reg) ) PXORRtoR(x86reg&0xf, x86reg&0xf);
				else XOR32RtoR(x86reg, x86reg);
			}

			return 0;
	}
}

void hwConstRead64(u32 mem, int mmreg) {
	if ((mem>=0x10002000) && (mem<0x10003000)) {
		ipuConstRead64(mem, mmreg);
		return;
	}

	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, (u32)PSM(mem));
	else {
		MOVQMtoR(mmreg, (u32)PSM(mem));
		SetMMXstate();
	}
}

PCSX2_ALIGNED16(u32 s_TempFIFO[4]);
void hwConstRead128(u32 mem, int xmmreg) {
	if (mem >= 0x10004000 && mem < 0x10008000) {
		iFlushCall(0);
		PUSH32I((u32)&s_TempFIFO[0]);
		PUSH32I(mem);
		CALLFunc((u32)ReadFIFO);
		ADD32ItoR(ESP, 8);
		_eeReadConstMem128( xmmreg, (u32)&s_TempFIFO[0]);
		return;
	}

	_eeReadConstMem128( xmmreg, (u32)PSM(mem));
}

// when writing imm
#define recDmaExecI8(name, num) { \
	MOV8ItoM((u32)&PS2MEM_HW[(mem) & 0xffff], g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]); \
	if( g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 1 ) { \
		TEST8ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		CALLFunc((u32)dma##name); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

#define recDmaExec8(name, num) { \
	iFlushCall(0); \
	if( IS_CONSTREG(mmreg) ) { \
		recDmaExecI8(name, num); \
	} \
	else { \
		_eeMoveMMREGtoR(EAX, mmreg); \
		_eeWriteConstMem8((u32)&PS2MEM_HW[(mem) & 0xffff], mmreg); \
		\
		TEST8ItoR(EAX, 1); \
		j8Ptr[5] = JZ8(0); \
		TEST8ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		\
		CALLFunc((u32)dma##name); \
		\
		x86SetJ8( j8Ptr[5] ); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

static void PrintDebug(u8 value)
{
	if (value == '\n') {
		sio_buffer[sio_count] = 0;
		SysPrintf(COLOR_GREEN "%s\n" COLOR_RESET, sio_buffer);
		sio_count = 0;
	} else {
		if (sio_count < 1023) {
			sio_buffer[sio_count++] = value;
		}
	}
}

#define CONSTWRITE_CALLTIMER(name, index, bit) { \
	if( !IS_CONSTREG(mmreg) ) { \
		if( bit == 8 ) MOVZX32R8toR(mmreg&0xf, mmreg&0xf); \
		else if( bit == 16 ) MOVZX32R16toR(mmreg&0xf, mmreg&0xf); \
	} \
	_recPushReg(mmreg); \
	iFlushCall(0); \
	PUSH32I(index); \
	CALLFunc((u32)name); \
	ADD32ItoR(ESP, 8); \
} \

#define CONSTWRITE_TIMERS(bit) \
	case 0x10000000: CONSTWRITE_CALLTIMER(rcntWcount, 0, bit); break; \
	case 0x10000010: CONSTWRITE_CALLTIMER(rcntWmode, 0, bit); break; \
	case 0x10000020: CONSTWRITE_CALLTIMER(rcntWtarget, 0, bit); break; \
	case 0x10000030: CONSTWRITE_CALLTIMER(rcntWhold, 0, bit); break; \
	\
	case 0x10000800: CONSTWRITE_CALLTIMER(rcntWcount, 1, bit); break; \
	case 0x10000810: CONSTWRITE_CALLTIMER(rcntWmode, 1, bit); break; \
	case 0x10000820: CONSTWRITE_CALLTIMER(rcntWtarget, 1, bit); break; \
	case 0x10000830: CONSTWRITE_CALLTIMER(rcntWhold, 1, bit); break; \
	\
	case 0x10001000: CONSTWRITE_CALLTIMER(rcntWcount, 2, bit); break; \
	case 0x10001010: CONSTWRITE_CALLTIMER(rcntWmode, 2, bit); break; \
	case 0x10001020: CONSTWRITE_CALLTIMER(rcntWtarget, 2, bit); break; \
	\
	case 0x10001800: CONSTWRITE_CALLTIMER(rcntWcount, 3, bit); break; \
	case 0x10001810: CONSTWRITE_CALLTIMER(rcntWmode, 3, bit); break; \
	case 0x10001820: CONSTWRITE_CALLTIMER(rcntWtarget, 3, bit); break; \

void hwConstWrite8(u32 mem, int mmreg)
{
	switch (mem) {
		CONSTWRITE_TIMERS(8)

		case 0x1000f180:
			_recPushReg(mmreg); \
			iFlushCall(0);
			CALLFunc((u32)PrintDebug);
			ADD32ItoR(ESP, 4);
			break;

		case 0x10008001: // dma0 - vif0
			recDmaExec8(VIF0, 0);
			break;

		case 0x10009001: // dma1 - vif1
			recDmaExec8(VIF1, 1);
			break;

		case 0x1000a001: // dma2 - gif
			recDmaExec8(GIF, 2);
			break;

		case 0x1000b001: // dma3 - fromIPU
			recDmaExec8(IPU0, 3);
			break;

		case 0x1000b401: // dma4 - toIPU
			recDmaExec8(IPU1, 4);
			break;

		case 0x1000c001: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec8(SIF0, 5);
			break;

		case 0x1000c401: // dma6 - sif1
			recDmaExec8(SIF1, 6);
			break;

		case 0x1000c801: // dma7 - sif2
			recDmaExec8(SIF2, 7);
			break;

		case 0x1000d001: // dma8 - fromSPR
			recDmaExec8(SPR0, 8);
			break;

		case 0x1000d401: // dma9 - toSPR
			recDmaExec8(SPR1, 9);
			break;

		case 0x1000f592: // DMAC_ENABLEW
			_eeWriteConstMem8( (u32)&PS2MEM_HW[0xf522], mmreg );
			_eeWriteConstMem8( (u32)&PS2MEM_HW[0xf592], mmreg );			
			break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem8( (u32)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x40:
					if( IS_CONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND32ItoM( (u32)&PS2MEM_HW[mem&0xfffc], ~0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST16ItoR(EAX, 0x100);
						j8Ptr[5] = JNZ8(0);
						AND32ItoM( (u32)&PS2MEM_HW[mem&0xfffc], ~0x100);
						x86SetJ8(j8Ptr[5]);
					}
					break;
				}
				return;
			}
			assert( (mem&0xff0f) != 0xf200 );

			switch(mem&~3) {
				case 0x1000f130:
				case 0x1000f410:
				case 0x1000f430:
					break;
				default:
#ifdef WIN32_VIRTUAL_MEM
					//NOTE: this might cause crashes, but is more correct
					_eeWriteConstMem8((u32)PS2MEM_BASE + mem, mmreg);
#else
					if (mem < 0x10010000)
					{
						_eeWriteConstMem8((u32)&PS2MEM_HW[mem&0xffff], mmreg);
					}
#endif
			}

			break;
	}
}

#define recDmaExecI16(name, num) { \
	MOV16ItoM((u32)&PS2MEM_HW[(mem) & 0xffff], g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]); \
	if( g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100 ) { \
		TEST8ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		CALLFunc((u32)dma##name); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

#define recDmaExec16(name, num) { \
	iFlushCall(0); \
	if( IS_CONSTREG(mmreg) ) { \
		recDmaExecI16(name, num); \
	} \
	else { \
		_eeMoveMMREGtoR(EAX, mmreg); \
		_eeWriteConstMem16((u32)&PS2MEM_HW[(mem) & 0xffff], mmreg); \
		\
		TEST8ItoR(EAX, 0x100); \
		j8Ptr[5] = JZ8(0); \
		TEST8ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		\
		CALLFunc((u32)dma##name); \
		\
		x86SetJ8( j8Ptr[5] ); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

void hwConstWrite16(u32 mem, int mmreg)
{
	switch(mem&~3) {
		CONSTWRITE_TIMERS(16)
		case 0x10008000: // dma0 - vif0
			recDmaExec16(VIF0, 0);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			recDmaExec16(VIF1, 1);
			break;

		case 0x1000a000: // dma2 - gif
			recDmaExec16(GIF, 2);
			break;
		case 0x1000b000: // dma3 - fromIPU
			recDmaExec16(IPU0, 3);
			break;
		case 0x1000b400: // dma4 - toIPU
			recDmaExec16(IPU1, 4);
			break;
		case 0x1000c000: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec16(SIF0, 5);
			break;
		case 0x1000c002:
			//?
			break;
		case 0x1000c400: // dma6 - sif1
			recDmaExec16(SIF1, 6);
			break;
		case 0x1000c800: // dma7 - sif2
			recDmaExec16(SIF2, 7);
			break;
		case 0x1000c802:
			//?
			break;
		case 0x1000d000: // dma8 - fromSPR
			recDmaExec16(SPR0, 8);
			break;
		case 0x1000d400: // dma9 - toSPR
			recDmaExec16(SPR1, 9);
			break;
		case 0x1000f592: // DMAC_ENABLEW
			_eeWriteConstMem16((u32)&PS2MEM_HW[0xf522], mmreg);
			_eeWriteConstMem16((u32)&PS2MEM_HW[0xf592], mmreg);
			break;
		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;
		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem16((u32)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x20:
					_eeWriteConstMem16OP((u32)&PS2MEM_HW[mem&0xffff], mmreg, 1);
					break;
				case 0x30:
					if( IS_CONSTREG(mmreg) ) {
						AND16ItoM((u32)&PS2MEM_HW[mem&0xffff], ~g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]);
					}
					else {
						NOT32R(mmreg&0xf);
						AND16RtoM((u32)&PS2MEM_HW[mem&0xffff], mmreg&0xf);
					}
					break;
				case 0x40:
					if( IS_CONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND16ItoM((u32)&PS2MEM_HW[mem&0xffff], ~0x100);
						}
						else {
							OR16ItoM((u32)&PS2MEM_HW[mem&0xffff], 0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST16ItoR(EAX, 0x100);
						j8Ptr[5] = JZ8(0);
						OR16ItoM((u32)&PS2MEM_HW[mem&0xffff], 0x100);
						j8Ptr[6] = JMP8(0);

						x86SetJ8( j8Ptr[5] );
						AND16ItoM((u32)&PS2MEM_HW[mem&0xffff], ~0x100);

						x86SetJ8( j8Ptr[6] );
					}

					break;
				case 0x60:
					_eeWriteConstMem16((u32)&PS2MEM_HW[mem&0xffff], 0);
					break;
				}
				return;
			}

#ifdef WIN32_VIRTUAL_MEM
		//NOTE: this might cause crashes, but is more correct
		_eeWriteConstMem16((u32)PS2MEM_BASE + mem, mmreg);
#else
		if (mem < 0x10010000)
		{
			_eeWriteConstMem16((u32)&PS2MEM_HW[mem&0xffff], mmreg);
		}
#endif
	}
}

// when writing an Imm
#define recDmaExecI(name, num) { \
	u32 c = g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]; \
	if(mem!= 0x1000c000 && mem != 0x1000c800) MOV16ItoM((u32)&PS2MEM_HW[(mem) & 0xffff], c); \
	else MOV32ItoM((u32)&PS2MEM_HW[(mem) & 0xffff], c); \
	if( c & 0x100 ) { \
		TEST8ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		CALLFunc((u32)dma##name); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

#define recDmaExec(name, num) { \
	iFlushCall(0); \
	if( IS_CONSTREG(mmreg) ) { \
		recDmaExecI(name, num); \
	} \
	else { \
		_eeMoveMMREGtoR(EAX, mmreg); \
		if(mem!= 0x1000c000 && mem != 0x1000c800) { \
			if( IS_XMMREG(mmreg) || IS_MMXREG(mmreg) ) { \
				MOV16RtoM((u32)&PS2MEM_HW[(mem) & 0xffff], EAX); \
			} \
			else { \
				_eeWriteConstMem16((u32)&PS2MEM_HW[(mem) & 0xffff], mmreg); \
			} \
		} \
		else _eeWriteConstMem32((u32)&PS2MEM_HW[(mem) & 0xffff], mmreg); \
		\
		TEST16ItoR(EAX, 0x100); \
		j8Ptr[5] = JZ8(0); \
		TEST32ItoM((u32)&PS2MEM_HW[DMAC_CTRL&0xffff], 1); \
		j8Ptr[6] = JZ8(0); \
		\
		CALLFunc((u32)dma##name); \
		\
		x86SetJ8( j8Ptr[5] ); \
		x86SetJ8( j8Ptr[6] ); \
	} \
} \

#define CONSTWRITE_CALLTIMER32(name, index, bit) { \
	_recPushReg(mmreg); \
	iFlushCall(0); \
	PUSH32I(index); \
	CALLFunc((u32)name); \
	ADD32ItoR(ESP, 8); \
} \

void hwConstWrite32(u32 mem, int mmreg)
{
	//IPU regs
	if ((mem>=0x10002000) && (mem<0x10003000)) {
    	//psHu32(mem) = value;
		ipuConstWrite32(mem, mmreg);
		return;
	}

	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((u32)vif0Write32);
		ADD32ItoR(ESP, 8);
		return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((u32)vif1Write32);
		ADD32ItoR(ESP, 8);
		return;
	}

	switch (mem) {
		case 0x10000000: CONSTWRITE_CALLTIMER32(rcntWcount, 0, bit); break;
		case 0x10000010: CONSTWRITE_CALLTIMER32(rcntWmode, 0, bit); break;
		case 0x10000020: CONSTWRITE_CALLTIMER32(rcntWtarget, 0, bit); break;
		case 0x10000030: CONSTWRITE_CALLTIMER32(rcntWhold, 0, bit); break;
		
		case 0x10000800: CONSTWRITE_CALLTIMER32(rcntWcount, 1, bit); break;
		case 0x10000810: CONSTWRITE_CALLTIMER32(rcntWmode, 1, bit); break;
		case 0x10000820: CONSTWRITE_CALLTIMER32(rcntWtarget, 1, bit); break;
		case 0x10000830: CONSTWRITE_CALLTIMER32(rcntWhold, 1, bit); break;
		
		case 0x10001000: CONSTWRITE_CALLTIMER32(rcntWcount, 2, bit); break;
		case 0x10001010: CONSTWRITE_CALLTIMER32(rcntWmode, 2, bit); break;
		case 0x10001020: CONSTWRITE_CALLTIMER32(rcntWtarget, 2, bit); break;
		
		case 0x10001800: CONSTWRITE_CALLTIMER32(rcntWcount, 3, bit); break;
		case 0x10001810: CONSTWRITE_CALLTIMER32(rcntWmode, 3, bit); break;
		case 0x10001820: CONSTWRITE_CALLTIMER32(rcntWtarget, 3, bit); break;

		case GIF_CTRL:

			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			TEST8ItoR(EAX, 1);
			j8Ptr[5] = JZ8(0);

			// reset GS
			CALLFunc((u32)gsGIFReset);
			j8Ptr[6] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND32I8toR(EAX, 8);
			MOV32RtoM((u32)&PS2MEM_HW[mem&0xffff], EAX);

			TEST16ItoR(EAX, 8);
			j8Ptr[5] = JZ8(0);
			OR8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], 8);
			j8Ptr[7] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], ~8);
			x86SetJ8( j8Ptr[6] );
			x86SetJ8( j8Ptr[7] );
			return;

		case GIF_MODE:
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32((u32)&PS2MEM_HW[mem&0xffff], mmreg);
			AND8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], ~5);
			AND8ItoR(EAX, 5);
			OR8RtoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], EAX);
			return;

		case GIF_STAT: // stat is readonly
			return;

		case 0x10008000: // dma0 - vif0
			recDmaExec(VIF0, 0);
			break;

		case 0x10009000: // dma1 - vif1 - chcr
			recDmaExec(VIF1, 1);
			break;

		case 0x1000a000: // dma2 - gif
			recDmaExec(GIF, 2);
			break;

		case 0x1000b000: // dma3 - fromIPU
			recDmaExec(IPU0, 3);
			break;
		case 0x1000b400: // dma4 - toIPU
			recDmaExec(IPU1, 4);
			break;
		case 0x1000c000: // dma5 - sif0
			//if (value == 0) psxSu32(0x30) = 0x40000;
			recDmaExec(SIF0, 5);
			break;

		case 0x1000c400: // dma6 - sif1
			recDmaExec(SIF1, 6);
			break;

		case 0x1000c800: // dma7 - sif2
			recDmaExec(SIF2, 7);
			break;

		case 0x1000d000: // dma8 - fromSPR
			recDmaExec(SPR0, 8);
			break;

		case 0x1000d400: // dma9 - toSPR
			recDmaExec(SPR1, 9);
			break;

		case 0x1000e010: // DMAC_STAT
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			MOV32RtoR(ECX, EAX);
			NOT32R(ECX);
			AND16RtoM((u32)&PS2MEM_HW[0xe010], ECX);

			SHR32ItoR(EAX, 16);
			XOR16RtoM((u32)&PS2MEM_HW[0xe012], EAX);
			
			MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status.val);
			AND32ItoR(EAX, 0x10007);
			CMP32ItoR(EAX, 0x10001);
			j8Ptr[5] = JNE8(0);
			CALLFunc((u32)cpuTestDMACInts);

			x86SetJ8( j8Ptr[5] );
			break;

		case 0x1000f000: // INTC_STAT
			_eeWriteConstMem32OP((u32)&PS2MEM_HW[0xf000], mmreg, 2);
			break;

		case 0x1000f010: // INTC_MASK
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			XOR16RtoM((u32)&PS2MEM_HW[0xf010], EAX);
			
			MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status.val);
			AND32ItoR(EAX, 0x10007);
			CMP32ItoR(EAX, 0x10001);
			j8Ptr[5] = JNE8(0);
			CALLFunc((u32)cpuTestDMACInts);

			x86SetJ8( j8Ptr[5] );
			break;
		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf590], mmreg);
			return;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:
			if ((mem & 0xffffff0f) == 0x1000f200) {
				u32 at = mem & 0xf0;
				switch(at)
				{
				case 0x00:
					_eeWriteConstMem32((u32)&PS2MEM_HW[mem&0xffff], mmreg);
					break;
				case 0x20:
					_eeWriteConstMem32OP((u32)&PS2MEM_HW[mem&0xffff], mmreg, 1);
					break;
				case 0x30:
					_eeWriteConstMem32OP((u32)&PS2MEM_HW[mem&0xffff], mmreg, 2);
					break;
				case 0x40:
					if( IS_CONSTREG(mmreg) ) {
						if( !(g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0] & 0x100) ) {
							AND32ItoM( (u32)&PS2MEM_HW[mem&0xfffc], ~0x100);
						}
						else {
							OR32ItoM((u32)&PS2MEM_HW[mem&0xffff], 0x100);
						}
					}
					else {
						_eeMoveMMREGtoR(EAX, mmreg);
						TEST32ItoR(EAX, 0x100);
						j8Ptr[5] = JZ8(0);
						OR32ItoM((u32)&PS2MEM_HW[mem&0xffff], 0x100);
						j8Ptr[6] = JMP8(0);

						x86SetJ8( j8Ptr[5] );
						AND32ItoM((u32)&PS2MEM_HW[mem&0xffff], ~0x100);

						x86SetJ8( j8Ptr[6] );
					}

					break;
				case 0x60:
					MOV32ItoM((u32)&PS2MEM_HW[mem&0xffff], 0);
					break;
				}
				return;
			}

#ifdef WIN32_VIRTUAL_MEM
			//NOTE: this might cause crashes, but is more correct
			_eeWriteConstMem32((u32)PS2MEM_BASE + mem, mmreg);
#else
			if (mem < 0x10010000)
			{
				_eeWriteConstMem32((u32)&PS2MEM_HW[mem&0xffff], mmreg);
			}
#endif
		break;
	}
}

void hwConstWrite64(u32 mem, int mmreg)
{
	if ((mem>=0x10002000) && (mem<=0x10002030)) {
		ipuConstWrite64(mem, mmreg);
		return;
	}
	
	if ((mem>=0x10003800) && (mem<0x10003c00)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((u32)vif0Write32);
		ADD32ItoR(ESP, 8);
		return;
	}
	if ((mem>=0x10003c00) && (mem<0x10004000)) {
		_recPushReg(mmreg);
		iFlushCall(0);
		PUSH32I(mem);
		CALLFunc((u32)vif1Write32);
		ADD32ItoR(ESP, 8);
		return;
	}

	switch (mem) {
		case GIF_CTRL:
			_eeMoveMMREGtoR(EAX, mmreg);
			
			iFlushCall(0);
			TEST8ItoR(EAX, 1);
			j8Ptr[5] = JZ8(0);

			// reset GS
			CALLFunc((u32)gsGIFReset);
			j8Ptr[6] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND32I8toR(EAX, 8);
			MOV32RtoM((u32)&PS2MEM_HW[mem&0xffff], EAX);

			TEST16ItoR(EAX, 8);
			j8Ptr[5] = JZ8(0);
			OR8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], 8);
			j8Ptr[7] = JMP8(0);

			x86SetJ8( j8Ptr[5] );
			AND8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], ~8);
			x86SetJ8( j8Ptr[6] );
			x86SetJ8( j8Ptr[7] );
			return;

		case GIF_MODE:
			_eeMoveMMREGtoR(EAX, mmreg);
			_eeWriteConstMem32((u32)&PS2MEM_HW[mem&0xffff], mmreg);

			AND8ItoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], ~5);
			AND8ItoR(EAX, 5);
			OR8RtoM((u32)&PS2MEM_HW[GIF_STAT&0xffff], EAX);
			break;

		case GIF_STAT: // stat is readonly
			return;

		case 0x1000a000: // dma2 - gif
			recDmaExec(GIF, 2);
			break;

		case 0x1000e010: // DMAC_STAT
			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			MOV32RtoR(ECX, EAX);
			NOT32R(ECX);
			AND16RtoM((u32)&PS2MEM_HW[0xe010], ECX);

			SHR32ItoR(EAX, 16);
			XOR16RtoM((u32)&PS2MEM_HW[0xe012], EAX);
			
			MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status.val);
			AND32ItoR(EAX, 0x10007);
			CMP32ItoR(EAX, 0x10001);
			j8Ptr[5] = JNE8(0);
			CALLFunc((u32)cpuTestDMACInts);

			x86SetJ8( j8Ptr[5] );
			break;

		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf590], mmreg);
			break;

		case 0x1000f000: // INTC_STAT
			_eeWriteConstMem32OP((u32)&PS2MEM_HW[mem&0xffff], mmreg, 2);
			break;

		case 0x1000f010: // INTC_MASK

			_eeMoveMMREGtoR(EAX, mmreg);

			iFlushCall(0);
			XOR16RtoM((u32)&PS2MEM_HW[0xf010], EAX);
			
			MOV32MtoR(EAX, (u32)&cpuRegs.CP0.n.Status.val);
			AND32ItoR(EAX, 0x10007);
			CMP32ItoR(EAX, 0x10001);
			j8Ptr[5] = JNE8(0);
			CALLFunc((u32)cpuTestDMACInts);

			x86SetJ8( j8Ptr[5] );
			
			break;

		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;
		default:

			_eeWriteConstMem64((u32)PSM(mem), mmreg);
			break;
	}
}

void hwConstWrite128(u32 mem, int mmreg)
{
	if (mem >= 0x10004000 && mem < 0x10008000) {
		_eeWriteConstMem128((u32)&s_TempFIFO[0], mmreg);
		iFlushCall(0);
		PUSH32I((u32)&s_TempFIFO[0]);
		PUSH32I(mem);
		CALLFunc((u32)WriteFIFO);
		ADD32ItoR(ESP, 8);
		return;
	}

	switch (mem) {
		case 0x1000f590: // DMAC_ENABLEW
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf520], mmreg);
			_eeWriteConstMem32((u32)&PS2MEM_HW[0xf590], mmreg);
			break;
		case 0x1000f130:
		case 0x1000f410:
		case 0x1000f430:
			break;

		default:

#ifdef WIN32_VIRTUAL_MEM
			_eeWriteConstMem128( PS2MEM_BASE_+mem, mmreg);
#else
			if (mem < 0x10010000)
				_eeWriteConstMem128((u32)&PS2MEM_HW[mem&0xffff], mmreg);
#endif
			break;
	}
}

