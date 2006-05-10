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

#ifdef __MSCW32__
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <malloc.h>

#include "PsxCommon.h"
#include "ix86/ix86.h"


#define ARITHMETICIMM_RECOMPILE
#define ARITHMETIC_RECOMPILE
#define MULTDIV_RECOMPILE
#define SHIFT_RECOMPILE
#define BRANCH_RECOMPILE
#define JUMP_RECOMPILE
#define LOADSTORE_RECOMPILE
#define SYS_RECOMPILE
#define MOVE_RECOMPILE
#define CP0_RECOMPILE
#define CP2_RECOMPILE

#define MAPBASE			0x48000000
#define RECMEM_SIZE		(8*1024*1024)

uptr *psxRecLUT;
int psxreclog = 0;

static u32 s_BranchCount = 0;
static char *recMem;	/* the recompiled blocks will be here */
static PSXBASEBLOCK *recRAM;	/* and the ptr to the blocks here */
static PSXBASEBLOCK *recROM;	/* and here */
static PSXBASEBLOCK *recROM1;	/* also here */
static char *recPtr;
static u32 pc;			/* recompiler pc */
static int branch;		/* set for branch */

static void (*recBSC[64])();
static void (*recSPC[64])();
static void (*recREG[32])();
static void (*recCP0[32])();
static void (*recCP2[64])();
static void (*recCP2BSC[32])();

u32 g_psxNextBranchCycle = 0;

static void iPsxBranchTest(u32 newpc);
static void recompileNextInstruction(int delayslot);

#define USE_PSX_FAST_CYCLES

void PSX_COUNT_CYCLES(u32 psxcurpc)
{
#ifdef USE_PSX_FAST_CYCLES
	MOV32ItoR(EAX, psxcurpc);
	SUB32MtoR(EAX, (u32)&psxRegs.pc);
	SHR32ItoR(EAX, 2);
	AND32ItoR(EAX, 0xffff);
	ADD32RtoM((u32)&psxRegs.cycle, EAX);
#endif
}

#define REC_FUNC(f) \
void psx##f(); \
static void rec##f() { \
	PSX_COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((u32)&psxRegs.pc, (u32)pc); \
	CALLFunc((u32)psx##f); \
/*	branch = 2; */\
}

#define REC_SYS(f) \
void psx##f(); \
static void rec##f() { \
	PSX_COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((u32)&psxRegs.pc, (u32)pc); \
	CALLFunc((u32)psx##f); \
	branch = 2; \
}

#define REC_BRANCH(f) \
void psx##f(); \
static void rec##f() { \
	PSX_COUNT_CYCLES(pc); \
	MOV32ItoM((u32)&psxRegs.code, (u32)psxRegs.code); \
	MOV32ItoM((u32)&psxRegs.pc, (u32)pc); \
	CALLFunc((u32)psx##f); \
	branch = 2; \
}

static void recRecompile(u32 startpc);

static int recInit() {
	int i;

	psxRecLUT = (uptr*) malloc(0x010000 * sizeof(uptr));
	memset(psxRecLUT, 0, 0x010000 * sizeof(uptr));

	recMem = SysMmap(0, RECMEM_SIZE);
	recRAM = (PSXBASEBLOCK*) _aligned_malloc(sizeof(PSXBASEBLOCK)/4*0x200000, 16);
	recROM = (PSXBASEBLOCK*) _aligned_malloc(sizeof(PSXBASEBLOCK)/4*0x400000, 16);
	recROM1= (PSXBASEBLOCK*) _aligned_malloc(sizeof(PSXBASEBLOCK)/4*0x040000, 16);
	if (recRAM == NULL || recROM == NULL || recROM1 == NULL ||
		recMem == NULL || psxRecLUT == NULL) {
		SysMessage(_("Error allocating memory")); return -1;
	}

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (uptr)&recRAM[(i & 0x1f) << 14];
	for (i=0; i<0x80; i++) psxRecLUT[i + 0x8000] = (uptr)&recRAM[(i & 0x1f) << 14];
	for (i=0; i<0x80; i++) psxRecLUT[i + 0xa000] = (uptr)&recRAM[(i & 0x1f) << 14];

	for (i=0; i<0x40; i++) psxRecLUT[i + 0x1fc0] = (uptr)&recROM[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0x9fc0] = (uptr)&recROM[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0xbfc0] = (uptr)&recROM[i << 14];

	for (i=0; i<0x40; i++) psxRecLUT[i + 0x1e00] = (uptr)&recROM1[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0x9e00] = (uptr)&recROM1[i << 14];
	for (i=0; i<0x40; i++) psxRecLUT[i + 0xbe00] = (uptr)&recROM1[i << 14];

	return 0;
}

static void recReset() {
	memset(recRAM, 0, sizeof(PSXBASEBLOCK)/4*0x200000);
	memset(recROM, 0, sizeof(PSXBASEBLOCK)/4*0x400000);
	memset(recROM1,0, sizeof(PSXBASEBLOCK)/4*0x040000);

	recPtr = recMem;
	branch = 0;
}

static void recShutdown() {
	if (recMem == NULL) return;
	free(psxRecLUT);
	SysMunmap((uptr)recMem, RECMEM_SIZE);
	_aligned_free(recRAM);
	_aligned_free(recROM);
	_aligned_free(recROM1);
	x86Shutdown();
}

static void recError() {
	SysReset();
	ClosePlugins();
	SysMessage(_("Unrecoverable error while running recompiler"));
	SysRunGui();
}

C_ASSERT( (int)&((PSXBASEBLOCK*)NULL)->fnptr == 0 );
C_ASSERT( (int)&((PSXBASEBLOCK*)NULL)->oldfnptr == 4 );

static PSXBASEBLOCK* s_pCurBlock = NULL;
static PSXBASEBLOCK* s_pDispatchBlock = NULL;
static u8* s_ptempfnptr;

// jumped to when invalid pc address
__declspec(naked,noreturn) void psxDispatcher()
{
	// EDX contains the current pc to jump to, stack contains the jump addr to modify
	__asm push edx

	// calc PC_GETBLOCK
	s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);

	__asm {

		mov eax, s_pDispatchBlock

		cmp dword ptr [eax], 0
		jne Done
		push psxRegs.pc // pc
		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock

Done:
		mov edx, eax
		add edx, 4
		mov edx, dword ptr [edx]
		cmp edx, 0
		je RegularPtr
		mov eax, edx
		jmp End
RegularPtr:
		mov eax, dword ptr [eax]
		mov edx, eax
End:
		pop ecx // x86Ptr to mod
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

__declspec(naked,noreturn) void psxDispatcherClear()
{
	// EDX contains the current pc
	__asm mov psxRegs.pc, edx
	__asm push edx

	// calc PC_GETBLOCK
	s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);
	assert( s_pDispatchBlock->fnptr );
	s_ptempfnptr = s_pDispatchBlock->fnptr + 6;
	s_pDispatchBlock->fnptr = 0;

	__asm {

		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock

		mov eax, dword ptr [eax]
		mov ecx, s_ptempfnptr
		mov edx, eax
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

// called when jumping to variable pc address
__declspec(naked,noreturn) void psxDispatcherReg()
{
	__asm {
		//s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);
		mov eax, psxRegs.pc
		mov edx, eax
		shr edx, 14
		and edx, 0xfffffffc
		add edx, psxRecLUT
		mov edx, dword ptr [edx]

		// edx += 3*(eax&0xffff)
		mov ecx, eax
		and ecx, 0xffff
		add edx, ecx
		add ecx, ecx
		add edx, ecx

		cmp dword ptr [edx], 0
		je recomp

setupcall:
		// jmp s_pDispatchBlock->oldfnptr ? s_pDispatchBlock->oldfnptr : s_pDispatchBlock->fnptr;
		cmp dword ptr [edx+4], 0
		je callfnptr
		add edx, 4 // oldfnptr

callfnptr:
		jmp dword ptr [edx] // fnptr

recomp:
		//if( !s_pDispatchBlock->fnptr ) recRecompile(cpuRegs.pc);
		sub esp, 8
		mov dword ptr [esp+4], edx
		mov dword ptr [esp], eax
		call recRecompile
		mov edx, dword ptr [esp+4]
		add esp, 8
		jmp setupcall
	}
}

extern void _psxTestInterrupts();

//#define PSXBRANCH_SKIPPING

static void iPsxBranchTest(u32 newpc)
{
#ifdef PSXBRANCH_SKIPPING
	MOV32MtoR(ECX, (int)&s_BranchCount);
	INC32M((int)&s_BranchCount);
	TEST32ItoR(ECX, 32);
	j8Ptr[3] = JZ8( 0 );
#endif

	MOV32MtoR(ECX, (int)&psxRegs.cycle);
	SUB32MtoR(ECX, (int)&g_psxNextBranchCycle);

	// check if should branch
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JL8( 0 );

	if( newpc == 0xffffffff ) 
		PUSH32M((int)&psxRegs.pc);

	CALLFunc((int)psxBranchTest);

	if( newpc == 0xffffffff ) {
		POP32R(ECX);
		CMP32MtoR(ECX, (int)&psxRegs.pc);
	}
	else
		CMP32ItoM((int)&psxRegs.pc, newpc);

	j8Ptr[1] = JE8(0);

	RET();
	x86SetJ8( j8Ptr[1] );

	// test if psx time span is up
	CMP32ItoM((int)&cpuRegs.EEsCycle, 0);
	j8Ptr[1] = JG8(0);

	RET();
	x86SetJ8( j8Ptr[1] );

	x86SetJ8( j8Ptr[0] );

#ifdef PSXBRANCH_SKIPPING
	x86SetJ8( j8Ptr[3] );
#endif
}

/*#define IOP_WAIT_CYCLE 32
static void iPsxBranchTest(u32 newpc)
{
	MOV32MtoR(ECX, (int)&psxRegs.cycle);
	SUB32MtoR(ECX, (int)&g_psxNextBranchCycle);

	// check if should branch
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JL8( 0 );

//	PUSH32I((int)s_BranchEvent);
//	CALLFunc((int)SetEvent);
//	PUSH32I((int)g_psxBranchThread);
//	CALLFunc((int)ResumeThread);
	//ADD32ItoR(ESP, 4);

	MOV32MtoR(EDX, (int)&psxRegs.cycle);
	MOV32RtoR(ECX, EDX);
	SUB32MtoR(ECX, (int)&cpuRegs.IOPoCycle);
	SHL32ItoR(ECX, 3);
	MOV32MtoR(EAX, (int)&cpuRegs.EEsCycle);
	SUB32RtoR(EAX, ECX);
	MOV32RtoM((int)&cpuRegs.IOPoCycle, EDX);
	MOV32RtoM((int)&cpuRegs.EEsCycle, EAX);

	//cpuRegs.EEsCycle -= (psxRegs.cycle - cpuRegs.IOPoCycle) << 3;
	//cpuRegs.IOPoCycle = psxRegs.cycle;

	CMP32ItoM((int)&s_nBranchTest, 0);
	j8Ptr[2] = JE8(0);

	if( newpc == 0xffffffff ) {
		PUSH32M((int)&psxRegs.pc);
	}
	else
		PUSH32I(newpc);

	CALLFunc((int)recR3000ABranchTest);
	ADD32ItoR(ESP, 4);
	x86SetJ8( j8Ptr[2] );

	CMP32ItoM((int)&cpuRegs.EEsCycle, 0);
	j8Ptr[1] = JG8(0);
	RET();

	x86SetJ8( j8Ptr[1] );

	ADD32ItoM((int)&g_psxNextBranchCycle, IOP_WAIT_CYCLE);
	x86SetJ8( j8Ptr[0] );
}*/

void psxRecClearMem(PSXBASEBLOCK* p, u32 mem)
{
	assert( p != NULL && p->fnptr != NULL );
	assert( p == PSX_GETBLOCK(mem) );

	x86Ptr = p->fnptr;

	MOV32ItoR(EDX, mem);
	JMP32((u32)psxDispatcherClear - ( (u32)x86Ptr + 5 ));

	if( p->oldfnptr ) {
		// have to mod oldfnptr too
		x86Ptr = p->oldfnptr;

		MOV32ItoR(EDX, mem);
		JMP32((u32)psxDispatcherClear - ( (u32)x86Ptr + 5 ));
	}
	else
		p->oldfnptr = p->fnptr;

	// don't clear fnptr!
	switch(PSX_BLOCKTYPE(p)) {
	
		case BLOCKTYPE_BRANCH:
			//p->fnptr = 0;
			p->type = 0;

			// clear the next slot to not be delay anymore
			(p+1)->type = 0;
			if( (p+1)->fnptr ) 
				psxRecClearMem(p+1, mem+4);

			break;

		default:
			//p->fnptr = 0;
			assert( p->type == 0 );
			p->type = 0;
	}
}

static void recClear(u32 Addr, u32 Size)
{
	u32 i;
	for(i = 0; i < Size; ++i, Addr+=4) {
		PSXREC_CLEARM(Addr);
	}
}

static int *s_pCode;

void psxSetBranchReg()
{
	branch = 1;

	recompileNextInstruction(1);
	PSX_COUNT_CYCLES(pc);
	POP32R( ECX );
	MOV32RtoM((int)&psxRegs.pc, ECX );
	iPsxBranchTest(0xffffffff);

	JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
}

void psxSetBranchImm( u32 imm )
{
	u32* ptr;
	assert(branch == 1);

	PSX_COUNT_CYCLES(pc);
	// end the current block
	MOV32ItoM( (u32)&psxRegs.pc, imm );
	iPsxBranchTest(imm);

	MOV32ItoR(EDX, 0);
	ptr = (u32*)(x86Ptr-4);
	*ptr = (u32)JMP32((u32)psxDispatcher - ( (u32)x86Ptr + 5 ));
}

#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code

typedef void (*R3000AFNPTR)();

static void execute()
{
	while (cpuRegs.EEsCycle > 0){
#ifdef _DEBUG
	u8* fnptr;
	u32 oldesi;
#endif
	PSXBASEBLOCK* pblock = PSX_GETBLOCK(psxRegs.pc);

#ifdef _DEBUG
	if ( pblock == NULL ) {
		recError( ); 
		return; 
	}
#endif

	if ( !pblock->fnptr) {
		recRecompile(psxRegs.pc);
	}

#ifdef _DEBUG
	if( psxreclog ) __Log("psxe: %x\n", psxRegs.pc);

	// skip the POPs
	fnptr = pblock->fnptr;
	__asm {

		// save data
		mov oldesi, esi
		push ebp

		call fnptr // jump into function
		// restore data
		pop ebp
		mov esi, oldesi
	}
#else
	((R3000AFNPTR)pblock->fnptr)();
#endif
	}
}

static void checkcodefn()
{
	int pctemp;

	__asm mov pctemp, eax
	SysPrintf("iop code changed! %x\n", pctemp);
}

static void recompileNextInstruction(int delayslot) {
	
	u8* ptempptr = x86Ptr;
	PSXBASEBLOCK* pCurBlock = PSX_GETBLOCK(pc);

	if( !delayslot ) {
		
		if( pCurBlock->oldfnptr && pCurBlock->fnptr ) {
			// code already in place, so jump to it and exit recomp
			JMP32((u32)pCurBlock->oldfnptr - ((u32)x86Ptr + 5));
			branch = 3;
			return;
		}
		if( pCurBlock->fnptr ) {
			// code already in place, so jump to it and exit recomp
			JMP32((u32)pCurBlock->fnptr - ((u32)x86Ptr + 5));
			branch = 3;
			return;
		}

		// don't erase delay slots
		if( pCurBlock->type != BLOCKTYPE_DELAY )
			pCurBlock->type = 0;

		pCurBlock->fnptr = x86Ptr;
	}
	else {

		// don't change fnptr
		pCurBlock->type = BLOCKTYPE_DELAY;
	}

	s_pCode = (int *)PSXM(pc);
	if (!s_pCode)
		recError();
	psxRegs.code = *s_pCode;

	pc += 4;
#ifndef USE_PSX_FAST_CYCLES
	INC32M((int)&psxRegs.cycle);
#endif

#ifdef _DEBUG
	CMP32ItoM((u32)s_pCode, psxRegs.code);
	j8Ptr[0] = JE8(0);
	MOV32ItoR(EAX, pc);
	CALLFunc((u32)checkcodefn);
	x86SetJ8( j8Ptr[ 0 ] );
#endif

	recBSC[ psxRegs.code >> 26 ]( );

	if( x86Ptr-ptempptr < 10 ) {

		switch(x86Ptr-ptempptr) {
			case 9: write8(0x90); break;
			case 8: write8(0x90); write8(0x90); break;
			default:
				JMP8(8-(int)(x86Ptr-ptempptr));
				x86Ptr = ptempptr+10;
		}
	}

	if( !delayslot && branch == 1 )
		pCurBlock->type = BLOCKTYPE_BRANCH;
}

static void DumpRegs() {
	int i, j;

	printf("%x %x\n", psxRegs.pc, psxRegs.cycle);
	for (i=0; i<4; i++) {
		for (j=0; j<8; j++)
			printf("%x ", psxRegs.GPR.r[j*i]);
		printf("\n");
	}
}

//static void recExecuteBios() {
//	while (psxRegs.pc != 0x80030000) {
//		execute();
//	}
//}

static void recExecute() {
	for (;;) execute();
}

static void recExecuteBlock() {
	 
	execute();
}

static void recNULL() {
	SysPrintf("recUNK: %8.8x\n", psxRegs.code);
}

/*********************************************************
* goes to opcodes tables...                              *
* Format:  table[something....]                          *
*********************************************************/

//REC_SYS(SPECIAL);
static void recSPECIAL() {
	recSPC[_Funct_]();
}

static void recREGIMM() {
	recREG[_Rt_]();
}

static void recCOP0() {
	recCP0[_Rs_]();
}

//REC_SYS(COP2);
static void recCOP2() {
	recCP2[_Funct_]();
}

static void recBASIC() {
	recCP2BSC[_Rs_]();
}

//end of Tables opcodes...

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/

#ifndef ARITHMETICIMM_RECOMPILE

REC_FUNC(ADDI);
REC_FUNC(ADDIU);
REC_FUNC(ANDI);
REC_FUNC(ORI);
REC_FUNC(XORI);
REC_FUNC(SLTI);
REC_FUNC(SLTIU);
REC_FUNC(LUI);

#else

static void recADDIU()  {
// Rt = Rs + Im
	if (!_Rt_) return;

	if (_Rs_) {
		if (_Rs_ == _Rt_) {
			ADD32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _Imm_);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (_Imm_) ADD32ItoR(EAX, _Imm_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	} else {
		MOV32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _Imm_);
	}
}

static void recADDI()  {
// Rt = Rs + Im
	recADDIU();
}

static void recSLTI() {
// Rt = Rs < Im (signed)
	if (!_Rt_) return;

    CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], (s32)_Imm_);
    SETL8R   (EAX);
    AND32ItoR(EAX, 0xff);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recSLTIU() {
// Rt = Rs < Im (unsigned)
	if (!_Rt_) return;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], (s32)_Imm_);
    SETB8R(EAX);
    AND32ItoR(EAX, 0xff);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recANDI() {
// Rt = Rs And Im
	if (!_Rt_) return;

	if (_Rs_ && _ImmU_) {
		if (_Rs_ == _Rt_) {
			AND32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			AND32ItoR(EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	} else {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
}

static void recORI() {
// Rt = Rs Or Im
	if (!_Rt_) return;

	if (_Rs_) {
		if (_Rs_ == _Rt_) {
			OR32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			if (_ImmU_) OR32ItoR (EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	} else {
		MOV32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
	}
}

static void recXORI() {
// Rt = Rs Xor Im
	if (!_Rt_) return;

	if (_Rs_) {
		if (_Rs_ == _Rt_) {
			XOR32ItoM((uptr)&psxRegs.GPR.r[_Rt_], _ImmU_);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			XOR32ItoR(EAX, _ImmU_);
			MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
		}
	} else {
		MOV32ItoR(EAX, _ImmU_ ^ 0);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
}

//end of * Arithmetic with immediate operand  

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/

static void recLUI()  {
// Rt = Imm << 16
	if (!_Rt_) return;

	MOV32ItoM((uptr)&psxRegs.GPR.r[_Rt_], psxRegs.code << 16);
}

#endif
//End of Load Higher .....


/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

#ifndef ARITHMETIC_RECOMPILE

REC_FUNC(ADD);
REC_FUNC(ADDU);
REC_FUNC(SUB);
REC_FUNC(SUBU);
REC_FUNC(AND);
REC_FUNC(OR);
REC_FUNC(XOR);
REC_FUNC(NOR);
REC_FUNC(SLT);
REC_FUNC(SLTU);

#else

static void recADDU() {
// Rd = Rs + Rt 
	if (!_Rd_) return;

	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		ADD32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	} else if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recADD() {
// Rd = Rs + Rt
	recADDU();
}

static void recSUBU() {
// Rd = Rs - Rt
	if (!_Rd_) return;

	if (_Rs_ || _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SUB32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}   

static void recSUB() {
// Rd = Rs - Rt
	recSUBU();
}   

static void recAND() {
// Rd = Rs And Rt
	if (!_Rd_) return;

	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		AND32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}   

static void recOR() {
// Rd = Rs Or Rt
	if (!_Rd_) return;

	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	} else if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}   

static void recXOR() {
// Rd = Rs Xor Rt
	if (!_Rd_) return;

	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		XOR32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		XOR32ItoR(EAX, 0);
	} else if (_Rt_) {
		XOR32RtoR(EAX, EAX);
		XOR32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recNOR() {
// Rd = Rs Nor Rt
	if (!_Rd_) return;

	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		OR32MtoR (EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		NOT32R   (EAX);
	} else if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		NOT32R   (EAX);
	} else if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		NOT32R   (EAX);
	} else {
		MOV32ItoR(EAX, ~0);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recSLT() {
// Rd = Rs < Rt (signed)
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
    SETL8R   (EAX);
    AND32ItoR(EAX, 0xff);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}  

static void recSLTU() { 
// Rd = Rs < Rt (unsigned)
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	SBB32RtoR(EAX, EAX);
	NEG32R   (EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

#endif
//End of * Register arithmetic

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/

#ifndef MULTDIV_RECOMPILE

REC_FUNC(MULT);
REC_FUNC(MULTU);
REC_FUNC(DIV);
REC_FUNC(DIVU);

#else

static void recMULT() {
// Lo/Hi = Rs * Rt (signed)

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	IMUL32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	MOV32ItoR(EAX, 3);
	ADD32RtoM((u32)&psxRegs.cycle, EAX);
}

static void recMULTU() {
// Lo/Hi = Rs * Rt (unsigned)

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	MUL32M   ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	MOV32ItoR(EAX, 3);
	ADD32RtoM((u32)&psxRegs.cycle, EAX);
}

static void recDIV() {
// Lo/Hi = Rs / Rt (signed)

	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JE8(0);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	CDQ();
	IDIV32R  (ECX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	x86SetJ8(j8Ptr[0]);

	MOV32ItoR(EAX, 36);
	ADD32RtoM((u32)&psxRegs.cycle, EAX);
}

static void recDIVU() {
// Lo/Hi = Rs / Rt (unsigned)

	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JE8(0);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	XOR32RtoR(EDX, EDX);
	DIV32R   (ECX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	x86SetJ8(j8Ptr[0]);
	MOV32ItoR(EAX, 36);
	ADD32RtoM((u32)&psxRegs.cycle, EAX);
}

#endif
//End of * Register mult/div & Register trap logic  

#ifndef LOADSTORE_RECOMPILE

REC_FUNC(LB);
REC_FUNC(LBU);
REC_FUNC(LH);
REC_FUNC(LHU);
REC_FUNC(LW);

REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);

#else

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);

static void recLB() {
// Rt = mem[Rs + Im] (signed)

#ifdef __x86_64__
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxMemRead8);
	if (_Rt_) {
		MOVSX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#else
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxMemRead8);
	if (_Rt_) {
		MOVSX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	ADD32ItoR(ESP, 4);
#endif
}

static void recLBU() {
// Rt = mem[Rs + Im] (unsigned)

#ifdef __x86_64__
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxMemRead8);
	if (_Rt_) {
		MOVZX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#else
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxMemRead8);
	if (_Rt_) {
		MOVZX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	ADD32ItoR(ESP, 4);
#endif
}

static void recLH() {
// Rt = mem[Rs + Im] (signed)

#ifdef __x86_64__
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxMemRead16);
	if (_Rt_) {
		MOVSX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#else
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxMemRead16);
	if (_Rt_) {
		MOVSX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	ADD32ItoR(ESP, 4);
#endif
}

static void recLHU() {
// Rt = mem[Rs + Im] (unsigned)

#ifdef __x86_64__
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxMemRead16);
	if (_Rt_) {
		MOVZX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#else
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxMemRead16);
	if (_Rt_) {
		MOVZX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	ADD32ItoR(ESP, 4);
#endif
}

static void recLW() {
// Rt = mem[Rs + Im] (unsigned)

#ifdef __x86_64__
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxMemRead32);
	if (_Rt_) {
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#else
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);

	TEST32ItoR(EAX, 0x10000000);
	j8Ptr[0] = JZ8(0);

	PUSH32R  (EAX);
	CALLFunc((int)psxMemRead32);
	if (_Rt_) {
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	ADD32ItoR(ESP, 4);

	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	// read from psM directly
	AND32ItoR(EAX, 0x0fffffff);
	ADD32ItoR(EAX, (u32)psxM);

	MOV32RmtoR( EAX, EAX );
	MOV32RtoM( (uptr)&psxRegs.GPR.r[_Rt_], EAX);

	x86SetJ8(j8Ptr[1]);
#endif
}

void psxRecMemWrite8(u32 mem, u8 value);
void psxRecMemWrite16(u32 mem, u16 value);
void psxRecMemWrite32(u32 mem, u32 value);


static void recSB() {
// mem[Rs + Im] = Rt

#ifdef __x86_64__
	MOV32MtoR(ESI, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxRecMemWrite8);
#else
	PUSH32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxRecMemWrite8);
	ADD32ItoR(ESP, 8);
#endif
}

static void recSH() {
// mem[Rs + Im] = Rt

#ifdef __x86_64__
	MOV32MtoR(ESI, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxRecMemWrite16);
#else
	PUSH32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxRecMemWrite16);
	ADD32ItoR(ESP, 8);
#endif
}

static void recSW() {
// mem[Rs + Im] = Rt

#ifdef __x86_64__
	MOV32MtoR(ESI, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EDI, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EDI, _Imm_);
	CALLFunc((int)psxRecMemWrite32);
#else
	PUSH32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(EAX, _Imm_);
	PUSH32R  (EAX);
	CALLFunc((int)psxRecMemWrite32);
	ADD32ItoR(ESP, 8);
#endif
}
/*
void recSWL() {
}

void recSWR() {
}
*/

#endif


#ifndef SHIFT_RECOMPILE

REC_FUNC(SLL);
REC_FUNC(SRL);
REC_FUNC(SRA);

REC_FUNC(SLLV);
REC_FUNC(SRLV);
REC_FUNC(SRAV);

#else

static void recSLL() {
// Rd = Rt << Sa
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SHL32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRL() {
// Rd = Rt >> Sa
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SHR32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

static void recSRA() {
// Rd = Rt >> Sa
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Sa_) SAR32ItoR(EAX, _Sa_);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	} else {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}


static void recSLLV() {
// Rd = Rt << Rs
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Rs_) {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
			SHL32CLtoR(EAX);
		}
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recSRLV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Rs_) {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
			SHR32CLtoR(EAX);
		}
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recSRAV() {
// Rd = Rt >> Rs
	if (!_Rd_) return;

	if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		if (_Rs_) {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
			SAR32CLtoR(EAX);
		}
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

#endif

#ifndef SYS_RECOMPILE

REC_SYS(SYSCALL);
REC_SYS(BREAK);

#else

static void recSYSCALL() {
	PSX_COUNT_CYCLES(pc);
#ifdef __x86_64__
	MOV32ItoM((uptr)&psxRegs.pc, pc - 4);
	MOV32ItoR(ESI, branch == 1 ? 1 : 0);
	MOV32ItoR(EDI, 0x20);
	CALLFunc ((int)psxException);
#else
	MOV32ItoR(EAX, pc - 4);
	MOV32RtoM((uptr)&psxRegs.pc, EAX);
	PUSH32I  (branch == 1 ? 1 : 0);
	PUSH32I  (0x20);
	CALLFunc ((int)psxException);
	ADD32ItoR(ESP, 8);
#endif

	if (!branch) branch = 2;
}

static void recBREAK() {
}

#endif

#ifndef MOVE_RECOMPILE

REC_FUNC(MFHI);
REC_FUNC(MTHI);
REC_FUNC(MFLO);
REC_FUNC(MTLO);

#else

static void recMFHI() {
// Rd = Hi
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.hi);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recMTHI() {
// Hi = Rs

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
}

static void recMFLO() {
// Rd = Lo
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.lo);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

static void recMTLO() {
// Lo = Rs

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
}

#endif

#ifndef JUMP_RECOMPILE

REC_BRANCH(J);
REC_BRANCH(JR);
REC_BRANCH(JAL);
REC_BRANCH(JALR);

#else

static void recJ() {
// j target
	u32 newpc = _Target_ * 4 + (pc & 0xf0000000);
	recompileNextInstruction(1);
	branch = 1;
	psxSetBranchImm(newpc);
}

static void recJAL() {
// jal target

	u32 newpc = _Target_ * 4 + (pc & 0xf0000000);
	MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc + 4);

	recompileNextInstruction(1);
	branch = 1;
	psxSetBranchImm(newpc);
}

static void recJR() {
// jr Rs

	PUSH32M((int)&psxRegs.GPR.r[_Rs_]);
	psxSetBranchReg();
}

static void recJALR() {
// jalr Rs

	PUSH32M((int)&psxRegs.GPR.r[_Rs_]);

	if (_Rd_) {
		MOV32ItoM((uptr)&psxRegs.GPR.r[_Rd_], pc + 4);
	}
	
	psxSetBranchReg();
}

#endif

#ifndef BRANCH_RECOMPILE

REC_BRANCH(BLTZ);
REC_BRANCH(BGTZ);
REC_BRANCH(BLTZAL);
REC_BRANCH(BGEZAL);
REC_BRANCH(BNE);
REC_BRANCH(BEQ);
REC_BRANCH(BLEZ);
REC_BRANCH(BGEZ);

#else

static void recBLTZ() {
// Branch if Rs < 0
	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JL32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);
	pc -= 4;
	recompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

static void recBGEZ() {
// Branch if Rs >= 0
	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JGE32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

static void recBLTZAL() {
// Branch if Rs < 0
	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JL32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

static void recBGEZAL() {
// Branch if Rs >= 0
	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JGE32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], pc);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

static void recBEQ() {
// Branch if Rs == Rt

	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	if (_Rs_ == _Rt_) {
		recompileNextInstruction(1);
		psxSetBranchImm(branchTo);
	} else {

		if (_Rs_) {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
			CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		} else {
			CMP32ItoM((uptr)&psxRegs.GPR.r[_Rt_], 0);
		}
		
		j32Ptr[16] = JE32(0);

		recompileNextInstruction(1);
		psxSetBranchImm(pc);

		x86SetJ32(j32Ptr[16]);

		pc -= 4;
		recompileNextInstruction(1);
		psxSetBranchImm(branchTo);
	}
}

static void recBNE() {
// Branch if Rs != Rt

	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		CMP32ItoM((int)&psxRegs.GPR.r[_Rt_], 0);
	}
	
	j32Ptr[16] = JNE32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

static void recBLEZ() {
// Branch if Rs <= 0

	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JLE32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

static void recBGTZ() {
// Branch if Rs > 0

	u32 branchTo = (s32)_Imm_ * 4 + pc;
	branch = 1;

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	j32Ptr[16] = JG32(0);

	recompileNextInstruction(1);
	psxSetBranchImm(pc);

	x86SetJ32(j32Ptr[16]);

	pc -= 4;
	recompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}
#endif

#ifndef CP0_RECOMPILE

REC_FUNC(MFC0);
REC_FUNC(MTC0);
REC_FUNC(CFC0);
REC_FUNC(CTC0);
REC_FUNC(RFE);

#else

static void recMFC0() {
// Rt = Cop0->Rd
	if (!_Rt_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.r[_Rd_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recCFC0() {
// Rt = Cop0->Rd
	if (!_Rt_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.r[_Rd_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

static void recMTC0() {
// Cop0->Rd = Rt

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.CP0.r[_Rd_], EAX);
}

static void recCTC0() {
// Cop0->Rd = Rt

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.CP0.r[_Rd_], EAX);
}

static void recRFE() {
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.n.Status);
	MOV32RtoR(ECX, EAX);
	AND32ItoR(EAX, 0xfffffff0);
	AND32ItoR(ECX, 0x3c);
	SHR32ItoR(ECX, 2);
	OR32RtoR (EAX, ECX);
	MOV32RtoM((uptr)&psxRegs.CP0.n.Status, EAX);
}

#endif
/*
#define CP2_FUNC(f) \
void gte##f(); \
static void rec##f() { \
	MOV32ItoM((uptr)&psxRegs.code, (int)psxRegs.code); \
	MOV32ItoM((uptr)&psxRegs.pc, (int)pc); \
	CALLFunc ((int)gte##f); \
/*	branch = 2; \
}

CP2_FUNC(MFC2);
CP2_FUNC(MTC2);
CP2_FUNC(CFC2);
CP2_FUNC(CTC2);
CP2_FUNC(LWC2);
CP2_FUNC(SWC2);

CP2_FUNC(RTPS);
CP2_FUNC(OP);
CP2_FUNC(NCLIP);
CP2_FUNC(DPCS);
CP2_FUNC(INTPL);
CP2_FUNC(MVMVA);
CP2_FUNC(NCDS);
CP2_FUNC(NCDT);
CP2_FUNC(CDP);
CP2_FUNC(NCCS);
CP2_FUNC(CC);
CP2_FUNC(NCS);
CP2_FUNC(NCT);
CP2_FUNC(SQR);
CP2_FUNC(DCPL);
CP2_FUNC(DPCT);
CP2_FUNC(AVSZ3);
CP2_FUNC(AVSZ4);
CP2_FUNC(RTPT);
CP2_FUNC(GPF);
CP2_FUNC(GPL);
CP2_FUNC(NCCT);
*/


static void (*recBSC[64])() = {
	recSPECIAL, recREGIMM, recJ   , recJAL  , recBEQ , recBNE , recBLEZ, recBGTZ,
	recADDI   , recADDIU , recSLTI, recSLTIU, recANDI, recORI , recXORI, recLUI ,
	recCOP0   , recNULL  , recCOP2, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	recLB     , recLH    , recLWL , recLW   , recLBU , recLHU , recLWR , recNULL,
	recSB     , recSH    , recSWL , recSW   , recNULL, recNULL, recSWR , recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL,
	recNULL   , recNULL  , recNULL, recNULL , recNULL, recNULL, recNULL, recNULL
};

static void (*recSPC[64])() = {
	recSLL , recNULL, recSRL , recSRA , recSLLV   , recNULL , recSRLV, recSRAV,
	recJR  , recJALR, recNULL, recNULL, recSYSCALL, recBREAK, recNULL, recNULL,
	recMFHI, recMTHI, recMFLO, recMTLO, recNULL   , recNULL , recNULL, recNULL,
	recMULT, recMULTU, recDIV, recDIVU, recNULL   , recNULL , recNULL, recNULL,
	recADD , recADDU, recSUB , recSUBU, recAND    , recOR   , recXOR , recNOR ,
	recNULL, recNULL, recSLT , recSLTU, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL   , recNULL , recNULL, recNULL
};

static void (*recREG[32])() = {
	recBLTZ  , recBGEZ  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recBLTZAL, recBGEZAL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL  , recNULL  , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP0[32])() = {
	recMFC0, recNULL, recCFC0, recNULL, recMTC0, recNULL, recCTC0, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recRFE , recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

static void (*recCP2[64])() = {
	recNULL, recNULL , recNULL , recNULL, recNULL, recNULL , recNULL, recNULL, // 00
	recNULL , recNULL , recNULL , recNULL, recNULL  , recNULL , recNULL , recNULL, // 08
	recNULL , recNULL, recNULL, recNULL, recNULL , recNULL , recNULL , recNULL, // 10
	recNULL , recNULL , recNULL , recNULL, recNULL  , recNULL , recNULL  , recNULL, // 18
	recNULL  , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 20
	recNULL  , recNULL , recNULL , recNULL, recNULL, recNULL, recNULL, recNULL, // 28 
	recNULL , recNULL , recNULL , recNULL, recNULL, recNULL , recNULL , recNULL, // 30
	recNULL , recNULL , recNULL , recNULL, recNULL, recNULL  , recNULL  , recNULL  // 38
};

static void (*recCP2BSC[32])() = {
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL,
	recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL, recNULL
};

extern void psxSLTI();

static void recRecompile(u32 startpc) {

#ifdef _DEBUG
	if( psxreclog ) __Log("psx: %x\n", startpc);
#endif

	/* if x86Ptr reached the mem limit reset whole mem */
	if (((uptr)recPtr - (uptr)recMem) >= (RECMEM_SIZE - 0x10000))
		recReset();

	x86SetPtr(recPtr);

	pc = psxRegs.pc;
	branch = 0;

	while(!branch) {
		recompileNextInstruction(0);
	}

	if( branch == 2 ) {
		iPsxBranchTest(0xffffffff);
		JMP32((u32)psxDispatcherReg - ( (u32)x86Ptr + 5 ));
	}
	
	recPtr = x86Ptr;	
}


R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};

