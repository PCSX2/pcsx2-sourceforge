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

// recompiler reworked to add dynamic linking zerofrog(@gmail.com) Jan06

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

#include "Common.h"
#include "Memory.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iR5900AritImm.h"
#include "iR5900Arit.h"
#include "iR5900MultDiv.h"
#include "iR5900Shift.h"
#include "iR5900Branch.h"
#include "iR5900Jump.h"
#include "iR5900LoadStore.h"
#include "iR5900Move.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCP0.h"
#include "iVUmicro.h"
#include "iVU0micro.h"
#include "iVU1micro.h"
#include "VU.h"
#include "VU0.h"


#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

typedef void (*R5900FNPTR)();

u32 maxrecmem = 0;
uptr *recLUT;

#define X86
#define CHECK_ESP()

#define USE_EE_FAST_CYCLES

void COUNT_CYCLES(u32 curpc)
{
#ifdef USE_EE_FAST_CYCLES
	MOV32ItoR(EAX, curpc);
	SUB32MtoR(EAX, (u32)&cpuRegs.pc);
	SHR32ItoR(EAX, 2);
	AND32ItoR(EAX, 0xffff);
	ADD32RtoM((u32)&cpuRegs.cycle, EAX);
	ADD32RtoM((u32)&cpuRegs.CP0.n.Count, EAX);
#endif
}

char *recMem;	   /* the recompiled blocks will be here */
BASEBLOCK *recRAM;	   /* and the ptr to the blocks here */
BASEBLOCK *recROM;	   /* and here */
BASEBLOCK *recROM1;	   /* also here */
char *recPtr;

void (*recBSC[64])();
void (*recSPC[64])();
void (*recREG[32])();
void (*recCP0[32])();
void (*recCP0BC0[32])();
void (*recCP0C0[64])();
void (*recCP1[32])();
void (*recCP1BC1[32])();
void (*recCP1S[64])();
void (*recCP1W[64])();
void (*recMMIt[64])();
void (*recMMI0t[32])();
void (*recMMI1t[32])();
void (*recMMI2t[32])();
void (*recMMI3t[32])();

u32 hasSSE;
u32 has3DNOW;
u32 has3DNOW2;
u32 hasMMX2;
u32 hasSSE2;

u32 g_SavedEsp = 0;


u32 startlog = 0;
u32 pc;			         /* recompiler pc */
int count;		         /* recompiler intruction count */
int branch;		         /* set for branch */
u64 _imm;		         /* temp immediate */
u64 _sa;			         /* temp sa */
BOOL bExecBIOS = FALSE;

#ifdef __x86_64__
char *txt0 = "RAX = %x : RDX = %x : RCX = %x\n";
char *txt0RC = "EAX = %x : EBX = %x : ECX = %x : EDX = %x : ESI = %x : EDI = %x\n";
char *txt1 = "REG[%d] = %x_%x\n";
char *txt2 = "M32 = %x\n";
#else
char *txt0 = "EAX = %x : ECX = %x : EDX = %x\n";
char *txt0RC = "EAX = %x : EBX = %x : ECX = %x : EDX = %x : ESI = %x : EDI = %x\n";
char *txt1 = "REG[%d] = %x_%x\n";
char *txt2 = "M32 = %x\n";
#endif

static void iBranchTest(u32 newpc);
static void recRecompile( u32 startpc );
void recCOP22( void );


////////////////////////////////////////////////////
/* uses ECX, result in ECX */
void ExtSign32to64( int reg ) 
{
	XOR32RtoR( ECX, ECX );
	TEST32ItoR( reg, 0x80000000 );
	j8Ptr[ 7 ] = JZ8( 0 );
	MOV32ItoR( ECX, 0xffffffff );
	x86SetJ8( j8Ptr[ 7 ] );
}

////////////////////////////////////////////////////
void iRet( BOOL freeRegs ) {
	/* store cycle */
	iFlushCall();
#ifdef __x86_64__
	ADD64ItoR(ESP, 8);
#endif
	RET( );
}

////////////////////////////////////////////////////
void LogX86( void ) {
#ifdef __x86_64__
	SUB64ItoR(RSP, 15*8);
	MOV32RtoRm(RSP, RAX);
	MOV32RtoRm(RSP, RBX);
	MOV32RtoRm(RSP, RCX);
	MOV32RtoRm(RSP, RDX);
	MOV32RtoRm(RSP, RSI);
	MOV32RtoRm(RSP, RDI);
	MOV32RtoRm(RSP, RBP);
	MOV32RtoRm(RSP, R8 );
	MOV32RtoRm(RSP, R9 );
	MOV32RtoRm(RSP, R10);
	MOV32RtoRm(RSP, R11);
	MOV32RtoRm(RSP, R12);
	MOV32RtoRm(RSP, R13);
	MOV32RtoRm(RSP, R14);
	MOV32RtoRm(RSP, R15);

	MOV64ItoR(RSI, RDX);
	MOV64MtoR(RDI, (u64)&txt2);
	CALLFunc( (u32)SysPrintf );

	MOV32RmtoR(RAX, RSP);
	MOV32RmtoR(RBX, RSP);
	MOV32RmtoR(RCX, RSP);
	MOV32RmtoR(RDX, RSP);
	MOV32RmtoR(RSI, RSP);
	MOV32RmtoR(RDI, RSP);
	MOV32RmtoR(RBP, RSP);
	MOV32RmtoR(R8 , RSP);
	MOV32RmtoR(R9 , RSP);
	MOV32RmtoR(R10, RSP);
	MOV32RmtoR(R11, RSP);
	MOV32RmtoR(R12, RSP);
	MOV32RmtoR(R13, RSP);
	MOV32RmtoR(R14, RSP);
	MOV32RmtoR(R15, RSP);
	ADD64ItoR(RSP, 15*8);
#else
	PUSHA32( );

	PUSH32R( EDX );
	PUSH32R( ECX );
	PUSH32R( EAX );
	PUSH32M( (u32)&txt0 );
	CALLFunc( (u32)SysPrintf );
	ADD32ItoR( ESP, 4 * 4 );

	POPA32( );
#endif
}

////////////////////////////////////////////////////
void LogM32( u32 mem ) {
	PUSH32M( mem );
	PUSH32M( (u32)&txt2 );
	CALLFunc( (u32)SysPrintf );
	ADD32ItoR( ESP, 4 * 2 );
}

////////////////////////////////////////////////////
void LogR64( int reg ) {
	PUSH32M( (u32)&cpuRegs.GPR.r[ reg ].UL[ 0 ] );
	PUSH32M( (u32)&cpuRegs.GPR.r[ reg ].UL[ 1 ] );
	PUSH32I( reg );
	PUSH32M( (u32)&txt1 );
	CALLFunc( (u32)SysPrintf );
	ADD32ItoR(ESP, 4 * 4);
}

////////////////////////////////////////////////////
void iDumpBlock( char * ptr ) {
	FILE *f;
	char command[ 256 ];
	char filename[ 256 ];
	u32 i;

	SysPrintf( "dump1 %x:%x, %x\n", cpuRegs.pc, pc, cpuRegs.cycle );
#ifdef __WIN32__
	CreateDirectory("dumps", NULL);
	sprintf( filename, "dumps\\dump%.8X_%s.txt", cpuRegs.pc, CHECK_REGCACHING ? "yes": "no" );
#else
	mkdir("dumps", 0755);
	sprintf( filename, "dumps/dump%.8X_%s.txt", cpuRegs.pc, CHECK_REGCACHING ? "yes": "no" );
#endif

	for ( i = cpuRegs.pc; i < pc; i += 4 ) {
		SysPrintf( "%s\n", disR5900Fasm( PSMu32( i ), i ) );
	}

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

	f = fopen( filename, "at" );
	fprintf( f, "\n\n" );
	for ( i = cpuRegs.pc; i < pc; i += 4 ) {
		fprintf( f, "%s\n", disR5900Fasm( PSMu32( i ), i ) );
	}
	fclose( f );
}


void iDumpRegisters( u32 _addr )
{
	FILE *f;
   char filename[ 256 ];

   sprintf( filename, "regdump\\r%.8X.%s", _addr, CHECK_REGCACHING ? "yes": "no" );
   f = fopen( filename, "wb" );
   fwrite( &cpuRegs, 1, sizeof( cpuRegs ), f );
   fclose( f );
}

void iLogCallOrder( u32 _addr )
{
	FILE *f;

   f = fopen( "regdump\\callorder.txt", "at" );
   fprintf( f, "%.8X\n", _addr );
   fflush( f );
   fclose( f );
}

#define TYPE_UNUSED 0
#define TYPE_TEMP	1
#define TYPE_GPRREG	2

typedef struct {
	int type;
	int gprreg;
	int mode;
	int needed;
} _x86regs;

#ifdef __x86_64__
#define X86REGS 16
#else
#define X86REGS 8
#endif

_x86regs x86regs[X86REGS];

void _initX86regs() {
	memset(x86regs, 0, sizeof(x86regs));
	x86regs[EBX].needed = 2;
	x86regs[EBP].needed = 2;
	x86regs[ESP].needed = 2;
}

int  _getFreeX86reg() {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].type == TYPE_UNUSED) return i;
	}
	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].needed) continue;
		if (x86regs[i].type != TYPE_TEMP) continue;

		_freeX86reg(i);
		return i;
	}
	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].needed) continue;

		_freeX86reg(i);
		return i;
	}
	SysPrintf("*PCSX2*: X86 ERROR\n");

	return -1;
}

int _allocTempX86reg(int x86reg) {
	if (x86reg == -1) {
		x86reg = _getFreeX86reg();
	}

	x86regs[x86reg].type = TYPE_TEMP;
	x86regs[x86reg].needed = 1;

//SysPrintf("_allocTempX86reg => %d\n", x86reg);
	return x86reg;
}

int _allocGPRtoX86reg(int x86reg, int gprreg, int mode) {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].type != TYPE_GPRREG) continue;
		if (x86regs[i].gprreg != gprreg) continue;

		x86regs[i].needed = 1;
		x86regs[i].mode|= mode;
//SysPrintf("_allocVFtoX86reg => %d (inuse)\n", i);
		return i;
	}

	if (x86reg == -1) {
		x86reg = _getFreeX86reg();
	}

	x86regs[x86reg].type = TYPE_GPRREG;
	x86regs[x86reg].gprreg = gprreg;
	x86regs[x86reg].mode = mode;
	x86regs[x86reg].needed = 1;
	if (mode & MODE_READ) {
		MOV64MtoR(x86reg, (uptr)&cpuRegs.GPR.r[x86regs[x86reg].gprreg].UD[0]);
	}

//SysPrintf("_allocGPRtoX86reg => %d\n", x86reg);
	return x86reg;
}

void _addNeededGPRtoX86reg(int gprreg) {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].type != TYPE_GPRREG) continue;
		if (x86regs[i].gprreg != gprreg) continue;

		x86regs[i].needed = 1;
	}
}

void _clearNeededX86regs() {
	int i;

	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].needed == 2) continue;
		x86regs[i].needed = 0;
	}
}

void _freeX86reg(int x86reg) {
	if (x86regs[x86reg].type == TYPE_GPRREG &&
		x86regs[x86reg].mode & MODE_WRITE) {
		MOV64RtoM((uptr)&cpuRegs.GPR.r[x86regs[x86reg].gprreg].UD[0], x86reg);

	}
	x86regs[x86reg].type = TYPE_UNUSED;
}

void _freeX86regs() {
	int i;

//SysPrintf("_freeX86regs\n");
	for (i=0; i<X86REGS; i++) {
		if (x86regs[i].type == TYPE_UNUSED) continue;
		if (x86regs[i].needed == 2) continue;

		_freeX86reg(i);
	}
}


#define VU_VFx_ADDR(x)  (u32)&VU->VF[x].UL[0]
#define VU_ACCx_ADDR    (u32)&VU->ACC.UL[0]

_xmmregs xmmregs[XMMREGS];

void _initXMMregs() {
	memset(xmmregs, 0, sizeof(xmmregs));
}

int  _getFreeXMMreg() {
	int i, tempi;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) return i;
	}
	tempi = -1;
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].needed) continue;
		if (xmmregs[i].type != XMMTYPE_TEMP) { tempi = i; continue; }

		_freeXMMreg(i);
		return i;
	}

	if( tempi != -1 ) {
		_freeXMMreg(tempi);
		return tempi;
	}
	SysPrintf("*PCSX2*: VUrec ERROR\n");

	return -1;
}

int _allocTempXMMreg(int xmmreg) {
	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_TEMP;
	xmmregs[xmmreg].needed = 1;

	return xmmreg;
}

int _allocVFtoXMMreg(VURegs *VU, int xmmreg, int vfreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0 || xmmregs[i].type != XMMTYPE_VFREG || xmmregs[i].reg != vfreg || xmmregs[i].VU != VU) continue;

		xmmregs[i].needed = 1;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ) ) {
			SSE_MOVAPS_M128_to_XMM(i, VU_VFx_ADDR(vfreg));
			xmmregs[i].mode |= MODE_READ;
		}

		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_VFREG;
	xmmregs[xmmreg].reg = vfreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].VU = VU;
	if (mode & MODE_READ) {
		SSE_MOVAPS_M128_to_XMM(xmmreg, VU_VFx_ADDR(xmmregs[xmmreg].reg));
	}

	return xmmreg;
}

int _checkVFtoXMMreg(VURegs* VU, int vfreg)
{
	int i;
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse && xmmregs[i].type == XMMTYPE_VFREG && xmmregs[i].reg == vfreg && xmmregs[i].VU == VU) {

			if( !(xmmregs[i].mode & MODE_READ) ) {
				SSE_MOVAPS_M128_to_XMM(i, VU_VFx_ADDR(vfreg));
				xmmregs[i].mode |= MODE_READ;
			}

			xmmregs[i].needed = 1;
			return i;
		}
	}

	return -1;
}

int _allocACCtoXMMreg(VURegs *VU, int xmmreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_ACC) continue;
		if (xmmregs[i].VU != VU) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVAPS_M128_to_XMM(i, VU_ACCx_ADDR);
			xmmregs[i].mode |= MODE_READ;
		}

		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_ACC;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	xmmregs[xmmreg].VU = VU;
	if (mode & MODE_READ) {
		SSE_MOVAPS_M128_to_XMM(xmmreg, VU_ACCx_ADDR);
	}

	return xmmreg;
}

int _allocFPtoXMMreg(int xmmreg, int fpreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPREG) continue;
		if (xmmregs[i].reg != fpreg) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVSS_M32_to_XMM(i, (u32)&fpuRegs.fpr[xmmregs[xmmreg].reg].f);
			xmmregs[i].mode |= MODE_READ;
		}

		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_FPREG;
	xmmregs[xmmreg].reg = fpreg;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	if (mode & MODE_READ) {
		SSE_MOVSS_M32_to_XMM(xmmreg, (u32)&fpuRegs.fpr[xmmregs[xmmreg].reg].f);
	}

	return xmmreg;
}

int _allocFPACCtoXMMreg(int xmmreg, int mode) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPACC) continue;

		if( !(xmmregs[i].mode & MODE_READ) && (mode&MODE_READ)) {
			SSE_MOVSS_M32_to_XMM(i, (u32)&fpuRegs.ACC.f);
			xmmregs[i].mode |= MODE_READ;
		}

		xmmregs[i].needed = 1;
		xmmregs[i].mode|= mode;
		return i;
	}

	if (xmmreg == -1) {
		xmmreg = _getFreeXMMreg();
	}

	xmmregs[xmmreg].inuse = 1;
	xmmregs[xmmreg].type = XMMTYPE_FPACC;
	xmmregs[xmmreg].mode = mode;
	xmmregs[xmmreg].needed = 1;
	if (mode & MODE_READ) {
		SSE_MOVSS_M32_to_XMM(xmmreg, (u32)&fpuRegs.ACC.f);
	}

	return xmmreg;
}

void _addNeededVFtoXMMreg(int vfreg) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_VFREG) continue;
		if (xmmregs[i].reg != vfreg) continue;

		xmmregs[i].needed = 1;
	}
}

void _addNeededACCtoXMMreg() {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_ACC) continue;

		xmmregs[i].needed = 1;
	}
}

void _addNeededFPtoXMMreg(int fpreg) {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPREG) continue;
		if (xmmregs[i].reg != fpreg) continue;

		xmmregs[i].needed = 1;
	}
}

void _addNeededFPACCtoXMMreg() {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;
		if (xmmregs[i].type != XMMTYPE_FPACC) continue;

		xmmregs[i].needed = 1;
	}
}

void _flushVF_XMMReg(VURegs* VU, int vfreg)
{
	int i;
	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse && xmmregs[i].VU == VU && xmmregs[i].type == XMMTYPE_VFREG && xmmregs[i].reg == vfreg) {
			SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(vfreg), i);
			xmmregs[i].inuse = 0;
			return;
		}
	}
}

void _clearNeededXMMregs() {
	int i;

	for (i=0; i<XMMREGS; i++) {

		if( xmmregs[i].needed ) {

			// setup read to any just written regs
			if( xmmregs[i].inuse && (xmmregs[i].mode&MODE_WRITE) )
				xmmregs[i].mode |= MODE_READ;
			xmmregs[i].needed = 0;
		}
	}
}

void _freeXMMreg(int xmmreg) {
	if (!xmmregs[xmmreg].inuse) return;

	if (xmmregs[xmmreg].type == XMMTYPE_VFREG &&
		(xmmregs[xmmreg].mode & MODE_WRITE) ) {
		VURegs *VU = xmmregs[xmmreg].VU;
		SSE_MOVAPS_XMM_to_M128(VU_VFx_ADDR(xmmregs[xmmreg].reg), xmmreg);
	}
	if (xmmregs[xmmreg].type == XMMTYPE_ACC &&
		(xmmregs[xmmreg].mode & MODE_WRITE) ) {
		VURegs *VU = xmmregs[xmmreg].VU;
		SSE_MOVAPS_XMM_to_M128(VU_ACCx_ADDR, xmmreg);
	}
	if (xmmregs[xmmreg].type == XMMTYPE_FPREG &&
		(xmmregs[xmmreg].mode & MODE_WRITE)) {
		SSE_MOVSS_XMM_to_M32((u32)&fpuRegs.fpr[xmmregs[xmmreg].reg], xmmreg);
	}
	if (xmmregs[xmmreg].type == XMMTYPE_FPACC &&
		(xmmregs[xmmreg].mode & MODE_WRITE)) {
		SSE_MOVSS_XMM_to_M32((u32)&fpuRegs.ACC.f, xmmreg);
	}
	xmmregs[xmmreg].inuse = 0;
}

void _freeXMMregs() {
	int i;

	for (i=0; i<XMMREGS; i++) {
		if (xmmregs[i].inuse == 0) continue;

		_freeXMMreg(i);
	}
}


////////////////////////////////////////////////////
int recInit( void ) 
{
	int i;

	remove( "callorder.txt" );

	recLUT = (uptr*) _aligned_malloc( 0x010000 * sizeof(uptr), 16 );
	memset( recLUT, 0, 0x010000 * sizeof(uptr) );
	recMem = SysMmap(0, 0x00800000);
	recRAM = (BASEBLOCK*) _aligned_malloc( sizeof(BASEBLOCK)/4*0x02000000 , 16);
	recROM = (BASEBLOCK*) _aligned_malloc( sizeof(BASEBLOCK)/4*0x00400000 , 16);
	recROM1= (BASEBLOCK*) _aligned_malloc( sizeof(BASEBLOCK)/4*0x00040000 , 16);

	if ( ( recRAM == NULL ) || ( recROM    == NULL ) || ( recROM1   == NULL ) ||
		  ( recMem == NULL ) || ( recLUT    == NULL ) ) 
   {
		SysMessage( _( "Error allocating memory" ) ); 
      return -1;
	}

	for ( i = 0x0000; i < 0x0200; i++ )
	{
		recLUT[ i + 0x0000 ] = (uptr)&recRAM[ i << 14 ];
		recLUT[ i + 0x2000 ] = (uptr)&recRAM[ i << 14 ];
		recLUT[ i + 0x3000 ] = (uptr)&recRAM[ i << 14 ];
	}

	for ( i = 0x0000; i < 0x0040; i++ )
	{
		recLUT[ i + 0x1fc0 ] = (uptr)&recROM[ i << 14 ];
		recLUT[ i + 0x9fc0 ] = (uptr)&recROM[ i << 14 ];
		recLUT[ i + 0xbfc0 ] = (uptr)&recROM[ i << 14 ];
	}

	for ( i = 0x0000; i < 0x0004; i++ )
	{
		recLUT[ i + 0x1e00 ] = (uptr)&recROM1[ i << 14 ];
		recLUT[ i + 0x9e00 ] = (uptr)&recROM1[ i << 14 ];
		recLUT[ i + 0xbe00 ] = (uptr)&recROM1[ i << 14 ];
	}

	memcpy( recLUT + 0x8000, recLUT, 0x2000 * sizeof(uptr) );
	memcpy( recLUT + 0xa000, recLUT, 0x2000 * sizeof(uptr) );
	
	memset(recMem, 0xcd, 0x00800000);

	SysPrintf( "x86Init: \n" );
	SysPrintf( "\tCPU vender name =  %s\n", cpuinfo.x86ID );
	SysPrintf( "\tFamilyID	=  %x\n", cpuinfo.x86StepID );
	SysPrintf( "\tx86Family =  %s\n", cpuinfo.x86Fam );
	SysPrintf( "\tCPU speed =  %d MHZ\n", cpuinfo.cpuspeed);
	SysPrintf( "\tx86PType  =  %s\n", cpuinfo.x86Type );
	SysPrintf( "\tx86Flags  =  %8.8x\n", cpuinfo.x86Flags );
	SysPrintf( "\tx86EFlags =  %8.8x\n", cpuinfo.x86EFlags );
   SysPrintf( "Features: \n" );
   SysPrintf( "\t%sDetected MMX\n",    cpucaps.hasMultimediaExtensions     ? "" : "Not " );
   SysPrintf( "\t%sDetected SSE\n",    cpucaps.hasStreamingSIMDExtensions  ? "" : "Not " );
   SysPrintf( "\t%sDetected SSE2\n",   cpucaps.hasStreamingSIMD2Extensions ? "" : "Not " );
	if ( cpuinfo.x86ID[0] == 'A' ) //AMD cpu
	{
      SysPrintf( " Extented AMD Features: \n" );
      SysPrintf( "\t%sDetected MMX2\n",     cpucaps.hasMultimediaExtensionsExt       ? "" : "Not " );
      SysPrintf( "\t%sDetected 3DNOW\n",    cpucaps.has3DNOWInstructionExtensions    ? "" : "Not " );
      SysPrintf( "\t%sDetected 3DNOW2\n",   cpucaps.has3DNOWInstructionExtensionsExt ? "" : "Not " );
	}
	if ( !( cpucaps.hasMultimediaExtensions  ) )
   {
		SysMessage( _( "Processor doesn't supports MMX, can't run recompiler without that" ) );
		return -1;
	}
		
	x86FpuState = FPU_STATE;

	return 0;
}

////////////////////////////////////////////////////
void recReset( void ) {
	SysPrintf("EE Recompiler data reset\n");

	maxrecmem = 0;
	memset( recRAM,  0, sizeof(BASEBLOCK)/4*0x02000000 );
	memset( recROM,  0, sizeof(BASEBLOCK)/4*0x00400000 );
	memset( recROM1, 0, sizeof(BASEBLOCK)/4*0x00040000 );

	recPtr = recMem;
	x86FpuState = FPU_STATE;
	iCWstate = 0;

	recResetVU0();
	recResetVU1();
	branch = 0;
	_imm = 0;
	_sa  = 0;
}

////////////////////////////////////////////////////
void recError( void ) {
	SysReset( );
	ClosePlugins( );
	SysMessage( _( "Unrecoverable error while running recompiler" ) );
	SysRunGui( );
}

void recGetNextCode() {
	
}

void _test() {
	Int_OpcodePrintTable[cpuRegs.code >> 26]();
}

////////////////////////////////////////////////////

extern u32 g_nextBranchCycle;

////////////////////////////////////////////////////
/* set a pending branch */
static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCK* s_pDispatchBlock = NULL;

static u8* s_ptempfnptr;

u32 g_lastpc = 0;

// jumped to when invalid pc address
__declspec(naked,noreturn) void Dispatcher()
{
	// EDX contains the current pc to jump to, stack contains the jump addr to modify
	__asm push edx

	// calc PC_GETBLOCK
	s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);

#ifdef _DEBUG
	g_lastpc = cpuRegs.pc;

	if( startlog ) {
		__Log("dis: %x\n", cpuRegs.pc);
	}
#endif

	__asm {

		mov eax, s_pDispatchBlock

		mov eax, dword ptr [eax]
		test eax, 0x0fffffff
		jnz CheckPtr
		push cpuRegs.pc // pc
		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]
CheckPtr:
		test eax, 0x40000000 // BLOCKTYPE_NEEDCLEAR
		jz Done
		// move new pc
		and eax, 0x0fffffff
		mov ecx, cpuRegs.pc
		mov dword ptr [eax+1], ecx
Done:
		and eax, 0x0fffffff
		mov edx, eax
		pop ecx // x86Ptr to mod
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

__declspec(naked,noreturn) void DispatcherClear()
{
	// EDX contains the current pc
	__asm mov cpuRegs.pc, edx
	__asm push edx

#ifdef _DEBUG
	g_lastpc = cpuRegs.pc;
#endif
	//__Log("disclear: %x\n", cpuRegs.pc);

	// calc PC_GETBLOCK
	s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);

	if( s_pDispatchBlock->Type != BLOCKTYPE_NEEDCLEAR ) {
		// already modded the code, jump to the new place
		__asm {
			pop edx
			mov eax, s_pDispatchBlock
			mov eax, dword ptr [eax]
			and eax, 0x0fffffff
			jmp eax
		}
	}

	s_ptempfnptr = (u8*)s_pDispatchBlock->Fnptr + 6;
	s_pDispatchBlock->Fnptr = 0;

	__asm {

		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock

		mov eax, dword ptr [eax]
		mov ecx, s_ptempfnptr
		and eax, 0x0fffffff
		mov edx, eax
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

// called when jumping to variable pc address
__declspec(naked,noreturn) void DispatcherReg()
{
#ifdef _DEBUG
	if( startlog ) {
		__Log("dis: %x\n", cpuRegs.pc);
	}
#endif

	__asm {
		//s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);
		mov edx, cpuRegs.pc
		mov ecx, edx
	}

#ifdef _DEBUG
	__asm mov g_lastpc, edx
#endif

	__asm {
		shr edx, 14
		and edx, 0xfffffffc
		add edx, recLUT
		mov edx, dword ptr [edx]

		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax
		mov eax, dword ptr [edx]
		test eax, 0x0fffffff
		jz recomp

		and eax, 0x0fffffff
		jmp eax // fnptr

recomp:
		//if( !s_pDispatchBlock->fnptr ) recRecompile(cpuRegs.pc);
		sub esp, 8
		mov dword ptr [esp+4], edx
		mov dword ptr [esp], ecx
		call recRecompile
		mov edx, dword ptr [esp+4]
		add esp, 8
		
		mov eax, dword ptr [edx]
		and eax, 0x0fffffff
		jmp eax // fnptr
	}
}

// check for end of bios
void CheckForBIOSEnd()
{
	MOV32MtoR(EAX, (int)&cpuRegs.pc);

	CMP32ItoR(EAX, 0x00200008);
	j8Ptr[0] = JE8(0);

	CMP32ItoR(EAX, 0x00100008);
	j8Ptr[1] = JE8(0);

	// return
	j8Ptr[2] = JMP8(0);

	x86SetJ8( j8Ptr[0] );
	x86SetJ8( j8Ptr[1] );

	// bios end
	RET();

	x86SetJ8( j8Ptr[2] );
}

static int *s_pCode;

void SetBranchReg( u32 reg )
{
	branch = 1;

	CHECK_ESP();

	if( reg != 0xffffffff ) {
		s_pCode = (int *)PSM( pc );
		if (!s_pCode) recError( );

		cpuRegs.code = *(int *)s_pCode;
		if (cpuRegs.code) {

			MOV32MtoR(ESI, (u32)&cpuRegs.GPR.r[ reg ].UL[ 0 ]);
			recompileNextInstruction(1);
			MOV32RtoM((int)&cpuRegs.pc, ESI);
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ reg ].UL[ 0 ] );
			MOV32RtoM( (int)&cpuRegs.pc, EAX );
			pc += 4;
		}
	}

	iFlushCall();

	iBranchTest(0xffffffff);
	if( bExecBIOS ) CheckForBIOSEnd();

	CHECK_ESP();

	JMP32((u32)DispatcherReg - ( (u32)x86Ptr + 5 ));
}

void SetBranchImm( u32 imm )
{
	u32* ptr;
	branch = 1;

	// end the current block
	MOV32ItoM( (u32)&cpuRegs.pc, imm );
	iFlushCall();

	iBranchTest(imm);
	if( bExecBIOS ) CheckForBIOSEnd();

	CHECK_ESP();

	MOV32ItoR(EDX, 0);
	ptr = (u32*)(x86Ptr-4);
	*ptr = (u32)JMP32((u32)Dispatcher - ( (u32)x86Ptr + 5 ));
}

////////////////////////////////////////////////////
#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code

static u32 g_SavedEBP = 0;
static void execute( void )
{
#ifdef _DEBUG
	u8* fnptr;
	u32 oldesi;
#endif
	BASEBLOCK* pblock = PC_GETBLOCK(cpuRegs.pc);

#ifdef _DEBUG
	if ( pblock == NULL ) {
		recError( ); 
		return; 
	}
#endif

	if ( !pblock->Fnptr) {
		recRecompile(cpuRegs.pc);
	}

	// skip the POPs
#ifdef _DEBUG
	fnptr = (u8*)pblock->Fnptr;
	__asm {
		// save data
		mov oldesi, esi
		push ebp

		mov g_SavedEsp, esp
		sub dword ptr [g_SavedEsp], 4

		call fnptr // jump into function
		// restore data
		pop ebp
		mov esi, oldesi
	}
#else
	__asm {
		mov g_SavedEBP, ebp
	}
	((R5900FNPTR)pblock->Fnptr)();
	__asm {
		mov ebp, g_SavedEBP
	}
#endif
}

////////////////////////////////////////////////////
void recStep( void ) {
}

////////////////////////////////////////////////////
void recExecute( void ) {
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);//ABOVE_NORMAL_PRIORITY_CLASS);
	for (;;)
		execute();
}

////////////////////////////////////////////////////
void recExecuteBlock( void ) {
	execute();
}

////////////////////////////////////////////////////
#define REC_CLEARM2(mem) { \
	BASEBLOCK* p = PC_GETBLOCK(mem); \
	if( GET_BLOCKTYPE(p) == BLOCKTYPE_DELAY ) \
		recClearMem(p-1, (mem)-4); \
	else if( *(u32*)p ) \
		recClearMem(p, mem); \
} \

void recClear32()
{
	u32 Addr;
	__asm mov Addr, ecx
	REC_CLEARM2(Addr);
}

void recClear64()
{
	BASEBLOCK* p;
	u32 Addr;
	__asm mov Addr, ecx

	p = PC_GETBLOCK(Addr);
	recClearMem(p-1, Addr-4);
	recClearMem(p, Addr);
	recClearMem(p+1, Addr+4);
}

void recClear128()
{
	BASEBLOCK* p;
	u32 Addr;
	__asm mov Addr, ecx

	p = PC_GETBLOCK(Addr);
	recClearMem(p-1, Addr-4);
	recClearMem(p, Addr);
	recClearMem(p+1, Addr+4);
	recClearMem(p+2, Addr+8);
	recClearMem(p+3, Addr+12);
}

void recClear( u32 Addr, u32 Size )
{
	u32 i;
	for(i = 0; i < Size; ++i, Addr+=4) {
		REC_CLEARM(Addr);
	}
}

void recClearMem(BASEBLOCK* p, u32 mem)
{
	assert( p != NULL );
	assert( p == PC_GETBLOCK(mem) );
	assert( (mem>>28) != 2 );

	if( p->Fnptr == 0 || (int)mem < 0 )
		return;

	x86Ptr = (u8*)p->Fnptr;

	// there is a small problem: mem can be orded with 0xa<<28 or 0x8<<28, and don't know which
	MOV32ItoR(EDX, mem);
	JMP32((u32)DispatcherClear - ( (u32)x86Ptr + 5 ));

	if( p->oldfnptr ) {
		// have to mod oldfnptr too
		x86Ptr = p->oldfnptr;

		MOV32ItoR(EDX, mem);
		JMP32((u32)DispatcherClear - ( (u32)x86Ptr + 5 ));
	}
	else
		p->oldfnptr = (u8*)p->Fnptr;

	if(GET_BLOCKTYPE(p) == BLOCKTYPE_BRANCH ) {
		
		// clear the next slot to not be delay anymore
		(p+1)->Type = 0;
		if( (p+1)->Fnptr ) 
			recClearMem(p+1, mem+4);
	}

	// don't clear fnptr!
	p->Type = BLOCKTYPE_NEEDCLEAR;
}

////////////////////////////////////////////////////
void recShutdown( void ) {
	if ( recMem == NULL ) {
		return;
	}
	_aligned_free( recLUT );
	SysMunmap(0x10000000, 0x00800000);
	_aligned_free( recRAM );
	_aligned_free( recROM );
	_aligned_free( recROM1 );

	x86Shutdown( );

   remove( "dump1.txt" );
   remove( "dump1" );
}

void recEnableVU0micro(int enable) {
}

void recEnableVU1micro(int enable) {
}


#ifndef EMMS_TRACE

void SetMMXstate() {
	EMMS();
}

void SetFPUstate() {
}

#else

void SetMMXstate() {
	x86FpuState = MMX_STATE;
}

void SetFPUstate() {
	if (x86FpuState==MMX_STATE) {
		if (has3DNOWInstructionExtensions) { FEMMS(); x86FpuState=FPU_STATE; }
		else          {  EMMS(); x86FpuState=FPU_STATE; }
	}
}

#endif

void iFlushCall()
{
	if (CHECK_REGCACHING) _freeX86regs();
	_freeXMMregs();
	LoadCW();
	SetFPUstate();
}

static u32 s_BranchCount = 0;

//#define BRANCH_SKIPPING
static void iBranchTest(u32 newpc)
{
#ifdef BRANCH_SKIPPING
	MOV32MtoR(ECX, (int)&s_BranchCount);
	INC32M((int)&s_BranchCount);
	TEST32ItoR(ECX, 32);
	j8Ptr[3] = JZ8( 0 );
#endif

	MOV32MtoR(ECX, (int)&cpuRegs.cycle);
	SUB32MtoR(ECX, (int)&g_nextBranchCycle);

	// check if should branch
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JL8( 0 );

	if( newpc == 0xffffffff ) 
		PUSH32M((int)&cpuRegs.pc);

	CALLFunc( (int)cpuBranchTest );

	if( newpc == 0xffffffff ) {
		POP32R(ECX);
		CMP32MtoR(ECX, (int)&cpuRegs.pc);
	}
	else {
		CMP32ItoM((int)&cpuRegs.pc, newpc);
	}

	j8Ptr[1] = JE8(0);

	RET();

	x86SetJ8( j8Ptr[0] );
	x86SetJ8( j8Ptr[1] );

#ifdef BRANCH_SKIPPING
	x86SetJ8( j8Ptr[3] );
#endif
}

static void checkcodefn()
{
	int pctemp;

	__asm mov pctemp, eax
	SysPrintf("code changed! %x\n", pctemp);
}

void recompileNextInstruction(int delayslot)
{
	u8* ptempptr = x86Ptr;
	BASEBLOCK* pCurBlock = PC_GETBLOCK(pc);

	if( !delayslot ) {
		
		if( pCurBlock->Type & BLOCKTYPE_NEEDCLEAR ) {
			// setup edx to point to real mem
			*(u32*)(pCurBlock->oldfnptr+1) = pc;
			pCurBlock->Type &= ~BLOCKTYPE_NEEDCLEAR;
			pCurBlock->Fnptr = 0;
		}

		if( pCurBlock->oldfnptr && pCurBlock->Fnptr ) {

			// code already in place, so jump to it and exit recomp
			JMP32((u32)pCurBlock->oldfnptr - ((u32)x86Ptr + 5));
			branch = 3;
			return;
		}
		if( pCurBlock->Fnptr ) {

			// code already in place, so jump to it and exit recomp
			JMP32((u32)pCurBlock->Fnptr - ((u32)x86Ptr + 5));
			branch = 3;
			return;
		}

		// don't erase delay slots
		if( GET_BLOCKTYPE(pCurBlock) != BLOCKTYPE_DELAY )
			pCurBlock->Type = 0;

		pCurBlock->Fnptr = (u32)x86Ptr;
	}
	else {

		// don't change fnptr
		pCurBlock->Type |= BLOCKTYPE_DELAY;
	}

	s_pCode = (int *)PSM( pc );
	if (!s_pCode) recError( );

	cpuRegs.code = *(int *)s_pCode;

#ifndef USE_EE_FAST_CYCLES
	INC32M( (int)&cpuRegs.cycle);
	INC32M( (int)&cpuRegs.CP0.n.Count);
#endif
	pc += 4;

	CHECK_ESP();
#ifdef _DEBUG
	CMP32ItoM((u32)s_pCode, cpuRegs.code);
	j8Ptr[0] = JE8(0);
	MOV32ItoR(EAX, pc);
	CALLFunc((u32)checkcodefn);
	x86SetJ8( j8Ptr[ 0 ] );
#endif

	recBSC[ cpuRegs.code >> 26 ]();

	if( x86Ptr-ptempptr < 10 ) {

		// insert nops
		switch(x86Ptr-ptempptr) {
			case 9: write8(0x90); break;
			case 8: write8(0x90); write8(0x90); break;
			default:
				JMP8(8-(int)(x86Ptr-ptempptr));
				x86Ptr = ptempptr+10;
		}
	}

	if( !delayslot && branch == 1 )
		pCurBlock->Type = BLOCKTYPE_BRANCH;
}

////////////////////////////////////////////////////
static void recNULL( void ) 
{
	SysPrintf("EE: Unimplemented op %x\n", cpuRegs.code);
}

////////////////////////////////////////////////////
static void recREGIMM( void ) 
{
	recREG[ _Rt_ ]( );
}

////////////////////////////////////////////////////
static void recSPECIAL( void )
{
	recSPC[ _Funct_ ]( );
}

////////////////////////////////////////////////////
static void recCOP0( void )
{
	recCP0[ _Rs_ ]( );
}

////////////////////////////////////////////////////
static void recCOP0BC0( void )
{
	recCP0BC0[ ( cpuRegs.code >> 16 ) & 0x03 ]( );
}

////////////////////////////////////////////////////
static void recCOP0C0( void ) 
{
	recCP0C0[ _Funct_ ]( );
}

////////////////////////////////////////////////////
static void recCOP1( void ) {
	recCP1[ _Rs_ ]( );
}



////////////////////////////////////////////////////
#ifndef CP2_RECOMPILE

REC_SYS(COP2);

#else

static void recCOP2( void )
{ 
#ifdef CPU_LOG
	CPU_LOG( "Recompiling COP2:%s\n", disR5900Fasm( cpuRegs.code, cpuRegs.pc ) );
#endif

   if ( !CHECK_COP2REC ) //disable the use of vus better this way :P
   {
		COUNT_CYCLES(pc);
		MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code ); 
		MOV32ItoM( (u32)&cpuRegs.pc, pc ); 
		iFlushCall();
		CALLFunc( (u32)COP2 ); 

		CMP32ItoM((int)&cpuRegs.pc, pc);
		j8Ptr[0] = JE8(0);
		JMP32((u32)DispatcherReg - ( (u32)x86Ptr + 5 ));
		x86SetJ8(j8Ptr[0]);
//		branch = 2; 
   }
   else
   {
      recCOP22( );
   }
}

#endif

////////////////////////////////////////////////////
static void recMMI( void ) 
{
	recMMIt[ _Funct_ ]( );
}


/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/

////////////////////////////////////////////////////
static void recSYSCALL( void ) {
	COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();

	CALLFunc( (u32)SYSCALL );

	CMP32ItoM((int)&cpuRegs.pc, pc);
	j8Ptr[0] = JE8(0);
	JMP32((u32)DispatcherReg - ( (u32)x86Ptr + 5 ));
	x86SetJ8(j8Ptr[0]);
	//branch = 2;
}

////////////////////////////////////////////////////
static void recBREAK( void ) {
	COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();
	
	CALLFunc( (u32)BREAK );

	CMP32ItoM((int)&cpuRegs.pc, pc);
	j8Ptr[0] = JE8(0);
	RET();
	x86SetJ8(j8Ptr[0]);
	//branch = 2;
}


/**********************************************************
*    UNHANDLED YET OPCODES
*
**********************************************************/

////////////////////////////////////////////////////
//REC_SYS(PREF);
////////////////////////////////////////////////////
//REC_SYS(MFSA);
////////////////////////////////////////////////////
//REC_SYS(MTSA);
////////////////////////////////////////////////////
REC_SYS(TGE);
////////////////////////////////////////////////////
REC_SYS(TGEU);
////////////////////////////////////////////////////
REC_SYS(TLT);
////////////////////////////////////////////////////
REC_SYS(TLTU);
////////////////////////////////////////////////////
REC_SYS(TEQ);
////////////////////////////////////////////////////
REC_SYS(TNE);
////////////////////////////////////////////////////
REC_SYS(TGEI);
////////////////////////////////////////////////////
REC_SYS(TGEIU);
////////////////////////////////////////////////////
REC_SYS(TLTI);
////////////////////////////////////////////////////
REC_SYS(TLTIU);
////////////////////////////////////////////////////
REC_SYS(TEQI);
////////////////////////////////////////////////////
REC_SYS(TNEI);
////////////////////////////////////////////////////
//REC_SYS(MTSAB);
////////////////////////////////////////////////////
//REC_SYS(MTSAH);


////////////////////////////////////////////////////
static void recCACHE( void ) {
	COUNT_CYCLES(pc);
	MOV32ItoM( (u32)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (u32)&cpuRegs.pc, pc );
	iFlushCall();

	CALLFunc( (u32)CACHE );
	//branch = 2;

	CMP32ItoM((int)&cpuRegs.pc, pc);
	j8Ptr[0] = JE8(0);
	RET();
	x86SetJ8(j8Ptr[0]);
}


static void recPREF( void ) 
{
}



////////////////////////////////////////////////////
static void recSYNC( void )
{
}

static void recMFSA( void ) 
{
	if (!_Rd_) return;
	MOV32MtoR(EAX, (u32)&cpuRegs.sa);
	MOV32RtoM((u32)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
	MOV32ItoM((u32)&cpuRegs.GPR.r[_Rd_].UL[1], 0);
}

static void recMTSA( void ) 
{
	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
	MOV32RtoM((u32)&cpuRegs.sa, EAX);
}
/*
void recTGE( void ) 
{
}

void recTGEU( void ) 
{
}

void recTLT( void ) 
{
}

void recTLTU( void ) 
{
}

void recTEQ( void ) 
{
}

void recTNE( void ) 
{
}

void recTGEI( void )
{
}

void recTGEIU( void ) 
{
}

void recTLTI( void ) 
{
}

void recTLTIU( void ) 
{
}

void recTEQI( void ) 
{
}

void recTNEI( void ) 
{
}

*/
static void recMTSAB( void ) 
{
	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
	AND32ItoR(EAX, 0xF);
	MOV32ItoR(ECX, _Imm_);
	AND32ItoR(ECX, 0xF);
	XOR32RtoR(EAX, ECX);
	SHL32ItoR(EAX, 3);
	MOV32RtoM((u32)&cpuRegs.sa, EAX);
}

static void recMTSAH( void ) 
{
	MOV32MtoR(EAX, (u32)&cpuRegs.GPR.r[_Rs_].UL[0]);
	AND32ItoR(EAX, 0x7);
	MOV32ItoR(ECX, _Imm_);
	AND32ItoR(ECX, 0x7);
	XOR32RtoR(EAX, ECX);
	SHL32ItoR(EAX, 4);
	MOV32RtoM((u32)&cpuRegs.sa, EAX);
}




void (*recBSC[64] )() = {
    recSPECIAL, recREGIMM, recJ,    recJAL,   recBEQ,  recBNE,  recBLEZ,  recBGTZ,
    recADDI,    recADDIU,  recSLTI, recSLTIU, recANDI, recORI,  recXORI,  recLUI,
    recCOP0,    recCOP1,   recCOP2, recNULL,  recBEQL, recBNEL, recBLEZL, recBGTZL,
    recDADDI,   recDADDIU, recLDL,  recLDR,   recMMI,  recNULL, recLQ,    recSQ,
    recLB,      recLH,     recLWL,  recLW,    recLBU,  recLHU,  recLWR,   recLWU,
    recSB,      recSH,     recSWL,  recSW,    recSDL,  recSDR,  recSWR,   recCACHE,
    recNULL,    recLWC1,   recNULL, recPREF,  recNULL, recNULL, recLQC2,  recLD,
    recNULL,    recSWC1,   recNULL, recNULL,  recNULL, recNULL, recSQC2,  recSD
};

void (*recSPC[64] )() = {
    recSLL,  recNULL,  recSRL,  recSRA,  recSLLV,    recNULL,  recSRLV,   recSRAV,
    recJR,   recJALR,  recMOVZ, recMOVN, recSYSCALL, recBREAK, recNULL,   recSYNC,
    recMFHI, recMTHI,  recMFLO, recMTLO, recDSLLV,   recNULL,  recDSRLV,  recDSRAV,
    recMULT, recMULTU, recDIV,  recDIVU, recNULL,    recNULL,  recNULL,   recNULL,
    recADD,  recADDU,  recSUB,  recSUBU, recAND,     recOR,    recXOR,    recNOR,
    recMFSA, recMTSA,  recSLT,  recSLTU, recDADD,    recDADDU, recDSUB,   recDSUBU,
    recTGE,  recTGEU,  recTLT,  recTLTU, recTEQ,     recNULL,  recTNE,    recNULL,
    recDSLL, recNULL,  recDSRL, recDSRA, recDSLL32,  recNULL,  recDSRL32, recDSRA32
};

void (*recREG[32] )() = {
    recBLTZ,   recBGEZ,   recBLTZL,   recBGEZL,   recNULL, recNULL, recNULL, recNULL,
    recTGEI,   recTGEIU,  recTLTI,    recTLTIU,   recTEQI, recNULL, recTNEI, recNULL,
    recBLTZAL, recBGEZAL, recBLTZALL, recBGEZALL, recNULL, recNULL, recNULL, recNULL,
    recMTSAB,  recMTSAH,  recNULL,    recNULL,    recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0[32] )() = {
    recMFC0,    recNULL, recNULL, recNULL, recMTC0, recNULL, recNULL, recNULL,
    recCOP0BC0, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
    recCOP0C0,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
    recNULL,    recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0BC0[32] )() = {
    recBC0F, recBC0T, recBC0FL, recBC0TL, recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
};

void (*recCP0C0[64] )() = {
    recNULL, recTLBR, recTLBWI, recNULL, recNULL, recNULL, recTLBWR, recNULL,
    recTLBP, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recERET, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recNULL, recNULL, recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
    recEI,   recDI,   recNULL,  recNULL, recNULL, recNULL, recNULL,  recNULL,
};

void (*recCP1[32] )() = {
    recMFC1,     recNULL, recCFC1, recNULL, recMTC1,   recNULL, recCTC1, recNULL,
    recCOP1_BC1, recNULL, recNULL, recNULL, recNULL,   recNULL, recNULL, recNULL,
    recCOP1_S,   recNULL, recNULL, recNULL, recCOP1_W, recNULL, recNULL, recNULL,
    recNULL,     recNULL, recNULL, recNULL, recNULL,   recNULL, recNULL, recNULL,
};

void (*recCP1BC1[32] )() = {
    recBC1F, recBC1T, recBC1FL, recBC1TL, recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
    recNULL, recNULL, recNULL,  recNULL,  recNULL, recNULL, recNULL, recNULL,
};

void (*recCP1S[64] )() = {
	recADD_S,  recSUB_S,  recMUL_S,  recDIV_S, recSQRT_S, recABS_S,  recMOV_S,   recNEG_S, 
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL,   
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recRSQRT_S, recNULL,  
	recADDA_S, recSUBA_S, recMULA_S, recNULL,  recMADD_S, recMSUB_S, recMADDA_S, recMSUBA_S,
	recNULL,   recNULL,   recNULL,   recNULL,  recCVT_W,  recNULL,   recNULL,    recNULL, 
	recMAX_S,  recMIN_S,  recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL, 
	recC_F,    recNULL,   recC_EQ,   recNULL,  recC_LT,   recNULL,   recC_LE,    recNULL, 
	recNULL,   recNULL,   recNULL,   recNULL,  recNULL,   recNULL,   recNULL,    recNULL, 
};
 
void (*recCP1W[64] )() = { 
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   	
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recCVT_S, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,   
	recNULL,  recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
};

void (*recMMIt[64] )() = {
	 recMADD,  recMADDU,  recNULL,  recNULL,  recPLZCW, recNULL, recNULL,  recNULL,
    recMMI0,  recMMI2,   recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recMFHI1, recMTHI1,  recMFLO1, recMTLO1, recNULL,  recNULL, recNULL,  recNULL,
    recMULT1, recMULTU1, recDIV1,  recDIVU1, recNULL,  recNULL, recNULL,  recNULL,
    recMADD1, recMADDU1, recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recMMI1 , recMMI3,   recNULL,  recNULL,  recNULL,  recNULL, recNULL,  recNULL,
    recPMFHL, recPMTHL,  recNULL,  recNULL,  recPSLLH, recNULL, recPSRLH, recPSRAH,
    recNULL,  recNULL,   recNULL,  recNULL,  recPSLLW, recNULL, recPSRLW, recPSRAW,
};

void (*recMMI0t[32] )() = { 
	recPADDW,  recPSUBW,  recPCGTW,  recPMAXW,       
	recPADDH,  recPSUBH,  recPCGTH,  recPMAXH,        
	recPADDB,  recPSUBB,  recPCGTB,  recNULL,
	recNULL,   recNULL,   recNULL,   recNULL,
	recPADDSW, recPSUBSW, recPEXTLW, recPPACW,        
	recPADDSH, recPSUBSH, recPEXTLH, recPPACH,        
	recPADDSB, recPSUBSB, recPEXTLB, recPPACB,        
	recNULL,   recNULL,   recPEXT5,  recPPAC5,
};

void (*recMMI1t[32] )() = { 
	recNULL,   recPABSW,  recPCEQW,  recPMINW, 
	recPADSBH, recPABSH,  recPCEQH,  recPMINH, 
	recNULL,   recNULL,   recPCEQB,  recNULL, 
	recNULL,   recNULL,   recNULL,   recNULL, 
	recPADDUW, recPSUBUW, recPEXTUW, recNULL,  
	recPADDUH, recPSUBUH, recPEXTUH, recNULL, 
	recPADDUB, recPSUBUB, recPEXTUB, recQFSRV, 
	recNULL,   recNULL,   recNULL,   recNULL, 
};

void (*recMMI2t[32] )() = { 
	recPMADDW, recNULL,   recPSLLVW, recPSRLVW, 
	recPMSUBW, recNULL,   recNULL,   recNULL,
	recPMFHI,  recPMFLO,  recPINTH,  recNULL,
	recPMULTW, recPDIVW,  recPCPYLD, recNULL,
	recPMADDH, recPHMADH, recPAND,   recPXOR, 
	recPMSUBH, recPHMSBH, recNULL,   recNULL, 
	recNULL,   recNULL,   recPEXEH,  recPREVH, 
	recPMULTH, recPDIVBW, recPEXEW,  recPROT3W, 
};

void (*recMMI3t[32] )() = { 
	recPMADDUW, recNULL,   recNULL,   recPSRAVW, 
	recNULL,    recNULL,   recNULL,   recNULL,
	recPMTHI,   recPMTLO,  recPINTEH, recNULL,
	recPMULTUW, recPDIVUW, recPCPYUD, recNULL,
	recNULL,    recNULL,   recPOR,    recPNOR,  
	recNULL,    recNULL,   recNULL,   recNULL,
	recNULL,    recNULL,   recPEXCH,  recPCPYH, 
	recNULL,    recNULL,   recPEXCW,  recNULL,
};

////////////////////////////////////////////////////
static void recRecompile( u32 startpc )
{
	//__Log("reg: %x\n", startpc);
	//if( startpc == 0x23c4f4) startlog = 1;

	/* if recPtr reached the mem limit reset whole mem */
	if ( ( (uptr)recPtr - (uptr)recMem ) >= 0x792000) {
		recReset();
	}

	x86SetPtr( recPtr );
	x86Align(32);

	branch = 0;
	//hascounted = 0;

	pc = startpc;
	x86FpuState = FPU_STATE;
	iCWstate = 0;
	if (CHECK_REGCACHING){ 
		_initX86regs();
		_initXMMregs();
	}

#ifdef __x86_64__
	SUB64ItoR(ESP, 8);
#endif

#ifdef _DEBUG
	// for debugging purposes
	MOV32ItoR(EAX, pc);
#endif

	while (!branch) {
		recompileNextInstruction(0);
	}

	//assert( branch == 3 || hascounted);

#ifdef __x86_64__
	ADD64ItoR(ESP, 8);
#endif

	if( branch == 2 ) {
		iFlushCall();

		iBranchTest(0xffffffff);
		if( bExecBIOS ) CheckForBIOSEnd();

		JMP32((u32)DispatcherReg - ( (u32)x86Ptr + 5 ));
	}

	if( !(pc&0x10000000) )
		maxrecmem = max( (pc&~0xa0000000), maxrecmem );

	assert( x86Ptr < recMem+0x00800000 );
	recPtr = x86Ptr;
}

R5900cpu recCpu = {
	recInit,
	recReset,
	recStep,
	recExecute,
	recExecuteBlock,
	recExecuteVU0Block,
	recExecuteVU1Block,
	recEnableVU0micro,
	recEnableVU1micro,
	recClear,
	recClearVU0,
	recClearVU1,
	recShutdown
};
