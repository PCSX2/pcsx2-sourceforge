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

// Super VU recompiler - author: zerofrog(@gmail.com)

#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <malloc.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif


#define PLUGINtypedefs // for GSgifTransfer1

#if defined(__WIN32__)
#include <windows.h>
#endif

#include "PS2Etypes.h"
#include "PS2Edefs.h"
#include "zlib.h"
#include "Misc.h"
#include "System.h"
#include "R5900.h"
#include "Vif.h"
#include "VU.h"

#include "Memory.h"
#include "Hw.h"
#include "GS.h"

#include "ix86/ix86.h"
#include "iR5900.h"

#include "iVUzerorec.h"

// temporary externs
extern u32 vudump;
extern void iDumpVU0Registers();
extern void iDumpVU1Registers();

extern char* disVU1MicroUF(u32 code, u32 pc);
extern char* disVU1MicroLF(u32 code, u32 pc);

extern _GSgifTransfer1 GSgifTransfer1;

#ifdef __cplusplus
}
#endif

#include <vector>
#include <list>
#include <map>
#include <algorithm>
using namespace std;

#ifdef __MSCW32__

#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

// SuperVURec optimization options, uncomment only for debugging purposes
#define SUPERVU_CACHING			// vu programs are saved and queried via CRC (might query the wrong program)
								// disable when in doubt
#define SUPERVU_X86CACHING		// use x86reg caching (faster)
#define SUPERVU_WRITEBACKS		// don't flush the writebacks after every block
#define SUPERVU_XGKICKDELAY		// yes this is needed

#ifndef _DEBUG
#define SUPERVU_INTERCACHING	// registers won't be flushed at block boundaries (faster)
#endif

#define VU_EXESIZE 0x00800000

#define _Imm11_ 	((s32)(vucode & 0x400 ? 0xfffffc00 | (vucode & 0x3ff) : vucode & 0x3ff)&0x3fff)
#define _UImm11_	((s32)(vucode & 0x7ff)&0x3fff)

#define _Ft_ ((VU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((VU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((VU->code >>  6) & 0x1F)  // The sa part of the instruction register

static const u32 QWaitTimes[] = { 6, 12 };
static const u32 PWaitTimes[] = { 53, 43, 28, 23, 17, 11, 10 };

static u32 s_vuInfo; // info passed into rec insts

static const u32 s_MemSize[2] = {VU0_MEMSIZE, VU1_MEMSIZE};
static char* s_recVUMem = NULL, *s_recVUPtr = NULL;

// tables
extern void (*recSVU_UPPER_OPCODE[64])();
extern void (*recSVU_LOWER_OPCODE[128])();

#define INST_Q_READ			0x0001 // flush Q
#define INST_P_READ			0x0002 // flush P
#define INST_BRANCH_DELAY	0x0004
#define INST_CLIP_WRITE		0x0040 // inst writes CLIP in the future
#define INST_STATUS_WRITE	0x0080
#define INST_MAC_WRITE		0x0100
#define INST_Q_WRITE		0x0200
#define INST_DUMMY_			0x8000
#define INST_DUMMY			0x83c0

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))

union VURecRegs
{
	struct {
		u16 reg;
		u16 type;
	};
	u32 id;
};

class VuBaseBlock;

struct VuFunctionHeader
{
	struct RANGE
	{
		RANGE() : pmem(NULL) {}
		
		u16 start, size;
		void* pmem; // all the mem
	};

	VuFunctionHeader() : pprogfunc(NULL), startpc(0xffffffff) {}
	~VuFunctionHeader() {
		for(vector<RANGE>::iterator it = ranges.begin(); it != ranges.end(); ++it) {
			free(it->pmem);
		}
	}

	// returns true if the checksum for the current mem is the same as this fn
	bool IsSame(void* pmem);

	u32 startpc;
	void* pprogfunc;

	vector<RANGE> ranges;
};

struct VuBlockHeader
{
	VuBaseBlock* pblock;
	u32 delay;
};

// one vu inst (lower and upper)
class VuInstruction
{
public:
	VuInstruction() { memset(this, 0, sizeof(VuInstruction)); nParentPc = -1; }

	int nParentPc; // used for syncing with flag writes, -1 for no parent

	_vuopinfo info;

	_VURegsNum regs[2]; // [0] - lower, [1] - upper
	u32 livevars[2]; // live variables right before this inst, [0] - inst, [1] - float
	u32 addvars[2]; // live variables to add
	u32 usedvars[2]; // set if var is used in the future including vars used in this inst
	u32 keepvars[2];
	u16 pqcycles; // the number of cycles to stall if function writes to the regs
	u16 type; // INST_

	u32 pClipWrite, pMACWrite, pStatusWrite; // addrs to write the flags
	u32 vffree[2];
	s8 vfwrite[2], vfread0[2], vfread1[2], vfacc[2];
	s8 vfflush[2]; // extra flush regs

	int SetCachedRegs(int upper, u32 vuxyz);
	void Recompile(list<VuInstruction>::const_iterator& itinst, u32 vuxyz);
};

#define BLOCKTYPE_EOP 0x01 // at least one of the children of the block contains eop (or the block itself)
#define BLOCKTYPE_FUNCTION 0x02
#define BLOCKTYPE_HASEOP 0x04 // last inst of block is an eop
#define BLOCKTYPE_MACFLAGS 0x08
#define BLOCKTYPE_ANALYZED 0x40
#define BLOCKTYPE_IGNORE 0x80 // special for recursive fns
#define BLOCKTYPE_ANALYZEDPARENT 0x100

// base block used when recompiling
class VuBaseBlock
{
public:
	typedef list<VuBaseBlock*> LISTBLOCKS;

	VuBaseBlock();
	
	// returns true if the leads to a EOP (ALL VU blocks must ret true)
	void AssignVFRegs();
	void AssignVIRegs(int parent);

	// returns true if only xyz of the reg has been used so far
	u32 GetModeXYZW(u32 curpc, int vfreg);

	list<VuInstruction>::iterator GetInstIterAtPc(int instpc);
	void GetInstsAtPc(int instpc, list<VuInstruction*>& listinsts);

	void Recompile();

	u16 type; // BLOCKTYPE_
	u16 id;
	u16 startpc;
	u16 endpc; // first inst not in block
	void* pcode; // x86 code pointer
	int cycles;
	list<VuInstruction> insts;
	list<VuBaseBlock*> parents;
	LISTBLOCKS blocks; // blocks branches to
	u32* pChildJumps[4]; // addrs that need to be filled with the children's start addrs
						// if highest bit is set, addr needs to be relational
	u32 vuxyz; // corresponding bit is set if reg's xyz channels are used only
	u32 vuxy; // corresponding bit is set if reg's xyz channels are used only

	_xmmregs startregs[XMMREGS], endregs[XMMREGS];
	int nStartx86, nEndx86; // indices into s_vecRegArray

	int allocX86Regs;
};

struct WRITEBACK
{
	void InitInst(VuInstruction* pinst, int cycle) const
	{
		u32 write = viwrite[0]|viwrite[1];
		pinst->type = ((write&(1<<REG_CLIP_FLAG))?INST_CLIP_WRITE:0)|
			((write&(1<<REG_MAC_FLAG))?INST_MAC_WRITE:0)|
			((write&(1<<REG_STATUS_FLAG))?INST_STATUS_WRITE:0)|
			((write&(1<<REG_Q))?INST_Q_WRITE:0);
		pinst->nParentPc = nParentPc;
		pinst->info.cycle = cycle;
		for(int i = 0; i < 2; ++i) {
			pinst->regs[i].VIwrite = viwrite[i];
			pinst->regs[i].VIread = viread[i];
		}
	}

	static int SortWritebacks(const WRITEBACK& w1, const WRITEBACK& w2) {
		return w1.cycle < w2.cycle;
	}

	int nParentPc;
	int cycle;
	u32 viwrite[2];
	u32 viread[2];
};

struct VUPIPELINES
{
	fmacPipe fmac[8];
	fdivPipe fdiv;
	efuPipe efu;
	list< WRITEBACK > listWritebacks;
};

VuBaseBlock::VuBaseBlock()
{
	type = 0; endpc = 0; cycles = 0; pcode = NULL; id = 0;
	memset(pChildJumps, 0, sizeof(pChildJumps));
	memset(startregs, 0, sizeof(startregs));
	memset(endregs, 0, sizeof(endregs));
	allocX86Regs = nStartx86 = nEndx86 = -1;
}

#define SUPERVU_STACKSIZE 0x1000

static list<VuFunctionHeader*> s_listVUHeaders[2];
static list<VuFunctionHeader*>* s_plistCachedHeaders[2];
static VuFunctionHeader** recVUHeaders[2] = {NULL};
static VuBlockHeader* recVUBlocks[2] = {NULL};
static u8* recVUStack = NULL, *recVUStackPtr = NULL;
static vector<_x86regs> s_vecRegArray(128);

static VURegs* VU = NULL;
static list<VuBaseBlock*> s_listBlocks;
static u32 s_vu = 0;
static u32 s_UnconditionalDelay = 0; // 1 if there are two sequential branches and the last is unconditional

// Global functions
static void* SuperVUGetProgram(u32 startpc, int vuindex);
static VuFunctionHeader* SuperVURecompileProgram(u32 startpc, int vuindex);
static VuBaseBlock* SuperVUBuildBlocks(VuBaseBlock* parent, u32 startpc, const VUPIPELINES& pipes);
static void SuperVUInitLiveness(VuBaseBlock* pblock);
static void SuperVULivenessAnalysis();
static void SuperVUEliminateDeadCode();
static void SuperVUAssignRegs();

//void SuperVUFreeXMMreg(int xmmreg, int xmmtype, int reg);
#define SuperVUFreeXMMreg 0&&
void SuperVUFreeXMMregs(u32* livevars);

static u32* SuperVUStaticAlloc(u32 size);
static void SuperVURecompile();
void SuperVUEndProgram();

// allocate VU resources
void SuperVUInit(int vuindex)
{
	if( vuindex < 0 ) {
		s_recVUMem = (char*)SysMmap(0, VU_EXESIZE);
		memset(s_recVUMem, 0xcd, VU_EXESIZE);
		s_recVUPtr = s_recVUMem;
		recVUStack = new u8[SUPERVU_STACKSIZE * 4];
	}
	else {
		recVUHeaders[vuindex] = new VuFunctionHeader* [s_MemSize[vuindex]/8];
		recVUBlocks[vuindex] = new VuBlockHeader[s_MemSize[vuindex]/8];
		s_plistCachedHeaders[vuindex] = new list<VuFunctionHeader*>[s_MemSize[vuindex]/8];
	}
}

// destroy VU resources
void SuperVUDestroy(int vuindex)
{
	list<VuFunctionHeader*>::iterator it;

	if( vuindex < 0 ) {
		SuperVUDestroy(0);
		SuperVUDestroy(1);
		SysMunmap((uptr)s_recVUMem, VU_EXESIZE);
		s_recVUPtr = NULL;
		delete[] recVUStack; recVUStack = NULL;
	}
	else {
		delete[] recVUHeaders[vuindex]; recVUHeaders[vuindex] = NULL;
		delete[] recVUBlocks[vuindex]; recVUBlocks[vuindex] = NULL;

		if( s_plistCachedHeaders[vuindex] != NULL ) {
			for(u32 j = 0; j < s_MemSize[vuindex]/8; ++j) {
				FORIT(it, s_plistCachedHeaders[vuindex][j]) delete *it;
				s_plistCachedHeaders[vuindex][j].clear();
			}
			delete[] s_plistCachedHeaders[vuindex]; s_plistCachedHeaders[vuindex] = NULL;
		}

		FORIT(it, s_listVUHeaders[vuindex]) delete *it;
		s_listVUHeaders[vuindex].clear();
	}
}

// reset VU
void SuperVUReset(int vuindex)
{
	list<VuFunctionHeader*>::iterator it;

	if( vuindex < 0 ) {
		SuperVUReset(0);
		SuperVUReset(1);

		//memset(s_recVUMem, 0xcd, VU_EXESIZE);
		s_recVUPtr = s_recVUMem;

		memset(recVUStack, 0, SUPERVU_STACKSIZE);
	}
	else {
		if( recVUHeaders[vuindex] ) memset( recVUHeaders[vuindex], 0, sizeof(VuFunctionHeader*) * (s_MemSize[vuindex]/8) );
		if( recVUBlocks[vuindex] ) memset( recVUBlocks[vuindex], 0, sizeof(VuBlockHeader) * (s_MemSize[vuindex]/8) );
		
		if( s_plistCachedHeaders[vuindex] != NULL ) {
			for(u32 j = 0; j < s_MemSize[vuindex]/8; ++j) {
				FORIT(it, s_plistCachedHeaders[vuindex][j]) delete *it;
				s_plistCachedHeaders[vuindex][j].clear();
			}
		}
		
		FORIT(it, s_listVUHeaders[vuindex]) delete *it;
		s_listVUHeaders[vuindex].clear();
	}
}

// clear the block and any joining blocks
void SuperVUClear(u32 startpc, u32 size, int vuindex)
{
	vector<VuFunctionHeader::RANGE>::iterator itrange;
	list<VuFunctionHeader*>::iterator it = s_listVUHeaders[vuindex].begin();
	u32 endpc = startpc+size;
	while( it != s_listVUHeaders[vuindex].end() ) {

		// for every fn, check if it has code in the range
		FORIT(itrange, (*it)->ranges) {
			if( startpc < (u32)itrange->start+itrange->size && itrange->start < endpc )
				break;
		}

		if( itrange != (*it)->ranges.end() ) {
			recVUHeaders[vuindex][(*it)->startpc/8] = NULL;
#ifdef SUPERVU_CACHING
			list<VuFunctionHeader*>* plist = &s_plistCachedHeaders[vuindex][(*it)->startpc/8];
			plist->push_back(*it);
			if( plist->size() > 10 ) {
				// list is too big, delete
				delete plist->front();
				plist->pop_front();
			}
			it = s_listVUHeaders[vuindex].erase(it);
#else
			delete *it;
			it = s_listVUHeaders[vuindex].erase(it);
#endif
		}
		else ++it;
	}
}

static VuFunctionHeader* s_pFnHeader = NULL;
static VuBaseBlock* s_pCurBlock = NULL;
static VuInstruction* s_pCurInst = NULL;
static u32 s_StatusRead = 0, s_MACRead = 0, s_ClipRead = 0; // read addrs
static u32 s_PrevStatusWrite = 0, s_PrevMACWrite = 0, s_PrevClipWrite = 0, s_PrevIWrite = 0;
static u32 s_WriteToReadQ = 0;
static u32 s_TotalVUCycles; // total cycles since start of program execution

int SuperVUGetLiveness(int vfreg)
{
	assert( s_pCurInst != NULL );
	if( vfreg == 32 ) return ((s_pCurInst->livevars[0]&(1<<REG_ACC_FLAG))?1:0)|((s_pCurInst->usedvars[0]&(1<<REG_ACC_FLAG))?2:0);
	else if( vfreg == 0 ) return ((s_pCurInst->livevars[0]&(1<<REG_VF0_FLAG))?1:0)|((s_pCurInst->usedvars[0]&(1<<REG_VF0_FLAG))?2:0);

	return ((s_pCurInst->livevars[1]&(1<<vfreg))?1:0)|((s_pCurInst->usedvars[1]&(1<<vfreg))?2:0);
}

u32 SuperVUGetVIAddr(int reg, int read)
{
	assert( s_pCurInst != NULL );

	switch(reg) {
		case REG_STATUS_FLAG:
		{
			u32 addr = (read==2) ? s_PrevStatusWrite : (read ? s_StatusRead : s_pCurInst->pStatusWrite);
			assert(!read || addr != 0);
			return addr;
		}
		case REG_MAC_FLAG:
		{
			return (read==2) ? s_PrevMACWrite : (read ? s_MACRead : s_pCurInst->pMACWrite);
		}
		case REG_CLIP_FLAG:
		{
			u32 addr = (read==2) ? s_PrevClipWrite : (read ? s_ClipRead : s_pCurInst->pClipWrite);
			assert( !read || addr != 0 );
			return addr;
		}
		case REG_Q: return (read || s_WriteToReadQ) ? (int)&VU->VI[REG_Q] : (u32)(u32)&VU->q;
		case REG_P: return read ? (int)&VU->VI[REG_P] : (u32)(u32)&VU->p;
		case REG_I: return s_PrevIWrite;
	}

	return (u32)&VU->VI[reg];
}

void SuperVUDumpBlock(list<VuBaseBlock*>& blocks, int vuindex)
{
	FILE *f;
	char filename[ 256 ], str[256];
	u32 *mem;
	u32 i;
	static int gid = 0;

#ifdef __WIN32__
	CreateDirectory("dumps", NULL);
	sprintf( filename, "dumps\\svu%c_%.4X.txt", s_vu?'1':'0', s_pFnHeader->startpc );
#else
	mkdir("dumps", 0755);
	sprintf( filename, "dumps/svu%c_%.4X.txt", s_vu?'1':'0', s_pFnHeader->startpc );
#endif
	//SysPrintf( "dump1 %x => %s\n", s_pFnHeader->startpc, filename );

	f = fopen( filename, "w" );

	fprintf(f, "Format: upper_inst lower_inst\ntype f:vf_live_vars vf_used_vars i:vi_live_vars vi_used_vars inst_cycle pq_inst\n");
	fprintf(f, "Type: %.2x - qread, %.2x - pread, %.2x - clip_write, %.2x - status_write\n"
		"%.2x - mac_write, %.2x -qflush\n",
		INST_Q_READ, INST_P_READ, INST_CLIP_WRITE, INST_STATUS_WRITE, INST_MAC_WRITE, INST_Q_WRITE);
	fprintf(f, "XMM: Upper: read0 read1 write acc temp; Lower: read0 read1 write acc temp\n\n");

	list<VuBaseBlock*>::iterator itblock;
	list<VuInstruction>::iterator itinst;
	VuBaseBlock::LISTBLOCKS::iterator itchild;

	FORIT(itblock, blocks) {
		fprintf(f, "block:%c %x-%x; children: ", ((*itblock)->type&BLOCKTYPE_HASEOP)?'*':' ',
			(*itblock)->startpc, (*itblock)->endpc-8);
		FORIT(itchild, (*itblock)->blocks) {
			fprintf(f, "%x ", (*itchild)->startpc);
		}
		fprintf(f, "; vuxyz = %x, vuxy = %x\n", (*itblock)->vuxyz&(*itblock)->insts.front().usedvars[1],
			(*itblock)->vuxy&(*itblock)->insts.front().usedvars[1]);

		itinst = (*itblock)->insts.begin();
		i = (*itblock)->startpc;
		while(itinst != (*itblock)->insts.end() ) {
			assert( i <= (*itblock)->endpc );
			if( itinst->type & INST_DUMMY ) {
				if( itinst->nParentPc >= 0 && !(itinst->type&INST_DUMMY_)) {
					// search for the parent
					fprintf(f, "writeback 0x%x (%x)\n", itinst->type, itinst->nParentPc);
				}
			}
			else {
				mem = (u32*)&VU->Micro[i];
				char* pstr = disVU1MicroUF( mem[1], i+4 );
				fprintf(f, "%.4x: %-40s",  i, pstr);
				if( mem[1] & 0x80000000 ) fprintf(f, " I=%f(%.8x)\n", *(float*)mem, mem[0]);
				else fprintf(f, "%s\n", disVU1MicroLF( mem[0], i ));
				i += 8;
			}

			++itinst;
		}

		fprintf(f, "\n");

		_x86regs* pregs;
		if( (*itblock)->nStartx86 >= 0 || (*itblock)->nEndx86 >= 0 ) {
			fprintf(f, "X86: AX CX DX BX SP BP SI DI\n");
		}

		if( (*itblock)->nStartx86 >= 0 ) {
			pregs = &s_vecRegArray[(*itblock)->nStartx86];
			fprintf(f, "STR: ");
			for(i = 0; i < X86REGS; ++i) {
				if( pregs[i].inuse ) fprintf(f, "%.2d ", pregs[i].reg);
				else fprintf(f, "-1 ");
			}
			fprintf(f, "\n");
		}

		if( (*itblock)->nEndx86 >= 0 ) {
			fprintf(f, "END: ");
			pregs = &s_vecRegArray[(*itblock)->nEndx86];
			for(i = 0; i < X86REGS; ++i) {
				if( pregs[i].inuse ) fprintf(f, "%.2d ", pregs[i].reg);
				else fprintf(f, "-1 ");
			}
			fprintf(f, "\n");
		}

		itinst = (*itblock)->insts.begin();
		for ( i = (*itblock)->startpc; i < (*itblock)->endpc; ++itinst ) {

			if( itinst->type & INST_DUMMY ) {
			}
			else {
				sprintf(str, "%.4x:%x f:%.8x_%.8x", i, itinst->type, itinst->livevars[1], itinst->usedvars[1]);
				fprintf(f, "%-46s i:%.8x_%.8x c:%d pq:%d\n", str,
					itinst->livevars[0], itinst->usedvars[0], (int)itinst->info.cycle, (int)itinst->pqcycles );

				sprintf(str, "XMM r0:%d r1:%d w:%d a:%d t:%x;",
					itinst->vfread0[1], itinst->vfread1[1], itinst->vfwrite[1], itinst->vfacc[1], itinst->vffree[1]);
				fprintf(f, "%-46s r0:%d r1:%d w:%d a:%d t:%x\n", str,
					itinst->vfread0[0], itinst->vfread1[0], itinst->vfwrite[0], itinst->vfacc[0], itinst->vffree[0]);
				i += 8;
			}
		}

		fprintf(f, "\n---------------\n");
	}

	fclose( f );
}

static LARGE_INTEGER svubase, svufinal;
static u32 svutime;

// uncomment to count svu exec time
//#define SUPERVU_COUNT
u32 SuperVUGetRecTimes(int clear)
{
	u32 temp = svutime;
	if( clear ) svutime = 0;
	return temp;
}

// Private methods
static void* SuperVUGetProgram(u32 startpc, int vuindex)
{
	assert( startpc < s_MemSize[vuindex] );
	assert( (startpc%8) == 0 );
	assert( recVUHeaders[vuindex] != NULL );
	VuFunctionHeader** pheader = &recVUHeaders[vuindex][startpc/8];

	if( *pheader == NULL ) {
#ifdef _DEBUG
//		if( vuindex ) VU1.VI[REG_TPC].UL = startpc;
//		else VU0.VI[REG_TPC].UL = startpc;
//		__Log("VU: %x\n", startpc);
//		iDumpVU1Registers();
//		vudump |= 2;
#endif

		// measure run time
		//QueryPerformanceCounter(&svubase);

#ifdef SUPERVU_CACHING
		void* pmem = (vuindex&1) ? VU1.Micro : VU0.Micro;
		// check if program exists in cache
		list<VuFunctionHeader*>::iterator it;
		FORIT(it, s_plistCachedHeaders[vuindex][startpc/8]) {
			if( (*it)->IsSame(pmem) ) {
				// found, transfer to regular lists
				void* pfn = (*it)->pprogfunc;
				recVUHeaders[vuindex][startpc/8] = *it;
				s_listVUHeaders[vuindex].push_back(*it);
				s_plistCachedHeaders[vuindex][startpc/8].erase(it);
				return pfn;
			}
		}
#endif

		*pheader = SuperVURecompileProgram(startpc, vuindex);

		if( *pheader == NULL ) {
			assert( s_TotalVUCycles > 0 );
			if( vuindex ) VU1.VI[REG_TPC].UL = startpc;
			else VU0.VI[REG_TPC].UL = startpc;
			return SuperVUEndProgram;
		}

		//QueryPerformanceCounter(&svufinal);
		//svutime += (u32)(svufinal.QuadPart-svubase.QuadPart);

		assert( (*pheader)->pprogfunc != NULL );
	}

	assert( (*pheader)->startpc == startpc );

	return (*pheader)->pprogfunc;
}

bool VuFunctionHeader::IsSame(void* pmem)
{
#ifdef SUPERVU_CACHING
	//u32 checksum[2];
	vector<RANGE>::iterator it;
	FORIT(it, ranges) {
		//memxor_mmx(checksum, (u8*)pmem+it->start, it->size);
		//if( checksum[0] != it->checksum[0] || checksum[1] != it->checksum[1] )
		//	return false;
		if( memcmp_mmx((u8*)pmem+it->start, it->pmem, it->size) )
			return false;
	}
#endif
	return true;
}

list<VuInstruction>::iterator VuBaseBlock::GetInstIterAtPc(int instpc)
{
	assert( instpc >= 0 );

	u32 curpc = startpc;
	list<VuInstruction>::iterator it;
	for(it = insts.begin(); it != insts.end(); ++it) {
		if( it->type & INST_DUMMY )
			continue;

		if( curpc == instpc )
			break;
		curpc += 8;
	}

	if( it != insts.end() )
		return it;

	assert( 0 );
	return insts.begin();
}

void VuBaseBlock::GetInstsAtPc(int instpc, list<VuInstruction*>& listinsts)
{
	assert( instpc >= 0 );

	listinsts.clear();

	u32 curpc = startpc;
	list<VuInstruction>::iterator it;
	for(it = insts.begin(); it != insts.end(); ++it) {
		if( it->type & INST_DUMMY )
			continue;

		if( curpc == instpc )
			break;
		curpc += 8;
	}

	if( it != insts.end() ) {
		listinsts.push_back(&(*it));
		return;
	}

	// look for the pc in other blocks
	for(list<VuBaseBlock*>::iterator itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); ++itblock) {
		if( *itblock == this )
			continue;

		if( instpc >= (*itblock)->startpc && instpc < (*itblock)->endpc ) {
			listinsts.push_back(&(*(*itblock)->GetInstIterAtPc(instpc)));
		}
	}

	assert(listinsts.size()>0);
}

static VuFunctionHeader* SuperVURecompileProgram(u32 startpc, int vuindex)
{
	assert( vuindex < 2 );
	assert( s_recVUPtr != NULL );
	//SysPrintf("svu%c rec: %x\n", '0'+vuindex, startpc);

	// if recPtr reached the mem limit reset whole mem
	if ( ( (u32)s_recVUPtr - (u32)s_recVUMem ) >= VU_EXESIZE-0x40000 ) { 
		//SysPrintf("SuperVU reset mem\n");
		SuperVUReset(-1); 
		if( s_TotalVUCycles > 0 ) {
			// already executing, so return NULL
			return NULL;
		}
	}

	list<VuBaseBlock*>::iterator itblock;

	s_vu = vuindex;
	VU = s_vu ? &VU1 : &VU0;
	s_pFnHeader = new VuFunctionHeader();
	s_listVUHeaders[vuindex].push_back(s_pFnHeader);
	s_pFnHeader->startpc = startpc;

	memset( recVUBlocks[s_vu], 0, sizeof(VuBlockHeader) * (s_MemSize[s_vu]/8) );

	// analyze the global graph
	s_listBlocks.clear();
	VUPIPELINES pipes;
	memset(pipes.fmac, 0, sizeof(pipes.fmac));
	memset(&pipes.fdiv, 0, sizeof(pipes.fdiv));
	memset(&pipes.efu, 0, sizeof(pipes.efu));
	SuperVUBuildBlocks(NULL, startpc, pipes);

	// fill parents
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	FORIT(itblock, s_listBlocks) {
		FORIT(itchild, (*itblock)->blocks)
			(*itchild)->parents.push_back(*itblock);

		//(*itblock)->type &= ~(BLOCKTYPE_IGNORE|BLOCKTYPE_ANALYZED);
	}

	assert( s_listBlocks.front()->startpc == startpc );
	s_listBlocks.front()->type |= BLOCKTYPE_FUNCTION;

	FORIT(itblock, s_listBlocks) {
		SuperVUInitLiveness(*itblock);
	}

	SuperVULivenessAnalysis();
	SuperVUEliminateDeadCode();
	SuperVUAssignRegs();

#ifdef _DEBUG
	if( (s_vu && (vudump&1)) || (!s_vu && (vudump&16)) )
		SuperVUDumpBlock(s_listBlocks, s_vu);
#endif

	// code generation
	x86SetPtr(s_recVUPtr);
	_initXMMregs();
	branch = 0;

	SuperVURecompile();

	s_recVUPtr = x86Ptr;

	// set the function's range
	VuFunctionHeader::RANGE r;
	s_pFnHeader->ranges.reserve(s_listBlocks.size());

	FORIT(itblock, s_listBlocks) {
		r.start = (*itblock)->startpc;
		r.size = (*itblock)->endpc-(*itblock)->startpc;
#ifdef SUPERVU_CACHING
		//memxor_mmx(r.checksum, &VU->Micro[r.start], r.size);
		r.pmem = malloc(r.size);
		memcpy(r.pmem, &VU->Micro[r.start], r.size);
#endif
		s_pFnHeader->ranges.push_back(r);
	}

	// destroy
	for(list<VuBaseBlock*>::iterator itblock = s_listBlocks.begin(); itblock != s_listBlocks.end(); ++itblock) {
		delete *itblock;
	}
	s_listBlocks.clear();

	assert( s_recVUPtr < s_recVUMem+VU_EXESIZE );

	return s_pFnHeader;
}

static int _recbranchAddr(u32 vucode) {
	u32 bpc = pc + (_Imm11_ << 3);
	if (bpc < 0) {
		bpc = pc + (_UImm11_ << 3); 
	}
	bpc &= (s_MemSize[s_vu]-1);

	return bpc;
}

// return inst that flushes everything
static VuInstruction SuperVUFlushInst()
{
	VuInstruction inst;
	// don't need to raed q/p
	inst.type = INST_DUMMY_;//|INST_Q_READ|INST_P_READ;
	return inst;
}

void SuperVUAddWritebacks(VuBaseBlock* pblock, const list<WRITEBACK>& listWritebacks)
{
#ifdef SUPERVU_WRITEBACKS
		// regardless of repetition, add the pipes (for selfloops)
		list<WRITEBACK>::const_iterator itwriteback = listWritebacks.begin();
		list<VuInstruction>::iterator itinst = pblock->insts.begin(), itinst2;

		while(itwriteback != listWritebacks.end()) {
			if( itinst != pblock->insts.end() && itinst->info.cycle < itwriteback->cycle ) {
				++itinst;
				continue;
			}

			itinst2 = pblock->insts.insert(itinst, VuInstruction());
			itwriteback->InitInst(&(*itinst2), vucycle);
			++itwriteback;
		}
#endif
}

static VuBaseBlock* SuperVUBuildBlocks(VuBaseBlock* parent, u32 startpc, const VUPIPELINES& pipes)
{
	// check if block already exists
	VuBlockHeader* pbh = &recVUBlocks[s_vu][startpc/8];
	if( pbh->pblock != NULL ) {

		VuBaseBlock* pblock = pbh->pblock;
		list<VuInstruction>::iterator itinst;

		if( pblock->startpc == startpc ) {
			SuperVUAddWritebacks(pblock, pipes.listWritebacks);
			return pblock;
		}

		// have to divide the blocks, pnewblock is first block
		assert( startpc > pblock->startpc );
		assert( startpc < pblock->endpc );

		u32 dummyinst = (startpc-pblock->startpc)>>3;

		// count inst non-dummy insts
		itinst = pblock->insts.begin();
		u32 inst = 0;
		while(dummyinst > 0) {
			if( itinst->type & INST_DUMMY )
				++itinst;
			else {
				++itinst;
				--dummyinst;
			}
		}

		// NOTE: still leaves insts with their writebacks in different blocks
		while( itinst->type & INST_DUMMY )
			++itinst;

		int cycleoff = itinst->info.cycle;

		// new block
		VuBaseBlock* pnewblock = new VuBaseBlock();
		s_listBlocks.push_back(pnewblock);

		pnewblock->startpc = startpc;
		pnewblock->endpc = pblock->endpc;
		pnewblock->cycles = pblock->cycles-cycleoff;
		
		pnewblock->blocks.splice(pnewblock->blocks.end(), pblock->blocks);
		pnewblock->insts.splice(pnewblock->insts.end(), pblock->insts, itinst, pblock->insts.end());
		pnewblock->type = pblock->type;
		
		// any writebacks in the next 3 cycles also belong to original block
		for(itinst = pnewblock->insts.begin(); itinst != pnewblock->insts.end(); ) {
			if( (itinst->type & INST_DUMMY) && itinst->nParentPc >= 0 && itinst->nParentPc < (int)startpc ) {

				if( !(itinst->type & INST_Q_WRITE) )
					pblock->insts.push_back(*itinst);
				itinst = pnewblock->insts.erase(itinst);
				continue;
			}

			++itinst;
		}

		pbh = &recVUBlocks[s_vu][startpc/8];
		for(u32 inst = startpc; inst < pblock->endpc; inst += 8) {
			if( pbh->pblock == pblock )
				pbh->pblock = pnewblock;
			++pbh;
		}

		FORIT(itinst, pnewblock->insts)
			itinst->info.cycle -= cycleoff;

		SuperVUAddWritebacks(pnewblock, pipes.listWritebacks);

		// old block
		pblock->blocks.push_back(pnewblock);
		pblock->endpc = startpc;
		pblock->cycles = cycleoff;
		pblock->type &= BLOCKTYPE_MACFLAGS;
		//pblock->insts.push_back(SuperVUFlushInst()); //don't need

		return pnewblock;
	}

	VuBaseBlock* pblock = new VuBaseBlock();
	s_listBlocks.push_back(pblock);
	
	int i = 0;
	branch = 0;
	pc = startpc;
	pblock->startpc = startpc;

	// clear stalls (might be a prob)
	memcpy(VU->fmac, pipes.fmac, sizeof(pipes.fmac));
	memcpy(&VU->fdiv, &pipes.fdiv, sizeof(pipes.fdiv));
	memcpy(&VU->efu, &pipes.efu, sizeof(pipes.efu));
//	memset(VU->fmac, 0, sizeof(VU->fmac));
//	memset(&VU->fdiv, 0, sizeof(VU->fdiv));
//	memset(&VU->efu, 0, sizeof(VU->efu));

	vucycle = 0;

	u8 macflags = 0;

	list< WRITEBACK > listWritebacks;
	list< WRITEBACK >::iterator itwriteback;
	list<VuInstruction>::iterator itinst;
	u32 hasSecondBranch = 0;
	u32 needFullStatusFlag = 0;

#ifdef SUPERVU_WRITEBACKS
	listWritebacks = pipes.listWritebacks;
#endif

	// first analysis pass for status flags
	while(1) {
		u32* ptr = (u32*)&VU->Micro[pc];
		pc += 8;
		int prevbranch = branch;

		if( ptr[1] & 0x40000000 )
			branch = 1;

		if( !(ptr[1] & 0x80000000) ) { // not I
			switch( ptr[0]>>25 ) {
				case 0x24: // jr
				case 0x25: // jalr
				case 0x20: // B
				case 0x21: // BAL
				case 0x28: // IBEQ
				case 0x2f: // IBGEZ
				case 0x2d: // IBGTZ
				case 0x2e: // IBLEZ
				case 0x2c: // IBLTZ
				case 0x29: // IBNE
					branch = 1;
					break;

				case 0x14: // fseq
				case 0x17: // fsor
					//needFullStatusFlag = 2;
					break;

				case 0x16: // fsand
					if( (ptr[0]&0xc0) ) {
						// sometimes full sticky bits are needed (simple series 2000 - oane chapara)
						//SysPrintf("needSticky: %x-%x\n", s_pFnHeader->startpc, startpc);
						needFullStatusFlag = 2;
					}
					break;
			}
		}

		if( prevbranch )
			break;

		assert( pc < s_MemSize[s_vu] );
	}

	// second full pass
	pc = startpc;
	branch = 0;

	while(1) {

		if( !branch && pbh->pblock != NULL ) {
			pblock->blocks.push_back(pbh->pblock);
			break;
		}

		int prevbranch = branch;

		if( !prevbranch ) {
			pbh->pblock = pblock;
		}
		else assert( prevbranch || pbh->pblock == NULL);

		pblock->insts.push_back(VuInstruction());

		VuInstruction* pinst = &pblock->insts.back();
		SuperVUAnalyzeOp(VU, &pinst->info, pinst->regs);

		if( prevbranch ) {
			if( pinst->regs[0].pipe == VUPIPE_BRANCH )
				hasSecondBranch = 1;
			pinst->type |= INST_BRANCH_DELAY;
		}

		// check write back
		for(itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end(); ) {
			if( pinst->info.cycle >= itwriteback->cycle ) {
				itinst = pblock->insts.insert(--pblock->insts.end(), VuInstruction());
				itwriteback->InitInst(&(*itinst), pinst->info.cycle);
				itwriteback = listWritebacks.erase(itwriteback);
			}
			else ++itwriteback;
		}

		// add new writebacks
		WRITEBACK w = {0};
		const u32 allflags = (1<<REG_CLIP_FLAG)|(1<<REG_MAC_FLAG)|(1<<REG_STATUS_FLAG);
		for(int j = 0; j < 2; ++j) w.viwrite[j] = pinst->regs[j].VIwrite & allflags;

		if( pinst->info.macflag & VUOP_WRITE ) w.viwrite[1] |= (1<<REG_MAC_FLAG);
		if( pinst->info.statusflag & VUOP_WRITE ) w.viwrite[1] |= (1<<REG_STATUS_FLAG);

		if( (pinst->info.macflag|pinst->info.statusflag) & VUOP_READ )
			macflags = 1;
		if( pinst->regs[0].VIread & ((1<<REG_MAC_FLAG)|(1<<REG_STATUS_FLAG)) )
			macflags = 1;

		//uregs->VIwrite |= lregs->VIwrite & (1<<REG_STATUS_FLAG);

		if( w.viwrite[0]|w.viwrite[1] ) {

			// only if coming from fmac pipeline
			if( ((pinst->info.statusflag&VUOP_WRITE)&&!(pinst->regs[0].VIwrite&(1<<REG_STATUS_FLAG))) && needFullStatusFlag ) {
				// don't read if first inst
				if( needFullStatusFlag == 1 )
					w.viread[1] |= (1<<REG_STATUS_FLAG);
				else --needFullStatusFlag;
			}

			for(int j = 0; j < 2; ++j) {
				w.viread[j] |= pinst->regs[j].VIread & allflags;

				if( (pinst->regs[j].VIread&(1<<REG_STATUS_FLAG)) && (pinst->regs[j].VIwrite&(1<<REG_STATUS_FLAG)) ) {
					// don't need the read anymore
					pinst->regs[j].VIread &= ~(1<<REG_STATUS_FLAG);
				}
				if( (pinst->regs[j].VIread&(1<<REG_MAC_FLAG)) && (pinst->regs[j].VIwrite&(1<<REG_MAC_FLAG)) ) {
					// don't need the read anymore
					pinst->regs[j].VIread &= ~(1<<REG_MAC_FLAG);
				}

				pinst->regs[j].VIwrite &= ~allflags;
			}

			if( pinst->info.macflag & VUOP_READ) w.viread[1] |= 1<<REG_MAC_FLAG;
			if( pinst->info.statusflag & VUOP_READ) w.viread[1] |= 1<<REG_STATUS_FLAG;

			w.nParentPc = pc-8;
			w.cycle = pinst->info.cycle+4;
			listWritebacks.push_back(w);
		}

		if( pinst->info.q&VUOP_READ ) pinst->type |= INST_Q_READ;
		if( pinst->info.p&VUOP_READ ) pinst->type |= INST_P_READ;

		if( pinst->info.q&VUOP_WRITE ) {
			pinst->pqcycles = QWaitTimes[pinst->info.pqinst]+1;

			memset(&w, 0, sizeof(w));
			w.nParentPc = pc-8;
			w.cycle = pinst->info.cycle+pinst->pqcycles;
			w.viwrite[0] = 1<<REG_Q;
			listWritebacks.push_back(w);
		}
		if( pinst->info.p&VUOP_WRITE )
			pinst->pqcycles = PWaitTimes[pinst->info.pqinst]+1;

		if( prevbranch ) {
			break;
		}

		// make sure there is always a branch
        // sensible soccer overflows on vu0, so increase the limit...
		if( (s_vu==1 && i >= 0x799) || (s_vu==0 && i >= 0x201) ) {
			SysPrintf("VuRec base block doesn't terminate!\n");
            assert(0);
			break;
		}

		i++;
		pbh++;
	}

	if( macflags )
		pblock->type |= BLOCKTYPE_MACFLAGS;

	pblock->endpc = pc;
	u32 lastpc = pc;

	pblock->cycles = vucycle;

#ifdef SUPERVU_WRITEBACKS
	if( !branch || (branch&8) )
#endif
	{
		// flush writebacks
		if( listWritebacks.size() > 0 ) {
			listWritebacks.sort(WRITEBACK::SortWritebacks);
			for(itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end(); ++itwriteback) {
				if( itwriteback->viwrite[0] & (1<<REG_Q) ) {
					// ignore all Q writebacks
					continue;
				}

				pblock->insts.push_back(VuInstruction());
				itwriteback->InitInst(&pblock->insts.back(), vucycle);
			}

			listWritebacks.clear();
		}
	}

	if( !branch )
		return pblock;

	if( branch & 8 ) {
		// what if also a jump?
		pblock->type |= BLOCKTYPE_EOP|BLOCKTYPE_HASEOP;

		// add an instruction to flush p and q (if written)
		pblock->insts.push_back(SuperVUFlushInst());
		return pblock;
	}

	// it is a (cond) branch or a jump
	u32 vucode = *(u32*)(VU->Micro+lastpc-16);
	int bpc = _recbranchAddr(vucode)-8;

	VUPIPELINES newpipes;
	memcpy(newpipes.fmac, VU->fmac, sizeof(newpipes.fmac));
	memcpy(&newpipes.fdiv, &VU->fdiv, sizeof(newpipes.fdiv));
	memcpy(&newpipes.efu, &VU->efu, sizeof(newpipes.efu));

	for(i = 0; i < 8; ++i) newpipes.fmac[i].sCycle -= vucycle;
	newpipes.fdiv.sCycle -= vucycle;
	newpipes.efu.sCycle -= vucycle;

	if( listWritebacks.size() > 0 ) {
		bool bFlushWritebacks = (vucode>>25)==0x24||(vucode>>25)==0x25||(vucode>>25)==0x20||(vucode>>25)==0x21;

		listWritebacks.sort(WRITEBACK::SortWritebacks);
		for(itwriteback = listWritebacks.begin(); itwriteback != listWritebacks.end(); ++itwriteback) {
			if( itwriteback->viwrite[0] & (1<<REG_Q) ) {
				// ignore all Q writebacks
				continue;
			}

			if( itwriteback->cycle <= vucycle || bFlushWritebacks ) {
				pblock->insts.push_back(VuInstruction());
				itwriteback->InitInst(&pblock->insts.back(), vucycle);
			}
			else {
				newpipes.listWritebacks.push_back(*itwriteback);
				newpipes.listWritebacks.back().cycle -= vucycle;
			}
		}
	}

    u32 firstbranch = vucode>>25;
	switch(firstbranch) {
		case 0x24: // jr
			pblock->type |= BLOCKTYPE_EOP; // jump out of procedure, since not returning, set EOP
			pblock->insts.push_back(SuperVUFlushInst());
			break;

		case 0x25: // jalr
		{
			// linking, so will return to procedure
			pblock->insts.push_back(SuperVUFlushInst());

			VuBaseBlock* pjumpblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

			// update pblock since could have changed
			pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
			assert( pblock != NULL );

			pblock->blocks.push_back(pjumpblock);
			break;
		}
		case 0x20: // B
		{
			VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

			// update pblock since could have changed
			pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
			assert( pblock != NULL );

			pblock->blocks.push_back(pbranchblock);
			break;
		}
		case 0x21: // BAL
		{
			VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

			// update pblock since could have changed
			pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
			assert( pblock != NULL );
			pblock->blocks.push_back(pbranchblock);
			break;
		}
		case 0x28: // IBEQ
		case 0x2f: // IBGEZ
		case 0x2d: // IBGTZ
		case 0x2e: // IBLEZ
		case 0x2c: // IBLTZ
		case 0x29: // IBNE
		{
			VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

			// update pblock since could have changed
			pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
			assert( pblock != NULL );
			pblock->blocks.push_back(pbranchblock);

			// if has a second branch that is B or BAL, skip this
            u32 secondbranch = (*(u32*)(VU->Micro+lastpc-8))>>25;
			if( !hasSecondBranch || (secondbranch != 0x21 && secondbranch != 0x20) ) {
				pbranchblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				pblock->blocks.push_back(pbranchblock);
			}

			break;
		}
		default:
			assert(pblock->blocks.size() == 1);
			break;
	}

	pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;

	if( hasSecondBranch ) {
		u32 vucode = *(u32*)(VU->Micro+lastpc-8);
		pc = lastpc;
		int bpc = _recbranchAddr(vucode);

		switch(vucode>>25) {
			case 0x24: // jr
				SysPrintf("svurec bad jr jump!\n");
				assert(0);
				break;

			case 0x25: // jalr
			{
				SysPrintf("svurec bad jalr jump!\n");
				assert(0);
				break;
			}
			case 0x20: // B
			{
                VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;

				pblock->blocks.push_back(pbranchblock);
				break;
			}
			case 0x21: // BAL
			{
                VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// replace instead of pushing a new block
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				pblock->blocks.push_back(pbranchblock);
				break;
			}
			case 0x28: // IBEQ
			case 0x2f: // IBGEZ
			case 0x2d: // IBGTZ
			case 0x2e: // IBLEZ
			case 0x2c: // IBLTZ
			case 0x29: // IBNE
			{
				VuBaseBlock* pbranchblock = SuperVUBuildBlocks(pblock, bpc, newpipes);

				// update pblock since could have changed
				pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				pblock->blocks.push_back(pbranchblock);

                // only add the block if the previous branch doesn't include the next instruction (ie, if a direct jump)
                if( firstbranch == 0x24 || firstbranch == 0x25 || firstbranch == 0x20 || firstbranch == 0x21 ) {
				    pbranchblock = SuperVUBuildBlocks(pblock, lastpc, newpipes);

				    pblock = recVUBlocks[s_vu][lastpc/8-2].pblock;
				    pblock->blocks.push_back(pbranchblock);
                }

				break;
			}
			default:
				assert(0);
		}
	}

	return recVUBlocks[s_vu][startpc/8].pblock;
}

static void SuperVUInitLiveness(VuBaseBlock* pblock)
{
	list<VuInstruction>::iterator itinst, itnext;

	assert( pblock->insts.size() > 0 );

	for(itinst = pblock->insts.begin(); itinst != pblock->insts.end(); ++itinst) {

		if( itinst->type & INST_DUMMY_ ) {
			itinst->addvars[0] = itinst->addvars[1] = 0xffffffff;
			itinst->livevars[0] = itinst->livevars[1] = 0xffffffff;
			itinst->keepvars[0] = itinst->keepvars[1] = 0xffffffff;
			itinst->usedvars[0] = itinst->usedvars[1] = 0;
		}
		else {
			itinst->addvars[0] = itinst->regs[0].VIread | itinst->regs[1].VIread;
			itinst->addvars[1] = (itinst->regs[0].VFread0  ? (1 << itinst->regs[0].VFread0) : 0) | 
				(itinst->regs[0].VFread1  ? (1 << itinst->regs[0].VFread1) : 0) | 
				(itinst->regs[1].VFread0  ? (1 << itinst->regs[1].VFread0) : 0) | 
				(itinst->regs[1].VFread1  ? (1 << itinst->regs[1].VFread1) : 0);

			// vf0 is not handled by VFread
			if( !itinst->regs[0].VFread0 && (itinst->regs[0].VIread & (1<<REG_VF0_FLAG)) ) itinst->addvars[1] |= 1;
			if( !itinst->regs[1].VFread0 && (itinst->regs[1].VIread & (1<<REG_VF0_FLAG)) ) itinst->addvars[1] |= 1;
			if( !itinst->regs[0].VFread1 && (itinst->regs[0].VIread & (1<<REG_VF0_FLAG)) && itinst->regs[0].VFr1xyzw != 0xff ) itinst->addvars[1] |= 1;
			if( !itinst->regs[1].VFread1 && (itinst->regs[1].VIread & (1<<REG_VF0_FLAG)) && itinst->regs[1].VFr1xyzw != 0xff ) itinst->addvars[1] |= 1;
			
			
			u32 vfwrite = 0;
			if( itinst->regs[0].VFwrite != 0 ) {
				if( itinst->regs[0].VFwxyzw != 0xf ) itinst->addvars[1] |= 1<<itinst->regs[0].VFwrite;
				else vfwrite |= 1<<itinst->regs[0].VFwrite;
			}
			if( itinst->regs[1].VFwrite != 0 ) {
				if( itinst->regs[1].VFwxyzw != 0xf ) itinst->addvars[1] |= 1<<itinst->regs[1].VFwrite;
				else vfwrite |= 1<<itinst->regs[1].VFwrite;
			}
			if( (itinst->regs[1].VIwrite & (1<<REG_ACC_FLAG)) && itinst->regs[1].VFwxyzw != 0xf )
				itinst->addvars[1] |= 1<<REG_ACC_FLAG;

			u32 viwrite = (itinst->regs[0].VIwrite|itinst->regs[1].VIwrite);

			itinst->usedvars[0] = itinst->addvars[0]|viwrite;
			itinst->usedvars[1] = itinst->addvars[1]|vfwrite;

//			itinst->addvars[0] &= ~viwrite;
//			itinst->addvars[1] &= ~vfwrite;
			itinst->keepvars[0] = ~viwrite;
			itinst->keepvars[1] = ~vfwrite;
		}
	}

	itinst = --pblock->insts.end();
	while( itinst != pblock->insts.begin() ) {
		itnext = itinst; --itnext;

		itnext->usedvars[0] |= itinst->usedvars[0];
		itnext->usedvars[1] |= itinst->usedvars[1];

		itinst = itnext;
	}
}

u32 COMPUTE_LIVE(u32 R, u32 K, u32 L)
{
	u32 live = R | ((L)&(K));
	// speciall process mac and status flags
	// only propagate liveness if doesn't write to the flag
	if( !(L&(1<<REG_STATUS_FLAG)) && !(K&(1<<REG_STATUS_FLAG)) )
		live &= ~(1<<REG_STATUS_FLAG);
	if( !(L&(1<<REG_MAC_FLAG)) && !(K&(1<<REG_MAC_FLAG)) )
		live &= ~(1<<REG_MAC_FLAG);
	return live;//|(1<<REG_STATUS_FLAG)|(1<<REG_MAC_FLAG);
}

static void SuperVULivenessAnalysis()
{
	BOOL changed;
	list<VuBaseBlock*>::reverse_iterator itblock;
	list<VuInstruction>::iterator itinst, itnext;
	VuBaseBlock::LISTBLOCKS::iterator itchild;

	u32 livevars[2];

	do {
		changed = FALSE;
		for(itblock = s_listBlocks.rbegin(); itblock != s_listBlocks.rend(); ++itblock) {

			u32 newlive;
			VuBaseBlock* pb = *itblock;

			// the last inst relies on the neighbor's insts
			itinst = --pb->insts.end();
			
			if( pb->blocks.size() > 0 ) {
				livevars[0] = 0; livevars[1] = 0;
				for( itchild = pb->blocks.begin(); itchild != pb->blocks.end(); ++itchild) {
					VuInstruction& front = (*itchild)->insts.front();
					livevars[0] |= front.livevars[0];
					livevars[1] |= front.livevars[1];
				}

				newlive = COMPUTE_LIVE(itinst->addvars[0], itinst->keepvars[0], livevars[0]);
				if( itinst->livevars[0] != newlive ) {
					changed = TRUE;
					itinst->livevars[0] = newlive;
				}

				newlive = COMPUTE_LIVE(itinst->addvars[1], itinst->keepvars[1], livevars[1]);
				if( itinst->livevars[1] != newlive ) {
					changed = TRUE;
					itinst->livevars[1] = newlive;
				}
			}

			while( itinst != pb->insts.begin() ) {

				itnext = itinst; --itnext;

				newlive = COMPUTE_LIVE(itnext->addvars[0], itnext->keepvars[0], itinst->livevars[0]);

				if( itnext->livevars[0] != newlive ) {
					changed = TRUE;
					itnext->livevars[0] = newlive;
					itnext->livevars[1] = COMPUTE_LIVE(itnext->addvars[1], itnext->keepvars[1], itinst->livevars[1]);
				}
				else {
					newlive = COMPUTE_LIVE(itnext->addvars[1], itnext->keepvars[1], itinst->livevars[1]);
					if( itnext->livevars[1] != newlive ) {
						changed = TRUE;
						itnext->livevars[1] = newlive;
					}
				}

				itinst = itnext;
			}

//			if( (livevars[0] | itinst->livevars[0]) != itinst->livevars[0] ) {
//				changed = TRUE;
//				itinst->livevars[0] |= livevars[0];
//			}
//			if( (livevars[1] | itinst->livevars[1]) != itinst->livevars[1] ) {
//				changed = TRUE;
//				itinst->livevars[1] |= livevars[1];
//			}
//
//			while( itinst != pb->insts.begin() ) {
//
//				itnext = itinst; --itnext;
//				if( (itnext->livevars[0] | (itinst->livevars[0] & itnext->keepvars[0])) != itnext->livevars[0] ) {
//					changed = TRUE;
//					itnext->livevars[0] |= itinst->livevars[0] & itnext->keepvars[0];
//					itnext->livevars[1] |= itinst->livevars[1] & itnext->keepvars[1];
//				}
//				else if( (itnext->livevars[1] | (itinst->livevars[1] & itnext->keepvars[1])) != itnext->livevars[1] ) {
//					changed = TRUE;
//					itnext->livevars[1] |= itinst->livevars[1] & itnext->keepvars[1];
//				}
//
//				itinst = itnext;
//			}
		}

	} while(changed);
}

static void SuperVUEliminateDeadCode()
{
	list<VuBaseBlock*>::iterator itblock;
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	list<VuInstruction>::iterator itinst, itnext;
	list<VuInstruction*> listParents;
	list<VuInstruction*>::iterator itparent;

	FORIT(itblock, s_listBlocks) {

#ifdef _DEBUG
		u32 startpc = (*itblock)->startpc;
		u32 curpc = startpc;
#endif

		itnext = (*itblock)->insts.begin();
		itinst = itnext++;
		while(itnext != (*itblock)->insts.end() ) {
			if( itinst->type & (INST_CLIP_WRITE|INST_MAC_WRITE|INST_STATUS_WRITE) ) {
				itinst->regs[0].VIwrite &= itnext->livevars[0];
				itinst->regs[1].VIwrite &= itnext->livevars[0];
				u32 viwrite = itinst->regs[0].VIwrite|itinst->regs[1].VIwrite;

				(*itblock)->GetInstsAtPc(itinst->nParentPc, listParents);
				int removetype = 0;

				FORIT(itparent, listParents) {
					VuInstruction* parent = *itparent;

					if( viwrite & (1<<REG_CLIP_FLAG) ) {
						parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_CLIP_FLAG));
						parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_CLIP_FLAG));
					}
					else
						removetype |= INST_CLIP_WRITE;
					
					if( parent->info.macflag && (itinst->type & INST_MAC_WRITE) ) {
						if( !(viwrite&(1<<REG_MAC_FLAG)) ) {
							//parent->info.macflag = 0;
	//							parent->regs[0].VIwrite &= ~(1<<REG_MAC_FLAG);
	//							parent->regs[1].VIwrite &= ~(1<<REG_MAC_FLAG);
							// can be nonzero when a writeback belong to a different block and one branch uses
							// it and this one doesn't
#ifndef SUPERVU_WRITEBACKS
							assert( !(parent->regs[0].VIwrite & (1<<REG_MAC_FLAG)) && !(parent->regs[1].VIwrite & (1<<REG_MAC_FLAG)) );
#endif
							removetype |= INST_MAC_WRITE;
						}
						else {
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_MAC_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_MAC_FLAG));
						}
					}
					else removetype |= INST_MAC_WRITE;

					if( parent->info.statusflag && (itinst->type & INST_STATUS_WRITE)) {
						if( !(viwrite&(1<<REG_STATUS_FLAG)) ) {
							//parent->info.statusflag = 0;
	//							parent->regs[0].VIwrite &= ~(1<<REG_STATUS_FLAG);
	//							parent->regs[1].VIwrite &= ~(1<<REG_STATUS_FLAG);

							// can be nonzero when a writeback belong to a different block and one branch uses
							// it and this one doesn't
#ifndef SUPERVU_WRITEBACKS
							assert( !(parent->regs[0].VIwrite & (1<<REG_STATUS_FLAG)) && !(parent->regs[1].VIwrite & (1<<REG_STATUS_FLAG)) );
#endif
							removetype |= INST_STATUS_WRITE;
						}
						else {
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_STATUS_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_STATUS_FLAG));
						}
					}
					else removetype |= INST_STATUS_WRITE;
				}

				itinst->type &= ~removetype;
				if( itinst->type == 0 ) {
					itnext = (*itblock)->insts.erase(itinst);
					itinst = itnext++;
					continue;
				}
			}
#ifdef _DEBUG
			else curpc += 8;
#endif
			itinst = itnext;
			++itnext;
		}

		if( itinst->type & INST_DUMMY ) {
			// last inst with the children
			u32 mask = 0;
			for(itchild = (*itblock)->blocks.begin(); itchild != (*itblock)->blocks.end(); ++itchild) {
				mask |= (*itchild)->insts.front().livevars[0];
			}
			itinst->regs[0].VIwrite &= mask;
			itinst->regs[1].VIwrite &= mask;
			u32 viwrite = itinst->regs[0].VIwrite|itinst->regs[1].VIwrite;

			if( itinst->nParentPc >= 0 ) {

				(*itblock)->GetInstsAtPc(itinst->nParentPc, listParents);
				int removetype = 0;

				FORIT(itparent, listParents) {
					VuInstruction* parent = *itparent;

					if( viwrite & (1<<REG_CLIP_FLAG) ) {
						parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_CLIP_FLAG));
						parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_CLIP_FLAG));
					}
					else removetype |= INST_CLIP_WRITE;
					
					if( parent->info.macflag && (itinst->type & INST_MAC_WRITE) ) {
						if( !(viwrite&(1<<REG_MAC_FLAG)) ) {
							//parent->info.macflag = 0;
#ifndef SUPERVU_WRITEBACKS
							assert( !(parent->regs[0].VIwrite & (1<<REG_MAC_FLAG)) && !(parent->regs[1].VIwrite & (1<<REG_MAC_FLAG)) );
#endif
							removetype |= INST_MAC_WRITE;
						}
						else {
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_MAC_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_MAC_FLAG));
						}
					}
					else removetype |= INST_MAC_WRITE;

					if( parent->info.statusflag && (itinst->type & INST_STATUS_WRITE)) {
						if( !(viwrite&(1<<REG_STATUS_FLAG)) ) {
							//parent->info.statusflag = 0;
#ifndef SUPERVU_WRITEBACKS
							assert( !(parent->regs[0].VIwrite & (1<<REG_STATUS_FLAG)) && !(parent->regs[1].VIwrite & (1<<REG_STATUS_FLAG)) );
#endif
							removetype |= INST_STATUS_WRITE;
						}
						else {
							parent->regs[0].VIwrite |= (itinst->regs[0].VIwrite&(1<<REG_STATUS_FLAG));
							parent->regs[1].VIwrite |= (itinst->regs[1].VIwrite&(1<<REG_STATUS_FLAG));
						}
					}
					else removetype |= INST_STATUS_WRITE;
				}

				itinst->type &= ~removetype;
				if( itinst->type == 0 ) {
					(*itblock)->insts.erase(itinst);
				}
			}
		}
	}
}

// assigns xmm/x86 regs to all instructions, ignore mode field
// returns true if changed
bool AlignStartRegsToEndRegs(_xmmregs* startregs, const list<VuBaseBlock*>& parents)
{
	list<VuBaseBlock*>::const_iterator itblock, itblock2;
	int bestscore;
	_xmmregs bestregs;
	bool bchanged = false;

	// find the best merge of regs that minimizes writes/reads
	for(int i = 0; i < XMMREGS; ++i) {
		
		bestscore = 1000;
		memset(&bestregs, 0, sizeof(bestregs));

		FORIT(itblock, parents) {
			int curscore = 0;
			if( ((*itblock)->type & BLOCKTYPE_ANALYZED) && (*itblock)->endregs[i].inuse ) {
				int type = (*itblock)->endregs[i].type;
				int reg = (*itblock)->endregs[i].reg;

				FORIT(itblock2, parents) {
					if( (*itblock2)->type & BLOCKTYPE_ANALYZED ) {
						if( (*itblock2)->endregs[i].inuse ) {
							if( (*itblock2)->endregs[i].type != type || (*itblock2)->endregs[i].reg != reg ) {
								curscore += 1;
							}
						}
						else curscore++;
					}
				}
			}

			if( curscore < 1 && curscore < bestscore ) {
				memcpy(&bestregs, &(*itblock)->endregs[i], sizeof(bestregs));
				bestscore = curscore;
			}
		}

		if( bestscore < 1 ) {
			if( startregs[i].inuse == bestregs.inuse ) {
				if( bestregs.inuse && (startregs[i].type != bestregs.type || startregs[i].reg != bestregs.reg) )
					bchanged = true;
			}
			else bchanged = true;

			memcpy(&startregs[i], &bestregs, sizeof(bestregs));
			FORIT(itblock, parents) memcpy(&(*itblock)->endregs[i], &bestregs, sizeof(bestregs));
		}
		else {
			if( startregs[i].inuse ) bchanged = true;
			startregs[i].inuse = 0;
			FORIT(itblock, parents) (*itblock)->endregs[i].inuse = 0;
		}
	}

	return bchanged;
}

void VuBaseBlock::AssignVFRegs()
{
	int i;
	VuBaseBlock::LISTBLOCKS::iterator itchild;
	list<VuBaseBlock*>::iterator itblock;
	list<VuInstruction>::iterator itinst, itnext, itinst2;

	// init the start regs
	if( type & BLOCKTYPE_ANALYZED ) return; // nothing changed
	memcpy(xmmregs, startregs, sizeof(xmmregs));

	if( type & BLOCKTYPE_ANALYZED ) {
		// check if changed
		for(i = 0; i < XMMREGS; ++i) {
			if( xmmregs[i].inuse != startregs[i].inuse )
				break;
			if( xmmregs[i].inuse && (xmmregs[i].reg != startregs[i].reg || xmmregs[i].type != startregs[i].type) )
				break;
		}

		if( i == XMMREGS ) return; // nothing changed
	}

	s8* oldX86 = x86Ptr;

	FORIT(itinst, insts) {

		if( itinst->type & INST_DUMMY )
			continue;

		// reserve, go from upper to lower
		int lastwrite = -1;

		for(i = 1; i >= 0; --i) {
			_VURegsNum* regs = itinst->regs+i;

			// redo the counters so that the proper regs are released
			for(int j = 0; j < XMMREGS; ++j) {
				if( xmmregs[j].inuse ) {
					if( xmmregs[j].type == XMMTYPE_VFREG ) {
						int count = 0;
						itinst2 = itinst;

						if( i ) {
							if( itinst2->regs[0].VFread0 == xmmregs[j].reg || itinst2->regs[0].VFread1 == xmmregs[j].reg || itinst2->regs[0].VFwrite == xmmregs[j].reg ) {
								itinst2 = insts.end();
								break;
							}
							else {
								++count;
								++itinst2;
							}
						}

						while(itinst2 != insts.end() ) {
							if( itinst2->regs[0].VFread0 == xmmregs[j].reg || itinst2->regs[0].VFread1 == xmmregs[j].reg || itinst2->regs[0].VFwrite == xmmregs[j].reg ||
								itinst2->regs[1].VFread0 == xmmregs[j].reg || itinst2->regs[1].VFread1 == xmmregs[j].reg || itinst2->regs[1].VFwrite == xmmregs[j].reg)
								break;
							
							++count;
							++itinst2;
						}
						xmmregs[j].counter = 1000-count;
					}
					else {
						assert( xmmregs[j].type == XMMTYPE_ACC );

						int count = 0;
						itinst2 = itinst;

						if( i ) ++itinst2; // acc isn't used in lower insts

						while(itinst2 != insts.end() ) {
							assert( !((itinst2->regs[0].VIread|itinst2->regs[0].VIwrite) & (1<<REG_ACC_FLAG)) );

							if( (itinst2->regs[1].VIread|itinst2->regs[1].VIwrite) & (1<<REG_ACC_FLAG) )
								break;

							++count;
							++itinst2;
						}

						xmmregs[j].counter = 1000-count;
					}
				}
			}

			if( regs->VFread0 ) _addNeededVFtoXMMreg(regs->VFread0);
			if( regs->VFread1 ) _addNeededVFtoXMMreg(regs->VFread1);
			if( regs->VFwrite ) _addNeededVFtoXMMreg(regs->VFwrite);
			if( regs->VIread & (1<<REG_ACC_FLAG) ) _addNeededACCtoXMMreg();
			if( regs->VIread & (1<<REG_VF0_FLAG) ) _addNeededVFtoXMMreg(0);

			// alloc
			itinst->vfread0[i] = itinst->vfread1[i] = itinst->vfwrite[i] = itinst->vfacc[i] = -1;
			itinst->vfflush[i] = -1;

			if( regs->VFread0 ) itinst->vfread0[i] = _allocVFtoXMMreg(VU, -1, regs->VFread0, 0);
			else if( regs->VIread & (1<<REG_VF0_FLAG) ) itinst->vfread0[i] = _allocVFtoXMMreg(VU, -1, 0, 0);
			if( regs->VFread1 ) itinst->vfread1[i] = _allocVFtoXMMreg(VU, -1, regs->VFread1, 0);
			else if( (regs->VIread & (1<<REG_VF0_FLAG)) && regs->VFr1xyzw != 0xff  ) itinst->vfread1[i] = _allocVFtoXMMreg(VU, -1, 0, 0);
			if( regs->VIread & (1<<REG_ACC_FLAG) ) itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);

			int reusereg = -1; // 0 - VFwrite, 1 - VFAcc

			if( regs->VFwrite ) {
				assert( !(regs->VIwrite&(1<<REG_ACC_FLAG)) );

				if( regs->VFwxyzw == 0xf ) {
					itinst->vfwrite[i] = _checkXMMreg(XMMTYPE_VFREG, regs->VFwrite, 0);
					if( itinst->vfwrite[i] < 0 ) reusereg = 0;
				}
				else {
					itinst->vfwrite[i] = _allocVFtoXMMreg(VU, -1, regs->VFwrite, 0);
				}
			}
			else if( regs->VIwrite & (1<<REG_ACC_FLAG) ) {

				if( regs->VFwxyzw == 0xf ) {
					itinst->vfacc[i] = _checkXMMreg(XMMTYPE_ACC, 0, 0);
					if( itinst->vfacc[i] < 0 ) reusereg = 1;
				}
				else {
					itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);
				}
			}

			if( reusereg >= 0 ) {
				// reuse
				itnext = itinst; itnext++;

				u8 type = reusereg ? XMMTYPE_ACC : XMMTYPE_VFREG;
				u8 reg = reusereg ? 0 : regs->VFwrite;

				if( itinst->vfacc[i] >= 0 && lastwrite != itinst->vfacc[i] &&
					(itnext == insts.end() || ((regs->VIread&(1<<REG_ACC_FLAG)) && (!(itnext->usedvars[0]&(1<<REG_ACC_FLAG)) || !(itnext->livevars[0]&(1<<REG_ACC_FLAG))))) ) {

					assert( reusereg == 0 );
					if(itnext == insts.end() || (itnext->livevars[0]&(1<<REG_ACC_FLAG))) _freeXMMreg(itinst->vfacc[i]);
					xmmregs[itinst->vfacc[i]].inuse = 1;
					xmmregs[itinst->vfacc[i]].reg = reg;
					xmmregs[itinst->vfacc[i]].type = type;
					xmmregs[itinst->vfacc[i]].mode = 0;
					itinst->vfwrite[i] = itinst->vfacc[i];
				}
				else if( itinst->vfread0[i] >= 0 && lastwrite != itinst->vfread0[i] &&
					(itnext == insts.end() || (regs->VFread0 > 0 && (!(itnext->usedvars[1]&(1<<regs->VFread0)) || !(itnext->livevars[1]&(1<<regs->VFread0))))) ) {

					if(itnext == insts.end() || (itnext->livevars[1]&regs->VFread0)) _freeXMMreg(itinst->vfread0[i]);
					xmmregs[itinst->vfread0[i]].inuse = 1;
					xmmregs[itinst->vfread0[i]].reg = reg;
					xmmregs[itinst->vfread0[i]].type = type;
					xmmregs[itinst->vfread0[i]].mode = 0;
					if( reusereg ) itinst->vfacc[i] = itinst->vfread0[i];
					else itinst->vfwrite[i] = itinst->vfread0[i];
				}
				else if( itinst->vfread1[i] >= 0 && lastwrite != itinst->vfread1[i] &&
					(itnext == insts.end() || (regs->VFread1 > 0 && (!(itnext->usedvars[1]&(1<<regs->VFread1)) || !(itnext->livevars[1]&(1<<regs->VFread1))))) ) {

					if(itnext == insts.end() || (itnext->livevars[1]&regs->VFread1)) _freeXMMreg(itinst->vfread1[i]);
					xmmregs[itinst->vfread1[i]].inuse = 1;
					xmmregs[itinst->vfread1[i]].reg = reg;
					xmmregs[itinst->vfread1[i]].type = type;
					xmmregs[itinst->vfread1[i]].mode = 0;
					if( reusereg ) itinst->vfacc[i] = itinst->vfread1[i];
					else itinst->vfwrite[i] = itinst->vfread1[i];
				}
				else {
					if( reusereg ) itinst->vfacc[i] = _allocACCtoXMMreg(VU, -1, 0);
					else itinst->vfwrite[i] = _allocVFtoXMMreg(VU, -1, regs->VFwrite, 0);
				}
			}

			if( itinst->vfwrite[i] >= 0 ) lastwrite = itinst->vfwrite[i];
			else if( itinst->vfacc[i] >= 0 ) lastwrite = itinst->vfacc[i];

			// always alloc at least 1 temp reg
			int free0 = (i||regs->VFwrite||regs->VFread0||regs->VFread1||(regs->VIwrite&(1<<REG_ACC_FLAG)))?_allocTempXMMreg(XMMT_FPS, -1):-1;
			int free1=0, free2=0;

			if( i==0 && itinst->vfwrite[1] >= 0 && (itinst->vfread0[0]==itinst->vfwrite[1]||itinst->vfread1[0]==itinst->vfwrite[1]) ) {
				itinst->vfflush[i] = _allocTempXMMreg(XMMT_FPS, -1);
			}

			if( i == 1 && (regs->VIwrite & (1<<REG_CLIP_FLAG)) ) {
				// CLIP inst, need two extra regs
				if( free0 < 0 )
					free0 = _allocTempXMMreg(XMMT_FPS, -1);
				free1 = _allocTempXMMreg(XMMT_FPS, -1);
				free2 = _allocTempXMMreg(XMMT_FPS, -1);
				_freeXMMreg(free1);
				_freeXMMreg(free2);
			}
			else if( regs->VIwrite & (1<<REG_P) ) {
				free1 = _allocTempXMMreg(XMMT_FPS, -1);
				// protects against insts like esadd vf0
				if( free0 == -1 )
					free0 = free1;
				_freeXMMreg(free1);
			}

			if( itinst->vfflush[i] >= 0 ) _freeXMMreg(itinst->vfflush[i]);
			if( free0 >= 0 ) _freeXMMreg(free0);

			itinst->vffree[i] = (free0&0xf)|(free1<<8)|(free2<<16);

			_clearNeededXMMregs();
		}
	}

	assert( x86Ptr == oldX86 );
	u32 analyzechildren = !(type&BLOCKTYPE_ANALYZED);
	type |= BLOCKTYPE_ANALYZED;

	//memset(endregs, 0, sizeof(endregs));

	if( analyzechildren ) {
		FORIT(itchild, blocks) (*itchild)->AssignVFRegs();
	}
}

struct MARKOVBLANKET
{
	list<VuBaseBlock*> parents;
	list<VuBaseBlock*> children;
};

static MARKOVBLANKET s_markov;

void VuBaseBlock::AssignVIRegs(int parent)
{
	const int maxregs = 6;

	if( parent ) {
		if( (type&BLOCKTYPE_ANALYZEDPARENT) )
			return;

		type |= BLOCKTYPE_ANALYZEDPARENT;
		s_markov.parents.push_back(this);
		for(LISTBLOCKS::iterator it = blocks.begin(); it != blocks.end(); ++it) {
			(*it)->AssignVIRegs(0);
		}
		return;
	}

	if( (type&BLOCKTYPE_ANALYZED) )
		return;

	// child
	assert( allocX86Regs == -1 );
	allocX86Regs = s_vecRegArray.size();
	s_vecRegArray.resize(allocX86Regs+X86REGS);

	_x86regs* pregs = &s_vecRegArray[allocX86Regs];
	memset(pregs, 0, sizeof(_x86regs)*X86REGS);

	assert( parents.size() > 0 );

	list<VuBaseBlock*>::iterator itparent;
	u32 usedvars = insts.front().usedvars[0];
	u32 livevars = insts.front().livevars[0];

	if( parents.size() > 0 ) {
		u32 usedvars2 = 0xffffffff;
		FORIT(itparent, parents) usedvars2 &= (*itparent)->insts.front().usedvars[0];
		usedvars |= usedvars2;
	}

	usedvars &= livevars;

	// currently order doesn't matter
	int num = 0;

	if( usedvars ) {
		for(int i = 1; i < 16; ++i) {
			if( usedvars & (1<<i) ) {
				pregs[num].inuse = 1;
				pregs[num].reg = i;

				livevars &= ~(1<<i);

				if( ++num >= maxregs ) break;
			}
		}
	}

	if( num < maxregs) {
		livevars &= ~usedvars;
		livevars &= insts.back().usedvars[0];

		if( livevars ) {
			for(int i = 1; i < 16; ++i) {
				if( livevars & (1<<i) ) {
					pregs[num].inuse = 1;
					pregs[num].reg = i;

					if( ++num >= maxregs) break;
				}
			}
		}
	}

	s_markov.children.push_back(this);
	type |= BLOCKTYPE_ANALYZED;
	FORIT(itparent, parents) {
		(*itparent)->AssignVIRegs(1);
	}
}

u32 VuBaseBlock::GetModeXYZW(u32 curpc, int vfreg)
{
	if( vfreg <= 0 ) return false;

	list<VuInstruction>::iterator itinst = insts.begin();
	advance(itinst, (curpc-startpc)/8);

	u8 mxy = 1;
	u8 mxyz = 1;

	while(itinst != insts.end()) {
		for(int i = 0; i < 2; ++i ) {
			if( itinst->regs[i].VFwrite == vfreg ) {
				if( itinst->regs[i].VFwxyzw != 0xe ) mxyz = 0;
				if( itinst->regs[i].VFwxyzw != 0xc ) mxy = 0;
			}
			if( itinst->regs[i].VFread0 == vfreg ) {
				if( itinst->regs[i].VFr0xyzw != 0xe ) mxyz = 0;
				if( itinst->regs[i].VFr0xyzw != 0xc ) mxy = 0;
			}
			if( itinst->regs[i].VFread1 == vfreg ) {
				if( itinst->regs[i].VFr1xyzw != 0xe ) mxyz = 0;
				if( itinst->regs[i].VFr1xyzw != 0xc ) mxy = 0;
			}

			if( !mxy && !mxyz ) return 0;
		}
		++itinst;
	}

	return (mxy?MODE_VUXY:0)|(mxyz?MODE_VUXYZ:0);
}

static void SuperVUAssignRegs()
{
	list<VuBaseBlock*>::iterator itblock, itblock2;

	// assign xyz regs
//	FORIT(itblock, s_listBlocks) {
//		(*itblock)->vuxyz = 0;
//		(*itblock)->vuxy = 0;
//
//		for(int i = 0; i < 32; ++i) {
//			u32 mode = (*itblock)->GetModeXYZW((*itblock)->startpc, i);
//			if( mode & MODE_VUXYZ ) {
//				 if( mode & MODE_VUZ ) (*itblock)->vuxyz |= 1<<i;
//				 else (*itblock)->vuxy |= 1<<i;
//			}
//		}
//	}

	FORIT(itblock, s_listBlocks) (*itblock)->type &= ~BLOCKTYPE_ANALYZED;
	s_listBlocks.front()->AssignVFRegs();

	// VI assignments, find markov blanket for each node in the graph
	// then allocate regs based on the commonly used ones
#ifdef SUPERVU_X86CACHING
	FORIT(itblock, s_listBlocks) (*itblock)->type &= ~(BLOCKTYPE_ANALYZED|BLOCKTYPE_ANALYZEDPARENT);
	s_vecRegArray.resize(0);
	u8 usedregs[16];

	// note: first block always has to start with no alloc regs
	bool bfirst = true;

	FORIT(itblock, s_listBlocks) {

		if( !((*itblock)->type & BLOCKTYPE_ANALYZED) ) {

			if( (*itblock)->parents.size() == 0 ) {
				(*itblock)->type |= BLOCKTYPE_ANALYZED;
				bfirst = false;
				continue;
			}

			s_markov.children.clear();
			s_markov.parents.clear();
			(*itblock)->AssignVIRegs(0);

			// assign the regs
			int regid = s_vecRegArray.size();
			s_vecRegArray.resize(regid+X86REGS);

			_x86regs* mergedx86 = &s_vecRegArray[regid];
			memset(mergedx86, 0, sizeof(_x86regs)*X86REGS);

			if( !bfirst ) {
				*(u32*)usedregs = *((u32*)usedregs+1) = *((u32*)usedregs+2) = *((u32*)usedregs+3) = 0;

				FORIT(itblock2, s_markov.children) {
					assert( (*itblock2)->allocX86Regs >= 0 );
					_x86regs* pregs = &s_vecRegArray[(*itblock2)->allocX86Regs];
					for(int i = 0; i < X86REGS; ++i) {
						if( pregs[i].inuse && pregs[i].reg < 16) {
							//assert( pregs[i].reg < 16);
							usedregs[pregs[i].reg]++;
						}
					}
				}

				int num = 1;
				for(int i = 0; i < 16; ++i) {
					if( usedregs[i] == s_markov.children.size() ) {
						// use
						mergedx86[num].inuse = 1;
						mergedx86[num].reg = i;
						mergedx86[num].type = (s_vu?X86TYPE_VU1:0)|X86TYPE_VI;
						mergedx86[num].mode = MODE_READ;
						if( ++num >= X86REGS )
							break;
						if( num == ESP )
							++num;
					}
				}

				FORIT(itblock2, s_markov.children) {
					assert( (*itblock2)->nStartx86 == -1 );
					(*itblock2)->nStartx86 = regid;
				}

				FORIT(itblock2, s_markov.parents) {
					assert( (*itblock2)->nEndx86 == -1 );
					(*itblock2)->nEndx86 = regid;
				}
			}

			bfirst = false;
		}
	}
#endif
}

//////////////////
// Recompilation
//////////////////

// cycles in which the last Q,P regs were finished (written to VU->VI[])
// the write occurs before the instruction is executed at that cycle
// compare with s_TotalVUCycles
// if less than 0, already flushed
static int s_writeQ, s_writeP;
static int s_recWriteQ, s_recWriteP; // wait times during recompilation
static int s_needFlush; // first bit - Q, second bit - P, third bit - Q has been written, fourth bit - P has been written

static u32 s_vu1ebp, s_vuedi, s_vuebx, s_vu1esp, s_vu1esi, s_callstack;//, s_vu1esp
static u32 s_ssecsr;
static int s_JumpX86;
static int s_ScheduleXGKICK = 0, s_XGKICKReg = -1;

extern "C" u32 g_sseVUMXCSR, g_sseMXCSR;

void recSVUMI_XGKICK_( VURegs *VU );

// entry point of all vu programs from emulator calls
__declspec(naked) void SuperVUExecuteProgram(u32 startpc, int vuindex)
{
#ifdef SUPERVU_COUNT
	QueryPerformanceCounter(&svubase);
#endif
	__asm {
		mov eax, dword ptr [esp]
		mov s_TotalVUCycles, 0 // necessary to be here!
		add esp, 4
		mov s_callstack, eax
		call SuperVUGetProgram

		// save cpu state
		mov s_vu1ebp, ebp
		mov s_vu1esi, esi // have to save even in Release
		mov s_vuedi, edi // have to save even in Release
		mov s_vuebx, ebx
	}
#ifdef _DEBUG
	__asm {
		mov s_vu1esp, esp
	}
#endif

	__asm {
		//stmxcsr s_ssecsr
		ldmxcsr g_sseVUMXCSR

		// init vars
		mov s_writeQ, 0xffffffff
		mov s_writeP, 0xffffffff

		jmp eax
	}
}

static void SuperVUCleanupProgram(u32 startpc, int vuindex)
{
#ifdef SUPERVU_COUNT
	QueryPerformanceCounter(&svufinal);
	svutime += (u32)(svufinal.QuadPart-svubase.QuadPart);
#endif

#ifdef _DEBUG
	assert( s_vu1esp == 0 );
#endif

	VU = vuindex ? &VU1 : &VU0;
	VU->cycle += s_TotalVUCycles;	
	if( (int)s_writeQ > 0 ) VU->VI[REG_Q] = VU->q;
	if( (int)s_writeP > 0 ) {
		assert(VU == &VU1);
		VU1.VI[REG_P] = VU1.p; // only VU1
	}
}

// exit point of all vu programs
__declspec(naked) static void SuperVUEndProgram()
{
	__asm {
		// restore cpu state
		ldmxcsr g_sseMXCSR

		mov ebp, s_vu1ebp
		mov esi, s_vu1esi
		mov edi, s_vuedi
		mov ebx, s_vuebx
	}

#ifdef _DEBUG
	__asm {
		sub s_vu1esp, esp
	}
#endif

	__asm {
		call SuperVUCleanupProgram
		jmp s_callstack // so returns correctly
	}
}

// Flushes P/Q regs
void SuperVUFlush(int p, int wait)
{
	u8* pjmp[3];
	if( !(s_needFlush&(1<<p)) ) return;

	int recwait = p ? s_recWriteP : s_recWriteQ;
	if( !wait && s_pCurInst->info.cycle < recwait ) return;

	if( recwait == 0 ) {
		// write didn't happen this block
		MOV32MtoR(EAX, p ? (u32)&s_writeP : (u32)&s_writeQ);
		OR32RtoR(EAX, EAX);
		pjmp[0] = JS8(0);

		if( s_pCurInst->info.cycle ) SUB32ItoR(EAX, s_pCurInst->info.cycle);

		// if writeQ <= total+offset
		if( !wait ) { // only write back if time is up
			CMP32MtoR(EAX, (u32)&s_TotalVUCycles);
			pjmp[1] = JG8(0);
		}
		else {
			// add (writeQ-total-offset) to s_TotalVUCycles
			// necessary?
			CMP32MtoR(EAX, (u32)&s_TotalVUCycles);
			pjmp[2] = JLE8(0);
			MOV32RtoM((u32)&s_TotalVUCycles, EAX);
			x86SetJ8(pjmp[2]);
		}
	}
	else if( wait && s_pCurInst->info.cycle < recwait ) {
		ADD32ItoM((u32)&s_TotalVUCycles, recwait);
	}

	MOV32MtoR(EAX, SuperVUGetVIAddr(p?REG_P:REG_Q, 0));
	MOV32ItoM(p ? (u32)&s_writeP : (u32)&s_writeQ, 0x80000000);
	MOV32RtoM(SuperVUGetVIAddr(p?REG_P:REG_Q, 1), EAX);

	if( recwait == 0 ) {
		if( !wait ) x86SetJ8(pjmp[1]);
		x86SetJ8(pjmp[0]);
	}

	if( wait || (!p && recwait == 0 && s_pCurInst->info.cycle >= 12) || (!p && recwait > 0 && s_pCurInst->info.cycle >= recwait ) )
		s_needFlush &= ~(1<<p);
}

// executed only once per program
static u32* SuperVUStaticAlloc(u32 size)
{
	assert( recVUStackPtr+size <= recVUStack+SUPERVU_STACKSIZE );
	// always zero
	if( size == 4 ) *(u32*)recVUStackPtr = 0;
	else memset(recVUStackPtr, 0, size);
	recVUStackPtr += size;
	return (u32*)(recVUStackPtr-size);
}

static void SuperVURecompile()
{
	// save cpu state
	recVUStackPtr = recVUStack;

	_initXMMregs();

	list<VuBaseBlock*>::iterator itblock;

	FORIT(itblock, s_listBlocks) (*itblock)->type &= ~BLOCKTYPE_ANALYZED;
	s_listBlocks.front()->Recompile();
	// make sure everything compiled
	FORIT(itblock, s_listBlocks) assert( ((*itblock)->type & BLOCKTYPE_ANALYZED) && (*itblock)->pcode != NULL );

	// link all blocks
	FORIT(itblock, s_listBlocks) {
		VuBaseBlock::LISTBLOCKS::iterator itchild;

		assert( (*itblock)->blocks.size() <= ARRAYSIZE((*itblock)->pChildJumps) );

		int i = 0;
		FORIT(itchild, (*itblock)->blocks) {

			if( (u32)(*itblock)->pChildJumps[i] == 0xffffffff )
				continue;

			if( (*itblock)->pChildJumps[i] == NULL ) {
				VuBaseBlock* pchild = *itchild;

				if( pchild->type & BLOCKTYPE_HASEOP) {
					assert( pchild->blocks.size() == 0);

					AND32ItoM( (u32)&VU0.VI[ REG_VPU_STAT ].UL, s_vu?~0x100:~0x001 ); // E flag 
					AND32ItoM( (u32)&VU->vifRegs->stat, ~0x4 );

					MOV32ItoM((u32)&VU->VI[REG_TPC], pchild->endpc);
					JMP32( (u32)SuperVUEndProgram - ( (u32)x86Ptr + 5 ));
				}
				// only other case is when there are two branches
				else assert( (*itblock)->insts.back().regs[0].pipe == VUPIPE_BRANCH );

				continue;
			}

			if( (u32)(*itblock)->pChildJumps[i] & 0x80000000 ) {
				// relative
				*(u32*)&(*itblock)->pChildJumps[i] &= 0x7fffffff;
				*(*itblock)->pChildJumps[i] = (u32)(*itchild)->pcode - ((u32)(*itblock)->pChildJumps[i] + 4);
			}
			else *(*itblock)->pChildJumps[i] = (u32)(*itchild)->pcode;

			++i;
		}
	}

	s_pFnHeader->pprogfunc = s_listBlocks.front()->pcode;
}

static u32 s_svulast = 0, s_vufnheader;
extern "C" u32 s_vucount = 0;
static u32 lastrec = 0, skipparent = -1;
static u32 s_saveecx, s_saveedx, s_saveebx, s_saveesi, s_saveedi, s_saveebp;
static u32 badaddrs[][2] = {0,0xffff};
				
__declspec(naked) static void svudispfn()
{
	static u32 i;
	static u32 curvu;

	__asm {
		mov curvu, eax
		mov s_saveecx, ecx
		mov s_saveedx, edx
		mov s_saveebx, ebx
		mov s_saveesi, esi
		mov s_saveedi, edi
		mov s_saveebp, ebp
	}

//	for(i = 1; i < 32; ++i) {
//		if( (VU1.VF[i].UL[3]&0x7f800000) == 0x7f800000 ) VU1.VF[i].UL[3] &= 0xff7fffff;
//		if( (VU1.VF[i].UL[2]&0x7f800000) == 0x7f800000 ) VU1.VF[i].UL[2] &= 0xff7fffff;
//		if( (VU1.VF[i].UL[1]&0x7f800000) == 0x7f800000 ) VU1.VF[i].UL[1] &= 0xff7fffff;
//		if( (VU1.VF[i].UL[0]&0x7f800000) == 0x7f800000 ) VU1.VF[i].UL[0] &= 0xff7fffff;
//	}

    if( ((vudump&8) && curvu) || ((vudump&0x80) && !curvu) ) { //&& lastrec != g_vu1last ) {

		if( skipparent != lastrec ) {
			for(i = 0; i < ARRAYSIZE(badaddrs); ++i) {
				if( s_svulast == badaddrs[i][1] && lastrec == badaddrs[i][0] )
					break;
			}
			
			if( i == ARRAYSIZE(badaddrs) )
			{
                static int curesp;
                __asm mov curesp, esp
				__Log("tVU: %x\n", s_svulast, s_vucount);
				if( curvu ) iDumpVU1Registers();
				else iDumpVU0Registers();
				s_vucount++;
			}
		}

		lastrec = s_svulast;
	}

	__asm {
		mov ecx, s_saveecx
		mov edx, s_saveedx
		mov ebx, s_saveebx
		mov esi, s_saveesi
		mov edi, s_saveedi
		mov ebp, s_saveebp
		ret
	}
}

// frees an xmmreg depending on the liveness info of hte current inst
//void SuperVUFreeXMMreg(int xmmreg, int xmmtype, int reg)
//{
//	if( !xmmregs[xmmreg].inuse ) return;
//	if( xmmregs[xmmreg].type == xmmtype && xmmregs[xmmreg].reg == reg ) return;
//
//	if( s_pNextInst == NULL ) {
//		// last inst, free
//		_freeXMMreg(xmmreg);
//		return;
//	}
//
//	if( xmmregs[xmmreg].type == XMMTYPE_VFREG ) {
//		if( (s_pCurInst->livevars[1]|s_pNextInst->livevars[1]) & (1<<xmmregs[xmmreg].reg) )
//			_freeXMMreg(xmmreg);
//		else
//			xmmregs[xmmreg].inuse = 0;
//	}
//	else if( xmmregs[xmmreg].type == XMMTYPE_ACC ) {
//		if( (s_pCurInst->livevars[0]|s_pNextInst->livevars[0]) & (1<<REG_ACC_FLAG) )
//			_freeXMMreg(xmmreg);
//		else
//			xmmregs[xmmreg].inuse = 0;
//	}
//}

// frees all regs taking into account the livevars
void SuperVUFreeXMMregs(u32* livevars)
{
	for(int i = 0; i < XMMREGS; ++i) {
		if( xmmregs[i].inuse ) {
			// same reg
			if( (xmmregs[i].mode & MODE_WRITE) ) {

#ifdef SUPERVU_INTERCACHING
				if( xmmregs[i].type == XMMTYPE_VFREG ) {
					if( !(livevars[1] & (1<<xmmregs[i].reg)) )
						continue;
				}
				else if( xmmregs[i].type == XMMTYPE_ACC ) {
					if( !(livevars[0] & (1<<REG_ACC_FLAG)) )
						continue;
				}
#endif

				if( xmmregs[i].mode & MODE_VUXYZ ) {
					// ALWAYS update
					u32 addr = xmmregs[i].type == XMMTYPE_VFREG ? (u32)&VU->VF[xmmregs[i].reg] : (u32)&VU->ACC;

					if( xmmregs[i].mode & MODE_VUZ ) {
						SSE_MOVHPS_XMM_to_M64(addr, (x86SSERegType)i);
						SSE_SHUFPS_M128_to_XMM((x86SSERegType)i, addr, 0xc4);
					}
					else SSE_MOVHPS_M64_to_XMM((x86SSERegType)i, addr+8);

					xmmregs[i].mode &= ~MODE_VUXYZ;
				}
				
				_freeXMMreg(i);
			}
		}
	}

	//_freeXMMregs();
}

//void timeout() { SysPrintf("VU0 timeout\n"); }
void SuperVUTestVU0Condition(u32 incstack)
{
	if( s_vu ) return; // vu0 only

	CMP32ItoM((u32)&s_TotalVUCycles, 512);	// sometimes games spin on vu0, so be careful with this value
											// woody hangs if too high

	if( incstack ) {
		u8* ptr = JB8(0);

		if( incstack ) ADD32ItoR(ESP, incstack);
		//CALLFunc((u32)timeout);
		JMP32( (u32)SuperVUEndProgram - ( (u32)x86Ptr + 5 ));

		x86SetJ8(ptr);
	}
	else {
		JAE32( (u32)SuperVUEndProgram - ( (u32)x86Ptr + 6 ) );
	}
}

void VuBaseBlock::Recompile()
{
	if( type & BLOCKTYPE_ANALYZED ) return;
	
	x86Align(16);
	pcode = x86Ptr;

#ifdef _DEBUG
	MOV32ItoM((u32)&s_vufnheader, s_pFnHeader->startpc);
	MOV32ItoM((u32)&VU->VI[REG_TPC], startpc);
	MOV32ItoM((u32)&s_svulast, startpc);
	list<VuBaseBlock*>::iterator itparent;
	for(itparent = parents.begin(); itparent != parents.end(); ++itparent) {
		if( (*itparent)->blocks.size()==1 && (*itparent)->blocks.front()->startpc == startpc &&
			((*itparent)->insts.size() < 2 || (----(*itparent)->insts.end())->regs[0].pipe != VUPIPE_BRANCH) ) {
			MOV32ItoM((u32)&skipparent, (*itparent)->startpc);
			break;
		}
	}

	if( itparent == parents.end() ) MOV32ItoM((u32)&skipparent, -1);

	MOV32ItoR(EAX, s_vu);
	CALLFunc((u32)svudispfn);
#endif

	s_pCurBlock = this;
	s_needFlush = 3;
	pc = startpc;
	branch = 0;
	s_recWriteQ = s_recWriteP = 0;
	s_XGKICKReg = -1;
	s_ScheduleXGKICK = 0;

	s_ClipRead = s_PrevClipWrite = (u32)&VU->VI[REG_CLIP_FLAG];
	s_StatusRead = s_PrevStatusWrite = (u32)&VU->VI[REG_STATUS_FLAG];
	s_MACRead = s_PrevMACWrite = (u32)&VU->VI[REG_MAC_FLAG];
	s_PrevIWrite = (u32)&VU->VI[REG_I];
	s_JumpX86 = 0;
    s_UnconditionalDelay = 0;

	memcpy(xmmregs, startregs, sizeof(xmmregs));
#ifdef SUPERVU_X86CACHING
	if( nStartx86 >= 0 )
		memcpy(x86regs, &s_vecRegArray[nStartx86], sizeof(x86regs));
	else _initX86regs();
#else
	_initX86regs();
#endif

	list<VuInstruction>::iterator itinst;
	FORIT(itinst, insts) {
		s_pCurInst = &(*itinst);
		if( s_JumpX86 > 0 ) {
			if( !x86regs[s_JumpX86].inuse ) {
				// load
				s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_READ);
			}
			x86regs[s_JumpX86].needed = 1; 
		}
		if( s_ScheduleXGKICK && s_XGKICKReg > 0 ) {
			assert( x86regs[s_XGKICKReg].inuse );
			x86regs[s_XGKICKReg].needed = 1;
		}
		itinst->Recompile(itinst, vuxyz);

		if( s_ScheduleXGKICK > 0 ) {
			if( s_ScheduleXGKICK-- == 1 ) {
				recSVUMI_XGKICK_(VU);
			}
		}
	}
	assert( pc == endpc );
	assert( s_ScheduleXGKICK == 0 );

	// flush flags
	if( s_PrevClipWrite != (u32)&VU->VI[REG_CLIP_FLAG] ) {
		MOV32MtoR(EAX, s_PrevClipWrite);
		MOV32RtoM((u32)&VU->VI[REG_CLIP_FLAG], EAX);
	}
	if( s_PrevStatusWrite != (u32)&VU->VI[REG_STATUS_FLAG] ) {
		MOV32MtoR(EAX, s_PrevStatusWrite);
		MOV32RtoM((u32)&VU->VI[REG_STATUS_FLAG], EAX);
	}
	if( s_PrevMACWrite != (u32)&VU->VI[REG_MAC_FLAG] ) {
		MOV32MtoR(EAX, s_PrevMACWrite);
		MOV32RtoM((u32)&VU->VI[REG_MAC_FLAG], EAX);
	}
	if( s_PrevIWrite != (u32)&VU->VI[REG_I] ) {
		MOV32ItoM((u32)&VU->VI[REG_I], *(u32*)s_PrevIWrite); // never changes
	}

	ADD32ItoM((u32)&s_TotalVUCycles, cycles);

	// compute branches, jumps, eop
	if( type & BLOCKTYPE_HASEOP ) {
		// end
		_freeXMMregs();
		_freeX86regs();
		AND32ItoM( (u32)&VU0.VI[ REG_VPU_STAT ].UL, s_vu?~0x100:~0x001 ); // E flag 
		AND32ItoM( (u32)&VU->vifRegs->stat, ~0x4 );
		if( !branch ) MOV32ItoM((u32)&VU->VI[REG_TPC], endpc);
		JMP32( (u32)SuperVUEndProgram - ( (u32)x86Ptr + 5 ));
	}
	else {

		u32 livevars[2] = {0};

		list<VuInstruction>::iterator lastinst = GetInstIterAtPc(endpc-8);
		lastinst++;

		if( lastinst != insts.end() ) {
			livevars[0] = lastinst->livevars[0];
			livevars[1] = lastinst->livevars[1];
		}
		else {
			// take from children
			if( blocks.size() > 0 ) {
				LISTBLOCKS::iterator itchild;
				FORIT(itchild, blocks) {
					livevars[0] |= (*itchild)->insts.front().livevars[0];
					livevars[1] |= (*itchild)->insts.front().livevars[1];
				}
			}
			else {
				livevars[0] = ~0;
				livevars[1] = ~0;
			}
		}

		SuperVUFreeXMMregs(livevars);

		// get rid of any writes, otherwise _freeX86regs will write
		x86regs[s_JumpX86].mode &= ~MODE_WRITE;

		if( branch == 1 ) {
			if( !x86regs[s_JumpX86].inuse ) {
				assert( x86regs[s_JumpX86].type == X86TYPE_VUJUMP );
				s_JumpX86 = 0xffffffff; // notify to jump from g_recWriteback
			}
		}

		// align VI regs
#ifdef SUPERVU_X86CACHING
		if( nEndx86 >= 0 ) {
			_x86regs* endx86 = &s_vecRegArray[nEndx86];
			for(int i = 0; i < X86REGS; ++i) {
				if( endx86[i].inuse ) {

					if( s_JumpX86 == i && x86regs[s_JumpX86].inuse ) {
						x86regs[s_JumpX86].inuse = 0;
						x86regs[EAX].inuse = 1;
						MOV32RtoR(EAX, s_JumpX86);
						s_JumpX86 = EAX;
					}

					if( x86regs[i].inuse ) {
						if( x86regs[i].type == endx86[i].type && x86regs[i].reg == endx86[i].reg ) {
							_freeX86reg(i);
							// will continue to use it
							continue;
						}

						if( x86regs[i].type == (X86TYPE_VI|(s_vu?X86TYPE_VU1:0)) ) {
#ifdef SUPERVU_INTERCACHING
							if( livevars[0] & (1<<x86regs[i].reg) )
								_freeX86reg(i);
							else
								x86regs[i].inuse = 0;
#else
							_freeX86reg(i);
#endif
						}
						else _freeX86reg(i);
					}

					// realloc
					_allocX86reg(i, endx86[i].type, endx86[i].reg, MODE_READ);
					if( x86regs[i].mode & MODE_WRITE ) {
						_freeX86reg(i);
						x86regs[i].inuse = 1;
					}
				}
				else _freeX86reg(i);
			}
		}
		else _freeX86regs();
#else
		_freeX86regs();
#endif

		switch(branch) {
			case 1: // branch, esi has new prog

				SuperVUTestVU0Condition(0);

				if( s_JumpX86 == 0xffffffff )
					JMP32M((u32)&g_recWriteback);
				else
					JMP32R(s_JumpX86);

				break;
			case 4: // jalr
				pChildJumps[0] = (u32*)0xffffffff;
				// fall through

			case 0x10: // jump, esi has new vupc
			{
				_freeXMMregs();
				_freeX86regs();
				
				SuperVUTestVU0Condition(8);

				// already onto stack
				CALLFunc((u32)SuperVUGetProgram);
				ADD32ItoR(ESP, 8);
				JMP32R(EAX);

				break;
			}
            
            case 0x13: // jr with uncon branch, uncond branch takes precendence (dropship)
            {
//                s32 delta = (s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff) << 3;
//                ADD32ItoRmOffset(ESP, delta, 0);
                ADD32ItoR(ESP, 8); // restore
                pChildJumps[0] = (u32*)((u32)JMP32(0)|0x80000000);

				break;
            }
			case 0:
            case 3: // unconditional branch
                pChildJumps[s_UnconditionalDelay] = (u32*)((u32)JMP32(0)|0x80000000);
				break;

			default:
#ifdef PCSX2_DEVBUILD
				SysPrintf("Bad branch %x\n", branch);
#endif
				assert(0);
				break;
		}
	}

	type |= BLOCKTYPE_ANALYZED;

	LISTBLOCKS::iterator itchild;
	FORIT(itchild, blocks) {
		(*itchild)->Recompile();
	}
}

#define GET_VUXYZMODE(reg) 0//((vuxyz&(1<<(reg)))?MODE_VUXYZ:0)

int VuInstruction::SetCachedRegs(int upper, u32 vuxyz)
{
	if( vfread0[upper] >= 0 ) {
		SuperVUFreeXMMreg(vfread0[upper], XMMTYPE_VFREG, regs[upper].VFread0);
		_allocVFtoXMMreg(VU, vfread0[upper], regs[upper].VFread0, MODE_READ|GET_VUXYZMODE(regs[upper].VFread0));
	}
	if( vfread1[upper] >= 0 ) {
		SuperVUFreeXMMreg(vfread1[upper], XMMTYPE_VFREG, regs[upper].VFread1);
		_allocVFtoXMMreg(VU, vfread1[upper], regs[upper].VFread1, MODE_READ|GET_VUXYZMODE(regs[upper].VFread1));
	}
	if( vfacc[upper] >= 0 && (regs[upper].VIread&(1<<REG_ACC_FLAG))) {
		SuperVUFreeXMMreg(vfacc[upper], XMMTYPE_ACC, 0);
		_allocACCtoXMMreg(VU, vfacc[upper], MODE_READ);
	}
	if( vfwrite[upper] >= 0 ) {
		assert( regs[upper].VFwrite > 0);
		SuperVUFreeXMMreg(vfwrite[upper], XMMTYPE_VFREG, regs[upper].VFwrite);
		_allocVFtoXMMreg(VU, vfwrite[upper], regs[upper].VFwrite,
			MODE_WRITE|(regs[upper].VFwxyzw != 0xf?MODE_READ:0)|GET_VUXYZMODE(regs[upper].VFwrite));
	}
	if( vfacc[upper] >= 0 && (regs[upper].VIwrite&(1<<REG_ACC_FLAG))) {
		SuperVUFreeXMMreg(vfacc[upper], XMMTYPE_ACC, 0);
		_allocACCtoXMMreg(VU, vfacc[upper], MODE_WRITE|(regs[upper].VFwxyzw != 0xf?MODE_READ:0));
	}

	int info = PROCESS_VU_SUPER;
	if( vfread0[upper] >= 0 ) info |= PROCESS_EE_SET_S(vfread0[upper]);
	if( vfread1[upper] >= 0 ) info |= PROCESS_EE_SET_T(vfread1[upper]);
	if( vfacc[upper] >= 0 ) info |= PROCESS_VU_SET_ACC(vfacc[upper]);
	if( vfwrite[upper] >= 0 ) {
		if( regs[upper].VFwrite == _Ft_ && vfread1[upper] < 0 ) {
			info |= PROCESS_EE_SET_T(vfwrite[upper]);
		}
		else {
			assert( regs[upper].VFwrite == _Fd_ );
			info |= PROCESS_EE_SET_D(vfwrite[upper]);
		}
	}

	if( (vffree[upper]&0xf) < XMMREGS ) {
		SuperVUFreeXMMreg(vffree[upper]&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, vffree[upper]&0xf);
	}
	info |= PROCESS_VU_SET_TEMP(vffree[upper]&0xf);

	if( vfflush[upper] >= 0 ) {
		SuperVUFreeXMMreg(vfflush[upper], XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, vfflush[upper]);
	}

	if( upper && (regs[upper].VIwrite & (1 << REG_CLIP_FLAG)) ) {
		// CLIP inst, need two extra temp registers, put it EEREC_D and EEREC_ACC
		assert( vfwrite[upper] == -1 );
		SuperVUFreeXMMreg((vffree[upper]>>8)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper]>>8)&0xf);
		info |= PROCESS_EE_SET_D((vffree[upper]>>8)&0xf);

		SuperVUFreeXMMreg((vffree[upper]>>16)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper]>>16)&0xf);
		info |= PROCESS_EE_SET_ACC((vffree[upper]>>16)&0xf);

		_freeXMMreg((vffree[upper]>>8)&0xf); // don't need anymore
		_freeXMMreg((vffree[upper]>>16)&0xf); // don't need anymore
	}
	else if( regs[upper].VIwrite & (1<<REG_P) ) {
		SuperVUFreeXMMreg((vffree[upper]>>8)&0xf, XMMTYPE_TEMP, 0);
		_allocTempXMMreg(XMMT_FPS, (vffree[upper]>>8)&0xf);
		info |= PROCESS_EE_SET_D((vffree[upper]>>8)&0xf);
		_freeXMMreg((vffree[upper]>>8)&0xf); // don't need anymore
	}

	if( vfflush[upper] >= 0 ) _freeXMMreg(vfflush[upper]);
	if( (vffree[upper]&0xf) < XMMREGS ) _freeXMMreg(vffree[upper]&0xf); // don't need anymore

	if( (regs[0].VIwrite|regs[1].VIwrite) & ((1<<REG_STATUS_FLAG)|(1<<REG_MAC_FLAG)) )
		info |= PROCESS_VU_UPDATEFLAGS;

	return info;
}

static void checkvucodefn(u32 curpc, u32 vuindex, u32 oldcode)
{
	SysPrintf("vu%c code changed (old:%x, new: %x)! %x %x\n", '0'+vuindex, oldcode, s_vu?*(u32*)&VU1.Micro[curpc]:*(u32*)&VU0.Micro[curpc], curpc, cpuRegs.cycle);
}

void VuInstruction::Recompile(list<VuInstruction>::const_iterator& itinst, u32 vuxyz)
{
	static PCSX2_ALIGNED16(VECTOR _VF);
    static PCSX2_ALIGNED16(VECTOR _VFc);
	u32 *ptr;
	u8* pjmp;
	int vfregstore=0, viregstore=0;

	assert( s_pCurInst == this);
	s_WriteToReadQ = 0;

	ptr = (u32*)&VU->Micro[ pc ];

	if( type & INST_Q_READ )
		SuperVUFlush(0, (ptr[0] == 0x800003bf)||!!(regs[0].VIwrite & (1<<REG_Q)));
	if( type & INST_P_READ )
		SuperVUFlush(1, (ptr[0] == 0x800007bf)||!!(regs[0].VIwrite & (1<<REG_P)));

	if( type & INST_DUMMY ) {
		if( type & INST_CLIP_WRITE ) {
			if( nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc )
				// reading from out of this block, so already flushed to mem
				s_ClipRead = (u32)&VU->VI[REG_CLIP_FLAG];
			else {
				s_ClipRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pClipWrite;
			}
		}

		// before modifying, check if they will ever be read
		if( s_pCurBlock->type & BLOCKTYPE_MACFLAGS ) {
			if( type & INST_STATUS_WRITE ) {
				
				if( nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc )
					// reading from out of this block, so already flushed to mem
					s_StatusRead = (u32)&VU->VI[REG_STATUS_FLAG];
				else {
					s_StatusRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pStatusWrite;
				}
			}
			if( type & INST_MAC_WRITE ) {
				
				if( nParentPc < s_pCurBlock->startpc || nParentPc >= (int)pc )
					// reading from out of this block, so already flushed to mem
					s_MACRead = (u32)&VU->VI[REG_MAC_FLAG];
				else {
					s_MACRead = s_pCurBlock->GetInstIterAtPc(nParentPc)->pMACWrite;
				}
			}
		}

		assert( s_ClipRead != 0 );
		assert( s_MACRead != 0 );
		assert( s_StatusRead != 0 );
		return;
	}

#ifdef _DEBUG
//	CMP32ItoM((u32)ptr, ptr[0]);
//	j8Ptr[0] = JNE8(0);
//	CMP32ItoM((u32)(ptr+1), ptr[1]);
//	j8Ptr[1] = JNE8(0);
//	j8Ptr[2] = JMP8(0);
//	x86SetJ8( j8Ptr[0] );
//	x86SetJ8( j8Ptr[1] );
//	PUSH32I(ptr[0]);
//	PUSH32I(s_vu);
//	PUSH32I(pc);
//	CALLFunc((u32)checkvucodefn);
//	ADD32ItoR(ESP, 12);
//	x86SetJ8( j8Ptr[ 2 ] );

	MOV32ItoR(EAX, pc);
#endif

	assert( !(type & (INST_CLIP_WRITE|INST_STATUS_WRITE|INST_MAC_WRITE)) );
	pc += 8;

    list<VuInstruction>::const_iterator itinst2;

	if( (regs[0].VIwrite|regs[1].VIwrite) & ((1<<REG_MAC_FLAG)|(1<<REG_STATUS_FLAG)) ) {
		if( s_pCurBlock->type & BLOCKTYPE_MACFLAGS ) {
            if( pMACWrite == NULL ) {
                pMACWrite = (u32)SuperVUStaticAlloc(4);
            }
			if( pStatusWrite == NULL )
				pStatusWrite = (u32)SuperVUStaticAlloc(4);
		}
		else {
			assert( s_StatusRead == (u32)&VU->VI[REG_STATUS_FLAG] );
			assert( s_MACRead == (u32)&VU->VI[REG_MAC_FLAG] );
			pMACWrite = s_MACRead;
			pStatusWrite = s_StatusRead;
		}
	}

	if( pClipWrite == NULL && ((regs[0].VIwrite|regs[1].VIwrite) & (1<<REG_CLIP_FLAG)) )
		pClipWrite = (u32)SuperVUStaticAlloc(4);

#ifdef SUPERVU_X86CACHING
	// redo the counters so that the proper regs are released
	for(int j = 0; j < X86REGS; ++j) {
		if( x86regs[j].inuse && X86_ISVI(x86regs[j].type) ) {
			int count = 0;
			itinst2 = itinst;

			while(itinst2 != s_pCurBlock->insts.end() ) {
				if( (itinst2->regs[0].VIread|itinst2->regs[0].VIwrite|itinst2->regs[1].VIread|itinst2->regs[1].VIwrite) && (1<<x86regs[j].reg) )
					break;
				
				++count;
				++itinst2;
			}

			x86regs[j].counter = 1000-count;
		}
	}
#endif

	if (s_vu == 0 && (ptr[1] & 0x20000000)) { // M flag 
		OR8ItoM((u32)&VU->flags, VUFLAG_MFLAGSET);
	}
	if (ptr[1] & 0x10000000) { // D flag
		TEST32ItoM((u32)&VU0.VI[REG_FBRST].UL, s_vu?0x400:0x004);
		u8* ptr = JZ8(0);
		OR32ItoM((u32)&VU0.VI[REG_VPU_STAT].UL, s_vu?0x200:0x002);
		PUSH32I(s_vu?INTC_VU1:INTC_VU0);
		CALLFunc((u32)hwIntcIrq);
		ADD32ItoR(ESP, 4);
		x86SetJ8(ptr);
	}
	if (ptr[1] & 0x08000000) { // T flag
		TEST32ItoM((u32)&VU0.VI[REG_FBRST].UL, s_vu?0x800:0x008);
		u8* ptr = JZ8(0);
		OR32ItoM((u32)&VU0.VI[REG_VPU_STAT].UL, s_vu?0x400:0x004);
		PUSH32I(s_vu?INTC_VU1:INTC_VU0);
		CALLFunc((u32)hwIntcIrq);
		ADD32ItoR(ESP, 4);
		x86SetJ8(ptr);
	}

	// check upper flags
	if (ptr[1] & 0x80000000) { // I flag

		assert( !(regs[0].VIwrite & ((1<<REG_Q)|(1<<REG_P))) );

		VU->code = ptr[1]; 
		s_vuInfo = SetCachedRegs(1, vuxyz);
		if( s_JumpX86 > 0 ) x86regs[s_JumpX86].needed = 1;
		if( s_ScheduleXGKICK && s_XGKICKReg > 0 ) x86regs[s_XGKICKReg].needed = 1;

		recSVU_UPPER_OPCODE[ VU->code & 0x3f ]();
		
		s_PrevIWrite = (u32)ptr;
		_clearNeededXMMregs();
		_clearNeededX86regs();
	}
	else {
		if( regs[0].VIwrite & (1<<REG_Q) ) {

			// search for all the insts between this inst and writeback
			itinst2 = itinst;
			++itinst2;
			u32 cacheq = (itinst2 == s_pCurBlock->insts.end());
			u32* codeptr2 = ptr+2;

			while(itinst2 != s_pCurBlock->insts.end() ) {
				if( !(itinst2->type & INST_DUMMY) && ((itinst2->regs[0].VIwrite&(1<<REG_Q)) || codeptr2[0] == 0x800003bf) ) { // waitq, or fdiv inst
					break;
				}
				if( (itinst2->type & INST_Q_WRITE) && itinst2->nParentPc == pc-8 ) {
					break;
				}
				if( itinst2->type & INST_Q_READ ) {
					cacheq = 1;
					break;
				}
				if( itinst2->type & INST_DUMMY ) {
					++itinst2;
					continue;
				}
				codeptr2 += 2;
				++itinst2;
			}

			if( itinst2 == s_pCurBlock->insts.end() )
				cacheq = 1;

			int x86temp = -1;
			if( cacheq )
				x86temp = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);

			// new is written so flush old
			// if type & INST_Q_READ, already flushed
			if( !(type & INST_Q_READ) && s_recWriteQ == 0 ) MOV32MtoR(EAX, (u32)&s_writeQ);
			
			if( cacheq )
				MOV32MtoR(x86temp, (u32)&s_TotalVUCycles);

			if( !(type & INST_Q_READ) ) {
				if( s_recWriteQ == 0 ) {
					OR32RtoR(EAX, EAX);
					pjmp = JS8(0);
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_Q, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_Q, 1), EAX);
					x86SetJ8(pjmp);
				}
				else if( s_needFlush & 1 ) {
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_Q, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_Q, 1), EAX);
					s_needFlush &= ~1;
				}
			}

			// write new Q
			if( cacheq ) {
				assert(s_pCurInst->pqcycles>1);
				ADD32ItoR(x86temp, s_pCurInst->info.cycle+s_pCurInst->pqcycles);
				MOV32RtoM((u32)&s_writeQ, x86temp);
				s_needFlush |= 1;
			}
			else {
				// won't be writing back
				s_WriteToReadQ = 1;
				s_needFlush &= ~1;
				MOV32ItoM((u32)&s_writeQ, 0x80000001);
			}

			s_recWriteQ = s_pCurInst->info.cycle+s_pCurInst->pqcycles;

			if( x86temp >= 0 )
				_freeX86reg(x86temp);
		}

		if( regs[0].VIwrite & (1<<REG_P) ) {
			int x86temp = _allocX86reg(-1, X86TYPE_TEMP, 0, 0);

			// new is written so flush old
			if( !(type & INST_P_READ) && s_recWriteP == 0)
				MOV32MtoR(EAX, (u32)&s_writeP);
			MOV32MtoR(x86temp, (u32)&s_TotalVUCycles);

			if( !(type & INST_P_READ) ) {
				if( s_recWriteP == 0 ) {
					OR32RtoR(EAX, EAX);
					pjmp = JS8(0);
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_P, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_P, 1), EAX);
					x86SetJ8(pjmp);
				}
				else if( s_needFlush & 2 ) {
					MOV32MtoR(EAX, SuperVUGetVIAddr(REG_P, 0));
					MOV32RtoM(SuperVUGetVIAddr(REG_P, 1), EAX);
					s_needFlush &= ~2;
				}
			}

			// write new P
			assert(s_pCurInst->pqcycles>1);
			ADD32ItoR(x86temp, s_pCurInst->info.cycle+s_pCurInst->pqcycles);
			MOV32RtoM((u32)&s_writeP, x86temp);
			s_needFlush |= 2;

			s_recWriteP = s_pCurInst->info.cycle+s_pCurInst->pqcycles;

			_freeX86reg(x86temp);
		}

		if( ptr[0] == 0x800003bf ) // waitq
			SuperVUFlush(0, 1);

		if( ptr[0] == 0x800007bf ) // waitp
			SuperVUFlush(1, 1);

#ifdef PCSX2_DEVBUILD
		if ( regs[1].VIread & regs[0].VIwrite & ~((1<<REG_Q)|(1<<REG_P)|(1<<REG_VF0_FLAG)|(1<<REG_ACC_FLAG))) {
			SysPrintf("*PCSX2*: Warning, VI write to the same reg %x in both lower/upper cycle %x\n", regs[1].VIread & regs[0].VIwrite, s_pCurBlock->startpc);
		}
#endif

		u32 modewrite = 0;
		if( vfwrite[1] >= 0 && xmmregs[vfwrite[1]].inuse && xmmregs[vfwrite[1]].type == XMMTYPE_VFREG && xmmregs[vfwrite[1]].reg == regs[1].VFwrite )
			modewrite = xmmregs[vfwrite[1]].mode & MODE_WRITE;

		VU->code = ptr[1]; 
		s_vuInfo = SetCachedRegs(1, vuxyz);

		if (vfwrite[1] >= 0) {
			assert( regs[1].VFwrite > 0 );

			if (vfwrite[0] == vfwrite[1]) {
				//SysPrintf("*PCSX2*: Warning, VF write to the same reg in both lower/upper cycle %x\n", s_pCurBlock->startpc);
			}

			if (vfread0[0] == vfwrite[1] || vfread1[0] == vfwrite[1] ) {
				assert( regs[0].VFread0 == regs[1].VFwrite || regs[0].VFread1 == regs[1].VFwrite );
				assert( vfflush[0] >= 0 );
				if( modewrite ) {
					SSE_MOVAPS_XMM_to_M128((u32)&VU->VF[regs[1].VFwrite], (x86SSERegType)vfwrite[1]);
				}
				vfregstore = 1;
			}
        }

		if( s_JumpX86 > 0 ) x86regs[s_JumpX86].needed = 1;
		if( s_ScheduleXGKICK && s_XGKICKReg > 0 ) x86regs[s_XGKICKReg].needed = 1;

		recSVU_UPPER_OPCODE[ VU->code & 0x3f ]();
		_clearNeededXMMregs();
		_clearNeededX86regs();
	
		// necessary because status can be set by both upper and lower
		if( regs[1].VIwrite & (1<<REG_STATUS_FLAG) ) {
			assert( pStatusWrite != 0 );
			s_PrevStatusWrite = pStatusWrite;
		}

		VU->code = ptr[0]; 
		s_vuInfo = SetCachedRegs(0, vuxyz);

		if( vfregstore ) {
			// load
			SSE_MOVAPS_M128_to_XMM(vfflush[0], (u32)&VU->VF[regs[1].VFwrite]);

			assert( xmmregs[vfwrite[1]].mode & MODE_WRITE );

			// replace with vfflush
			if( _Fs_ == regs[1].VFwrite ) {
				s_vuInfo &= ~PROCESS_EE_SET_S(0xf);
				s_vuInfo |= PROCESS_EE_SET_S(vfflush[0]);
			}
			if( _Ft_ == regs[1].VFwrite ) {
				s_vuInfo &= ~PROCESS_EE_SET_T(0xf);
				s_vuInfo |= PROCESS_EE_SET_T(vfflush[0]);
			}

			xmmregs[vfflush[0]].mode |= MODE_NOFLUSH|MODE_WRITE; // so that lower inst doesn't flush
		}

		if( s_JumpX86 > 0 ) x86regs[s_JumpX86].needed = 1;
		if( s_ScheduleXGKICK && s_XGKICKReg > 0 ) x86regs[s_XGKICKReg].needed = 1;

		// check if inst before branch and the write is the same as the read in the branch (wipeout)
		int oldreg=0;
//		if( pc == s_pCurBlock->endpc-16 ) {
//			itinst2 = itinst; ++itinst2;
//			if( itinst2->regs[0].pipe == VUPIPE_BRANCH && (itinst->regs[0].VIwrite&itinst2->regs[0].VIread) ) {
//				
//				CALLFunc((u32)branchfn);
//				assert( itinst->regs[0].VIwrite & 0xffff );
//				SysPrintf("vi write before branch\n");
//				for(s_CacheVIReg = 0; s_CacheVIReg < 16; ++s_CacheVIReg) {
//					if( itinst->regs[0].VIwrite & (1<<s_CacheVIReg) )
//						break;
//				}
//				
//				oldreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), s_CacheVIReg, MODE_READ);
//				s_CacheVIX86 = _allocX86reg(-1, X86TYPE_VITEMP, s_CacheVIReg, MODE_WRITE);
//				MOV32RtoR(s_CacheVIX86, oldreg);
//			}
//		}
//		else if( pc == s_pCurBlock->endpc-8 && s_CacheVIReg >= 0 ) {
//			assert( s_CacheVIX86 > 0 && x86regs[s_CacheVIX86].inuse && x86regs[s_CacheVIX86].reg == s_CacheVIReg && x86regs[s_CacheVIX86].type == X86TYPE_VITEMP );
//
//			oldreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), s_CacheVIReg, MODE_READ);
//			x86regs[s_CacheVIX86].needed = 1;
//			assert( x86regs[oldreg].mode & MODE_WRITE );
//
//			x86regs[s_CacheVIX86].type = X86TYPE_VI|(s_vu?X86TYPE_VU1:0);
//			x86regs[oldreg].type = X86TYPE_VITEMP;
//		}

		recSVU_LOWER_OPCODE[ VU->code >> 25 ]();

//		if( pc == s_pCurBlock->endpc-8 && s_CacheVIReg >= 0 ) {
//			// revert
//			x86regs[s_CacheVIX86].inuse = 0;
//			x86regs[oldreg].type = X86TYPE_VI|(s_vu?X86TYPE_VU1:0);
//		}

		_clearNeededXMMregs();
		_clearNeededX86regs();
	}

	// clip is always written so ok
	if( (regs[0].VIwrite|regs[1].VIwrite) & (1<<REG_CLIP_FLAG) ) {
		assert( pClipWrite != 0 );
		s_PrevClipWrite = pClipWrite;
	}

	if( (regs[0].VIwrite|regs[1].VIwrite) & (1<<REG_STATUS_FLAG) ) {
		assert( pStatusWrite != 0 );
		s_PrevStatusWrite = pStatusWrite;
	}

	if( (regs[0].VIwrite|regs[1].VIwrite) & (1<<REG_MAC_FLAG) ) {
		assert( pStatusWrite != 0 );
		s_PrevMACWrite = pMACWrite;
	}
}

///////////////////////////////////
// Super VU Recompilation Tables //
///////////////////////////////////

void recSVUMI_BranchHandle()
{
	int bpc = _recbranchAddr(VU->code); 
	int curjump = 0;

	if( s_pCurInst->type & INST_BRANCH_DELAY ) {
		assert( (branch&0x17)!=0x10 && (branch&0x17)!=4 ); // no jump handlig for now

		if( (branch & 0x7) == 3 ) {
			// previous was a direct jump
			curjump = 1;
		}
		else if( branch & 1 ) curjump = 2;
	}

	assert( s_JumpX86 > 0 );

	if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);
	MOV32ItoR(s_JumpX86, 0);
	s_pCurBlock->pChildJumps[curjump] = (u32*)x86Ptr-1;

	if( !(s_pCurInst->type & INST_BRANCH_DELAY) ) {
		j8Ptr[1] = JMP8(0);
		x86SetJ8( j8Ptr[ 0 ] );

		if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), pc+8);
		MOV32ItoR(s_JumpX86, 0);
		s_pCurBlock->pChildJumps[curjump+1] = (u32*)x86Ptr-1;

		x86SetJ8( j8Ptr[ 1 ] );
	}
	else
		x86SetJ8( j8Ptr[ 0 ] );

	branch |= 1;
}

// supervu specific insts
void recSVUMI_IBQ_prep()
{
	int fsreg, ftreg;

	if( _Fs_ == 0 ) {
		ftreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Ft_, MODE_READ);
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if( ftreg >= 0 ) {
			CMP16ItoR( ftreg, 0 );
		}
		else CMP16ItoM(SuperVUGetVIAddr(_Ft_, 1), 0);
	}
	else if( _Ft_ == 0 ) {
		fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if( fsreg >= 0 ) {
			CMP16ItoR( fsreg, 0 );
		}
		else CMP16ItoM(SuperVUGetVIAddr(_Fs_, 1), 0);
	}
	else {
		_addNeededX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Ft_);
		fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
		ftreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Ft_, MODE_READ);
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

		if( fsreg >= 0 ) {
			if( ftreg >= 0 ) {
				CMP16RtoR( fsreg, ftreg );
			}
			else CMP16MtoR(fsreg, SuperVUGetVIAddr(_Ft_, 1));
		}
		else if( ftreg >= 0 ) {
			CMP16MtoR(ftreg, SuperVUGetVIAddr(_Fs_, 1));
		}
		else {
			fsreg = _allocX86reg(-1, X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
			CMP16MtoR(fsreg, SuperVUGetVIAddr(_Ft_, 1));
		}
	}
}

void recSVUMI_IBEQ()
{
	recSVUMI_IBQ_prep();
	j8Ptr[ 0 ] = JNE8( 0 );
	recSVUMI_BranchHandle();
}

void recSVUMI_IBGEZ()
{
	int fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

	if( fsreg >= 0 ) {
		OR16RtoR(fsreg, fsreg);
		j8Ptr[ 0 ] = JS8( 0 );
	}
	else {
		CMP16ItoM( SuperVUGetVIAddr(_Fs_, 1), 0x0 );
		j8Ptr[ 0 ] = JL8( 0 );
	}

	recSVUMI_BranchHandle();
}

void recSVUMI_IBGTZ()
{
	int fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

	if( fsreg >= 0 ) {
		CMP16ItoR(fsreg, 0);
		j8Ptr[ 0 ] = JLE8( 0 );
	}
	else {
		CMP16ItoM( SuperVUGetVIAddr(_Fs_, 1), 0x0 );
		j8Ptr[ 0 ] = JLE8( 0 );
	}
	recSVUMI_BranchHandle();
}

void recSVUMI_IBLEZ()
{
	int fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

	if( fsreg >= 0 ) {
		CMP16ItoR(fsreg, 0);
		j8Ptr[ 0 ] = JG8( 0 );
	}
	else {
		CMP16ItoM( SuperVUGetVIAddr(_Fs_, 1), 0x0 );
		j8Ptr[ 0 ] = JG8( 0 );
	}
	recSVUMI_BranchHandle();
}

void recSVUMI_IBLTZ()
{
	int fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);

	if( fsreg >= 0 ) {
		OR16RtoR(fsreg, fsreg);
		j8Ptr[ 0 ] = JNS8( 0 );
	}
	else {
		CMP16ItoM( SuperVUGetVIAddr(_Fs_, 1), 0x0 );
		j8Ptr[ 0 ] = JGE8( 0 );
	}
	recSVUMI_BranchHandle();
}

void recSVUMI_IBNE()
{
	recSVUMI_IBQ_prep();
	j8Ptr[ 0 ] = JE8( 0 );
	recSVUMI_BranchHandle();
}

void recSVUMI_B()
{
	// supervu will take care of the rest
	int bpc = _recbranchAddr(VU->code); 
	if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);

	// loops to self, so check condition
	if( bpc == s_pCurBlock->startpc && s_vu == 0 ) {
		SuperVUTestVU0Condition(0);
	}

	if( s_pCurBlock->blocks.size() > 1 ) {
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);
		MOV32ItoR(s_JumpX86, 0);
        s_pCurBlock->pChildJumps[(s_pCurInst->type & INST_BRANCH_DELAY)?1:0] = (u32*)x86Ptr-1;
        s_UnconditionalDelay = 1;
	}

	branch |= 3;
}

void recSVUMI_BAL()
{
	int bpc = _recbranchAddr(VU->code); 
	if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32ItoM(SuperVUGetVIAddr(REG_TPC, 0), bpc);

	// loops to self, so check condition
	if( bpc == s_pCurBlock->startpc && s_vu == 0 ) {
		SuperVUTestVU0Condition(0);
	}

	if ( _Ft_ ) {
		_deleteX86reg(X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Ft_, 2);
		MOV16ItoM( SuperVUGetVIAddr(_Ft_, 0), (pc+8)>>3 );
	}

	if( s_pCurBlock->blocks.size() > 1 ) {
		s_JumpX86 = _allocX86reg(-1, X86TYPE_VUJUMP, 0, MODE_WRITE);
		MOV32ItoR(s_JumpX86, 0);
        s_pCurBlock->pChildJumps[(s_pCurInst->type & INST_BRANCH_DELAY)?1:0] = (u32*)x86Ptr-1;
        s_UnconditionalDelay = 1;
	}

	branch |= 3;
}

void recSVUMI_JR()
{
	int fsreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Fs_, MODE_READ);
	LEA32RStoR(EAX, fsreg, 3);
	CWDE();
	
	if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32RtoM(SuperVUGetVIAddr(REG_TPC, 0), EAX);
	
	if( !(s_pCurBlock->type & BLOCKTYPE_HASEOP) ) {
		PUSH32I(s_vu);
		PUSH32R(EAX);
	}
	branch |= 0x10; // 0x08 is reserved
}

void recSVUMI_JALR()
{
	_addNeededX86reg(X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Ft_);

	int fsreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Fs_, MODE_READ);
	LEA32RStoR(EAX, fsreg, 3);
	CWDE(); // necessary, charlie and chocolate factory gives bad addrs, but graphics are ok

	if ( _Ft_ ) {
		_deleteX86reg(X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Ft_, 2);
		MOV16ItoM( SuperVUGetVIAddr(_Ft_, 0), (pc+8)>>3 );
	}

	if( (s_pCurBlock->type & BLOCKTYPE_HASEOP) || s_vu == 0 ) MOV32RtoM(SuperVUGetVIAddr(REG_TPC, 0), EAX);
	
	if( !(s_pCurBlock->type & BLOCKTYPE_HASEOP) ) {
		PUSH32I(s_vu);
		PUSH32R(EAX);
	}

	branch |= 4;
}

#ifdef SUPERVU_COUNT
void StopSVUCounter()
{
	QueryPerformanceCounter(&svufinal);
	svutime += (u32)(svufinal.QuadPart-svubase.QuadPart);
}

void StartSVUCounter()
{
	QueryPerformanceCounter(&svubase);
}
#endif

#ifdef PCSX2_DEVBUILD
void vu1xgkick(u32* pMem, u32 addr)
{
	assert( addr < 0x4000 );
#ifdef _DEBUG
	static int scount = 0;
    static int curesp;
    __asm mov curesp, esp
	scount++;
	if( vudump & 8 ) {
		__Log("xgkick 0x%x (%d)\n", addr, scount);
	}
#endif

	GSGIFTRANSFER1(pMem, addr);
}

//extern u32 vudump;
//void countfn()
//{
//	static int scount = 0;
//	scount++;
//
//	if( scount > 16 ) {
//		__Log("xgkick %d\n", scount);
//		vudump |= 8;
//	}
//}

#endif

void recSVUMI_XGKICK_( VURegs *VU )
{
	assert( s_XGKICKReg > 0 && x86regs[s_XGKICKReg].inuse && x86regs[s_XGKICKReg].type == X86TYPE_VITEMP);

	x86regs[s_XGKICKReg].inuse = 0; // so free doesn't flush
	_freeX86regs();
	_freeXMMregs();

	PUSH32R(s_XGKICKReg);
	PUSH32I((int)VU->Mem);

#ifdef SUPERVU_COUNT
	CALLFunc((u32)StopSVUCounter);
#endif

	//CALLFunc((u32)countfn);

	if( CHECK_MULTIGS ) {
		CALLFunc((int)VU1XGKICK_MTGSTransfer);
		ADD32ItoR(ESP, 8);
	}
	else {
#ifdef PCSX2_DEVBUILD
		CALLFunc((int)vu1xgkick);
		ADD32ItoR(ESP, 8);
#else
		CALLFunc((int)GSgifTransfer1);	
#endif
	}

#ifdef SUPERVU_COUNT
	CALLFunc((u32)StartSVUCounter);
#endif

	s_ScheduleXGKICK = 0;
}

void recSVUMI_XGKICK( VURegs *VU, int info )
{
	if( s_ScheduleXGKICK ) {
		// second xgkick, so launch the first
		recSVUMI_XGKICK_(VU);
	}

	int fsreg = _allocX86reg(-1, X86TYPE_VI|(s_vu?X86TYPE_VU1:0), _Fs_, MODE_READ);
	_freeX86reg(fsreg); // flush
	x86regs[fsreg].inuse = 1;
	x86regs[fsreg].type = X86TYPE_VITEMP;
	x86regs[fsreg].needed = 1;
	x86regs[fsreg].mode = MODE_WRITE|MODE_READ;
	SHL32ItoR(fsreg, 4);
	AND32ItoR(fsreg, 0x3fff);
	s_XGKICKReg = fsreg;

#ifdef SUPERVU_XGKICKDELAY
	if( pc == s_pCurBlock->endpc ) {
		recSVUMI_XGKICK_(VU);
	}
	else {
		s_ScheduleXGKICK = 2;
	}
#else
	recSVUMI_XGKICK_(VU);
#endif
}

// upper inst
void recSVUMI_ABS()   { recVUMI_ABS(VU, s_vuInfo); } 

void recSVUMI_ADD()  { recVUMI_ADD(VU, s_vuInfo); }
void recSVUMI_ADDi() { recVUMI_ADDi(VU, s_vuInfo); } 
void recSVUMI_ADDq() { recVUMI_ADDq(VU, s_vuInfo); } 
void recSVUMI_ADDx() { recVUMI_ADDx(VU, s_vuInfo); } 
void recSVUMI_ADDy() { recVUMI_ADDy(VU, s_vuInfo); } 
void recSVUMI_ADDz() { recVUMI_ADDz(VU, s_vuInfo); } 
void recSVUMI_ADDw() { recVUMI_ADDw(VU, s_vuInfo); }

void recSVUMI_ADDA()  { recVUMI_ADDA(VU, s_vuInfo); } 
void recSVUMI_ADDAi() { recVUMI_ADDAi(VU, s_vuInfo); } 
void recSVUMI_ADDAq() { recVUMI_ADDAq(VU, s_vuInfo); } 
void recSVUMI_ADDAx() { recVUMI_ADDAx(VU, s_vuInfo); } 
void recSVUMI_ADDAy() { recVUMI_ADDAy(VU, s_vuInfo); } 
void recSVUMI_ADDAz() { recVUMI_ADDAz(VU, s_vuInfo); } 
void recSVUMI_ADDAw() { recVUMI_ADDAw(VU, s_vuInfo); } 

void recSVUMI_SUB()  { recVUMI_SUB(VU, s_vuInfo); } 
void recSVUMI_SUBi() { recVUMI_SUBi(VU, s_vuInfo); } 
void recSVUMI_SUBq() { recVUMI_SUBq(VU, s_vuInfo); } 
void recSVUMI_SUBx() { recVUMI_SUBx(VU, s_vuInfo); } 
void recSVUMI_SUBy() { recVUMI_SUBy(VU, s_vuInfo); } 
void recSVUMI_SUBz() { recVUMI_SUBz(VU, s_vuInfo); } 
void recSVUMI_SUBw() { recVUMI_SUBw(VU, s_vuInfo); } 

void recSVUMI_SUBA()  { recVUMI_SUBA(VU, s_vuInfo); } 
void recSVUMI_SUBAi() { recVUMI_SUBAi(VU, s_vuInfo); } 
void recSVUMI_SUBAq() { recVUMI_SUBAq(VU, s_vuInfo); } 
void recSVUMI_SUBAx() { recVUMI_SUBAx(VU, s_vuInfo); } 
void recSVUMI_SUBAy() { recVUMI_SUBAy(VU, s_vuInfo); } 
void recSVUMI_SUBAz() { recVUMI_SUBAz(VU, s_vuInfo); } 
void recSVUMI_SUBAw() { recVUMI_SUBAw(VU, s_vuInfo); } 

void recSVUMI_MUL()  { recVUMI_MUL(VU, s_vuInfo); }
void recSVUMI_MULi() { recVUMI_MULi(VU, s_vuInfo); } 
void recSVUMI_MULq() { recVUMI_MULq(VU, s_vuInfo); }
void recSVUMI_MULx() { recVUMI_MULx(VU, s_vuInfo); } 
void recSVUMI_MULy() { recVUMI_MULy(VU, s_vuInfo); } 
void recSVUMI_MULz() { recVUMI_MULz(VU, s_vuInfo); } 
void recSVUMI_MULw() { recVUMI_MULw(VU, s_vuInfo); }

void recSVUMI_MULA()  { recVUMI_MULA(VU, s_vuInfo); } 
void recSVUMI_MULAi() { recVUMI_MULAi(VU, s_vuInfo); } 
void recSVUMI_MULAq() { recVUMI_MULAq(VU, s_vuInfo); } 
void recSVUMI_MULAx() { recVUMI_MULAx(VU, s_vuInfo); } 
void recSVUMI_MULAy() { recVUMI_MULAy(VU, s_vuInfo); } 
void recSVUMI_MULAz() { recVUMI_MULAz(VU, s_vuInfo); } 
void recSVUMI_MULAw() { recVUMI_MULAw(VU, s_vuInfo); } 

void recSVUMI_MADD()  { recVUMI_MADD(VU, s_vuInfo); } 
void recSVUMI_MADDi() { recVUMI_MADDi(VU, s_vuInfo); } 
void recSVUMI_MADDq() { recVUMI_MADDq(VU, s_vuInfo); } 
void recSVUMI_MADDx() { recVUMI_MADDx(VU, s_vuInfo); } 
void recSVUMI_MADDy() { recVUMI_MADDy(VU, s_vuInfo); } 
void recSVUMI_MADDz() { recVUMI_MADDz(VU, s_vuInfo); } 
void recSVUMI_MADDw() { recVUMI_MADDw(VU, s_vuInfo); } 

void recSVUMI_MADDA()  { recVUMI_MADDA(VU, s_vuInfo); } 
void recSVUMI_MADDAi() { recVUMI_MADDAi(VU, s_vuInfo); } 
void recSVUMI_MADDAq() { recVUMI_MADDAq(VU, s_vuInfo); } 
void recSVUMI_MADDAx() { recVUMI_MADDAx(VU, s_vuInfo); } 
void recSVUMI_MADDAy() { recVUMI_MADDAy(VU, s_vuInfo); } 
void recSVUMI_MADDAz() { recVUMI_MADDAz(VU, s_vuInfo); } 
void recSVUMI_MADDAw() { recVUMI_MADDAw(VU, s_vuInfo); } 

void recSVUMI_MSUB()  { recVUMI_MSUB(VU, s_vuInfo); } 
void recSVUMI_MSUBi() { recVUMI_MSUBi(VU, s_vuInfo); } 
void recSVUMI_MSUBq() { recVUMI_MSUBq(VU, s_vuInfo); } 
void recSVUMI_MSUBx() { recVUMI_MSUBx(VU, s_vuInfo); } 
void recSVUMI_MSUBy() { recVUMI_MSUBy(VU, s_vuInfo); } 
void recSVUMI_MSUBz() { recVUMI_MSUBz(VU, s_vuInfo); } 
void recSVUMI_MSUBw() { recVUMI_MSUBw(VU, s_vuInfo); } 

void recSVUMI_MSUBA()  { recVUMI_MSUBA(VU, s_vuInfo); } 
void recSVUMI_MSUBAi() { recVUMI_MSUBAi(VU, s_vuInfo); } 
void recSVUMI_MSUBAq() { recVUMI_MSUBAq(VU, s_vuInfo); } 
void recSVUMI_MSUBAx() { recVUMI_MSUBAx(VU, s_vuInfo); } 
void recSVUMI_MSUBAy() { recVUMI_MSUBAy(VU, s_vuInfo); } 
void recSVUMI_MSUBAz() { recVUMI_MSUBAz(VU, s_vuInfo); } 
void recSVUMI_MSUBAw() { recVUMI_MSUBAw(VU, s_vuInfo); } 

void recSVUMI_MAX()  { recVUMI_MAX(VU, s_vuInfo); } 
void recSVUMI_MAXi() { recVUMI_MAXi(VU, s_vuInfo); } 
void recSVUMI_MAXx() { recVUMI_MAXx(VU, s_vuInfo); } 
void recSVUMI_MAXy() { recVUMI_MAXy(VU, s_vuInfo); } 
void recSVUMI_MAXz() { recVUMI_MAXz(VU, s_vuInfo); } 
void recSVUMI_MAXw() { recVUMI_MAXw(VU, s_vuInfo); } 

void recSVUMI_MINI()  { recVUMI_MINI(VU, s_vuInfo); } 
void recSVUMI_MINIi() { recVUMI_MINIi(VU, s_vuInfo); } 
void recSVUMI_MINIx() { recVUMI_MINIx(VU, s_vuInfo); } 
void recSVUMI_MINIy() { recVUMI_MINIy(VU, s_vuInfo); } 
void recSVUMI_MINIz() { recVUMI_MINIz(VU, s_vuInfo); } 
void recSVUMI_MINIw() { recVUMI_MINIw(VU, s_vuInfo); }

void recSVUMI_FTOI0()  { recVUMI_FTOI0(VU, s_vuInfo); }
void recSVUMI_FTOI4()  { recVUMI_FTOI4(VU, s_vuInfo); } 
void recSVUMI_FTOI12() { recVUMI_FTOI12(VU, s_vuInfo); } 
void recSVUMI_FTOI15() { recVUMI_FTOI15(VU, s_vuInfo); } 
void recSVUMI_ITOF0()  { recVUMI_ITOF0(VU, s_vuInfo); } 
void recSVUMI_ITOF4()  { recVUMI_ITOF4(VU, s_vuInfo); } 
void recSVUMI_ITOF12() { recVUMI_ITOF12(VU, s_vuInfo); } 
void recSVUMI_ITOF15() { recVUMI_ITOF15(VU, s_vuInfo); } 

void recSVUMI_OPMULA() { recVUMI_OPMULA(VU, s_vuInfo); } 
void recSVUMI_OPMSUB() { recVUMI_OPMSUB(VU, s_vuInfo); } 
void recSVUMI_NOP()    { } 
void recSVUMI_CLIP() { recVUMI_CLIP(VU, s_vuInfo); }

// lower inst
void recSVUMI_MTIR()    { recVUMI_MTIR(VU, s_vuInfo); }
void recSVUMI_MR32()    { recVUMI_MR32(VU, s_vuInfo); } 
void recSVUMI_MFIR()    { recVUMI_MFIR(VU, s_vuInfo); }
void recSVUMI_MOVE()    { recVUMI_MOVE(VU, s_vuInfo); } 
void recSVUMI_WAITQ()   { recVUMI_WAITQ(VU, s_vuInfo); }
void recSVUMI_MFP()     { recVUMI_MFP(VU, s_vuInfo); } 
void recSVUMI_WAITP()   { recVUMI_WAITP(VU, s_vuInfo); }

void recSVUMI_SQRT() { recVUMI_SQRT(VU, s_vuInfo); }
void recSVUMI_RSQRT() { recVUMI_RSQRT(VU, s_vuInfo); }
void recSVUMI_DIV() { recVUMI_DIV(VU, s_vuInfo); }

void recSVUMI_ESADD()   { recVUMI_ESADD(VU, s_vuInfo); } 
void recSVUMI_ERSADD()  { recVUMI_ERSADD(VU, s_vuInfo); } 
void recSVUMI_ELENG()   { recVUMI_ELENG(VU, s_vuInfo); } 
void recSVUMI_ERLENG()  { recVUMI_ERLENG(VU, s_vuInfo); } 
void recSVUMI_EATANxy() { recVUMI_EATANxy(VU, s_vuInfo); } 
void recSVUMI_EATANxz() { recVUMI_EATANxz(VU, s_vuInfo); } 
void recSVUMI_ESUM()    { recVUMI_ESUM(VU, s_vuInfo); } 
void recSVUMI_ERCPR()   { recVUMI_ERCPR(VU, s_vuInfo); } 
void recSVUMI_ESQRT()   { recVUMI_ESQRT(VU, s_vuInfo); } 
void recSVUMI_ERSQRT()  { recVUMI_ERSQRT(VU, s_vuInfo); } 
void recSVUMI_ESIN()    { recVUMI_ESIN(VU, s_vuInfo); } 
void recSVUMI_EATAN()   { recVUMI_EATAN(VU, s_vuInfo); } 
void recSVUMI_EEXP()    { recVUMI_EEXP(VU, s_vuInfo); } 

void recSVUMI_XITOP()   { recVUMI_XITOP(VU, s_vuInfo); }
void recSVUMI_XGKICK()  { recSVUMI_XGKICK(VU, s_vuInfo); }
void recSVUMI_XTOP()    { recVUMI_XTOP(VU, s_vuInfo); }

void recSVUMI_RINIT()     { recVUMI_RINIT(VU, s_vuInfo); }
void recSVUMI_RGET()      { recVUMI_RGET(VU, s_vuInfo); }
void recSVUMI_RNEXT()     { recVUMI_RNEXT(VU, s_vuInfo); }
void recSVUMI_RXOR()      { recVUMI_RXOR(VU, s_vuInfo); }

void recSVUMI_FSAND()   { recVUMI_FSAND(VU, s_vuInfo); } 
void recSVUMI_FSEQ()    { recVUMI_FSEQ(VU, s_vuInfo); }
void recSVUMI_FSOR()    { recVUMI_FSOR(VU, s_vuInfo); }
void recSVUMI_FSSET()   { recVUMI_FSSET(VU, s_vuInfo); } 
void recSVUMI_FMEQ()    { recVUMI_FMEQ(VU, s_vuInfo); }
void recSVUMI_FMOR()    { recVUMI_FMOR(VU, s_vuInfo); }
void recSVUMI_FCEQ()    { recVUMI_FCEQ(VU, s_vuInfo); }
void recSVUMI_FCOR()    { recVUMI_FCOR(VU, s_vuInfo); }
void recSVUMI_FCSET()   { recVUMI_FCSET(VU, s_vuInfo); }
void recSVUMI_FCGET()   { recVUMI_FCGET(VU, s_vuInfo); }
void recSVUMI_FCAND()   { recVUMI_FCAND(VU, s_vuInfo); }
void recSVUMI_FMAND()   { recVUMI_FMAND(VU, s_vuInfo); }

void recSVUMI_LQ()      { recVUMI_LQ(VU, s_vuInfo); } 
void recSVUMI_LQD()     { recVUMI_LQD(VU, s_vuInfo); } 
void recSVUMI_LQI()     { recVUMI_LQI(VU, s_vuInfo); } 
void recSVUMI_SQ()      { recVUMI_SQ(VU, s_vuInfo); }
void recSVUMI_SQD()     { recVUMI_SQD(VU, s_vuInfo); }
void recSVUMI_SQI()     { recVUMI_SQI(VU, s_vuInfo); }
void recSVUMI_ILW()     { recVUMI_ILW(VU, s_vuInfo); }  
void recSVUMI_ISW()     { recVUMI_ISW(VU, s_vuInfo); }
void recSVUMI_ILWR()    { recVUMI_ILWR(VU, s_vuInfo); }
void recSVUMI_ISWR()    { recVUMI_ISWR(VU, s_vuInfo); }

void recSVUMI_IADD()    { recVUMI_IADD(VU, s_vuInfo); }
void recSVUMI_IADDI()   { recVUMI_IADDI(VU, s_vuInfo); }
void recSVUMI_IADDIU()  { recVUMI_IADDIU(VU, s_vuInfo); } 
void recSVUMI_IOR()     { recVUMI_IOR(VU, s_vuInfo); }
void recSVUMI_ISUB()    { recVUMI_ISUB(VU, s_vuInfo); }
void recSVUMI_IAND()    { recVUMI_IAND(VU, s_vuInfo); }
void recSVUMI_ISUBIU()  { recVUMI_ISUBIU(VU, s_vuInfo); } 

void recSVU_UPPER_FD_00( void );
void recSVU_UPPER_FD_01( void );
void recSVU_UPPER_FD_10( void );
void recSVU_UPPER_FD_11( void );
void recSVULowerOP( void );
void recSVULowerOP_T3_00( void );
void recSVULowerOP_T3_01( void );
void recSVULowerOP_T3_10( void );
void recSVULowerOP_T3_11( void );
void recSVUunknown( void );

void (*recSVU_LOWER_OPCODE[128])() = { 
	recSVUMI_LQ    , recSVUMI_SQ    , recSVUunknown , recSVUunknown,  
	recSVUMI_ILW   , recSVUMI_ISW   , recSVUunknown , recSVUunknown,  
	recSVUMI_IADDIU, recSVUMI_ISUBIU, recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUMI_FCEQ  , recSVUMI_FCSET , recSVUMI_FCAND, recSVUMI_FCOR, /* 0x10 */ 
	recSVUMI_FSEQ  , recSVUMI_FSSET , recSVUMI_FSAND, recSVUMI_FSOR, 
	recSVUMI_FMEQ  , recSVUunknown  , recSVUMI_FMAND, recSVUMI_FMOR, 
	recSVUMI_FCGET , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUMI_B     , recSVUMI_BAL   , recSVUunknown , recSVUunknown, /* 0x20 */  
	recSVUMI_JR    , recSVUMI_JALR  , recSVUunknown , recSVUunknown, 
	recSVUMI_IBEQ  , recSVUMI_IBNE  , recSVUunknown , recSVUunknown, 
	recSVUMI_IBLTZ , recSVUMI_IBGTZ , recSVUMI_IBLEZ, recSVUMI_IBGEZ, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x30 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVULowerOP  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x40*/  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x50 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x60 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x70 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
}; 
 
void (*recSVULowerOP_T3_00_OPCODE[32])() = { 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUMI_MOVE  , recSVUMI_LQI   , recSVUMI_DIV  , recSVUMI_MTIR,  
	recSVUMI_RNEXT , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x10 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUMI_MFP   , recSVUMI_XTOP , recSVUMI_XGKICK,  
	recSVUMI_ESADD , recSVUMI_EATANxy, recSVUMI_ESQRT, recSVUMI_ESIN,  
}; 
 
void (*recSVULowerOP_T3_01_OPCODE[32])() = { 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUMI_MR32  , recSVUMI_SQI   , recSVUMI_SQRT , recSVUMI_MFIR,  
	recSVUMI_RGET  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x10 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUMI_XITOP, recSVUunknown,  
	recSVUMI_ERSADD, recSVUMI_EATANxz, recSVUMI_ERSQRT, recSVUMI_EATAN, 
}; 
 
void (*recSVULowerOP_T3_10_OPCODE[32])() = { 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUMI_LQD   , recSVUMI_RSQRT, recSVUMI_ILWR,  
	recSVUMI_RINIT , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x10 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUMI_ELENG , recSVUMI_ESUM  , recSVUMI_ERCPR, recSVUMI_EEXP,  
}; 
 
void (*recSVULowerOP_T3_11_OPCODE[32])() = { 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUMI_SQD   , recSVUMI_WAITQ, recSVUMI_ISWR,  
	recSVUMI_RXOR  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x10 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUMI_ERLENG, recSVUunknown  , recSVUMI_WAITP, recSVUunknown,  
}; 
 
void (*recSVULowerOP_OPCODE[64])() = { 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x10 */  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown, /* 0x20 */  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVUMI_IADD  , recSVUMI_ISUB  , recSVUMI_IADDI, recSVUunknown, /* 0x30 */ 
	recSVUMI_IAND  , recSVUMI_IOR   , recSVUunknown , recSVUunknown,  
	recSVUunknown  , recSVUunknown  , recSVUunknown , recSVUunknown,  
	recSVULowerOP_T3_00, recSVULowerOP_T3_01, recSVULowerOP_T3_10, recSVULowerOP_T3_11,  
}; 
 
void (*recSVU_UPPER_OPCODE[64])() = { 
	recSVUMI_ADDx  , recSVUMI_ADDy  , recSVUMI_ADDz  , recSVUMI_ADDw, 
	recSVUMI_SUBx  , recSVUMI_SUBy  , recSVUMI_SUBz  , recSVUMI_SUBw, 
	recSVUMI_MADDx , recSVUMI_MADDy , recSVUMI_MADDz , recSVUMI_MADDw, 
	recSVUMI_MSUBx , recSVUMI_MSUBy , recSVUMI_MSUBz , recSVUMI_MSUBw, 
	recSVUMI_MAXx  , recSVUMI_MAXy  , recSVUMI_MAXz  , recSVUMI_MAXw,  /* 0x10 */  
	recSVUMI_MINIx , recSVUMI_MINIy , recSVUMI_MINIz , recSVUMI_MINIw, 
	recSVUMI_MULx  , recSVUMI_MULy  , recSVUMI_MULz  , recSVUMI_MULw, 
	recSVUMI_MULq  , recSVUMI_MAXi  , recSVUMI_MULi  , recSVUMI_MINIi, 
	recSVUMI_ADDq  , recSVUMI_MADDq , recSVUMI_ADDi  , recSVUMI_MADDi, /* 0x20 */ 
	recSVUMI_SUBq  , recSVUMI_MSUBq , recSVUMI_SUBi  , recSVUMI_MSUBi, 
	recSVUMI_ADD   , recSVUMI_MADD  , recSVUMI_MUL   , recSVUMI_MAX, 
	recSVUMI_SUB   , recSVUMI_MSUB  , recSVUMI_OPMSUB, recSVUMI_MINI, 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown,  /* 0x30 */ 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown, 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown, 
	recSVU_UPPER_FD_00, recSVU_UPPER_FD_01, recSVU_UPPER_FD_10, recSVU_UPPER_FD_11,  
}; 
 
void (*recSVU_UPPER_FD_00_TABLE[32])() = { 
	recSVUMI_ADDAx, recSVUMI_SUBAx , recSVUMI_MADDAx, recSVUMI_MSUBAx, 
	recSVUMI_ITOF0, recSVUMI_FTOI0, recSVUMI_MULAx , recSVUMI_MULAq , 
	recSVUMI_ADDAq, recSVUMI_SUBAq, recSVUMI_ADDA  , recSVUMI_SUBA  , 
	recSVUunknown , recSVUunknown , recSVUunknown  , recSVUunknown  , 
	recSVUunknown , recSVUunknown , recSVUunknown  , recSVUunknown  , 
	recSVUunknown , recSVUunknown , recSVUunknown  , recSVUunknown  , 
	recSVUunknown , recSVUunknown , recSVUunknown  , recSVUunknown  , 
	recSVUunknown , recSVUunknown , recSVUunknown  , recSVUunknown  , 
}; 
 
void (*recSVU_UPPER_FD_01_TABLE[32])() = { 
	recSVUMI_ADDAy , recSVUMI_SUBAy  , recSVUMI_MADDAy, recSVUMI_MSUBAy, 
	recSVUMI_ITOF4 , recSVUMI_FTOI4 , recSVUMI_MULAy , recSVUMI_ABS   , 
	recSVUMI_MADDAq, recSVUMI_MSUBAq, recSVUMI_MADDA , recSVUMI_MSUBA , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
}; 
 
void (*recSVU_UPPER_FD_10_TABLE[32])() = { 
	recSVUMI_ADDAz , recSVUMI_SUBAz  , recSVUMI_MADDAz, recSVUMI_MSUBAz, 
	recSVUMI_ITOF12, recSVUMI_FTOI12, recSVUMI_MULAz , recSVUMI_MULAi , 
	recSVUMI_MADDAi, recSVUMI_SUBAi , recSVUMI_MULA  , recSVUMI_OPMULA, 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
}; 
 
void (*recSVU_UPPER_FD_11_TABLE[32])() = { 
	recSVUMI_ADDAw , recSVUMI_SUBAw  , recSVUMI_MADDAw, recSVUMI_MSUBAw, 
	recSVUMI_ITOF15, recSVUMI_FTOI15, recSVUMI_MULAw , recSVUMI_CLIP  , 
	recSVUMI_MADDAi, recSVUMI_MSUBAi, recSVUunknown  , recSVUMI_NOP   , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
	recSVUunknown  , recSVUunknown  , recSVUunknown  , recSVUunknown  , 
}; 

void recSVU_UPPER_FD_00( void )
{ 
	recSVU_UPPER_FD_00_TABLE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVU_UPPER_FD_01( void )
{ 
	recSVU_UPPER_FD_01_TABLE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVU_UPPER_FD_10( void )
{ 
	recSVU_UPPER_FD_10_TABLE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVU_UPPER_FD_11( void )
{ 
	recSVU_UPPER_FD_11_TABLE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVULowerOP( void )
{ 
	recSVULowerOP_OPCODE[ VU->code & 0x3f ]( ); 
} 
 
void recSVULowerOP_T3_00( void )
{ 
	recSVULowerOP_T3_00_OPCODE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVULowerOP_T3_01( void )
{ 
	recSVULowerOP_T3_01_OPCODE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVULowerOP_T3_10( void )
{ 
	recSVULowerOP_T3_10_OPCODE[ ( VU->code >> 6 ) & 0x1f ]( ); 
} 
 
void recSVULowerOP_T3_11( void )
{ 
	recSVULowerOP_T3_11_OPCODE[ ( VU->code >> 6 ) & 0x1f ]( ); 
}

void recSVUunknown( void )
{ 
	SysPrintf("Unknown SVU micromode opcode called\n"); 
}

