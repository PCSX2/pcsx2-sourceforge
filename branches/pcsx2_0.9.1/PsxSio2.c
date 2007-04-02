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

#include <stdio.h>
#include <string.h>

#include "PsxCommon.h"

/*
w [8268]=0x3bc		sio2_start/sio2man
r [8270]		padman_start/padman
	padman->7480[00]=bit4;
	padman->7480[13]=bit5;
  packetExchange(&703F8);
w [8268]|=0x0C;
........
w [8268]|=0x01;

only recv2 & dataout influences padman
*/

//		0xBF808200,0xBF808204,0xBF808208,0xBF80820C,
//		0xBF808210,0xBF808214,0xBF808218,0xBF80821C,	packet->sendArray3
//		0xBF808220,0xBF808224,0xBF808228,0xBF80822C,	call12/13_s/getparams
//		0xBF808230,0xBF808234,0xBF808238,0xBF80823C,

//		0xBF808240,0xBF808248,0xBF808250,0xBF808258,	packet->sendArray1/call_7/8
//		0xBF808244,0xBF80824C,0xBF808254,0xBF80825C,	packet->sendArray2/call_9/10

//		0xBF808260,										serial data/fifo in/out s/get8260_datain/out packet->sendbuf(nomem!)
//                            0xBF808268,				ctrl s/get8268_ctrl

//                                       0xBF80826C,	packet->recv1/2/3 get826C_recv1, get8270_recv2, get8274_recv3
//		0xBF808270,0xBF808274,

//                            0xBF808278,0xBF80827C,	s/get8278, s/get827C
//		0xBF808280										interrupt related s/get8280_intr


void sio2Reset() {
	SysPrintf("Sio2 init\n");
	memset(&sio2, 0, sizeof(sio2));
	sio2.packet.recvVal1 = 0x1D100; // Nothing is connected at start
}

u32 sio2_getRecv1() {
#ifdef PAD_LOG
	PAD_LOG("Reading Recv1 = %x\n",sio2.packet.recvVal1);
#endif
	return sio2.packet.recvVal1;
}

u32 sio2_getRecv2() {
#ifdef PAD_LOG
	PAD_LOG("Reading Recv2 = %x\n",0xF);
#endif
	return 0xF;
}//0, 0x10, 0x20, 0x10 | 0x20; bits 4 & 5

u32 sio2_getRecv3() { 
	if(sio2.packet.recvVal3 == 0x8C || sio2.packet.recvVal3 == 0x8b ||
	   sio2.packet.recvVal3 == 0x83)
	{
#ifdef PAD_LOG
		PAD_LOG("Reading Recv3 = %x\n",sio2.packet.recvVal3);
#endif
		sio.packetsize = sio2.packet.recvVal3;
		sio2.packet.recvVal3 = 0; // Reset
		return sio.packetsize;
	}else{
#ifdef PAD_LOG
		PAD_LOG("Reading Recv3 = %x\n",sio.packetsize << 16);
#endif
        return sio.packetsize << 16;
	}
}

void sio2_setSend1(u32 index, u32 value){sio2.packet.sendArray1[index]=value;}	//0->3
u32 sio2_getSend1(u32 index){return sio2.packet.sendArray1[index];}				//0->3
void sio2_setSend2(u32 index, u32 value){sio2.packet.sendArray2[index]=value;}	//0->3
u32 sio2_getSend2(u32 index){return sio2.packet.sendArray2[index];}				//0->3
void sio2_setSend3(u32 index, u32 value) {int i;
/*	if (index == 0) {
		printf("value: %x\n", value);
	}*/
	sio2.packet.sendArray3[index]=value;
#ifdef PAD_LOG
	if (index==15){
		for (i=0; i<4; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray1[i]);}PAD_LOG("\n");
		for (i=0; i<4; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray2[i]);}PAD_LOG("\n");
		for (i=0; i<8; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray3[i]);}PAD_LOG("\n");
		for (  ; i<16; i++){PAD_LOG("0x%08X ", sio2.packet.sendArray3[i]);}PAD_LOG("\n");
	}
#endif
}	//0->15

u32 sio2_getSend3(u32 index) {return sio2.packet.sendArray3[index];}				//0->15

void sio2_setCtrl(u32 value){
	sio2.ctrl=value;
	if (sio2.ctrl & 1){	//recv packet
		//handle data that had been sent

		//trigger interupt for SIO2
		psxHu32(0x1070)|=0x20000;
		sio2.recvIndex=0;
		sio2.ctrl &= ~1;
	} else { // send packet
		//clean up
		sio2.packet.sendSize=0;	//reset size
		sio2.cmdport=0;
		sio2.cmdlength=0;
		sioWriteCtrl16(SIO_RESET);
	}
}
u32 sio2_getCtrl(){return sio2.ctrl;}

void sio2_setIntr(u32 value){sio2.intr=value;}
u32 sio2_getIntr(){
	return sio2.intr;
}

void sio2_set8278(u32 value){sio2._8278=value;}
u32 sio2_get8278(){return sio2._8278;}
void sio2_set827C(u32 value){sio2._827C=value;}
u32 sio2_get827C(){return sio2._827C;}

void sio2_fifoIn(u8 value){
	u16 ctrl=0x0002;
	if (sio2.packet.sendArray3[sio2.cmdport] && (sio2.cmdlength==0)){//else do nothing!
		
		sio2.cmdlength=(sio2.packet.sendArray3[sio2.cmdport] >> 8) & 0x1FF;
		ctrl &= ~0x2000;
		ctrl |= (sio2.packet.sendArray3[sio2.cmdport] & 1) << 13;//it is correct this way:P; what did you want to do?
		//sioWriteCtrl16(SIO_RESET);
		sioWriteCtrl16(ctrl);
#ifdef PSXDMA_LOG
		PSXDMA_LOG("sio2_fifoIn: ctrl = %x, cmdlength = %x, cmdport = %d (%x)\n", ctrl, sio2.cmdlength, sio2.cmdport, sio2.packet.sendArray3[sio2.cmdport]);
#endif
		sio2.cmdport++;//position in sendArray3
	}
	if (sio2.cmdlength) sio2.cmdlength--;

//	sioWriteCtrl16(ctrl);
	
	sioWrite8(value);
	if (sio2.packet.sendSize > BUFSIZE) {//asadr
		SysPrintf("*PCSX2*: sendSize >= %d\n", BUFSIZE);
	} else {
		sio2.buf[sio2.packet.sendSize] = sioRead8();
		sio2.packet.sendSize++;
	}
}

u8 sio2_fifoOut(){
	u8 value=0;

	if (sio2.recvIndex <= sio2.packet.sendSize){
		value = sio2.buf[sio2.recvIndex++];
	} else {
		SysPrintf("*PCSX2*: buffer overrun\n");
	}
	return value;	//no data
}

/////////////////////////////////////////////////
////////////////////////////////////////////  DMA
/////////////////////////////////////////////////

void psxDma11(u32 madr, u32 bcr, u32 chcr) {
	int wordsize, totalblocks;
#ifdef PSXDMA_LOG
	PSXDMA_LOG("*** DMA 11 - SIO2 in *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif

	if (chcr != 0x01000201) return;
	wordsize = (bcr & 0xffff);
	totalblocks = (bcr >> 16);
	bcr = (totalblocks * wordsize) * 4;	// 8 bits
	PSX_INT(11,(bcr>>2));
	while (bcr > 0) {								
		sio2_fifoIn(PSXMu8(madr));
		bcr--; madr++;
		if(sio2.packet.sendSize == BUFSIZE) break;
	}
	HW_DMA11_MADR = madr;
	HW_DMA11_BCR = bcr;
	
}

int psxDMA11Interrupt()
{
	HW_DMA11_CHCR &= ~0x01000000;
	psxDmaInterrupt2(4);
	return 1;
}

void psxDma12(u32 madr, u32 bcr, u32 chcr) {
	int wordsize, totalblocks;
#ifdef PSXDMA_LOG
	PSXDMA_LOG("*** DMA 12 - SIO2 out *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);
#endif

	if (chcr != 0x41000200) return;

	wordsize = (bcr & 0xffff);
	totalblocks = (bcr >> 16);
	bcr = (totalblocks * wordsize) * 4;	// 8 bits
	sio2.recvIndex = 0; // Set To start    asadr
	PSX_INT(12,(bcr>>2));
	while (bcr > 0) {
		PSXMu8(madr) = sio2_fifoOut();
		bcr--; madr++;
		if(sio2.recvIndex == sio2.packet.sendSize) break;
	}
	HW_DMA12_MADR = madr;
	HW_DMA12_BCR = bcr;
	PSX_INT(12,(bcr>>2));
	
}

int psxDMA12Interrupt()
{
	HW_DMA12_CHCR &= ~0x01000000;
	psxDmaInterrupt2(5);
	return 1;
}

int  sio2Freeze(gzFile f, int Mode) {
	gzfreeze(&sio2, sizeof(sio2));

	return 0;
}

