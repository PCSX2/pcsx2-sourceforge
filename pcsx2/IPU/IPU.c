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

#include "Common.h"
#include "IPU.h"
#include "mpeg2lib/Mpeg.h"
#include "yuv2rgb.h"

#include <assert.h>
#include <time.h>

#ifdef __WIN32__
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif

#ifndef WIN32_VIRTUAL_MEM
IPUregisters g_ipuRegsReal;
#endif

#define ipu0dma ((DMACh *)&PS2MEM_HW[0xb000])
#define ipu1dma ((DMACh *)&PS2MEM_HW[0xb400])

extern pthread_mutex_t g_mtxDMAs[10];

static int g_nIPUInterrupt = 0; // if 1, IPU interrupt is generated
static int g_nDMATransfer = 0;	// if highest bit set, abort current op

static int s_nIPU0Start = 0, s_nIPU1Start = 0;

int g_nOutputIPUFIFO = 0;
const void* g_pOutputIPUFIFO = NULL;

pthread_t IPUthread = {0};
pthread_mutex_t g_mtxIPU = NULL, g_mtxIPUThread = NULL, g_mtxLog = NULL, g_mtxHwIntc = NULL;
pthread_cond_t g_condIPU0 = NULL, g_condIPU1 = NULL, // signalled when the dma is exec
				g_condNewCMD = NULL;					// signalled when a new command arrives0

// read from the input fifo, if mandatory is 1, will block until all the memory is received
int fifo_wread(void *value, int size);

// read one qword, will return an error if 1 qword is not present
int fifo_wread1(void *value);

void fifo_wclear();
//int g_BP.IFC = 0;
int readwpos = 0, writewpos = 0;
__declspec(align(16)) u32 fifo_input[32];

void ReorderBitstream();
void FillInternalBuffer(u32 * pointer, u32 advance);
void getBits32(u8 *address, u32 advance);
void getBits16(u8 *address, u32 advance);
void getBits8(u8 *address, u32 advance);
void getBits(u8 *address, u32 size, u32 advance);

// the BP doesn't advance and returns -1 if there is no data to be read
//int peakBits32(u8* address);
tIPU_BP g_BP;

void IPUCMD_WRITE(u32 val);
void* IPUMainThread(void * null);
void IPU0dma(const void* pMem, int size);
void IPU1dma();

int ipu0cycles = 0;
int ipu1cycles = 0;

u32 FASTCALL BigEndian(u32 a){
	return ((((a >> 24) & 0xFF) <<  0) + (((a >> 16) & 0xFF) <<  8) +
			(((a >>  8) & 0xFF) << 16) + (((a >>  0) & 0xFF) << 24));
}

// Color conversion stuff
char convert_data_buffer[0x1C];
convert_init_t	convert_init={convert_data_buffer, 0x1C};
convert_t *convert;

// Quantization matrix
u8 niq[64],			//non-intraquant matrix
	iq[64];			//intraquant matrix
u16 vqclut[16];			//clut conversion table
u8 th0=0, th1=0;		//thresholds for color conversions
int coded_block_pattern=0;
struct macroblock_8  mb8;
struct macroblock_16 mb16;
struct macroblock_rgb32 rgb32;
struct macroblock_rgb16 rgb16;
u32 data = 0;
u8 indx4[16*16/2];
u32	mpeg2_inited;		//mpeg2_idct_init() must be called only once
u8 PCT[]={'r', 'I', 'P', 'B', 'D', '-', '-', '-'};
static decoder_t decoder;						//static, only to place it in bss
decoder_t tempdec;

extern u8 mpeg2_scan_norm[64];
extern u8 mpeg2_scan_alt[64];

__declspec(align(16)) u8 _readbits[80];	//local buffer (ring buffer)
u8* readbits = _readbits; // always can decrement by one 1qw
int bufferhasnew = 0;

#define SATURATE_4BITS(val)		((val)>15 ? 15 : (val))


#define IPUProcessInterrupt() { \
	if( InterlockedExchange(&g_nIPUInterrupt, 0) ) { \
		ipuRegs->cmd.BUSY = 0; \
		ipuRegs->ctrl.BUSY = 0; \
		ipuRegs->topbusy = 0; \
		hwIntcIrq(INTC_IPU); \
	} \
} \

/////////////////////////////////////////////////////////
// Register accesses (run on EE thread)
int ipuInit()
{
	memset(ipuRegs, 0, sizeof(IPUregisters));

	pthread_cond_init(&g_condIPU0, NULL);
	pthread_cond_init(&g_condIPU1, NULL);
	pthread_cond_init(&g_condNewCMD, NULL);

	pthread_mutex_init(&g_mtxIPU, NULL);
	pthread_mutex_init(&g_mtxIPUThread, NULL);
	pthread_mutex_init(&g_mtxLog, NULL);
	pthread_mutex_init(&g_mtxHwIntc, NULL);

	pthread_create(&IPUthread, NULL, IPUMainThread, NULL);

	return 0;
}

void ipuReset()
{
	memset(ipuRegs, 0, sizeof(IPUregisters));
}

void ipuShutdown()
{
	pthread_cancel(IPUthread);
	pthread_cond_signal(&g_condNewCMD);
	pthread_join(IPUthread, NULL);

	DESTROY_COND(g_condIPU0);
	DESTROY_COND(g_condIPU1);
	DESTROY_COND(g_condNewCMD);

	DESTROY_MUTEX(g_mtxIPU);
	DESTROY_MUTEX(g_mtxIPUThread);
	DESTROY_MUTEX(g_mtxLog);
	DESTROY_MUTEX(g_mtxHwIntc);
}

u32 IPU_read32(u32 mem)
{
	if( CHECK_SAFEIPU ) {
		pthread_mutex_lock(&g_mtxIPU);
		pthread_mutex_unlock(&g_mtxIPU);
	}
#ifdef IPU_LOG
			IPU_LOG("Ipu read32: %x\n",mem);
#endif
	IPUProcessInterrupt();

	switch (mem){
//		case 0x10002000: // IPU_CMD
//#ifdef IPU_LOG
//			IPU_LOG("Ipu read32: IPU_CMD=0x%08X\n", ipuRegs->cmd.DATA);
//#endif
//			return ipuRegs->cmd.DATA;

		case 0x10002010: // IPU_CTRL

			ipuRegs->ctrl.IFC = g_BP.IFC;
			ipuRegs->ctrl.OFC = min(8, g_nOutputIPUFIFO);
			ipuRegs->ctrl.CBP = coded_block_pattern;

#ifdef IPU_LOG
			if( !ipuRegs->ctrl.BUSY ) {
				IPU_LOG("Ipu read32: IPU_CTRL=0x%08X\n", ipuRegs->ctrl._u32);
			}
#endif
			return ipuRegs->ctrl._u32;

		case 0x10002020: // IPU_BP

			ipuRegs->ipubp = g_BP.BP & 0x7f;
			ipuRegs->ipubp |= (g_BP.IFC <<  8);
			ipuRegs->ipubp |= (g_BP.FP+bufferhasnew) << 16;

#ifdef IPU_LOG
			IPU_LOG("Ipu read32: IPU_BP=0x%08X\n", *(u32*)&g_BP);
#endif
			return ipuRegs->ipubp;
	}

	return *(u32*)(((u8*)ipuRegs)+(mem&0xff)); // ipu repeats every 0x100
}

u64 IPU_read64(u32 mem)
{
#ifdef IPU_LOG
			IPU_LOG("Ipu read64: %x\n",mem);
#endif
	IPUProcessInterrupt();

//	switch (mem){
//		case 0x10002000: // IPU_CMD
//#ifdef IPU_LOG
//			if(!ipuRegs->cmd.BUSY){
//				IPU_LOG("Ipu read64: IPU_CMD=BUSY=%x, DATA=%08X\n", ipuRegs->cmd.BUSY?1:0,ipuRegs->cmd.DATA);
//			}
//#endif
//			//return *(u64*)&ipuRegs->cmd;
//			break;
//
//		case 0x10002030: // IPU_TOP
//#ifdef IPU_LOG
//			IPU_LOG("Ipu read64: IPU_TOP=%x,  bp = %d\n",ipuRegs->top,g_BP.BP);
//#endif
//			//return *(u64*)&ipuRegs->top;
//			break;
//
//		default:
//#ifdef IPU_LOG
//			IPU_LOG("Ipu read64: Unknown=%x\n", mem);
//#endif
//			break;
//
//	}
	return *(u64*)(((u8*)ipuRegs)+(mem&0xff));
}

void IPU_write32(u32 mem,u32 value)
{
	IPUProcessInterrupt();

	switch (mem){
		case 0x10002000: // IPU_CMD
#ifdef IPU_LOG
	        IPU_LOG("Ipu write32: IPU_CMD=0x%08X\n",value);
#endif
			ipuRegs->cmd.DATA = value;
			IPUCMD_WRITE(value);
			break;
		case 0x10002010: // IPU_CTRL
			ipuRegs->ctrl._u32 = (value&0x47f30000)|(ipuRegs->ctrl._u32&0x8000ffff);
			if (ipuRegs->ctrl.RST & 0x1) { // RESET

				// threads can be waiting on dma signals that will never come, so set abort bit and signal
				g_nDMATransfer |= 0x80000000;

				while(1) {
					// signal so threads can be freed
					pthread_cond_signal(&g_condIPU0);
					pthread_cond_signal(&g_condIPU1);

					if( pthread_mutex_trylock(&g_mtxIPUThread) != EBUSY ) {
						IPUProcessInterrupt();
						if( !ipuRegs->ctrl.BUSY )
							break;
						pthread_mutex_unlock(&g_mtxIPUThread);
						pthread_cond_signal(&g_condNewCMD);
					}

					// threads are not ready so wait
					sched_yield();
				}

				if (!mpeg2_inited){
                    mpeg2_idct_init();
					convert=convert_rgb (CONVERT_RGB, 32);
					convert(16, 16, 0, NULL, &convert_init);
					memset(mb8.Y,0,sizeof(mb8.Y));
					memset(mb8.Cb,0,sizeof(mb8.Cb));
					memset(mb8.Cr,0,sizeof(mb8.Cr));
					memset(mb16.Y,0,sizeof(mb16.Y));
					memset(mb16.Cb,0,sizeof(mb16.Cb));
					memset(mb16.Cr,0,sizeof(mb16.Cr));
					mpeg2_inited=1;
				}
				fifo_wclear();
				coded_block_pattern = 0;

				ipuRegs->ctrl._u32 = 0;
				g_BP.BP     = 0;
				g_BP.IFC    = 0;
				g_BP.FP     = 0;
				ipuRegs->top = 0;
				g_nDMATransfer = 0;
				g_nIPUInterrupt = 0;
				g_nOutputIPUFIFO = 0;

				pthread_mutex_unlock(&g_mtxIPUThread);
			}

#ifdef IPU_LOG
	        IPU_LOG("Ipu write32: IPU_CTRL=0x%08X\n",value);
#endif

			break;
		default:
#ifdef IPU_LOG
			IPU_LOG("Ipu write32: Unknown=%x\n", mem);
#endif
			*(u32*)((u8*)ipuRegs + (mem&0xfff)) = value;
			break;
	}
}

void IPU_write64(u32 mem, u64 value)
{
	IPUProcessInterrupt();

	switch (mem){
		case 0x10002000:
#ifdef IPU_LOG
	        IPU_LOG("Ipu write64: IPU_CMD=0x%08X\n",value);
#endif
			IPUCMD_WRITE((u32)value);
			break;

		default:
#ifdef IPU_LOG
			IPU_LOG("Ipu write64: Unknown=%x\n", mem);
#endif
			*(u64*)((u8*)ipuRegs + (mem&0xfff)) = value;
			break;
	}
}

///////////////////////////////////////////
// IPU Commands (exec on worker thread only)

static void ipuBCLR(u32 val) {
    fifo_wclear();
	g_BP.BP = val & 0x7F;
	g_BP.FP = 0;
	g_BP.IFC = 0;
#ifdef IPU_LOG
	IPU_LOG("Clear IPU input FIFO. Set Bit offset=0x%X\n", g_BP.BP);
#endif
}

void ipuIDEC(u32 val) {
	tIPU_CMD_IDEC idec={0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	*(u32*)&idec=val;
#ifdef IPU_LOG
						IPU_LOG("IPU IDEC command.\n");
	if (idec.FB){		IPU_LOG(" Skip %d bits.",idec.FB);}
						IPU_LOG(" Quantizer step code=0x%X.",idec.QSC);
	if (idec.DTD==0){	IPU_LOG(" Does not decode DT.");
	}else{				IPU_LOG(" Decodes DT.");}
	if (idec.SGN==0){	IPU_LOG(" No bias.");
	}else{				IPU_LOG(" Bias=128.");}
	if (idec.DTE==1){	IPU_LOG(" Dither Enabled.");}
	if (idec.OFM==0){	IPU_LOG(" Output format is RGB32.");
	}else{				IPU_LOG(" Output format is RGB16.");}
						IPU_LOG("\n");
#endif
	g_BP.BP+= idec.FB;//skip FB bits
	//from IPU_CTRL
	ipuRegs->ctrl.PCT = I_TYPE; //Intra DECoding;)
	decoder.coding_type		=ipuRegs->ctrl.PCT;
	decoder.mpeg1			=ipuRegs->ctrl.MP1;
	decoder.q_scale_type	=ipuRegs->ctrl.QST;
	decoder.intra_vlc_format=ipuRegs->ctrl.IVF;
	decoder.scan			=ipuRegs->ctrl.AS ? mpeg2_scan_alt: mpeg2_scan_norm;
	decoder.intra_dc_precision=ipuRegs->ctrl.IDP;
	//from IDEC value
	decoder.quantizer_scale	=idec.QSC;
	decoder.frame_pred_frame_dct=!idec.DTD;
	decoder.sgn				=idec.SGN;
	decoder.dte				=idec.DTE;
	decoder.ofm				=idec.OFM;
	//other stuff
	decoder.dcr				  =1;//resets DC prediction value
	decoder.intra_quantizer_matrix		=(u8*)iq;
	decoder.non_intra_quantizer_matrix	=(u8*)niq;
	decoder.picture_structure = FRAME_PICTURE;	//default: progressive...my guess:P
	decoder.mb8	=&mb8;
	decoder.mb16=&mb16;
	decoder.rgb32=&rgb32;
	decoder.rgb16=&rgb16;
	decoder.stride=16;

	mpeg2sliceIDEC(&decoder);
}

static void ipuBDEC(u32 val) {
	tIPU_CMD_BDEC bdec={0, 0, 0, 0, 0, 0, 0, 0};
	*(u32*)&bdec=val;

#ifdef IPU_LOG
							IPU_LOG("IPU BDEC(macroblock decode) command.\n");
	if (bdec.FB){			IPU_LOG(" Skip 0x%X bits.", bdec.FB);}
	if (bdec.MBI){			IPU_LOG(" Intra MB.");}
	else{					IPU_LOG(" Non-intra MB.");}
	if (bdec.DCR){			IPU_LOG(" Resets DC prediction value.");}
	else{					IPU_LOG(" Doesn't reset DC prediction value.");}
	if (bdec.DT){			IPU_LOG(" Use field DCT.");}
	else{					IPU_LOG(" Use frame DCT.");}
							IPU_LOG(" Quantiser step=0x%X\n",bdec.QSC);
#endif
	g_BP.BP+= bdec.FB;//skip FB bits
	decoder.coding_type		= I_TYPE;
	decoder.mpeg1			=ipuRegs->ctrl.MP1;
	decoder.q_scale_type	=ipuRegs->ctrl.QST;
	decoder.intra_vlc_format=ipuRegs->ctrl.IVF;
	decoder.scan			=ipuRegs->ctrl.AS ? mpeg2_scan_alt: mpeg2_scan_norm;
	decoder.intra_dc_precision=ipuRegs->ctrl.IDP;
	//from BDEC value
	/* JayteeMaster: the quantizer (linear/non linear) depends on the q_scale_type */
	decoder.quantizer_scale  =decoder.q_scale_type?non_linear_quantizer_scale [bdec.QSC]:bdec.QSC<<1;
	decoder.macroblock_modes =bdec.DT ? DCT_TYPE_INTERLACED : 0;
	decoder.dcr				 =bdec.DCR;
	decoder.macroblock_modes|=bdec.MBI ? MACROBLOCK_INTRA : MACROBLOCK_PATTERN;
	//other stuff
	decoder.intra_quantizer_matrix		=(u8*)iq;
	decoder.non_intra_quantizer_matrix	=(u8*)niq;
	decoder.picture_structure = FRAME_PICTURE;	//default: progressive...my guess:P
	decoder.mb8	=&mb8;
	decoder.mb16=&mb16;
	decoder.rgb32=&rgb32;
	decoder.rgb16=&rgb16;
	decoder.stride=16;

	memset(&mb8, 0, sizeof(struct macroblock_8));
	memset(&mb16, 0, sizeof(struct macroblock_16));
	
	mpeg2_slice(&decoder);
}

static void ipuVDEC(u32 val) {
	g_BP.BP+= val & 0x3F;	
	
	ipuRegs->cmd.DATA = 0;
 	getBits32((u8*)&decoder.bitstream_buf, 0);
	
	decoder.bitstream_bits = -16;	 
	decoder.bitstream_buf=BigEndian(decoder.bitstream_buf);
 
	//value = BigEndian(value);
	switch((val >> 26) & 3){
		case 0://Macroblock Address Increment
			decoder.mpeg1			=ipuRegs->ctrl.MP1;
			ipuRegs->cmd.DATA = get_macroblock_address_increment(&decoder);
			break;
		case 1://Macroblock Type	//known issues: no error detected
			decoder.frame_pred_frame_dct=1;//prevent DCT_TYPE_INTERLACED
			decoder.coding_type		=ipuRegs->ctrl.PCT;
			ipuRegs->cmd.DATA=get_macroblock_modes(&decoder);
			break;
		case 2://Motion Code		//known issues: no error detected
			ipuRegs->cmd.DATA=get_motion_delta(&decoder,0);
			//g_BP.BP += ipuRegs->cmd.DATA >> 16;
			break;
		case 3://DMVector
			ipuRegs->cmd.DATA=get_dmv(&decoder);
			//g_BP.BP += ipuRegs->cmd.DATA >> 16;
			break;
	}

	//g_BP.BP += ipuRegs->cmd.DATA >> 16;
	g_BP.BP+=(decoder.bitstream_bits+16);
	if((int)g_BP.BP < 0) {
		g_BP.BP += 128;
		ReorderBitstream();
	}

	ipuRegs->cmd.DATA = (ipuRegs->cmd.DATA & 0xFFFF) | ((decoder.bitstream_bits+16) << 16);
	FillInternalBuffer(&g_BP.BP,1);

	ipuRegs->ctrl.ECD = (ipuRegs->cmd.DATA==0);

#ifdef IPU_LOG
	IPU_LOG("IPU VDEC command data 0x%x(0x%x). Skip 0x%X bits/Table=%d (%s), pct %d\n",
		ipuRegs->cmd.DATA,ipuRegs->cmd.DATA >> 16,val & 0x3f, (val >> 26) & 3, (val >> 26) & 1 ?
		((val >> 26) & 2 ? "DMV" : "MBT") : (((val >> 26) & 2 ? "MC" : "MBAI")),ipuRegs->ctrl.PCT);
#endif
}

static void ipuFDEC(u32 val)
{
#ifdef IPU_LOG
	IPU_LOG("IPU FDEC command. Skip 0x%X bits, FIFO 0x%X bytes, BP 0x%X, FP %d, CHCR 0x%x\n",
		val & 0x3f,g_BP.IFC,(int)g_BP.BP,g_BP.FP,((DMACh*)&PS2MEM_HW[0xb400])->chcr);
#endif
	g_BP.BP+= val & 0x3F;
	getBits32((u8*)&ipuRegs->cmd.DATA, 0);
	ipuRegs->cmd.DATA = BigEndian(ipuRegs->cmd.DATA);
}

static void ipuSETIQ(u32 val)
{
	int i;

#ifdef IPU_LOG
	IPU_LOG("IPU SETIQ command.\n");
#endif
#ifdef IPU_LOG
	if (val & 0x3f){
		IPU_LOG("Skip %d bits.\n", val & 0x3f);
	}
#endif
	g_BP.BP+= val & 0x3F;

 	if ((val >> 27) & 1){
		getBits((u8*)niq, 512, 1); // 8*8*8
#ifdef IPU_LOG
		IPU_LOG("Read non-intra quantisation matrix from IPU FIFO.\n");
		for (i=0; i<8; i++){
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X\n",
				niq[i*8+0], niq[i*8+1], niq[i*8+2], niq[i*8+3],
				niq[i*8+4], niq[i*8+5], niq[i*8+6], niq[i*8+7]);
		}
#endif
	}else{
		getBits((u8*)iq, 512, 1);
#ifdef IPU_LOG
		IPU_LOG("Read intra quantisation matrix from IPU FIFO.\n");
		for (i=0; i<8; i++){
			IPU_LOG("%02X %02X %02X %02X %02X %02X %02X %02X\n",
				iq[i*8+0], iq[i*8+1], iq[i*8+2], iq[i*8+3],
				iq[i*8+4], iq[i*8+5], iq[i*8+6], iq[i*8+7]);
		}
#endif
	}
}

static void ipuSETVQ(u32 val)
{	
	getBits((u8*)vqclut, 256, 1); // 16*2*8

#ifdef IPU_LOG
	IPU_LOG("IPU SETVQ command.\nRead VQCLUT table from IPU FIFO.\n");
	IPU_LOG(
		"%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d "
		"%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d\n"
		"%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d "
		"%02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d %02d:%02d:%02d\n",
		vqclut[0] >> 10, (vqclut[0] >> 5) & 0x1F, vqclut[0] & 0x1F,
		vqclut[1] >> 10, (vqclut[1] >> 5) & 0x1F, vqclut[1] & 0x1F,
		vqclut[2] >> 10, (vqclut[2] >> 5) & 0x1F, vqclut[2] & 0x1F,
		vqclut[3] >> 10, (vqclut[3] >> 5) & 0x1F, vqclut[3] & 0x1F,
		vqclut[4] >> 10, (vqclut[4] >> 5) & 0x1F, vqclut[4] & 0x1F,
		vqclut[5] >> 10, (vqclut[5] >> 5) & 0x1F, vqclut[5] & 0x1F,
		vqclut[6] >> 10, (vqclut[6] >> 5) & 0x1F, vqclut[6] & 0x1F,
		vqclut[7] >> 10, (vqclut[7] >> 5) & 0x1F, vqclut[7] & 0x1F,
		vqclut[8] >> 10, (vqclut[8] >> 5) & 0x1F, vqclut[8] & 0x1F,
		vqclut[9] >> 10, (vqclut[9] >> 5) & 0x1F, vqclut[9] & 0x1F,
		vqclut[10] >> 10, (vqclut[10] >> 5) & 0x1F, vqclut[10] & 0x1F,
		vqclut[11] >> 10, (vqclut[11] >> 5) & 0x1F, vqclut[11] & 0x1F,
		vqclut[12] >> 10, (vqclut[12] >> 5) & 0x1F, vqclut[12] & 0x1F,
		vqclut[13] >> 10, (vqclut[13] >> 5) & 0x1F, vqclut[13] & 0x1F,
		vqclut[14] >> 10, (vqclut[14] >> 5) & 0x1F, vqclut[14] & 0x1F,
		vqclut[15] >> 10, (vqclut[15] >> 5) & 0x1F, vqclut[15] & 0x1F);
#endif
}

// IPU Transfers are split into 8Qwords so we need to send ALL the data
static void ipuCSC(u32 val)
{
	tIPU_CMD_CSC csc ={0, 0, 0, 0, 0};
	int i;
	*(u32*)&csc=val;

#ifdef IPU_LOG
	IPU_LOG("IPU CSC(Colorspace conversion from YCbCr) command (%d).\n",csc.MBC);
	if (csc.OFM){	IPU_LOG("Output format is RGB16. ");}
	else{			IPU_LOG("Output format is RGB32. ");}
	if (csc.DTE){	IPU_LOG("Dithering enabled.");	}
#endif
	//SysPrintf("CSC\n");
	for (i=0; i<(int)csc.MBC; i++){
		getBits((u8*)&mb8, 3072, 1);
		ipu_csc(&mb8, &rgb32, 0);
	
		if (csc.OFM){
			ipu_dither(&mb8, &rgb16, csc.DTE);
			IPU0dma(((u32*)&rgb16), 32);
		}else{
			IPU0dma(((u32*)&rgb32), 64);
		}
	}
}

// Todo - Need to add the same stop and start code as CSC
static void ipuPACK(u32 val) {
	int i;
	tIPU_CMD_CSC  csc ={0, 0, 0, 0, 0};
 
	*(u32*)&csc=val;
#ifdef IPU_LOG
	IPU_LOG("IPU PACK (Colorspace conversion from RGB32) command.\n");
	if (csc.OFM){	IPU_LOG("Output format is RGB16. ");}
	else{			IPU_LOG("Output format is INDX4. ");}
	if (csc.DTE){	IPU_LOG("Dithering enabled.");	}
	IPU_LOG("Number of macroblocks to be converted: %d\n", csc.MBC);
#endif
	for (i=0; i<(int)csc.MBC; i++){
		fifo_wread(&rgb32, 64);
		//ipu_dither(&mb8, &rgb16, csc.DTE);
		if (csc.OFM)
			IPU0dma(&rgb16, 32);
		else{
		//	ipu_vq(&rgb16, indx4);
			IPU0dma(indx4,8);
		}
	}	
}

static void ipuSETTH(u32 val) {
	th0=val & 0xff;
	th1=(val>>16) & 0xff;
#ifdef IPU_LOG
	IPU_LOG("IPU SETTH (Set threshold value)command.\n"
		"Transparent threshold=%x/Translucent threshold=%x\n", th0, th1);
#endif
}

///////////////////////
// IPU Worker Thread //
///////////////////////
void IPUCMD_WRITE(u32 val) {
	// don't process anything if currently busy
	if( ipuRegs->ctrl.BUSY ) {
		// wait for thread
		SysPrintf("IPU BUSY!\n");

		// uncomment return and comment locks if this freezes games
		pthread_mutex_lock(&g_mtxIPUThread);
		pthread_mutex_unlock(&g_mtxIPUThread);
		//return;
	}

	ipuRegs->ctrl.ECD = 0;
	ipuRegs->ctrl.SCD = 0; //clear ECD/SCD
	ipuRegs->cmd.DATA = val;

	switch (ipuRegs->cmd.CMD) {
		case SCE_IPU_BCLR:
			ipuBCLR(val);
			hwIntcIrq(INTC_IPU);
			return;

		case SCE_IPU_VDEC:

			// check if enough data in queue
			if( g_BP.FP == 2 || (g_BP.FP >= 1 && bufferhasnew) ) {
				ipuVDEC(val);

				getBits32((u8*)&ipuRegs->top, 0);
				ipuRegs->top = BigEndian(ipuRegs->top);
				hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs->cmd.BUSY = 0x80000000;
			ipuRegs->topbusy = 0x80000000;

			break;

		case SCE_IPU_FDEC:
			if( g_BP.FP == 2 || (g_BP.FP >= 1 && bufferhasnew) ) {
				ipuFDEC(val);

				getBits32((u8*)&ipuRegs->top, 0);
				ipuRegs->top = BigEndian(ipuRegs->top);
				hwIntcIrq(INTC_IPU);
				return;
			}

			ipuRegs->cmd.BUSY = 0x80000000;
			ipuRegs->topbusy = 0x80000000;

			break;
		
		case SCE_IPU_SETTH:
			ipuSETTH(val);
			hwIntcIrq(INTC_IPU);
			return;
	}

	// have to resort to the thread
	ipuRegs->ctrl.BUSY = 1;

	// use the mutex to make sure thread is woken
	pthread_mutex_lock(&g_mtxIPU);

	pthread_mutex_lock(&g_mtxIPUThread);
	pthread_mutex_unlock(&g_mtxIPUThread);
	pthread_cond_signal(&g_condNewCMD);
	
	sched_yield(); // TODO: necessary?

	if( CHECK_SAFEIPU ) {
		pthread_mutex_lock(&g_mtxIPU);
		pthread_mutex_unlock(&g_mtxIPU);
	}
}

void* IPUMainThread(void * null)
{
	int oldcmd, id = 0;

	pthread_mutex_lock(&g_mtxIPUThread);

	__try {

	while(1) {

#ifdef IPU_LOG
	//IPU_LOG("=============================================================\n");
#endif
		// wait until the next command
		pthread_cond_wait(&g_condNewCMD, &g_mtxIPUThread);
		pthread_testcancel();
	
		id = 0;
		oldcmd = ipuRegs->cmd.CMD;
		switch (ipuRegs->cmd.CMD) {
			case SCE_IPU_BCLR: ipuBCLR(ipuRegs->cmd.DATA); break;
			case SCE_IPU_IDEC: ipuIDEC(ipuRegs->cmd.DATA); id = 1; break;
			case SCE_IPU_BDEC: ipuBDEC(ipuRegs->cmd.DATA); id = 1; break;
			case SCE_IPU_VDEC: ipuVDEC(ipuRegs->cmd.DATA); id = 1; break;
			case SCE_IPU_FDEC: ipuFDEC(ipuRegs->cmd.DATA); id = 1; break;
			case SCE_IPU_SETIQ:	ipuSETIQ(ipuRegs->cmd.DATA); break;
			case SCE_IPU_SETVQ: ipuSETVQ(ipuRegs->cmd.DATA); break;
			case SCE_IPU_CSC: ipuCSC(ipuRegs->cmd.DATA); break;		
			case SCE_IPU_PACK: ipuPACK(ipuRegs->cmd.DATA); break;
			case SCE_IPU_SETTH: ipuSETTH(ipuRegs->cmd.DATA); break;
			default:
				SysPrintf("Unknown IPU command: %x\n", ipuRegs->cmd.CMD);
				break;
		}

		if( id ) {
			ipuRegs->top = 0;
			getBits32((u8*)&ipuRegs->top, 0);
			ipuRegs->top = BigEndian(ipuRegs->top);
		}

#ifdef IPU_LOG
	//IPU_LOG("=============================================================\n");
#endif

		g_nIPUInterrupt = 1;
		pthread_mutex_unlock(&g_mtxIPU);
	}

	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}

	return NULL;
}

/////////////////
// Buffer reader

// move the readbits queue
__forceinline void inc_readbits()
{
	readbits += 16;
	if( readbits >= _readbits+64 ) {

		// move back
		*(u64*)(_readbits) = *(u64*)(_readbits+64);
		*(u64*)(_readbits+8) = *(u64*)(_readbits+72);
		readbits = _readbits;
	}
}

// returns the pointer of readbits moved by 1 qword
__forceinline u8* next_readbits()
{
	return readbits + 16;
}

// returns the pointer of readbits moved by 1 qword
u8* prev_readbits()
{
	if( readbits < _readbits+16 ) {
		return _readbits+48-(readbits-_readbits);
	}
	
	return readbits-16;
}

void ReorderBitstream()
{
	readbits = prev_readbits();
	bufferhasnew = 1;
	//g_BP.FP++;
}

// IPU has a 2qword internal buffer whose status is pointed by FP.
// If FP is 1, there's 1 qword in buffer. Second qword is only loaded
// incase there are less than 32bits available in the first qword.
void FillInternalBuffer(u32 * pointer, u32 advance)
{
	if(g_BP.FP == 0)
	{
		bufferhasnew = 0;
		inc_readbits();
		fifo_wread1(readbits);
		g_BP.FP = 1;

		if( *(int*)pointer >= 128 ) {
			inc_readbits();
			fifo_wread1(readbits);
			*pointer &= 127;
		}
	}
   	else if(g_BP.FP < 2 && bufferhasnew == 0 ) 
	{
		// in reality, need only > 96, but IPU reads ahead
		if( (*(int*)pointer) > 96 || ((*(int*)pointer) >= 96 && g_BP.IFC > 0) ) {
			g_BP.FP += fifo_wread1(next_readbits());
			//assert(g_BP.FP == 2);
		}
	}

	if(*(int*)pointer >= 128)
	{
		assert( g_BP.FP >= 1 || bufferhasnew);
		inc_readbits();

		if(advance)
		{
			// Incase BDEC reorders bits, we need to make sure we have the old
			// data backed up. So we store the last read qword into the 2nd slot
			// of the internal buffer. After that we copy the new qword read in
			// the 2nd slot to the 1st slot to be read.
			if( !bufferhasnew )
				g_BP.FP--;
			bufferhasnew = 0;
		}

		*pointer &= 127;
	}
}

// whenever reading fractions of bytes. The low bits always come from the next byte
// while the high bits come from the current byte
void getBits32(u8 *address, u32 advance)
{
	register u32 mask, shift=0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	FillInternalBuffer(&g_BP.BP,1);

	readpos = readbits+(int)g_BP.BP/8;

	if (g_BP.BP & 7) {
		
		shift = g_BP.BP&7;
		mask = (0xff>>shift);
		mask = mask|(mask<<8)|(mask<<16)|(mask<<24);
			
		*(u32*)address = ((~mask&*(u32*)(readpos+1))>>(8-shift)) | (((mask)&*(u32*)readpos)<<shift);
	}
	else
		*(u32*)address = *(u32*)readpos;

	if (advance) g_BP.BP+=32;
}

void getBits16(u8 *address, u32 advance)
{
	register u32 mask, shift=0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	FillInternalBuffer(&g_BP.BP,1);

	readpos = readbits+(int)g_BP.BP/8;

	if (g_BP.BP & 7) {
		
		shift = g_BP.BP&7;
		mask = (0xff>>shift);
		mask = mask|(mask<<8);
			
		*(u16*)address = ((~mask&*(u16*)(readpos+1))>>(8-shift)) | (((mask)&*(u16*)readpos)<<shift);
	}
	else
		*(u16*)address = *(u16*)readpos;

	if (advance) g_BP.BP+=16;
}

void getBits8(u8 *address, u32 advance)
{
	register u32 mask, shift=0;
	u8* readpos;

	// Check if the current BP has exceeded or reached the limit of 128
	FillInternalBuffer(&g_BP.BP,1);

	readpos = readbits+(int)g_BP.BP/8;

	if (g_BP.BP & 7) {
		
		shift = g_BP.BP&7;
		mask = (0xff>>shift);
			
		*(u8*)address = (((~mask)&readpos[1])>>(8-shift)) | (((mask)&*readpos)<<shift);
	}
	else
		*(u8*)address = *(u8*)readpos;

	if (advance) g_BP.BP+=8;
}

void getBits(u8 *address, u32 size, u32 advance)
{
	register u32 mask = 0, shift=0, howmuch;
	u8* oldbits;
	u32 pointer = 0;

	// Check if the current BP has exceeded or reached the limit of 128
	FillInternalBuffer(&g_BP.BP,1);
	
	oldbits = readbits;
    // Backup the current BP in case of VDEC/FDEC
	pointer = g_BP.BP;

	if (pointer & 7)
	{
		address--;
		while (size)
		{
			if (shift==0)
			{
				*++address=0;
				shift=8;
			}
			FillInternalBuffer(&pointer,advance);
			
			howmuch   = min(min(8-(pointer&7), 128-pointer),
						min(size, shift));
			mask	  = ((0xFF >> (pointer&7)) << 
					    (8-howmuch-(pointer&7))) & 0xFF;
			mask     &= readbits[((pointer)>>3)];
			mask    >>= 8-howmuch-(pointer&7);
			pointer  += howmuch;
			size     -= howmuch;
			shift	 -= howmuch;
			*address |= mask << shift;
		}
	}
	else
	{
		u8* readmem;
		while (size)
		{
			FillInternalBuffer(&pointer,advance);
			
			howmuch  = min(128-pointer, size);
			size    -= howmuch;

			readmem = readbits + (pointer>>3);
			pointer += howmuch;
			howmuch >>= 3;
			
			while(howmuch >= 4) {
				*(u32*)address = *(u32*)readmem;
				howmuch -= 4;
				address += 4;
				readmem += 4;
			}

			switch(howmuch) {
				case 3: address[2] = readmem[2];
				case 2: address[1] = readmem[1];
				case 1: address[0] = readmem[0];
				case 0:
					break;
				default: __assume(0);
			}

			address += howmuch;
		}
	}

	// If not advance then reset the Reading buffer value
	if(advance) g_BP.BP = pointer;
	else readbits = oldbits; // restore the last pointer
}

//////////////////////////////////////////////////////
///////////////////// CORE FUNCTIONS /////////////////
//////////////////////////////////////////////////////
void ipu_csc(struct macroblock_8 *mb8, struct macroblock_rgb32 *rgb32, int sgn){
	convert_init.start(convert_init.id, (u8*)rgb32, CONVERT_FRAME);
	convert_init.copy(convert_init.id, (u8*)mb8->Y, (u8*)mb8->Cr, (u8*)mb8->Cb, 0);
}

void ipu_dither(struct macroblock_8 *mb8, struct macroblock_rgb16 *rgb16, int dte){
	SysPrintf("IPU: Dither not implemented");
}

void ipu_vq(struct macroblock_rgb16 *rgb16, u8* indx4){
	SysPrintf("IPU: VQ not implemented");
}

void ipu_copy(struct macroblock_8 *mb8, struct macroblock_16 *mb16){
	unsigned char	*s=(unsigned char*)mb8;
	signed short	*d=(signed short*)mb16;
	int i;
	for (i=0; i< 256; i++)	*d++ = *s++;		//Y  bias	- 16
	for (i=0; i< 64; i++)	*d++ = *s++;		//Cr bias	- 128
	for (i=0; i< 64; i++)	*d++ = *s++;		//Cb bias	- 128
}

//////////////////////////////////////////////////////
///////////////////// IPU DMA ////////////////////////
//////////////////////////////////////////////////////
void fifo_wclear()
{
	memset(fifo_input,0,sizeof(fifo_input));
	g_BP.IFC = 0;
	readwpos = 0;
	writewpos = 0; 
}

int fifo_wread(void *value, int size)
{
	int transsize, firstsize;

	if( g_nDMATransfer&0x80000000 )
		return 0;

	transsize = size;

	// transfer what is left in fifo
	firstsize = min((int)g_BP.IFC, size);
	g_BP.IFC -= firstsize;
	size -= firstsize;

	while( firstsize-- > 0) {
		
		// transfer firstsize qwords, split into two transfers
		((u32*)value)[0] = fifo_input[readwpos];
		((u32*)value)[1] = fifo_input[readwpos+1];
		((u32*)value)[2] = fifo_input[readwpos+2];
		((u32*)value)[3] = fifo_input[readwpos+3];
		readwpos = (readwpos + 4) & 31;
		value = (u32*)value + 4;
	}

	if(size > 0) {

		// fifo is too small, so have to get from DMA
		IPU1dma();

		do {

			while(g_BP.IFC == 0) {
				// wait until ready
				pthread_cond_wait(&g_condIPU1,&g_mtxIPU);
				IPU1dma();

				// abort if ipu is resetting
				if( g_nDMATransfer&0x80000000 ) {
					return 0;
				}
			}

			firstsize = min(size, (int)g_BP.IFC);
			size -= firstsize;
			g_BP.IFC -= firstsize;
			
			while( firstsize-- > 0) {
		
				// transfer firstsize qwords, split into two transfers
				((u32*)value)[0] = fifo_input[readwpos];
				((u32*)value)[1] = fifo_input[readwpos+1];
				((u32*)value)[2] = fifo_input[readwpos+2];
				((u32*)value)[3] = fifo_input[readwpos+3];
				readwpos = (readwpos + 4) & 31;
				value = (u32*)value + 4;
			}

		} while(size > 0);
	}

	return transsize-size;
}

int fifo_wread1(void *value)
{
	if( g_nDMATransfer&0x80000000 )
		return 0;

	if( g_BP.IFC > 0 ) {
		// transfer 1 qword, split into two transfers
		((u32*)value)[0] = fifo_input[readwpos];
		((u32*)value)[1] = fifo_input[readwpos+1];
		((u32*)value)[2] = fifo_input[readwpos+2];
		((u32*)value)[3] = fifo_input[readwpos+3];
		readwpos = (readwpos + 4) & 31;
		g_BP.IFC--;
		return 1;
	}

	// fifo is too small, so have to get from DMA
	IPU1dma();

	while(g_BP.IFC == 0) {
		// wait until ready
		pthread_cond_wait(&g_condIPU1,&g_mtxIPU);
		IPU1dma();

		// abort if ipu is resetting
		if( g_nDMATransfer&0x80000000 ) {
			return 0;
		}
	}

	((u32*)value)[0] = fifo_input[readwpos];
	((u32*)value)[1] = fifo_input[readwpos+1];
	((u32*)value)[2] = fifo_input[readwpos+2];
	((u32*)value)[3] = fifo_input[readwpos+3];

	// finished, fix the fifo and leave
	g_BP.IFC--;
	readwpos = (readwpos+4)&31;

	return 1;
}

int fifo_wwrite_int(u32* pMem, int size)
{
	int transsize;
	int firsttrans = min(size, 8-(int)g_BP.IFC);

	g_BP.IFC+=firsttrans;
	transsize = firsttrans;

	while(transsize-- > 0) {
		fifo_input[writewpos] = pMem[0];
		fifo_input[writewpos+1] = pMem[1];
		fifo_input[writewpos+2] = pMem[2];
		fifo_input[writewpos+3] = pMem[3];
		writewpos = (writewpos+4)&31;
		pMem +=4;
	}

	return firsttrans;
}

int fifo_wwrite(u32* pMem, int size)
{
	size = fifo_wwrite_int(pMem, size);
	return size;
}

#define IPU1chain() \
	while (ipu1dma->qwc) { \
		int ret; \
		pMem = (u32*)dmaGetAddr(ipu1dma->madr); \
		if (pMem == NULL) { return; } \
		ret = fifo_wwrite_int(pMem,ipu1dma->qwc); \
		ipu1dma->madr += ret<< 4; \
		ipu1dma->qwc -= ret; \
		ipu1cycles+= ret<<1; \
		if(ipu1dma->qwc > 0) { return; } \
}

void IPU1dma()
{
	u32 *ptag, *pMem;
	int loop=1;

#ifdef SPR_LOG
	SPR_LOG("dmaIPU1 chcr = %lx, madr = %lx, qwc  = %lx, tadr = %lx\n",
			ipu1dma->chcr, ipu1dma->madr, ipu1dma->qwc, ipu1dma->tadr);
#endif

	assert( !(ipu1dma->chcr&0x40) );

StartIPU1:

	if ((ipu1dma->chcr & 0xc) == 0 || ipu1dma->qwc) { // Normal Mode
		IPU1chain();
		
		// end if in normal mode, or last tag was refe or end
		if( !(ipu1dma->chcr&0xc) ) {
			ipu1dma->chcr &= ~0x100;
			ipu1cycles -= (cpuRegs.cycle-s_nIPU1Start);
			ipu1cycles = max( (int)ipu1cycles, 0 );
			INT(4, ipu1cycles);
			ipu1cycles = 0;
			return;
		}
		else {
			// do end processing
			u32 tag = ipu1dma->chcr; // upper bits describe current tag

			if ((ipu1dma->chcr & 0x80) && (tag&0x80000000)) {
				ptag = (u32*)dmaGetAddr(ipu1dma->tadr);

				switch(tag&0x70000000) {
					case 0x00000000: ipu1dma->tadr += 16; break;
					case 0x70000000: ipu1dma->tadr = ipu1dma->madr; break;
				}

				ipu1dma->chcr = (ipu1dma->chcr & 0xFFFF) | ( (*ptag) & 0xFFFF0000 );
#ifdef IPU_LOG
				IPU_LOG("dmaIrq Set\n"); 
#endif
				ipu1cycles -= (cpuRegs.cycle-s_nIPU1Start);
				ipu1cycles = max( (int)ipu1cycles, 0 );
				INT(4, ipu1cycles);
				ipu1cycles = 0;
				ipu1dma->chcr &= ~0x100;
				return;
			}

			switch( tag&0x70000000 )
			{
			case 0x00000000:
				ipu1dma->chcr &= ~0x100;
				ipu1dma->tadr += 16;
				ipu1cycles -= (cpuRegs.cycle-s_nIPU1Start);
				ipu1cycles = max( (int)ipu1cycles, 0 );
				INT(4, ipu1cycles);
				ipu1cycles = 0;
				return;

			case 0x70000000:
				ipu1dma->chcr &= ~0x100;
				ipu1dma->tadr = ipu1dma->madr;
				ipu1cycles -= (cpuRegs.cycle-s_nIPU1Start);
				ipu1cycles = max( (int)ipu1cycles, 0 );
				INT(4, ipu1cycles);
				ipu1cycles = 0;
				return;
			}
		}
	}

	if (ipu1dma->chcr & 0x100) {	// Chain Mode

		ptag = (u32*)dmaGetAddr(ipu1dma->tadr);  //Set memory pointer to TADR
		if (ptag == NULL) {					 //Is ptag empty?
			psHu32(DMAC_STAT)|= 1<<15;		 //If yes, set BEIS (BUSERR) in DMAC_STAT register
			ipu1dma->chcr &= ~0x100; //?
			return;
		}

		if (ipu1dma->chcr & 0x40) {
			SysPrintf("Warning! IPU1 TTE: %x_%x, Please report to PCSX2 Team\n", ptag[3], ptag[2]);
		}

		ipu1dma->chcr = (ipu1dma->chcr & 0xFFFF) | ( (*ptag) & 0xFFFF0000 );
		ipu1dma->qwc  = (u16)(ptag[0] & 0xffff);
		ipu1cycles += 2;
		assert( !(ipu1dma->chcr&0x40) );

#ifdef IPU_LOG
		IPU_LOG("dmaIPU1 dmaChain %8.8x_%8.8x size=%d, addr=%lx, fifosize=%x\n",
				ptag[1], ptag[0], ipu1dma->qwc, ipu1dma->madr, 8 - g_BP.IFC);
#endif
		switch(ptag[0] & 0x70000000) {
			
			case 0x00000000: // refe
				// do not change tadr
				ipu1dma->madr = ptag[1];
				break;

			case 0x10000000: // cnt
				ipu1dma->madr = ipu1dma->tadr + 16;
				// Set the taddr to the next tag
				ipu1dma->tadr += 16 + (ipu1dma->qwc << 4);
				break;

			case 0x20000000: // next
				ipu1dma->madr = ipu1dma->tadr + 16;
				ipu1dma->tadr = ptag[1];
				break;

			case 0x30000000: // ref
				ipu1dma->madr = ptag[1];
				ipu1dma->tadr += 16;
				break;

			case 0x70000000: // end
				// do not change tadr
				ipu1dma->madr = ipu1dma->tadr + 16;
				break;

			default:
#ifdef IPU_LOG
				IPU_LOG("ERROR: different transfer mode!, Please report to PCSX2 Team\n");
#endif
				break;
		}

		goto StartIPU1; // go back to top
	}
}

void IPU0dma(const void* pFromIPUMem, int size)
{
	u32 *pMem;
	int readsize;

	if( g_nDMATransfer&0x80000000 )
		return;

	//pthread_mutex_lock(&g_mtxIPU0);

#ifdef IPU_LOG
	IPU_LOG("dmaIPU0 chcr = %lx, madr = %lx, qwc  = %lx, ipuout_fifo.count=%x\n",
			ipu0dma->chcr, ipu0dma->madr, ipu0dma->qwc, size);
#endif

	g_nOutputIPUFIFO = size;
	g_pOutputIPUFIFO = pFromIPUMem;

	while(g_nOutputIPUFIFO > 0) {
		if ((ipu0dma->chcr & 0xC) != 0)
			SysMessage("IPU0 out: not a normal transfer!");

		// wait until ready
		while(ipu0dma->qwc == 0 || !(ipu0dma->chcr&0x100) ) {
			pthread_cond_wait(&g_condIPU0,&g_mtxIPU);

			// abort if ipu is resetting
			if( g_nOutputIPUFIFO == 0 || (g_nDMATransfer&0x80000000) ) {
				g_pOutputIPUFIFO = NULL;
				pthread_mutex_unlock(&g_mtxIPU);
				return;
			}
		}

		if( g_nOutputIPUFIFO > 0 ) {
			readsize = min(ipu0dma->qwc, g_nOutputIPUFIFO);
 			//_dmaGetAddr(ipu0dma, pMem, ipu0dma->madr, 3);
			pMem = (u32*)dmaGetAddr(ipu0dma->madr);

			memcpy(pMem, g_pOutputIPUFIFO, readsize<<4);
			pMem += readsize<<2;
			g_pOutputIPUFIFO = (u8*)g_pOutputIPUFIFO + (readsize<<4);
			g_nOutputIPUFIFO -= readsize;
			
			ipu0dma->madr += readsize<< 4;
			ipu0dma->qwc -= readsize; // note: qwc is u16
			ipu0cycles+= readsize<<1; // guessing
		}
		
		if(ipu0dma->qwc > 0) {
			assert(g_nOutputIPUFIFO == 0);
			break; // more to go
		}
		
		if ((psHu32(DMAC_CTRL) & 0x30) == 0x30) { // STS == fromIPU
			psHu32(DMAC_STADR) = ipu0dma->madr;
			switch (psHu32(DMAC_CTRL) & 0xC0) {
				case 0x80: // GIF
					g_nDMATransfer |= 1;
					break;
			}
		}

		ipu0dma->chcr &= ~0x100;

		ipu0cycles -= (cpuRegs.cycle-s_nIPU0Start);
		ipu0cycles = max( (int)ipu0cycles, 0);
		INT(3, ipu0cycles);
		ipu0cycles = 0;
	}

	assert( g_nOutputIPUFIFO == 0 );
	g_pOutputIPUFIFO = NULL;

	//pthread_mutex_unlock(&g_mtxIPU0);
}

void dmaIPU0() // fromIPU
{
	pthread_mutex_lock(&g_mtxIPU);
	pthread_mutex_unlock(&g_mtxIPU);

	s_nIPU0Start = cpuRegs.cycle;
	pthread_cond_signal(&g_condIPU0);

	sched_yield();
}

void dmaIPU1() // toIPU
{
	pthread_mutex_lock(&g_mtxIPU);
	pthread_mutex_unlock(&g_mtxIPU);

	s_nIPU1Start = cpuRegs.cycle;
	pthread_cond_signal(&g_condIPU1);
	
	sched_yield();
}

int ipu0Interrupt() {
#ifdef IPU_LOG 
	IPU_LOG("ipu0Interrupt: %8.8x\n", cpuRegs.cycle);
#endif

	if( g_nDMATransfer & 1 ) {
		// gif
		g_nDMATransfer &= ~1;
		dmaGIF();
	}
					
	ipu0cycles = 0;
	//((DMACh *)&PS2MEM_HW[0xb000])->chcr &= ~0x100;
	hwDmacIrq(DMAC_FROM_IPU);

	return 1;
}

int ipu1Interrupt() {
#ifdef IPU_LOG 
	IPU_LOG("ipu1Interrupt: %8.8x\n", cpuRegs.cycle);
#endif

	ipu1cycles = 0;
	//((DMACh *)&PS2MEM_HW[0xb400])->chcr &= ~0x100;
	hwDmacIrq(DMAC_TO_IPU);

	return 1;
}
