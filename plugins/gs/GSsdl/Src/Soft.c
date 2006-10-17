#include <math.h>

#include "GS.h"

#undef ABS
#define ABS(a) (((a)<0) ? -(a) : (a))

__inline int shl10idiv(int x, int y) {
	s64 bi = x;
	bi <<= 10;
	return (int)(bi / y);
}

__inline s64 shl10idiv64(s64 x, s64 y) {
	s64 bi = x;
	bi <<= 10;
	return (s64)(bi / y);
}

/* blend 2 colours together and return the result */
__inline u32 blender(const u8 *A, const u8 *B, const u32 C, const u8 *D) {
	int i;
	u32 res = 0;

	for (i = 0; i < 4; i++) {
		u32 temp;

		temp = ((((u32) A[i] -  (u32) B[i]) * C) >> 7) + D[i];
		if(temp > 0xFF) {
			temp = 0xFF;
		}

		res |= temp << (i << 3);
	}

	return res;
}

void (*drawPixel) (int, int, u32);
void (*drawPixel2) (int, int, u32);

void _drawPixel(int x, int y, u32 p) {
	fBufUL[y * gsfb->fbw + x] = p;
}


#define _drawPixelA(name, testm) \
	void _drawPixelA_##name(int x, int y, u32 p) { \
		if ((p >> 24) testm test->aref) \
			fBufUL[y * gsfb->fbw + x] = p; \
	}

void _drawPixelA_F(int x, int y, u32 p) {
}

_drawPixelA(L, <)
_drawPixelA(LE, <=)
_drawPixelA(E, ==)
_drawPixelA(GE, >=)
_drawPixelA(G, >)
_drawPixelA(NE, !=)

void (*drawPixelT[]) (int, int, u32) = {
	_drawPixel,
	_drawPixelA_F, _drawPixel,
	_drawPixelA_L, _drawPixelA_LE,
	_drawPixelA_E, _drawPixelA_GE, _drawPixelA_G, _drawPixelA_NE
};

void _drawPixelABE(int x, int y, u32 p) {
	u32 A, B, C, D, c;
	u32 Cs = p;
	u32 Cd = fBufUL[y * gsfb->fbw + x];

	switch (alpha->a) {
		case 0x00: A = Cs & 0xffffff; break;
		case 0x01: A = Cd & 0xffffff; break;
		default:   A = 0; break;
	}

	switch (alpha->b) {
		case 0x00: B = Cs & 0xffffff; break;
		case 0x01: B = Cd & 0xffffff; break;
		default:   B = 0; break;
	}

	switch (alpha->c) {
		case 0x00: C = Cs >> 24; break;
		case 0x01: C = Cd >> 24; break;
		case 0x02: C = alpha->fix; break;
		default:   C = 0; break;
	}

	switch (alpha->d) {
		case 0x00: D = Cs & 0xffffff; break;
		case 0x01: D = Cd & 0xffffff; break;
		default:   D = 0; break;
	}
//	C|= fba->fba << 8;

	c = blender((u8 *) &A, (u8 *) &B, C, (u8 *) &D);

	drawPixel2(x, y, c);
}

static __inline void SETdrawPixel() {
	drawPixel = drawPixelT[(test->ate == 1 ? (test->atst+1) : 0)];
/*	if (prim->abe) { // alpha blending (experimental)
		drawPixel2 = drawPixel;
		drawPixel = _drawPixelABE;
	}*/
}

int _av, _rv, _gv, _bv; /* Colour of vertex */

static __inline void colModulateSetCol(u32 crgba) {
	_av = (crgba >> 24) & 0xFF;
	_rv = (crgba >> 16) & 0xFF;
	_gv = (crgba >> 8) & 0xFF;
	_bv = crgba & 0xFF;
}

static __inline u32 colModulate(u32 texcol) {
	u32 at, rt, gt, bt; /* Colour of texture */

	at = (texcol >> 24) & 0xFF;
	rt = (texcol >> 16) & 0xFF;
	gt = (texcol >> 8) & 0xFF;
	bt = texcol & 0xFF; /* Extract colours from texture */

	at = (at * _av) >> 7; /* Multiply by colour value */
	rt = (rt * _rv) >> 7;
	gt = (gt * _gv) >> 7;
	bt = (bt * _bv) >> 7;
	if(at > 0xFF)
		at = 0xFF;
	if(rt > 0xFF)
		rt = 0xFF;
	if(gt > 0xFF)
		gt = 0xFF;
	if(bt > 0xFF)
		bt = 0xFF;

	return (at << 24) | (rt << 16) | (gt << 8) | bt;
}

//

void drawLineF(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;

#ifdef GS_LOG
	GS_LOG("drawLineF %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, rgba);
#endif

	SETdrawPixel();

	if (ax > ay) {
		int d = ay - (ax >> 1);
		while (x != v[1].x) {
			drawPixel(x, y, rgba);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {
		int d = ax - (ay >> 1);
		while (y != v[1].y) {
			drawPixel(x, y, rgba);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

void drawLineG(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;

#ifdef GS_LOG
	GS_LOG("drawLineG %dx%d %x - %dx%d %x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[0].rgba, v[1].rgba);
#endif

	if (ax > ay) {
		int d = ay - (ax >> 1);
		while (x != v[1].x) {
			drawPixel(x, y, rgba);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {
		int d = ax - (ay >> 1);
		while (y != v[1].y) {
			drawPixel(x, y, rgba);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

typedef struct SOFTVTAG {
	int x, y;
	int u, v;
	long R, G, B;
	s64 z; /* 64 bit z value (as has to be fxp and could be 32bit) */
} soft_vertex;

static soft_vertex vtx[4];
static soft_vertex *left_array[4], *right_array[4];
static int left_section, right_section;
static int left_section_height, right_section_height;
static int left_x, delta_left_x, right_x, delta_right_x;
static int left_u, delta_left_u, left_v, delta_left_v;
static int right_u, delta_right_u, right_v, delta_right_v;
static int left_R, delta_left_R, delta_right_R;
static int left_G, delta_left_G, delta_right_G;
static int left_B, delta_left_B, delta_right_B;
static s64 left_z, right_z, delta_left_z, delta_right_z;

int Ymin;
int Ymax;
int drawX, drawY, drawW, drawH;

////////////////////////////////////////////////////////////////////////

__inline int RightSection_F(void) {
	soft_vertex *v1 = right_array[right_section];
	soft_vertex *v2 = right_array[right_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;

	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_F(void) {
	soft_vertex *v1 = left_array[left_section];
	soft_vertex *v2 = left_array[left_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;

	left_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_F(void) {
	if (--left_section_height <= 0) {
		if (--left_section <= 0) {
			return TRUE;
		}
		if (LeftSection_F() <= 0) {
			return TRUE;
		}
	} else {
		left_x += delta_left_x;
	}

	if (--right_section_height <= 0) {
		if (--right_section <= 0) {
			return TRUE;
		}
		if (RightSection_F() <= 0) {
			return TRUE;
		}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_F(int x1, int y1, int x2, int y2, int x3, int y3) {
	soft_vertex *v1, *v2, *v3;
	int height, longest;

	v1 = vtx;
	v1->x = x1 << 16;
	v1->y = y1;
	v2 = vtx + 1;
	v2->x = x2 << 16;
	v2->y = y2;
	v3 = vtx + 2;
	v3->x = x3 << 16;
	v3->y = y3;

	if (v1->y > v2->y) {
		soft_vertex *v = v1;
		v1 = v2;
		v2 = v;
	}
	if (v1->y > v3->y) {
		soft_vertex *v = v1;
		v1 = v3;
		v3 = v;
	}
	if (v2->y > v3->y) {
		soft_vertex *v = v2;
		v2 = v3;
		v3 = v;
	}

	height = v3->y - v1->y;
	if (height == 0) {
		return FALSE;
	}
	longest = (((v2->y - v1->y) << 16) / height) *
		((v3->x - v1->x) >> 16) + (v1->x - v2->x);
	if (longest == 0) {
		return FALSE;
	}

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section = 2;
		left_array[0] = v3;
		left_array[1] = v1;
		left_section = 1;

		if (LeftSection_F() <= 0)
			return FALSE;
		if (RightSection_F() <= 0) {
			right_section--;
			if (RightSection_F() <= 0)
				return FALSE;
		}
	} else {
		left_array[0] = v3;
		left_array[1] = v2;
		left_array[2] = v1;
		left_section = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section = 1;

		if (RightSection_F() <= 0)
			return FALSE;
		if (LeftSection_F() <= 0) {
			left_section--;
			if (LeftSection_F() <= 0)
				return FALSE;
		}
	}

	Ymin = v1->y;
	Ymax = min(v3->y - 1, drawH);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// POLY 3/4 FLAT SHADED
////////////////////////////////////////////////////////////////////////

void drawTriangleF(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;

#ifdef GS_LOG
	GS_LOG("drawTriangleF %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, rgba);
#endif

	drawX = scissor->x0; drawW = scissor->x1 + 1;
	drawY = scissor->y0; drawH = scissor->y1 + 1;

	if (!SetupSections_F(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y))
		return;

	ymax = Ymax;

	for (ymin = Ymin; ymin < drawY; ymin++)
		if (NextRow_F())
			return;
	SETdrawPixel();

	for (i = ymin; i <= ymax; i++) {
		xmin = left_x >> 16;
		if (drawX > xmin)
			xmin = drawX;
		xmax = (right_x >> 16) - 1;
		if (drawW < xmax)
			xmax = drawW;

		for (j = xmin; j <= xmax; j++) {
			drawPixel(j, i, rgba);
		}

		if (NextRow_F())
			return;
	}
}

////////////////////////////////////////////////////////////////////////
/* Z Buffered Flat Shaded Triangle */

__inline int RightSection_F_Z(void) {
	soft_vertex *v1 = right_array[right_section];
	soft_vertex *v2 = right_array[right_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;
	
	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_F_Z(void) {
	soft_vertex *v1 = left_array[left_section];
	soft_vertex *v2 = left_array[left_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;
	delta_left_z = (v2->z - v1->z) / height;
	left_z = v1->z;

	left_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_F_Z(void) {
	if (--left_section_height <= 0) {
		if (--left_section <= 0) {
			return TRUE;
		}
		if (LeftSection_F_Z() <= 0) {
			return TRUE;
		}
	} else {
		left_x += delta_left_x;
		left_z += delta_left_z;
	}

	if (--right_section_height <= 0) {
		if (--right_section <= 0) {
			return TRUE;
		}
		if (RightSection_F_Z() <= 0) {
			return TRUE;
		}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_F_Z(int x1, int y1, u32 z1,int x2, int y2, u32 z2, int x3, int y3, u32 z3) {
	soft_vertex *v1, *v2, *v3;
	int height, longest, temp;

	v1 = vtx;
	v1->x = x1 << 16;
	v1->y = y1;
	v1->z = (s64) z1; /* Assign z values */
	v1->z = v1->z << 16;

	v2 = vtx + 1;
	v2->x = x2 << 16;
	v2->y = y2;
	v2->z = (s64) z2;
	v2->z = v2->z << 16;

	v3 = vtx + 2;
	v3->x = x3 << 16;
	v3->y = y3;
	v3->z = (s64) z3;
	v3->z = v3->z << 16;

	if (v1->y > v2->y) {
		soft_vertex *v = v1;
		v1 = v2;
		v2 = v;
	}
	if (v1->y > v3->y) {
		soft_vertex *v = v1;
		v1 = v3;
		v3 = v;
	}
	if (v2->y > v3->y) {
		soft_vertex *v = v2;
		v2 = v3;
		v3 = v;
	}

	height = v3->y - v1->y;
	if (height == 0) {
		return FALSE;
	}

	temp = (((v2->y - v1->y) << 16) / height);
	longest = temp * ((v3->x - v1->x) >> 16) + (v1->x - v2->x);

	if (longest == 0) {
		return FALSE;
	}

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section = 2;
		left_array[0] = v3;
		left_array[1] = v1;
		left_section = 1;

		if (LeftSection_F_Z() <= 0)
			return FALSE;
		if (RightSection_F_Z() <= 0) {
			right_section--;
			if (RightSection_F_Z() <= 0)
				return FALSE;
		}
	} else {
		left_array[0] = v3;
		left_array[1] = v2;
		left_array[2] = v1;
		left_section = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section = 1;

		if (RightSection_F_Z() <= 0)
			return FALSE;
		if (LeftSection_F_Z() <= 0) {
			left_section--;
			if (LeftSection_F_Z() <= 0)
				return FALSE;
		}
	}

	Ymin = v1->y;
	Ymax = min(v3->y - 1, drawH);

	delta_right_z = shl10idiv64(temp * ((v3->z - v1->z) >> 10) + ((v1->z - v2->z) << 6), longest);
	/* Calculate the Z increment per line */
		
	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// POLY 3/4 FLAT SHADED - Z BUFFERED
////////////////////////////////////////////////////////////////////////

void drawTriangleF_Z(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;
	s64 difz, currz;
	u32 zval;
	int offs;

#ifdef GS_LOG
	GS_LOG("drawTriangleF %dx%dx%lx - %dx%dx%lx - %dx%dx%lx rgba=%x\n",
		   v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, v[0].rgba);
#endif

	drawX = scissor->x0; drawW = scissor->x1 + 1;
	drawY = scissor->y0; drawH = scissor->y1 + 1;

	if (!SetupSections_F_Z(v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z))
		return;

	ymax = Ymax;
	difz = delta_right_z;

	for (ymin = Ymin; ymin < drawY; ymin++)
		if (NextRow_F_Z())
			return;
	offs = ymin * gsfb->fbw;
	SETdrawPixel();

	switch (test->ztst) {
		case ZTST_NEVER:
			return;

		case ZTST_ALWAYS:
			for (i = ymin; i <= ymax; i++) {
				xmin = left_x >> 16;
				if (drawX > xmin)
					xmin = drawX;
				xmax = (right_x >> 16) - 1;
				if (drawW < xmax)
					xmax = drawW;
							
				currz = left_z;

				for (j = xmin; j <= xmax; j++) {
					zval = (u32) (currz >> 16);
					drawPixel(j, i, rgba);
					zBufUL[offs + j] = zval;

					currz += difz;
				}
				offs += gsfb->fbw;

				if (NextRow_F_Z())
					return;
			}
			break;

		case ZTST_GEQUAL:
			for (i = ymin; i <= ymax; i++) {
				xmin = left_x >> 16;
				if (drawX > xmin)
					xmin = drawX;
				xmax = (right_x >> 16) - 1;
				if (drawW < xmax)
					xmax = drawW;

				currz = left_z;

				for (j = xmin; j <= xmax; j++) {
					zval = (u32) (currz >> 16);
					if (zval >= zBufUL[(i * gsfb->fbw) + j]) {
						drawPixel(j, i, rgba);
						zBufUL[offs + j] = zval;
					}

					currz += difz;
				}

				offs += gsfb->fbw;

				if (NextRow_F_Z())
					return;
			}
			break;

		case ZTST_GREATER: 
			for (i = ymin; i <= ymax; i++) {
				xmin = left_x >> 16;
				if (drawX > xmin)
					xmin = drawX;
				xmax = (right_x >> 16) - 1;
				if (drawW < xmax)
					xmax = drawW;

				currz = left_z;

				for (j = xmin; j <= xmax; j++) {
					zval = (u32) (currz >> 16);
					if (zval > zBufUL[(i * gsfb->fbw) + j]) {
						drawPixel(j, i, rgba);
						zBufUL[offs + j] = zval;
					}

					currz += difz;
				}

				offs += gsfb->fbw;

				if (NextRow_F_Z())
					return;
			}
			break;
	}
}

////////////////////////////////////////////////////////////////////////

__inline int RightSection_FT(void) {
	soft_vertex *v1 = right_array[right_section];
	soft_vertex *v2 = right_array[right_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;

	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_FT(void) {
	soft_vertex *v1 = left_array[left_section];
	soft_vertex *v2 = left_array[left_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;

	delta_left_u = ((v2->u - v1->u)) / height;
	left_u = v1->u;
	delta_left_v = ((v2->v - v1->v)) / height;
	left_v = v1->v;

	left_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_FT(void) {
	if (--left_section_height <= 0) {
		if (--left_section <= 0) {
			return TRUE;
		}
		if (LeftSection_FT() <= 0) {
			return TRUE;
		}
	} else {
		left_x += delta_left_x;
		left_u += delta_left_u;
		left_v += delta_left_v;
	}

	if (--right_section_height <= 0) {
		if (--right_section <= 0) {
			return TRUE;
		}
		if (RightSection_FT() <= 0) {
			return TRUE;
		}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_FT(int x1, int y1, int x2, int y2, int x3, int y3,
							   int tx1, int ty1, int tx2, int ty2, int tx3, int ty3) {
	soft_vertex *v1, *v2, *v3;
	int height, longest, temp;

	v1 = vtx;
	v1->x = x1 << 16;
	v1->y = y1;
	v1->u = tx1 << 16;
	v1->v = ty1 << 16;
	v2 = vtx + 1;
	v2->x = x2 << 16;
	v2->y = y2;
	v2->u = tx2 << 16;
	v2->v = ty2 << 16;
	v3 = vtx + 2;
	v3->x = x3 << 16;
	v3->y = y3;
	v3->u = tx3 << 16;
	v3->v = ty3 << 16;

	if (v1->y > v2->y) {
		soft_vertex *v = v1;
		v1 = v2;
		v2 = v;
	}
	if (v1->y > v3->y) {
		soft_vertex *v = v1;
		v1 = v3;
		v3 = v;
	}
	if (v2->y > v3->y) {
		soft_vertex *v = v2;
		v2 = v3;
		v3 = v;
	}

	height = v3->y - v1->y;
	if (height == 0) {
		return FALSE;
	}

	temp = (((v2->y - v1->y) << 16) / height);
	longest = temp * ((v3->x - v1->x) >> 16) + (v1->x - v2->x);

	if (longest == 0) {
		return FALSE;
	}

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section = 2;
		left_array[0] = v3;
		left_array[1] = v1;
		left_section = 1;

		if (LeftSection_FT() <= 0)
			return FALSE;
		if (RightSection_FT() <= 0) {
			right_section--;
			if (RightSection_FT() <= 0)
				return FALSE;
		}
		if (longest > -0x1000)
			longest = -0x1000;
	} else {
		left_array[0] = v3;
		left_array[1] = v2;
		left_array[2] = v1;
		left_section = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section = 1;

		if (RightSection_FT() <= 0)
			return FALSE;
		if (LeftSection_FT() <= 0) {
			left_section--;
			if (LeftSection_FT() <= 0)
				return FALSE;
		}
		if (longest < 0x1000)
			longest = 0x1000;
	}

	Ymin = v1->y;
	Ymax = min(v3->y - 1, scissor->y1);

	delta_right_u =
		shl10idiv(temp * ((v3->u - v1->u) >> 10) + ((v1->u - v2->u) << 6),
				  longest);
	delta_right_v =
		shl10idiv(temp * ((v3->v - v1->v) >> 10) + ((v1->v - v2->v) << 6),
				  longest);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// POLY 3 F-SHADED TEX 15 BIT
////////////////////////////////////////////////////////////////////////

void drawTriangleFTDecal(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;
	long difX, difY;
	long posX, posY;
	u32 *tex;

#ifdef GS_LOG
	GS_LOG("drawTriangleFTDecal %dx%dx%d - %dx%dx%d - %dx%dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[0].z,
		   v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	tex = vRamUL + tex0->tbp0;

	if (!SetupSections_FT(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y,
						  v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v))
		return;

	ymax = Ymax;

	for (ymin = Ymin; ymin < scissor->y0; ymin++)
		if (NextRow_FT())
			return;

	difX = delta_right_u;
	difY = delta_right_v;
	SETdrawPixel();

	for (i = ymin; i <= ymax; i++) {
		xmin = (left_x >> 16);
		xmax = (right_x >> 16) - 1;
		if (scissor->x1 < xmax)
			xmax = scissor->x1;

		if (xmax >= xmin) {
			posX = left_u;
			posY = left_v;

			if (xmin < scissor->x0) {
				j = scissor->x0 - xmin;
				xmin = scissor->x0;
				posX += j * difX;
				posY += j * difY;
			}

			for (j = xmin; j <= xmax; j ++) {
				drawPixel(j, i, tex[((posY >> 16) * tex0->tbw) + (posX >> 16)]);
				posX += difX;
				posY += difY;
			}
		}
		if (NextRow_FT()) {
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////
// POLY 3 F-SHADED TEX 15 BIT
////////////////////////////////////////////////////////////////////////

void drawTriangleFTModulate(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;
	long difX, difY;
	long posX, posY;
	u32 *tex;
	u32 rv, gv, bv; /* Colour of vertex */
	u32 rt, gt, bt; /* Colour of texture */

#ifdef GS_LOG
	GS_LOG("drawTriangleFTModulate %dx%dx%d - %dx%dx%d - %dx%dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[0].z,
		   v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, v[0].rgba);
#endif

	rv = (rgba >> 16) & 0xFF;
	gv = (rgba >> 8) & 0xFF;
	bv = rgba & 0xFF;

	tex = vRamUL + tex0->tbp0;

	if (!SetupSections_FT(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y,
						  v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v))
		return;

	ymax = Ymax;

	for (ymin = Ymin; ymin < scissor->y0; ymin++)
		if (NextRow_FT())
			return;

	difX = delta_right_u;
	difY = delta_right_v;
	SETdrawPixel();

	for (i = ymin; i <= ymax; i++) {
		xmin = (left_x >> 16);
		xmax = (right_x >> 16) - 1;
		if (scissor->x1 < xmax)
			xmax = scissor->x1;

		if (xmax >= xmin) {
			posX = left_u;
			posY = left_v;

			if (xmin < scissor->x0) {
				j = scissor->x0 - xmin;
				xmin = scissor->x0;
				posX += j * difX;
				posY += j * difY;
			}

			for (j = xmin; j <= xmax; j ++) {
				u32 texcol;
	
				texcol = tex[((posY >> 16) * tex0->tbw) + (posX >> 16)];
				rt = (texcol >> 16) & 0xFF;
				gt = (texcol >> 8) & 0xFF;
				bt = texcol & 0xFF; /* Extract colours from texture */

				rt = (rt * rv) >> 7; /* Multiply by colour value */
				gt = (gt * gv) >> 7;
				bt = (bt * bv) >> 7;
				if(rt > 0xFF)
					rt = 0xFF;
				if(gt > 0xFF)
					gt = 0xFF;
				if(bt > 0xFF)
					bt = 0xFF;

				drawPixel(j, i, (rt << 16) | (gt << 8) | bt);
				posX += difX;
				posY += difY;
			}
		}
		if (NextRow_FT()) {
			return;
		}
	}
}

/* FT triangle with Z buffer */

__inline int RightSection_FT_Z(void) {
	soft_vertex *v1 = right_array[right_section];
	soft_vertex *v2 = right_array[right_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;

	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_FT_Z(void) {
	soft_vertex *v1 = left_array[left_section];
	soft_vertex *v2 = left_array[left_section - 1];

	int height = v2->y - v1->y;
	if (height == 0)
		return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;

	delta_left_u = ((v2->u - v1->u)) / height;
	left_u = v1->u;
	delta_left_v = ((v2->v - v1->v)) / height;
	left_v = v1->v;

	delta_left_z = (v2->z - v1->z) / height;
	left_z = v1->z;

	left_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_FT_Z(void) {
	if (--left_section_height <= 0) {
		if (--left_section <= 0) {
			return TRUE;
		}
		if (LeftSection_FT_Z() <= 0) {
			return TRUE;
		}
	} else {
		left_x += delta_left_x;
		left_u += delta_left_u;
		left_v += delta_left_v;
		left_z += delta_left_z;
	}

	if (--right_section_height <= 0) {
		if (--right_section <= 0) {
			return TRUE;
		}
		if (RightSection_FT_Z() <= 0) {
			return TRUE;
		}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_FT_Z(int x1, int y1, u32 z1, int x2, int y2, u32 z2, int x3, int y3, u32 z3,
							   int tx1, int ty1, int tx2, int ty2, int tx3, int ty3) {
	soft_vertex *v1, *v2, *v3;
	int height, longest, temp;

	v1 = vtx;
	v1->x = x1 << 16;
	v1->y = y1;
	v1->u = tx1 << 16;
	v1->v = ty1 << 16;
	v1->z = (s64) z1; /* Assign z values */
	v1->z = v1->z << 16;

	v2 = vtx + 1;
	v2->x = x2 << 16;
	v2->y = y2;
	v2->u = tx2 << 16;
	v2->v = ty2 << 16;
	v2->z = (s64) z2; /* Assign z values */
	v2->z = v2->z << 16;

	v3 = vtx + 2;
	v3->x = x3 << 16;
	v3->y = y3;
	v3->u = tx3 << 16;
	v3->v = ty3 << 16;
	v3->z = (s64) z3; /* Assign z values */
	v3->z = v3->z << 16;

	if (v1->y > v2->y) {
		soft_vertex *v = v1;
		v1 = v2;
		v2 = v;
	}
	if (v1->y > v3->y) {
		soft_vertex *v = v1;
		v1 = v3;
		v3 = v;
	}
	if (v2->y > v3->y) {
		soft_vertex *v = v2;
		v2 = v3;
		v3 = v;
	}

	height = v3->y - v1->y;
	if (height == 0) {
		return FALSE;
	}

	temp = (((v2->y - v1->y) << 16) / height);
	longest = temp * ((v3->x - v1->x) >> 16) + (v1->x - v2->x);

	if (longest == 0) {
		return FALSE;
	}

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section = 2;
		left_array[0] = v3;
		left_array[1] = v1;
		left_section = 1;

		if (LeftSection_FT_Z() <= 0)
			return FALSE;
		if (RightSection_FT_Z() <= 0) {
			right_section--;
			if (RightSection_FT_Z() <= 0)
				return FALSE;
		}
		if (longest > -0x1000)
			longest = -0x1000;
	} else {
		left_array[0] = v3;
		left_array[1] = v2;
		left_array[2] = v1;
		left_section = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section = 1;

		if (RightSection_FT_Z() <= 0)
			return FALSE;
		if (LeftSection_FT_Z() <= 0) {
			left_section--;
			if (LeftSection_FT_Z() <= 0)
				return FALSE;
		}
		if (longest < 0x1000)
			longest = 0x1000;
	}

	Ymin = v1->y;
	Ymax = min(v3->y - 1, scissor->y1);

	delta_right_u =
		shl10idiv(temp * ((v3->u - v1->u) >> 10) + ((v1->u - v2->u) << 6),
				  longest);
	delta_right_v =
		shl10idiv(temp * ((v3->v - v1->v) >> 10) + ((v1->v - v2->v) << 6),
				  longest);

	delta_right_z = shl10idiv64(temp * ((v3->z - v1->z) >> 10) + ((v1->z - v2->z) << 6), longest);
	/* Calculate the Z increment per line */

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
// POLY 3 F-SHADED TEX 15 BIT
////////////////////////////////////////////////////////////////////////

void drawTriangleFTDecal_Z(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;
	long difX, difY;
	long posX, posY;
	u32 *tex;
	s64 difZ, posZ;
	u32 zval;

#ifdef GS_LOG
	GS_LOG("drawTriangleFTDecal_Z %dx%dx%d - %dx%dx%d - %dx%dx%d (fst=%x, psm=%x, cpsm=%x)\n",
		   v[0].x, v[0].y, v[0].z,
		   v[1].x, v[1].y, v[1].z,
		   v[2].x, v[2].y, v[2].z, prim->fst, tex0->psm, tex0->cpsm);
#endif

	tex = vRamUL + tex0->tbp0;

	if (!SetupSections_FT_Z(v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, 
							v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v))
		return;

	ymax = Ymax;

	for (ymin = Ymin; ymin < scissor->y0; ymin++)
		if (NextRow_FT_Z())
			return;

	difX = delta_right_u;
	difY = delta_right_v;
	difZ = delta_right_z;
	SETdrawPixel();

	switch(test->ztst) {
		case ZTST_NEVER:
			return;

		case ZTST_ALWAYS:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						zval = (u32) (posZ >> 16);
						drawPixel(j, i, tex[((posY >> 16) * tex0->tbw) + (posX >> 16)]);
						zBufUL[i * gsfb->fbw + j] = zval;
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;

		case ZTST_GEQUAL:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						zval = (u32) (posZ >> 16);
						
						if (zval >= zBufUL[i * gsfb->fbw + j]) {
							drawPixel(j, i, tex[((posY >> 16) * tex0->tbw) + (posX >> 16)]);
							zBufUL[i * gsfb->fbw + j] = zval;
						}
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;

		case ZTST_GREATER:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						zval = (u32) (posZ >> 16);
						
						if (zval > zBufUL[i * gsfb->fbw + j]) {
							drawPixel(j, i, tex[((posY >> 16) * tex0->tbw) + (posX >> 16)]);
							zBufUL[i * gsfb->fbw + j] = zval;
						}
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;
	}
}

////////////////////////////////////////////////////////////////////////
// POLY 3 F-SHADED TEX 15 BIT
////////////////////////////////////////////////////////////////////////

void drawTriangleFTModulate_Z(Vertex * v) {
	int i, j, xmin, xmax, ymin, ymax;
	long difX, difY;
	long posX, posY;
	u32 *tex;
	u32 rv, gv, bv; /* Colour of vertex */
	u32 rt, gt, bt; /* Colour of texture */
	s64 difZ, posZ;
	u32 zval;

#ifdef GS_LOG
	GS_LOG("drawTriangleFTModulate_Z %dx%dx%d - %dx%dx%d - %dx%dx%d rgba=%x (fst=%d)\n",
		   v[0].x, v[0].y, v[0].z,
		   v[1].x, v[1].y, v[1].z,
		   v[2].x, v[2].y, v[2].z, rgba, prim->fst);
	GS_LOG("(psm=%x, cbp=%x)\n", tex0->psm, tex0->cbp);
#endif

	rv = (rgba >> 16) & 0xFF;
	gv = (rgba >> 8) & 0xFF;
	bv = rgba & 0xFF;

	tex = vRamUL + tex0->tbp0;

	if (!SetupSections_FT_Z(v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, 
						  v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v))
		return;

	ymax = Ymax;

	for (ymin = Ymin; ymin < scissor->y0; ymin++)
		if (NextRow_FT_Z())
			return;

	difX = delta_right_u;
	difY = delta_right_v;
	difZ = delta_right_z;
	SETdrawPixel();

	switch(test->ztst) {
		case ZTST_NEVER:
			return;

		case ZTST_ALWAYS:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						u32 texcol;
						
						texcol = tex[((posY >> 16) * tex0->tbw) + (posX >> 16)];
						rt = (texcol >> 16) & 0xFF;
						gt = (texcol >> 8) & 0xFF;
						bt = texcol & 0xFF; /* Extract colours from texture */
						zval = (u32) (posZ >> 16);

						rt = (rt * rv) >> 7; /* Multiply by colour value */
						gt = (gt * gv) >> 7;
						bt = (bt * bv) >> 7;
						if(rt > 0xFF)
							rt = 0xFF;
						if(gt > 0xFF)
							gt = 0xFF;
						if(bt > 0xFF)
							bt = 0xFF;

						drawPixel(j, i, (rt << 16) | (gt << 8) | bt);
						zBufUL[i * gsfb->fbw + j] = zval;
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;

		case ZTST_GEQUAL:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						u32 texcol;
						zval = (u32) (posZ >> 16);
						
						if (zval >= zBufUL[i * gsfb->fbw + j]) {
							texcol = tex[((posY >> 16) * tex0->tbw) + (posX >> 16)];
							rt = (texcol >> 16) & 0xFF;
							gt = (texcol >> 8) & 0xFF;
							bt = texcol & 0xFF; /* Extract colours from texture */

							rt = (rt * rv) >> 7; /* Multiply by colour value */
							gt = (gt * gv) >> 7;
							bt = (bt * bv) >> 7;
							if(rt > 0xFF)
								rt = 0xFF;
							if(gt > 0xFF)
								gt = 0xFF;
							if(bt > 0xFF)
								bt = 0xFF;

							drawPixel(j, i, (rt << 16) | (gt << 8) | bt);
							zBufUL[i * gsfb->fbw + j] = zval;
						}
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;
		
		case ZTST_GREATER:
			for (i = ymin; i <= ymax; i++) {
				xmin = (left_x >> 16);
				xmax = (right_x >> 16) - 1;
				if (scissor->x1 < xmax)
					xmax = scissor->x1;

				posZ = left_z;

				if (xmax >= xmin) {
					posX = left_u;
					posY = left_v;

					if (xmin < scissor->x0) {
						j = scissor->x0 - xmin;
						xmin = scissor->x0;
						posX += j * difX;
						posY += j * difY;
						posZ += j * difZ;
					}

					for (j = xmin; j <= xmax; j ++) {
						u32 texcol;
						zval = (u32) (posZ >> 16);
						
						if (zval > zBufUL[i * gsfb->fbw + j]) {
							texcol = tex[((posY >> 16) * tex0->tbw) + (posX >> 16)];
							rt = (texcol >> 16) & 0xFF;
							gt = (texcol >> 8) & 0xFF;
							bt = texcol & 0xFF; /* Extract colours from texture */

							rt = (rt * rv) >> 7; /* Multiply by colour value */
							gt = (gt * gv) >> 7;
							bt = (bt * bv) >> 7;
							if(rt > 0xFF)
								rt = 0xFF;
							if(gt > 0xFF)
								gt = 0xFF;
							if(bt > 0xFF)
								bt = 0xFF;

							drawPixel(j, i, (rt << 16) | (gt << 8) | bt);
							zBufUL[i * gsfb->fbw + j] = zval;
						}
						posX += difX;
						posY += difY;
						posZ += difZ;
					}
				}
				if (NextRow_FT_Z()) {
					return;
				}
			}
			break;
	}

}


////////////////////////////////////////////////////////////////////////

__inline int RightSection_G(void) {
	soft_vertex * v1 = right_array[ right_section ];
	soft_vertex * v2 = right_array[ right_section-1 ];

	int height = v2->y - v1->y;
	if(height == 0) return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;

	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_G(void) {
	soft_vertex * v1 = left_array[ left_section ];
	soft_vertex * v2 = left_array[ left_section-1 ];

	int height = v2->y - v1->y;
	if(height == 0) return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;

	delta_left_R = ((v2->R - v1->R)) / height;
	left_R = v1->R;
	delta_left_G = ((v2->G - v1->G)) / height;
	left_G = v1->G;
	delta_left_B = ((v2->B - v1->B)) / height;
	left_B = v1->B;

	left_section_height = height;
	return height;  
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_G(void) {
	if(--left_section_height<=0) {
		if(--left_section <= 0) {return TRUE;}
		if(LeftSection_G()  <= 0) {return TRUE;}
	} else {
		left_x += delta_left_x;
		left_R += delta_left_R;
		left_G += delta_left_G;
		left_B += delta_left_B;
	}

	if(--right_section_height<=0) {
		if(--right_section<=0) {return TRUE;}
		if(RightSection_G() <=0) {return TRUE;}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_G(int x1,int y1,int x2,int y2,int x3,int y3,long rgb1, long rgb2, long rgb3) {
	soft_vertex * v1, * v2, * v3;
	int height,longest,temp;

	v1 = vtx;
	v1->x=x1<<16;
	v1->y=y1;
	v1->R=rgb1 & 0x00ff0000;
	v1->G=(rgb1<<8) & 0x00ff0000;
	v1->B=(rgb1<<16) & 0x00ff0000;

	v2 = vtx+1;
	v2->x=x2<<16;
	v2->y=y2;
	v2->R=rgb2 & 0x00ff0000;
	v2->G=(rgb2<<8) & 0x00ff0000;
	v2->B=(rgb2<<16) & 0x00ff0000;

	v3 = vtx+2;
	v3->x=x3<<16;
	v3->y=y3;
	v3->R=rgb3 & 0x00ff0000;
	v3->G=(rgb3<<8) & 0x00ff0000;
	v3->B=(rgb3<<16) & 0x00ff0000;

	if (v1->y > v2->y) { soft_vertex * v = v1; v1 = v2; v2 = v; }
	if (v1->y > v3->y) { soft_vertex * v = v1; v1 = v3; v3 = v; }
	if (v2->y > v3->y) { soft_vertex * v = v2; v2 = v3; v3 = v; }

	height = v3->y - v1->y;
	if (height == 0) {return FALSE;}
	temp=(((v2->y - v1->y) << 16) / height);
	longest = temp * ((v3->x - v1->x)>>16) + (v1->x - v2->x);
	if (longest == 0) {return FALSE;}

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section  = 2;
		left_array[0]  = v3;
		left_array[1]  = v1;
		left_section   = 1;

		if (LeftSection_G() <= 0) return FALSE;
		if (RightSection_G() <= 0) {
			right_section--;
			if (RightSection_G() <= 0) return FALSE;
		}
		if (longest > -0x1000) longest = -0x1000;     
	} else {
		left_array[0]  = v3;
		left_array[1]  = v2;
		left_array[2]  = v1;
		left_section   = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section  = 1;

		if (RightSection_G() <= 0) return FALSE;
		if (LeftSection_G() <= 0) {
			left_section--;
			if (LeftSection_G() <= 0) return FALSE;
		}
		if (longest < 0x1000) longest = 0x1000;     
	}

	Ymin=v1->y;
	Ymax=min(v3->y-1,scissor->y1);    

	delta_right_R=shl10idiv(temp*((v3->R - v1->R)>>10)+((v1->R - v2->R)<<6),longest);
	delta_right_G=shl10idiv(temp*((v3->G - v1->G)>>10)+((v1->G - v2->G)<<6),longest);
	delta_right_B=shl10idiv(temp*((v3->B - v1->B)>>10)+((v1->B - v2->B)<<6),longest);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
// POLY 3/4 G-SHADED
////////////////////////////////////////////////////////////////////////
 
void drawTriangleG(Vertex * v) {
	int i,j,xmin,xmax,ymin,ymax;
	long cR1,cG1,cB1;
	long difR,difB,difG;

#ifdef GS_LOG
	GS_LOG("drawTriangleG %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[0].rgba);
#endif

	if(!SetupSections_G(v[0].x,v[0].y,v[1].x,v[1].y,v[2].x,v[2].y,v[0].rgba,v[1].rgba,v[2].rgba)) return;

	ymax=Ymax;

	for (ymin=Ymin;ymin<scissor->y0;ymin++)
		if (NextRow_G()) return;

	difR=delta_right_R;
	difG=delta_right_G;
	difB=delta_right_B;
	SETdrawPixel();

	for (i=ymin;i<=ymax;i++) {
		xmin=(left_x >> 16);
		xmax=(right_x >> 16)-1;
		if (scissor->x1<xmax) xmax=scissor->x1;

		if (xmax>=xmin) {
			cR1=left_R;
			cG1=left_G;
			cB1=left_B;

			if (xmin<scissor->x0) {
				j=scissor->x0-xmin;
				xmin=scissor->x0;
				cR1+=j*difR;
				cG1+=j*difG;
				cB1+=j*difB;
			}

			for (j=xmin;j<=xmax;j++) {
				drawPixel(j, i, (cR1 & 0xff0000)|(((cG1) >> 8)&0x00ff00)|(((cB1) >> 16)&0x0000ff));
				cR1+=difR;
				cG1+=difG;
				cB1+=difB;
			}
		}
		if(NextRow_G()) return;
	}
}

////////////////////////////////////////////////////////////////////////

__inline int RightSection_GT(void) {
	soft_vertex * v1 = right_array[ right_section ];
	soft_vertex * v2 = right_array[ right_section-1 ];

	int height = v2->y - v1->y;
	if (height == 0) return 0;
	delta_right_x = (v2->x - v1->x) / height;
	right_x = v1->x;

	right_section_height = height;
	return height;
}

////////////////////////////////////////////////////////////////////////

__inline int LeftSection_GT(void) {
	soft_vertex * v1 = left_array[ left_section ];
	soft_vertex * v2 = left_array[ left_section-1 ];

	int height = v2->y - v1->y;
	if (height == 0) return 0;
	delta_left_x = (v2->x - v1->x) / height;
	left_x = v1->x;

	delta_left_u = ((v2->u - v1->u)) / height;
	left_u = v1->u;
	delta_left_v = ((v2->v - v1->v)) / height;
	left_v = v1->v;

	delta_left_R = ((v2->R - v1->R)) / height;
	left_R = v1->R;
	delta_left_G = ((v2->G - v1->G)) / height;
	left_G = v1->G;
	delta_left_B = ((v2->B - v1->B)) / height;
	left_B = v1->B;

	left_section_height = height;
	return height;  
}

////////////////////////////////////////////////////////////////////////

__inline BOOL NextRow_GT(void) {
	if (--left_section_height<=0) {
		if(--left_section <= 0) {return TRUE;}
		if(LeftSection_GT()  <= 0) {return TRUE;}
	} else {
		left_x += delta_left_x;
		left_u += delta_left_u;
		left_v += delta_left_v;
		left_R += delta_left_R;
		left_G += delta_left_G;
		left_B += delta_left_B;
	}

	if (--right_section_height<=0) {
		if (--right_section<=0) {return TRUE;}
		if (RightSection_GT() <=0) {return TRUE;}
	} else {
		right_x += delta_right_x;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////

__inline BOOL SetupSections_GT(int x1, int y1, int x2, int y2, int x3, int y3, int tx1, int ty1, int tx2, int ty2, int tx3, int ty3, long rgb1, long rgb2, long rgb3) {
	soft_vertex * v1, * v2, * v3;
	int height,longest,temp;

	v1 = vtx;
	v1->x=x1<<16;
	v1->y=y1;
	v1->u=tx1<<16;
	v1->v=ty1<<16;
	v1->R=(rgb1) & 0x00ff0000;
	v1->G=(rgb1<<8) & 0x00ff0000;
	v1->B=(rgb1<<16) & 0x00ff0000;

	v2 = vtx+1;
	v2->x=x2<<16;
	v2->y=y2;
	v2->u=tx2<<16;
	v2->v=ty2<<16;
	v2->R=(rgb2) & 0x00ff0000;
	v2->G=(rgb2<<8) & 0x00ff0000;
	v2->B=(rgb2<<16) & 0x00ff0000;

	v3 = vtx+2;
	v3->x=x3<<16;
	v3->y=y3;
	v3->u=tx3<<16;
	v3->v=ty3<<16;
	v3->R=(rgb3) & 0x00ff0000;
	v3->G=(rgb3<<8) & 0x00ff0000;
	v3->B=(rgb3<<16) & 0x00ff0000;

	if (v1->y > v2->y) { soft_vertex * v = v1; v1 = v2; v2 = v; }
	if (v1->y > v3->y) { soft_vertex * v = v1; v1 = v3; v3 = v; }
	if (v2->y > v3->y) { soft_vertex * v = v2; v2 = v3; v3 = v; }

	height = v3->y - v1->y;
	if (height == 0) return FALSE;

	temp=(((v2->y - v1->y) << 16) / height);
	longest = temp * ((v3->x - v1->x)>>16) + (v1->x - v2->x);

	if (longest == 0) return FALSE;

	if (longest < 0) {
		right_array[0] = v3;
		right_array[1] = v2;
		right_array[2] = v1;
		right_section  = 2;
		left_array[0]  = v3;
		left_array[1]  = v1;
		left_section   = 1;

		if (LeftSection_GT() <= 0) return FALSE;
		if (RightSection_GT() <= 0) {
			right_section--;
			if (RightSection_GT() <= 0) return FALSE;
		}
		if (longest > -0x1000) longest = -0x1000;     
	} else {
		left_array[0]  = v3;
		left_array[1]  = v2;
		left_array[2]  = v1;
		left_section   = 2;
		right_array[0] = v3;
		right_array[1] = v1;
		right_section  = 1;

		if (RightSection_GT() <= 0) return FALSE;
		if (LeftSection_GT() <= 0) {    
			left_section--;
			if (LeftSection_GT() <= 0) return FALSE;
		}
		if (longest < 0x1000) longest = 0x1000;     
	}

	Ymin=v1->y;
	Ymax=min(v3->y-1,scissor->y1);

	delta_right_R=shl10idiv(temp*((v3->R - v1->R)>>10)+((v1->R - v2->R)<<6),longest);
	delta_right_G=shl10idiv(temp*((v3->G - v1->G)>>10)+((v1->G - v2->G)<<6),longest);
	delta_right_B=shl10idiv(temp*((v3->B - v1->B)>>10)+((v1->B - v2->B)<<6),longest);

	delta_right_u=shl10idiv(temp*((v3->u - v1->u)>>10)+((v1->u - v2->u)<<6),longest);
	delta_right_v=shl10idiv(temp*((v3->v - v1->v)>>10)+((v1->v - v2->v)<<6),longest);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
// POLY 3 G-SHADED TEX 15 BIT
////////////////////////////////////////////////////////////////////////

void drawTriangleGT(Vertex * v) {
	int i,j,xmin,xmax,ymin,ymax;
	long cR1,cG1,cB1;
	long difR,difB,difG;
	long difX, difY;
	long posX,posY;
	u32 *tex;

#ifdef GS_LOG
	GS_LOG("drawTriangleGT %dx%d - %dx%d - %dx%d rgba=%x, tex %dx%d - %dx%d - %dx%d (fst=%x)\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[0].rgba,
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v,
		   prim->fst);
#endif

	tex = vRamUL + tex0->tbp0;

	if (!SetupSections_GT(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y,
						 v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v, v[0].rgba,v[1].rgba,v[2].rgba)) return;

	ymax=Ymax;

	for (ymin=Ymin; ymin<scissor->y0; ymin++)
		if (NextRow_GT()) return;

	difR=delta_right_R;
	difG=delta_right_G;
	difB=delta_right_B;
	difX=delta_right_u;
	difY=delta_right_v;
	SETdrawPixel();

	for (i=ymin; i<=ymax; i++) {
		xmin=(left_x >> 16);
		xmax=(right_x >> 16)-1;
		if (scissor->x1<xmax) xmax=scissor->x1;

		if (xmax>=xmin) {
			posX=left_u;
			posY=left_v;
			cR1=left_R;
			cG1=left_G;
			cB1=left_B;

			if (xmin<scissor->x0) {
				j=scissor->x0-xmin;
				xmin=scissor->x0;
				posX+=j*difX;
				posY+=j*difY;
				cR1+=j*difR;
				cG1+=j*difG;
				cB1+=j*difB;
			}

			for (j=xmin;j<=xmax;j++) {
				drawPixel(j, i, tex[((posY >> 16) * tex0->tbw) + (posX >> 16)]);
//					(cR1 & 0xff0000)|(((cG1) >> 8)&0x00ff00)|(((cB1) >> 16)&0x0000ff);
				posX+=difX;
				posY+=difY;
				cR1+=difR;
				cG1+=difG;
				cB1+=difB;
			}
		}
		if (NextRow_GT())
			return;
	}
}

void drawSprite(Vertex * v) {
	int x, y;

#ifdef GS_LOG
	GS_LOG("drawSprite %dx%d - %dx%d %lx\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, rgba);
#endif

	SETdrawPixel();
	for (y = v[0].y; y < v[1].y; y++)
		for (x = v[0].x; x < v[1].x; x++)
			drawPixel(x, y, rgba);
}

#define _drawSprite_Z(testm) \
	for (y = v[0].y; y < v[1].y; y++) { \
		for (x = v[0].x; x < v[1].x; x++) { \
			if (v[1].z testm zBufUL[offs + x]) { \
				fBufUL[offs + x] = rgba; \
				zBufUL[offs + x] = v[1].z; \
			} \
		} \
		offs += gsfb->fbw; \
	}
	

void drawSprite_Z(Vertex * v) {
	int x, y;
	int offs;

#ifdef GS_LOG
	GS_LOG("drawSprite_Z %dx%d %lx - %dx%d %lx\n",
		   v[0].x, v[0].y, v[0].rgba, v[1].x, v[1].y, v[1].rgba);
#endif

	offs = v[0].y * gsfb->fbw;
	SETdrawPixel();

	switch (test->ztst)	{
		case ZTST_NEVER:
			return;
		case ZTST_ALWAYS:
			for (y = v[0].y; y < v[1].y; y++) {
				for (x = v[0].x; x < v[1].x; x++) {
					drawPixel(x, y, rgba);
					zBufUL[offs + x] = v[1].z;
				}
				offs += gsfb->fbw;
			}
			break;
		case ZTST_GEQUAL:
			_drawSprite_Z(>=);
			break;
		case ZTST_GREATER:
			_drawSprite_Z(>);
			break;
	}
}

void copy_clut_pscm32(u32 *clud, u32 *clut) {
	int loop1, loop2;
	u32 *p = clut;

	for(loop1 = 0; loop1 < 8; loop1++) {
		for(loop2 = 0; loop2 < 8; loop2++) {
			clud[(loop1*32 + loop2) & 0xff] = *p++;
		}
		for(loop2 = 16; loop2 < 24; loop2++) {
  			 clud[(loop1*32 + loop2) & 0xff] = *p++;
		}
		for(loop2 = 8; loop2 < 16; loop2++) {
			clud[(loop1*32 + loop2) & 0xff] = *p++;
		}
		for(loop2 = 24; loop2 < 32; loop2++) {
			 clud[(loop1*32 + loop2) & 0xff] = *p++;
		}
	}
}

void copy_clut_pscm324(u32 *clud, u32 *clut) {
	int loop1, loop2;
	u32 *p = clut;

	for(loop1 = 0; loop1 < 8; loop1++) {
		for(loop2 = 0; loop2 < 8; loop2++) {
			clud[loop1*8 + loop2] = *p++;
		}
	}
}

void copy_clut_cpsm16(u32 *clud, u16 *tex16) {
	u32 *clup=clud;
	int x;

	for (x = 0; x < 256; x++) {
		u32 c = *tex16++;
		*clup = ((c & 0x7c00) << 9) |
				((c & 0x03e0) << 6) |
				((c & 0x001f) << 3);
		if (texa.aem) {
			if (c & 0x8000) {
				*clup|= texa.ta[1] << 24;
			} else {
				if (*clup) *clup|= texa.ta[0] << 24;
			}
		} else {
			*clup|= texa.ta[(c & 0x8000) >> 15] << 24;
		}
		clup++;
	}
}

void drawSpriteT(Vertex * vx) {
	u32 clud[256];
	u8  *tex8;
	u16 *tex16;
	u32 *tex;
	int x, y;
	int u, v;

#ifdef GS_LOG
	GS_LOG("drawSpriteT %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colModulateSetCol(rgba);

	if (tex0->psm == 0x13) {
		if (tex0->cpsm == 0) {
			if (tex0->csm == 0) {
				tex = (u32*)(vRamUL + tex0->cbp);
			} else {
				tex = (u32*)(vRamUL + tex0->cbp + clut.cov * clut.cbw + clut.cou);
			}
			copy_clut_pscm32(clud, tex);
		}
		if (tex0->cpsm == 0x2) {
			tex16 = (u16*)(vRamUL + tex0->cbp + clut.cov * clut.cbw + clut.cou);
			copy_clut_cpsm16(clud, tex16);
		}
	}

	if (tex0->psm == 0x14) {
		if (tex0->cpsm == 0) {
			if (tex0->csm == 0) {
				tex = (u32*)(vRamUL + tex0->cbp);
			} else {
				tex = (u32*)(vRamUL + tex0->cbp + clut.cov * clut.cbw + clut.cou);
			}
			copy_clut_pscm324(clud,tex);
		}
		if (tex0->cpsm == 0x2) {
			if (tex0->csm == 0) {
				tex16 = (u16*)(vRamUL + tex0->cbp);
			} else {
#ifdef GS_LOG
				GS_LOG("clut %dx%d cbw=%d\n",clut.cou, clut.cov, clut.cbw);
#endif
				tex16 = (u16*)(vRamUL + tex0->cbp + clut.cov * clut.cbw + clut.cou);
			}
			copy_clut_cpsm16(clud, tex16);
		}
	}

	SETdrawPixel();

	if (tex0->psm == 0x13) {
		for (y = vx[0].y, v = vx[0].v; y < vx[1].y && v < vx[1].v; y++, v++) {
			tex8 = vRam + tex0->tbp0 * 4 + v * tex0->tbw;
			for (x = vx[0].x, u = vx[0].u; x < vx[1].x && u < vx[1].u; x++, u++) { 
				drawPixel(x, y, clud[tex8[u]]);
			}
		}
		return;
	}

	if (tex0->psm == 0x14) {
		for (y = vx[0].y, v = vx[0].v; y < vx[1].y && v < vx[1].v; y++, v++) {
			tex8 = vRam + tex0->tbp0 * 4 + v * (tex0->tbw/2);
			for (x = vx[0].x, u = vx[0].u; x < vx[1].x && u < vx[1].u; x++, u++) { 
				drawPixel(x, y, clud[tex8[u/2] & 0xf]);
				x++; u++;
				if (x >= vx[1].x || u >= vx[1].u) break;
				drawPixel(x, y, clud[tex8[u/2] >> 4]);
			}
		}
		return;
	}

	for (y = vx[0].y, v = vx[0].v; y < vx[1].y && v < vx[1].v; y++, v++) {
		tex = vRamUL + tex0->tbp0 + v * tex0->tbw;
		if ((tex0->tbp0 + v * tex0->tbw) > (1024*1024)) return;
		for (x = vx[0].x, u = vx[0].u; x < vx[1].x && u < vx[1].u; x++, u++) {
			drawPixel(x, y, colModulate(tex[u]));
		}
	}
}
