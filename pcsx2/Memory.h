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

//////////
// Rewritten by zerofrog to add os virtual memory
//////////

#ifndef __MEMORY_H__
#define __MEMORY_H__

//#define ENABLECACHE

#define PS2MEM_BASE_	0x15000000
#define PS2MEM_PSX_		(PS2MEM_BASE_+0x1c000000)

#ifdef PCSX2_VIRTUAL_MEM

#ifdef _WIN32
typedef struct _PSMEMORYMAP
{
	uptr* aPFNs, *aVFNs;
} PSMEMORYMAP;
#endif

#define TRANSFORM_ADDR(memaddr) ( ((u32)(memaddr)>=0x40000000) ? ((memaddr)&~0xa0000000) : (memaddr) )

//new memory model
#define PS2MEM_BASE		((u8*)PS2MEM_BASE_)
#define PS2MEM_HW		((u8*)((u32)PS2MEM_BASE+0x10000000))
#define PS2MEM_ROM		((u8*)((u32)PS2MEM_BASE+0x1fc00000))
#define PS2MEM_ROM1		((u8*)((u32)PS2MEM_BASE+0x1e000000))
#define PS2MEM_ROM2		((u8*)((u32)PS2MEM_BASE+0x1e400000))
#define PS2MEM_EROM		((u8*)((u32)PS2MEM_BASE+0x1e040000))
#define PS2MEM_PSX		((u8*)PS2MEM_PSX_)
#define PS2MEM_SCRATCH	((u8*)((u32)PS2MEM_BASE+0x50000000))
#define PS2MEM_VU0MICRO	((u8*)((u32)PS2MEM_BASE+0x11000000))
#define PS2MEM_VU0MEM	((u8*)((u32)PS2MEM_BASE+0x11004000))
#define PS2MEM_VU1MICRO	((u8*)((u32)PS2MEM_BASE+0x11008000))
#define PS2MEM_VU1MEM	((u8*)((u32)PS2MEM_BASE+0x1100c000))

// function for mapping memory
#define PS2MEM_PSXHW	((u8*)((u32)PS2MEM_BASE+0x1f800000))
#define PS2MEM_PSXHW4	((u8*)((u32)PS2MEM_BASE+0x1f400000))
#define PS2MEM_GS		((u8*)((u32)PS2MEM_BASE+0x12000000))
#define PS2MEM_DEV9		((u8*)((u32)PS2MEM_BASE+0x14000000))
#define PS2MEM_SPU2		((u8*)((u32)PS2MEM_BASE+0x1f900000))
#define PS2MEM_SPU2_	((u8*)((u32)PS2MEM_BASE+0x1f000000)) // ?
#define PS2MEM_B80		((u8*)((u32)PS2MEM_BASE+0x18000000))
#define PS2MEM_BA0		((u8*)((u32)PS2MEM_BASE+0x1a000000))

#define PSM(mem)	(PS2MEM_BASE + TRANSFORM_ADDR(mem))

#else

extern u8  *psM; //32mb Main Ram
extern u8  *psR; //4mb rom area
extern u8  *psR1; //256kb rom1 area (actually 196kb, but can't mask this)
extern u8  *psR2; // 0x00080000
extern u8  *psER; // 0x001C0000
extern u8  *psS; //0.015 mb, scratch pad

#define PS2MEM_BASE		psM
#define PS2MEM_HW		psH
#define PS2MEM_ROM		psR
#define PS2MEM_ROM1		psR1
#define PS2MEM_ROM2		psR2
#define PS2MEM_EROM		psER
#define PS2MEM_SCRATCH	psS

extern u8 g_RealGSMem[0x2000];
#define PS2MEM_GS	g_RealGSMem

//#define _PSM(mem)	(memLUTR[(mem) >> 12] == 0 ? NULL : (void*)(memLUTR[(mem) >> 12] + ((mem) & 0xfff)))
#define PSM(mem)	((void*)(memLUTR[(mem) >> 12] + ((mem) & 0xfff)))
#define FREE(ptr) _aligned_free(ptr)

extern uptr *memLUTR;
extern uptr *memLUTW;
extern uptr *memLUTRK;
extern uptr *memLUTWK;
extern uptr *memLUTRU;
extern uptr *memLUTWU;

#endif

#define psMs8(mem)	(*(s8 *)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs16(mem)	(*(s16*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs32(mem)	(*(s32*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs64(mem)	(*(s64*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu8(mem)	(*(u8 *)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu16(mem)	(*(u16*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu32(mem)	(*(u32*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu64(mem)	(*(u64*)&PS2MEM_BASE[(mem) & 0x1ffffff])

#define psRs8(mem)	(*(s8 *)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs16(mem)	(*(s16*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs32(mem)	(*(s32*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs64(mem)	(*(s64*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu8(mem)	(*(u8 *)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu16(mem)	(*(u16*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu32(mem)	(*(u32*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu64(mem)	(*(u64*)&PS2MEM_ROM[(mem) & 0x3fffff])

#define psR1s8(mem)		(*(s8 *)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s16(mem)	(*(s16*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s32(mem)	(*(s32*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s64(mem)	(*(s64*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u8(mem)		(*(u8 *)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u16(mem)	(*(u16*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u32(mem)	(*(u32*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u64(mem)	(*(u64*)&PS2MEM_ROM1[(mem) & 0x3ffff])

#define psR2s8(mem)		(*(s8 *)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s16(mem)	(*(s16*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s32(mem)	(*(s32*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s64(mem)	(*(s64*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u8(mem)		(*(u8 *)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u16(mem)	(*(u16*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u32(mem)	(*(u32*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u64(mem)	(*(u64*)&PS2MEM_ROM2[(mem) & 0x3ffff])

#define psERs8(mem)		(*(s8 *)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs16(mem)	(*(s16*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs32(mem)	(*(s32*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs64(mem)	(*(s64*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu8(mem)		(*(u8 *)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu16(mem)	(*(u16*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu32(mem)	(*(u32*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu64(mem)	(*(u64*)&PS2MEM_EROM[(mem) & 0x3ffff])

#define psSs8(mem)	(*(s8 *)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs16(mem)	(*(s16*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs32(mem)	(*(s32*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs64(mem)	(*(s64*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu8(mem)	(*(u8 *)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu16(mem)	(*(u16*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu32(mem)	(*(u32*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu64(mem)	(*(u64*)&PS2MEM_SCRATCH[(mem) & 0x3fff])

#define PSMs8(mem)	(*(s8 *)PSM(mem))
#define PSMs16(mem)	(*(s16*)PSM(mem))
#define PSMs32(mem)	(*(s32*)PSM(mem))
#define PSMs64(mem)	(*(s64*)PSM(mem))
#define PSMu8(mem)	(*(u8 *)PSM(mem))
#define PSMu16(mem)	(*(u16*)PSM(mem))
#define PSMu32(mem)	(*(u32*)PSM(mem))
#define PSMu64(mem)	(*(u64*)PSM(mem))

int  memInit();
void memReset();
void memSetKernelMode();
void memSetSupervisorMode();
void memSetUserMode();
void memSetPageAddr(u32 vaddr, u32 paddr);
void memClearPageAddr(u32 vaddr);
void memShutdown();

int  memRead8(u32 mem, u8  *out);
int  memRead8RS(u32 mem, u64 *out);
int  memRead8RU(u32 mem, u64 *out);
int  memRead16(u32 mem, u16 *out);
int  memRead16RS(u32 mem, u64 *out);
int  memRead16RU(u32 mem, u64 *out);
int  memRead32(u32 mem, u32 *out);
int  memRead32RS(u32 mem, u64 *out);
int  memRead32RU(u32 mem, u64 *out);
int  memRead64(u32 mem, u64 *out);
int  memRead128(u32 mem, u64 *out);
void memWrite8 (u32 mem, u8  value);
void memWrite16(u32 mem, u16 value);
void memWrite32(u32 mem, u32 value);
void memWrite64(u32 mem, u64 value);
void memWrite128(u32 mem, u64 *value);

// recMemConstRead8, recMemConstRead16, recMemConstRead32 return 1 if a call was made, 0 otherwise
u8 recMemRead8();
u16 recMemRead16();
u32 recMemRead32();
void recMemRead64(u64 *out);
void recMemRead128(u64 *out);

// returns 1 if mem should be cleared
void recMemWrite8();
void recMemWrite16();
void recMemWrite32();
void recMemWrite64();
void recMemWrite128();

// VM only functions
#ifdef PCSX2_VIRTUAL_MEM

void _eeReadConstMem8(int mmreg, u32 mem, int sign);
void _eeReadConstMem16(int mmreg, u32 mem, int sign);
void _eeReadConstMem32(int mmreg, u32 mem);
void _eeReadConstMem128(int mmreg, u32 mem);
void _eeWriteConstMem8(u32 mem, int mmreg);
void _eeWriteConstMem16(u32 mem, int mmreg);
void _eeWriteConstMem32(u32 mem, int mmreg);
void _eeWriteConstMem64(u32 mem, int mmreg);
void _eeWriteConstMem128(u32 mem, int mmreg);
void _eeMoveMMREGtoR(int to, int mmreg);

// extra ops
void _eeWriteConstMem16OP(u32 mem, int mmreg, int op);
void _eeWriteConstMem32OP(u32 mem, int mmreg, int op);

int recMemConstRead8(u32 x86reg, u32 mem, u32 sign);
int recMemConstRead16(u32 x86reg, u32 mem, u32 sign);
int recMemConstRead32(u32 x86reg, u32 mem);
void recMemConstRead64(u32 mem, int mmreg);
void recMemConstRead128(u32 mem, int xmmreg);

int recMemConstWrite8(u32 mem, int mmreg);
int recMemConstWrite16(u32 mem, int mmreg);
int recMemConstWrite32(u32 mem, int mmreg);
int recMemConstWrite64(u32 mem, int mmreg);
int recMemConstWrite128(u32 mem, int xmmreg);

extern int SysPageFaultExceptionFilter(struct _EXCEPTION_POINTERS* eps);

#else

#define _eeReadConstMem8 0&&
#define _eeReadConstMem16 0&&
#define _eeReadConstMem32 0&&
#define _eeReadConstMem128 0&&
#define _eeWriteConstMem8 0&&
#define _eeWriteConstMem16 0&&
#define _eeWriteConstMem32 0&&
#define _eeWriteConstMem64 0&&
#define _eeWriteConstMem128 0&&
#define _eeMoveMMREGtoR 0&&

// extra ops
#define _eeWriteConstMem16OP 0&&
#define _eeWriteConstMem32OP 0&&

#define recMemConstRead8 0&&
#define recMemConstRead16 0&&
#define recMemConstRead32 0&&
#define recMemConstRead64 0&&
#define recMemConstRead128 0&&

#define recMemConstWrite8 0&&
#define recMemConstWrite16 0&&
#define recMemConstWrite32 0&&
#define recMemConstWrite64 0&&
#define recMemConstWrite128 0&&

#endif

#endif
