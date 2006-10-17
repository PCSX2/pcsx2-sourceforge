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

#include "GStaris.h"

// update checking funcs

void IDTcheckTextureUpdate() {
	s32 i;
	s32 dbp = bitbltbuf.dbp+bitbltbuf.dbw*trxpos.dsay;

	for (i=0; i<=tCount; i++) {
		if ((dbp >= textures[i].tbp0 && dbp < textures[i].tbp0+textures[i].th*textures[i].tbw) || // texture part
			((textures[i].psm == 0x13 || textures[i].psm == 0x1B) && dbp >= textures[i].cbp && dbp < textures[i].cbp + 0x400) || // CLUT8 part
			((textures[i].psm == 0x14 || textures[i].psm == 0x24 || textures[i].psm == 0x2C) && dbp >= textures[i].cbp && dbp < textures[i].cbp + 0x40)) // CLUT4 part
			textures[i].update = 1;

		if (config.log.idt) {
			GSlog("textures[%d].update = 1\n",i);
		}
	}
}

// writePixel funcs

void (*writePixel)(u32 pixel);

void wNoPixel(u32 pixel)
{
}

void wPixel(u32 pixel)
{
	if (imgx_ < trxreg.rrw && imgy_ < trxreg.rrh)
		image[imgx_+imgy_*trxreg.rrw] = pixel;
	if (++imgx_ == trxreg.rrw) {
		imgx_ = 0;
		if (++imgy_ == trxreg.rrh)
			imgy_ = 0;
	}
}

void wPixelvRam(u32 pixel)
{
	if (imgx >= 0 && imgx < bitbltbuf.dbw && imgy >= 0)
		image[imgx+imgy*bitbltbuf.dbw] = pixel;
	if (++imgx == trxreg.rrw+trxpos.dsax) {
		imgx = trxpos.dsax;
		if (++imgy == trxreg.rrh+trxpos.dsay)
			imgy = trxpos.dsay;
	}
}

// IDTwrite

int onscreen = 0;
int canwrite = 1;

void IDTwriteBegin()
{
	int bx1  = dispfb->dbx;
	int bx2  = bx1+display->w;
	int by1  = dispfb->fbp+dispfb->fbw*dispfb->dby;
	int by2  = by1+dispfb->fbw*display->h;

	int dptr = bitbltbuf.dbp+bitbltbuf.dbw*trxpos.dsay;

	if (trxpos.dsax >= bx1 && trxpos.dsax < bx2 && dptr >= by1 && dptr < by2) onscreen = 1;
	else                                                                      onscreen = 0;

	if (config.fixes.disable_idt_onscreen)
		onscreen = 0;

	if (onscreen || (bitbltbuf.dpsm == 0x1 && last_bitbltbuf_psm < 0x13)) {
		if (!dirc) {
			canwrite = 1;
			return;
		}
		dirc = 0;

		if (trxreg.rrw == 0 || trxreg.rrh == 0) {
			canwrite = 0;
			writePixel = wNoPixel;
			return;
		}

		image = (u32*)malloc(trxreg.rrw*trxreg.rrh*sizeof(u32));
		if (!image) {
			canwrite = 0;
			writePixel = wNoPixel;
			return;
		}

		canwrite = 1;
		writePixel = wPixel;
	} else { 
		if (!dirc) free(image);
		image = (u32*)(vRamUL+bitbltbuf.dbp);
		canwrite = 1;
		writePixel = wPixelvRam;
	}
}

void IDTwriteEnd()
{
	if (!canwrite) return;

	if (onscreen) {
		float x,y;

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);

		switch (config.video.ratio) {
			case 0: // 1:1
				x = y = 1.f;
				break;

			case 1: // stretched to full window size
				x = (float)config.video.width /(float)(display->w);
				y = (float)config.video.height/(float)(display->h);
				break;

			case 2: // stretched but keep the ps2 ratio
				x = (float)config.video.width/(float)display->w;
				y = (float)config.video.height/(float)display->h;

				if (x > y) {
					x = y;
					if (smode2.intm && smode2.ffmd)
						y *= 2.f;
				} else {
					y = x;
					if (smode2.intm && smode2.ffmd)
						y *= 2.f;
				}
				break;
		}

		// xmas (little and bad) FIX
//		if (trxpos.dsay >= display->h) trxpos.dsay -= display->h;

		glPixelZoom(x,-y);
		glRasterPos2i(trxpos.dsax,trxpos.dsay);
		glDrawPixels(trxreg.rrw,trxreg.rrh,GL_RGBA,GL_UNSIGNED_BYTE,image);
		glPixelZoom(1,1);

		GSsetAlphaTest();
		GSsetDepthTest();

	} else if (bitbltbuf.dpsm == 0x1 && last_bitbltbuf_psm < 0x13) {

		int x,y;
		char *cptr = (char*)image;
		int  *iptr = image;
		int  *fbuf = (vRamUL+bitbltbuf.dbp);

		for (y=0; y<trxreg.rrh; y++) {
			for (x=0; x<trxreg.rrw; x++) {
				fbuf[(x+trxpos.dsax)+(y+trxpos.dsay)*bitbltbuf.dbw] = ((*iptr)&0xffffff) | ((0xff) << 24);
				cptr += 3;
				iptr = (int*)cptr;
			}
		}

		IDTcheckTextureUpdate();

	} else IDTcheckTextureUpdate();

	canwrite = 0;
}

void IDTwrite(u32* pMem)
{
	writePixel(pMem[0]);
	writePixel(pMem[1]);
	writePixel(pMem[2]);
	writePixel(pMem[3]);
}

// IDTmove

void IDTmove_onscreen_to_onscreen() {

	// Err... this code don't work in fullscreen mode ^^

	float x,y;

	switch (config.video.ratio) {
		case 0: // 1:1
			x = y = 1.f;
			break;

		case 1: // stretched to full window size
			x = (float)config.video.width /(float)(display->w);
			y = (float)config.video.height/(float)(display->h);
			break;

		case 2: // stretched but keep the ps2 ratio
			x = (float)config.video.width/(float)display->w;
			y = (float)config.video.height/(float)display->h;

			if (x > y) {
				x = y;
				if (smode2.intm && smode2.ffmd)
					y *= 2.f;
			} else {
				y = x;
				if (smode2.intm && smode2.ffmd)
					y *= 2.f;
			}
			break;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	glRasterPos2i((float)(trxpos.dsax)*x,(float)(trxpos.dsay+trxreg.rrh)*y);
	glCopyPixels((float)(trxpos.ssax)*x,config.video.height-(float)(trxpos.ssay+trxreg.rrh)*y,(float)(trxreg.rrw)*x,(float)(trxreg.rrh)*y,GL_COLOR);

	GSsetAlphaTest();
	GSsetDepthTest();
}

void IDTmove_offscreen_to_onscreen() {
	s32 sx = trxpos.ssax;
	s32 sy = trxpos.ssay;
	s32 dx = trxpos.dsax;
	s32 dy = trxpos.dsay;
	s32 sw,sh,dw,dh;
	s32 x,y;
	u32 *src;
	float zx,zy;

	u32 *t = (unsigned long*)malloc(trxreg.rrw*trxreg.rrh*sizeof(unsigned long));
	if (!t) return;

	sw = dw = trxreg.rrw;
	sh = dh = trxreg.rrh;

	src = (unsigned long *)(vRamUL+bitbltbuf.sbp);

	if (sx < dx) {
		for (y=0;y<sh;y++)
			for (x=sw-1;x>=0;x--)
				t[(y)*trxreg.rrw+x] = src[(sy+y)*bitbltbuf.sbw+sx+x];
	} else {
		for (y=0; y<sh; y++)
			for (x=0; x<sw; x++)
				t[(y)*trxreg.rrw+x] = src[(sy+y)*bitbltbuf.sbw+sx+x];
	}

	switch (config.video.ratio) {
		case 0: // 1:1
			zx = zy = 1.f;
			break;

		case 1: // stretched to full window size
			zx = (float)config.video.width /(float)(display->w);
			zy = (float)config.video.height/(float)(display->h);
			break;

		case 2: // stretched but keep the ps2 ratio
			zx = (float)config.video.width/(float)display->w;
			zy = (float)config.video.height/(float)display->h;

			if (zx > zy) {
				zx = zy;
				if (smode2.intm && smode2.ffmd)
					zy *= 2.f;
			} else {
				zy = zx;
				if (smode2.intm && smode2.ffmd)
					zy *= 2.f;
			}
			break;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);

	glPixelZoom(zx,-zy);
	glRasterPos2i(trxpos.dsax,trxpos.dsay);
	glDrawPixels(trxreg.rrw,trxreg.rrh,GL_RGBA,GL_UNSIGNED_BYTE,t);
	glPixelZoom(1,1);

	free(t);

	GSsetAlphaTest();
	GSsetDepthTest();
}

void IDTmove()
{
	u32 dir;
	s32 bx1,bx2,by1,by2,sptr,dptr,d,s;

	if (bitbltbuf.dpsm != bitbltbuf.spsm) return;

	// dir variable HELP
	// dir = 0 : no transfer
	// dir = 1 : transfer onscreen  to onscreen
	// dir = 2 : transfer offscreen to onscreen

	dir = 0;

	bx1  = dispfb->dbx;
	bx2  = bx1+display->w;
	by1  = dispfb->fbp+dispfb->fbw*dispfb->dby;
	by2  = by1+dispfb->fbw*display->h;

	sptr = bitbltbuf.sbp+bitbltbuf.sbw*trxpos.ssay;
	dptr = bitbltbuf.dbp+bitbltbuf.dbw*trxpos.dsay;

	// d,s variable HELP
	// _ = 0 : offscreen
	// _ = 1 : onscreen

	d=0;
	s=0;

	if (trxpos.ssax >= bx1 && trxpos.ssax < bx2 && sptr >= by1 && sptr < by2) s = 1;
	else                                                                      s = 0;
	if (trxpos.dsax >= bx1 && trxpos.dsax < bx2 && dptr >= by1 && dptr < by2) d = 1;
	else                                                                      d = 0;

	     if (s == 1 && d == 1) dir = 1;
	else if (s == 0 && d == 1) dir = 2;
	else                       dir = 0;

	switch (dir) {
		case 0:
			break;
		case 1: // onscreen to onscreen
			IDTmove_onscreen_to_onscreen();
			break;
		case 2: // offscreen to onscreen
			IDTmove_offscreen_to_onscreen();
			break;
	}
}