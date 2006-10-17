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

void NewTexture()
{
	tCount++;
	textures = (texture_info*)realloc(textures,(tCount+1)*sizeof(texture_info));
}

void SetLastTexture(s32 ctxt)
{
	textures[tCount].tbp0 = _tex0[ctxt].tbp0;
	textures[tCount].tbw  = _tex0[ctxt].tbw;
	textures[tCount].psm  = _tex0[ctxt].psm;
	textures[tCount].tw   = _tex0[ctxt].tw;
	textures[tCount].th   = _tex0[ctxt].th;
	textures[tCount].tcc  = _tex0[ctxt].tcc;
	textures[tCount].tfx  = _tex0[ctxt].tfx;
	textures[tCount].cbp  = _tex0[ctxt].cbp;
	textures[tCount].cpsm = _tex0[ctxt].cpsm;
	textures[tCount].csm  = _tex0[ctxt].csm;
	textures[tCount].csa  = _tex0[ctxt].csa;
	textures[tCount].cld  = _tex0[ctxt].cld;
}

void AddTexture(texture_info *t)
{
	tCount++;
	textures = (texture_info*)realloc(textures,(tCount+1)*sizeof(texture_info));
	textures[tCount]        = *t;
	textures[tCount].update = 0;
}

void FreeTextures()
{
	if (tCount == -1) return;
	tCount = -1;
	free(textures);
	textures = NULL;
}

s32 IsTexture(s32 ctxt)
{
	s32 i;
	for (i=0; i<=tCount; i++) {
		if (_tex0[ctxt].tbp0 == textures[i].tbp0 && _tex0[ctxt].tbw == textures[i].tbw && _tex0[ctxt].psm == textures[i].psm &&
			_tex0[ctxt].tw == textures[i].tw && _tex0[ctxt].th == textures[i].th && _tex0[ctxt].tcc == textures[i].tcc)
			return textures[i].name;
	}
	return 0;
}

s32 IsTexture2(texture_info *t)
{
	s32 i;
	for (i=0; i<=tCount; i++) {
		if (t->tbp0 == textures[i].tbp0 && t->tbw == textures[i].tbw && t->psm == textures[i].psm &&
		    t->tw == textures[i].tw && t->th == textures[i].th && t->tcc == textures[i].tcc)
			return textures[i].name;
	}
	return 0;
}

s32 NeedUpdateTexture(s32 name)
{
	s32 i;

	for (i=0; i<=tCount; i++) {
		if (name == textures[i].name)
			return textures[i].update;
	}

	return 0;
}

void UpdateTexture(s32 name)
{
	s32 i;

	for (i=0; i<=tCount; i++) {
		if (name == textures[i].name) {
			textures[i].update = 0;
			break;
		}
	}
}

s32 GSprocessTEXA_16(s32 r, s32 g, s32 b, s32 a)
{
	// This return alpha for RGBA16 textures based on TEXA register

	if (a==0) {
		if (texa.aem==0) return texa.ta0;
		else { // texa.aem==1
			if (r==0 && g==0 && b==0) return 0;
			else                      return texa.ta0;
		}
	}

	// else a = 1;
	// texa.aem==0 || texa.aem==1
	return texa.ta1;
}

s32 GSprocessTEXA_24(s32 r, s32 g, s32 b)
{
	// This return alpha for RGB24 textures based on TEXA register

	if (texa.aem) { // texa.aem==1
		if (r==0 && g==0 && b==0) return 0;
		else                      return texa.ta0;
	}

	// else texa.aem==0
	return texa.ta0;
}

s32 GSprocessTexture(s32 ctxt)
{
	u32 *tex = NULL;
	u16 *tex16;
	u32 tbuf_clut[256];
	u32 *ptbuf_clut = tbuf_clut;
	s32 n,i,j,tbp,tbw,h;
	u32 c,a,r,g,b;

	if (n = IsTexture(ctxt)) {
		glBindTexture(GL_TEXTURE_2D, n);
		if (NeedUpdateTexture(n))
			UpdateTexture(n);
		else {
			GSprocessTex1();
			return 0;
		}
	} else {
		NewTexture();
		SetLastTexture(ctxt);
		textures[tCount].update = 0;

		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &textures[tCount].name);
		glBindTexture(GL_TEXTURE_2D, textures[tCount].name);
	}

	h = _tex0[ctxt].th;

	switch (_tex0[ctxt].psm) {

		case 0x01: // PSMCT24
			// ok  = rgb24,test
			// bad = 

			tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
			tbp = _tex0[ctxt].tbp0;

			for (j=0; j<_tex0[ctxt].th; j++) {
				for (i=0; i<_tex0[ctxt].tw; i++) {
					c = vRamUL[tbp+j*_tex0[ctxt].tbw+i];
					a = GSprocessTEXA_24(c&0xff, c&0xff00, c&0xff0000);
					tex[j*_tex0[ctxt].tw+i] = (c&0xffffff)|((a<<24));
				}
			}

			break;

		case 0x02: // PSMCT16
			// Note: ok it's not the best but it work ^^
			// i'll try to understand why i have some differences in monalisa without h/=2 later :)
			// maybe it's an idt bug thought...

			// ok  = monalisa,jum1,soundcheck
			// bad = monalisa [text]

			h /= 2;

			tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
			tbp = _tex0[ctxt].tbp0;

			for (j=0; j<_tex0[ctxt].th; j++) {
				for (i=0; i<_tex0[ctxt].tw; i+=2) {
					c = vRamUL[tbp+j*_tex0[ctxt].tbw+i/2];
					r = c&0x001f;
					g = c&0x03e0;
					b = c&0x7c00;
					a = c&0x8000;
					a = GSprocessTEXA_16(r,g,b,a);
					tex[j*_tex0[ctxt].tw+i  ] = (a<<24)|(b<<9)|(g<<6)|(r<<3);

					c>>=16;

					r = c&0x001f;
					g = c&0x03e0;
					b = c&0x7c00;
					a = c&0x8000;
					a = GSprocessTEXA_16(r,g,b,a);
					tex[j*_tex0[ctxt].tw+i+1] = (a<<24)|(b<<9)|(g<<6)|(r<<3);
				}
			}

			break;

		case 0x13: // PSMT8

			if (_tex0[ctxt].csm == 0x0) { // CSM1
				if (_tex0[ctxt].cpsm == 0) { // PSMCT32
					// ok  = ps2mame,funslower,colors15
					// bad = 

					tex = (u32*)(vRamUL+_tex0[ctxt].cbp);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						tex += 16;
					}
					ptbuf_clut = tbuf_clut+8;
					tex = (u32*)(vRamUL+_tex0[ctxt].cbp+16);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						tex += 16;
					}

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw; i++) {
							tex[j*_tex0[ctxt].tw+i] = tbuf_clut[(*(vRam+tbp+j*_tex0[ctxt].tbw+i))&0xff];
						}
					}

				} else { // PSMCT16 & PSMCT16S
					
					// I must implement TEXA here too

					// ok  =
					// bad =

					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp);
					#define set_ptbuf_clut() { c = *tex16++; \
						*ptbuf_clut++ = ((c&0x8000)<<16)|((c&0x7c00)<<9)|((c&0x03e0)<<6)|((c&0x001f)<<3); }
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}
					ptbuf_clut = tbuf_clut+8;
					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp+16/2);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw; i++) {
							tex[j*_tex0[ctxt].tw+i] = tbuf_clut[*(vRam+tbp+j*_tex0[ctxt].tbw+i)];
						}
					}

				}
			} else { // CSM2 - PSMCT16
				// ok  = psms
				// bad =

				tex16 = (u16*)(vRamUL + _tex0[ctxt].cbp + texclut.cov * texclut.cbw + texclut.cou);
				for (i=0; i<256; i++) {
					u32 c = *tex16++;
					u32 r,g,b,a;
					r = c&0x001f;
					g = c&0x03e0;
					b = c&0x7c00;
					a = c&0x8000;
					a = GSprocessTEXA_16(r,g,b,a);
					*ptbuf_clut++ = (a<<24)|(b<<9)|(g<<6)|(r<<3);
				}

				tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
				tbp = _tex0[ctxt].tbp0 * 4;

				for (j=0; j<_tex0[ctxt].th; j++) {
					for (i=0; i<_tex0[ctxt].tw; i++) {
						tex[j*_tex0[ctxt].tw+i] = tbuf_clut[(*(vRam+tbp+j*_tex0[ctxt].tbw+i))&0xff];
					}
				}

			}
			break;

		case 0x14: // PSMT4

			if (_tex0[ctxt].csm == 0x0) { // CSM1
				if (_tex0[ctxt].cpsm == 0) { // PSMCT32
					// ok  = soundcheck, ps2mame
					// bad =

					tex = (u32*)(vRamUL + _tex0[ctxt].cbp);
					for (i=0; i<16; i++)
						*ptbuf_clut++ = *tex++;

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;
					tbw = _tex0[ctxt].tbw  / 2;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw/2; i++) {
							u32 color = *(vRam+tbp+j*tbw+i);
							tex[j*_tex0[ctxt].tw+2*i]   = tbuf_clut[(color&0xf)];
							tex[j*_tex0[ctxt].tw+2*i+1] = tbuf_clut[((color>>4)&0xf)];
						}
					}

				} else { // PSMCT16 & PSMCT16S
					
					// I must implement TEXA here too.

					// ok  =
					// bad =

					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp);
					#define set_ptbuf_clut() { c = *tex16++; \
						*ptbuf_clut++ = ((c&0x8000)<<16)|((c&0x7c00)<<9)|((c&0x03e0)<<6)|((c&0x001f)<<3); }
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}
					ptbuf_clut = tbuf_clut+8;
					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp+16/2);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;
					tbw = _tex0[ctxt].tbw  / 2;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw/2; i++) {
							c = *(vRam+tbp+j*tbw+i);
							tex[j*_tex0[ctxt].tw+2*i]   = tbuf_clut[(c&0xf)];
							tex[j*_tex0[ctxt].tw+2*i+1] = tbuf_clut[((c>>4)&0xf)];
						}
					}

				}
			} else { // CSM2 - PSMCT16

				// I must implement TEXA here too.

				// ok  =
				// bad =

				tex16 = (u16*)(vRamUL + _tex0[ctxt].cbp + texclut.cov * texclut.cbw + texclut.cou);
				for (i=0; i<256; i++) {
					c = *tex16++;
					*ptbuf_clut++ = ((c&0x8000)<<16)|((c&0x7c00)<<9)|((c&0x03e0)<<6)|((c&0x001f)<<3);
				}

				tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
				tbp = _tex0[ctxt].tbp0 * 4;
				tbw = _tex0[ctxt].tbw  / 2;

				for (j=0; j<_tex0[ctxt].th; j++) {
					for (i=0; i<_tex0[ctxt].tw/2; i++) {
						c = *(vRam+tbp+j*tbw+i);
						tex[j*_tex0[ctxt].tw+2*i]   = tbuf_clut[(c&0xf)];
						tex[j*_tex0[ctxt].tw+2*i+1] = tbuf_clut[((c>>4)&0xf)];
					}
				}

			}
			break;

		case 0x1B: // PSMT8H
			// 31->24 TEX8 23->0 NOTHING

			if (_tex0[ctxt].csm == 0x0) { // CSM1
				if (_tex0[ctxt].cpsm == 0x0) { // PSMCT32
					// ok  =
					// bad =

					tex = (u32*)(vRamUL+_tex0[ctxt].cbp);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						tex += 16;
					}
					ptbuf_clut = tbuf_clut+8;
					tex = (u32*)(vRamUL+_tex0[ctxt].cbp+16);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						for (i=0; i<8; i++) *ptbuf_clut++ = *tex++;
						ptbuf_clut += 8;
						tex += 16;
					}

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw; i++) {
							tex[j*_tex0[ctxt].tw+i] = tbuf_clut[((*(vRam+tbp+j*_tex0[ctxt].tbw+i*4))>>24)&0xff];
						}
					}

				} else { // PSMCT16 & PSMCT16S

					// I must implement TEXA here too.

					// ok  =
					// bad =

					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp);
					#define set_ptbuf_clut() { c = *tex16++; \
						*ptbuf_clut++ = ((c&0x8000)<<16)|((c&0x7c00)<<9)|((c&0x03e0)<<6)|((c&0x001f)<<3); }
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}
					ptbuf_clut = tbuf_clut+8;
					tex16 = (u16*)(vRamUL+_tex0[ctxt].cbp+16/2);
					for (j=0; j<8; j++) {
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						for (i=0; i<8; i++) set_ptbuf_clut();
						ptbuf_clut += 8;
						tex16 += 16;
					}

					tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
					tbp = _tex0[ctxt].tbp0 * 4;

					for (j=0; j<_tex0[ctxt].th; j++) {
						for (i=0; i<_tex0[ctxt].tw; i++) {
							tex[j*_tex0[ctxt].tw+i] = tbuf_clut[((*(vRam+tbp+j*_tex0[ctxt].tbw+i*4))>>24)&0xff];
						}
					}

				}
			} else { // CSM2 - PSMCT16

				// I must implement TEXA here too.

				// ok  =
				// bad =

				tex16 = (u16*)(vRamUL + _tex0[ctxt].cbp + texclut.cov * texclut.cbw + texclut.cou);
				for (i=0; i<256; i++) {
					c = *tex16++;
					*ptbuf_clut++ = ((c&0x8000)<<16)|((c&0x7c00)<<9)|((c&0x03e0)<<6)|((c&0x001f)<<3);
				}

				tex = (u32*)malloc(_tex0[ctxt].tw*_tex0[ctxt].th*sizeof(u32));
				tbp = _tex0[ctxt].tbp0 * 4;

				for (j=0; j<_tex0[ctxt].th; j++) {
					for (i=0; i<_tex0[ctxt].tw; i++) {
						tex[j*_tex0[ctxt].tw+i] = tbuf_clut[((*(vRam+tbp+j*_tex0[ctxt].tbw+i*4))>>24)&0xff];
					}
				}

			}

		default:
			tex = &vRamUL[_tex0[ctxt].tbp0];
			glPixelStorei(GL_UNPACK_ROW_LENGTH, _tex0[ctxt].tbw);
			break;
	}

	if (!tex) return 0;

	GSprocessTex1();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, _tex0[ctxt].tw, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);

	if (_tex0[ctxt].psm >= 0x13 || _tex0[ctxt].psm == 0x01 || _tex0[ctxt].psm == 0x02) free(tex);

	if (glGetError()) return 0;

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	return 1;
}

void GSprocessMipmap(s32 ctxt)
{
}

void GSprocessTex1()
{
	s32 param;

	if (config.video.filtering) {

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	} else {

		switch (tex1->mmag) {
			default:
			case 0: param = GL_NEAREST; break; // 000 NEAREST
			case 1: param = GL_LINEAR;  break; // 001 LINEAR
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, param);

		switch (tex1->mmin) {
			default:
			case 0: param = GL_NEAREST; break; // 000 NEAREST
			case 1: param = GL_LINEAR;  break; // 001 LINEAR
//			case 2: param = GL_NEAREST_MIPMAP_NEAREST; break; // 010 NEAREST_MIPMAP_NEAREST
//			case 3: param = GL_NEAREST_MIPMAP_LINEAR;  break; // 011 NEAREST_MIPMAP_LINEAR
//			case 4: param = GL_LINEAR_MIPMAP_NEAREST;  break; // 100 LINEAR_MIPMAP_NEAREST
//			case 5: param = GL_LINEAR_MIPMAP_LINEAR;   break; // 101 LINEAR_MIPMAP_LINEAR
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, param);

	}

}

void ShowTexture()
{
	s32 u; if (config.itf.menu_visible) u=0; else u=1;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(0,14*(1-u)-u,0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D,textures[config.itf.texture_view].name);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2i(0,u);
		glTexCoord2f(1,0); glVertex2i(textures[config.itf.texture_view].tw,u);
		glTexCoord2f(1,1); glVertex2i(textures[config.itf.texture_view].tw,textures[config.itf.texture_view].th+u);
		glTexCoord2f(0,1); glVertex2i(0,textures[config.itf.texture_view].th+u);
	glEnd();
	if (!prim->tme) glDisable(GL_TEXTURE_2D);
	if (prim->abe) glEnable(GL_BLEND);
	if (test->ate) glEnable(GL_ALPHA_TEST);
	GSsetDepthTest();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}