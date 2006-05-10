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

#include <string.h>
#include <time.h>

#include "Common.h"
#include "PsxCommon.h"

void statsOpen() {
	stats.vsyncCount = 0;
	stats.vsyncTime = time(NULL);
	stats.eeCycles = 0;
	stats.eeSCycle = 0;
	stats.iopCycles = 0;
	stats.iopSCycle = 0;
}

void statsClose() {
	time_t t;
	FILE *f;

	t = time(NULL) - stats.vsyncTime;
#ifdef __WIN32__
	f = fopen("logs\\stats.txt", "w");
#else
	f = fopen("logs/stats.txt", "w");
#endif
	if (!f) { SysPrintf("Can't open stats.txt\n"); return; }
	fprintf(f, "-- PCSX2 v%s statics--\n\n", PCSX2_VERSION);
	fprintf(f, "Ran for %d seconds\n", t);
	fprintf(f, "Total VSyncs: %d (%s)\n", stats.vsyncCount, Config.PsxType ? "PAL" : "NTSC");
#ifndef __x86_64__
	fprintf(f, "VSyncs per Seconds: %g\n", (double)stats.vsyncCount / t);
#endif
	fprintf(f, "Total EE  Instructions Executed: %lld\n", stats.eeCycles);
	fprintf(f, "Total IOP Instructions Executed: %lld\n", stats.iopCycles);
	if (!CHECK_EEREC) fprintf(f, "Interpreter Mode\n");
	else fprintf(f, "Recompiler Mode: RegCaching %s, VUrec1 %s, VUrec0 %s\n", 
		CHECK_REGCACHING ? "Enabled" : "Disabled", CHECK_VU1REC ? "Enabled" : "Disabled", CHECK_VU0REC ? "Enabled" : "Disabled");
	fclose(f);
}

void statsVSync() {
	static u64 accum = 0, accumvu1 = 0;
	static u32 frame = 0;

	stats.eeCycles+= cpuRegs.cycle - stats.eeSCycle;
	stats.eeSCycle = cpuRegs.cycle;
	stats.iopCycles+= psxRegs.cycle - stats.iopSCycle;
	stats.iopSCycle = psxRegs.cycle;
	stats.vsyncCount++;

#if (defined(__i386__) || defined(__x86_64__)) && defined(PCSX2_DEVBUILD)
	stats.framecount = GetCPUTick() - stats.ticko;
	stats.ticko = GetCPUTick();

	CPU_LOG("frame %d\n", stats.vsyncCount);
	CPU_LOG("framecount = %I64d\n", stats.framecount);
//	CPU_LOG("vif1count = %I64d\n", stats.vif1count);
//	CPU_LOG("vu1count = %I64d\n", stats.vu1count);
//
//	if( ++frame == 4 ) {
//		
//		SysPrintf("vu1 CPU=%d%%\n", (int)((accumvu1*100)/accum));
//		frame = 0;
//		accum = 0;
//		accumvu1 = 0;
//	}
//
//	accum += stats.framecount;
//	accumvu1 += stats.vu1count;

#endif

	stats.vif1count = 0;
	stats.vu1count = 0;
}
