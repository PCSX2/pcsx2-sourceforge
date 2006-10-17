#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <windowsx.h>

#include "GS.h"


#ifdef __WIN32__
#pragma warning(disable:4244)

HINSTANCE hInst=NULL;
#endif

int norender = 0;
int finish=0;	//hmm is that int?? 
int imageTransfer = 0;
int imageX, imageY, imageW, imageH;
u32 *imagePtr;

int gifMode = 0;
int interlace = 0;
u32 rgba = 0; /* The current rgba setting */
int Log = 1;
int ldbw;

int primTableC[8] = {1, 2, 2, 3, 3, 3, 2, 0};
void (*primTable[8])(Vertex *v);

const unsigned char version  = 1;    // GS library v1
const unsigned char revision = VERSION;
const unsigned char build    = BUILD;    // increase that with each version

#ifdef __WIN32__
static char *libraryName     = "GS SDL Driver";
#else
static char *libraryName     = "GS SDL Driver";
#endif

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_GS;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion() {
	return version<<16|revision<<8|build;
}

#ifdef __WIN32__

void SysMessage(char *fmt, ...) {
	va_list list;
	char tmp[512];

	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "GSsoftdx Msg", 0);
}

#endif

void __Log(char *fmt, ...) {
	va_list list;

	if (!Log) return;

	va_start(list, fmt);
	vfprintf(gsLog, fmt, list);
	va_end(list);
}

static void gsSetPmode(mode) {
	gsdspfb = &_gsdspfb[mode];
	gsmode  = &_gsmode[mode];
	dBuf = vRam + gsdspfb->fbp * 4;
	dBufUL = (u32*)dBuf;
}

static void gsSetCtxt(int ctxt) {
	offset  = &_offset[ctxt];
	test    = &_test[ctxt];
	scissor = &_scissor[ctxt];
	tex0	= &_tex0[ctxt];
	tex1	= &_tex1[ctxt];
	tex2	= &_tex2[ctxt];
	clamp	= &_clamp[ctxt];
	miptbp0 = &_miptbp0[ctxt];
	miptbp1 = &_miptbp1[ctxt];
	gsfb    = &_gsfb[ctxt];
	alpha   = &_alpha[ctxt];
	zbuf    = &_zbuf[ctxt];
	fba		= &_fba[ctxt];
 	fBuf   = vRam + gsfb->fbp * 4;
 	fBufUL = (u32*)fBuf;
	zBuf   = vRam + zbuf->zbp * 4;
	zBufUL = (u32*)zBuf;
}

s32 CALLBACK GSinit() {
	u32 rmask, gmask, bmask, amask;
	int i;
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#ifdef GS_LOG
	gsLog = fopen("gsLog.txt", "w");
	setvbuf(gsLog, NULL,  _IONBF, 0);
	GS_LOG("GSinit\n");
#endif
	i = SDL_Init( SDL_INIT_VIDEO );
	if ( i != 0 ) printf("SDL VIDEO INIT failed: %s\n", SDL_GetError());

    vRamSDL = SDL_CreateRGBSurface(SDL_HWSURFACE, 1024, 1024, 32,rmask, gmask, bmask, amask);
    if(vRamSDL == NULL)  printf("CreateRGBSurface failed: %s\n", SDL_GetError());

	vRam = (u8*) vRamSDL->pixels;

	vRamUL = (u32*)vRam;

	

	dBuf = vRam;
	dBufUL = vRamUL;

	gsSetCtxt(0);
	gsSetPmode(0);

	_gsmode[0].x = 0;
	_gsmode[0].y = 0;
	_gsmode[0].w = 640;
	_gsmode[0].h = 480;

	_gsmode[1].x = 0;
	_gsmode[1].y = 0;
	_gsmode[1].w = 640;
	_gsmode[1].h = 480;

	_gsdspfb[0].dbx = 0;
	_gsdspfb[0].dby = 0;
	_gsdspfb[0].fbw = 640;
	_gsdspfb[0].fbh = (1024*1024) / _gsdspfb[0].fbw;

	_gsdspfb[1].dbx = 0;
	_gsdspfb[1].dby = 0;
	_gsdspfb[1].fbw = 640;
	_gsdspfb[1].fbh = (1024*1024) / _gsdspfb[1].fbw;

	prac = 1;
	prim = &_prim[0];

#ifdef GS_LOG
	GS_LOG("GSinit ok\n");
#endif

	return 0;
}

void CALLBACK GSshutdown() {
	//-lu free(vRam);
	SDL_FreeSurface(vRamSDL);
#ifdef GS_LOG
	fclose(gsLog);
#endif
}
int CreateGSWindow() {
	int i;
	
	

	display = SDL_SetVideoMode ( conf.mode.width , conf.mode.height ,conf.bpp , SDL_HWPALETTE | SDL_HWSURFACE | SDL_DOUBLEBUF );
	if ( display == NULL ) printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());


	i = SDL_SetAlpha(display, 0 ,255);//changed in SDL 1.2.5
	if ( i != 0 ) printf("SDL_SetAlpha failed: %s\n", SDL_GetError());	
	
	i =  SDL_SetColorKey(display, 0 , 0);
	if ( i != 0 ) printf("SDL_SetColorKey failed: %s\n", SDL_GetError());
	



	SDL_WM_SetCaption ( GStitle , NULL );

	return 0;
}

s32 CALLBACK GSopen(void *pDsp, char *Title) {

#ifdef GS_LOG
	GS_LOG("GSopen\n");
#endif

	LoadConfig();

	strcpy(GStitle, Title);
	if (CreateGSWindow() == -1) return -1;
    
	//(SDL_Surface *)pDsp = display;
    *(s32*)pDsp = (s32)GetActiveWindow();

   


	if (DXopen() == -1) return -1;

#ifdef GS_LOG
	GS_LOG("GSopen ok\n");
#endif

	return 0;	
}

void CALLBACK GSclose() {
	DXclose();
	SDL_Quit();
  
/*
#ifdef __WIN32__
	DestroyWindow(GShwnd);
#endif*/
}

void CALLBACK GSmakeSnapshot(char *path) {

    FILE *bmpfile;
	char filename[256];
    u32 snapshotnr = 0;
     // increment snapshot value & try to get filename
	for (;;) {
		snapshotnr++;

		sprintf(filename,"%ssnap%03ld.bmp", path, snapshotnr);

		bmpfile=fopen(filename,"rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	}
 
    SDL_SaveBMP(display, filename);
}

void CALLBACK GSvsync() {
#ifdef GS_LOG
//	GS_LOG("\nGSvsync db %d,%d\n\n", gsdspfb->dbx, gsdspfb->dby);
#endif
//	interlace = 1 - interlace;

	DXupdate();
}


int magh, magv;
int pmode=0;

void CALLBACK GSwrite32(u32 mem, u32 value) {
#ifdef GS_LOG
	GS_LOG("GSwrite32 mem %x value %8.8lx\n", mem, value);
#endif

	switch (mem) {
		case 0x12000000: // PMODE
			if (value & 0x1)
				 pmode = 0;
			else pmode = 1;
			gsSetPmode(pmode);
			break;

		case 0x12000070: // DISPFB1
			_gsdspfb[0].fbp =((value      ) & 0x1ff) * 2048;
			_gsdspfb[0].fbw =((value >>  9) & 0x3f) * 64;
			_gsdspfb[0].psm = (value >> 15) & 0x1f;
			_gsdspfb[0].dbx = 0;
			_gsdspfb[0].dby = 0;
			if (_gsdspfb[0].fbw > 0) _gsdspfb[0].fbh = (1024*1024) / _gsdspfb[0].fbw;
			else _gsdspfb[0].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("dispfb1 fbw=%x, psm=%x\n", 
				   _gsdspfb[0].fbw, _gsdspfb[0].psm);
#endif
			break;

		case 0x12000090: // DISPFB2
			_gsdspfb[1].fbp =((value      ) & 0x1ff) * 2048;
			_gsdspfb[1].fbw =((value >>  9) & 0x3f) * 64;
			_gsdspfb[1].psm = (value >> 15) & 0x1f;
			_gsdspfb[1].dbx = 0;
			_gsdspfb[1].dby = 0;
			if (_gsdspfb[1].fbw > 0) _gsdspfb[1].fbh = (1024*1024) / _gsdspfb[1].fbw;
			else _gsdspfb[1].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("dispfb2 fbw=%x, psm=%x\n", 
				   _gsdspfb[1].fbw, _gsdspfb[1].psm);
#endif
			break;

		case 0x12001000: // CSR
//			printf("CSR64 %x\n", value);
			if (value & 0x2) {
				if (finish == -1) finish = 0;
			} else {
				finish = -1;
			}
			break;
	}
	gsSetPmode(pmode);
}

void CALLBACK GSwrite64(u32 mem, u64 value) {
	u32 *data = (u32*)&value;

#ifdef GS_LOG
	GS_LOG("GSwrite64 mem %x value %8.8lx_%8.8lx\n", mem, *(data+1), *data);
#endif
	switch (mem) {
		case 0x12000000: // PMODE
			if (*data & 0x1)
				 pmode = 0;
			else pmode = 1;
			gsSetPmode(pmode);
			break;

		case 0x12000020: // SMODE2
			smode2.inter = (data[0]     ) & 0x1;
			smode2.ffmd  = (data[0] >> 1) & 0x1;
			smode2.dpms  = (data[0] >> 2) & 0x3;
			break;

		case 0x12000070: // DISPFB1
			_gsdspfb[0].fbp =((data[0]      ) & 0x1ff) * 2048;
			_gsdspfb[0].fbw =((data[0] >>  9) & 0x3f) * 64;
			_gsdspfb[0].psm = (data[0] >> 15) & 0x1f;
			_gsdspfb[0].dbx = (data[1]      ) & 0x7ff;
			_gsdspfb[0].dby = (data[1] >> 11) & 0x7ff;
			if (_gsdspfb[0].fbw > 0) _gsdspfb[0].fbh = (1024*1024) / _gsdspfb[0].fbw;
			else _gsdspfb[0].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("dispfb2 fbw=%x, psm=%x, db %d,%d\n", 
				   _gsdspfb[0].fbw, _gsdspfb[0].psm, _gsdspfb[0].dbx, _gsdspfb[0].dby);
#endif
			break;

		case 0x12000080: // DISPLAY1
			magh = ((data[0] >> 23) & 0xf) + 1;
			magv = ((data[1] >> 27) & 0x3) + 1;
//			_gsmode[0].x = ((data[0]      ) & 0xfff) / magh;
//			_gsmode[0].y = ((data[0] >> 12) & 0x7ff) / magv;
			_gsmode[0].w = (((data[1]      ) & 0xfff) + 1) / magh;
			_gsmode[0].h = (((data[1] >> 12) & 0x7ff) + 1) / magv;
//			if(((smode2.inter == 0) || (smode2.ffmd == 1)) && (_gsmode[1].h > 256))
//				_gsmode[0].h >>= 1;
			if (smode2.inter && smode2.ffmd)
				_gsmode[1].h >>= 1;
			break;

		case 0x12000090: // DISPFB2
			_gsdspfb[1].fbp =((data[0]      ) & 0x1ff) * 2048;
			_gsdspfb[1].fbw =((data[0] >>  9) & 0x3f) * 64;
			_gsdspfb[1].psm = (data[0] >> 15) & 0x1f;
			_gsdspfb[1].dbx = (data[1]      ) & 0x7ff;
			_gsdspfb[1].dby = (data[1] >> 11) & 0x7ff;
			if (_gsdspfb[1].fbw > 0) _gsdspfb[1].fbh = (1024*1024) / _gsdspfb[1].fbw;
			else _gsdspfb[1].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("dispfb2 fbw=%x, psm=%x, db %d,%d\n", 
				   _gsdspfb[1].fbw, _gsdspfb[1].psm, _gsdspfb[1].dbx, _gsdspfb[1].dby);
#endif
			break;

		case 0x120000A0: // DISPLAY2
			magh = ((data[0] >> 23) & 0xf) + 1;
			magv = ((data[1] >> 27) & 0x3) + 1;
//			_gsmode[1].x = ((data[0]      ) & 0xfff) / magh;
//			_gsmode[1].y = ((data[0] >> 12) & 0x7ff) / magv;
			_gsmode[1].w = (((data[1]      ) & 0xfff) + 1) / magh;
			_gsmode[1].h = (((data[1] >> 12) & 0x7ff) + 1) / magv;
//			if(((smode2.inter == 0) || (smode2.ffmd == 1)) && (_gsmode[1].h > 256))
			if (smode2.inter && smode2.ffmd)
				_gsmode[1].h >>= 1;

#ifdef GS_LOG
			GS_LOG("display2 %8.8lx_%8.8lx: mode = %dx%d %dx%d, magh=%d, magv=%d\n", 
				   *(data+1), *data, 
				   _gsmode[1].x, _gsmode[1].y, _gsmode[1].w, _gsmode[1].h, magh, magv);
#endif
			break;

		case 0x12001000: // CSR
//			printf("CSR64 %x_%x\n", data[1], data[0]);
			if (*data & 0x2) {
				if (finish == -1) finish = 0;
			} else {
				finish = -1;
			}
			break;
	}
	gsSetPmode(pmode);
}

u32 CALLBACK GSread32(u32 mem) {
	u32 ret;

	switch (mem) {
		case 0x12001000: // CSR
			ret = 0x3000;
			ret = interlace << 13;
			if (finish == 1) {
				ret|= 0x2;
			}
			break;

		default:
			ret = 0;
			break;
	}

#ifdef GS_LOG
	GS_LOG("GSread32 mem %x ret %8.8lx\n", mem, ret);
#endif
	return ret;
}

u64 CALLBACK GSread64(u32 mem) {
	u64 ret;

	switch (mem) {
		case 0x12000400: // ???
//			interlace = 1 - interlace;
			ret = interlace << 13;
			break;

		case 0x12001000: // CSR
			ret = 0x3000;
			ret = interlace << 13;
			if (finish == 1) {
				ret|= 0x2;
			}
			break;

		default:
			ret = 0;
			break;
	}
#ifdef GS_LOG
	GS_LOG("GSread64 mem %x ret %8.8lx_%8.8lx\n", mem, ((u32*)&ret)[1], ((u32*)&ret)[0]);
#endif
	return ret;
}

void FBmoveImage() {
	Point s, d;
	int sw, sh, dw, dh;
	int x, y, w, h;
	u32 *src, *dst;

	s.x = trxpos.sx;
	s.y = trxpos.sy;
	d.x = trxpos.dx;
	d.y = trxpos.dy;
	sw = dw = imageW;
	sh = dh = imageH;

	dst = vRamUL + dstbuf.bp;
	src = vRamUL + srcbuf.bp;

	if (d.x >= dstbuf.bw || d.y >= dstbuf.bh) return;
	if (s.x >= srcbuf.bw || s.y >= srcbuf.bh) return;

	if ((d.x + dw) < 0 || (d.y + dh) < 0) return;
	if ((s.x + sw) < 0 || (s.y + sh) < 0) return;

	if (d.x < 0) { dw-= -d.x; d.x = 0; }
	if (d.y < 0) { dh-= -d.y; d.y = 0; }

	if (s.x < 0) { sw-= -s.x; s.x = 0; }
	if (s.y < 0) { sh-= -s.y; s.y = 0; }

	if ((d.x + dw) > dstbuf.bw) dw = dstbuf.bw;
	if ((d.y + dh) > dstbuf.bh) dh = dstbuf.bh;

	if ((s.x + sw) > srcbuf.bw) sw = srcbuf.bw;
	if ((s.y + sh) > srcbuf.bh) sh = srcbuf.bh;

	w = min(dw, sw);
	h = min(dh, sh);

	if (s.x < d.x) {
		for(y=0; y<h; y++) {
			for(x=w-1; x>=0; x--)
				dst[(d.y + y) * dstbuf.bw + d.x + x] = src[(s.y + y) * srcbuf.bw + s.x + x];
		}
	} else {
		for(y=0; y<h; y++) {
			for(x=0; x<w; x++)
				dst[(d.y + y) * dstbuf.bw + d.x + x] = src[(s.y + y) * srcbuf.bw + s.x + x];
		}
	}
}

__inline void stepGSvertex() {
	int i;

	primC--;

	for (i=0; i<primC; i++) {
		gsvertex[i].x = gsvertex[i+1].x;
		gsvertex[i].y = gsvertex[i+1].y;
		gsvertex[i].z = gsvertex[i+1].z;
		gsvertex[i].f = gsvertex[i+1].f;

		if (prim->tme) {
			if (prim->fst) {
				gsvertex[i].u = gsvertex[i+1].u;
				gsvertex[i].v = gsvertex[i+1].v;
			} else { 
				gsvertex[i].s = gsvertex[i+1].s;
				gsvertex[i].t = gsvertex[i+1].t;
				gsvertex[i].q = gsvertex[i+1].q;
			}
		}

		if (prim->iip) {
			gsvertex[i].rgba = gsvertex[i+1].rgba;
		}
	}
}

__inline void stepGSfanvertex() {
	int i;

	primC--;

	for (i=1; i<primC; i++) {
		gsvertex[i].x = gsvertex[i+1].x;
		gsvertex[i].y = gsvertex[i+1].y;
		gsvertex[i].z = gsvertex[i+1].z;
		gsvertex[i].f = gsvertex[i+1].f;

		if (prim->tme) {
			if (prim->fst) {
				gsvertex[i].u = gsvertex[i+1].u;
				gsvertex[i].v = gsvertex[i+1].v;
			} else {
				gsvertex[i].s = gsvertex[i+1].s;
				gsvertex[i].t = gsvertex[i+1].t;
				gsvertex[i].q = gsvertex[i+1].q;
			}
		}

		if (prim->iip) {
			gsvertex[i].rgba = gsvertex[i+1].rgba;
		}
	}
}
 
/*
 * GSwrite: 128bit wide
 */
void GSwrite(u32 *data, int reg) {

#ifdef GS_LOG
	GS_LOG("write %x: %8.8lx_%8.8lx\n", reg, data[1], data[0]);
#endif

	switch (reg) {
		case 0x00: // prim
			prim->prim = (data[0]      ) & 0x7;
			if (prac == 0) break;
//			prim->prim = (data[0]      ) & 0x7;
			prim->iip  = (data[0] >>  3) & 0x1;
			prim->tme  = (data[0] >>  4) & 0x1;
			prim->fge  = (data[0] >>  5) & 0x1;
			prim->abe  = (data[0] >>  6) & 0x1;
			prim->aai  = (data[0] >>  7) & 0x1;
			prim->fst  = (data[0] >>  8) & 0x1;
			prim->ctxt = (data[0] >>  9) & 0x1;
			prim->fix  = (data[0] >> 10) & 0x1;
			gsSetCtxt(prim->ctxt);
			primC = 0;
			break;

		case 0x01: // rgbaq
			gsvertex[primC].rgba = data[0];
			gsvertex[primC].q    = data[1];
			rgba = data[0];
			break;

		case 0x02: // st
			gsvertex[primC].s = data[0];
			gsvertex[primC].t = data[1];
			break;

		case 0x03: // uv
			gsvertex[primC].u = (data[0] >> 4) & 0x7ff;
			gsvertex[primC].v = (data[0] >> (16+4)) & 0x7ff;
			break;

		case 0x04: // xyzf2
			gsvertex[primC].x = (data[0] >> 4) & 0xfff;
			gsvertex[primC].y = (data[0] >> (16+4)) & 0xfff;
			gsvertex[primC].z = data[1] & 0xffffff;
			gsvertex[primC].f = data[1] >> 24;
			primC++;

			if (primC >= primTableC[prim->prim]) {
#ifdef GS_LOG
				GS_LOG("prim %x\n", prim->prim);
#endif
				if (!norender) {
					primTable[prim->prim](gsvertex);
				}

				if(prim->prim != 5)
					stepGSvertex();
				else
					stepGSfanvertex();
			}
			break;

		case 0x05: // xyz2
			gsvertex[primC].x = (data[0] >> 4) & 0xfff;
			gsvertex[primC].y = (data[0] >> (16+4)) & 0xfff;
			gsvertex[primC].z = data[1];
			primC++;

			if (primC >= primTableC[prim->prim]) {
#ifdef GS_LOG
				GS_LOG("prim %x\n", prim->prim);
#endif
				if (!norender) {
					primTable[prim->prim](gsvertex);
				}

				if(prim->prim != 5)
					stepGSvertex();
				else
					stepGSfanvertex();
			}
			break;

		case 0x06: // tex0_1
			_tex0[0].tbp0 =  (data[0]        & 0x3fff) * 64;
			_tex0[0].tbw  = ((data[0] >> 14) & 0x3f) * 64;
			_tex0[0].psm  =  (data[0] >> 20) & 0x3f;
			_tex0[0].tw   =  (data[0] >> 26) & 0xf;
			if (_tex0[0].tw > 10) _tex0[0].tw = 10;
			_tex0[0].tw   = (int)pow(2, (double)_tex0[0].tw);
			_tex0[0].th   = ((data[0] >> 30) & 0x3) | ((data[1] & 0x3) << 2);
			if (_tex0[0].th > 10) _tex0[0].th = 10;
			_tex0[0].th   = (int)pow(2, (double)_tex0[0].th);
			_tex0[0].tcc  =  (data[1] >>  2) & 0x1;
			_tex0[0].tfx  =  (data[1] >>  3) & 0x3;
			_tex0[0].cbp  = ((data[1] >>  5) & 0x3fff) * 64;
			_tex0[0].cpsm =  (data[1] >> 19) & 0xf;
			_tex0[0].csm  =  (data[1] >> 23) & 0x1;
#ifdef GS_LOG
			GS_LOG("tex0_1: tw=%d, th=%d\n", _tex0[0].tw, _tex0[0].th);
#endif
			break;

		case 0x07: // tex0_2
			_tex0[1].tbp0 =  (data[0]        & 0x3fff) * 64;
			_tex0[1].tbw  = ((data[0] >> 14) & 0x3f) * 64;
			_tex0[1].psm  =  (data[0] >> 20) & 0x3f;
			_tex0[1].tw   =  (data[0] >> 26) & 0xf;
			if (_tex0[1].tw > 10) _tex0[1].tw = 10;
			_tex0[1].tw   = (int)pow(2, (double)_tex0[1].tw);
			_tex0[1].th   = ((data[1] >> 30) & 0x3) | ((data[1] & 0x3) << 2);
			if (_tex0[1].th > 10) _tex0[1].th = 10;
			_tex0[1].th   = (int)pow(2, (double)_tex0[1].th);
			_tex0[1].tcc  =  (data[1] >>  2) & 0x1;
			_tex0[1].tfx  =  (data[1] >>  3) & 0x3;
			_tex0[1].cbp  = ((data[1] >>  5) & 0x3fff) * 64;
			_tex0[1].cpsm =  (data[1] >> 19) & 0xf;
			_tex0[1].csm  =  (data[1] >> 23) & 0x1;
#ifdef GS_LOG
			GS_LOG("tex0_2: tw=%d, th=%d\n", _tex0[1].tw, _tex0[1].th);
#endif
			break;

		case 0x08: // clamp_1
			_clamp[0].wms  = (data[0]      ) & 0x2;
			_clamp[0].wmt  = (data[0] >>  2) & 0x2;
			_clamp[0].minu = (data[0] >>  4) & 0x3ff;
			_clamp[0].maxu = (data[0] >> 14) & 0x3ff;
			_clamp[0].minv =((data[0] >> 24) & 0xff) | ((data[1] & 0x3) << 8);
			_clamp[0].maxv = (data[1] >> 2) & 0x3ff;
			break;

		case 0x09: // clamp_2
			_clamp[1].wms  = (data[0]      ) & 0x2;
			_clamp[1].wmt  = (data[0] >>  2) & 0x2;
			_clamp[1].minu = (data[0] >>  4) & 0x3ff;
			_clamp[1].maxu = (data[0] >> 14) & 0x3ff;
			_clamp[1].minv =((data[0] >> 24) & 0xff) | ((data[1] & 0x3) << 8);
			_clamp[1].maxv = (data[1] >> 2) & 0x3ff;
			break;

		case 0x0a: // fog
			fogf = data[1] >> 24;
			break;

		case 0x0c: // xyzf3
			gsvertex[primC].x = (data[0] >> 4) & 0xfff;
			gsvertex[primC].y = (data[0] >> (16+4)) & 0xfff;
			gsvertex[primC].z = data[1] & 0xffffff;
			gsvertex[primC].f = data[1] >> 24;
			primC++;
			if (primC >= primTableC[prim->prim]) {
				if(prim->prim != 5)
					stepGSvertex();
				else
					stepGSfanvertex();
			}

			break;

		case 0x0d: // xyz3
			gsvertex[primC].x = (data[0] >> 4) & 0xfff;
			gsvertex[primC].y = (data[0] >> (16+4)) & 0xfff;
			gsvertex[primC].z = data[1];
			primC++;
			if (primC >= primTableC[prim->prim]) {
				if(prim->prim != 5)
					stepGSvertex();
				else
					stepGSfanvertex();
			}
			break;

		case 0x14: // tex1_1
			_tex1[0].lcm  = (data[0]      ) & 0x1; 
			_tex1[0].mxl  = (data[0] >>  2) & 0x7;
			_tex1[0].mmag = (data[0] >>  5) & 0x1;
			_tex1[0].mmin = (data[0] >>  6) & 0x7;
			_tex1[0].mtba = (data[0] >>  9) & 0x1;
			_tex1[0].l    = (data[0] >> 19) & 0x3;
			_tex1[0].k    = (data[1] >> 4) & 0xff;
			break;

		case 0x15: // tex1_2
			_tex1[1].lcm  = (data[0]      ) & 0x1; 
			_tex1[1].mxl  = (data[0] >>  2) & 0x7;
			_tex1[1].mmag = (data[0] >>  5) & 0x1;
			_tex1[1].mmin = (data[0] >>  6) & 0x7;
			_tex1[1].mtba = (data[0] >>  9) & 0x1;
			_tex1[1].l    = (data[0] >> 19) & 0x3;
			_tex1[1].k    = (data[1] >> 4) & 0xff;
			break;

		case 0x16: // tex2_1
			_tex2[0].psm  = (data[0] >> 20) & 0x3f; 
			_tex2[0].cbp  = (data[1] >>  5) & 0x3fff;
			_tex2[0].cpsm = (data[1] >> 19) & 0xf;
			_tex2[0].csm  = (data[1] >> 23) & 0x1;
			_tex2[0].csa  = (data[1] >> 24) & 0x1f;
			_tex2[0].cld  = (data[1] >> 29) & 0x7;
			break;

		case 0x17: // tex2_2
			_tex2[1].psm  = (data[0] >> 20) & 0x3f; 
			_tex2[1].cbp  = (data[1] >>  5) & 0x3fff;
			_tex2[1].cpsm = (data[1] >> 19) & 0xf;
			_tex2[1].csm  = (data[1] >> 23) & 0x1;
			_tex2[1].csa  = (data[1] >> 24) & 0x1f;
			_tex2[1].cld  = (data[1] >> 29) & 0x7;
			break;

		case 0x18: // xyoffset_1
			_offset[0].x = (data[0] >> 4) & 0xfff;
			_offset[0].y = (data[1] >> 4) & 0xfff;
			break;

		case 0x19: // xyoffset_2
			_offset[1].x = (data[0] >> 4) & 0xfff;
			_offset[1].y = (data[1] >> 4) & 0xfff;
			break;

		case 0x1a: // prmodecont
			prac = data[0] & 0x1;
			prim = &_prim[prac];
			break;

		case 0x1b: // prmode
			if (prac == 1) break;
			prim->iip  = (data[0] >>  3) & 0x1;
			prim->tme  = (data[0] >>  4) & 0x1;
			prim->fge  = (data[0] >>  5) & 0x1;
			prim->abe  = (data[0] >>  6) & 0x1;
			prim->aai  = (data[0] >>  7) & 0x1;
			prim->fst  = (data[0] >>  8) & 0x1;
			prim->ctxt = (data[0] >>  9) & 0x1;
			prim->fix  = (data[0] >> 10) & 0x1;
			gsSetCtxt(prim->ctxt);
			primC = 0;
			break;

		case 0x1c: // texclut
			clut.cbw = (data[0]      ) & 0x3f;
			clut.cou = (data[0] >>  6) & 0x3f;
			clut.cov = (data[0] >> 12) & 0x3ff;
			break;

		case 0x22: // scanmsk
			smask = data[0] & 0x3;
			break;

		case 0x34: // miptbp1_1
			_miptbp0[0].tbp[0] = (data[0]      ) & 0x3fff;
			_miptbp0[0].tbw[0] = (data[0] >> 14) & 0x3f;
			_miptbp0[0].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			_miptbp0[0].tbw[1] = (data[1] >>  2) & 0x3f;
			_miptbp0[0].tbp[2] = (data[1] >>  8) & 0x3fff;
			_miptbp0[0].tbw[2] = (data[1] >> 22) & 0x3f;
			break;

		case 0x35: // miptbp1_2
			_miptbp0[1].tbp[0] = (data[0]      ) & 0x3fff;
			_miptbp0[1].tbw[0] = (data[0] >> 14) & 0x3f;
			_miptbp0[1].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			_miptbp0[1].tbw[1] = (data[1] >>  2) & 0x3f;
			_miptbp0[1].tbp[2] = (data[1] >>  8) & 0x3fff;
			_miptbp0[1].tbw[2] = (data[1] >> 22) & 0x3f;
			break;

		case 0x36: // miptbp2_1
			_miptbp1[0].tbp[0] = (data[0]      ) & 0x3fff;
			_miptbp1[0].tbw[0] = (data[0] >> 14) & 0x3f;
			_miptbp1[0].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			_miptbp1[0].tbw[1] = (data[1] >>  2) & 0x3f;
			_miptbp1[0].tbp[2] = (data[1] >>  8) & 0x3fff;
			_miptbp1[0].tbw[2] = (data[1] >> 22) & 0x3f;
			break;

		case 0x37: // miptbp2_2
			_miptbp1[1].tbp[0] = (data[0]      ) & 0x3fff;
			_miptbp1[1].tbw[0] = (data[0] >> 14) & 0x3f;
			_miptbp1[1].tbp[1] = ((data[0] >> 20) & 0xfff) | ((data[1] & 0x3) << 12);
			_miptbp1[1].tbw[1] = (data[1] >>  2) & 0x3f;
			_miptbp1[1].tbp[2] = (data[1] >>  8) & 0x3fff;
			_miptbp1[1].tbw[2] = (data[1] >> 22) & 0x3f;
			break;

		case 0x3b: // texa
			texa.ta[0] = data[0] & 0xff;
			texa.aem   = (data[0] >> 15) & 0x1;
			texa.ta[1] = data[1] & 0xff;
			break;

		case 0x3d: // fogcol
			fogcol = data[0] & 0xffffff;
			break;

		case 0x3f: // texflush
			break;

		case 0x40: // scissor_1
			_scissor[0].x0 = (data[0]      ) & 0x7ff;
			_scissor[0].x1 = (data[0] >> 16) & 0x7ff;
			_scissor[0].y0 = (data[1]      ) & 0x7ff;
			_scissor[0].y1 = (data[1] >> 16) & 0x7ff;
#ifdef GS_LOG
/*			GS_LOG("scissor_1 %8.8lx_%8.8lx: %dx%d - %dx%d\n", 
				   data[1], data[0], 
				   _scissor[0].x0, _scissor[0].y0, _scissor[0].x1, _scissor[0].y1);*/
#endif
			break;

		case 0x41: // scissor_2
			_scissor[1].x0 = (data[0]      ) & 0x7ff;
			_scissor[1].x1 = (data[0] >> 16) & 0x7ff;
			_scissor[1].y0 = (data[1]      ) & 0x7ff;
			_scissor[1].y1 = (data[1] >> 16) & 0x7ff;
#ifdef GS_LOG
/*			GS_LOG("scissor_2 %8.8lx_%8.8lx: %dx%d - %dx%d\n", 
				   data[1], data[0], 
				   _scissor[1].x0, _scissor[1].y0, _scissor[1].x1, _scissor[1].y1);*/
#endif
			break;

		case 0x42: // alpha_1
			_alpha[0].a   = (data[0]     ) & 0x3;
			_alpha[0].b   = (data[0] >> 2) & 0x3;
			_alpha[0].c   = (data[0] >> 4) & 0x3;
			_alpha[0].d   = (data[0] >> 6) & 0x3;
			_alpha[0].fix = (data[1]     ) & 0xff;
			break;

		case 0x43: // alpha_2
			_alpha[1].a   = (data[0]     ) & 0x3;
			_alpha[1].b   = (data[0] >> 2) & 0x3;
			_alpha[1].c   = (data[0] >> 4) & 0x3;
			_alpha[1].d   = (data[0] >> 6) & 0x3;
			_alpha[1].fix = (data[1]     ) & 0xff;
			break;

		case 0x44: // dimx
			break;

		case 0x45: // dthe
			dthe = data[0] & 0x1;
			break;

		case 0x46: // colclamp
			colclamp = data[0] & 0x1;
			break;

		case 0x47: // test_1
			_test[0].ate   = (data[0]      ) & 0x1;
			_test[0].atst  = (data[0] >>  1) & 0x7;
			_test[0].aref  = (data[0] >>  4) & 0xff;
			_test[0].afail = (data[0] >> 12) & 0x3;
			_test[0].date  = (data[0] >> 14) & 0x1;
			_test[0].datm  = (data[0] >> 15) & 0x1;
			_test[0].zte   = (data[0] >> 16) & 0x1;
			_test[0].ztst  = (data[0] >> 17) & 0x3;
			break;

		case 0x48: // test_2
			_test[1].ate   = (data[0]      ) & 0x1;
			_test[1].atst  = (data[0] >>  1) & 0x7;
			_test[1].aref  = (data[0] >>  4) & 0xff;
			_test[1].afail = (data[0] >> 12) & 0x3;
			_test[1].date  = (data[0] >> 14) & 0x1;
			_test[1].datm  = (data[0] >> 15) & 0x1;
			_test[1].zte   = (data[0] >> 16) & 0x1;
			_test[1].ztst  = (data[0] >> 17) & 0x3;
			break;

		case 0x49: // pabe
			pabe = *data & 0x1;
			break;

		case 0x4a: // fba_1
			_fba[0].fba = *data & 0x1;
			break;

		case 0x4b: // fba_2
			_fba[1].fba = *data & 0x1;
			break;

		case 0x4c: // frame_1
			_gsfb[0].fbp = ((data[0]      ) & 0x1ff) * 2048;
			_gsfb[0].fbw = ((data[0] >> 16) & 0x3f) * 64;
			_gsfb[0].psm =  (data[0] >> 24) & 0x3f;
			_gsfb[0].fbm = data[1];
			if (_gsfb[0].fbw > 0) _gsfb[0].fbh = (1024*1024) / _gsfb[0].fbw;
			else _gsfb[0].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("frame_1 %8.8lx_%8.8lx: fbp=%x fbw=%x psm=%x fbm=%x\n", 
				   data[1], data[0], 
				   _gsfb[0].fbp, _gsfb[0].fbw, _gsfb[0].psm, _gsfb[0].fbm);
#endif
			break;

		case 0x4d: // frame_2
			_gsfb[1].fbp = ((data[0]      ) & 0x1ff) * 2048;
			_gsfb[1].fbw = ((data[0] >> 16) & 0x3f) * 64;
			_gsfb[1].psm =  (data[0] >> 24) & 0x3f;
			_gsfb[1].fbm = data[1];
			if (_gsfb[1].fbw > 0) _gsfb[1].fbh = (1024*1024) / _gsfb[1].fbw;
			else _gsfb[1].fbh = 1024*1024;
#ifdef GS_LOG
			GS_LOG("frame_2 %8.8lx_%8.8lx: fbp=%x fbw=%x psm=%x fbm=%x\n", 
				   data[1], data[0], 
				   _gsfb[1].fbp, _gsfb[1].fbw, _gsfb[1].psm, _gsfb[1].fbm);
#endif
			break;

		case 0x4e: // ZBUF_1
			_zbuf[0].zbp = (data[0] & 0x1FF) * 2048;
			_zbuf[0].psm = (data[0] >> 24) & 0xF;
			_zbuf[0].zmsk = data[1] & 0x1;
			break;

		case 0x4f: // ZBUF_2
			_zbuf[1].zbp = (data[0] & 0x1FF) * 2048;
			_zbuf[1].psm = (data[0] >> 24) & 0xF;
			_zbuf[1].zmsk = data[1] & 0x1;
 			break;

		case 0x50: // bitbltbuf
			srcbuf.bp  = ((data[0]      ) & 0x3fff) * 64;
			srcbuf.bw  = ((data[0] >> 16) & 0x3f) * 64;
			srcbuf.psm =  (data[0] >> 24) & 0x3f;
			if (srcbuf.bw > 0) srcbuf.bh = (1024*1024) / srcbuf.bw;
			else srcbuf.bh = 1024*1024;
			dstbuf.bp  = ((data[1]      ) & 0x3fff) * 64;
			dstbuf.bw  = ((data[1] >> 16) & 0x3f) * 64;
			dstbuf.psm =  (data[1] >> 24) & 0x3f;
			if (dstbuf.bw > 0) dstbuf.bh = (1024*1024) / dstbuf.bw;
			else dstbuf.bh = 1024*1024;

			if (dstbuf.bw != 0) ldbw = dstbuf.bw;
			else dstbuf.bw = ldbw/32;
			break;

		case 0x51: // trxpos
			trxpos.sx  = (data[0]      ) & 0x7ff;
			trxpos.sy  = (data[0] >> 16) & 0x7ff;
			trxpos.dx  = (data[1]      ) & 0x7ff;
			trxpos.dy  = (data[1] >> 16) & 0x7ff;
			trxpos.dir = (data[1] >> 27) & 0x3;
#ifdef GS_LOG
			GS_LOG("write: %8.8lx_%8.8lx_%8.8lx_%8.8lx: %dx%d %dx%d : dir=%d\n", data[3], data[2], data[1], data[0],
				   trxpos.sx, trxpos.sy, trxpos.dx, trxpos.dy, trxpos.dir);
#endif
			break;

		case 0x52: // trxreg
			imageW = data[0];
			imageH = data[1];
			break;

		case 0x53: // trxdir
			imageTransfer = data[0] & 0x3;
			if (imageTransfer == 0x2) {
#ifdef GS_LOG
				GS_LOG("moveImage %dx%d %dx%d %dx%d (dir=%d)\n",
					   trxpos.sx, trxpos.sy, trxpos.dx, trxpos.dy, imageW, imageH, trxpos.dir);
#endif
				FBmoveImage();
				break;
			}
			if (imageTransfer == 1) {
				gifMode = 4;
				imagePtr = vRamUL + srcbuf.bp;
#ifdef GS_LOG
				GS_LOG("imageTransferSrc size %lx, %dx%d %dx%d (psm=%x, bp=%x)\n",
					   gtag.nloop, trxpos.sx, trxpos.sy, imageW, imageH, srcbuf.psm, srcbuf.bp);
#endif
				imageX = trxpos.sx;
				imageY = trxpos.sy;
			} else {
				imageX = trxpos.dx;
				imageY = trxpos.dy;
			}
			break;
		case 0x61: // finish
#ifdef GS_LOG
			GS_LOG("finish register\n");
#endif
			if (finish == 0) finish = 1;
			break;
		default:
//			printf("unhandled %x\n", reg);
			break;
	}
	gsSetCtxt(prim->ctxt);
}

u64 regs;
int regn;

__inline void TransferPixel(u32 pixel) {
	if (imageX >= 0 && imageX < dstbuf.bw &&
		imageY >= 0 && imageY < dstbuf.bh) {
		imagePtr[imageY * dstbuf.bw + imageX] = pixel;
	}
#ifdef GS_LOG
	 else GS_LOG("transfer err %d, %d\n", imageX, imageY);
#endif

	if (++imageX == (imageW + trxpos.dx)) {
		imageX = trxpos.dx;
		if (++imageY == (imageH + trxpos.dy)) {
#ifdef GS_LOG
			GS_LOG("imageY end\n");
#endif
			imageY = trxpos.dy;
		}
	}
}

__inline void TransferPixelSrc(u32 *pixel) {
	if (imageX >= 0 && imageX < srcbuf.bw &&
		imageY >= 0 && imageY < srcbuf.bh) {
		*pixel = imagePtr[imageY * srcbuf.bw + imageX];
	}
#ifdef GS_LOG
	 else GS_LOG("transfer err %d, %d\n", imageX, imageY);
#endif

	if (++imageX == (imageW + trxpos.sx)) {
		imageX = trxpos.sx;
		if (++imageY == (imageH + trxpos.sy)) {
#ifdef GS_LOG
			GS_LOG("imageY end\n");
#endif
			imageY = trxpos.sy;
		}
	}
}

void FBwrite(u32 *data) {
	TransferPixel(data[0]);
	TransferPixel(data[1]);
	TransferPixel(data[2]);
	TransferPixel(data[3]);
}

void FBwrite24_3(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++)
		TransferPixel((*(u32*)(data8+i*3)) & 0xffffff);
}

void FBread(u32 *data) {
	TransferPixelSrc(&data[0]);
	TransferPixelSrc(&data[1]);
	TransferPixelSrc(&data[2]);
	TransferPixelSrc(&data[3]);
}

u64 primP;

void GIFtag(u32 *data) {
	gtag.nloop = data[0] & 0x7fff;
	gtag.eop   = (data[0] >> 15) & 0x1;
	gtag.pre   = (data[1] >> 14) & 0x1;
	gtag.prim  = (data[1] >> 15) & 0x7ff;
	gtag.flg   = (data[1] >> 26) & 0x3;
	gtag.nreg  = data[1] >> 28;
	if (gtag.nreg == 0) gtag.nreg = 16;

#ifdef GS_LOG
	GS_LOG("GIFtag: %8.8lx_%8.8lx_%8.8lx_%8.8lx: EOP=%d, NLOOP=%x, FLG=%x, NREG=%d, PRE=%d\n",
		    data[3], data[2], data[1], data[0],
			gtag.eop, gtag.nloop, gtag.flg, gtag.nreg, gtag.pre);
#endif

	switch (gtag.flg) {
		case 0x0:
			regs = *(u64 *)(data+2);
			regn = 0;
			gifMode = 1;
			if (gtag.pre) {
				u32 *pprim = (u32*)&primP;

				pprim[0] = gtag.prim;
				pprim[1] = 0;
				
				GSwrite(pprim, 0);
			}
			break;

		case 0x1:
			regs = *(u64 *)(data+2);
			regn = 0;
			gifMode = 2;
			break;

		case 0x3:
		case 0x2:
//			if (dstbuf.bw == 0) printf("dstbuf == 0!!!\n");
			if (imageTransfer == 0x2) {
/*#ifdef GS_LOG
				GS_LOG("moveImage %dx%d %dx%d %dx%d (dir=%d)\n",
					   trxpos.sx, trxpos.sy, trxpos.dx, trxpos.dy, imageW, imageH, trxpos.dir);
#endif
				FBmoveImage();*/
				break;
			}
			gifMode = 3;
			imagePtr = vRamUL + dstbuf.bp;
#ifdef GS_LOG
			GS_LOG("imageTransfer size %lx, %dx%d %dx%d (psm=%x, bp=%x)\n",
				   gtag.nloop, trxpos.dx, trxpos.dy, imageW, imageH, dstbuf.psm, dstbuf.bp);
#endif
			break;
	}
}

u64 out;
u32 *pout = (u32*)&out;

void GIFprocessReg(u32 *data, int reg) {
//	GS_LOG("GIFprocessReg %d\n", reg);

	switch (reg) {
		case 0x0: // prim
			pout[0] = data[0];
			GSwrite(pout, 0x0);
			break;

		case 0x1: // rgbaq
			pout[0] = (data[0] & 0xff) | 
					 ((data[1] & 0xff) <<  8) |
					 ((data[2] & 0xff) << 16) |
					 ((data[3] & 0xff) << 24);
			pout[1] = gsvertex[primC].q;
			GSwrite(pout, 0x1);
			break;

		case 0x2: // st
			GSwrite(data, 0x2);
			pout[0] = gsvertex[primC].rgba;
			pout[1] = data[2];
			GSwrite(pout, 0x1);
			break;

		case 0x3: // uv
			pout[0] = (data[0] & 0x7fff) |
					 ((data[1] & 0x7fff) << 16);
			GSwrite(pout, 0x3);
			break;

		case 0x4: // xyzf2
			pout[0] = (data[0] & 0xffff) | 
				     ((data[1] & 0xffff) << 16);
			pout[1] = ((data[2] >> 4) & 0xffffff) | 
				      ((data[3] & 0xff00) << 16);
			if ((data[3] >> 15) & 0x1)
				 GSwrite(pout, 0xc);
			else GSwrite(pout, 0x4);
			break;

		case 0x5: //xyz2
			pout[0] = (data[0] & 0xffff) | 
				     ((data[1] & 0xffff) << 16);
			pout[1] = data[2];
			if ((data[3] >> 15) & 0x1)
				 GSwrite(pout, 0xd);
			else GSwrite(pout, 0x5);
			break;

		case 0xe: // ad
			GSwrite(data, data[2] & 0xff);
			break;

		case 0xf: // nop
			break;

		default:
#ifdef GS_LOG
			GS_LOG("UNHANDLED %x!!!\n",reg);
#endif
			break;
	}
}

void CALLBACK GSgifTransfer(u32 *pMem, u32 size) {

#ifdef GS_LOG
	GS_LOG("GSgifTransfer size = %lx (mode %d, gtag.nloop = %d)\n", size, gifMode, gtag.nloop);
#endif

	while (size) {
		switch (gifMode) {
			case 0: /* GIF TAG */
/*				if (gtag.eop == 1) {
					gtag.eop = 0;
					return;
				}*/
				GIFtag(pMem); pMem+= 4; size--;
				break;

			case 1: /* GIF PACKET */
				if (gtag.nloop == 0) { gifMode = 0; break; }
				while (size) {
					int reg = (int)((regs >> (regn*4)) & 0xf);

					GIFprocessReg(pMem, reg); pMem+=4; size--;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							if (gifMode != 4) gifMode = 0;
							break;
						}
					}
				}
				break;

			case 2: /* GIF REGLIST */
				if (gtag.nloop == 0) { gifMode = 0; break; }
				while (size) {
					int reg;

					reg = (int)((regs >> (regn*4)) & 0xf);
					GSwrite(pMem, reg); pMem+=2;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							if (gifMode != 4) gifMode = 0;
							pMem+=2; // padding
							size--;
							break;
						}
					}

					reg = (int)((regs >> (regn*4)) & 0xf);
					GSwrite(pMem, reg); pMem+=2; size--;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							if (gifMode != 4) gifMode = 0;
							break;
						}
					}
				}
				break;

			case 3: /* GIF IMAGE */
				if (dstbuf.psm == 1) {
					while (size) {
						if (size >= 3) {
							FBwrite24_3(pMem); pMem+= 12; size-=3;
						} else {
						}
						if (--gtag.nloop == 0) {
							gifMode = 0;
							break;
						}
					}
				} else
				while (size) {
					FBwrite(pMem); pMem+= 4; size--;
					if (--gtag.nloop == 0) {
						gifMode = 0;
						break;
					}
				}
				break;

			case 4: /* GIF IMAGE (FROM VRAM) */
				while (size) {
					FBread(pMem); pMem+= 4; size--;
/*					if (--gtag.nloop == 0) {
						gifMode = 0;
						break;
					}*/
				}
				gifMode = 0;
				break;
		}
	}
}

void CALLBACK GSgifTransfer2(u32 *pMem) {

#ifdef GS_LOG
	GS_LOG("GSgifTransfer2 (mode %d)\n", gifMode);
#endif

	gtag.eop = 0;
	for (;;) {
		switch (gifMode) {
			case 0: /* GIF TAG */
				if (gtag.eop == 1) {
					gtag.eop = 0;
					return;
				}
				GIFtag(pMem); pMem+= 4;
				break;

			case 1: /* GIF PACKET */
				if (gtag.nloop == 0) { gifMode = 0; break; }
				for (;;) {
					int reg = (int)((regs >> (regn*4)) & 0xf);

					GIFprocessReg(pMem, reg); pMem+=4;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							gifMode = 0;
							break;
						}
					}
				}
				break;

			case 2: /* GIF REGLIST */
				if (gtag.nloop == 0) { gifMode = 0; break; }
				for (;;) {
					int reg;

					reg = (int)((regs >> (regn*4)) & 0xf);
					GSwrite(pMem, reg); pMem+=2;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							gifMode = 0;
							pMem+=2; // padding
							break;
						}
					}

					reg = (int)((regs >> (regn*4)) & 0xf);
					GSwrite(pMem, reg); pMem+=2;
					regn++;
					if (gtag.nreg == regn) {
						regn = 0;

						if (--gtag.nloop == 0) {
							gifMode = 0;
							break;
						}
					}
				}
				break;

			case 3: /* GIF IMAGE */
				for (;;) {
					FBwrite(pMem); pMem+= 4;
					if (--gtag.nloop == 0) {
						gifMode = 0;
						break;
					}
				}
				break;
		}
	}
}

extern int fpspos;
extern int fpspress;



void CALLBACK GSkeyEvent(keyEvent *ev) {
	switch (ev->event) {
		case KEYPRESS:
			switch (ev->key) {
				case VK_PRIOR:
					if (conf.fps) fpspos++; break;
				case VK_NEXT:
					if (conf.fps) fpspos--; break;
				case VK_END:
					if (conf.fps) fpspress = 1; break;
				case VK_DELETE:
					conf.fps = 1 - conf.fps;
					if (!conf.stretch) DXclearScr();
					break;
			}
			break;
	}
}

#include "resource.h"
SDL_Rect **sdlmodes;



BOOL CALLBACK ConfigureDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hWC;
	char str[256];
	int i;

	switch(uMsg) {
		case WM_INITDIALOG:
			LoadConfig();
            sdlmodes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE | SDL_DOUBLEBUF);//hmmm
            hWC = GetDlgItem(hW, IDC_DSPRES);
			for (i=0; sdlmodes[i]; ++i) {
				sprintf(str, "%d x %d", sdlmodes[i]->w, sdlmodes[i]->h);
				modes[i].width= sdlmodes[i]->w;
				modes[i].height=sdlmodes[i]->h;
				ComboBox_AddString(hWC, str);
			}
			
			for (i=0; sdlmodes[i]; ++i) {
				if (memcmp(&conf.mode, &modes[i], sizeof(SDLmode)) == 0) {
					ComboBox_SetCurSel(hWC, i); break;
				}
			}


		//	if (conf.fullscreen) CheckDlgButton(hW, IDC_FULLSCREEN, TRUE);
			if (conf.bpp ==16)   CheckDlgButton(hW, IDC_16BIT, TRUE);
			if (conf.bpp ==32)   CheckDlgButton(hW, IDC_32BIT, TRUE);
			if (conf.fps)        CheckDlgButton(hW, IDC_FPSCOUNT, TRUE);
			if (conf.frameskip)  CheckDlgButton(hW, IDC_FRAMESKIP, TRUE);
			if (!conf.stretch)   CheckDlgButton(hW, IDC_STRETCH, TRUE);

			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
				case IDOK:
					hWC = GetDlgItem(hW, IDC_DSPRES);
					i = ComboBox_GetCurSel(hWC);
					memcpy(&conf.mode, &modes[i], sizeof(SDLmode));
             
                /*
					if (IsDlgButtonChecked(hW, IDC_FULLSCREEN))
						 conf.fullscreen = 1;
					else conf.fullscreen = 0;*/
					if(IsDlgButtonChecked(hW, IDC_16BIT))
						conf.bpp = 16;
					else conf.bpp=32;
				
					if (IsDlgButtonChecked(hW, IDC_FPSCOUNT))
						 conf.fps = 1;
					else conf.fps = 0;

					if (IsDlgButtonChecked(hW, IDC_FRAMESKIP))
						 conf.frameskip = 1;
					else conf.frameskip = 0;

					if (IsDlgButtonChecked(hW, IDC_STRETCH))
						 conf.stretch = 0;
					else conf.stretch = 1;

					SaveConfig();
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hW, FALSE);
					return TRUE;
			}
	}
	return FALSE;
}

void CALLBACK GSconfigure() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_CONFIG),
              GetActiveWindow(),  
              (DLGPROC)ConfigureDlgProc);
}

s32 CALLBACK GStest() {
	return 0;
}

void CALLBACK GSabout() {
    DialogBox(hInst,
              MAKEINTRESOURCE(IDD_ABOUT),
              GetActiveWindow(),  
              (DLGPROC)AboutDlgProc);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}


