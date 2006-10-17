/******************************************************************************
    GStaris - GS plugin for PS2 emulators < gstaris.ngemu.com        >
    Copyright (C) 2003 Absolute0          < absolutezero@ifrance.com >

    This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/
// 1/8/2003 : modificate for 0.4.0 specifications (shadow)

#include "GStaris.h"

// GSgifTransfer
// 'normal transfer' to GS

void GIFreadTag(u32 *pMem)
{
	giftag.nloop = (pMem[0])&0x7fff;
	giftag.eop   = (pMem[0]>>15)&0x1;
	giftag.pre   = (pMem[1]>>14)&0x1;
	giftag.prim  = (pMem[1]>>15)&0x7ff;
	giftag.flg   = (pMem[1]>>26)&0x3;
	giftag.nreg  = (pMem[1]>>28)&0xf; if (giftag.nreg == 0) giftag.nreg = 16;
	giftag.regs  = *(u64 *)(pMem+2);
	gifmode      = (/*(giftag.flg == 0x3) ? 3 :*/ giftag.flg+1);

	if (gifmode >= 3 && trxdir == 2)
		IDTmove();

	if (config.log.gif)
		GSlog("gifmode=%x giftag[nloop=%x eop=%x pre=%x prim=%x flg=%x nreg=%x regs=%x_%x]\r\n",
			gifmode,giftag.nloop,giftag.eop,giftag.pre,giftag.prim,giftag.flg,giftag.nreg,((u32*)&giftag.regs+1),((u32*)&giftag.regs+1));
}

u64   packetdata;
u32 *ppacketdata = (u32 *)&packetdata;

void GIFpacket(s32 reg, u32 *pMem)
{
	if (config.log.gif)
		GSlog("GIFpacket reg=%x pMem=%x_%x\n",reg,pMem[1],pMem[0]);

	switch (reg) {
		case 0x00: // PRIM
			GSwrite(0x00,pMem);
			break;
		case 0x01: // RGBAQ
			ppacketdata[0] = (pMem[0]&0xff)|((pMem[1]&0xff)<<8)|((pMem[2]&0xff)<<16)|((pMem[3]&0xff)<<24);
			ppacketdata[1] = vertex[vCount].q;
			GSwrite(0x01,ppacketdata);
			break;
		case 0x02: // ST
			GSwrite(0x02,pMem);
			ppacketdata[0] = rgba[vCount].rgba;
			ppacketdata[1] = pMem[2];
			GSwrite(0x01,ppacketdata);
			break;
		case 0x03: // UV
			ppacketdata[0] = (pMem[0]&0x7fff)|((pMem[1]&0x7fff)<<16);
			GSwrite(0x03,ppacketdata);
			break;
		case 0x04: // XYZF2
			ppacketdata[0] = (pMem[0]&0xffff)|((pMem[1]&0xffff)<<16);
			ppacketdata[1] = ((pMem[2]>>4)&0xffffff)|((pMem[3]&0xff00)<<16);
			if ((pMem[3]>>15)&0x1)
				GSwrite(0x0C,ppacketdata);
			else GSwrite(0x04,ppacketdata);
			break;
		case 0x05: // XYZ2
			ppacketdata[0] = (pMem[0]&0xffff)|((pMem[1]&0xffff)<<16);
			ppacketdata[1] = pMem[2];
			if ((pMem[3]>>15)&0x1)
				GSwrite(0x0D,ppacketdata);
			else GSwrite(0x05,ppacketdata);
			break;
		case 0x0A: // FOG
			ppacketdata[0] = 0;
			ppacketdata[1] = (((pMem[3]>>4)&0xff)<<24);
			GSwrite(0x0A,ppacketdata);
			break;
		case 0x0E: // A+D
//			if (config.log.gif)
//				GSlog("A+D=%x\n",pMem[2]&0xff);
			GSwrite(pMem[2]&0xff,pMem);
			break;
//		case 0x0F: // NOP
//			break;
	}
}

s32 reg,nreg;

void CALLBACK GSgifTransfer1(u32 *pMem)
{

	if (config.log.gif) GSlog("GSgifTransfer2 ¤ ");

	giftag.eop = 0;
	
	for (;;) {
		switch (gifmode) {
			case 0: // GIF_TAG
				if (giftag.eop == 1) {
					giftag.eop = 0;
					return;
				}
				GIFreadTag(pMem); pMem+=4;
				break;

			case 1: // GIF_PACKET
				if (giftag.nloop == 0) { gifmode = 0; break; }
				if (giftag.pre) GSwrite(0x00, (u32*)&giftag.prim);
				nreg = 0;
				for (;;) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					GIFpacket(reg,pMem); pMem+=4;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0)
							break;
					}
				}
				gifmode = 0;
				break;

			case 2: // GIF_REGLIST
				if (giftag.nloop == 0) { gifmode = 0; break; }
				nreg = 0;
				for (;;) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem+=2;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0) {
							pMem+=2;
							break;
						}
					}

					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem += 2;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0)
							break;
					}
				}
				gifmode = 0;
				break;

			case 3: // GIF_IMAGE
			case 4:
				if (trxdir == 0) { // host->local
					if (giftag.nloop == 0) { gifmode = 0; break; }

					if ((last_bitbltbuf_psm == 0x13 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14) ||
						(last_bitbltbuf_psm == 0x14 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14)) {
						s32 i = 0;
						for (;;) {
							vRamUL[bitbltbuf.dbp+i+0] = pMem[0];
							vRamUL[bitbltbuf.dbp+i+1] = pMem[1];
							vRamUL[bitbltbuf.dbp+i+2] = pMem[2];
							vRamUL[bitbltbuf.dbp+i+3] = pMem[3];
							i += 4;
							pMem += 4;
							if (--giftag.nloop == 0) break;
						}

					} else {

						IDTwriteBegin();
						for (;;) {
							IDTwrite(pMem); pMem+=4;
							if (--giftag.nloop == 0)
								break;
						}
						IDTwriteEnd();
					}
				}
				gifmode = 0;
				break;
		}
	}

}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	if (config.log.gif) GSlog("GSgifTransfer ¤ ");

	while (size > 0) {
		switch (gifmode) {
			case 0: // tag
				GIFreadTag(pMem); pMem+=4; size--;
				break;

			case 1: // packet
				if (giftag.pre) GSwrite(0x00, (u32*)&giftag.prim);
				if (giftag.nloop == 0) {
					gifmode = 0;
					break;
				}
				nreg = 0;
				while (size) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					GIFpacket(reg,pMem); pMem += 4; size--;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0)
							break;
					}
				}
				gifmode = 0;
				break;

			case 2: // reglist
				if (giftag.nloop == 0) { gifmode = 0; return; }
				nreg = 0;
				while (size) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem += 2;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0) {
							pMem += 2; size--;
							break;
						}
					}

					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem += 2; size--;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0) 
							break;
					}
				}
				gifmode = 0;
				break;

			case 3: // image
			case 4:
				if (trxdir == 0) { // host->local
					if (giftag.nloop == 0) { gifmode = 0; break; }

					if ((last_bitbltbuf_psm == 0x13 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14) ||
						(last_bitbltbuf_psm == 0x14 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14)) {
						s32 i = 0;
						while (size > 0) {
							vRamUL[bitbltbuf.dbp+i+0] = /*0;//*/pMem[0];
							vRamUL[bitbltbuf.dbp+i+1] = /*0;//*/pMem[1];
							vRamUL[bitbltbuf.dbp+i+2] = /*0;//*/pMem[2];
							vRamUL[bitbltbuf.dbp+i+3] = /*0;//*/pMem[3];
							i += 4;
							pMem += 4;
							size --;
							if (--giftag.nloop == 0) break;
						}
					} else {
						IDTwriteBegin();
						while (size) {
							IDTwrite(pMem); pMem += 4; size--;
							if (--giftag.nloop == 0)
								break;
						}
						IDTwriteEnd();
					}
				}
				gifmode = 0;
				break;
		}
	}
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	if (config.log.gif) GSlog("GSgifTransfer ¤ ");

	while (size > 0) {
		switch (gifmode) {
			case 0: // tag
				GIFreadTag(pMem); pMem+=4; size--;
				break;

			case 1: // packet
				if (giftag.pre) GSwrite(0x00, (u32*)&giftag.prim);
				if (giftag.nloop == 0) {
					gifmode = 0;
					break;
				}
				nreg = 0;
				while (size) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					GIFpacket(reg,pMem); pMem += 4; size--;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0)
							break;
					}
				}
				gifmode = 0;
				break;

			case 2: // reglist
				if (giftag.nloop == 0) { gifmode = 0; return; }
				nreg = 0;
				while (size) {
					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem += 2;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0) {
							pMem += 2; size--;
							break;
						}
					}

					reg = (s32)((giftag.regs>>(nreg*4))&0xf);
					if (reg < 0xe) GSwrite(reg,pMem);
					pMem += 2; size--;
					if (++nreg == giftag.nreg) {
						nreg = 0;
						if (--giftag.nloop == 0) 
							break;
					}
				}
				gifmode = 0;
				break;

			case 3: // image
			case 4:
				if (trxdir == 0) { // host->local
					if (giftag.nloop == 0) { gifmode = 0; break; }

					if ((last_bitbltbuf_psm == 0x13 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14) ||
						(last_bitbltbuf_psm == 0x14 && bitbltbuf.dpsm != 0x13 && bitbltbuf.dpsm != 0x14)) {
						s32 i = 0;
						while (size > 0) {
							vRamUL[bitbltbuf.dbp+i+0] = /*0;//*/pMem[0];
							vRamUL[bitbltbuf.dbp+i+1] = /*0;//*/pMem[1];
							vRamUL[bitbltbuf.dbp+i+2] = /*0;//*/pMem[2];
							vRamUL[bitbltbuf.dbp+i+3] = /*0;//*/pMem[3];
							i += 4;
							pMem += 4;
							size --;
							if (--giftag.nloop == 0) break;
						}
					} else {
						IDTwriteBegin();
						while (size) {
							IDTwrite(pMem); pMem += 4; size--;
							if (--giftag.nloop == 0)
								break;
						}
						IDTwriteEnd();
					}
				}
				gifmode = 0;
				break;
		}
	}
}

