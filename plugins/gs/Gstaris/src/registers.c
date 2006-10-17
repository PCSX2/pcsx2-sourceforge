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
#include <gl/glext.h>

// Blending

#define SET_FIX() glBlendColor(1.f,1.f,1.f,(float)(alpha->fix)/255.f)

void GSsetBlend()
{
//	A: Cs,Cd,0
//	B: Cs,Cd,0
//	C: As,Ad,FIX
//	D: Cs,Cd,0

//	Output Color = (A - B) * C + D

	if (config.fixes.disable_blend) {

		glDisable(GL_BLEND);

	} else {

		if (prim->abe) glEnable(GL_BLEND); 
		else           glDisable(GL_BLEND);

		// default blend func
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		switch (alpha->a) {
			case 0:
				switch (alpha->b) {
					case 0:
						// a-b = 0
						switch (alpha->d) {
							case 0:
								// Cs
								glBlendFunc(GL_ONE,GL_ZERO);
								break;
							case 1:
								// Cd
								glBlendFunc(GL_ZERO,GL_ONE);
								break;
							case 2:
								// 0
								glBlendFunc(GL_ZERO,GL_ZERO);
								break;
						}
						break;
					case 1:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0: // ?
										// Cs(As+1) - Cd*As
										glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
									case 1: // ps2mame,psms,flatline
										// Cs*As + Cd(1-As)
										glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
//										glBlendFunc(GL_SRC_ALPHA,GL_ZERO); //dst=GL_ZERO
										break;
									case 2:
										// Cs*As - Cd*As
										glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
								}
								break;
							case 1:
								switch (alpha->d) {
									case 0: // ?
										// Cs(Ad+1) - Cd*Ad
										glBlendFunc(GL_DST_ALPHA,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 1:
										// Cs*Ad + Cd(1-Ad)
										glBlendFunc(GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA);
										break;
									case 2:
										// Cs*Ad - Cd*Ad
										glBlendFunc(GL_DST_ALPHA,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
								}
								break;
							case 2:
/*								switch (alpha->d) {
									case 0: // ?
										// Cs(FIX+1) - Cd*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 1: // ? boredom,flatline
										// Cs*FIX - Cd*(FIX+1)
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 2:
										// Cs*FIX - Cd*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
								}
*/								// Since i don't do 3 differents things i don't need to check alpha->d
								// But the real formulas are differents ...
								SET_FIX();
								glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
								glBlendEquation(GL_FUNC_SUBTRACT);
								break;
						}
						break;
					case 2:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0: // ?
										// Cs*(As+1)
										glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
										break;
									case 1: // ps2flight,adresd_round7,ifs,plasmadreams
										// Cs*As + Cd
										glBlendFunc(GL_SRC_ALPHA,GL_ONE);
										break;
									case 2:
										// Cs*As
										glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
										break;
								}
								break;
							case 1:
								switch (alpha->d) {
									case 0: // ?
										// Cs*(Ad+1)
										glBlendFunc(GL_DST_ALPHA,GL_ZERO);
										break;
									case 1:
										// Cs*Ad + Cd
										glBlendFunc(GL_DST_ALPHA,GL_ONE);
										break;
									case 2:
										// Cs*Ad
										glBlendFunc(GL_DST_ALPHA,GL_ZERO);
										break;
								}
								break;
							case 2:
								switch (alpha->d) {
									case 0: // ?
										// Cs*(FIX+1)
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_ZERO);
										break;
									case 1:
										// Cs*FIX + Cd
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE);
										break;
									case 2:
										// Cs*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_ZERO);
										break;
								}
								break;
						}
						break;
				}
				break;
			case 1: // I'M HERE !!! MUST CONTINUE WRITING THIS FUNC !!!
				switch (alpha->b) {
					case 0:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0:
										// Cs(1-As) + Cd*As
										glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA);
										break;
									case 1: // ?
									case 2:
										// Cd(As+1) - Cs*As
										glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
/*									case 2:
										// Cd*As - Cs*As
										glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
*/								}
								break;
							case 1:
								switch (alpha->d) {
									case 0:
										// Cs(1-Ad) + Cd*Ad
										glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);
										break;
									case 1: // ?
									case 2:
										// Cd(Ad+1) - Cs*Ad
										glBlendFunc(GL_DST_ALPHA,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
/*									case 2:
										// Cd*Ad - Cs*Ad
										glBlendFunc(GL_DST_ALPHA,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
*/								}
								break;
							case 2:
								switch (alpha->d) {
									case 0:
										// Cs(1-FIX) + Cd*FIX
										SET_FIX();
										glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										break;
									case 1: // ?
									case 2:
										// Cd(FIX+1) - Cs*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
/*									case 2:
										// Cd*FIX - Cs*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
*/								}
								break;
						}
						break;
					case 1:
						// a-b = 0
						switch (alpha->d) {
							case 0:
								// Cs
								glBlendFunc(GL_ONE,GL_ZERO);
								break;
							case 1:
								// Cd
								glBlendFunc(GL_ZERO,GL_ONE);
								break;
							case 2:
								// 0
								glBlendFunc(GL_ZERO,GL_ZERO);
								break;
						}
						break;
					case 2:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0:
										// Cs + Cd*As
										glBlendFunc(GL_ONE,GL_SRC_ALPHA);
										break;
									case 1: // ?
										// Cd(As+1)
										glBlendFunc(GL_ZERO,GL_SRC_ALPHA);
										break;
									case 2:
										// Cd*As
										glBlendFunc(GL_ZERO,GL_SRC_ALPHA);
										break;
								}
								break;
							case 1:
								switch (alpha->d) {
									case 0:
										// Cs + Cd*Ad
										glBlendFunc(GL_ONE,GL_DST_ALPHA);
										break;
									case 1: // ?
										// Cd(Ad+1)
										glBlendFunc(GL_ZERO,GL_DST_ALPHA);
										break;
									case 2:
										// Cd*Ad
										glBlendFunc(GL_ZERO,GL_DST_ALPHA);
										break;
								}
								break;
							case 2:
								switch (alpha->d) {
									case 0: // 1fx
										// Cs + Cd*FIX
										glBlendColor(1,1,1,(float)(alpha->fix)/255.f);
										glBlendFunc(GL_ONE,GL_CONSTANT_ALPHA);
										break;
									case 1: // ?
										// Cd*(FIX+1)
										glBlendColor(1,1,1,(float)(alpha->fix)/255.f);
										glBlendFunc(GL_ZERO,GL_CONSTANT_ALPHA);
										break;
									case 2: // boredom,flatline
										// Cd*FIX
										glBlendColor(1,1,1,(float)(alpha->fix)/255.f);
										glBlendFunc(GL_ZERO,GL_CONSTANT_ALPHA);
										break;
								}
								break;
						}
						break;
				}
				break;
			case 2:
				switch (alpha->b) {
					case 0:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0:
										// Cs(1-As)
										glBlendFunc(GL_ONE_MINUS_SRC_ALPHA,GL_ZERO);
										break;
									case 1:
										// Cd - Cs*As
										glBlendFunc(GL_SRC_ALPHA,GL_ONE);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
									case 2:
										// -Cs*As
										glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
								}
								break;
							case 1:
								switch (alpha->d) {
									case 0:
										// Cs(1-Ad)
										glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_ZERO);
										break;
									case 1:
										// Cd - Cs*Ad
										glBlendFunc(GL_DST_ALPHA,GL_ONE);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
									case 2:
										// -Cs*Ad
										glBlendFunc(GL_DST_ALPHA,GL_ZERO);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
								}
								break;
							case 2:
								switch (alpha->d) {
									case 0:
										// Cs(1-FIX)
										SET_FIX();
										glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA,GL_ZERO);
										break;
									case 1:
										// Cd - Cs*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
									case 2:
										// -Cs*FIX
										SET_FIX();
										glBlendFunc(GL_CONSTANT_ALPHA,GL_ZERO);
										glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
										break;
								}
								break;
						}
						break;
					case 1:
						switch (alpha->c) {
							case 0:
								switch (alpha->d) {
									case 0:
										// Cs - Cd*As
										glBlendFunc(GL_ONE,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 1: // UNTITLED
										// Cd(1-As)
										glBlendFunc(GL_ZERO,GL_ONE_MINUS_SRC_ALPHA);
										break;
									case 2:
										// -Cd*As
										glBlendFunc(GL_ZERO,GL_SRC_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
								}
								break;
							case 1:
								switch (alpha->d) {
									case 0:
										// Cs - Cd*Ad
										glBlendFunc(GL_ONE,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 1: // UNTITLED
										// Cd(1-Ad)
										glBlendFunc(GL_ZERO,GL_ONE_MINUS_DST_ALPHA);
										break;
									case 2:
										// -Cd*Ad
										glBlendFunc(GL_ZERO,GL_DST_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
								}
								break;
							case 2:
								switch (alpha->d) {
									case 0:
										// Cs - Cd*FIX
										SET_FIX();
										glBlendFunc(GL_ONE,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
									case 1: // UNTITLED
										// Cd(1-As)
										SET_FIX();
										glBlendFunc(GL_ZERO,GL_ONE_MINUS_CONSTANT_ALPHA);
										break;
									case 2:
										// -Cd*As
										SET_FIX();
										glBlendFunc(GL_ZERO,GL_CONSTANT_ALPHA);
										glBlendEquation(GL_FUNC_SUBTRACT);
										break;
								}
								break;
						}
						break;
					case 2:
						// a-b=0
						switch (alpha->d) {
							case 0:
								// Cs
								glBlendFunc(GL_ONE,GL_ZERO);
								break;
							case 1:
								// Cd
								glBlendFunc(GL_ZERO,GL_ONE);
								break;
							case 2:
								// 0
								glBlendFunc(GL_ZERO,GL_ZERO);
								break;
						}
						break;
				}
				break;
		}

	}
}

// Depth test

void GSsetDepthTest()
{
	if (test->zte && test->ztst == 2) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GEQUAL);
	} else if (test->zte && test->ztst == 3) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);
	} else
		glDisable(GL_DEPTH_TEST);
}

// Alpha test

void GSsetAlphaTest() {
	if (test->ate) {
		s32 fnc;

		switch (test->atst) {
			case 0: fnc = GL_NEVER;    break; // 000 - NEVER
			case 1: fnc = GL_ALWAYS;   break; // 001 - ALWAYS
			case 2: fnc = GL_LESS;     break; // 010 - LESS
			case 3: fnc = GL_LEQUAL;   break; // 011 - LEQUAL
			case 4: fnc = GL_EQUAL;    break; // 100 - EQUAL
			case 5: fnc = GL_GEQUAL;   break; // 101 - GEQUAL
			case 6: fnc = GL_GREATER;  break; // 110 - GREATER
			case 7: fnc = GL_NOTEQUAL; break; // 111 - NOTEQUAL
		}

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(fnc,(float)test->aref/255.f);
	}
	else glDisable(GL_ALPHA_TEST);
}

// Antialiasing

void GSsetAA1() {

	if (config.video.antialiasing) {

		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);

	} else {

		if (prim->aa1) {
			glEnable(GL_POINT_SMOOTH);
			glEnable(GL_LINE_SMOOTH);
			glEnable(GL_POLYGON_SMOOTH);
		} else {
			glDisable(GL_POINT_SMOOTH);
			glDisable(GL_LINE_SMOOTH);
			glDisable(GL_POLYGON_SMOOTH);
		}

	}
}

// update context

void GSupdateContext()
{
	alpha    = &_alpha[prim->ctxt];
	clamp    = &_clamp[prim->ctxt];
	frame    = &_frame[prim->ctxt];
	miptbp   = &_miptbp[prim->ctxt];
	scissor  = &_scissor[prim->ctxt];
	test     = &_test[prim->ctxt];
	tex0     = &_tex0[prim->ctxt];
	tex1     = &_tex1[prim->ctxt];
	xyoffset = &_xyoffset[prim->ctxt];
	zbuf     = &_zbuf[prim->ctxt];
}

// vertex kick

void vertexKick() {
	s32 i;

	vCount--;

	for (i=0; i<vCount; i++) {
		vertex[i].x = vertex[i+1].x;
		vertex[i].y = vertex[i+1].y;
		vertex[i].z = vertex[i+1].z;
		vertex[i].f = vertex[i+1].f;

		if (prim->tme) {
			if (prim->fst) {
				vertex[i].u = vertex[i+1].u;
				vertex[i].v = vertex[i+1].v;
			} else {
				vertex[i].s = vertex[i+1].s;
				vertex[i].t = vertex[i+1].t;
				vertex[i].q = vertex[i+1].q;
			}
		}

		if (prim->iip && prim->prim != 0 && prim->prim != 6)
			rgba[i] = rgba[i+1];
	}

//	if (prim->prim == 0x3)
//		vCount = 0;
}

void vertexKickFan() {
	vCount--;

	vertex[1].x = vertex[2].x;
	vertex[1].y = vertex[2].y;
	vertex[1].z = vertex[2].z;
	vertex[1].f = vertex[2].f;

	if (prim->tme) {
		if (prim->fst) {
			vertex[1].u = vertex[2].u;
			vertex[1].v = vertex[2].v;
		} else {
			vertex[1].s = vertex[2].s;
			vertex[1].t = vertex[2].t;
			vertex[1].q = vertex[2].q;
		}
	}

	if (prim->iip && prim->prim != 0 && prim->prim != 6)
		rgba[1] = rgba[2];
}

// Write to 'normal' registers of the GS

void GSwrite(s32 reg, u32* pMem)
{
	switch (reg) {
		case 0x00: { // PRIM
			s32 ctxt = prim->ctxt;
			prim->prim    = (pMem[0])&0x7;
			_prim[1].iip  = (pMem[0]>>3)&0x1;
			_prim[1].tme  = (pMem[0]>>4)&0x1;
			_prim[1].fge  = (pMem[0]>>5)&0x1;
			_prim[1].abe  = (pMem[0]>>6)&0x1;
			_prim[1].aa1  = (pMem[0]>>7)&0x1;
			_prim[1].fst  = (pMem[0]>>8)&0x1;
			_prim[1].ctxt = (pMem[0]>>9)&0x1;
			_prim[1].fix  = (pMem[0]>>10)&0x1;
			vCount=0;

			if (prmodecont == 1) {
				GSupdateContext();
				if (prim->iip) glShadeModel(GL_SMOOTH); else glShadeModel(GL_FLAT);
				if (prim->tme) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
				if (prim->fge) glEnable(GL_FOG);        else glDisable(GL_FOG);
				GSsetAA1();
				GSsetBlend();

				if (ctxt != prim->ctxt) {
					GSprocessTex1();
					GSsetAlphaTest();
					GSsetDepthTest();
				}
			}

			if (config.log.registers)
				GSlog("PRIM prim=%x iip=%x tme=%x fge=%x abe=%x aa1=%x fst=%x ctxt=%x fix=%x\n",
					prim->prim,_prim[1].iip,_prim[1].tme,_prim[1].fge,_prim[1].abe,_prim[1].aa1,
					_prim[1].fst,_prim[1].ctxt,_prim[1].fix);
			break; }

		case 0x01: // RGBAQ
			rgba[vCount].rgba = pMem[0];
			rgba[vCount].r = /*2.f*/((float)((pMem[0]    )&0xff))/255.f;
			rgba[vCount].g = /*2.f*/((float)((pMem[0]>>8 )&0xff))/255.f;
			rgba[vCount].b = /*2.f*/((float)((pMem[0]>>16)&0xff))/255.f;
			rgba[vCount].a = 2.f*((float)((pMem[0]>>24)&0xff))/255.f;
			rgbac = rgba[vCount];
			vertex[vCount].q = pMem[1];

			if (config.log.registers)
				GSlog("RGBAQ rgba=%x q=%x\n",pMem[0],pMem[1]);
			break;

		case 0x02: // ST
			vertex[vCount].s = pMem[0];
			vertex[vCount].t = pMem[1];

			if (config.log.registers)
				GSlog("ST s=%x t=%x\n",pMem[0],pMem[1]);
			break;

		case 0x03: // UV
			vertex[vCount].u = (pMem[0]>>4)&0x7ff;
			vertex[vCount].v = (pMem[0]>>(16+4))&0x7ff;

			if (config.log.registers)
				GSlog("UV u=%x v=%x\n",vertex[vCount].u,vertex[vCount].v);
			break;

		case 0x04: // XYZF2
			vertex[vCount].x = (pMem[0]>>4)&0xfff;
			vertex[vCount].y = (pMem[0]>>(16+4))&0xfff;
			vertex[vCount].z = pMem[1]&0xffffff;
			vertex[vCount].f = (pMem[1]>>24)&0xff;

			if (config.log.registers)
				GSlog("XYZF2 vCount=%x x=%x y=%x z=%x f=%x\n",vCount,vertex[vCount].x,vertex[vCount].y,vertex[vCount].z,vertex[vCount].f);

			vCount++;

			if (vCount >= primCount[prim->prim]) {
				// drawing kick
				if (((test->zte == 1) && (test->ztst != 0)) || (!test->zte))
					primTable[prim->prim](vertex);
				// vertex kick
				if (prim->prim == 5)
					 vertexKickFan();
				else vertexKick();
			}
			break;

		case 0x05: // XYZ2
			vertex[vCount].x = (pMem[0]>>4)&0xfff;
			vertex[vCount].y = (pMem[0]>>(16+4))&0xfff;
			vertex[vCount].z = pMem[1];

			if (config.log.registers)
				GSlog("XYZ2 vCount=%x x=%x y=%x z=%x\n",vCount,vertex[vCount].x,vertex[vCount].y,vertex[vCount].z);

			vCount++;

			if (vCount >= primCount[prim->prim]) {
				// drawing kick
				if (((test->zte == 1) && (test->ztst != 0)) || (!test->zte))
					primTable[prim->prim](vertex);
				// vertex kick
				if (prim->prim == 5)
					 vertexKickFan();
				else vertexKick();
			}
			break;

		case 0x06: // TEX0_1
			_tex0[0].tbp0 = (pMem[0]&0x3fff)*64;
			_tex0[0].tbw  = ((pMem[0]>>14)&0x3f)*64;
			_tex0[0].psm  = (pMem[0]>>20)&0x3f;
			_tex0[0].tw   = (pMem[0]>>26)&0xf;
			if (_tex0[0].tw > 10) _tex0[0].tw = 10;
			_tex0[0].tw   = (int)pow(2, (double)_tex0[0].tw);
			_tex0[0].th   = ((pMem[0]>>30)&0x3)|((pMem[1]&0x3)<<2);
			if (_tex0[0].th > 10) _tex0[0].th = 10;
			_tex0[0].th   = (int)pow(2, (double)_tex0[0].th);
			_tex0[0].tcc  = (pMem[1]>>2)&0x1;
			_tex0[0].tfx  = (pMem[1]>>3)&0x3;
			_tex0[0].cbp  = ((pMem[1]>>5)&0x3fff)*64;
			_tex0[0].cpsm = (pMem[1]>>19)&0xf;
			_tex0[0].csm  = (pMem[1]>>23)&0x1;
			_tex0[0].csa  = ((pMem[1]>>24)&0x3f)*16;
			_tex0[0].cld  = (pMem[1]>>29)&0x7;

			if (config.log.registers)
				GSlog("TEX0(1) tbp0=%x tbw=%x psm=%x tw=%x th=%x tcc=%x tfx=%x cbp=%x cpsm=%x csm=%x csa=%x cld=%x\n",
					_tex0[0].tbp0,_tex0[0].tbw,_tex0[0].psm,_tex0[0].tw,_tex0[0].th,_tex0[0].tcc,
					_tex0[0].tfx,_tex0[0].cbp,_tex0[0].cpsm,_tex0[0].csm,_tex0[0].csa,_tex0[0].cld);

			if (GSprocessTexture(0))
				GSprocessMipmap(0);
			break;

		case 0x07: // TEX0_2
			_tex0[1].tbp0 = (pMem[0]&0x3fff)*64;
			_tex0[1].tbw  = ((pMem[0]>>14)&0x3f)*64;
			_tex0[1].psm  = (pMem[0]>>20)&0x3f;
			_tex0[1].tw   = (pMem[0]>>26)&0xf;
			if (_tex0[1].tw > 10) _tex0[1].tw = 10;
			_tex0[1].tw   = (int)pow(2, (double)_tex0[1].tw);
			_tex0[1].th   = ((pMem[0]>>30)&0x3)|((pMem[1]&0x3)<<2);
			if (_tex0[1].th > 10) _tex0[1].th = 10;
			_tex0[1].th   = (int)pow(2, (double)_tex0[1].th);
			_tex0[1].tcc  = (pMem[1]>>2)&0x1;
			_tex0[1].tfx  = (pMem[1]>>3)&0x3;
			_tex0[1].cbp  = ((pMem[1]>>5)&0x3fff)*64;
			_tex0[1].cpsm = (pMem[1]>>19)&0xf;
			_tex0[1].csm  = (pMem[1]>>23)&0x1;
			_tex0[1].csa  = ((pMem[1]>>24)&0x3f)*16;
			_tex0[1].cld  = (pMem[1]>>29)&0x7;

			if (config.log.registers)
				GSlog("TEX0(2) tbp0=%x tbw=%x psm=%x tw=%x th=%x tcc=%x tfx=%x cbp=%x cpsm=%x csm=%x csa=%x cld=%x\n",
					_tex0[1].tbp0,_tex0[1].tbw,_tex0[1].psm,_tex0[1].tw,_tex0[1].th,_tex0[1].tcc,
					_tex0[1].tfx,_tex0[1].cbp,_tex0[1].cpsm,_tex0[1].csm,_tex0[1].csa,_tex0[1].cld);

			if (GSprocessTexture(1))
				GSprocessMipmap(1);
			break;

		case 0x08: // CLAMP_1
			_clamp[0].wms  = pMem[0]&0x3;
			_clamp[0].wmt  = (pMem[0]>>2)&0x3;
			_clamp[0].minu = (pMem[0]>>4)&0x3ff;
			_clamp[0].maxu = (pMem[0]>>14)&0x3ff;
			_clamp[0].minv = ((pMem[0]>>24)&0xff)|((pMem[1]&0x3)<<8);
			_clamp[0].maxv = (pMem[1]>>2)&0x3ff;

			if (config.log.registers)
				GSlog("CLAMP(1) wms=%x wmt=%x minu=%x maxu=%x minv=%x maxv=%x\n",
					_clamp[0].wms,_clamp[0].wmt,_clamp[0].minu,_clamp[0].maxu,_clamp[0].minv,_clamp[0].maxv);
			break;

		case 0x09: // CLAMP_2
			_clamp[1].wms  = pMem[0]&0x3;
			_clamp[1].wmt  = (pMem[0]>>2)&0x3;
			_clamp[1].minu = (pMem[0]>>4)&0x3ff;
			_clamp[1].maxu = (pMem[0]>>14)&0x3ff;
			_clamp[1].minv = ((pMem[0]>>24)&0xff)|((pMem[1]&0x3)<<8);
			_clamp[1].maxv = (pMem[1]>>2)&0x3ff;

			if (config.log.registers)
				GSlog("CLAMP(2) wms=%x wmt=%x minu=%x maxu=%x minv=%x maxv=%x\n",
					_clamp[1].wms,_clamp[1].wmt,_clamp[1].minu,_clamp[1].maxu,_clamp[1].minv,_clamp[1].maxv);
			break;
/*
		case 0x0A: // FOG
//			glFogf(GL_FOG_DENSITY,((float)((pMem[1]>>24)&0xff))/255.f);
			break;
*/
		case 0x0C: // XYZF3
			vertex[vCount].x = (pMem[0]>>4)&0xfff;
			vertex[vCount].y = (pMem[0]>>(16+4))&0xfff;
			vertex[vCount].z = pMem[1]&0xffffff;
			vertex[vCount].f = (pMem[1]>>24)&0xff;

			if (config.log.registers)
				GSlog("XYZF3 vCount=%x x=%x y=%x z=%x f=%x\n",vCount,vertex[vCount].x,vertex[vCount].y,vertex[vCount].z,vertex[vCount].f);

			vCount++;

			// No drawing kick
			// vertex kick
			if (vCount >= primCount[prim->prim]) {
				if (prim->prim == 5)
					 vertexKickFan();
				else vertexKick();
			}
			break;

		case 0x0D: // XYZ3
			vertex[vCount].x = (pMem[0]>>4)&0xfff;
			vertex[vCount].y = (pMem[0]>>(16+4))&0xfff;
			vertex[vCount].z = pMem[1];

			if (config.log.registers)
				GSlog("XYZ3 vCount=%x x=%x y=%x z=%x\n",vCount,vertex[vCount].x,vertex[vCount].y,vertex[vCount].z);

			vCount++;

			// No drawing kick
			// vertex kick
			if (vCount >= primCount[prim->prim]) {
				if (prim->prim == 5)
					 vertexKickFan();
				else vertexKick();
			}
			break;

		case 0x14: // TEX1_1
			_tex1[0].lcm  = pMem[0]&0x1;
			_tex1[0].mxl  = (pMem[0]>>2)&0x7;
			_tex1[0].mmag = (pMem[0]>>5)&0x1;
			_tex1[0].mmin = (pMem[0]>>6)&0x7;
			_tex1[0].mtba = (pMem[0]>>9)&0x1;
			_tex1[0].l    = (pMem[0]>>19)&0x3;
			_tex1[0].k    = pMem[1]&0xfff;

			if (prim->ctxt == 0)
				GSprocessTex1();

			if (config.log.registers)
				GSlog("TEX1(1) lcm=%x mxl=%x mmag=%x mmin=%x mtba=%x l=%x k=%x\n",
					_tex1[0].lcm,_tex1[0].mxl,_tex1[0].mmag,_tex1[0].mmin,_tex1[0].mtba,_tex1[0].l,_tex1[0].k);
			break;

		case 0x15: // TEX1_2
			_tex1[1].lcm  = pMem[0]&0x1;
			_tex1[1].mxl  = (pMem[0]>>2)&0x7;
			_tex1[1].mmag = (pMem[0]>>5)&0x1;
			_tex1[1].mmin = (pMem[0]>>6)&0x7;
			_tex1[1].mtba = (pMem[0]>>9)&0x1;
			_tex1[1].l    = (pMem[0]>>19)&0x3;
			_tex1[1].k    = pMem[1]&0xfff;

			if (prim->ctxt == 1)
				GSprocessTex1();

			if (config.log.registers)
				GSlog("TEX1(2) lcm=%x mxl=%x mmag=%x mmin=%x mtba=%x l=%x k=%x\n",
					_tex1[1].lcm,_tex1[1].mxl,_tex1[1].mmag,_tex1[1].mmin,_tex1[1].mtba,_tex1[1].l,_tex1[1].k);
			break;

		case 0x18: // XYOFFSET_1
			_xyoffset[0].x = (pMem[0]>>4)&0xfff;
			_xyoffset[0].y = (pMem[1]>>4)&0xfff;

			if (config.log.registers)
				GSlog("XYOFFSET(1) x=%x y=%x\n",_xyoffset[0].x,_xyoffset[0].y);
			break;

		case 0x19: // XYOFFSET_2
			_xyoffset[1].x = (pMem[0]>>4)&0xfff;
			_xyoffset[1].y = (pMem[1]>>4)&0xfff;

			if (config.log.registers)
				GSlog("XYOFFSET(2) x=%x y=%x\n",_xyoffset[1].x,_xyoffset[1].y);
			break;

		case 0x1A: { // PRMODECONT
			s32 prmode = prmodecont;
			prmodecont = pMem[0]&0x1;
			prim       = &_prim[prmodecont];

			if (config.log.registers)
				GSlog("PRMODECONT %x\n",prmodecont);

			if (prmode != prmodecont) {
				GSupdateContext();
				if (prim->iip) glShadeModel(GL_SMOOTH); else glShadeModel(GL_FLAT);
				if (prim->tme) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
				if (prim->fge) glEnable(GL_FOG);        else glDisable(GL_FOG);
				if (prim->abe) glEnable(GL_BLEND);      else glDisable(GL_BLEND);
				GSsetAA1();

				GSprocessTex1();
				GSsetBlend();
				GSsetAlphaTest();
				GSsetDepthTest();
			}
			break; }

		case 0x1B: { // PRMODE
			s32 ctxt = prim->ctxt;
			_prim[0].iip  = (pMem[0]>>3)&0x1;
			_prim[0].tme  = (pMem[0]>>4)&0x1;
			_prim[0].fge  = (pMem[0]>>5)&0x1;
			_prim[0].abe  = (pMem[0]>>6)&0x1;
			_prim[0].aa1  = (pMem[0]>>7)&0x1;
			_prim[0].fst  = (pMem[0]>>8)&0x1;
			_prim[0].ctxt = (pMem[0]>>9)&0x1;
			_prim[0].fix  = (pMem[0]>>10)&0x1;
			vCount=0;

			if (prmodecont == 0) {
				GSupdateContext();
				if (prim->iip) glShadeModel(GL_SMOOTH); else glShadeModel(GL_FLAT);
				if (prim->tme) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
				if (prim->fge) glEnable(GL_FOG);        else glDisable(GL_FOG);
				if (prim->abe) glEnable(GL_BLEND);      else glDisable(GL_BLEND);
				GSsetAA1();
				GSsetBlend();

				if (ctxt != prim->ctxt) {
					GSprocessTex1();
					GSsetAlphaTest();
					GSsetDepthTest();
				}
			}

			if (config.log.registers)
				GSlog("PRMODE iip=%x tme=%x fge=%x abe=%x aa1=%x fst=%x ctxt=%x fix=%x\n",
					_prim[0].iip,_prim[0].tme,_prim[0].fge,_prim[0].abe,_prim[0].aa1,
					_prim[0].fst,_prim[0].ctxt,_prim[0].fix);

			break; }

/*		case 0x1C: // TEXCLUT
			texclut.cbw = (pMem[0]&0x3f)*64;
			texclut.cou = ((pMem[0]>>6)&0x3f)*16;
			texclut.cov = (pMem[0]>>12)&0x3ff;
			break;
*/
		case 0x34: // MIPTBP1_1
			_miptbp[0].tbp[0] = (((pMem[0])&0x3fff))*64;
			_miptbp[0].tbw[0] = (((pMem[0]>>14)&0x3f))*64;
			_miptbp[0].tbp[1] = (((pMem[0]>>20)&0xfff)|((pMem[1]&0x3)<<12))*64;
			_miptbp[0].tbw[1] = (((pMem[1]>>2)&0x3f))*64;
			_miptbp[0].tbp[2] = (((pMem[1]>>8)&0x3fff))*64;
			_miptbp[0].tbw[2] = (((pMem[1]>>22)&0x3f))*64;
			break;

		case 0x35: // MIPTBP1_2
			_miptbp[1].tbp[0] = (((pMem[0])&0x3fff))*64;
			_miptbp[1].tbw[0] = (((pMem[0]>>14)&0x3f))*64;
			_miptbp[1].tbp[1] = (((pMem[0]>>20)&0xfff)|((pMem[1]&0x3)<<12))*64;
			_miptbp[1].tbw[1] = (((pMem[1]>>2)&0x3f))*64;
			_miptbp[1].tbp[2] = (((pMem[1]>>8)&0x3fff))*64;
			_miptbp[1].tbw[2] = (((pMem[1]>>22)&0x3f))*64;
			break;

		case 0x36: // MIPTBP2_1
			_miptbp[0].tbp[3] = (((pMem[0])&0x3fff))*64;
			_miptbp[0].tbw[3] = (((pMem[0]>>14)&0x3f))*64;
			_miptbp[0].tbp[4] = (((pMem[0]>>20)&0xfff)|((pMem[1]&0x3)<<12))*64;
			_miptbp[0].tbw[4] = (((pMem[1]>>2)&0x3f))*64;
			_miptbp[0].tbp[5] = (((pMem[1]>>8)&0x3fff))*64;
			_miptbp[0].tbw[5] = (((pMem[1]>>22)&0x3f))*64;
			break;

		case 0x37: // MIPTBP2_2
			_miptbp[1].tbp[3] = (((pMem[0])&0x3fff))*64;
			_miptbp[1].tbw[3] = (((pMem[0]>>14)&0x3f))*64;
			_miptbp[1].tbp[4] = (((pMem[0]>>20)&0xfff)|((pMem[1]&0x3)<<12))*64;
			_miptbp[1].tbw[4] = (((pMem[1]>>2)&0x3f))*64;
			_miptbp[1].tbp[5] = (((pMem[1]>>8)&0x3fff))*64;
			_miptbp[1].tbw[5] = (((pMem[1]>>22)&0x3f))*64;
			break;

		case 0x3B: // TEXA
			texa.ta0 = (pMem[0]    )&0xff;
			texa.aem = (pMem[0]>>15)&0x1;
			texa.ta1 = (pMem[1]    )&0xff;
			if (config.log.registers)
				GSlog("TEXA aem=%x ta0=%x ta1=%x\n",texa.aem,texa.ta0,texa.ta1);
			break;

/*
#ifdef LOG
		case 0x3F: // TEXFLUSH
	LOG("TEXFLUSH");
			break;
#endif
*/
		case 0x40: // SCISSOR_1
			_scissor[0].x0 = pMem[0]&0x7ff;
			_scissor[0].x1 = (pMem[0]>>16)&0x7ff;
			_scissor[0].y0 = pMem[1]&0x7ff;
			_scissor[0].y1 = (pMem[1]>>16)&0x7ff;

			if (config.log.registers)
				GSlog("SCISSOR(1) x0=%x x1=%x y0=%x y1=%x\n",_scissor[0].x0,_scissor[0].x1,_scissor[0].y0,_scissor[0].y1);
			break;

		case 0x41: // SCISSOR_2
			_scissor[1].x0 = pMem[0]&0x7ff;
			_scissor[1].x1 = (pMem[0]>>16)&0x7ff;
			_scissor[1].y0 = pMem[1]&0x7ff;
			_scissor[1].y1 = (pMem[1]>>16)&0x7ff;

			if (config.log.registers)
				GSlog("SCISSOR(2) x0=%x x1=%x y0=%x y1=%x\n",_scissor[1].x0,_scissor[1].x1,_scissor[1].y0,_scissor[1].y1);
			break;

		case 0x42: // ALPHA_1
			_alpha[0].a   = pMem[0]&0x3;
			_alpha[0].b   = (pMem[0]>>2)&0x3;
			_alpha[0].c   = (pMem[0]>>4)&0x3;
			_alpha[0].d   = (pMem[0]>>6)&0x3;
			_alpha[0].fix = pMem[1]&0xff;

			if (config.log.registers)
				GSlog("ALPHA(1) a=%x b=%x c=%x d=%x fix=%x\n",
					_alpha[0].a,_alpha[0].b,_alpha[0].c,_alpha[0].d,_alpha[0].fix);

			_alpha[0].fix *= 2;

			if (prim->ctxt == 0)
				GSsetBlend();
			break;

		case 0x43: // ALPHA_2
			_alpha[1].a   = pMem[0]&0x3;
			_alpha[1].b   = (pMem[0]>>2)&0x3;
			_alpha[1].c   = (pMem[0]>>4)&0x3;
			_alpha[1].d   = (pMem[0]>>6)&0x3;
			_alpha[1].fix = pMem[1]&0xff;

			if (config.log.registers)
				GSlog("ALPHA(2) a=%x b=%x c=%x d=%x fix=%x\n",
					_alpha[1].a,_alpha[1].b,_alpha[1].c,_alpha[1].d,_alpha[1].fix);

			_alpha[1].fix *= 2;

			if (prim->ctxt == 1)
				GSsetBlend();
			break;
/*
		case 0x45: // DTHE
			dthe = pMem[0]&0x1;

			if (dthe) glEnable(GL_DITHER);
			else      glDisable(GL_DITHER);
			break;

/*		case 0x46: // COLCLAMP
			colclamp = pMem[0]&0x1;

#ifdef LOG
	LOG("COLCLAMP colclamp=%x",colclamp);
#endif

			break;
*/
		case 0x47: // TEST_1
			_test[0].ate   = pMem[0]&0x1;
			_test[0].atst  = (pMem[0]>>1)&0x7;
			_test[0].aref  = (pMem[0]>>4)&0xff;
			_test[0].afail = (pMem[0]>>12)&0x3;
			_test[0].date  = (pMem[0]>>14)&0x1;
			_test[0].datm  = (pMem[0]>>15)&0x1;
			_test[0].zte   = (pMem[0]>>16)&0x1;
			_test[0].ztst  = (pMem[0]>>17)&0x3;

			if (prim->ctxt == 0) {
				GSsetAlphaTest();
				GSsetDepthTest();
			}

			if (config.log.registers)
				GSlog("TEST(1) ate=%x atst=%x aref=%x afail=%x date=%x datm=%x zte=%x ztst=%x\n",
					_test[0].ate,_test[0].atst,_test[0].aref,_test[0].afail,_test[0].date,_test[0].datm,
					_test[0].zte,_test[0].ztst);
			break;

		case 0x48: // TEST_2
			_test[1].ate   = pMem[0]&0x1;
			_test[1].atst  = (pMem[0]>>1)&0x7;
			_test[1].aref  = (pMem[0]>>4)&0xff;
			_test[1].afail = (pMem[0]>>12)&0x3;
			_test[1].date  = (pMem[0]>>14)&0x1;
			_test[1].datm  = (pMem[0]>>15)&0x1;
			_test[1].zte   = (pMem[0]>>16)&0x1;
			_test[1].ztst  = (pMem[0]>>17)&0x3;

			if (prim->ctxt == 1) {
				GSsetAlphaTest();
				GSsetDepthTest();
			}

			if (config.log.registers)
				GSlog("TEST(2) ate=%x atst=%x aref=%x afail=%x date=%x datm=%x zte=%x ztst=%x\n",
					_test[1].ate,_test[1].atst,_test[1].aref,_test[1].afail,_test[1].date,_test[1].datm,
					_test[1].zte,_test[1].ztst);
			break;

		case 0x4C: // FRAME_1
			_frame[0].fbp   = (pMem[0]&0x1ff)*2048;
			_frame[0].fbw   = ((pMem[0]>>16)&0x3f)*64;
			_frame[0].psm   = (pMem[0]>>24)&0x3f;
			_frame[0].fbmsk = pMem[1];

			if (config.fixes.fbfix == 2)
				GSupdateGL(1);

			if (config.log.registers)
				GSlog("FRAME(1) fbp=%x fbw=%x psm=%x fbmsk=%x\n",_frame[0].fbp,_frame[0].fbw,_frame[0].psm,_frame[0].fbmsk);
			break;

		case 0x4D: // FRAME_2
			_frame[1].fbp   = (pMem[0]&0x1ff)*2048;
			_frame[1].fbw   = ((pMem[0]>>16)&0x3f)*64;
			_frame[1].psm   = (pMem[0]>>24)&0x3f;
			_frame[1].fbmsk = pMem[1];

			if (config.fixes.fbfix == 2)
				GSupdateGL(1);

			if (config.log.registers)
				GSlog("FRAME(2) fbp=%x fbw=%x psm=%x fbmsk=%x\n",_frame[1].fbp,_frame[1].fbw,_frame[1].psm,_frame[1].fbmsk);
			break;

		case 0x4E: // ZBUF_1
			_zbuf[0].zbp  = (pMem[0]&0x1ff)*2048;
			_zbuf[0].psm  = (pMem[0]>>24)&0xf;
			_zbuf[0].zmsk = pMem[1]&0x1;

			if (config.log.registers)
				GSlog("ZBUF(1) zbp=%x psm=%x zmsk=%x\n",_zbuf[0].zbp,_zbuf[0].psm,_zbuf[0].zmsk);
			break;

		case 0x4F: // ZBUF_2
			_zbuf[1].zbp  = (pMem[0]&0x1ff)*2048;
			_zbuf[1].psm  = (pMem[0]>>24)&0xf;
			_zbuf[1].zmsk = pMem[1]&0x1;

			if (config.log.registers)
				GSlog("ZBUF(2) zbp=%x psm=%x zmsk=%x\n",_zbuf[1].zbp,_zbuf[1].psm,_zbuf[1].zmsk);
			break;

		case 0x50: // BITBLTBUF

			last_bitbltbuf_dbw = bitbltbuf.dbw;
			last_bitbltbuf_psm = bitbltbuf.dpsm;

			bitbltbuf.sbp  = (pMem[0]&0x3fff)*64;
			bitbltbuf.sbw  = ((pMem[0]>>16)&0x3f)*64;
			bitbltbuf.spsm = (pMem[0]>>24)&0x3f;
			bitbltbuf.dbp  = (pMem[1]&0x3fff)*64;
			bitbltbuf.dbw  = ((pMem[1]>>16)&0x3f)*64;
			bitbltbuf.dpsm = (pMem[1]>>24)&0x3f;

			if (bitbltbuf.dbw == 0) bitbltbuf.dbw = last_bitbltbuf_dbw/64;

			if (config.log.registers)
				GSlog("BITBLTBUF sbp=%x sbw=%x spsm=%x dbp=%x dbw=%x dpsm=%x\n",
					bitbltbuf.sbp,bitbltbuf.sbw,bitbltbuf.spsm,bitbltbuf.dbp,bitbltbuf.dbw,bitbltbuf.dpsm);
			break;

		case 0x51: // TRXPOS
			trxpos.ssax = pMem[0]&0x7ff;
			trxpos.ssay = (pMem[0]>>16)&0x7ff;
			trxpos.dsax = pMem[1]&0x7ff;
			trxpos.dsay = (pMem[1]>>16)&0x7ff;
			trxpos.dir  = (pMem[1]>>27)&0x3;

			imgx  = trxpos.dsax;
			imgy  = trxpos.dsay;
			imgx_ = 0;
			imgy_ = 0;

			if (config.log.registers)
				GSlog("TRXPOS ssax=%x ssay=%x dsax=%x dsay=%x dir=%x\n",
					trxpos.ssax,trxpos.ssay,trxpos.dsax,trxpos.dsay,trxpos.dir);
			break;

		case 0x52: // TRXREG
			trxreg.rrw = pMem[0]&0xfff;
			trxreg.rrh = pMem[1]&0xfff;

			if (config.log.registers)
				GSlog("TRXREG rrw=%x rrh=%x\n",trxreg.rrw,trxreg.rrh);
			break;

		case 0x53: // TRXDIR
			trxdir = pMem[0]&0x3;

			if (config.log.registers)
				GSlog("TRXDIR trxdir=%x dirc=%x\n",trxdir,dirc);

			if (!dirc) free(image);
			dirc   = 1;
			break;
	}
}

// Direct write to privilegied registers
// 32 or 64 bits values

void CALLBACK GSwrite32(u32 mem, u32 value)
{
	if (config.log.pregisters)
		GSlog("w32 mem=%x value=%x\n",mem,value);
}

void CALLBACK GSwrite64(u32 mem, u64 value)
{
	u32 *v = (u32*)&value;

	switch (mem) {
		case 0x12000000: // PMODE
			pmode.en1  = (v[0]   )&0x1;
			pmode.en2  = (v[0]>>1)&0x1;
			pmode.mmod = (v[0]>>5)&0x1;
			pmode.amod = (v[0]>>6)&0x1;
			pmode.slbg = (v[0]>>7)&0x1;
			pmode.alp  = (v[0]>>8)&0xff;
			if (pmode.en2) {
				dispfb  = &_dispfb[1];
				display = &_display[1];
			} else {
				dispfb  = &_dispfb[0];
				display = &_display[0];
			}

			if (config.log.pregisters)
				GSlog("w64 PMODE en1=%x en2=%x mmod=%x amod=%x slbg=%x alp=%x\n",pmode.en1,pmode.en2,pmode.mmod,
					pmode.amod,pmode.slbg,pmode.alp);

			break;

		case 0x12000020: // SMODE2
			smode2.intm = (v[0]   )&0x1;
			smode2.ffmd = (v[0]>>1)&0x1;
			smode2.dpms = (v[0]>>2)&0x3;

			if (config.log.pregisters)
				GSlog("w64 SMODE2 intm=%x ffmd=%x dpms=%x\n",smode2.intm,smode2.ffmd,smode2.dpms);

			GSupdateView();

			break;

		case 0x12000070: // DISPFB1
			_dispfb[0].fbp = ((v[0]    )&0x1ff)*2048;
			_dispfb[0].fbw = ((v[0]>> 9)&0x3f )*64;
			_dispfb[0].psm = ((v[0]>>15)&0x1f );
			_dispfb[0].dbx = ((v[1]    )&0x7ff);
			_dispfb[0].dby = ((v[1]>>11)&0x7ff);

			if (config.log.pregisters)
				GSlog("w64 DISPFB1 fbp=%x fbw=%x psm=%x dbx=%x dby=%x\n",
					_dispfb[0].fbp,_dispfb[0].fbw,_dispfb[0].psm,_dispfb[0].dbx,_dispfb[0].dby);
			
			if (config.fixes.fbfix == 1)
				GSupdateGL(1);

			break;

		case 0x12000080: // DISPLAY1
			_display[0].dx = ((v[0]    )&0xfff);
			_display[0].dy = ((v[0]>>12)&0x7ff);
			_display[0].w  = ((v[1]&0xfff)+1)/(((v[0]>>23)&0xf)+1);
			_display[0].h  = (((v[1]>>12)&0x7ff)+1)/(((v[1]>>27)&0x3)+1);

//			if (smode2.intm && smode2.ffmd)
//				_display[0].h >>= 1;

			if (config.log.pregisters)
				GSlog("w64 DISPLAY1 dx=%x dy=%x w=%x h=%x\n",_display[0].dx,_display[0].dy,_display[0].w,_display[0].h);

//			_display[0].h += 40;

			GSupdateView();

			break;

		case 0x12000090: // DISPFB2
			_dispfb[1].fbp = ((v[0]    )&0x1ff)*2048;
			_dispfb[1].fbw = ((v[0]>> 9)&0x3f )*64;
			_dispfb[1].psm = ((v[0]>>15)&0x1f );
			_dispfb[1].dbx = ((v[1]    )&0x7ff);
			_dispfb[1].dby = ((v[1]>>11)&0x7ff);

			if (config.log.pregisters)
				GSlog("w64 DISPFB2 fbp=%x fbw=%x psm=%x dbx=%x dby=%x\n",
					_dispfb[1].fbp,_dispfb[1].fbw,_dispfb[1].psm,_dispfb[1].dbx,_dispfb[1].dby);

			if (config.fixes.fbfix == 1)
				GSupdateGL(1);

			break;

		case 0x120000A0: // DISPLAY2
			_display[1].dx = ((v[0]    )&0xfff);
			_display[1].dy = ((v[0]>>12)&0x7ff);
			_display[1].w  = ((v[1]&0xfff)+1)/(((v[0]>>23)&0xf)+1);
			_display[1].h  = (((v[1]>>12)&0x7ff)+1)/(((v[1]>>27)&0x3)+1);

//			if (smode2.intm && smode2.ffmd)
//				_display[1].h >>= 1;

			if (config.log.pregisters)
				GSlog("w64 DISPLAY2 dx=%x dy=%x w=%x h=%x\n",_display[1].dx,_display[1].dy,_display[1].w,_display[1].h);

//			_display[1].h += 40;

			GSupdateView();

			break;

		case 0x120000E0: // BGCOLOR
			bgcolor.r = ((float)((v[0]    )&0xff))/255.f;
			bgcolor.g = ((float)((v[0]>> 8)&0xff))/255.f;
			bgcolor.b = ((float)((v[0]>>16)&0xff))/255.f;

			if (config.log.pregisters)
				GSlog("w64 BGCOLOR\n");

			break;

		case 0x12001000: { // CSR

			// 5-6 -> 0

			csr.signal = v[0]&0x1;
			csr.finish = (v[0]>>1)&0x1;
			csr.hsint  = (v[0]>>2)&0x1;
			csr.vsint  = (v[0]>>3)&0x1;
			csr.edwint = (v[0]>>4)&0x1;
			csr.flush  = (v[0]>>8)&0x1;
			csr.reset  = (v[0]>>9)&0x1;
			csr.nfield = (v[0]>>12)&0x1;
			csr.field  = (v[0]>>13)&0x1;
			csr.fifo   = (v[0]>>14)&0x3;
			csr.rev    = (v[0]>>16)&0xf;
			csr.id     = (v[0]>>24)&0xf;

			if (csr.reset) GSreset();
			
			if (config.log.pregisters)
				GSlog("w64 CSR signal=%x finish=%x hsint=%x vsint=%x edwint=%x flush=%x reset=%x nfield=%x field=%x fifo=%x rev=%x id=%x\n",
					csr.signal,csr.finish,csr.hsint,csr.vsint,csr.edwint,csr.flush,
					csr.reset,csr.nfield,csr.field,csr.fifo,csr.rev,csr.id);

			break; }

		default:
			if (config.log.pregisters)
				GSlog("w64 mem=%x value=%8.8x_%8.8x\n",mem,v[1],v[0]);
			break;
	}
}

// Direct read from privilegied registers
// 32 or 64 bits values

u32 CALLBACK GSread32(u32 mem)
{
	if (config.log.pregisters)
		GSlog("r32 mem=%x\n",mem);

	return 0;
}

u64 CALLBACK GSread64(u32 mem)
{
	if (config.log.pregisters)
		GSlog("r64 mem=%x\n",mem);

//	if (config.fixes.fbfix == 3)
//		GSupdateGL(1);

//	switch (mem) {
//		case 0x12001000:
//			return 0x8;
//	}

	return 0;
}