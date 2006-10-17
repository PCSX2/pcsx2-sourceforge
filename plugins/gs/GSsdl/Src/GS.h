#ifndef __GS_H__
#define __GS_H__

#include <stdio.h>
#include <SDL.h>

#define VERSION 0
#define BUILD 2

#define GSdefs
#include "PS2Edefs.h"




SDL_Surface * display;


//#define __inline inline

typedef int BOOL;

//#define TRUE  1
//#define FALSE 0

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))


char GStitle[256];

typedef struct {
	int x, y, w, h;
} Rect;

typedef struct {
	int x, y;
} Point;

typedef struct {
	int x0, y0;
	int x1, y1;
} Rect2;

typedef struct {
	int x, y, c;
} PointC;

typedef struct {
	int width;
	int height;
	//hmm sdl not support bpp in how to get it..
} SDLmode;

typedef struct {

	SDLmode mode;
    int bpp;
	int fullscreen;
	int fps;
	int frameskip;
	int stretch;
} GSconf;

typedef struct {
	int x, y, f;
	u32 z;
	int u, v;
	u32 s, t, q;
	u32 rgba;
} Vertex;

SDLmode modes[64];
GSconf conf;
extern int norender;

SDL_Surface * vRamSDL;

unsigned char *vRam;
unsigned long *vRamUL;

unsigned char *fBuf;
unsigned long *fBufUL;

unsigned char *dBuf;
unsigned long *dBufUL;

unsigned char *zBuf;
unsigned long *zBufUL;

Vertex gsvertex[3];
extern u32 rgba;
int primC;
int psm;
int prac;
int dthe;
int colclamp;
int fogf;
int fogcol;
int smask;
int pabe;

typedef struct {
	int nloop;
	int eop;
	int flg;
	int nreg;
	int prim;
	int pre;
} tagInfo;

tagInfo gtag;

/* privileged regs structs */

Rect *gsmode;
Rect _gsmode[2];

typedef struct {
	int fbp;
	int fbw;
	int fbh;
	int psm;
	int dbx;
	int dby;
} dspfbInfo;

dspfbInfo *gsdspfb;
dspfbInfo _gsdspfb[2];

typedef struct {
	int inter;
	int ffmd;
	int dpms;
} smodeInfo;

smodeInfo smode2;

/* general purpose regs structs */

typedef struct {
	int fbp;
	int fbw;
	int fbh;
	int psm;
	int fbm;
} frameInfo;

frameInfo *gsfb;
frameInfo _gsfb[2];

Point *offset;
Point _offset[2];

Rect2 *scissor;
Rect2 _scissor[2];

typedef struct {
	int prim;
	int iip;
	int tme;
	int fge;
	int abe;
	int aai;
	int fst;
	int ctxt;
	int fix;
} primInfo;

primInfo *prim;
primInfo _prim[2];

typedef struct {
	int ate;
	int	atst;
	unsigned int aref;
	int afail;
	int date;
	int datm;
	int zte;
	int ztst;
} pixTest;

pixTest *test;
pixTest _test[2];

#define ZTST_NEVER   0
#define ZTST_ALWAYS  1
#define ZTST_GEQUAL  2
#define ZTST_GREATER 3

typedef struct {
	int bp;
	int bw;
	int bh;
	int psm;
} bufInfo;

bufInfo srcbuf;
bufInfo dstbuf;

typedef struct {
	int tbp0;
	int tbw;
	int psm;
	int tw, th;
	int tcc;
	int tfx;
	int cbp;
	int cpsm;
	int csm;
} tex0Info;

tex0Info *tex0;
tex0Info _tex0[2];

#define TEX_MODULATE 0
#define TEX_DECAL 1
#define TEX_HIGHLIGHT 2
#define TEX_HIGHLIGHT2 3

typedef struct {
	int lcm;
	int mxl;
	int mmag;
	int mmin;
	int mtba;
	int l;
	int k;
} tex1Info;

tex1Info *tex1;
tex1Info _tex1[2];

typedef struct {
	int psm;
	int cbp;
	int cpsm;
	int csm;
	int csa;
	int cld;
} tex2Info;

tex2Info *tex2;
tex2Info _tex2[2];

typedef struct {
	int u, v;
} uvInfo;

uvInfo uv;

typedef struct {
	int s, t;
} stInfo;

stInfo st;

typedef struct {
	int wms;
	int wmt;
	int minu;
	int maxu;
	int minv;
	int maxv;
} clampInfo;

clampInfo *clamp;
clampInfo _clamp[2];

typedef struct {
	int cbw;
	int cou;
	int cov;
} clutInfo;

clutInfo clut;

typedef struct {
	int tbp[3];
	int tbw[3];
} miptbpInfo;

miptbpInfo *miptbp0;
miptbpInfo _miptbp0[2];

miptbpInfo *miptbp1;
miptbpInfo _miptbp1[2];

typedef struct {
	int ta[2];
	int aem;
} texaInfo;

texaInfo texa;

typedef struct {
	int sx;
	int sy;
	int dx;
	int dy;
	int dir;
} trxposInfo;

trxposInfo trxpos;

typedef struct {
	int a, b, c, d;
	int fix;
} alphaInfo;

alphaInfo *alpha;
alphaInfo _alpha[2];

typedef struct {
	int zbp;
	int psm;
	int zmsk;
} zbufInfo;

zbufInfo *zbuf;
zbufInfo _zbuf[2];

typedef struct {
	int fba;
} fbaInfo;

fbaInfo *fba;
fbaInfo _fba[2];

FILE *gsLog;
//#define GS_LOG __Log

extern int Log;
void __Log(char *fmt, ...);

void LoadConfig();
void SaveConfig();

int  DXopen();
void DXclose();
void DXupdate();
void DXclearScr();

void SysMessage(char *fmt, ...);


#endif
