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

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <malloc.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"
#include "VU0.h"
#include "VUmicro.h"
#include "iVUmicro.h"
#include "iVU1micro.h"
#include "iVUops.h"
#include "VUops.h"

//#include "ivu1zerorec.h"

// TODO: there's a bug in spyro start menu where release vurec works but debug vurec breaks

#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

#define USE_SUPER 0

static VURegs *VU = (VURegs*)&VU1;

char *recMemVU1;	/* VU1 x86 executable mem */
char *recVU1;	   	/* VU1 program pointers */
char *recVU1mac;
char *recVU1status;
char *recVU1clip;
char *recVU1Q;
char *recVU1P;
char *recVU1cycles;
char* recVU1XMMRegs;
char *recPtrVU1;
char* recVU1Blocks;

u32 vu1recpcold = -1;
u32 vu1reccountold = -1;

static _vuopinfo _opinfo[256];

//Lower/Upper instructions can use that..
#define _Ft_ ((VU1.code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((VU1.code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((VU1.code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X ((VU1.code>>24) & 0x1)
#define _Y ((VU1.code>>23) & 0x1)
#define _Z ((VU1.code>>22) & 0x1)
#define _W ((VU1.code>>21) & 0x1)

#define _Fsf_ ((VU1.code >> 21) & 0x03)
#define _Ftf_ ((VU1.code >> 23) & 0x03)


#define VU1_VFx_ADDR(x) (u32)&VU1.VF[x].UL[0]
#define VU1_VFy_ADDR(x) (u32)&VU1.VF[x].UL[1]
#define VU1_VFz_ADDR(x) (u32)&VU1.VF[x].UL[2]
#define VU1_VFw_ADDR(x) (u32)&VU1.VF[x].UL[3]

#define VU1_REGR_ADDR (u32)&VU1.VI[REG_R]
#define VU1_REGI_ADDR (u32)&VU1.VI[REG_I]
#define VU1_REGQ_ADDR (u32)&VU1.VI[REG_Q]
#define VU1_REGMAC_ADDR (u32)&VU0.VI[REG_MAC_FLAG]

#define VU1_VI_ADDR(x)	(u32)&VU1.VI[x].UL

#define VU1_ACCx_ADDR (u32)&VU1.ACC.UL[0]
#define VU1_ACCy_ADDR (u32)&VU1.ACC.UL[1]
#define VU1_ACCz_ADDR (u32)&VU1.ACC.UL[2]
#define VU1_ACCw_ADDR (u32)&VU1.ACC.UL[3]

static void VU1RecompileBlock(void);

void recVU1Init()
{
	recMemVU1 = SysMmap(0, 0x00800000);
	memset(recMemVU1, 0xcd, 0x00800000);

	recVU1 = (char*) _aligned_malloc( 0x00004000, 16);

	recVU1mac = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1status = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1clip = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1Q = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1P = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1cycles = (char*) _aligned_malloc( 0x00004000 , 16);
	recVU1XMMRegs = (char*)_aligned_malloc( 0x00008000, 16 );
	memset(recVU1cycles,0,0x4000);

	if( USE_SUPER ) {
		recVU1Blocks = (char*)_aligned_malloc( 0x00004000, 16 );
	}
}

void recVU1Shutdown()
{
	int i;
	_aligned_free( recVU1 );
	_aligned_free( recVU1mac );
	_aligned_free( recVU1status );
	_aligned_free( recVU1clip );
	_aligned_free( recVU1Q );
	_aligned_free( recVU1P );

	for (i=0; i<0x4000; i+=4) { if (*(u32*)&recVU1cycles[i]) free((void*)*(u32*)&recVU1cycles[i]); }
	_aligned_free( recVU1XMMRegs);
	_aligned_free( recVU1cycles );

	SysMunmap((uptr)recMemVU1, 0x00800000);

	if( USE_SUPER ) {
		_aligned_free( recVU1Blocks );
	}
}

void recResetVU1( void ) {

	if( CHECK_VU1REC ) {
		memset( recVU1,  0, 0x00004000 );
		memset( recVU1mac,  0, 0x00004000 );
		memset( recVU1status,  0, 0x00004000 );
		memset( recVU1clip,  0, 0x00004000 );
		memset( recVU1Q,  0, 0x00004000 );
		memset( recVU1P,  0, 0x00004000 );

		if( USE_SUPER ) {
			memset( recVU1Blocks, 0, 0x00004000 );
		}
	}

	vu1recpcold = 0;
	recPtrVU1 = recMemVU1;
	x86FpuState = FPU_STATE;
	iCWstate = 0;

	branch = 0;
}

void executeVU1( void ) {

	void (*recFunc)( void );

	if (VU1.VI[REG_TPC].UL >= VU1.maxmicro) { 
#ifdef CPU_LOG
		SysPrintf("VU1 memory overflow!!: %x\n", VU->VI[REG_TPC].UL);
#endif
		VU0.VI[REG_VPU_STAT].UL&= ~0x100;
		VU->cycle++;
		return;
	}

//	SysPrintf("executeVU1 %x\n", VU1.VI[ REG_TPC ].UL);

	assert( (VU1.VI[ REG_TPC ].UL&7) == 0 );

	if( USE_SUPER ) {
		//recFunc = (void (*)(void))SuperVU1GetProgram(VU1.VI[ REG_TPC ].UL);
	}
	else {
		int** p = (int**)&recVU1[VU1.VI[ REG_TPC ].UL ];
		if (p) recFunc = (void (*)(void))*p;
		else { recError(); return; }

		if (recFunc == 0) {
			VU1RecompileBlock();
			recFunc = (void (*)(void))*p;
		}
	}

	__asm call recFunc;
	
//	SysPrintf("exec ok %x %x\n", VU1.VI[REG_VPU_STAT].UL, VU1.branchpc);
}

void recExecuteVU1Block(void) {
/*#if (defined(__i386__) || defined(__x86_64__)) && defined(CPU_LOG)
	u32 ticko;
#endif*/
	if (CHECK_VU1REC) {
/*#if (defined(__i386__) || defined(__x86_64__)) && defined(CPU_LOG)
	ticko = GetCPUTick();
#endif*/
		//_controlfp(_MCW_PC, _PC_24);

		//while(VU0.VI[REG_VPU_STAT].UL & 0x100) {
	executeVU1();
			
		//}
		//_controlfp(_MCW_PC, 0);

/*#if (defined(__i386__) || defined(__x86_64__)) && defined(CPU_LOG)
	stats.vu1count+= GetCPUTick() - ticko;
#endif*/
	}
	else { 
		intExecuteVU1Block();
	}
}

void recClearVU1( u32 Addr, u32 Size ) {
	assert( (Addr&7) == 0 );

	if( CHECK_VU1REC ) {

		if( USE_SUPER ) {
			//SuperVU1Clear(Addr, Size);
		}
		else {
			memset( (void*)&recVU1[Addr], 0, Size * 4 );
		}
	}
}

__declspec(align(16)) static VECTOR _VF;
__declspec(align(16)) static VECTOR _VFc;
__declspec(align(16)) static REG_VI _VI;
__declspec(align(16)) static REG_VI _VIc;

static int recMacStop;
static int recStatusStop;
static int recClipStop;

static u32 recQstop;
static u32 recQpending;

static u32 recPstop;
static u32 recPpending;

static void checkvucodefn()
{
	int pctemp;

	__asm mov pctemp, eax
	SysPrintf("vu1 code changed! %x\n", pctemp);
}

static void VU1Recompile(_vuopinfo *info, int i) { 
	_VURegsNum lregs;
	_VURegsNum uregs;
	u32 *ptr;
	int vfreg;
	int vireg;

	ptr = (u32*)&VU1.Micro[ pc ]; 
	pc += 8;
	cinfo = info;

#ifdef _DEBUG
	CMP32ItoM((u32)ptr, ptr[0]);
	j8Ptr[0] = JNE8(0);
	CMP32ItoM((u32)(ptr+1), ptr[1]);
	j8Ptr[1] = JNE8(0);
	j8Ptr[2] = JMP8(0);
	x86SetJ8( j8Ptr[0] );
	x86SetJ8( j8Ptr[1] );
	MOV32ItoR(EAX, pc);
	CALLFunc((u32)checkvucodefn);
	x86SetJ8( j8Ptr[ 2 ] );
#endif

/*	SysPrintf("vucycle %d (%p)\n", info->cycle, info);
	SysPrintf("VU1Recompile Upper: %s\n", disVU1MicroUF( ptr[1], pc ) );
	if ((ptr[1] & 0x80000000) == 0) {
		SysPrintf("VU1Recompile Lower: %s\n", disVU1MicroLF( ptr[0], pc ) );
	}*/

	if ((info->macflag & 0x2) || (info->statusflag & 0x2) || (info->clipflag & 0x2)) {
		int j=i-1;

		while (j >= 0)
		{
			if (_opinfo[i-1].cycle - _opinfo[j].cycle >= 3)
			{
				if (info->macflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU1mac[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL,EAX);
					recMacStop = 1;
				}
				if (info->statusflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU1status[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL,EAX);
					recStatusStop = 1;
				}
				if (info->clipflag & 0x2) {
					MOV32MtoR(EAX,(u32)&recVU1clip[VU->VI[REG_TPC].UL+j*8]);
					MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL,EAX);
					recClipStop = 1;
				}
				break;
			}
			j--;
		}
	}
	if (((info->macflag & 0x2) && !recMacStop) ||
		((info->statusflag & 0x2) && !recStatusStop) ||
		((info->clipflag & 0x2) && !recClipStop))
	{
		if (i==0) MOV32ItoR(ECX,3);
		else MOV32ItoR(ECX,3 - (_opinfo[i-1].cycle - (vucycleold-1)));
		MOV32MtoR(EAX,(u32)&vu1reccountold);
		DEC32R(EAX);

		j8Ptr[2] = (u8*)x86Ptr;
		CMP32ItoR(EAX,0);
		j8Ptr[0] = JL8(0);
		CMP32ItoR(ECX,0);
		j8Ptr[1] = JLE8(0);

		MOV32RtoR(EDX,EAX);
		SHL32ItoR(EDX,2);
		MOV32ItoR(ESI,(u32)recVU1cycles);
		ADD32MtoR(ESI,(u32)&vu1recpcold);
		MOV32RmtoR(ESI,ESI);
		ADD32RtoR(EDX,ESI);
		MOV32RmtoR(EDX,EDX);
		SUB32RtoR(ECX,EDX);
		DEC32R(EAX);
		JMP8((u32)j8Ptr[2] - (u32)x86Ptr - 2);

		x86SetJ8(j8Ptr[1]);
		x86SetJ8(j8Ptr[0]);

		if (3 - (_opinfo[i-1].cycle - (vucycleold-1)) > 0) INC32R(EAX);
		SHL32ItoR(EAX,3);
		ADD32MtoR(EAX,(u32)&vu1recpcold);

		if (info->macflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU1mac);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_MAC_FLAG].UL,EDX);
		}
		if (info->statusflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU1status);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_STATUS_FLAG].UL,EDX);
		}
		if (info->clipflag & 0x2) {
			MOV32RtoR(EDX,EAX);
			ADD32ItoR(EDX,(u32)recVU1clip);
			MOV32RmtoR(EDX,EDX);
			MOV32RtoM((u32)&VU0.VI[REG_CLIP_FLAG].UL,EDX);
		}
	}

	if ((info->q & 0x4) && !recQstop) {
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[0] = JLE8(0);
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		MOV32ItoM((u32)&recQpending,0);
		x86SetJ8(j8Ptr[0]);
		recQstop = 1;
	}
	else if ((info->q & 0x1) && !recQstop) {
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		MOV32ItoM((u32)&recQpending,0);
		recQstop = 1;
	}

	if ((info->p & 0x4) && !recPstop) {
		CMP32ItoM((u32)&recPpending,0);
		j8Ptr[0] = JLE8(0);
		MOV32MtoR(EAX, (u32)&VU->p);
		MOV32RtoM((u32)&VU->VI[REG_P].UL, EAX);
		MOV32ItoM((u32)&recPpending,0);
		x86SetJ8(j8Ptr[0]);
		recPstop = 1;
	}
	else if ((info->p & 0x1) && !recPstop) {
		MOV32MtoR(EAX, (u32)&VU->p);
		MOV32RtoM((u32)&VU->VI[REG_P].UL, EAX);
		MOV32ItoM((u32)&recPpending,0);
		recPstop = 1;
	}

	if (ptr[1] & 0x40000000 ) { 
		AND32ItoM( (u32)&VU0.VI[ REG_VPU_STAT ].UL, ~0x100 ); /* E flag */ 
		AND32ItoM( (u32)&VU->vifRegs->stat, ~0x4 );
		branch |= 8; 
	}

	VU->code = ptr[1];
	VU1regs_UPPER_OPCODE[VU->code & 0x3f](&uregs);
	/* check upper flags */ 
	if (ptr[1] & 0x80000000) { /* I flag */ 
		VU1.code = ptr[1]; 
		recVU1_UPPER_OPCODE[ VU1.code & 0x3f ]( );

		MOV32ItoM( (u32)&VU1.VI[ REG_I ].UL, ptr[ 0 ] ); 
	} else {
		VU->code = ptr[0]; 
		VU1regs_LOWER_OPCODE[VU->code >> 25](&lregs);

		vfreg = 0; vireg = 0;
		if (uregs.VFwrite) {
			if (lregs.VFwrite == uregs.VFwrite) {
				SysPrintf("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle\n");
			}
			if (lregs.VFread0 == uregs.VFwrite ||
				lregs.VFread1 == uregs.VFwrite) {
				int reg = _allocVFtoXMMreg(VU, -1, uregs.VFwrite, MODE_READ);
				SSE_MOVAPS_XMM_to_M128((u32)&_VF, reg);
				vfreg = uregs.VFwrite;
			}
        }

		if (uregs.VIread & (1 << REG_CLIP_FLAG)) {
			if (lregs.VIwrite & (1 << REG_CLIP_FLAG)) {
				SysPrintf("*PCSX2*: Warning, VI write to the same reg in both lower/upper cycle\n");
			}
			if (lregs.VIread & (1 << REG_CLIP_FLAG)) {
				SysPrintf("*PCSX2*: VUrec: fixme, save clip flag !\n");
				_VI = VU0.VI[REG_CLIP_FLAG];
				vireg = REG_CLIP_FLAG;
			}
		}

		VU1.code = ptr[1]; 
		recVU1_UPPER_OPCODE[ VU1.code & 0x3f ]( );

		if (vfreg) {
			int reg = _allocVFtoXMMreg(VU, -1, vfreg, MODE_READ | MODE_WRITE);
			SSE_MOVAPS_XMM_to_M128((u32)&_VFc, reg);
			SSE_MOVAPS_M128_to_XMM(reg, (u32)&_VF);
		}
		if (vireg) {
			SysPrintf("should be recompiled\n");
			_VIc = VU1.VI[vireg];
			VU1.VI[vireg] = _VI;
		}

		VU1.code = ptr[0]; 
		recVU1_LOWER_OPCODE[ VU1.code >> 25 ]( ); 

		if (vfreg) {
			int reg = _allocVFtoXMMreg(VU, -1, vfreg, MODE_WRITE);
			SSE_MOVAPS_M128_to_XMM(reg, (u32)&_VFc);
		}
		if (vireg) {
			VU1.VI[vireg] = _VIc;
		}
	}

	MOV32MtoR(EAX, (u32)&VU->macflag);
	MOV32RtoM((u32)&recVU1mac[pc-8],EAX);

	MOV32MtoR(EAX, (u32)&VU->statusflag);
	MOV32RtoM((u32)&recVU1status[pc-8],EAX);

	MOV32MtoR(EAX, (u32)&VU->clipflag);
	MOV32RtoM((u32)&recVU1clip[pc-8],EAX);

	if (info->q & 0x8) {
//		SysPrintf("flushing q\n");
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
	}
	if (!recQstop) {
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[1] = JLE8(0);
		SUB32ItoM((u32)&recQpending,(*(u32**)&recVU1cycles[VU1.VI[ REG_TPC ].UL])[i]);
		CMP32ItoM((u32)&recQpending,0);
		j8Ptr[0] = JG8(0);
		MOV32MtoR(EAX, (u32)&VU->q);
		MOV32RtoM((u32)&VU->VI[REG_Q].UL, EAX);
		x86SetJ8(j8Ptr[0]);
		x86SetJ8(j8Ptr[1]);
	}

	if (info->p & 0x8) {
//		SysPrintf("flushing p\n");
		MOV32MtoR(EAX, (u32)&VU->p);
		MOV32RtoM((u32)&VU->VI[REG_P].UL, EAX);
	}
	if (!recPstop) {
		CMP32ItoM((u32)&recPpending,0);
		j8Ptr[1] = JLE8(0);
		SUB32ItoM((u32)&recPpending,(*(u32**)&recVU1cycles[VU1.VI[ REG_TPC ].UL])[i]);
		CMP32ItoM((u32)&recPpending,0);
		j8Ptr[0] = JG8(0);
		MOV32MtoR(EAX, (u32)&VU->p);
		MOV32RtoM((u32)&VU->VI[REG_P].UL, EAX);
		x86SetJ8(j8Ptr[0]);
		x86SetJ8(j8Ptr[1]);
	}
}

static void iDumpBlock(char * ptr) {
	FILE *f;
	char command[ 256 ];
	char filename[ 256 ];
	u32 *mem;
	u32 i;

#ifdef __WIN32__
	CreateDirectory("dumps", NULL);
	sprintf( filename, "dumps\\dump%.8X.txt", VU1.VI[ REG_TPC ].UL );
#else
	mkdir("dumps", 0755);
	sprintf( filename, "dumps/dump%.8X.txt", VU1.VI[ REG_TPC ].UL );
#endif
	SysPrintf( "dump1 %x => %x (%s)\n", VU1.VI[ REG_TPC ].UL, pc, filename );
//	sprintf( filename, "dump.txt");

#ifndef __x86_64__
#if 0
	for ( i = VU1.VI[ REG_TPC ].UL; i < pc; i += 8 ) {
		mem = (u32*)&VU1.Micro[ i ]; 
		SysPrintf( "Upper: %s\n", disVU1MicroUF( mem[ 1 ], i + 4 ) );
		if ( mem[ 1 ] & 0x80000000 ) { /* I flag */ 
			SysPrintf( "Lower: Immediate %x\n", mem[ 0 ] );
		} else {
			SysPrintf( "Lower: %s\n", disVU1MicroLF( mem[ 0 ], i ) );
		}
	}
#endif
#endif

	fflush( stdout );
	f = fopen( "dump1", "wb" );
	fwrite( ptr, 1, (u32)x86Ptr - (u32)ptr, f );
	fclose( f );

#ifdef __x86_64__
	sprintf( command, "objdump -D --target=binary --architecture=i386:x86-64 dump1 > %s", filename );
#else
	sprintf( command, "objdump -D --target=binary --architecture=i386 dump1 > %s", filename );
#endif
	system( command );

#ifndef __x86_64__
	f = fopen( filename, "at" );
	fprintf( f, "\n\n" );
	for ( i = VU1.VI[REG_TPC].UL; i < pc; i += 8 ) {
		mem = (u32*)&VU1.Micro[i]; 
		fprintf(f, "Upper: %s\n", disVU1MicroUF( mem[1], i+4 ) );
		fprintf(f, "Lower: %s\n", disVU1MicroLF( mem[0], i ) );
	}
	fprintf(f, "\n---------------\n");
	fclose( f );
#endif
}

static u32 _mxcsr = 0x7F80;
//u32 _mxcsrold;

static void VU1RecompileBlock(void) {
	char *ptr;
	int i;

	/* if recPtr reached the mem limit reset whole mem */ 
	if ( ( (u32)recPtrVU1 - (u32)recMemVU1 ) >= 0x760000 ) { 
		recResetVU1(); 
	} 

	recPtrVU1 += 4;

	x86SetPtr(recPtrVU1);
	recResetFlags( );

//	SysPrintf("recompile %x\n", VU1.VI[ REG_TPC ].UL );
	*(u32*)&recVU1[ VU1.VI[ REG_TPC ].UL ] = (u32)x86Ptr; 
	ptr = x86Ptr;
	pc = VU1.VI[ REG_TPC ].UL;
	branch = 0;
	vucycleold = vucycle;

	memset(VU->fmac,0,sizeof(VU->fmac));
	memset(&VU->fdiv,0,sizeof(VU->fdiv));
	memset(&VU->efu,0,sizeof(VU->efu));

	memset(_opinfo, 0, sizeof(_opinfo));

#ifdef _DEBUG
	// save/restore ESI since VC++ uses it for debugging
	PUSH32R(ESI);
#endif

	for (i=0; i<255; i++) { 
		_vurecAnalyzeOp(&VU1, &_opinfo[i]);
 
		if (branch) { i++; break; }
	}
	if (branch != 0) {
		_vurecAnalyzeOp(&VU1, &_opinfo[i++]);
	}

	_vurecAnalyzeBlock(&VU1, _opinfo, i);

	pc = VU1.VI[ REG_TPC ].UL; 
	branch = 0;

	_initXMMregs();
	//SSE_STMXCSR((u32)&_mxcsrold);
	SSE_STMXCSR((u32)recPtrVU1-4);
	SSE_LDMXCSR((u32)&_mxcsr);
	MOV32ItoM((u32)&VU1.branch, 0);

	recMacStop = 0;
	recStatusStop = 0;
	recClipStop = 0;
	recQstop = 0;
	recPstop = 0;

	CMP32ItoM((u32)&vu1recpcold,(u32)-1);
	j8Ptr[0] = JNE8(0);

	MOV32ItoM((u32)&recQpending,0);
	MOV32ItoM((u32)&recPpending,0);
	j8Ptr[1] = JMP8(0);

	x86SetJ8(j8Ptr[0]);
	CMP32ItoM((u32)&recQpending,0);
	j8Ptr[2] = JG8(0);
	MOV32ItoR(EAX,(u32)recVU1Q);
	ADD32MtoR(EAX,(u32)&vu1recpcold);
	MOV32RmtoR(EAX,EAX);
	MOV32RtoM((u32)&recQpending,EAX);
	x86SetJ8(j8Ptr[2]);

	CMP32ItoM((u32)&recPpending,0);
	j8Ptr[2] = JG8(0);
	MOV32ItoR(EAX,(u32)recVU1P);
	ADD32MtoR(EAX,(u32)&vu1recpcold);
	MOV32RmtoR(EAX,EAX);
	MOV32RtoM((u32)&recPpending,EAX);
	x86SetJ8(j8Ptr[2]);

	x86SetJ8(j8Ptr[1]);

	for (i=0; i<255; i++) {
		if (pc >= VU1.maxmicro) break;
		VU1Recompile(&_opinfo[i],i);

		if (branch) { i++; break; }
	}

	if( branch & 8 ) {
		
		VU1Recompile(&_opinfo[i++],i);

		// check if didn't come from a branch
		switch( (branch&7) ) {
			case 0:
				MOV32ItoM( (u32)&VU1.VI[ REG_TPC ].UL, pc ); 
				break;
			case 1:
				MOV32MtoR( EAX, (u32)&VU1.branchpc ); 
				MOV32RtoM( (u32)&VU1.VI[ REG_TPC ].UL, EAX );
				break;
			case 3:
				break;
		}
	}
	else {
		
		if (branch == 3) {
			VU1Recompile(&_opinfo[i++],i);
		} else
		if (branch == 0) { 
			MOV32ItoM( (u32)&VU1.VI[ REG_TPC ].UL, pc ); 
		} else {
			VU1Recompile(&_opinfo[i++],i);

			CMP32ItoM( (u32)&VU1.branch, 2);
			j8Ptr[0] = JNE8(0);

			MOV32MtoR( EAX, (u32)&VU1.branchpc ); 
			MOV32RtoM( (u32)&VU1.VI[ REG_TPC ].UL, EAX );
			j8Ptr[1] = JMP8(0);

			x86SetJ8( j8Ptr[ 0 ] );
			MOV32ItoM( (u32)&VU1.VI[ REG_TPC ].UL, pc ); 

			x86SetJ8( j8Ptr[ 1 ] );
		}
	}

	ADD32ItoM((u32)&VU->cycle,vucycle-vucycleold);
	MOV32ItoM((u32)&vu1recpcold,VU1.VI[ REG_TPC ].UL);
	MOV32ItoM((u32)&vu1reccountold,i);

	_freeXMMregs();

	if( !(branch&8) ) {
		MOV32MtoR(EAX,(u32)&VU1.VI[ REG_TPC ].UL);
		CMP32ItoR(EAX,VU1.maxmicro);
		j8Ptr[1] = JGE8(0);
		ADD32ItoR(EAX,(u32)recVU1);
		MOV32RmtoR(EAX,EAX);
		CMP32ItoR(EAX,0);
		j8Ptr[0] = JE8(0);
		iFlushCall();
		CALL32R(EAX);
		x86SetJ8(j8Ptr[0]);
		x86SetJ8(j8Ptr[1]);
	}

	//SSE_LDMXCSR((u32)&_mxcsrold);
	SSE_LDMXCSR((u32)recPtrVU1-4);

#ifdef _DEBUG
	POP32R(ESI);
#endif

	RET();
//	iDumpBlock(ptr);

	recPtrVU1 = x86Ptr;
}

void (*recVU1_LOWER_OPCODE[128])() = { 
	recVU1MI_LQ    , recVU1MI_SQ    , recVU1unknown , recVU1unknown,  
	recVU1MI_ILW   , recVU1MI_ISW   , recVU1unknown , recVU1unknown,  
	recVU1MI_IADDIU, recVU1MI_ISUBIU, recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1MI_FCEQ  , recVU1MI_FCSET , recVU1MI_FCAND, recVU1MI_FCOR, /* 0x10 */ 
	recVU1MI_FSEQ  , recVU1MI_FSSET , recVU1MI_FSAND, recVU1MI_FSOR, 
	recVU1MI_FMEQ  , recVU1unknown  , recVU1MI_FMAND, recVU1MI_FMOR, 
	recVU1MI_FCGET , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1MI_B     , recVU1MI_BAL   , recVU1unknown , recVU1unknown, /* 0x20 */  
	recVU1MI_JR    , recVU1MI_JALR  , recVU1unknown , recVU1unknown, 
	recVU1MI_IBEQ  , recVU1MI_IBNE  , recVU1unknown , recVU1unknown, 
	recVU1MI_IBLTZ , recVU1MI_IBGTZ , recVU1MI_IBLEZ, recVU1MI_IBGEZ, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x30 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1LowerOP  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x40*/  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x50 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x60 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x70 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
}; 
 
void (*recVU1LowerOP_T3_00_OPCODE[32])() = { 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1MI_MOVE  , recVU1MI_LQI   , recVU1MI_DIV  , recVU1MI_MTIR,  
	recVU1MI_RNEXT , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x10 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1MI_MFP   , recVU1MI_XTOP , recVU1MI_XGKICK,  
	recVU1MI_ESADD , recVU1MI_EATANxy, recVU1MI_ESQRT, recVU1MI_ESIN,  
}; 
 
void (*recVU1LowerOP_T3_01_OPCODE[32])() = { 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1MI_MR32  , recVU1MI_SQI   , recVU1MI_SQRT , recVU1MI_MFIR,  
	recVU1MI_RGET  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x10 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1MI_XITOP, recVU1unknown,  
	recVU1MI_ERSADD, recVU1MI_EATANxz, recVU1MI_ERSQRT, recVU1MI_EATAN, 
}; 
 
void (*recVU1LowerOP_T3_10_OPCODE[32])() = { 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1MI_LQD   , recVU1MI_RSQRT, recVU1MI_ILWR,  
	recVU1MI_RINIT , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x10 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1MI_ELENG , recVU1MI_ESUM  , recVU1MI_ERCPR, recVU1MI_EEXP,  
}; 
 
void (*recVU1LowerOP_T3_11_OPCODE[32])() = { 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1MI_SQD   , recVU1MI_WAITQ, recVU1MI_ISWR,  
	recVU1MI_RXOR  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x10 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1MI_ERLENG, recVU1unknown  , recVU1MI_WAITP, recVU1unknown,  
}; 
 
void (*recVU1LowerOP_OPCODE[64])() = { 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x10 */  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown, /* 0x20 */  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1MI_IADD  , recVU1MI_ISUB  , recVU1MI_IADDI, recVU1unknown, /* 0x30 */ 
	recVU1MI_IAND  , recVU1MI_IOR   , recVU1unknown , recVU1unknown,  
	recVU1unknown  , recVU1unknown  , recVU1unknown , recVU1unknown,  
	recVU1LowerOP_T3_00, recVU1LowerOP_T3_01, recVU1LowerOP_T3_10, recVU1LowerOP_T3_11,  
}; 
 
void (*recVU1_UPPER_OPCODE[64])() = { 
	recVU1MI_ADDx  , recVU1MI_ADDy  , recVU1MI_ADDz  , recVU1MI_ADDw, 
	recVU1MI_SUBx  , recVU1MI_SUBy  , recVU1MI_SUBz  , recVU1MI_SUBw, 
	recVU1MI_MADDx , recVU1MI_MADDy , recVU1MI_MADDz , recVU1MI_MADDw, 
	recVU1MI_MSUBx , recVU1MI_MSUBy , recVU1MI_MSUBz , recVU1MI_MSUBw, 
	recVU1MI_MAXx  , recVU1MI_MAXy  , recVU1MI_MAXz  , recVU1MI_MAXw,  /* 0x10 */  
	recVU1MI_MINIx , recVU1MI_MINIy , recVU1MI_MINIz , recVU1MI_MINIw, 
	recVU1MI_MULx  , recVU1MI_MULy  , recVU1MI_MULz  , recVU1MI_MULw, 
	recVU1MI_MULq  , recVU1MI_MAXi  , recVU1MI_MULi  , recVU1MI_MINIi, 
	recVU1MI_ADDq  , recVU1MI_MADDq , recVU1MI_ADDi  , recVU1MI_MADDi, /* 0x20 */ 
	recVU1MI_SUBq  , recVU1MI_MSUBq , recVU1MI_SUBi  , recVU1MI_MSUBi, 
	recVU1MI_ADD   , recVU1MI_MADD  , recVU1MI_MUL   , recVU1MI_MAX, 
	recVU1MI_SUB   , recVU1MI_MSUB  , recVU1MI_OPMSUB, recVU1MI_MINI, 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown,  /* 0x30 */ 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown, 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown, 
	recVU1_UPPER_FD_00, recVU1_UPPER_FD_01, recVU1_UPPER_FD_10, recVU1_UPPER_FD_11,  
}; 
 
void (*recVU1_UPPER_FD_00_TABLE[32])() = { 
	recVU1MI_ADDAx, recVU1MI_SUBAx , recVU1MI_MADDAx, recVU1MI_MSUBAx, 
	recVU1MI_ITOF0, recVU1MI_FTOI0, recVU1MI_MULAx , recVU1MI_MULAq , 
	recVU1MI_ADDAq, recVU1MI_SUBAq, recVU1MI_ADDA  , recVU1MI_SUBA  , 
	recVU1unknown , recVU1unknown , recVU1unknown  , recVU1unknown  , 
	recVU1unknown , recVU1unknown , recVU1unknown  , recVU1unknown  , 
	recVU1unknown , recVU1unknown , recVU1unknown  , recVU1unknown  , 
	recVU1unknown , recVU1unknown , recVU1unknown  , recVU1unknown  , 
	recVU1unknown , recVU1unknown , recVU1unknown  , recVU1unknown  , 
}; 
 
void (*recVU1_UPPER_FD_01_TABLE[32])() = { 
	recVU1MI_ADDAy , recVU1MI_SUBAy  , recVU1MI_MADDAy, recVU1MI_MSUBAy, 
	recVU1MI_ITOF4 , recVU1MI_FTOI4 , recVU1MI_MULAy , recVU1MI_ABS   , 
	recVU1MI_MADDAq, recVU1MI_MSUBAq, recVU1MI_MADDA , recVU1MI_MSUBA , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
}; 
 
void (*recVU1_UPPER_FD_10_TABLE[32])() = { 
	recVU1MI_ADDAz , recVU1MI_SUBAz  , recVU1MI_MADDAz, recVU1MI_MSUBAz, 
	recVU1MI_ITOF12, recVU1MI_FTOI12, recVU1MI_MULAz , recVU1MI_MULAi , 
	recVU1MI_MADDAi, recVU1MI_SUBAi , recVU1MI_MULA  , recVU1MI_OPMULA, 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
}; 
 
void (*recVU1_UPPER_FD_11_TABLE[32])() = { 
	recVU1MI_ADDAw , recVU1MI_SUBAw  , recVU1MI_MADDAw, recVU1MI_MSUBAw, 
	recVU1MI_ITOF15, recVU1MI_FTOI15, recVU1MI_MULAw , recVU1MI_CLIP  , 
	recVU1MI_MADDAi, recVU1MI_MSUBAi, recVU1unknown  , recVU1MI_NOP   , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
	recVU1unknown  , recVU1unknown  , recVU1unknown  , recVU1unknown  , 
}; 

void recVU1_UPPER_FD_00( void )
{ 
	recVU1_UPPER_FD_00_TABLE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1_UPPER_FD_01( void )
{ 
	recVU1_UPPER_FD_01_TABLE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1_UPPER_FD_10( void )
{ 
	recVU1_UPPER_FD_10_TABLE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1_UPPER_FD_11( void )
{ 
	recVU1_UPPER_FD_11_TABLE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1LowerOP( void )
{ 
	recVU1LowerOP_OPCODE[ VU1.code & 0x3f ]( ); 
} 
 
void recVU1LowerOP_T3_00( void )
{ 
	recVU1LowerOP_T3_00_OPCODE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1LowerOP_T3_01( void )
{ 
	recVU1LowerOP_T3_01_OPCODE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1LowerOP_T3_10( void )
{ 
	recVU1LowerOP_T3_10_OPCODE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
} 
 
void recVU1LowerOP_T3_11( void )
{ 
	recVU1LowerOP_T3_11_OPCODE[ ( VU1.code >> 6 ) & 0x1f ]( ); 
}

void recVU1unknown( void )
{ 
#ifdef CPU_LOG
	CPU_LOG("Unknown VU1 micromode opcode calledn"); 
#endif
}  
 
 
 
/****************************************/ 
/*   VU Micromode Upper instructions    */ 
/****************************************/ 

#ifdef RECOMPILE_VUMI_ABS
void recVU1MI_ABS()   { recVUMI_ABS(VU); } 
#else
void recVU1MI_ABS()   { rec_vuABS(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_ADD
void recVU1MI_ADD()  { recVUMI_ADD(VU); } //Causes Texture flicker in FFX and kills RE4 gfx
void recVU1MI_ADDi() { recVUMI_ADDi(VU); } 
void recVU1MI_ADDq() { recVUMI_ADDq(VU); } 
void recVU1MI_ADDx() { recVUMI_ADDx(VU); } 
void recVU1MI_ADDy() { recVUMI_ADDy(VU); } 
void recVU1MI_ADDz() { recVUMI_ADDz(VU); } 
void recVU1MI_ADDw() { recVUMI_ADDw(VU); }
#else
void recVU1MI_ADD()   { rec_vuADD(VU1); } 
void recVU1MI_ADDi()  { rec_vuADDi(VU1); } 
void recVU1MI_ADDq()  { rec_vuADDq(VU1); } 
void recVU1MI_ADDx()  { rec_vuADDx(VU1); } 
void recVU1MI_ADDy()  { rec_vuADDy(VU1); } 
void recVU1MI_ADDz()  { rec_vuADDz(VU1); } 
void recVU1MI_ADDw()  { rec_vuADDw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_ADDA
void recVU1MI_ADDA()  { recVUMI_ADDA(VU); } 
void recVU1MI_ADDAi() { recVUMI_ADDAi(VU); } 
void recVU1MI_ADDAq() { recVUMI_ADDAq(VU); } 
void recVU1MI_ADDAx() { recVUMI_ADDAx(VU); } 
void recVU1MI_ADDAy() { recVUMI_ADDAy(VU); } 
void recVU1MI_ADDAz() { recVUMI_ADDAz(VU); } 
void recVU1MI_ADDAw() { recVUMI_ADDAw(VU); } 
#else
void recVU1MI_ADDA()  { rec_vuADDA(VU1); } 
void recVU1MI_ADDAi() { rec_vuADDAi(VU1); } 
void recVU1MI_ADDAq() { rec_vuADDAq(VU1); } 
void recVU1MI_ADDAx() { rec_vuADDAx(VU1); } 
void recVU1MI_ADDAy() { rec_vuADDAy(VU1); } 
void recVU1MI_ADDAz() { rec_vuADDAz(VU1); } 
void recVU1MI_ADDAw() { rec_vuADDAw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_SUB
void recVU1MI_SUB()  { recVUMI_SUB(VU); } 
void recVU1MI_SUBi() { recVUMI_SUBi(VU); } 
void recVU1MI_SUBq() { recVUMI_SUBq(VU); } 
void recVU1MI_SUBx() { recVUMI_SUBx(VU); } 
void recVU1MI_SUBy() { recVUMI_SUBy(VU); } 
void recVU1MI_SUBz() { recVUMI_SUBz(VU); } 
void recVU1MI_SUBw() { recVUMI_SUBw(VU); } 
#else
void recVU1MI_SUB()   { rec_vuSUB(VU1); } 
void recVU1MI_SUBi()  { rec_vuSUBi(VU1); } 
void recVU1MI_SUBq()  { rec_vuSUBq(VU1); } 
void recVU1MI_SUBx()  { rec_vuSUBx(VU1); } 
void recVU1MI_SUBy()  { rec_vuSUBy(VU1); } 
void recVU1MI_SUBz()  { rec_vuSUBz(VU1); } 
void recVU1MI_SUBw()  { rec_vuSUBw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_SUBA
void recVU1MI_SUBA()  { recVUMI_SUBA(VU); } 
void recVU1MI_SUBAi() { recVUMI_SUBAi(VU); } 
void recVU1MI_SUBAq() { recVUMI_SUBAq(VU); } 
void recVU1MI_SUBAx() { recVUMI_SUBAx(VU); } 
void recVU1MI_SUBAy() { recVUMI_SUBAy(VU); } 
void recVU1MI_SUBAz() { recVUMI_SUBAz(VU); } 
void recVU1MI_SUBAw() { recVUMI_SUBAw(VU); } 
#else
void recVU1MI_SUBA()  { rec_vuSUBA(VU1); } 
void recVU1MI_SUBAi() { rec_vuSUBAi(VU1); } 
void recVU1MI_SUBAq() { rec_vuSUBAq(VU1); } 
void recVU1MI_SUBAx() { rec_vuSUBAx(VU1); } 
void recVU1MI_SUBAy() { rec_vuSUBAy(VU1); } 
void recVU1MI_SUBAz() { rec_vuSUBAz(VU1); } 
void recVU1MI_SUBAw() { rec_vuSUBAw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MUL 
void recVU1MI_MUL()  { recVUMI_MUL(VU); }
void recVU1MI_MULi() { recVUMI_MULi(VU); } 
void recVU1MI_MULq() { recVUMI_MULq(VU); }
void recVU1MI_MULx() { recVUMI_MULx(VU); } 
void recVU1MI_MULy() { recVUMI_MULy(VU); } 
void recVU1MI_MULz() { recVUMI_MULz(VU); } 
void recVU1MI_MULw() { recVUMI_MULw(VU); }
#else
void recVU1MI_MUL()   { rec_vuMUL(VU1); } 
void recVU1MI_MULi()  { rec_vuMULi(VU1); } 
void recVU1MI_MULq()  { rec_vuMULq(VU1); } 
void recVU1MI_MULx()  { rec_vuMULx(VU1); } 
void recVU1MI_MULy()  { rec_vuMULy(VU1); } 
void recVU1MI_MULz()  { rec_vuMULz(VU1); } 
void recVU1MI_MULw()  { rec_vuMULw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MULA
void recVU1MI_MULA()  { recVUMI_MULA(VU); } 
void recVU1MI_MULAi() { recVUMI_MULAi(VU); } 
void recVU1MI_MULAq() { recVUMI_MULAq(VU); } 
void recVU1MI_MULAx() { recVUMI_MULAx(VU); } 
void recVU1MI_MULAy() { recVUMI_MULAy(VU); } 
void recVU1MI_MULAz() { recVUMI_MULAz(VU); } 
void recVU1MI_MULAw() { recVUMI_MULAw(VU); } 
#else
void recVU1MI_MULA()  { rec_vuMULA(VU1); } 
void recVU1MI_MULAi() { rec_vuMULAi(VU1); } 
void recVU1MI_MULAq() { rec_vuMULAq(VU1); } 
void recVU1MI_MULAx() { rec_vuMULAx(VU1); } 
void recVU1MI_MULAy() { rec_vuMULAy(VU1); } 
void recVU1MI_MULAz() { rec_vuMULAz(VU1); } 
void recVU1MI_MULAw() { rec_vuMULAw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MADD
void recVU1MI_MADD()  { recVUMI_MADD(VU); } 
void recVU1MI_MADDi() { recVUMI_MADDi(VU); } 
void recVU1MI_MADDq() { recVUMI_MADDq(VU); } 
void recVU1MI_MADDx() { recVUMI_MADDx(VU); } 
void recVU1MI_MADDy() { recVUMI_MADDy(VU); } 
void recVU1MI_MADDz() { recVUMI_MADDz(VU); } 
void recVU1MI_MADDw() { recVUMI_MADDw(VU); } 
#else
void recVU1MI_MADD()  { rec_vuMADD(VU1); } 
void recVU1MI_MADDi() { rec_vuMADDi(VU1); } 
void recVU1MI_MADDq() { rec_vuMADDq(VU1); } 
void recVU1MI_MADDx() { rec_vuMADDx(VU1); } 
void recVU1MI_MADDy() { rec_vuMADDy(VU1); } 
void recVU1MI_MADDz() { rec_vuMADDz(VU1); } 
void recVU1MI_MADDw() { rec_vuMADDw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MADDA
void recVU1MI_MADDA()  { recVUMI_MADDA(VU); } 
void recVU1MI_MADDAi() { recVUMI_MADDAi(VU); } 
void recVU1MI_MADDAq() { recVUMI_MADDAq(VU); } 
void recVU1MI_MADDAx() { recVUMI_MADDAx(VU); } 
void recVU1MI_MADDAy() { recVUMI_MADDAy(VU); } 
void recVU1MI_MADDAz() { recVUMI_MADDAz(VU); } 
void recVU1MI_MADDAw() { recVUMI_MADDAw(VU); } 
#else
void recVU1MI_MADDA()  { rec_vuMADDA(VU1); } 
void recVU1MI_MADDAi() { rec_vuMADDAi(VU1); } 
void recVU1MI_MADDAq() { rec_vuMADDAq(VU1); } 
void recVU1MI_MADDAx() { rec_vuMADDAx(VU1); } 
void recVU1MI_MADDAy() { rec_vuMADDAy(VU1); } 
void recVU1MI_MADDAz() { rec_vuMADDAz(VU1); } 
void recVU1MI_MADDAw() { rec_vuMADDAw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MSUB
void recVU1MI_MSUB()  { recVUMI_MSUB(VU); } 
void recVU1MI_MSUBi() { recVUMI_MSUBi(VU); } 
void recVU1MI_MSUBq() { recVUMI_MSUBq(VU); } 
void recVU1MI_MSUBx() { recVUMI_MSUBx(VU); } 
void recVU1MI_MSUBy() { recVUMI_MSUBy(VU); } 
void recVU1MI_MSUBz() { recVUMI_MSUBz(VU); } 
void recVU1MI_MSUBw() { recVUMI_MSUBw(VU); } 
#else
void recVU1MI_MSUB()  { rec_vuMSUB(VU1); } 
void recVU1MI_MSUBi() { rec_vuMSUBi(VU1); } 
void recVU1MI_MSUBq() { rec_vuMSUBq(VU1); } 
void recVU1MI_MSUBx() { rec_vuMSUBx(VU1); } 
void recVU1MI_MSUBy() { rec_vuMSUBy(VU1); } 
void recVU1MI_MSUBz() { rec_vuMSUBz(VU1); } 
void recVU1MI_MSUBw() { rec_vuMSUBw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MSUBA
void recVU1MI_MSUBA()  { recVUMI_MSUBA(VU); } 
void recVU1MI_MSUBAi() { recVUMI_MSUBAi(VU); } 
void recVU1MI_MSUBAq() { recVUMI_MSUBAq(VU); } 
void recVU1MI_MSUBAx() { recVUMI_MSUBAx(VU); } 
void recVU1MI_MSUBAy() { recVUMI_MSUBAy(VU); } 
void recVU1MI_MSUBAz() { recVUMI_MSUBAz(VU); } 
void recVU1MI_MSUBAw() { recVUMI_MSUBAw(VU); } 
#else
void recVU1MI_MSUBA()  { rec_vuMSUBA(VU1); } 
void recVU1MI_MSUBAi() { rec_vuMSUBAi(VU1); } 
void recVU1MI_MSUBAq() { rec_vuMSUBAq(VU1); } 
void recVU1MI_MSUBAx() { rec_vuMSUBAx(VU1); } 
void recVU1MI_MSUBAy() { rec_vuMSUBAy(VU1); } 
void recVU1MI_MSUBAz() { rec_vuMSUBAz(VU1); } 
void recVU1MI_MSUBAw() { rec_vuMSUBAw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MAX
void recVU1MI_MAX()  { recVUMI_MAX(VU); } 
void recVU1MI_MAXi() { recVUMI_MAXi(VU); } 
void recVU1MI_MAXx() { recVUMI_MAXx(VU); } 
void recVU1MI_MAXy() { recVUMI_MAXy(VU); } 
void recVU1MI_MAXz() { recVUMI_MAXz(VU); } 
void recVU1MI_MAXw() { recVUMI_MAXw(VU); } 
#else
void recVU1MI_MAX()  { rec_vuMAX(VU1); } 
void recVU1MI_MAXi() { rec_vuMAXi(VU1); } 
void recVU1MI_MAXx() { rec_vuMAXx(VU1); } 
void recVU1MI_MAXy() { rec_vuMAXy(VU1); } 
void recVU1MI_MAXz() { rec_vuMAXz(VU1); } 
void recVU1MI_MAXw() { rec_vuMAXw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_MINI
void recVU1MI_MINI()  { recVUMI_MINI(VU); } 
void recVU1MI_MINIi() { recVUMI_MINIi(VU); } 
void recVU1MI_MINIx() { recVUMI_MINIx(VU); } 
void recVU1MI_MINIy() { recVUMI_MINIy(VU); } 
void recVU1MI_MINIz() { recVUMI_MINIz(VU); } 
void recVU1MI_MINIw() { recVUMI_MINIw(VU); }
#else
void recVU1MI_MINI()  { rec_vuMINI(VU1); } 
void recVU1MI_MINIi() { rec_vuMINIi(VU1); } 
void recVU1MI_MINIx() { rec_vuMINIx(VU1); } 
void recVU1MI_MINIy() { rec_vuMINIy(VU1); } 
void recVU1MI_MINIz() { rec_vuMINIz(VU1); } 
void recVU1MI_MINIw() { rec_vuMINIw(VU1); } 
#endif

#ifdef RECOMPILE_VUMI_FTOI
void recVU1MI_FTOI0()  { recVUMI_FTOI0(VU); }
void recVU1MI_FTOI4()  { recVUMI_FTOI4(VU); } 
void recVU1MI_FTOI12() { recVUMI_FTOI12(VU); } 
void recVU1MI_FTOI15() { recVUMI_FTOI15(VU); } 

void recVU1MI_ITOF0()  { recVUMI_ITOF0(VU); } 
void recVU1MI_ITOF4()  { recVUMI_ITOF4(VU); } 
void recVU1MI_ITOF12() { recVUMI_ITOF12(VU); } 
void recVU1MI_ITOF15() { recVUMI_ITOF15(VU); } 
#else
void recVU1MI_FTOI0()  { rec_vuFTOI0(VU1); } 
void recVU1MI_FTOI4()  { rec_vuFTOI4(VU1); } 
void recVU1MI_FTOI12() { rec_vuFTOI12(VU1); } 
void recVU1MI_FTOI15() { rec_vuFTOI15(VU1); } 
void recVU1MI_ITOF0()  { rec_vuITOF0(VU1); } 
void recVU1MI_ITOF4()  { rec_vuITOF4(VU1); } 
void recVU1MI_ITOF12() { rec_vuITOF12(VU1); } 
void recVU1MI_ITOF15() { rec_vuITOF15(VU1); } 
#endif

void recVU1MI_OPMULA() { recVUMI_OPMULA(VU); } 
void recVU1MI_OPMSUB() { recVUMI_OPMSUB(VU); } 
void recVU1MI_NOP()    { } 
void recVU1MI_CLIP()
{ 
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_CLIP(VU);
	else
		rec_vuCLIP(VU1);
}



/*****************************************/ 
/*   VU Micromode Lower instructions    */ 
/*****************************************/ 


#ifdef RECOMPILE_VUMI_MISC

void recVU1MI_MTIR()    { recVUMI_MTIR(VU); }
void recVU1MI_MR32()    { recVUMI_MR32(VU); } 
void recVU1MI_MFIR()    { recVUMI_MFIR(VU); }
void recVU1MI_MOVE()    { recVUMI_MOVE(VU); } 
void recVU1MI_WAITQ()   { } 
void recVU1MI_MFP()     { recVUMI_MFP(VU); } 
void recVU1MI_WAITP()   { recVUMI_WAITP(VU); } 

#else

void recVU1MI_MOVE()    { rec_vuMOVE(VU1); } 
void recVU1MI_MFIR()    { rec_vuMFIR(VU1); } 
void recVU1MI_MTIR()    { rec_vuMTIR(VU1); } 
void recVU1MI_MR32()    { rec_vuMR32(VU1); } 
void recVU1MI_WAITQ()   { } 
void recVU1MI_MFP()     { rec_vuMFP(VU1); } 
void recVU1MI_WAITP()   { rec_vuWAITP(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_MATH

void recVU1MI_SQRT()
{ 
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_SQRT(VU);
	else
		rec_vuSQRT(VU1);
}

void recVU1MI_RSQRT()
{
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_RSQRT(VU);
	else
		rec_vuRSQRT(VU1);
}

void recVU1MI_DIV()
{
	if( cpucaps.hasStreamingSIMD2Extensions )
		recVUMI_DIV(VU);
	else
		rec_vuDIV(VU1);
}

#else

void recVU1MI_DIV()     { rec_vuDIV(VU1);} 
void recVU1MI_SQRT()    { rec_vuSQRT(VU1); } 
void recVU1MI_RSQRT()   { rec_vuRSQRT(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_E

void recVU1MI_ESADD()   { rec_vuESADD(VU1); } 
void recVU1MI_ERSADD()  { rec_vuERSADD(VU1); } 
void recVU1MI_ELENG()   { recVUMI_ELENG(VU); } 
void recVU1MI_ERLENG()  { rec_vuERLENG(VU1); } 
void recVU1MI_EATANxy() { rec_vuEATANxy(VU1); } 
void recVU1MI_EATANxz() { rec_vuEATANxz(VU1); } 
void recVU1MI_ESUM()    { rec_vuESUM(VU1); } 
void recVU1MI_ERCPR()   { rec_vuERCPR(VU1); } 
void recVU1MI_ESQRT()   { rec_vuESQRT(VU1); } 
void recVU1MI_ERSQRT()  { rec_vuERSQRT(VU1); } 
void recVU1MI_ESIN()    { rec_vuESIN(VU1); } 
void recVU1MI_EATAN()   { rec_vuEATAN(VU1); } 
void recVU1MI_EEXP()    { rec_vuEEXP(VU1); } 

#else

void recVU1MI_ESADD()   { rec_vuESADD(VU1); } 
void recVU1MI_ERSADD()  { rec_vuERSADD(VU1); } 
void recVU1MI_ELENG()   { rec_vuELENG(VU1); } 
void recVU1MI_ERLENG()  { rec_vuERLENG(VU1); } 
void recVU1MI_EATANxy() { rec_vuEATANxy(VU1); } 
void recVU1MI_EATANxz() { rec_vuEATANxz(VU1); } 
void recVU1MI_ESUM()    { rec_vuESUM(VU1); } 
void recVU1MI_ERCPR()   { rec_vuERCPR(VU1); } 
void recVU1MI_ESQRT()   { rec_vuESQRT(VU1); } 
void recVU1MI_ERSQRT()  { rec_vuERSQRT(VU1); } 
void recVU1MI_ESIN()    { rec_vuESIN(VU1); } 
void recVU1MI_EATAN()   { rec_vuEATAN(VU1); } 
void recVU1MI_EEXP()    { rec_vuEEXP(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_X

void recVU1MI_XITOP()   { recVUMI_XITOP(VU); }
void recVU1MI_XGKICK()  { recVUMI_XGKICK(VU); }
void recVU1MI_XTOP()    { recVUMI_XTOP(VU); }

#else

void recVU1MI_XITOP()   { rec_vuXITOP(VU1); }
void recVU1MI_XGKICK()  { REC_VUOP(VU1, XGKICK);}
void recVU1MI_XTOP()    { REC_VUOP(VU1, XTOP);}

#endif

#ifdef RECOMPILE_VUMI_RANDOM

void recVU1MI_RINIT()     { recVUMI_RINIT(VU); }
void recVU1MI_RGET()      { recVUMI_RGET(VU); }
void recVU1MI_RNEXT()     { recVUMI_RNEXT(VU); }
void recVU1MI_RXOR()      { recVUMI_RXOR(VU); }

#else

void recVU1MI_RINIT()     { rec_vuRINIT(VU1); }
void recVU1MI_RGET()      { rec_vuRGET(VU1); }
void recVU1MI_RNEXT()     { rec_vuRNEXT(VU1); }
void recVU1MI_RXOR()      { rec_vuRXOR(VU1); }

#endif

#ifdef RECOMPILE_VUMI_FLAG

void recVU1MI_FSAND()   { recVUMI_FSAND(VU); } 
void recVU1MI_FSEQ()    { recVUMI_FSEQ(VU); }
void recVU1MI_FSOR()    { recVUMI_FSOR(VU); }
void recVU1MI_FSSET()   { recVUMI_FSSET(VU); } 
void recVU1MI_FMEQ()    { recVUMI_FMEQ(VU); }
void recVU1MI_FMOR()    { recVUMI_FMOR(VU); }
void recVU1MI_FCEQ()    { recVUMI_FCEQ(VU); }
void recVU1MI_FCOR()    { recVUMI_FCOR(VU); }
void recVU1MI_FCSET()   { recVUMI_FCSET(VU); }
void recVU1MI_FCGET()   { recVUMI_FCGET(VU); }
void recVU1MI_FCAND()   { recVUMI_FCAND(VU); }
void recVU1MI_FMAND()   { recVUMI_FMAND(VU); }

#else

void recVU1MI_FSAND()   { rec_vuFSAND(VU1); } 
void recVU1MI_FSEQ()    { rec_vuFSEQ(VU1); } 
void recVU1MI_FSOR()    { rec_vuFSOR(VU1); } 
void recVU1MI_FSSET()   { rec_vuFSSET(VU1); } 
void recVU1MI_FMAND()   { rec_vuFMAND(VU1); } 
void recVU1MI_FMEQ()    { rec_vuFMEQ(VU1); } 
void recVU1MI_FMOR()    { rec_vuFMOR(VU1); } 
void recVU1MI_FCAND()   { rec_vuFCAND(VU1); } 
void recVU1MI_FCEQ()    { rec_vuFCEQ(VU1); } 
void recVU1MI_FCOR()    { rec_vuFCOR(VU1); } 
void recVU1MI_FCSET()   { rec_vuFCSET(VU1); } 
void recVU1MI_FCGET()   { rec_vuFCGET(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_LOADSTORE

void recVU1MI_LQ()      { recVUMI_LQ(VU); } 
void recVU1MI_LQD()     { recVUMI_LQD(VU); } 
void recVU1MI_LQI()     { recVUMI_LQI(VU); } 
void recVU1MI_SQ()      { recVUMI_SQ(VU); } //causes some corrupt textures/2d
void recVU1MI_SQD()     { recVUMI_SQD(VU); }
void recVU1MI_SQI()     { recVUMI_SQI(VU); }
void recVU1MI_ILW()     { recVUMI_ILW(VU); }  
void recVU1MI_ISW()     { recVUMI_ISW(VU); } //causes corrupt 2d
void recVU1MI_ILWR()    { recVUMI_ILWR(VU); }
void recVU1MI_ISWR()    { recVUMI_ISWR(VU); }

#else

void recVU1MI_LQ()      { rec_vuLQ(VU1); } 
void recVU1MI_LQD()     { rec_vuLQD(VU1); } 
void recVU1MI_LQI()     { rec_vuLQI(VU1); } 
void recVU1MI_SQ()      { rec_vuSQ(VU1); } 
void recVU1MI_SQD()     { rec_vuSQD(VU1); } 
void recVU1MI_SQI()     { rec_vuSQI(VU1); } 
void recVU1MI_ILW()     { rec_vuILW(VU1); } 
void recVU1MI_ISW()     { rec_vuISW(VU1); } 
void recVU1MI_ILWR()    { rec_vuILWR(VU1); } 
void recVU1MI_ISWR()    { rec_vuISWR(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_ARITHMETIC

void recVU1MI_IADD()    { recVUMI_IADD(VU); }
void recVU1MI_IADDI()   { recVUMI_IADDI(VU); }
void recVU1MI_IADDIU()  { recVUMI_IADDIU(VU); } 
void recVU1MI_IOR()     { recVUMI_IOR(VU); }
void recVU1MI_ISUB()    { recVUMI_ISUB(VU); }
void recVU1MI_IAND()    { recVUMI_IAND(VU); }
void recVU1MI_ISUBIU()  { recVUMI_ISUBIU(VU); } 

#else

void recVU1MI_IADD()    { rec_vuIADD(VU1); }
void recVU1MI_IADDI()   { rec_vuIADDI(VU1); }
void recVU1MI_IADDIU()  { rec_vuIADDIU(VU1); } 
void recVU1MI_IOR()     { rec_vuIOR(VU1); }
void recVU1MI_ISUB()    { rec_vuISUB(VU1); }
void recVU1MI_IAND()    { rec_vuIAND(VU1); }
void recVU1MI_ISUBIU()  { rec_vuISUBIU(VU1); } 

#endif

#ifdef RECOMPILE_VUMI_BRANCH

void recVU1MI_IBEQ()    { recVUMI_IBEQ(VU); } 
void recVU1MI_IBGEZ()   { recVUMI_IBGEZ(VU); } 
void recVU1MI_IBLTZ()   { recVUMI_IBLTZ(VU); } 
void recVU1MI_IBLEZ()   { recVUMI_IBLEZ(VU); } 
void recVU1MI_IBGTZ()   { recVUMI_IBGTZ(VU); } 
void recVU1MI_IBNE()    { recVUMI_IBNE(VU); } 
void recVU1MI_B()       { recVUMI_B(VU); } 
void recVU1MI_BAL()     { recVUMI_BAL(VU); } 
void recVU1MI_JR()      { recVUMI_JR(VU); } 
void recVU1MI_JALR()    { recVUMI_JALR(VU); } 

#else

void recVU1MI_IBEQ()    { rec_vuIBEQ(VU1); } 
void recVU1MI_IBGEZ()   { rec_vuIBGEZ(VU1); } 
void recVU1MI_IBGTZ()   { rec_vuIBGTZ(VU1); } 
void recVU1MI_IBLTZ()   { rec_vuIBLTZ(VU1); } 
void recVU1MI_IBLEZ()   { rec_vuIBLEZ(VU1); } 
void recVU1MI_IBNE()    { rec_vuIBNE(VU1); } 
void recVU1MI_B()       { rec_vuB(VU1); } 
void recVU1MI_BAL()     { rec_vuBAL(VU1); } 
void recVU1MI_JR()      { rec_vuJR(VU1); } 
void recVU1MI_JALR()    { rec_vuJALR(VU1); } 

#endif

