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
// 1/8/2003 : modificate Version info (shadow)
#ifndef __GStaris_H__
#define __GStaris_H__

#define __WIN32__
// #define __LINUX__

#if defined(__WIN32__)
#include <windows.h>
// WIN32 special part here
#elif defined(__LINUX__)
// LINUX special part here
#endif

#define GSdefs
#include "PS2Edefs.h"
#include "PS2Etypes.h"

#include <GL/gl.h>

// Plugin std defs

#define PLUGIN_VERSION PS2E_GS_VERSION
#define VERSION        0
#define BUILD          66
#define NAME           "GStaris OpenGL"

// Interface & config stuff

s32 framec,fps,fpsc,to,fps2,fpsc2;

void  GSsnapFrame();
void  GSsnapVram();
s32   snap_frame;
char* snap_path;

char* emulator_title;

s32  config_load();
s32  config_save();
void config_default();

typedef struct __gs_config_video {
	s32 width;
	s32 height;
	s32 bpp;
	s32 fullscreen;
	s32 force75hz; // force 75 hz in fullscreen mode
	s32 ratio; // 0:normal 1:stretched 2:keep_ps2_ratio

	// special features

	s32 antialiasing;
	s32 filtering;
} gs_config_video;

typedef struct __gs_config_speed {
	s32 limit_on;
	s32 limit_to;
} gs_config_speed;

typedef struct __gs_config_interface {
	s32 menu_visible;
	s32 menu_index;
	s32 texture_view; // -1 nothing, else...
	s32 texture_info;
} gs_config_interface;

typedef struct __gs_config_specialfixes {
	s32 fbfix;
	s32 missing_stuff;
	s32 disable_blend;
	s32 disable_idt_onscreen;
} gs_config_specialfixes;

typedef struct __gs_config_logging {
	s32 logon;
	s32 isoc;
	s32 gif;
	s32 idt;
	s32 prim;
	s32 registers;
	s32 pregisters;
	s32 textures;
	s32 texturesbuffer;
} gs_config_logging;

typedef struct __gs_config {
	gs_config_video        video;
	gs_config_speed        speed;
	gs_config_interface    itf;
	gs_config_specialfixes fixes;
	gs_config_logging      log;
} gs_config;

gs_config config;

// Logging funcs

void GSlog(char *string, ...);

// Stuff

s32 already_opened;
s32 config_changed;

// Message-window management

void* GScreateMsgWindow();
void  GSdestroyMsgWindow();

// Window management

s32  GScreateWindow(char* title);
void GSdestroyWindow();
s32  GSshowWindow();
void GShideWindow();

// OpenGL management

s32  GScreateGL();
void GSdestroyGL();
s32  GSfullscreenGL();

void GSprint(const char* format, ...);

// GSreset
void GSreset();

// Vsync

void GSupdateGL(s32 render);

// OpenGL extensions

typedef void (CALLBACK* _glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (CALLBACK* _glBlendEquation)(GLenum mode);

_glBlendColor    glBlendColor;
_glBlendEquation glBlendEquation;

void CALLBACK default_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void CALLBACK default_glBlendEquation(GLenum mode);
void APIENTRY InitBlendColor();
void APIENTRY InitBlendEquation();

// Misc funcs

void GSupdateView();

// VRAM

u8*  vRam;
u32* vRamUL;

// GIF

typedef struct __gif_tag {
	s32 nloop,eop,pre,prim,flg,nreg;
	u64 regs;
} gif_tag;

gif_tag giftag;
s32     gifmode;

s32 last_bitbltbuf_dbw;
s32 last_bitbltbuf_psm;

// IDT

void IDTwriteBegin();
void IDTwriteEnd();
void IDTwrite(u32* pMem);
void IDTmove();

u32 *image;
int imgx;
int imgy;
int imgx_;
int imgy_;
int dirc;

// Textures

typedef struct {
	// tex0/2 information

	s32 tbp0;
	s32 tbw;
	s32 psm;
	s32 tw;
	s32 th;
	s32 tcc;
	s32 tfx;
	s32 cbp;
	s32 cpsm;
	s32 csm;
	s32 csa;
	s32 cld;

	// other

	int name;
	int update; // update texture ?
} texture_info;

s32 GSprocessTexture(s32 ctxt);
void GSprocessMipmap(s32 ctxt);
void GSprocessTex1();

texture_info *textures;
int           tCount;

void NewTexture();
void SetLastTexture(s32 ctxt);
void AddTexture(texture_info *t);
void FreeTextures();
s32  IsTexture(s32 ctxt);
s32  IsTexture2(texture_info *t);
s32  NeedUpdateTexture(s32 name);
void UpdateTexture(s32 name);

void ShowTexture();

// vertex information

typedef struct {
	s32 x,y;
	s32 z;
	s32 f;
	s32 u,v;
	u32 s,t,q;
} gsvertex;

typedef struct {
	s32   rgba;
	float r;
	float g;
	float b;
	float a;
} gsrgba;

gsvertex vertex[3];
gsrgba   rgba[3];
gsrgba   rgbac;
s32      vCount;

/*
000 point
001 line
010 line strip
011 triangle
100 triangle strip
101 triangle fan
110 sprite
111 nothing
*/

void primPoint(gsvertex *v);
void primLine(gsvertex *v);
void primLineStrip(gsvertex *v);
void primTriangle(gsvertex *v);
void primTriangleStrip(gsvertex *v);
void primTriangleFan(gsvertex *v);
void primSprite(gsvertex *v);
void primNothing(gsvertex *v);

int primCount[];
void (*primTable[])(gsvertex *v);

// Registers

void GSwrite(s32 reg, u32* pMem);
void GSupdateContext();

void GSsetBlend();
void GSsetDepthTest();
void GSsetAlphaTest();

typedef struct {
	s32 x;
	s32 y;
} point;

typedef struct {
	s32 x0;
	s32 y0;
	s32 x1;
	s32 y1;
} rect;

typedef struct {
	s32 a;
	s32 b;
	s32 c;
	s32 d;
	s32 fix;
} reg_alpha;

typedef struct {
	s32 sbp;
	s32 sbw;
	s32 spsm;
	s32 dbp;
	s32 dbw;
	s32 dpsm;
} reg_bitbltbuf;

typedef struct {
	s32 wms;
	s32 wmt;
	s32 minu;
	s32 maxu;
	s32 minv;
	s32 maxv;
} reg_clamp;

typedef struct {
	s32 fbp;
	s32 fbw;
	s32 psm;
	s32 fbmsk;
} reg_frame;

typedef struct {
	s32 tbp[6];
	s32 tbw[6];
} reg_miptbp;

typedef struct {
	s32 prim;
	s32 iip;
	s32 tme;
	s32 fge;
	s32 abe;
	s32 aa1;
	s32 fst;
	s32 ctxt;
	s32 fix;
} reg_prim;

typedef struct {
	s32 ate;
	s32 atst;
	s32 aref;
	s32 afail;
	s32 date;
	s32 datm;
	s32 zte;
	s32 ztst;
} reg_test;

typedef struct {
	s32 tbp0;
	s32 tbw;
	s32 psm;
	s32 tw;
	s32 th;
	s32 tcc;
	s32 tfx;
	s32 cbp;
	s32 cpsm;
	s32 csm;
	s32 csa;
	s32 cld;
} reg_tex0;

typedef struct {
	s32 lcm;
	s32 mxl;
	s32 mmag;
	s32 mmin;
	s32 mtba;
	s32 l;
	s32 k;
} reg_tex1;

typedef struct {
	s32 ta0;
	s32 aem;
	s32 ta1;
} reg_texa;

typedef struct {
	s32 cbw;
	s32 cou;
	s32 cov;
} reg_texclut;

typedef struct {
	s32 ssax;
	s32 ssay;
	s32 dsax;
	s32 dsay;
	s32 dir;
} reg_trxpos;

typedef struct {
	s32 rrw;
	s32 rrh;
} reg_trxreg;

typedef struct {
	s32 zbp;
	s32 psm;
	s32 zmsk;
} reg_zbuf;

reg_alpha      *alpha;
reg_alpha      _alpha[2];
reg_bitbltbuf  bitbltbuf;
reg_clamp      *clamp;
reg_clamp      _clamp[2];
s32            colclamp;
s32            dthe;
reg_frame      *frame;
reg_frame      _frame[2];
reg_miptbp     *miptbp;
reg_miptbp     _miptbp[2];
reg_prim       *prim;
reg_prim       _prim[2];
s32             prmodecont;
rect           *scissor;
rect           _scissor[2];
reg_test       *test;
reg_test       _test[2];
reg_tex0       *tex0;
reg_tex0       _tex0[2];
reg_tex1       *tex1;
reg_tex1       _tex1[2];
reg_texa        texa;
reg_texclut     texclut;
s32             trxdir;
reg_trxpos      trxpos;
reg_trxreg      trxreg;
point          *xyoffset;
point          _xyoffset[2];
reg_zbuf       *zbuf;
reg_zbuf       _zbuf[2];

// Privilegied registers

typedef struct __preg_bgcolor {
	float r,g,b;
} preg_bgcolor;

typedef struct __preg_csr {
	s32 signal,finish,hsint,vsint,edwint,flush,reset,nfield,field,fifo,rev,id;
} preg_csr;

typedef struct __preg_dispfb {
	s32 fbp,fbw,psm,dbx,dby;
} preg_dispfb;

typedef struct __preg_display {
	s32 dx,dy,w,h;
} preg_display;

typedef struct __preg_pmode {
	s32 en1,en2,mmod,amod,slbg,alp;
} preg_pmode;

typedef struct __preg_smode2 {
	s32 intm,ffmd,dpms;
} preg_smode2;

preg_bgcolor bgcolor;
preg_csr     csr;
preg_dispfb  _dispfb[2];
preg_dispfb  *dispfb;
preg_display _display[2];
preg_display *display;
preg_pmode   pmode;
preg_smode2  smode2;

#endif // __GStaris_H__