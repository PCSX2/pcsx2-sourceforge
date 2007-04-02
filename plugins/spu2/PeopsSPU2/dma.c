/***************************************************************************
                            dma.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2004/04/04 - Pete
// - changed plugin to emulate PS2 spu
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"
#include "externals.h"
#include "registers.h"
#include "debug.h"
extern void (CALLBACK *irqCallbackDMA4)();                  // func of main emu, called on spu irq
extern void (CALLBACK *irqCallbackDMA7)();                  // func of main emu, called on spu irq
extern void (CALLBACK *irqCallbackSPU2)();                  // func of main emu, called on spu irq
unsigned short interrupt;
extern int SPUCycles;
int SPUStartCycle[2];
int SPUTargetCycle[2];

unsigned long MemAddr[2];

ADMA Adma4;
ADMA Adma7;

////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

EXPORT_GCC void CALLBACK SPU2readDMA4Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;
#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("READDMA4 %X - %X\r\n",spuAddr2[0],iSize);
   
   if(spuAddr2[0]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }

#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[0]];                  // spu addr 0 got by writeregister
   if(spuCtrl2[0]&0x40 && spuIrq2[0] == spuAddr2[0]){
	   regArea[0x7C0] |= 0x4;
	   irqCallbackSPU2();
   }
   spuAddr2[0]++;                                     // inc spu addr
   if(spuAddr2[0]>0x1fffff) spuAddr2[0]=0;             // wrap
  }

 spuAddr2[0]+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)
 

 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[0]&=~0x80;                                     // DMA complete
 //if(regArea[(PS2_C0_ADMAS)>>1] != 1) {
	// if((regArea[(PS2_C0_ATTR)>>1] & 0x30)) {
	  SPUStartCycle[0] = SPUCycles;
	SPUTargetCycle[0] = iSize;
	interrupt |= (1<<1);
	// }
//}
 //regArea[(PS2_C0_ADMAS)>>1] = 0;
}

EXPORT_GCC void CALLBACK SPU2readDMA7Mem(unsigned short * pusPSXMem,int iSize)
{
 int i;
#ifdef _WINDOWS
 if(iDebugMode==1) 
  { 
   logprintf("READDMA7 %X - %X\r\n",spuAddr2[1],iSize);
   
   if(spuAddr2[1]<=0x1fff)
    logprintf("# OUTPUT AREA ACCESS #############\r\n");
  }
#endif

 for(i=0;i<iSize;i++)
  {
   *pusPSXMem++=spuMem[spuAddr2[1]];                   // spu addr 1 got by writeregister
   if(spuCtrl2[1]&0x40 && spuIrq2[1] == spuAddr2[1]){
	   regArea[0x7C0] |= 0x8;
	   irqCallbackSPU2();
   }
   spuAddr2[1]++;                                      // inc spu addr
   if(spuAddr2[1]>0x1fffff) spuAddr2[1]=0;              // wrap
  }

spuAddr2[1]+=19; //Transfer Local To Host TSAH/L + Data Size + 20 (already +1'd)

 iSpuAsyncWait=0;

 // got from J.F. and Kanodin... is it needed?
 spuStat2[1]&=~0x80;                                     // DMA complete
 // if(regArea[(PS2_C1_ADMAS)>>1] != 2) {
	// if((regArea[(PS2_C1_ATTR)>>1] & 0x30)) {
	 SPUStartCycle[1] = SPUCycles;
	SPUTargetCycle[1] = iSize;
	interrupt |= (1<<2);
	// }
 //}
  //regArea[(PS2_C1_ADMAS)>>1] = 0;
}

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////


// AutoDMA's are used to transfer to the DIRECT INPUT area of the spu2 memory
// Left and Right channels are always interleaved together in the transfer so 
// the AutoDMA's deinterleaves them and transfers them. An interrupt is
// generated when half of the buffer (256 short-words for left and 256 
// short-words for right ) has been transferred. Another interrupt occurs at 
// the end of the transfer.
int ADMAS4Write()
{
 if(interrupt & 0x2) return 0;
 if(Adma4.AmountLeft <= 0) return 1;

#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("ADMAWRITE4 %X - %X\r\n",spuAddr2[0],Adma4.AmountLeft);
   if(Adma4.AmountLeft<512) logprintf("FUCK YOU %X\r\n",Adma4.AmountLeft);
  }   
#endif

 // SPU2 Deinterleaves the Left and Right Channels
 memcpy((short*)(spuMem + spuAddr2[0] + 0x2000),(short*)Adma4.MemAddr,512);
 Adma4.MemAddr += 256;
 memcpy((short*)(spuMem + spuAddr2[0] + 0x2200),(short*)Adma4.MemAddr,512);
 Adma4.MemAddr += 256;
 spuAddr2[0] = (spuAddr2[0] + 256) & 511;
 
 //MemAddr[0] += 1024;
 Adma4.AmountLeft-=512;
 if(Adma4.AmountLeft == 0) 
 {
	SPUStartCycle[0] = SPUCycles;
	SPUTargetCycle[0] = 1;//512*48000;
	spuStat2[0]&=~0x80;
	interrupt |= (1<<1);
 }
 return 0;
}


int ADMAS7Write()
{
if(interrupt & 0x4) return 0;
 if(Adma7.AmountLeft <= 0) return 1;
 
#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("ADMAWRITE7 %X - %X  %X\r\n",Adma7.IntPointer,Adma7.AmountLeft,spuAddr2[1]);
  }   
#endif

 // SPU2 Deinterleaves the Left and Right Channels
  memcpy((unsigned char*)(spuMem + spuAddr2[1] + 0x2400),(unsigned char*)Adma7.MemAddr,512);
  Adma7.MemAddr += 256;
  memcpy((unsigned char*)(spuMem + spuAddr2[1] + 0x2600),(unsigned char*)Adma7.MemAddr,512);
  Adma7.MemAddr += 256;
  spuAddr2[1] = (spuAddr2[1] + 256) & 511;

 //MemAddr[1] += 1024;

 Adma7.AmountLeft -= 512;
 if(Adma7.AmountLeft <= 0)
 {
	SPUStartCycle[1] = SPUCycles;
	SPUTargetCycle[1] = 1;//512*48000; 
	spuStat2[1]&=~0x80;
	interrupt |= (1<<2);
 }
 return 0;
}

#include <stdio.h>
extern FILE * LogFile;
EXPORT_GCC void CALLBACK SPU2writeDMA4Mem(short * pMem,unsigned int iSize)
{
 //if(Adma4.AmountLeft > 0) return;
 if(regArea[PS2_C0_ADMAS] & 0x1 && (spuCtrl2[0] & 0x30) == 0 && iSize)
 {
	 //fwrite(pMem,iSize<<1,1,LogFile);
	 memset(&Adma4,0,sizeof(ADMA));
	 spuAddr2[0] = 0;
	 Adma4.MemAddr = pMem;
	 Adma4.AmountLeft = iSize;
	 ADMAS4Write();
	 return;
 }

#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("WRITEDMA4 %X - %X\r\n",spuAddr2[0],iSize);
  } 
#endif

 memcpy((unsigned char*)(spuMem+spuAddr2[0]),(unsigned char*)pMem,iSize<<1);
 spuAddr2[0] += iSize;
 if(spuCtrl2[0]&0x40 && spuIrq2[0] == spuAddr2[0]){
	   regArea[0x7C0] |= 0x4;
	   irqCallbackSPU2();
 }
 if(spuAddr2[0]>0xFFFFE) spuAddr2[0] = 0x2800;
  
 MemAddr[0] += iSize<<1;
 spuStat2[0]&=~0x80;
 SPUStartCycle[0] = SPUCycles;
 SPUTargetCycle[0] = 1;//iSize;
 interrupt |= (1<<1);
}

EXPORT_GCC void CALLBACK SPU2writeDMA7Mem(unsigned short * pMem,int iSize)
{
 // For AutoDMA, the ATTR register's bit 5 and 6 are cleared.
 // bit 5 means Data Input Thru Register
 // bit 6 means Data Input Thru DMA
 //if(Adma7.AmountLeft > 0) return;

 if((regArea[PS2_C1_ADMAS] & 0x2) && (spuCtrl2[1] & 0x30) == 0 && iSize)
 {
	// fwrite((void*)pMem,iSize<<1,1,LogFile);
	 memset(&Adma7,0,sizeof(ADMA));
	 spuAddr2[1] = 0;
	 Adma7.MemAddr = pMem;
	 Adma7.AmountLeft = iSize;
	 ADMAS7Write();
	 return;
 }
#ifdef _WINDOWS
 if(iDebugMode==1) 
  {
   logprintf("WRITEDMA7 %X - %X\r\n",spuAddr2[1],iSize);
  } 
#endif

 memcpy((short*)(spuMem+spuAddr2[1]),(short*)pMem,iSize<<1);
 spuAddr2[1] += iSize;
 if(spuCtrl2[1]&0x40 && spuIrq2[1] == spuAddr2[1]){
   regArea[0x7C0] |= 0x8;
   irqCallbackSPU2();
 }
 if(spuAddr2[1]>0xFFFFE) spuAddr2[1] = 0x2800;
 
 MemAddr[1] += iSize<<1;
 spuStat2[1]&=~0x80;
 SPUStartCycle[1] = SPUCycles;
 SPUTargetCycle[1] = 1;//iSize;
 interrupt |= (1<<2); 
}


////////////////////////////////////////////////////////////////////////
// INTERRUPTS
////////////////////////////////////////////////////////////////////////

void InterruptDMA4(void) 
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE0_ATTR)&= ~0x30;
//	spu2Rs16(REG__1B0) = 0;
//	spu2Rs16(SPU2_STATX_WRDY_M)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA4\r\n");
#endif

	spuCtrl2[0]&=~0x30;
	spuStat2[0]|=0x80;
}
                       
EXPORT_GCC void CALLBACK SPU2interruptDMA4(void) 
{
 InterruptDMA4();
}

void InterruptDMA7(void) 
{
// taken from linuzappz NULL spu2
//	spu2Rs16(CORE1_ATTR)&= ~0x30;
//	spu2Rs16(REG__5B0) = 0;
//	spu2Rs16(SPU2_STATX_DREQ)|= 0x80;

#ifdef _WINDOWS
 if(iDebugMode==1) logprintf("IRQDMA7\r\n");
#endif

	spuStat2[1]|=0x80;
	spuCtrl2[1]&=~0x30;
}

EXPORT_GCC void CALLBACK SPU2interruptDMA7(void) 
{
 InterruptDMA7();
}

EXPORT_GCC void CALLBACK SPU2WriteMemAddr(int core, unsigned long value)
{
	MemAddr[core] = value;
}

EXPORT_GCC unsigned long CALLBACK SPU2ReadMemAddr(int core)
{
	return MemAddr[core];
}