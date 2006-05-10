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

#ifndef __MISC_H__
#define __MISC_H__

#undef s_addr

u32 BiosVersion;

char CdromId[12];

int LoadCdrom();
int CheckCdrom();
int GetPS2ElfName(char*);

extern char *LabelAuthors;
extern char *LabelGreets;
int SaveState(char *file);
int LoadState(char *file);
int CheckState(char *file);

char *ParseLang(char *id);

#ifdef __WIN32__
void ListPatches (HWND hW);
int ReadPatch (HWND hW, char fileName[1024]);
char * lTrim (char *s);
BOOL Save_Patch_Proc( char * filename );
#endif

#define DIRENTRY_SIZE 16

#if defined(__WIN32__)
#pragma pack(1)
#endif

struct romdir{
	char fileName[10];
	u16 extInfoSize;
	u32 fileSize;
#if defined(__WIN32__)
};						//+22
#else
} __attribute__((packed));
#endif

u32 GetBiosVersion();
int IsBIOS(char *filename, char *description);

void * memcpy_amd(void *dest, const void *src, size_t n);

#ifdef	__WIN32__
#pragma pack()
#endif

void injectIRX(char *filename);

// cross-platform atomic operations
#if defined (__MSCW32__)

LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
LONG  __cdecl _InterlockedCompareExchange(LPLONG volatile Dest, LONG Exchange, LONG Comp);
LONG  __cdecl _InterlockedExchange(LPLONG volatile Target, LONG Value);
PVOID __cdecl _InterlockedExchangePointer(PVOID volatile* Target, PVOID Value);

LONG  __cdecl _InterlockedExchangeAdd(LPLONG volatile Addend, LONG Value);
LONG  __cdecl _InterlockedAnd(LPLONG volatile Addend, LONG Value);

#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange 

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

#pragma intrinsic (_InterlockedAnd)
#define InterlockedAnd _InterlockedAnd

//#pragma intrinsic (_InterlockedExchangePointer)
//#define InterlockedExchangePointer _InterlockedExchangePointer

#else

// declare linux equivalents

#endif

#endif /* __MISC_H__ */
