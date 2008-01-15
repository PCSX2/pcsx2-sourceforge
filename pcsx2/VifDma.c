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

#include <math.h>
#include <string.h>

#include "Common.h"
#include "Vif.h"
#include "VUmicro.h"
#include "GS.h"

#include "VifDma.h" 

#include <assert.h>

#define gif ((DMACh*)&PS2MEM_HW[0xA000])

// Extern variables
extern VIFregisters *_vifRegs;
extern vifStruct *_vif;
extern u32* _vifMaskRegs;
extern PCSX2_ALIGNED16_DECL(u32 g_vifRow0[4]);
extern PCSX2_ALIGNED16_DECL(u32 g_vifCol0[4]);
extern PCSX2_ALIGNED16_DECL(u32 g_vifRow1[4]);
extern PCSX2_ALIGNED16_DECL(u32 g_vifCol1[4]);
extern u32* _vifRow, *_vifCol;

vifStruct vif0, vif1;

PCSX2_ALIGNED16(u32 g_vif1Masks[64]);
PCSX2_ALIGNED16(u32 g_vif0Masks[64]);
u32 g_vif1HasMask3[4] = {0}, g_vif0HasMask3[4] = {0};

// Generic constants
static const unsigned int VIF0intc = 4;
static const unsigned int VIF1intc = 5;
static const unsigned int VIF0dmanum = 0;
static const unsigned int VIF1dmanum = 1;

#if defined(_WIN32) && !defined(WIN32_PTHREADS)
extern HANDLE g_hGsEvent;
#else
extern pthread_cond_t g_condGsEvent;
#endif

int g_vifCycles = 0;
int path3hack = 0;

#ifndef __x86_64__
extern void * memcpy_fast(void *dest, const void *src, size_t n);
#endif

typedef void (*UNPACKFUNCTYPE)( u32 *dest, u32 *data, int size );
typedef int  (*UNPACKPARTFUNCTYPESSE)( u32 *dest, u32 *data, int size );
void (*Vif1CMDTLB[82])();
void (*Vif0CMDTLB[75])();
int (*Vif1TransTLB[128])(u32 *data, int size);
int (*Vif0TransTLB[128])(u32 *data, int size);

typedef struct {
	UNPACKFUNCTYPE       funcU;
	UNPACKFUNCTYPE       funcS;

	int bsize; // currently unused
	int dsize; // byte size of one channel
	int gsize; // size of data in bytes used for each write cycle
	int qsize; // used for unpack parts, num of vectors that 
			   // will be decompressed from data for 1 cycle
} VIFUnpackFuncTable;

/* block size; data size; group size; qword size; */
#define _UNPACK_TABLE32(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name,         UNPACK_##name, \
	 bsize, dsize, gsize, qsize },

#define _UNPACK_TABLE(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name##u,      UNPACK_##name##s, \
	 bsize, dsize, gsize, qsize },

// Main table for function unpacking
static const VIFUnpackFuncTable VIFfuncTable[16] = {
	_UNPACK_TABLE32(S_32, 1, 4, 4, 4)		// 0x0 - S-32
	_UNPACK_TABLE(S_16, 2, 2, 2, 4)			// 0x1 - S-16
	_UNPACK_TABLE(S_8, 4, 1, 1, 4)			// 0x2 - S-8
	{ NULL, NULL, 0, 0, 0, 0 },				// 0x3

	_UNPACK_TABLE32(V2_32, 24, 4, 8, 2)		// 0x4 - V2-32
	_UNPACK_TABLE(V2_16, 12, 2, 4, 2)		// 0x5 - V2-16
	_UNPACK_TABLE(V2_8, 6, 1, 2, 2)			// 0x6 - V2-8
	{ NULL, NULL, 0, 0, 0, 0 },	// 0x7
	
	_UNPACK_TABLE32(V3_32, 36, 4, 12, 3)	// 0x8 - V3-32
	_UNPACK_TABLE(V3_16, 18, 2, 6, 3)		// 0x9 - V3-16
	_UNPACK_TABLE(V3_8, 9, 1, 3, 3)			// 0xA - V3-8
	{ NULL, NULL, 0, 0, 0, 0 },				// 0xB

	_UNPACK_TABLE32(V4_32, 48, 4, 16, 4)	// 0xC - V4-32
	_UNPACK_TABLE(V4_16, 24, 2, 8, 4)		// 0xD - V4-16
	_UNPACK_TABLE(V4_8, 12, 1, 4, 4)		// 0xE - V4-8
	_UNPACK_TABLE32(V4_5, 6, 2, 2, 4)		// 0xF - V4-5
};


#if !defined(PCSX2_NORECBUILD)

typedef struct {
	// regular 0, 1, 2; mask 0, 1, 2
	UNPACKPARTFUNCTYPESSE       funcU[9], funcS[9];
} VIFSSEUnpackTable;

#define DECL_UNPACK_TABLE_SSE(name, sign) \
extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_0(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_1(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_2(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_0(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_1(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_2(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_0(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_1(u32* dest, u32* data, int dmasize); \
extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_2(u32* dest, u32* data, int dmasize); \

#define _UNPACK_TABLE_SSE(name, sign) \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_0, \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_1, \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_2, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_0, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_1, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_2, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_0, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_1, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_2 \

#define _UNPACK_TABLE_SSE_NULL \
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL

// Main table for function unpacking
DECL_UNPACK_TABLE_SSE(S_32, u);
DECL_UNPACK_TABLE_SSE(S_16, u);
DECL_UNPACK_TABLE_SSE(S_8, u);
DECL_UNPACK_TABLE_SSE(S_16, s);
DECL_UNPACK_TABLE_SSE(S_8, s);

DECL_UNPACK_TABLE_SSE(V2_32, u);
DECL_UNPACK_TABLE_SSE(V2_16, u);
DECL_UNPACK_TABLE_SSE(V2_8, u);
DECL_UNPACK_TABLE_SSE(V2_16, s);
DECL_UNPACK_TABLE_SSE(V2_8, s);

DECL_UNPACK_TABLE_SSE(V3_32, u);
DECL_UNPACK_TABLE_SSE(V3_16, u);
DECL_UNPACK_TABLE_SSE(V3_8, u);
DECL_UNPACK_TABLE_SSE(V3_16, s);
DECL_UNPACK_TABLE_SSE(V3_8, s);

DECL_UNPACK_TABLE_SSE(V4_32, u);
DECL_UNPACK_TABLE_SSE(V4_16, u);
DECL_UNPACK_TABLE_SSE(V4_8, u);
DECL_UNPACK_TABLE_SSE(V4_16, s);
DECL_UNPACK_TABLE_SSE(V4_8, s);
DECL_UNPACK_TABLE_SSE(V4_5, u);

static const VIFSSEUnpackTable VIFfuncTableSSE[16] = {
	{ _UNPACK_TABLE_SSE(S_32, u), _UNPACK_TABLE_SSE(S_32, u) },
	{ _UNPACK_TABLE_SSE(S_16, u), _UNPACK_TABLE_SSE(S_16, s) },
	{ _UNPACK_TABLE_SSE(S_8, u), _UNPACK_TABLE_SSE(S_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL  },

	{ _UNPACK_TABLE_SSE(V2_32, u), _UNPACK_TABLE_SSE(V2_32, u) },
	{ _UNPACK_TABLE_SSE(V2_16, u), _UNPACK_TABLE_SSE(V2_16, s) },
	{ _UNPACK_TABLE_SSE(V2_8, u), _UNPACK_TABLE_SSE(V2_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL },

	{ _UNPACK_TABLE_SSE(V3_32, u), _UNPACK_TABLE_SSE(V3_32, u) },
	{ _UNPACK_TABLE_SSE(V3_16, u), _UNPACK_TABLE_SSE(V3_16, s) },
	{ _UNPACK_TABLE_SSE(V3_8, u), _UNPACK_TABLE_SSE(V3_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL },

	{ _UNPACK_TABLE_SSE(V4_32, u), _UNPACK_TABLE_SSE(V4_32, u) },
	{ _UNPACK_TABLE_SSE(V4_16, u), _UNPACK_TABLE_SSE(V4_16, s) },
	{ _UNPACK_TABLE_SSE(V4_8, u), _UNPACK_TABLE_SSE(V4_8, s) },
	{ _UNPACK_TABLE_SSE(V4_5, u), _UNPACK_TABLE_SSE(V4_5, u) },
};

#endif


__forceinline void vif0FLUSH() {
	int _cycles;
	_cycles = VU0.cycle;

	vu0Finish();
	g_vifCycles+= (VU0.cycle - _cycles)*BIAS;
}

__forceinline void vif1FLUSH() {
	int _cycles;
	_cycles = VU1.cycle;

	if( VU0.VI[REG_VPU_STAT].UL & 0x100 ) {
		FreezeXMMRegs(1);
		do {
			Cpu->ExecuteVU1Block();
		} while(VU0.VI[REG_VPU_STAT].UL & 0x100);

//		FreezeXMMRegs(0);
//		FreezeMMXRegs(0);
		
		g_vifCycles+= (VU1.cycle - _cycles)*BIAS;
	}
}

void vifDmaInit() {
}

__inline static int _limit( int a, int max ) {
   return ( a > max ? max : a );
}

void DummyExecuteVU1Block(void)
{
	VU0.VI[ REG_VPU_STAT ].UL &= ~0x100;
    VU1.vifRegs->stat &= ~4; // also reset the bit (grandia 3 works)
}

//#define VIFUNPACKDEBUG //enable unpack debugging output 

static void ProcessMemSkip(int size, unsigned int unpackType, const unsigned int VIFdmanum){
	const VIFUnpackFuncTable *unpack;
	vifStruct *vif;
	unpack = &VIFfuncTable[ unpackType ];
//	varLog |= 0x00000400;
	if (VIFdmanum == 0) vif = &vif0;
	else vif = &vif1;

	switch(unpackType){
		case 0x0:
			vif->tag.addr += size*4;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing S-32 skip, size = %d\n", size);
#endif
			break;
		case 0x1:
			vif->tag.addr += size*8;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing S-16 skip, size = %d\n", size);
#endif
			break;
		case 0x2:
			vif->tag.addr += size*16;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing S-8 skip, size = %d\n", size);
#endif
			break;
		case 0x4:
			vif->tag.addr += size + ((size / unpack->gsize) * 8);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V2-32 skip, size = %d\n", size);
#endif
			break;
		case 0x5:
			vif->tag.addr += (size * 2) + ((size / unpack->gsize) * 8);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V2-16 skip, size = %d\n", size);
#endif
			break;
		case 0x6:
			vif->tag.addr += (size * 4) + ((size / unpack->gsize) * 8);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V2-8 skip, size = %d\n", size);
#endif
			break;
		case 0x8:
			vif->tag.addr += size + ((size / unpack->gsize) * 4);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V3-32 skip, size = %d\n", size);
#endif
			break;
		case 0x9:
			vif->tag.addr += (size * 2) + ((size / unpack->gsize) * 4);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V3-16 skip, size = %d\n", size);
#endif
			break;
		case 0xA:
			vif->tag.addr += (size * 4) + ((size / unpack->gsize) * 4);
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V3-8 skip, size = %d\n", size);
#endif
			break;
		case 0xC:
			vif->tag.addr += size;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V4-32 skip, size = %d, CL = %d, WL = %d\n", size, vif1Regs->cycle.cl, vif1Regs->cycle.wl);
#endif
			break;
		case 0xD:
			vif->tag.addr += size * 2;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V4-16 skip, size = %d\n", size);
#endif
			break;
		case 0xE:
			vif->tag.addr += size * 4;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V4-8 skip, size = %d\n", size);
#endif
			break;
		case 0xF:
			vif->tag.addr +=  size * 8;
#ifdef VIFUNPACKDEBUG
			SysPrintf("Processing V4-5 skip, size = %d\n", size);
#endif
			break;		
		default:
			SysPrintf("Invalid unpack type %x\n", unpackType);
			break;
	}
}

#ifdef _MSC_VER
//#define __MMX__
//#define __SSE__
#include <xmmintrin.h>
#include <emmintrin.h>
#endif

static void VIFunpack(u32 *data, vifCode *v, int size, const unsigned int VIFdmanum) {
	u32 *dest;
	unsigned int unpackType;
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	vifStruct *vif;
	VIFregisters *vifRegs;
	VURegs * VU;
	u8 *cdata = (u8*)data;
	int memsize;

#ifdef _MSC_VER
	_mm_prefetch((char*)data, _MM_HINT_NTA);
#endif

	if (VIFdmanum == 0) {
		VU = &VU0;
		vif = &vif0;
		vifRegs = vif0Regs;
		memsize = 0x1000;
		assert( v->addr < 0x1000 );
		v->addr &= 0xfff;
	} else {

		VU = &VU1;
		vif = &vif1;
		vifRegs = vif1Regs;
		memsize = 0x4000;
		assert( v->addr < 0x4000 );
		v->addr &= 0x3fff;

		if( Cpu->ExecuteVU1Block == DummyExecuteVU1Block ) {
			// don't process since the frame is dummy
			vif->tag.addr += (size / (VIFfuncTable[ vif->cmd & 0xf ].gsize* vifRegs->cycle.wl)) * ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);
			return;
		}
	}

	dest = (u32*)(VU->Mem + v->addr);
  
#ifdef VIF_LOG
	VIF_LOG("VIF%d UNPACK: Mode=%x, v->size=%d, size=%d, v->addr=%x\n", 
            VIFdmanum, v->cmd & 0xf, v->size, size, v->addr );
#endif

/*	if (vifRegs->cycle.cl > vifRegs->cycle.wl) {
	   SysPrintf( "VIF%d UNPACK: Mode=%x, v->size=%d, size=%d, v->addr=%x\n", 
            VIFdmanum, v->cmd & 0xf, v->size, size, v->addr );
	}*/
#ifdef _DEBUG
	if (v->size != size) {
#ifdef VIF_LOG
		VIF_LOG("*PCSX2*: warning v->size != size\n");
#endif
	}
	if ((v->addr+size*4) > memsize) {
		SysPrintf("*PCSX2*: fixme unpack overflow\n");
		SysPrintf( "VIF%d UNPACK: Mode=%x, v->size=%d, size=%d, v->addr=%x\n", 
            VIFdmanum, v->cmd & 0xf, v->size, size, v->addr );
	}
#endif
	// The unpack type
	unpackType = v->cmd & 0xf;
	/*if (v->size != size) {
		SysPrintf("*PCSX2*: v->size = %d, size = %d mode = %x\n", v->size, size, unpackType);
	}*/
#ifdef VIFUNPACKDEBUG
	if (size == 0) {
		SysPrintf("*PCSX2*: Unpack %x with size 0!! v->size = %d cl = %d, wl = %d, mode %d mask %x\n", v->cmd, v->size, vifRegs->cycle.cl, vifRegs->cycle.wl, vifRegs->mode, vifRegs->mask);
		//return;
	}
#endif
    
	if(unpackType == 0xC && vifRegs->cycle.cl == vifRegs->cycle.wl) {
		// v4-32
		if(vifRegs->mode == 0 && !(vifRegs->code & 0x10000000)){
			if (v->size != size)ProcessMemSkip(size << 2, unpackType, VIFdmanum);

			memcpy_fast((u8*)dest, cdata, size << 2);
			size = 0;
			return;
		}
	}

#ifdef _MSC_VER
	_mm_prefetch((char*)data+128, _MM_HINT_NTA);
#endif
	_vifRegs = (VIFregisters*)vifRegs;
	_vifMaskRegs = VIFdmanum ? g_vif1Masks : g_vif0Masks;
    _vif = vif;
	_vifRow = VIFdmanum ? g_vifRow1 : g_vifRow0;

	// Unpacking
	//vif->wl = 0; vif->cl = 0;
	
    size<<= 2;
#ifdef _DEBUG
	memsize = size;
#endif
    if( _vifRegs->offset > 0) {
        int destinc, unpacksize;
#ifdef VIFUNPACKDEBUG
        SysPrintf("aligning packet size = %d offset %d addr %x\n", size, vifRegs->offset, vif->tag.addr);
#endif
        // SSE doesn't handle such small data
        ft = &VIFfuncTable[ unpackType ];
        func = vif->usn ? ft->funcU : ft->funcS;
        if(vifRegs->offset < (u32)ft->qsize){
            unpacksize = (ft->qsize - vifRegs->offset);
        } else {
            unpacksize = 0;
            SysPrintf("Unpack align offset = 0\n");
        }
        destinc = 4 - vifRegs->offset;
        func(dest, (u32*)cdata, unpacksize);
        size -= unpacksize*ft->dsize;
        cdata += unpacksize*ft->dsize;

        vifRegs->num--;
        ++vif->cl;
        if (vif->cl == vifRegs->cycle.wl) {
            if(vifRegs->cycle.cl != vifRegs->cycle.wl){
                dest += ((vifRegs->cycle.cl - vifRegs->cycle.wl)<<2) + destinc;
                vif->tag.addr += (destinc<<2) + ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);
            } else {
                dest += destinc;
                vif->tag.addr += destinc << 2;
            }
            vif->cl = 0;	
        }
        else {
            dest += destinc;
            vif->tag.addr += destinc << 2;
        }
#ifdef VIFUNPACKDEBUG
        SysPrintf("aligning packet done size = %d offset %d addr %x\n", size, vifRegs->offset, vif->tag.addr);
#endif
        //}
        if (v->size != (size>>2))ProcessMemSkip(size, unpackType, VIFdmanum);
        //skipmeminc += (((vifRegs->cycle.cl - vifRegs->cycle.wl)<<2)*4) * skipped;
    } else if (v->size != (size>>2))ProcessMemSkip(size, unpackType, VIFdmanum);

	if (vifRegs->cycle.cl >= vifRegs->cycle.wl && size > 0 && vifRegs->num > 0) { // skipping write

#ifdef _DEBUG
		static s_count=0;
#endif
		u32* olddest = dest;
		ft = &VIFfuncTable[ unpackType ];

        if( vif->cl != 0 ) {
            // continuation from last stream
            int incdest;

		    func = vif->usn ? ft->funcU : ft->funcS;
		    incdest = ((vifRegs->cycle.cl - vifRegs->cycle.wl)<<2) + 4;
		    while (size >= ft->gsize && vifRegs->num > 0) {
			    func( dest, (u32*)cdata, ft->qsize);
			    cdata += ft->gsize;
			    size -= ft->gsize;
				
			    vifRegs->num--;
			    ++vif->cl;
			    if (vif->cl == vifRegs->cycle.wl) {
				    dest += incdest;
				    break;
			    }
			    
                dest += 4;
		    }

		    // have to update
		    _vifRow[0] = _vifRegs->r0;
		    _vifRow[1] = _vifRegs->r1;
		    _vifRow[2] = _vifRegs->r2;
		    _vifRow[3] = _vifRegs->r3;
            vif->cl = 0;
        }

#if !defined(PCSX2_NORECBUILD)

		if( size > 0 && vifRegs->num > 0 && !(v->addr&0xf) && cpucaps.hasStreamingSIMD2Extensions) {
			const UNPACKPARTFUNCTYPESSE* pfn;
			int writemask;
			//static LARGE_INTEGER lbase, lfinal;
			//QueryPerformanceCounter(&lbase);
			u32 oldcycle = -1;
			FreezeXMMRegs(1);

//			u16 tempdata[4] = { 0x8000, 0x7fff, 0x1010, 0xd0d0 };
//			vifRegs->cycle.cl = 4;
//			vifRegs->cycle.wl = 1;
//			SetNewMask(g_vif1Masks, g_vif1HasMask3, 0x3f, ~0x3f);
//			memset(dest, 0xcd, 64*4);
//			VIFfuncTableSSE[1].funcS[6](dest, (u32*)tempdata, 8);

#ifdef _MSC_VER

#ifdef __x86_64__
            _vifCol = VIFdmanum ? g_vifCol1 : g_vifCol0;
#else
			if( VIFdmanum ) {
				__asm movaps XMM_ROW, qword ptr [g_vifRow1]
				__asm movaps XMM_COL, qword ptr [g_vifCol1]
			}
			else {
				__asm movaps XMM_ROW, qword ptr [g_vifRow0]
				__asm movaps XMM_COL, qword ptr [g_vifCol0]
			}
#endif

#else
			if( VIFdmanum ) {
				__asm__(".intel_syntax\n"
						"movaps %%xmm6, qword ptr [%0]\n"
						"movaps %%xmm7, qword ptr [%1]\n"
						".att_syntax\n" : :"r"(g_vifRow1), "r"(g_vifCol1) );
			}
			else {
				__asm__(".intel_syntax\n"
						"movaps %%xmm6, qword ptr [%0]\n"
						"movaps %%xmm7, qword ptr [%1]\n"
						".att_syntax\n" : : "r"(g_vifRow0), "r"(g_vifCol0) );
			}
#endif

			if( vifRegs->cycle.cl == 0 || vifRegs->cycle.wl == 0 || (vifRegs->cycle.cl == vifRegs->cycle.wl && !(vifRegs->code&0x10000000)) ) {
				oldcycle = *(u32*)&vifRegs->cycle;
				vifRegs->cycle.cl = vifRegs->cycle.wl = 1;
			}

			size = min(size, (int)vifRegs->num*ft->gsize);
			pfn = vif->usn ? VIFfuncTableSSE[unpackType].funcU: VIFfuncTableSSE[unpackType].funcS;
			writemask = VIFdmanum ? g_vif1HasMask3[min(vifRegs->cycle.wl,3)] : g_vif0HasMask3[min(vifRegs->cycle.wl,3)];
			writemask = pfn[(((vifRegs->code & 0x10000000)>>28)<<writemask)*3+vifRegs->mode](dest, (u32*)cdata, size);

			if( oldcycle != -1 ) *(u32*)&vifRegs->cycle = oldcycle;

			// if size is left over, update the src,dst pointers
			if( writemask > 0 ) {
				int left;
				left = (size-writemask)/ft->gsize;
				cdata += size-writemask;
				dest = (u32*)((u8*)dest + ((left/vifRegs->cycle.wl)*vifRegs->cycle.cl + left%vifRegs->cycle.wl)*16);
				vifRegs->num -= left;
			}
			else vifRegs->num -= size/ft->gsize;
			// Add split transfer skipping
			vif->tag.addr += (size / (ft->gsize* vifRegs->cycle.wl)) * ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);

            // check for left over write cycles (so can spill to next transfer)
            _vif->cl = (size % (ft->gsize*vifRegs->cycle.wl)) / ft->gsize;

			size = writemask;

			_vifRegs->r0 = _vifRow[0];
			_vifRegs->r1 = _vifRow[1];
			_vifRegs->r2 = _vifRow[2];
			_vifRegs->r3 = _vifRow[3];
			//QueryPerformanceCounter(&lfinal);
			//((LARGE_INTEGER*)g_nCounters)->QuadPart += lfinal.QuadPart - lbase.QuadPart;
		}
		else
#endif // !PCSX2_NORECBUILD
		{
			int incdest;

			// Assigning the normal upack function, the part type is assigned later
			func = vif->usn ? ft->funcU : ft->funcS;

			incdest = ((vifRegs->cycle.cl - vifRegs->cycle.wl)<<2) + 4;
			
			//SysPrintf("slow vif\n");
			//if(skipped > 0) skipped = 0;
			// Add split transfer skipping
			vif->tag.addr += (size / (ft->gsize*vifRegs->cycle.wl)) * ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);

			while (size >= ft->gsize && vifRegs->num > 0) {
				func( dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				size -= ft->gsize;
				
				vifRegs->num--;
				//SysPrintf("%d transferred, remaining %d, vifnum %d\n", ft->gsize, size, vifRegs->num);
				++vif->cl;
				if (vif->cl == vifRegs->cycle.wl) {
					dest += incdest;
					vif->cl = 0;					
				}
				else 
				{					
					dest += 4;
				}
			}

			// have to update
			_vifRow[0] = _vifRegs->r0;
			_vifRow[1] = _vifRegs->r1;
			_vifRow[2] = _vifRegs->r2;
			_vifRow[3] = _vifRegs->r3;
		}

		// used for debugging vif
//		{
//			int i, j, k;
//			u32* curdest = olddest;
//            FILE* ftemp = fopen("temp.txt", s_count?"a+":"w");
//			fprintf(ftemp, "%x %x %x\n", s_count, size, vif->tag.addr);
//			fprintf(ftemp, "%x %x %x\n", vifRegs->code>>24, vifRegs->mode, *(u32*)&vifRegs->cycle);
//			fprintf(ftemp, "row: %x %x %x %x\n", _vifRow[0], _vifRow[1], _vifRow[2], _vifRow[3]);
//			//fprintf(ftemp, "row2: %x %x %x %x\n", _vifRegs->r0, _vifRegs->r1, _vifRegs->r2, _vifRegs->r3);
//			
//			for(i = 0; i < memsize; ) {
//                for(k = 0; k < vifRegs->cycle.wl; ++k) {
//				    for(j = 0; j <= ((vifRegs->code>>26)&3); ++j) {
//					    fprintf(ftemp, "%x ", curdest[4*k+j]);
//				    }
//                }
//
//				fprintf(ftemp, "\n");
//				curdest += 4*vifRegs->cycle.cl;
//				i += (((vifRegs->code>>26)&3)+1)*ft->dsize*vifRegs->cycle.wl;
//			}
//			fclose(ftemp);
//		}
//		s_count++;

		if( size >= ft->dsize && vifRegs->num > 0) {
	#ifdef VIF_LOG
			VIF_LOG("warning, end with size = %d\n", size);
	#endif
			// SSE doesn't handle such small data
			ft = &VIFfuncTable[ unpackType ];
			func = vif->usn ? ft->funcU : ft->funcS;
	#ifdef VIFUNPACKDEBUG
			SysPrintf("end with size %x dsize = %x last cdata %x\n", size, ft->dsize, (u32)cdata);
	#endif
			while (size >= ft->dsize) {
					/* unpack one qword */
					func(dest, (u32*)cdata, 1);
					dest += 1;
					size -= ft->dsize;
			}
	#ifdef VIFUNPACKDEBUG
			SysPrintf("leftover done, size %d, vifnum %d, addr %x\n", size, vifRegs->num, vif->tag.addr);
	#endif
		}
		
	}
	else if (vifRegs->cycle.cl < vifRegs->cycle.wl) { /* filling write */
#ifdef VIF_LOG
		VIF_LOG("*PCSX2*: filling write\n");
#endif
		ft = &VIFfuncTable[ unpackType ];
		func = vif->usn ? ft->funcU : ft->funcS;
#ifdef VIFUNPACKDEBUG
		SysPrintf("filling write %d cl %d, wl %d\n", vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl);
#endif
		while (size >= ft->gsize || vifRegs->num > 0) {
			if (vif->cl == vifRegs->cycle.wl) {
				vif->cl = 0;
			}
			//
			if (vif->cl < vifRegs->cycle.cl) { /* unpack one qword */
				func( dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				size -= ft->gsize;
				vif->cl++;
				vifRegs->num--;
				if (vif->cl == vifRegs->cycle.wl) {
	   	      		vif->cl = 0;
				}
			} 
			else
			{
				func( dest, (u32*)cdata, ft->qsize);
				//cdata += ft->gsize;
				//size -= ft->gsize;
				vif->tag.addr += 16;
				vifRegs->num--;
				++vif->cl;

			}
			dest += 4;
			//++vif->wl;
			if(vifRegs->num == 0) break;
		}
	} 

	if(vifRegs->num == 0 && size > 3) SysPrintf("Size = %x, Vifnum = 0!\n", size);
}

static void vuExecMicro( u32 addr, const u32 VIFdmanum )
{
	int _cycles;
	VURegs * VU;
	//void (*_vuExecMicro)();

//	MessageBox(NULL, "3d doesn't work\n", "Query", MB_OK);
//	return;

	if (VIFdmanum == 0) {
		//_vuExecMicro = Cpu->ExecuteVU0Block;
		VU = &VU0;
		vif0FLUSH();
	} else {
		//_vuExecMicro = Cpu->ExecuteVU1Block;
		VU = &VU1;
		vif1FLUSH();
	}
	if(VU->vifRegs->itops > (VIFdmanum ? 0x3ffu : 0xffu)) 
		SysPrintf("VIF%d ITOP overrun! %x\n", VIFdmanum, VU->vifRegs->itops);

	VU->vifRegs->itop = VU->vifRegs->itops;

	if (VIFdmanum == 1) {
		/* in case we're handling a VIF1 execMicro 
		   set the top with the tops value */
		VU->vifRegs->top = VU->vifRegs->tops & 0x3ff;

		/* is DBF flag set in VIF_STAT? */
		if (VU->vifRegs->stat & 0x80) {
			/* it is, so set tops with base + ofst 
			   and clear stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base;
			VU->vifRegs->stat &= ~0x80;
		} else {
			/* it is not, so set tops with base
			   and set the stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base + VU->vifRegs->ofst;
			VU->vifRegs->stat |= 0x80;
		}
	}

	if (VIFdmanum == 0) {
		_cycles = VU0.cycle;
		vu0ExecMicro(addr);
		// too much delay
		//g_vifCycles+= (VU0.cycle - _cycles)*BIAS;
	} else {
		_cycles = VU1.cycle;
		vu1ExecMicro(addr);
		// too much delay
		//g_vifCycles+= (VU1.cycle - _cycles)*BIAS;
	}
}

u8 s_maskwrite[256];
void vif0Init()
{

	u32 i;

	for(i = 0; i < 256; ++i ) {
		s_maskwrite[i] = ((i&3)==3)||((i&0xc)==0xc)||((i&0x30)==0x30)||((i&0xc0)==0xc0);
	}

	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);
}

__inline void vif0UNPACK(u32 *data) {
	int vifNum;
    int vl, vn;
    int len;

	if(vif0Regs->cycle.wl == 0 && vif0Regs->cycle.wl < vif0Regs->cycle.cl){
		SysPrintf("Vif0 CL %d, WL %d\n", vif0Regs->cycle.cl, vif0Regs->cycle.wl);
		vif0.cmd &= ~0x7f;
		return;
	}

	vif0FLUSH();

    vl = (vif0.cmd     ) & 0x3;
    vn = (vif0.cmd >> 2) & 0x3;
    vif0.tag.addr = (data[0] & 0x3ff) << 4;
    vif0.usn = (data[0] >> 14) & 0x1;
    vifNum = (data[0] >> 16) & 0xff;
	if ( vifNum == 0 ) vifNum = 256;
	vif0Regs->num = vifNum;

    if ( vif0Regs->cycle.wl <= vif0Regs->cycle.cl ) {
        len = ((( 32 >> vl ) * ( vn + 1 )) * vifNum + 31) >> 5;
    } else {
        int n = vif0Regs->cycle.cl * (vifNum / vif0Regs->cycle.wl) + 
                _limit( vifNum % vif0Regs->cycle.wl, vif0Regs->cycle.cl );

		len = ( ((( 32 >> vl ) * ( vn + 1 )) * n) + 31 ) >> 5;
    }
	//if((vif0.tag.addr + (vifNum * 16)) > 0x1000) SysPrintf("VIF0 Oops, Addr %x, NUM %x overlaps to %x\n", vif0.tag.addr, vifNum, (vif0.tag.addr + (vifNum * 16)));
	vif0.wl = 0; vif0.cl = 0;
    vif0.tag.cmd  = vif0.cmd;
	vif0.tag.addr &= 0xfff;
    vif0.tag.size = len;
    vif0Regs->offset = 0;
}

__inline void _vif0mpgTransfer(u32 addr, u32 *data, int size) {
/*	SysPrintf("_vif0mpgTransfer addr=%x; size=%x\n", addr, size);
	{
		FILE *f = fopen("vu1.raw", "wb");
		fwrite(data, 1, size*4, f);
		fclose(f);
	}*/
	if (memcmp(VU0.Micro + addr, data, size << 2)) {
		memcpy_fast(VU0.Micro + addr, data, size << 2);
		Cpu->ClearVU0(addr, size);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int Vif0TransNull(u32 *data, int size){ // Shouldnt go here
	SysPrintf("VIF0 Shouldnt go here CMD = %x\n", vif0Regs->code);
	vif0.cmd &= ~0x7f;
	return 0;
}
static int Vif0TransSTMask(u32 *data, int size){ // STMASK
	SetNewMask(g_vif0Masks, g_vif0HasMask3, data[0], vif0Regs->mask);
	vif0Regs->mask = data[0];
#ifdef VIF_LOG
	VIF_LOG("STMASK == %x\n", vif0Regs->mask);
#endif
	vif0.tag.size = 0;
	vif0.cmd &= ~0x7f;
	return 1;
}

static int Vif0TransSTRow(u32 *data, int size){ // STROW
    int ret;

	u32* pmem = &vif0Regs->r0+(vif0.tag.addr<<2);
	u32* pmem2 = g_vifRow0+vif0.tag.addr;
	assert( vif0.tag.addr < 4 );
	ret = min(4-vif0.tag.addr, size);
	assert( ret > 0 );
	switch(ret) {
		case 4: pmem[12] = data[3]; pmem2[3] = data[3];
		case 3: pmem[8] = data[2]; pmem2[2] = data[2];
		case 2: pmem[4] = data[1]; pmem2[1] = data[1];
		case 1: pmem[0] = data[0]; pmem2[0] = data[0]; break;
#ifdef _MSC_VER
		default: __assume(0);
#endif
	}
    vif0.tag.addr += ret;
    vif0.tag.size -= ret;
	if(vif0.tag.size == 0) vif0.cmd &= ~0x7f;

	return ret;
}

static int Vif0TransSTCol(u32 *data, int size){ // STCOL
	int ret;

	u32* pmem = &vif0Regs->c0+(vif0.tag.addr<<2);
	u32* pmem2 = g_vifCol0+vif0.tag.addr;
	ret = min(4-vif0.tag.addr, size);
    switch(ret) {
		case 4: pmem[12] = data[3]; pmem2[3] = data[3];
		case 3: pmem[8] = data[2]; pmem2[2] = data[2];
		case 2: pmem[4] = data[1]; pmem2[1] = data[1];
		case 1: pmem[0] = data[0]; pmem2[0] = data[0]; break;
#ifdef _MSC_VER
		default: __assume(0);
#endif
	}
	vif0.tag.addr += ret;
    vif0.tag.size -= ret;
	if(vif0.tag.size == 0) vif0.cmd &= ~0x7f;
	return ret;
}

static int Vif0TransMPG(u32 *data, int size){ // MPG
	if (size < vif0.tag.size) {
		_vif0mpgTransfer(vif0.tag.addr, data, size);
        vif0.tag.addr += size << 2;
        vif0.tag.size -= size; 
        return size;
    } else {
		int ret;
		_vif0mpgTransfer(vif0.tag.addr, data, vif0.tag.size);
		ret = vif0.tag.size;
        vif0.tag.size = 0;
		vif0.cmd &= ~0x7f;
		return ret;
    }
}

static int Vif0TransUnpack(u32 *data, int size){ // UNPACK
	if (size < vif0.tag.size) {
			/* size is less that the total size, transfer is 
			   'in pieces' */
			VIFunpack(data, &vif0.tag, size, VIF0dmanum);
		//	g_vifCycles+= size >> 1;
			//vif0.tag.addr += size << 2;
			vif0.tag.size -= size; 
			return size;
		} else {
			int ret;
			/* we got all the data, transfer it fully */
			VIFunpack(data, &vif0.tag, vif0.tag.size, VIF0dmanum);
			//g_vifCycles+= vif0.tag.size >> 1;
			ret = vif0.tag.size;
			vif0.tag.size = 0;
			vif0.cmd &= ~0x7f;
			return ret;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif0 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Vif0CMDNop(){ // NOP
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTCycl(){ // STCYCL
	vif0Regs->cycle.cl =  (u8)vif0Regs->code;
    vif0Regs->cycle.wl = (u8)(vif0Regs->code >> 8);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDITop(){ // ITOP
	vif0Regs->itops = vif0Regs->code & 0x3ff;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMod(){ // STMOD
	vif0Regs->mode = vif0Regs->code & 0x3;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMark(){ // MARK
	vif0Regs->mark = (u16)vif0Regs->code;
	vif0Regs->stat |= VIF0_STAT_MRK;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDFlushE(){ // FLUSHE
	vif0FLUSH();
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCALF(){ //MSCAL/F
	vuExecMicro( (u16)(vif0Regs->code) << 3, VIF0dmanum );
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCNT(){ // MSCNT
	vuExecMicro( -1, VIF0dmanum );
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMask(){ // STMASK 
	vif0.tag.size = 1;
}

static void Vif0CMDSTRowCol(){// STROW / STCOL
	vif0.tag.addr = 0;
    vif0.tag.size = 4;
}

static void Vif0CMDMPGTransfer(){ // MPG
	int vifNum;
	vif0FLUSH();
    vifNum = (u8)(vif0Regs->code >> 16);
    if (vifNum == 0) vifNum = 256;
    vif0.tag.addr = (u16)(vif0Regs->code) << 3;
    vif0.tag.size = vifNum * 2;
}

static void Vif0CMDNull(){ // invalid opcode
	// if ME1, then force the vif to interrupt
     if ((vif0Regs->err & 0x4) == 0) {  //Ignore vifcode and tag mismatch error
			SysPrintf( "UNKNOWN VifCmd: %x\n", vif0.cmd );
            vif0Regs->stat |= 1 << 13;
			vif0.irq++;
     }
	 vif0.cmd &= ~0x7f;
}

int VIF0transfer(u32 *data, int size, int istag) {
	int ret;
	int transferred=vif0.vifstalled ? vif0.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)
	//vif0.irqoffset = 0;
#ifdef VIF_LOG 
	VIF_LOG( "VIF0transfer: size %x (vif0.cmd %x)\n", size, vif0.cmd );
#endif

	vif0.stallontag = 0;
	vif0.vifstalled = 0;

	while (size > 0) {

		if (vif0.cmd & 0x7f) {
			//vif0Regs->stat |= VIF0_STAT_VPS_T;
			ret = Vif0TransTLB[(vif0.cmd & 0x7f)](data, size);
			data+= ret; size-= ret;
			transferred+= ret;
			//vif0Regs->stat &= ~VIF0_STAT_VPS_T;
			continue;
		}
		
		vif0Regs->stat &= ~VIF0_STAT_VPS_W;

		
		if(vif0.tag.size > 0) SysPrintf("VIF0 Tag size %x when cmd == 0!\n", vif0.tag.size);
		// if interrupt and new cmd is NOT MARK
		if(vif0.irq && vif0.tag.size == 0) {
			break;
		}

		vif0.cmd = (data[0] >> 24);
		vif0Regs->code = data[0];
		

		//vif0Regs->stat |= VIF0_STAT_VPS_D;
		if ((vif0.cmd & 0x60) == 0x60) {
			vif0UNPACK(data);
		} else {
#ifdef VIF_LOG 
		VIF_LOG( "VIFtransfer: cmd %x, num %x, imm %x, size %x\n", vif0.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, size );
#endif
			if((vif0.cmd & 0x7f) > 0x4A){
				if ((vif0Regs->err & 0x4) == 0) {  //Ignore vifcode and tag mismatch error
						SysPrintf( "UNKNOWN VifCmd: %x\n", vif0.cmd );
						vif0Regs->stat |= 1 << 13;
						vif0.irq++;
				 }
				vif0.cmd = 0;
			} else Vif0CMDTLB[(vif0.cmd & 0x7f)]();
		}
		//vif0Regs->stat &= ~VIF0_STAT_VPS_D;
		if(vif0.tag.size > 0) vif0Regs->stat |= VIF0_STAT_VPS_W;
		++data; 
		--size;
		++transferred;

		if ((vif0.cmd & 0x80) && !(vif0Regs->err & 0x1) ) { //i bit on vifcode and not masked by VIF0_ERR
#ifdef VIF_LOG
			VIF_LOG( "Interrupt on VIFcmd: %x (INTC_MASK = %x)\n", vif0.cmd, psHu32(INTC_MASK) );
#endif
			
			++vif0.irq;
			vif0.cmd &= 0x7f;
			if(istag && vif0.tag.size == 0) vif0.stallontag = 1;
		} 
	}
	g_vifCycles+= (transferred >> 2)*BIAS; /* guessing */
	// use tag.size because some game doesn't like .cmd
	//if( !vif0.cmd )
	if( !vif0.tag.size )
		vif0Regs->stat &= ~VIF0_STAT_VPS_W;
		
	if (vif0.irq && vif0.tag.size == 0) {
		vif0.vifstalled = 1;

		if(((vif0Regs->code >> 24) & 0x7f) != 0x7)vif0Regs->stat|= VIF0_STAT_VIS;
		else SysPrintf("VIF0 IRQ on MARK\n");
		// spiderman doesn't break on qw boundaries
		vif0.irqoffset = transferred%4; // cannot lose the offset
		if(vif0.tag.size > 0) SysPrintf("Oh dear, possible VIF0 stall problem. CMD %x, tag.size %x\n", vif0.cmd, vif0.tag.size);
		if( istag ) {
			return -2;
		}
		
		transferred = transferred >> 2;
		vif0ch->madr+= (transferred << 4);
		vif0ch->qwc-= transferred;
		//SysPrintf("Stall on vif0, FromSPR = %x, Vif0MADR = %x Sif0MADR = %x STADR = %x\n", psHu32(0x1000d010), vif0ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return -2;
	}

	if( !istag ) {
		transferred = transferred >> 2;
		vif0ch->madr+= (transferred << 4);
		vif0ch->qwc-= transferred;
	}
	
	return 0;
}

int  _VIF0chain() {
	u32 *pMem;
	u32 qwc = vif0ch->qwc;
	u32 ret;

	if (vif0ch->qwc == 0 && vif0.vifstalled == 0) return 0;

	pMem = (u32*)dmaGetAddr(vif0ch->madr);
	if (pMem == NULL)
		return -1;

	if( vif0.vifstalled ) {
		ret = VIF0transfer(pMem+vif0.irqoffset, vif0ch->qwc*4-vif0.irqoffset, 0);
	}
	else {
		ret = VIF0transfer(pMem, vif0ch->qwc*4, 0);
	}
	/*vif0ch->madr+= (vif0ch->qwc << 4);
	vif0ch->qwc-= qwc;*/
	
	return ret;
}


int _chainVIF0() {
	int id;
	u32 *ptag;
	//int done=0;
	int ret;
	
	ptag = (u32*)dmaGetAddr(vif0ch->tadr); //Set memory pointer to TADR
	if (ptag == NULL) {						//Is ptag empty?
		SysPrintf("Vif0 Tag BUSERR\n");
		vif0ch->chcr = ( vif0ch->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 ); //Transfer upper part of tag to CHCR bits 31-15
		psHu32(DMAC_STAT)|= 1<<15;          //If yes, set BEIS (BUSERR) in DMAC_STAT register
		return -1;						   //Return -1 as an error has occurred
	}
	
	id        = (ptag[0] >> 28) & 0x7; //ID for DmaChain copied from bit 28 of the tag
	vif0ch->qwc  = (u16)ptag[0];       //QWC set to lower 16bits of the tag
	vif0ch->madr = ptag[1];            //MADR = ADDR field
	g_vifCycles+=1; // Add 1 g_vifCycles from the QW read for the tag
#ifdef VIF_LOG
	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			ptag[1], ptag[0], vif0ch->qwc, id, vif0ch->madr, vif0ch->tadr);
#endif

	vif0ch->chcr = ( vif0ch->chcr & 0xFFFF ) | ( (*ptag) & 0xFFFF0000 ); //Transfer upper part of tag to CHCR bits 31-15
	// Transfer dma tag if tte is set
	
	if (vif0ch->chcr & 0x40) {
		if(vif0.vifstalled == 1) ret = VIF0transfer(ptag+(2+vif0.irqoffset), 2-vif0.irqoffset, 1);  //Transfer Tag on stall
		else ret = VIF0transfer(ptag+2, 2, 1);  //Transfer Tag
		if (ret == -1) return -1;       //There has been an error
		if (ret == -2) {
			//SysPrintf("VIF0 Stall on tag %x\n", vif0.irqoffset);
			//vif0.vifstalled = 1;
			return vif0.done;        //IRQ set by VIFTransfer
		}
	}
	
	vif0.done |= hwDmacSrcChainWithStack(vif0ch, id);

#ifdef VIF_LOG
		VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
				ptag[1], ptag[0], vif0ch->qwc, id, vif0ch->madr, vif0ch->tadr);
#endif

	//done |= hwDmacSrcChainWithStack(vif0ch, id);
	ret = _VIF0chain();											   //Transfers the data set by the switch
	if (ret == -1) { return -1; }									   //There's been an error
	if (ret == -2) { 							   //IRQ has been set by VifTransfer
		//vif0.vifstalled = 1;
		return vif0.done;
	}

	//if(id == 7)vif0ch->tadr = vif0ch->madr;

	vif0.vifstalled = 0;
	
	if ((vif0ch->chcr & 0x80) && (ptag[0] >> 31)) {			       //Check TIE bit of CHCR and IRQ bit of tag
#ifdef VIF_LOG
		VIF_LOG( "dmaIrq Set\n" );
#endif
		//SysPrintf("VIF0 TIE\n");
		//SysPrintf( "VIF0dmaIrq Set\n" );
		//vif0ch->qwc = 0;
		//vif0Regs->stat|= VIF0_STAT_VIS;							   //Set the Tag Interrupt flag of VIF0_STAT
		vif0.done = 1;
		return vif0.done;												   //End Transfer
	}
	return vif0.done;												   //Return Done
}

void  vif0Interrupt() {
	int ret;
	g_vifCycles = 0; //Reset the cycle count, Wouldnt reset on stall if put lower down.
#ifdef VIF_LOG 
	VIF_LOG("vif0Interrupt: %8.8x\n", cpuRegs.cycle);
#endif

	//if(vif0.vifstalled == 1) {
		if(vif0.irq && vif0.tag.size == 0) {
			vif0Regs->stat|= VIF0_STAT_INT;
			hwIntcIrq(VIF0intc);
			--vif0.irq;	
			
			if (vif0Regs->stat & (VIF0_STAT_VSS|VIF0_STAT_VIS|VIF0_STAT_VFS)) 
			{
				vif0Regs->stat&= ~0xF000000; // FQC=0
				vif0ch->chcr &= ~0x100;
				cpuRegs.interrupt &= ~1;
				return;
			}					
		}
		
	//}
	if((vif0ch->chcr & 0x100) == 0) {
		SysPrintf("Vif0 running when CHCR = %x\n", vif0ch->chcr);
		/*prevviftag = NULL;
	prevvifcycles = 0;
	vif1ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF1);
	hwIntcIrq(VIF1intc);
	vif1Regs->stat&= ~0x1F000000; // FQC=0
		return 1;*/
	}
	if (vif0ch->chcr & 0x4 && vif0.done == 0 && vif0.vifstalled == 0) {

		if( !(psHu32(DMAC_CTRL) & 0x1) ) {
			SysPrintf("vif0 dma masked\n");
			return;
		}

		if(vif0ch->qwc > 0) _VIF0chain();
		ret = _chainVIF0();
		INT(0, g_vifCycles);
		FreezeMMXRegs(0);
		FreezeXMMRegs(0);
		return;
		//if(ret!=2)
		/*else*/ //return 1;
	}
	

	if(vif0ch->qwc > 0) SysPrintf("VIF0 Ending with QWC left\n");

	vif0ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs->stat&= ~0xF000000; // FQC=0

	cpuRegs.interrupt &= ~1;
}

//  Vif1 Data Transfer Table
int (*Vif0TransTLB[128])(u32 *data, int size) = 
{
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x7*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0xF*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x17*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x1F*/
	Vif0TransSTMask  , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x27*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x2F*/
	Vif0TransSTRow	 , Vif0TransSTCol	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x37*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x3F*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x47*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransMPG	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x4F*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , /*0x57*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x5F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x67*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , /*0x6F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x77*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransNull   , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack   /*0x7F*/
};

// Vif1 CMD Table
void (*Vif0CMDTLB[75])() = 
{
	Vif0CMDNop	   , Vif0CMDSTCycl  , Vif0CMDNull		, Vif0CMDNull , Vif0CMDITop  , Vif0CMDSTMod , Vif0CMDNull, Vif0CMDMark , /*0x7*/
	Vif0CMDNull	   , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0xF*/
	Vif0CMDFlushE   , Vif0CMDNull   , Vif0CMDNull		, Vif0CMDNull, Vif0CMDMSCALF, Vif0CMDMSCALF, Vif0CMDNull	, Vif0CMDMSCNT, /*0x17*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x1F*/
	Vif0CMDSTMask  , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x27*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x2F*/
	Vif0CMDSTRowCol, Vif0CMDSTRowCol, Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x37*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x3F*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x47*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDMPGTransfer
};

void dmaVIF0() {

#ifdef VIF_LOG
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
			"        tadr = %lx, asr0 = %lx, asr1 = %lx\n",
			vif0ch->chcr, vif0ch->madr, vif0ch->qwc,
			vif0ch->tadr, vif0ch->asr0, vif0ch->asr1 );
#endif

	/* Check if there is a pending irq */
	/*if (vif0.irq > 0) {
		vif0.irq--;
		hwIntcIrq(VIF0intc);
		return;
	}*/
//	if(vif0ch->qwc > 0) {
//		 _VIF0chain();
//		 INT(0, g_vifCycles);
//	}
	g_vifCycles = 0;

	
	vif0Regs->stat|= 0x8000000; // FQC=8

	if (!(vif0ch->chcr & 0x4) || vif0ch->qwc > 0) { // Normal Mode 
		if(_VIF0chain() == -2) {
			SysPrintf("Stall on normal %x\n", vif0Regs->stat);
			//vif0.vifstalled = 1;
			return;
		}
		vif0.done = 1;
		INT(0, g_vifCycles);
		FreezeMMXRegs(0);
		FreezeXMMRegs(0);
		return;
	}

/*	if (_VIF0chain() != 0) {
		INT(0, g_vifCycles);
		return;
	}*/
	// Chain Mode
	vif0.done = 0;
	INT(0, g_vifCycles);
}


void vif0Write32(u32 mem, u32 value) {
	if (mem == 0x10003830) { // MARK
#ifdef VIF_LOG
		VIF_LOG("VIF0_MARK write32 0x%8.8x\n", value);
#endif
		/* Clear mark flag in VIF0_STAT and set mark with 'value' */
		vif0Regs->stat&= ~VIF0_STAT_MRK;
		vif0Regs->mark = value;
	} else
	if (mem == 0x10003810) { // FBRST
#ifdef VIF_LOG
		VIF_LOG("VIF0_FBRST write32 0x%8.8x\n", value);
#endif
		if (value & 0x1) {
			/* Reset VIF */
			//SysPrintf("Vif0 Reset %x\n", vif0Regs->stat);
			memset(&vif0, 0, sizeof(vif0));
			vif0ch->qwc = 0; //?
			psHu64(0x10004000) = 0;
			psHu64(0x10004008) = 0;
			vif0.done = 1;
			vif0Regs->err = 0;
			vif0Regs->stat&= ~(0xF000000|VIF0_STAT_INT|VIF0_STAT_VSS|VIF0_STAT_VIS|VIF0_STAT_VFS); // FQC=0
		}
		if (value & 0x2) {
			/* Force Break the VIF */
			/* I guess we should stop the VIF dma here 
			   but not 100% sure (linuz) */
			vif0Regs->stat |= VIF0_STAT_VFS;
			SysPrintf("vif0 force break\n");
		}
		if (value & 0x4) {
			/* Stop VIF */
			/* Not completly sure about this, can't remember what game 
			   used this, but 'draining' the VIF helped it, instead of 
			   just stoppin the VIF (linuz) */
			vif0Regs->stat |= VIF0_STAT_VSS;
			//SysPrintf("Vif0 Stop\n");
			//dmaVIF0();	// Drain the VIF  --- VIF Stops as not to outstrip dma source (refraction)
			//FreezeXMMRegs(0);
		}
		if (value & 0x8) {
			int cancel = 0;

			/* Cancel stall, first check if there is a stall to cancel, 
			   and then clear VIF0_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
			if (vif0Regs->stat & (VIF0_STAT_VSS|VIF0_STAT_VIS|VIF0_STAT_VFS)) {
				cancel = 1;
			}

			vif0Regs->stat &= ~(VIF0_STAT_VSS | VIF0_STAT_VFS | VIF0_STAT_VIS |
								VIF0_STAT_INT | VIF0_STAT_ER0 | VIF0_STAT_ER1);
			if (cancel) {
				//SysPrintf("VIF0 Stall Resume\n");
				if( vif0.vifstalled ) {
					// loop necessary for spiderman
					if(vif0.stallontag == 1){
							//SysPrintf("Sorting VIF0 Stall on tag\n");
							_chainVIF0();
						} else _VIF0chain();
						
					vif0ch->chcr |= 0x100;
					INT(0, g_vifCycles); // Gets the timing right - Flatout
				}
				FreezeMMXRegs(0);
				FreezeXMMRegs(0);
			}
		}			
	} else
	if (mem == 0x10003820) { // ERR
#ifdef VIF_LOG
		VIF_LOG("VIF0_ERR write32 0x%8.8x\n", value);
#endif
		/* Set VIF0_ERR with 'value' */
		vif0Regs->err = value;
	} else{
		SysPrintf("Unknown Vif0 write to %x\n", mem);
		if( mem >= 0x10003900 && mem < 0x10003980 ) {
			
			assert( (mem&0xf) == 0 );
			if( mem < 0x10003940 ) g_vifRow0[(mem>>4)&3] = value;
			else g_vifCol0[(mem>>4)&3] = value;
		} else psHu32(mem) = value;
	}

	/* Other registers are read-only so do nothing for them */
}

void vif0Reset() {
	/* Reset the whole VIF, meaning the internal pcsx2 vars
	   and all the registers */
	memset(&vif0, 0, sizeof(vif0));
	memset(vif0Regs, 0, sizeof(vif0Regs));
	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);
	psHu64(0x10004000) = 0;
	psHu64(0x10004008) = 0;
	vif0.done = 1;
	vif0Regs->stat&= ~0xF000000; // FQC=0
	FreezeMMXRegs(0);
	FreezeXMMRegs(0);
}

int vif0Freeze(gzFile f, int Mode) {
	gzfreeze(&vif0, sizeof(vif0));
	if (Mode == 0)
		SetNewMask(g_vif0Masks, g_vif0HasMask3, vif0Regs->mask, ~vif0Regs->mask);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


void vif1Init() {
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
}

__inline void vif1UNPACK(u32 *data) {
	int vifNum;
    int vl, vn;
    int len;

	if(vif1Regs->cycle.wl == 0 && vif1Regs->cycle.wl < vif1Regs->cycle.cl){
		SysPrintf("Vif1 CL %d, WL %d\n", vif1Regs->cycle.cl, vif1Regs->cycle.wl);
		vif1.cmd &= ~0x7f;
		return;
	}
	vif1FLUSH();

    vl = (vif1.cmd     ) & 0x3;
    vn = (vif1.cmd >> 2) & 0x3;
    vif1.tag.addr = (data[0] & 0x3ff);
    vif1.usn = (data[0] >> 14) & 0x1;
    vifNum = (data[0] >> 16) & 0xff;
    if ( vifNum == 0 ) vifNum = 256;
	vif1Regs->num = vifNum;

    if ( vif1Regs->cycle.wl <= vif1Regs->cycle.cl ) {
        len = ((( 32 >> vl ) * ( vn + 1 )) * vifNum + 31) >> 5;
    } else {
        int n = vif1Regs->cycle.cl * (vifNum / vif1Regs->cycle.wl) + 
                _limit( vifNum % vif1Regs->cycle.wl, vif1Regs->cycle.cl );
        len = ( ((( 32 >> vl ) * ( vn + 1 )) * n) + 31 ) >> 5;
    }
   if ( ( data[0] >> 15) & 0x1 ) {
        vif1.tag.addr += (vif1Regs->tops & 0x3ff);
    }    
	vif1.wl = 0; vif1.cl = 0;
    vif1.tag.addr <<= 4;
	//if((vif1.tag.addr + (vifNum * 16)) > 0x4000) SysPrintf("Oops, Addr %x, NUM %x overlaps to %x\n", vif1.tag.addr, vifNum, (vif1.tag.addr + (vifNum * 16)));
	vif1.tag.addr &= 0x3fff;
    vif1.tag.cmd  = vif1.cmd;
	vif1.tag.size = len;
    vif1Regs->offset = 0;
}

__inline void _vif1mpgTransfer(u32 addr, u32 *data, int size) {
/*	SysPrintf("_vif1mpgTransfer addr=%x; size=%x\n", addr, size);
	{
		FILE *f = fopen("vu1.raw", "wb");
		fwrite(data, 1, size*4, f);
		fclose(f);
	}*/
    assert( VU1.Micro > 0 );
	if (memcmp(VU1.Micro + addr, data, size << 2)) {
		memcpy_fast(VU1.Micro + addr, data, size << 2);
		Cpu->ClearVU1(addr, size);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int Vif1TransNull(u32 *data, int size){ // Shouldnt go here
	SysPrintf("Shouldnt go here CMD = %x\n", vif1Regs->code);
	vif1.cmd &= ~0x7f;
	return 0;
}
static int Vif1TransSTMask(u32 *data, int size){ // STMASK
	SetNewMask(g_vif1Masks, g_vif1HasMask3, data[0], vif1Regs->mask);
	vif1Regs->mask = data[0];
#ifdef VIF_LOG
	VIF_LOG("STMASK == %x\n", vif1Regs->mask);
#endif
	vif1.tag.size = 0;
	vif1.cmd &= ~0x7f;
	return 1;
}

static int Vif1TransSTRow(u32 *data, int size){
    int ret;

	u32* pmem = &vif1Regs->r0+(vif1.tag.addr<<2);
	u32* pmem2 = g_vifRow1+vif1.tag.addr;
	assert( vif1.tag.addr < 4 );
	ret = min(4-vif1.tag.addr, size);
	assert( ret > 0 );
	switch(ret) {
		case 4: pmem[12] = data[3]; pmem2[3] = data[3];
		case 3: pmem[8] = data[2]; pmem2[2] = data[2];
		case 2: pmem[4] = data[1]; pmem2[1] = data[1];
		case 1: pmem[0] = data[0]; pmem2[0] = data[0]; break;
#ifdef _MSC_VER
		default: __assume(0);
#endif
	}
    vif1.tag.addr += ret;
    vif1.tag.size -= ret;
	if(vif1.tag.size == 0) vif1.cmd &= ~0x7f;

	return ret;
}

static int Vif1TransSTCol(u32 *data, int size){
	int ret;

	u32* pmem = &vif1Regs->c0+(vif1.tag.addr<<2);
	u32* pmem2 = g_vifCol1+vif1.tag.addr;
	ret = min(4-vif1.tag.addr, size);
    switch(ret) {
		case 4: pmem[12] = data[3]; pmem2[3] = data[3];
		case 3: pmem[8] = data[2]; pmem2[2] = data[2];
		case 2: pmem[4] = data[1]; pmem2[1] = data[1];
		case 1: pmem[0] = data[0]; pmem2[0] = data[0]; break;
#ifdef _MSC_VER
		default: __assume(0);
#endif
	}
	vif1.tag.addr += ret;
    vif1.tag.size -= ret;
	if(vif1.tag.size == 0) vif1.cmd &= ~0x7f;
	return ret;
}

static int Vif1TransMPG(u32 *data, int size){
	if (size < vif1.tag.size) {
		_vif1mpgTransfer(vif1.tag.addr, data, size);
        vif1.tag.addr += size << 2;
        vif1.tag.size -= size; 
        return size;
    } else {
		int ret;
		_vif1mpgTransfer(vif1.tag.addr, data, vif1.tag.size);
		ret = vif1.tag.size;
        vif1.tag.size = 0;
		vif1.cmd &= ~0x7f;
		return ret;
    }
}
u32 splittransfer[4];
u32 splitptr = 0;

static int Vif1TransDirectHL(u32 *data, int size){
	int ret = 0;
	
	if(splitptr > 0){  //Leftover data from the last packet, filling the rest and sending to the GS
		if(splitptr < 4 && size > (4-(int)splitptr)){
		
			while(splitptr < 4){
				splittransfer[splitptr++] = (u32)data++;
				ret++;
				vif1.tag.size--;
			}
		}
		if( CHECK_MULTIGS ) {
			u8* gsmem = GSRingBufCopy((u32*)splittransfer[0], 16, GS_RINGTYPE_P2);
			if( gsmem != NULL ) {
				memcpy_fast(gsmem, (u32*)splittransfer[0], 16);
				GSRINGBUF_DONECOPY(gsmem, 16);
				GSgifTransferDummy(1, (u32*)splittransfer[0], 1);
			}

			if( !CHECK_DUALCORE  ) GS_SETEVENT();
		}
		else {
			FreezeMMXRegs(1);
			FreezeXMMRegs(1);
			GSGIFTRANSFER2((u32*)splittransfer[0], 1);
		}
		splitptr = 0;
		return ret;
	}
	if (size < vif1.tag.size) {
		if(size < 4 && splitptr != 4) {  //Not a full QW left in the buffer, saving left over data
			ret = size;
				while(ret > 0){
					splittransfer[splitptr++] = (u32)data++;
					vif1.tag.size--;
					ret--;
				}
				return size;
			} 

		vif1.tag.size-= size;
		ret = size;
    } else {
        ret = vif1.tag.size;
        vif1.tag.size = 0;
		vif1.cmd &= ~0x7f;
    }

	
	if( CHECK_MULTIGS ) {
		u8* gsmem = GSRingBufCopy(data, ret<<2, GS_RINGTYPE_P2);
		if( gsmem != NULL ) {
			memcpy_fast(gsmem, data, ret<<2);
			GSRINGBUF_DONECOPY(gsmem, ret<<2);
			GSgifTransferDummy(1, data, ret>>2);
		}

		if( !CHECK_DUALCORE  ) GS_SETEVENT();
	}
	else {
		FreezeMMXRegs(1);
		FreezeXMMRegs(1);
		GSGIFTRANSFER2(data, (ret >> 2));
	}
	return ret;
}

static int Vif1TransUnpack(u32 *data, int size){
	if (size < vif1.tag.size) {
			/* size is less that the total size, transfer is 
			   'in pieces' */
			VIFunpack(data, &vif1.tag, size, VIF1dmanum);
		//	g_vifCycles+= size >> 1;
			//vif1.tag.addr += size << 2;
			vif1.tag.size -= size; 
			return size;
		} else {
			int ret;
			/* we got all the data, transfer it fully */
			VIFunpack(data, &vif1.tag, vif1.tag.size, VIF1dmanum);
			//g_vifCycles+= vif1.tag.size >> 1;
			ret = vif1.tag.size;
			vif1.tag.size = 0;
			vif1.cmd &= ~0x7f;
			return ret;
		}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int transferred = 0;
int Path3transfer=0;
static void Vif1CMDNop(){ // NOP
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDSTCycl(){ // STCYCL
	vif1Regs->cycle.cl =  (u8)vif1Regs->code;
    vif1Regs->cycle.wl = (u8)(vif1Regs->code >> 8);
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDOffset(){ // OFFSET
	vif1Regs->ofst  = vif1Regs->code & 0x3ff;
    vif1Regs->stat &= ~0x80;
    vif1Regs->tops  = vif1Regs->base;
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDBase(){ // BASE
	vif1Regs->base = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDITop(){ // ITOP
	vif1Regs->itops = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTMod(){ // STMOD
	vif1Regs->mode = vif1Regs->code & 0x3;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMskPath3(){ // MSKPATH3
	vif1Regs->mskpath3 = (vif1Regs->code >> 15) & 0x1; 
	//SysPrintf("VIF MSKPATH3 %x\n", vif1Regs->mskpath3);
#ifdef GSPATH3FIX
	
    if ( (vif1Regs->code >> 15) & 0x1 ) {
		while((gif->chcr & 0x100)){ //Can be done 2 different ways, depends on the game/company 
			if(path3hack == 0)if(Path3transfer == 0 && gif->qwc == 0) break;
			gsInterrupt();
			if(path3hack == 1)if(gif->qwc == 0) break; //add games not working with it to elfheader.c to enable this instead
		}
		//while(gif->chcr & 0x100) gsInterrupt();		// Finish the transfer first
		psHu32(GIF_STAT) |= 0x2;
    } else {
		if(gif->chcr & 0x100) INT(2, ((transferred>>2)*BIAS));	// Restart Path3 on its own, time it right!
		psHu32(GIF_STAT) &= ~0x2;
    }
#else
	if ( vif1Regs->mskpath3 ) {
		if(gif->qwc) _GIFchain();		// Finish the transfer first
        psHu32(GIF_STAT) |= 0x2;
    } else {
		psHu32(GIF_STAT) &= ~0x2;
		if(gif->qwc) _GIFchain();		// Finish the transfer first
    }
#endif
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMark(){ // MARK
	vif1Regs->mark = (u16)vif1Regs->code;
	vif1Regs->stat |= VIF1_STAT_MRK;
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDFlush(){ // FLUSH/E/A
	
	if((vif1.cmd & 0x7f) == 0x13) {
		//SysPrintf("FlushA\n");
		while((gif->chcr & 0x100)){
			if(Path3transfer == 0 && gif->qwc == 0) break;
			gsInterrupt();	
		}
	}
	vif1FLUSH();
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDMSCALF(){ //MSCAL/F
	vuExecMicro( (u16)(vif1Regs->code) << 3, VIF1dmanum );
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDMSCNT(){ // MSCNT
	vuExecMicro( -1, VIF1dmanum );
	vif1.cmd &= ~0x7f;
}
static void Vif1CMDSTMask(){ // STMASK 
	vif1.tag.size = 1;
}
static void Vif1CMDSTRowCol(){// STROW / STCOL
	vif1.tag.addr = 0;
    vif1.tag.size = 4;
}

static void Vif1CMDMPGTransfer(){ // MPG
	int vifNum;
	vif1FLUSH();
    vifNum = (u8)(vif1Regs->code >> 16);
    if (vifNum == 0) vifNum = 256;
    vif1.tag.addr = (u16)(vif1Regs->code) << 3;
    vif1.tag.size = vifNum * 2;
}
static void Vif1CMDDirectHL(){ // DIRECT/HL
	int vifImm;
	vifImm = (u16)vif1Regs->code;
    if (vifImm == 0) {
		vif1.tag.size = 65536 << 2;
	} else {
		vif1.tag.size = vifImm << 2;
	}
	while((gif->chcr & 0x100) && (vif1.cmd & 0x7f) == 0x51){ 
			gsInterrupt(); //DirectHL flushes the lot
			//if((psHu32(GIF_STAT) & 0xE00) == 0) break;			
		}
}
static void Vif1CMDNull(){ // invalid opcode
	// if ME1, then force the vif to interrupt	
	
     if ((vif1Regs->err & 0x4) == 0) {  //Ignore vifcode and tag mismatch error
			SysPrintf( "UNKNOWN VifCmd: %x\n", vif1.cmd );
            vif1Regs->stat |= 1 << 13;
			vif1.irq++;
     }
	 vif1.cmd &= ~0x7f;
}

//  Vif1 Data Transfer Table

int (*Vif1TransTLB[128])(u32 *data, int size) = 
{
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x7*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0xF*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x17*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x1F*/
	Vif1TransSTMask  , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x27*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x2F*/
	Vif1TransSTRow	 , Vif1TransSTCol	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x37*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x3F*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x47*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransMPG	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x4F*/
	Vif1TransDirectHL, Vif1TransDirectHL, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , /*0x57*/
	Vif1TransNull	 , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x5F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x67*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , /*0x6F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x77*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransNull   , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack   /*0x7F*/
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Table
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void (*Vif1CMDTLB[82])() = 
{
	Vif1CMDNop	   , Vif1CMDSTCycl  , Vif1CMDOffset		, Vif1CMDBase , Vif1CMDITop  , Vif1CMDSTMod , Vif1CMDMskPath3, Vif1CMDMark , /*0x7*/
	Vif1CMDNull	   , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0xF*/
	Vif1CMDFlush   , Vif1CMDFlush   , Vif1CMDNull		, Vif1CMDFlush, Vif1CMDMSCALF, Vif1CMDMSCALF, Vif1CMDNull	, Vif1CMDMSCNT, /*0x17*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x1F*/
	Vif1CMDSTMask  , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x27*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x2F*/
	Vif1CMDSTRowCol, Vif1CMDSTRowCol, Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x37*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x3F*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x47*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDMPGTransfer, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x4F*/
	Vif1CMDDirectHL, Vif1CMDDirectHL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int VIF1transfer(u32 *data, int size, int istag) {
	int ret;
	transferred=vif1.vifstalled ? vif1.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)
	vif1.irqoffset = 0;
#ifdef VIF_LOG 
	VIF_LOG( "VIF1transfer: size %x (vif1.cmd %x)\n", size, vif1.cmd );
#endif

	vif1.vifstalled = 0;
	vif1.stallontag = 0;
	//vif1.irq = 0;
	while (size > 0) { 		

		if (vif1.cmd & 0x7f) {
			//vif1Regs->stat |= VIF1_STAT_VPS_T;
			ret = Vif1TransTLB[(vif1.cmd & 0x7f)](data, size);
			data+= ret; size-= ret;
			transferred+= ret;
			//vif1Regs->stat &= ~VIF1_STAT_VPS_T;
			continue;
		}
		
		vif1Regs->stat &= ~VIF1_STAT_VPS_W;

		if(vif1.tag.size > 0) SysPrintf("VIF1 Tag size %x when cmd == 0!\n", vif1.tag.size);

		
		if(vif1.irq && vif1.tag.size == 0) {
			break;
		}
		
		vif1.cmd = (data[0] >> 24);
		vif1Regs->code = data[0];
		

		//vif1Regs->stat |= VIF1_STAT_VPS_D;
		if ((vif1.cmd & 0x60) == 0x60) {
			vif1UNPACK(data);
		} else {
#ifdef VIF_LOG 
		VIF_LOG( "VIFtransfer: cmd %x, num %x, imm %x, size %x\n", vif1.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, size );
#endif
			//vif1CMD(data, size);
			if((vif1.cmd & 0x7f) > 0x51){
				if ((vif1Regs->err & 0x4) == 0) {  //Ignore vifcode and tag mismatch error
						SysPrintf( "UNKNOWN VifCmd: %x\n", vif1.cmd );
						vif1Regs->stat |= 1 << 13;
						vif1.irq++;
				 }
				vif1.cmd = 0;
			} else 	Vif1CMDTLB[(vif1.cmd & 0x7f)]();
		}
		//vif1Regs->stat &= ~VIF1_STAT_VPS_D;
		if(vif1.tag.size > 0) vif1Regs->stat |= VIF1_STAT_VPS_W;
		++data; 
		--size;
		++transferred;

		if ((vif1.cmd & 0x80) && !(vif1Regs->err & 0x1)) { //i bit on vifcode and not masked by VIF1_ERR
#ifdef VIF_LOG
			VIF_LOG( "Interrupt on VIFcmd: %x (INTC_MASK = %x)\n", vif1.cmd, psHu32(INTC_MASK) );
#endif
			/*if((psHu32(DMAC_CTRL) & 0xC) == 0x8){
				SysPrintf("VIF1 Stall on MFIFO, not implemented!\n");
			}*/
			
			
			++vif1.irq;
			vif1.cmd &= 0x7f;
			if(istag && vif1.tag.size == 0) vif1.stallontag = 1;
		} 
	}
	g_vifCycles+= (transferred>>2)*BIAS; /* guessing */
	// use tag.size because some game doesn't like .cmd
	//if( !vif1.cmd )
	if( !vif1.tag.size )
		vif1Regs->stat &= ~VIF1_STAT_VPS_W;
		
	if (vif1.irq && vif1.tag.size == 0) {
		vif1.vifstalled = 1;
		if(((vif1Regs->code >> 24) & 0x7f) != 0x7)vif1Regs->stat|= VIF1_STAT_VIS;
		else SysPrintf("Stall on Vif1 MARK\n");
		// spiderman doesn't break on qw boundaries
		vif1.irqoffset = transferred%4; // cannot lose the offset
		if(vif1.tag.size > 0) SysPrintf("Oh dear, possible VIF1 stall problem. CMD %x, tag.size %x\n", vif1.cmd, vif1.tag.size);

		if( istag ) {
			return -2;
		}

		

		transferred = transferred >> 2;
		vif1ch->madr+= (transferred << 4);
		vif1ch->qwc-= transferred;
		//SysPrintf("Stall on vif1, FromSPR = %x, Vif1MADR = %x Sif0MADR = %x STADR = %x\n", psHu32(0x1000d010), vif1ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return -2;
	}
	

	if( !istag ) {
		/*if(transferred & 0x3) vif1.irqoffset = transferred%4;
		else vif1.irqoffset = 0;*/
		
		transferred = transferred >> 2;
		vif1ch->madr+= (transferred << 4);
		vif1ch->qwc-= transferred;

		//if(vif1ch->qwc > 0 && size != 0) vif1.vifstalled = 1;

	}
	

	return 0;
}

int  _VIF1chain() {
	u32 *pMem;
	u32 qwc = vif1ch->qwc;
	u32 ret;

	
	if (vif1ch->qwc == 0 && vif1.vifstalled == 0) return 0;
	
	pMem = (u32*)dmaGetAddr(vif1ch->madr);
	if (pMem == NULL)
		return -1;

#ifdef VIF_LOG
	VIF_LOG("dmaChain size=%d, madr=%lx, tadr=%lx\n",
			vif1ch->qwc, vif1ch->madr, vif1ch->tadr);
#endif
	if( vif1.vifstalled ) {
		ret = VIF1transfer(pMem+vif1.irqoffset, vif1ch->qwc*4-vif1.irqoffset, 0);
	}
	else {
		ret = VIF1transfer(pMem, vif1ch->qwc*4, 0);
	}
	/*vif1ch->madr+= (vif1ch->qwc << 4);
	vif1ch->qwc-= qwc;*/
	
	return ret;
}

static int prevvifcycles = 0;
static u32* prevviftag = NULL;
u32 *vifptag;
int _chainVIF1() {
	int id;
	//int done=0;
	int ret;
	//g_vifCycles = prevvifcycles;
	
	vifptag = (u32*)dmaGetAddr(vif1ch->tadr); //Set memory pointer to TADR
	if (vifptag == NULL) {						//Is ptag empty?
		SysPrintf("Vif1 Tag BUSERR\n");
		vif1ch->chcr = ( vif1ch->chcr & 0xFFFF ) | ( (*vifptag) & 0xFFFF0000 ); //Transfer upper part of tag to CHCR bits 31-15
		psHu32(DMAC_STAT)|= 1<<15;          //If yes, set BEIS (BUSERR) in DMAC_STAT register
		return -1;						   //Return -1 as an error has occurred	
	}
	
	id        = (vifptag[0] >> 28) & 0x7; //ID for DmaChain copied from bit 28 of the tag
	vif1ch->qwc  = (u16)vifptag[0];       //QWC set to lower 16bits of the tag
	vif1ch->madr = vifptag[1];            //MADR = ADDR field
	g_vifCycles+=1; // Add 1 g_vifCycles from the QW read for the tag

	vif1ch->chcr = ( vif1ch->chcr & 0xFFFF ) | ( (*vifptag) & 0xFFFF0000 ); //Transfer upper part of tag to CHCR bits 31-15
	// Transfer dma tag if tte is set
	

	
	
	
#ifdef VIF_LOG
	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			vifptag[1], vifptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr);
#endif
	

	//} else
	
	
	if (!vif1.done && (psHu32(DMAC_CTRL) & 0xC0) == 0x40 && id == 4) { // STD == VIF1
				//vif1.done |= hwDmacSrcChainWithStack(vif1ch, id);
				// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
				if( (vif1ch->madr + vif1ch->qwc * 16) >= psHu32(DMAC_STADR) ) {
					// stalled

					//SysPrintf("Vif1 Stalling %x, %x, DMA_CTRL = %x\n",vif1ch->madr, psHu32(DMAC_STADR), psHu32(DMAC_CTRL));
					/*prevvifcycles = g_vifCycles;
					prevviftag = vifptag;*/
					hwDmacIrq(13);
					//vif1ch->tadr -= 16;
					return 0;
				}
			}
	//prevvifcycles = 0;

	if (vif1ch->chcr & 0x40) {
		if(vif1.vifstalled == 1) ret = VIF1transfer(vifptag+(2+vif1.irqoffset), 2-vif1.irqoffset, 1);  //Transfer Tag on stall
		else ret = VIF1transfer(vifptag+2, 2, 1);  //Transfer Tag
		if (ret == -1) return -1;       //There has been an error
		if (ret == -2) {
			if(vif1.tag.size > 0)SysPrintf("VIF1 Stall on tag %x code %x\n", vif1.irqoffset, vif1Regs->code);
			return 0;        //IRQ set by VIFTransfer
		}
	}
	//if((psHu32(DMAC_CTRL) & 0xC0) != 0x40 || id != 4)
	vif1.done |= hwDmacSrcChainWithStack(vif1ch, id);

#ifdef VIF_LOG
		VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
				vifptag[1], vifptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr);
#endif

	//done |= hwDmacSrcChainWithStack(vif1ch, id);
	ret = _VIF1chain();											   //Transfers the data set by the switch
	//if (ret == -1) { return -1; }									   //There's been an error
	//if (ret == -2) { 							   //IRQ has been set by VifTransfer
	//	return 0;
	//}

	
	if ((vif1ch->chcr & 0x80) && (vifptag[0] >> 31)) {			       //Check TIE bit of CHCR and IRQ bit of tag
#ifdef VIF_LOG
		VIF_LOG( "dmaIrq Set\n" );
#endif
		//SysPrintf("VIF1 TIE\n");
		//SysPrintf( "VIF1dmaIrq Set\n" );
		//vif1ch->qwc = 0;
		//vif1Regs->stat|= VIF1_STAT_VIS;							   //Set the Tag Interrupt flag of VIF1_STAT
		vif1.done = 1;
		return 0;												   //End Transfer
	}
	return vif1.done;												   //Return Done
}

void vif1Interrupt() {

#ifdef VIF_LOG 
	VIF_LOG("vif1Interrupt: %8.8x\n", cpuRegs.cycle);
#endif

	g_vifCycles = 0;
    
	//if(vif1.vifstalled == 1) {
		if(vif1.irq && vif1.tag.size == 0) {
			vif1Regs->stat|= VIF1_STAT_INT;
			hwIntcIrq(VIF1intc);
			--vif1.irq;
			if(vif1Regs->stat & (VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS))
				{
					vif1Regs->stat&= ~0x1F000000; // FQC=0
					// One game doesnt like vif stalling at end, cant remember what. Spiderman isnt keen on it tho
					vif1ch->chcr &= ~0x100;
					cpuRegs.interrupt &= ~(1 << 1);
					return;
				} 
			//return 0;
			if(vif1ch->qwc > 0){
				_VIF1chain();
				INT(1, g_vifCycles);
				return;
			}
		}
		
		
	//}
	if((vif1ch->chcr & 0x100) == 0) {
		SysPrintf("Vif1 running when CHCR == %x\n", vif1ch->chcr);
		/*prevviftag = NULL;
	prevvifcycles = 0;
	vif1ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF1);
	hwIntcIrq(VIF1intc);
	vif1Regs->stat&= ~0x1F000000; // FQC=0
		return 1;*/
	}
	/*if(vif1ch->qwc > 0){
		_VIF1chain();
		INT(1, g_vifCycles);
		return 0;
	}*/
	if ((vif1ch->chcr & 0x104) == 0x104 && vif1.done == 0 && vif1.vifstalled == 0) {

		if( !(psHu32(DMAC_CTRL) & 0x1) ) {
			SysPrintf("vif1 dma masked\n");
			return;
		}

		_chainVIF1();
		INT(1, g_vifCycles);
		FreezeMMXRegs(0);
		FreezeXMMRegs(0);
		return;
	}
	if(vif1ch->qwc > 0) SysPrintf("VIF1 Ending with QWC left\n");
	//SysPrintf("VIF Interrupt\n");
	//if((gif->chcr & 0x100) && vif1Regs->mskpath3) gsInterrupt();
	prevviftag = NULL;
	prevvifcycles = 0;
	vif1ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF1);
	if(vif1Regs->mskpath3 == 0 || (vif1ch->chcr & 0x1) == 0x1)vif1Regs->stat&= ~0x1F000000; // FQC=0

	cpuRegs.interrupt &= ~(1 << 1);
}

extern void gsWaitGS();
extern u32 g_MTGSVifStart, g_MTGSVifCount;
#define spr0 ((DMACh*)&PS2MEM_HW[0xD000])
void dmaVIF1()
{

#ifdef VIF_LOG
	VIF_LOG("dmaVIF1 chcr = %lx, madr = %lx, qwc  = %lx\n"
			"        tadr = %lx, asr0 = %lx, asr1 = %lx\n",
			vif1ch->chcr, vif1ch->madr, vif1ch->qwc,
			vif1ch->tadr, vif1ch->asr0, vif1ch->asr1 );
#endif

	/*if ((psHu32(DMAC_CTRL) & 0xC0)) { 
			SysPrintf("DMA Stall Control %x\n",(psHu32(DMAC_CTRL) & 0xC0));
			}*/
	/* Check if there is a pending irq */
	/*if (vif1.irq > 0) {
		vif1.irq--;
		hwIntcIrq(VIF1intc);
		return;
	}*/
//	if(vif1ch->qwc > 0) {
//		 _VIF1chain();
//		 INT(1, g_vifCycles);
//	}
	vif1.done = 0;
	g_vifCycles = 0;

	/*if( prevvifcycles != 0 ) {
		int stallret = 0;
		assert( prevviftag != NULL );

		vifptag = prevviftag;
		// transfer interrupted, so continue
		//_VIF1chain();
		_chainVIF1();

		if (vif1ch->chcr & 0x80 && vifptag[0] >> 31) {			 //Check TIE bit of CHCR and IRQ bit of tag
#ifdef VIF_LOG
			VIF_LOG("dmaIrq Set\n");
#endif					
			vif1.done = 1;
			INT(1, g_vifCycles);
			return;
		}
		//vif1.done = 1;
		INT(1, g_vifCycles);
		return;
	}*/

	if (((psHu32(DMAC_CTRL) & 0xC) == 0x8)) { // VIF MFIFO
		//SysPrintf("VIFMFIFO\n");
		if(vif1ch->madr != spr0->madr)vifMFIFOInterrupt();
		return;
	}

#ifdef PCSX2_DEVBUILD
	if ((psHu32(DMAC_CTRL) & 0xC0) == 0x40) { // STD == VIF1
		//SysPrintf("VIF Stall Control Source = %x, Drain = %x\n", (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3);
		//return;
	}
#endif

	
	vif1Regs->stat|= 0x10000000; // FQC=16

	if (!(vif1ch->chcr & 0x4) || vif1ch->qwc > 0) { // Normal Mode 
		if ((psHu32(DMAC_CTRL) & 0xC0) == 0x40) { 
			SysPrintf("DMA Stall Control on VIF1 normal\n");
		}
		if ((vif1ch->chcr & 0x1)) { // to Memory
			if(_VIF1chain() == -2) {
				SysPrintf("Stall on normal\n");
				vif1.vifstalled = 1;
				return;
			}
            INT(1, g_vifCycles);
		} else { // from Memory

			if( CHECK_MULTIGS ) {
				u8* pTempMem, *pEndMem;

				u8* pMem = GSRingBufCopy(NULL, 0, GS_RINGTYPE_VIFFIFO);
				*(u32*)(pMem-16) = GS_RINGTYPE_VIFFIFO|(vif1ch->qwc<<16); // hack
				*(u32*)(pMem-12) = vif1ch->madr;
				*(u32*)(pMem-8) = cpuRegs.cycle;

				// touch all the pages to make sure they are in memory
				pTempMem = dmaGetAddr(vif1ch->madr);
				pEndMem = (u8*)((uptr)(pTempMem + vif1ch->qwc*16 + 0xfff)&~0xfff);
				pTempMem = (u8*)((uptr)pTempMem&~0xfff);
				while(pTempMem < pEndMem ) {
					pTempMem[0]++;
                    pTempMem[0]--;
					pTempMem += 0x1000;
				}

				GSRINGBUF_DONECOPY(pMem, 0);
				
				if( !CHECK_DUALCORE  )
					GS_SETEVENT();

                g_MTGSVifStart = cpuRegs.cycle;
                g_MTGSVifCount = 4000000; // a little less than 1/60th of a second
				// wait is, the safest option
                //gsWaitGS();
                //SysPrintf("waiting for gs\n");
			}
			else {

				int size;
				u64* pMem = (u64*)dmaGetAddr(vif1ch->madr);
				if (pMem == NULL) {						//Is ptag empty?
					SysPrintf("Vif1 Tag BUSERR\n");
					psHu32(DMAC_STAT)|= 1<<15;          //If yes, set BEIS (BUSERR) in DMAC_STAT register
					vif1.done = 1;
					vif1Regs->stat&= ~0x1f000000;
					vif1ch->qwc = 0;
					INT(1, g_vifCycles);
					return;						   //Return -1 as an error has occurred	
				}

				if( GSreadFIFO2 == NULL ) {
					for (size=vif1ch->qwc; size>0; --size) {
						if (size > 1) GSreadFIFO((u64*)&PS2MEM_HW[0x5000]);
						pMem[0] = psHu64(0x5000);
						pMem[1] = psHu64(0x5008); pMem+= 2;
					}
				}
				else {
					GSreadFIFO2(pMem, vif1ch->qwc);

					// set incase read
					psHu64(0x5000) = pMem[2*vif1ch->qwc-2];
					psHu64(0x5008) = pMem[2*vif1ch->qwc-1];
				}

				
				if(vif1Regs->mskpath3 == 0)vif1Regs->stat&= ~0x1f000000;
				g_vifCycles += vif1ch->qwc * 2;
                vif1ch->madr += vif1ch->qwc * 16; // mgs3 scene changes
				vif1ch->qwc = 0;
				INT(1, g_vifCycles);
			}
		}
		FreezeMMXRegs(0);
		FreezeXMMRegs(0);
		vif1.done = 1;
		return;
	}

/*	if (_VIF1chain() != 0) {
		INT(1, g_vifCycles);
		return;
	}*/

	// Chain Mode
	vif1.done = 0;
	INT(1, g_vifCycles);
}


void vif1Write32(u32 mem, u32 value) {
	if (mem == 0x10003c30) { // MARK
#ifdef VIF_LOG
		VIF_LOG("VIF1_MARK write32 0x%8.8x\n", value);
#endif
		/* Clear mark flag in VIF1_STAT and set mark with 'value' */
		vif1Regs->stat&= ~VIF1_STAT_MRK;
		vif1Regs->mark = value;
	} else
	if (mem == 0x10003c10) { // FBRST
#ifdef VIF_LOG
		VIF_LOG("VIF1_FBRST write32 0x%8.8x\n", value);
#endif
		if (value & 0x1) {
			/* Reset VIF */
			//SysPrintf("Vif1 Reset %x\n", vif1Regs->stat);
			memset(&vif1, 0, sizeof(vif1));
			vif1ch->qwc = 0; //?
			psHu64(0x10005000) = 0;
			psHu64(0x10005008) = 0;
			vif1.done = 1;
			vif1Regs->err = 0;
			vif1Regs->stat&= ~(0x1F800000|VIF1_STAT_INT|VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS); // FQC=0
		}
		if (value & 0x2) {
			/* Force Break the VIF */
			/* I guess we should stop the VIF dma here 
			   but not 100% sure (linuz) */
			vif1Regs->stat |= VIF1_STAT_VFS;
			cpuRegs.interrupt &= ~0x402;
			vif1.vifstalled = 1;
			SysPrintf("vif1 force break\n");
		}
		if (value & 0x4) {
			/* Stop VIF */
			/* Not completly sure about this, can't remember what game 
			   used this, but 'draining' the VIF helped it, instead of 
			   just stoppin the VIF (linuz) */
			vif1Regs->stat |= VIF1_STAT_VSS;
			vif1.vifstalled = 1;
			//SysPrintf("Vif1 Stop\n");
			//dmaVIF1();	// Drain the VIF  --- VIF Stops as not to outstrip dma source (refraction)
			//FreezeXMMRegs(0);
		}
		if (value & 0x8) {
			int cancel = 0;

			/* Cancel stall, first check if there is a stall to cancel, 
			   and then clear VIF1_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
			if (vif1Regs->stat & (VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS)) {
				cancel = 1;
			}

			vif1Regs->stat &= ~(VIF1_STAT_VSS | VIF1_STAT_VFS | VIF1_STAT_VIS |
								VIF1_STAT_INT | VIF1_STAT_ER0 | VIF1_STAT_ER1);
			if (cancel) {
				//SysPrintf("VIF1 Stall Resume\n");
				if( vif1.vifstalled ) {
					// loop necessary for spiderman
					if((psHu32(DMAC_CTRL) & 0xC) == 0x8){
						//vif1.vifstalled = 0;
						//SysPrintf("MFIFO Stall\n");
						INT(10, 0);
					}else {
						if(vif1.stallontag == 1){
							//SysPrintf("Sorting VIF Stall on tag\n");
							_chainVIF1();
						} else _VIF1chain();
							//vif1.vifstalled = 0'
						INT(1, g_vifCycles); // Gets the timing right - Flatout
					}
					vif1ch->chcr |= 0x100;
					FreezeXMMRegs(0);
					FreezeMMXRegs(0);
				}
			}
		}			
	} else
	if (mem == 0x10003c20) { // ERR
#ifdef VIF_LOG
		VIF_LOG("VIF1_ERR write32 0x%8.8x\n", value);
#endif
		/* Set VIF1_ERR with 'value' */
		vif1Regs->err = value;
	} else
	if (mem == 0x10003c00) { // STAT
#ifdef VIF_LOG
		VIF_LOG("VIF1_STAT write32 0x%8.8x\n", value);
#endif

#ifdef PCSX2_DEVBUILD
		/* Only FDR bit is writable, so mask the rest */
		if( (vif1Regs->stat & VIF1_STAT_FDR) ^ (value & VIF1_STAT_FDR) ) {
			// different so can't be stalled
			if (vif1Regs->stat & (VIF1_STAT_INT|VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS)) {
				SysPrintf("changing dir when vif1 fifo stalled\n");
			}
		}
#endif

		vif1Regs->stat = (vif1Regs->stat & ~VIF1_STAT_FDR) | (value & VIF1_STAT_FDR);
		if (vif1Regs->stat & VIF1_STAT_FDR) {
			vif1Regs->stat|= 0x01000000;
		} else {
			vif1ch->qwc = 0;
			vif1.vifstalled = 0;
			vif1.done = 1;
			vif1Regs->stat&= ~0x1F000000; // FQC=0
		}
	}
	 else
	if (mem == 0x10003c50) { // MODE
		vif1Regs->mode = value;
	}
	else {
		SysPrintf("Unknown Vif1 write to %x\n", mem);
		if( mem >= 0x10003d00 && mem < 0x10003d80 ) {
			assert( (mem&0xf) == 0 );
			if( mem < 0x10003d40) g_vifRow1[(mem>>4)&3] = value;
			else g_vifCol1[(mem>>4)&3] = value;
		} else psHu32(mem) = value;
	}

	/* Other registers are read-only so do nothing for them */
}

void vif1Reset() {
	/* Reset the whole VIF, meaning the internal pcsx2 vars
	   and all the registers */
	memset(&vif1, 0, sizeof(vif1));
	memset(vif1Regs, 0, sizeof(vif1Regs));
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
	psHu64(0x10005000) = 0;
	psHu64(0x10005008) = 0;
	vif1.done = 1;
	vif1Regs->stat&= ~0x1F000000; // FQC=0
	FreezeXMMRegs(0);
	FreezeMMXRegs(0);
}

int vif1Freeze(gzFile f, int Mode) {
	gzfreeze(&vif1, sizeof(vif1));
	if (Mode == 0)
		SetNewMask(g_vif1Masks, g_vif1HasMask3, vif1Regs->mask, ~vif1Regs->mask);

	return 0;
}
