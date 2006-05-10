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

//well that might need a recheck :P
//(shadow bug0r work :P)

//plz, change this file according to FIFO defs in hw.h
/*	u64 VIF0[0x1000];
	u64 VIF1[0x1000];
	u64 GIF[0x1000];
	u64 IPU_out_FIFO[0x160];	// 128bit
	u64 IPU_in_FIFO[0x160];	// 128bit
*/

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "Common.h"
#include "Hw.h"

#include <assert.h>

//////////////////////////////////////////////////////////////////////////
/////////////////////////// Quick & dirty FIFO :D ////////////////////////
//////////////////////////////////////////////////////////////////////////
void fifo_add2(struct fifo *fifo, void *value, u32 size)
{
	if(value == NULL || !size) return;

	
}

void fifo_add(struct fifo *fifo, void *value, u32 size){
	struct item *p;

	if (value==NULL || size==0)	return;

	p=(struct item *)malloc(sizeof(struct item));
	p->next=NULL;
	p->size=size;
	p->data=malloc(size);
	memcpy(p->data, value, size);
	fifo->last->next=p;
	fifo->last=p;
	fifo->count+=size;
}

void fifo_get3(struct fifo *fifo, void *value, u32 size, int type){
	struct item * p;
	register u32 q;
	if ( value == NULL ) return;
	
	if(fifo->first->next == NULL) memset(value, 0, size);
	else
	{
		p = fifo->first->next;
		for(; p && size; p = fifo->first->next)
		{
			q = min(p->size - fifo->used, size);
			memcpy(value,(u8*)p->data + fifo->used, q);
			size -= q;
			(u8*)value += q;
			if(type)
			{
				fifo->count -= q;
				fifo->used += q;
				if(fifo->used >= p->size)
				{
					fifo->used = 0;
					fifo->first->next = p->next;
					free(p->data);
					free(p);
				}
			}else
				p = fifo->first->next;
		}
		memset(value,0, size);
	}
}

void fifo_get(struct fifo *fifo, void *value, u32 size){
	struct item *p;
	register u32 q;
	
	if (value==NULL)	return;

	if (EMPTY(fifo))
		memset(value, 0, size);
	else{
		p=fifo->first->next;
		while (size && p){
			q = min(p->size-fifo->used, size);
			memcpy(value, (u8*)p->data + fifo->used, q);
			fifo->used+=q;
			fifo->count-=q;
			size-=q;
			(u8*)value+=q;
			if (fifo->used>=p->size){
				fifo->used=0;
				fifo->first->next=p->next;
				free(p->data);
				free(p);
				p=fifo->first->next;
			}
		}
		memset(value, 0, size);
		if (fifo->first->next==NULL)	fifo->last=fifo->first;
	}
}

void fifo_get2(struct fifo *fifo, void *value, u32 size){
	struct item *p;
	register u32 q;

	if (value==NULL)	return;

	if (EMPTY(fifo))
		memset(value, 0, size);
	else{
		p=fifo->first->next;
		while (size && p){
			q=min(p->size-fifo->used, size);
			memcpy(value, (u8*)p->data+fifo->used, q);
			size-=q;
			(u8*)value+=q;
			p=fifo->first->next;
		}
		memset(value, 0, size);
		if (fifo->first->next==NULL)	fifo->last=fifo->first;
	}
}

u32  fifo_count(struct fifo *fifo){
	 return fifo->count;
}

void fifo_empty(struct fifo *fifo){
	register struct item *p;
	while (fifo->first->next){
		p=fifo->first->next;
		fifo->first->next=p->next;
		free(p->data);
		free(p);
	}
	fifo->last=fifo->first;
	fifo->used=fifo->count=0;
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////// Quick & dirty FIFO :D ////////////////////////
//////////////////////////////////////////////////////////////////////////
extern pthread_mutex_t g_mtxIPU;
extern pthread_cond_t g_condIPU0, g_condIPU1;
extern int g_nOutputIPUFIFO;
extern const void* g_pOutputIPUFIFO;

extern int fifo_wwrite(u32* pMem, int size);
extern int fifo_wread(void *value, int size);

void ReadFIFO(u32 mem, u64 *out) {
	if ((mem >= 0x10004000) && (mem < 0x10005000)) {
#ifdef VIF_LOG
		VIF_LOG("ReadFIFO VIF0 0x%08X\n", mem);
#endif
		out[0] = psHu64(mem  );
		out[1] = psHu64(mem+8);
		return;
	} else
	if ((mem >= 0x10005000) && (mem < 0x10006000)) {

#ifdef PCSX_DEVBUILD
		VIF_LOG("ReadFIFO VIF1 0x%08X\n", mem);

		if( vif1Regs->stat & (VIF1_STAT_INT|VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS) ) {
			SysPrintf("reading from vif1 fifo when stalled\n");
		}
#endif

		if (vif1Regs->stat & 0x800000) {
			if (--psHu32(D1_QWC) == 0) {
				vif1Regs->stat&= ~0x1f000000;
			} else {
			}
		}
		out[0] = psHu64(mem  );
		out[1] = psHu64(mem+8);
		return;
	} else if( (mem&0xfffff010) == 0x10007000) {
	//} else if (mem == 0x10007000) {
		pthread_mutex_lock(&g_mtxIPU);

		if( g_nOutputIPUFIFO > 0 ) {
			((u64*)out)[0] = ((u64*)g_pOutputIPUFIFO)[0];
			((u64*)out)[1] = ((u64*)g_pOutputIPUFIFO)[1];
			g_pOutputIPUFIFO = (u64*)g_pOutputIPUFIFO + 2;
			if( g_nOutputIPUFIFO-- == 1 ) {
				// signal thread to complete
				pthread_cond_signal(&g_condIPU0);
			}
		}

		pthread_mutex_unlock(&g_mtxIPU);
		return;
	}else if ( (mem&0xfffff010) == 0x10007010) {
	//}else if (mem == 0x10007010) {
		fifo_wread((void*)out, 1);
		return;
	}
	SysPrintf("ReadFIFO Unknown %x\n", mem);

	//or here..
}

extern HANDLE g_hGsEvent;

void WriteFIFO(u32 mem, u64 *value) {
	int ret;

	if ((mem >= 0x10004000) && (mem < 0x10005000)) {
#ifdef VIF_LOG
		VIF_LOG("WriteFIFO VIF0 0x%08X\n", mem);
#endif
		psHu64(mem  ) = value[0];
		psHu64(mem+8) = value[1];
		ret = VIF0transfer((u32*)value, 4, 0);
		assert(ret == 0 ); // vif stall code not implemented
	} else
	if ((mem >= 0x10005000) && (mem < 0x10006000)) {
#ifdef VIF_LOG
		VIF_LOG("WriteFIFO VIF1 0x%08X\n", mem);
#endif
		psHu64(mem  ) = value[0];
		psHu64(mem+8) = value[1];

#ifdef PCSX2_DEVBUILD
		if(vif1Regs->stat & VIF1_STAT_FDR)
			SysPrintf("writing to fifo when fdr is set!\n");
		if( vif1Regs->stat & (VIF1_STAT_INT|VIF1_STAT_VSS|VIF1_STAT_VIS|VIF1_STAT_VFS) ) {
			SysPrintf("writing to vif1 fifo when stalled\n");
		}
#endif

		ret = VIF1transfer((u32*)value, 4, 0);
		assert(ret == 0 ); // vif stall code not implemented
		/*if ((vif1Regs->stat & 0x1f000000) < 0x10000000) {
			vif1Regs->stat+= 0x01000000;
		}*/
	} else
	if ((mem >= 0x10006000) && (mem < 0x10007000)) {
		u64* data;
#ifdef GIF_LOG
		GIF_LOG("WriteFIFO GIF 0x%08X\n", mem);
#endif

		psHu64(mem  ) = value[0];
		psHu64(mem+8) = value[1];

		if( CHECK_MULTIGS ) {
			data = (u64*)GSRingBufCopy(NULL, 16, GS_RINGTYPE_P3);

			data[0] = value[0];
			data[1] = value[1];
			GSgifTransferDummy(2, (u32*)data, 1);
			GSRINGBUF_DONECOPY(data, 16);

			if( !CHECK_DUALCORE )
				SetEvent(g_hGsEvent);
		}
		else {
#ifdef GSCAPTURE
			extern u32 g_loggs, g_gstransnum, g_gsfinalnum;

			if( !g_loggs || (g_loggs && g_gstransnum++ < g_gsfinalnum)) {
				GSgifTransfer3((u32*)value, 1-GSgifTransferDummy(2, (u32*)value, 1));
			}
#else
			GSgifTransfer3((u32*)value, 1);
#endif
		}

	} else
	//if (mem == 0x10007010) {
	if ((mem&0xfffff010) == 0x10007010) {
		int ret;
#ifdef IPU_LOG
		IPU_LOG("WriteFIFO IPU_in[%d] <- %8.8X_%8.8X_%8.8X_%8.8X\n", (mem - 0x10007010)/8, ((u32*)value)[3], ((u32*)value)[2], ((u32*)value)[1], ((u32*)value)[0]);
#endif
		ret = fifo_wwrite((void*)value, 1);//commiting every 16 bytes
		assert( ret == 1 );
		pthread_cond_signal(&g_condIPU1);
	} else {
		//we should never gone here..
		SysPrintf("WriteFIFO Unknown %x\n", mem);
	}
}
