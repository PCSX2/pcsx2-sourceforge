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

#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__

#ifdef WIN32_VIRTUAL_MEM
#define psxM PS2MEM_PSX
#else
s8 *psxM;
#endif

#define SIFINTRCONTROL  (*(u32*)((char*)psxMemRLUT[0xbd00] + 0x0040))

#define psxMs8(mem)		psxM[(mem) & 0x1fffff]
#define psxMs16(mem)	(*(s16*)&psxM[(mem) & 0x1fffff])
#define psxMs32(mem)	(*(s32*)&psxM[(mem) & 0x1fffff])
#define psxMu8(mem)		(*(u8*) &psxM[(mem) & 0x1fffff])
#define psxMu16(mem)	(*(u16*)&psxM[(mem) & 0x1fffff])
#define psxMu32(mem)	(*(u32*)&psxM[(mem) & 0x1fffff])
#define psxMu64(mem)	(*(u64*)&psxM[(mem) & 0x1fffff])

s8 *psxP;
#define psxPs8(mem)		psxP[(mem) & 0xffff]
#define psxPs16(mem)	(*(s16*)&psxP[(mem) & 0xffff])
#define psxPs32(mem)	(*(s32*)&psxP[(mem) & 0xffff])
#define psxPu8(mem)		(*(u8*) &psxP[(mem) & 0xffff])
#define psxPu16(mem)	(*(u16*)&psxP[(mem) & 0xffff])
#define psxPu32(mem)	(*(u32*)&psxP[(mem) & 0xffff])

s8 *psxH;
#define psxHs8(mem)		psxH[(mem) & 0xffff]
#define psxHs16(mem)	(*(s16*)&psxH[(mem) & 0xffff])
#define psxHs32(mem)	(*(s32*)&psxH[(mem) & 0xffff])
#define psxHu8(mem)		(*(u8*) &psxH[(mem) & 0xffff])
#define psxHu16(mem)	(*(u16*)&psxH[(mem) & 0xffff])
#define psxHu32(mem)	(*(u32*)&psxH[(mem) & 0xffff])

s8 *psxS;
#define psxSs8(mem)		psxS[(mem) & 0xffff]
#define psxSs16(mem)	(*(s16*)&psxS[(mem) & 0xffff])
#define psxSs32(mem)	(*(s32*)&psxS[(mem) & 0xffff])
#define psxSu8(mem)		(*(u8*) &psxS[(mem) & 0xffff])
#define psxSu16(mem)	(*(u16*)&psxS[(mem) & 0xffff])
#define psxSu32(mem)	(*(u32*)&psxS[(mem) & 0xffff])

uptr *psxMemWLUT;
uptr *psxMemRLUT;

#define PSXM(mem) (psxMemRLUT[(mem) >> 16] == 0 ? NULL : (void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
#define _PSXM(mem) ((void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
#define PSXMs8(mem)  (*(s8 *)_PSXM(mem))
#define PSXMs16(mem) (*(s16*)_PSXM(mem))
#define PSXMs32(mem) (*(s32*)_PSXM(mem))
#define PSXMu8(mem)  (*(u8 *)_PSXM(mem))
#define PSXMu16(mem) (*(u16*)_PSXM(mem))
#define PSXMu32(mem) (*(u32*)_PSXM(mem))

#ifndef DISABLE_REC
extern uptr *psxRecLUT;

#define BLOCKTYPE_BRANCH	1		// inst branches with delay
#define BLOCKTYPE_DELAY		2		// in branch delay slot)

typedef struct _PSXBASEBLOCK
{
	u8* fnptr;
	u8* oldfnptr; // if modified already, x86ptr of target jumping from
	u32 type; // where inst is located with respect to block
	//u32 modified; // 1 when modified
} PSXBASEBLOCK;

#define PSX_BLOCKTYPE(b) ((b)->type)

#define PSX_GETBLOCK(x) ((PSXBASEBLOCK*)(psxRecLUT[((u32)(x)) >> 16] + (sizeof(PSXBASEBLOCK)/4)*((x) & 0xffff)))

void psxRecClearMem(PSXBASEBLOCK* p, u32 mem);
#define PSXREC_CLEARM(mem) { \
	if (psxRecLUT[(mem) >> 16]) { \
		PSXBASEBLOCK* p = PSX_GETBLOCK(mem); \
		if( PSX_BLOCKTYPE(p) == BLOCKTYPE_DELAY ) \
			psxRecClearMem(p-1, (mem)-4); \
		else if( p->fnptr ) \
			psxRecClearMem(p, mem); \
	} \
} \

#else

#define PSXREC_CLEARM(mem)

#endif

int  psxMemInit();
void psxMemReset();
void psxMemShutdown();

u8   psxMemRead8 (u32 mem);
u16  psxMemRead16(u32 mem);
u32  psxMemRead32(u32 mem);
void UpdateSIF0();
void UpdateSIF1();
void psxMemWrite8 (u32 mem, u8 value);
void psxMemWrite16(u32 mem, u16 value);
void psxMemWrite32(u32 mem, u32 value);

#endif /* __PSXMEMORY_H__ */
