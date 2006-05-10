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

#include "Common.h"
#include "Memory.h"
#include "Hw.h"
#include "Debug.h"
#include "VU0.h"
#include "R3000A.h"
#include "VUmicro.h"

static int inter;

__declspec(align(16)) cpuRegisters cpuRegs;
__declspec(align(16)) fpuRegisters fpuRegs;
__declspec(align(16)) tlbs tlb[48];

#ifdef PCSX2_MULTICORE
pthread_spinlock_t g_lockInterrupt = PTHREAD_SPINLOCK_INITIALIZER;
#else

// slower
pthread_mutex_t g_lockInterrupt = PTHREAD_MUTEX_INITIALIZER;
#endif

void ExecuteIOP()
{
	psxCpu->ExecuteBlock();
}

int cpuInit() {
	int ret;

	SysPrintf("PCSX2 v" PCSX2_VERSION "\n");
	SysPrintf("Color Legend: White - PCSX2 message\n");
	SysPrintf(COLOR_GREEN "              Green - EE sio2 printf\n" COLOR_RESET);
	SysPrintf(COLOR_RED   "              Red   - IOP printf\n" COLOR_RESET);

	InitFPUOps();
	cpudetectInit();

	Cpu = CHECK_EEREC ? &recCpu : &intCpu;

	ret = Cpu->Init();
	if (ret == -1 && CHECK_EEREC) {
		SysMessage(_("Error initializing Recompiler, switching to Interpreter"));
		Config.Options &= ~(PCSX2_EEREC|PCSX2_VU1REC|PCSX2_VU0REC);
		Cpu = &intCpu;
		ret = Cpu->Init();
	}

#ifdef WIN32_VIRTUAL_MEM
	if (memInit() == -1) return -1;
#endif
	if (hwInit() == -1) return -1;
	if (vu0Init() == -1) return -1;
	if (vu1Init() == -1) return -1;
#ifndef WIN32_VIRTUAL_MEM
	if (memInit() == -1) return -1;
#endif

#ifdef PCSX2_DEVBUILD
	Log = 0;
#endif

	return ret;
}

void cpuReset() {
	Cpu->Reset();

	memReset();

	memset(&cpuRegs, 0, sizeof(cpuRegs));
	memset(&fpuRegs, 0, sizeof(fpuRegs));
	memset(&tlb, 0, sizeof(tlb));

	cpuRegs.pc = 0xbfc00000; ///set pc reg to stack 

	cpuRegs.CP0.n.Status.val = 0x70400004; //0x10900000 <-- wrong; // COP0 enabled | BEV = 1 | TS = 1
	cpuRegs.CP0.n.PRid   = 0x00002e20; // PRevID = Revision ID, same as R5900
	fpuRegs.fprc[0]   = 0x00002e00; // fpu Revision..
	fpuRegs.fprc[31]  = 0x01000001; // fpu Status/Control

    vu0Reset();
    vu1Reset();  
	hwReset();
	vif0Reset();
    vif1Reset();
	rcntInit();
	psxReset();
}

void cpuShutdown() {
	LOCKINT_DESTROY();

	hwShutdown();
//	biosShutdown();
	psxShutdown();
	vu0Shutdown();
	vu1Shutdown();
	memShutdown();
	gsShutdown();
	disR5900FreeSyms();

	Cpu->Shutdown();
}

void cpuException(u32 code, u32 bd) {
	u32 offset;
	cpuRegs.CP0.n.Cause = code & 0xffff;

	if(cpuRegs.CP0.n.Status.b.ERL == 0){ //Error Level 0-1
		if(((code & 0x7C) >= 0x8) && ((code & 0x7C) <= 0xC)) offset = 0x0; //TLB Refill
		else if ((code & 0x7C) == 0x0) offset = 0x200; //Interrupt
		else	offset = 0x180; // Everything else
		

		if (cpuRegs.CP0.n.Status.b.EXL == 0) {
			cpuRegs.CP0.n.Status.b.EXL = 1;
			if (bd) {
				SysPrintf("branch delay!!\n");
				cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
				cpuRegs.CP0.n.Cause |= 0x80000000;
			} else {
				cpuRegs.CP0.n.EPC = cpuRegs.pc;
				cpuRegs.CP0.n.Cause &= ~0x80000000;
			}
		} else {
			offset = 0x180; //Overrride the cause		
			SysPrintf("cpuException: Status.EXL = 1 cause %x\n", code);
		}
		if (cpuRegs.CP0.n.Status.b.BEV == 0) {
			cpuRegs.pc = 0x80000000 + offset;
		} else {
			cpuRegs.pc = 0xBFC00200 + offset;
		}
	} else { //Error Level 2
		SysPrintf("FIX ME: Level 2 cpuException\n");
		if((code & 0x38000) <= 0x8000 ) { //Reset / NMI
			cpuRegs.pc = 0xBFC00000;
			SysPrintf("Reset request\n");
			UpdateCP0Status();
			return;
		} else if((code & 0x38000) == 0x10000) offset = 0x80; //Performance Counter
		else if((code & 0x38000) == 0x18000)  offset = 0x100; //Debug
		else SysPrintf("Unknown Level 2 Exception!! Cause %x\n", code);

		if (cpuRegs.CP0.n.Status.b.EXL == 0) {
			cpuRegs.CP0.n.Status.b.EXL = 1;
			if (bd) {
				SysPrintf("branch delay!!\n");
				cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
				cpuRegs.CP0.n.Cause |= 0x80000000;
			} else {
				cpuRegs.CP0.n.EPC = cpuRegs.pc;
				cpuRegs.CP0.n.Cause &= ~0x80000000;
			}
		} else {
			offset = 0x180; //Overrride the cause		
			SysPrintf("cpuException: Status.EXL = 1 cause %x\n", code);
		}
		if (cpuRegs.CP0.n.Status.b.DEV == 0) {
			cpuRegs.pc = 0x80000000 + offset;
		} else {
			cpuRegs.pc = 0xBFC00200 + offset;
		}
	}
	UpdateCP0Status();
}

void cpuTlbMiss(u32 addr, u32 bd, u32 excode) {
	SysPrintf("cpuTlbMiss %x, %x, status=%x, code=%x\n", cpuRegs.pc, cpuRegs.cycle, cpuRegs.CP0.n.Status.val, excode);
	if (bd) {
		SysPrintf("branch delay!!\n");
	}
	cpuRegs.CP0.n.BadVAddr = addr;
	cpuRegs.CP0.n.Context &= 0xFF80000F;
	cpuRegs.CP0.n.Context |= (addr >> 9) & 0x007FFFF0;
	cpuRegs.CP0.n.EntryHi = (addr & 0xFFFFE000) | (cpuRegs.CP0.n.EntryHi & 0x1FFF);

	cpuRegs.CP0.n.Cause = excode;
	if (!(cpuRegs.CP0.n.Status.val & 0x2)) { // EXL bit
		cpuRegs.CP0.n.EPC = cpuRegs.pc - 4;
	}

	if ((cpuRegs.CP0.n.Status.val & 0x1) == 0) {
		cpuRegs.pc = 0x80000000;
	} else {
		cpuRegs.pc = 0x80000180;
	}

	cpuRegs.CP0.n.Status.b.EXL = 1;
	UpdateCP0Status();
//	Log=1; varLog|= 0x40000000;
}

void cpuTlbMissR(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBL);
}

void cpuTlbMissW(u32 addr, u32 bd) {
	cpuTlbMiss(addr, bd, EXC_CODE_TLBS);
}



void JumpCheckSym(u32 addr, u32 pc) {
#if 0
//	if (addr == 0x80051770) { SysPrintf("Log!: %s\n", PSM(cpuRegs.GPR.n.a0.UL[0])); Log=1; varLog|= 0x40000000; }
	if (addr == 0x8002f150) { SysPrintf("printk: %s\n", PSM(cpuRegs.GPR.n.a0.UL[0])); }
	if (addr == 0x8002aba0) return;
	if (addr == 0x8002f450) return;
	if (addr == 0x800dd520) return;
//	if (addr == 0x80049300) SysPrintf("register_blkdev: %x\n", cpuRegs.GPR.n.a0.UL[0]);
	if (addr == 0x8013cb70) { SysPrintf("change_root: %x\n", cpuRegs.GPR.n.a0.UL[0]); }
//	if (addr == 0x8013d1e8) { SysPrintf("Log!\n"); Log++; if (Log==2) exit(0); varLog|= 0x40000000; }
//	if (addr == 0x00234e88) { SysPrintf("StoreImage\n"); Log=1; /*psMu32(0x234e88) = 0x03e00008; psMu32(0x234e8c) = 0;*/ }
#endif
/*	if ((pc >= 0x00131D50 &&
		 pc <  0x00132454) ||
		(pc >= 0x00786a90 &&
		 pc <  0x00786ac8))*/
	if (varLog & 0x40000000) {
		char *str;
		char *strf;

		str = disR5900GetSym(addr);
		if (str != NULL) {
			strf = disR5900GetUpperSym(pc);
			if (strf) {
				SysPrintf("Func %8.8x: %s (called by %8.8x: %s)\n", addr, str, pc, strf);
			} else {
				SysPrintf("Func %8.8x: %s (called by %x)\n", addr, str, pc);
			}
			if (!strcmp(str, "printf")) { SysPrintf("%s\n", (char*)PSM(cpuRegs.GPR.n.a0.UL[0])); }
			if (!strcmp(str, "printk")) { SysPrintf("%s\n", (char*)PSM(cpuRegs.GPR.n.a0.UL[0])); }
		}
	}
}

void JumpCheckSymRet(u32 addr) {
	if (varLog & 0x40000000) {
		char *str;
		str = disR5900GetUpperSym(addr);
		if (str != NULL) {
			SysPrintf("Return       : %s, v0=%8.8x\n", str, cpuRegs.GPR.n.v0.UL[0]);
		}
	}
}

__inline void _cpuTestMissingINTC() {
	if (cpuRegs.CP0.n.Status.val & 0x400 &&
		psHu32(INTC_STAT) & psHu32(INTC_MASK)) {
		if ((cpuRegs.interrupt & (1 << 30)) == 0) {
			SysPrintf("*PCSX2*: Error, missing INTC Interrupt\n");
		}
	}
}

__inline void _cpuTestMissingDMAC() {
	if (cpuRegs.CP0.n.Status.val & 0x800 &&
		(psHu16(0xe012) & psHu16(0xe010) || 
		 psHu16(0xe010) & 0x8000)) {
		if ((cpuRegs.interrupt & (1 << 31)) == 0) {
			SysPrintf("*PCSX2*: Error, missing DMAC Interrupt\n");
		}
	}
}

void cpuTestMissingHwInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestMissingINTC();
		_cpuTestMissingDMAC();
//		_cpuTestTIMR();
	}
}

#define TESTINT(n, callback) { \
	LOCKINT_LOCK(n); \
	if ( (cpuRegs.interrupt & (1 << n)) ) { \
		if( ((cpuRegs.cycle - cpuRegs.sCycle[n]) >= cpuRegs.eCycle[n]) ) { \
			LOCKINT_UNLOCK(n); \
			if (callback() == 1) { \
				/* insetad of relocking, just do an atomic op */ \
				/*InterlockedAnd(&cpuRegs.interrupt, ~(1 << n));*/ \
				cpuRegs.interrupt &= ~(1 << n); \
			} \
		} \
		else if( g_nextBranchCycle - cpuRegs.sCycle[n] > cpuRegs.eCycle[n] ) { \
			g_nextBranchCycle = cpuRegs.sCycle[n] + cpuRegs.eCycle[n]; \
		} \
	} \
	LOCKINT_UNLOCK(n); \
} \

void _cpuTestInterrupts() {

	inter = cpuRegs.interrupt;
	/* These are 'pcsx2 interrupts', they handle asynchronous stuff
	   that depends on the cycle timings */

	TESTINT(0, vif0Interrupt);
	TESTINT(10, vifMFIFOInterrupt);
	TESTINT(1, vif1Interrupt);
	TESTINT(11, gifMFIFOInterrupt);
	TESTINT(2, gsInterrupt);
	TESTINT(3, ipu0Interrupt);
	TESTINT(4, ipu1Interrupt);
	TESTINT(5, EEsif0Interrupt);
	TESTINT(6, EEsif1Interrupt);
	TESTINT(8, SPRFROMinterrupt);
	TESTINT(9, SPRTOinterrupt);

	if ((cpuRegs.CP0.n.Status.val & 0x10007) != 0x10001) return;
	TESTINT(30, intcInterrupt);
	TESTINT(31, dmacInterrupt);
}


#define EE_WAIT_CYCLE 256

// if cpuRegs.cycle is greater than this cycle, should check cpuBranchTest for updates
u32 g_nextBranchCycle = 0;
extern u32 g_psxNextBranchCycle;
u32 s_temp = 1;
u32 s_count = 0;
u32 s_lastvsync[2];

void logtime(int index) { __Log("%u %u %d\n", cpuRegs.cycle-s_lastvsync[1], timeGetTime()-s_lastvsync[0], index); }

void cpuBranchTest()
{
	g_nextBranchCycle = cpuRegs.cycle + EE_WAIT_CYCLE;

	if ((cpuRegs.cycle - nextsCounter) >= nextCounter)
		rcntUpdate();

	if (cpuRegs.interrupt)
		_cpuTestInterrupts();

//	if( s_count++ == 300) {
//		logtime(0);
//		s_count = 0;
//	}

	if( g_nextBranchCycle-nextsCounter >= nextCounter )
		g_nextBranchCycle = nextsCounter+nextCounter;

/*#ifdef CPU_LOG
	cpuTestMissingHwInts();
#endif*/

	cpuRegs.EEsCycle += cpuRegs.cycle - cpuRegs.EEoCycle;
	cpuRegs.EEoCycle = cpuRegs.cycle;

	
	psxCpu->ExecuteBlock();
	
	
	if (VU0.VI[REG_VPU_STAT].UL & 0x1) {
		Cpu->ExecuteVU0Block();
	}
	if (VU0.VI[REG_VPU_STAT].UL & 0x100) {
		Cpu->ExecuteVU1Block();
	}
}

static void _cpuTestINTC() {
	if (cpuRegs.CP0.n.Status.val & 0x400 ){
		if	(psHu32(INTC_STAT) & psHu32(INTC_MASK)) {
			if ((cpuRegs.interrupt & (1 << 30)) == 0) {
				INT(30,4);
			}
		}
	}
}

static void _cpuTestDMAC() {
	if (cpuRegs.CP0.n.Status.val & 0x800 ){
		if	(psHu16(0xe012) & psHu16(0xe010) || 
			 psHu16(0xe010) & 0x8000) {
			if ( (cpuRegs.interrupt & (1 << 31)) == 0) {
				INT(31, 4);
			}
		}
	}
}

static void _cpuTestTIMR() {
	if (cpuRegs.CP0.n.Status.val & 0x8000 &&
		cpuRegs.CP0.n.Count >= cpuRegs.CP0.n.Compare) {
//		SysPrintf("timr intr: %x, %x\n", cpuRegs.CP0.n.Count, cpuRegs.CP0.n.Compare);
		cpuException(0x808000, cpuRegs.branch);
	}
}

void cpuTestHwInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) != 0x10001) return;
	_cpuTestINTC();
	_cpuTestDMAC();
	_cpuTestTIMR();
}

void cpuTestINTCInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestINTC();
	}
}

void cpuTestDMACInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestDMAC();
	}
}

void cpuTestTIMRInts() {
	if ((cpuRegs.CP0.n.Status.val & 0x10007) == 0x10001) {
		_cpuTestTIMR();
	}
}

extern BOOL bExecBIOS;
void cpuExecuteBios() {
	SysPrintf("* PCSX2 *: ExecuteBios\n");

	bExecBIOS = TRUE;
	while (cpuRegs.pc != 0x00200008 &&
		   cpuRegs.pc != 0x00100008) {
		Cpu->ExecuteBlock();
	}

	bExecBIOS = FALSE;
//	REC_CLEARM(0x00200008);
//	REC_CLEARM(0x00100008);
//	REC_CLEARM(cpuRegs.pc);
	if( CHECK_EEREC ) Cpu->Reset();

	SysPrintf("* PCSX2 *: ExecuteBios Complete\n");
	GSprintf(5, "PCSX2 v" PCSX2_VERSION "\nExecuteBios Complete\n");
}

void cpuRestartCPU()
{
	Cpu = CHECK_EEREC ? &recCpu : &intCpu;

	// restart vus
	if (Cpu->Init() == -1) {
		SysClose();
		exit(1);
	}

	vu0Init();
	vu1Init();
	Cpu->Reset();
	psxRestartCPU();
}

