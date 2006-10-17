#include "Draw.h"

#ifdef __WIN32__
#pragma warning(disable:4244)
#endif

void ScrBlit15S(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u16 *pscr;
	u32 *ptr;
	int x, y, cy;
	int xpos, xinc;
	int ypos, yinc;

	xinc = (gsScr->w << 16) / conf.mode.width;

	ypos=0;
	yinc = (gsScr->h << 16) / conf.mode.height;

	if (conf.fps) { y = 15; ypos+= yinc * 15; }
	else y = 0;

	for (; y<conf.mode.height; y++, ypos+=yinc) {
		cy=(ypos>>16);

		pscr = (u16*)(scr + y * lPitch);
		ptr = (u32*)dbuf + (cy + gsScr->y) * gsdspfb->fbw + gsScr->x;
		xpos = 0x10000L;
		for(x=0; x<conf.mode.width; x++, xpos+= xinc) {
			u32 c;

			ptr+= xpos>>16;
			xpos-= xpos&0xffff0000;
			c = *ptr;
			*pscr++= ((c & 0x0000f8) <<  7) | // 0x7c00
					 ((c & 0x00f800) >>  6) | // 0x03e0
					 ((c & 0xf80000) >> 19);  // 0x001f
		}
	}
}

void ScrBlit16S(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u16 *pscr;
	u32 *ptr;
	int x, y, cy;
	int xpos, xinc;
	int ypos, yinc;

	xinc = (gsScr->w << 16) / conf.mode.width;

	ypos=0;
	yinc = (gsScr->h << 16) / conf.mode.height;

	if (conf.fps) { y = 15; ypos+= yinc * 15; }
	else y = 0;

	for (; y<conf.mode.height; y++, ypos+=yinc) {
		cy=ypos>>16;

		pscr = (u16*)(scr + y * lPitch);
		ptr = (u32*)dbuf + (cy + gsScr->y) * gsdspfb->fbw + gsScr->x;
		xpos = 0x10000L;
		for(x=0; x<conf.mode.width; x++, xpos+= xinc) {
			u32 c;

			ptr+= xpos>>16;
			xpos-= xpos&0xffff0000;
			c = *ptr;
			*pscr++= ((c & 0x0000f8) <<  8) | // 0xf800
					 ((c & 0x00fc00) >>  5) | // 0x07e0
					 ((c & 0xf80000) >> 19);  // 0x001f
		}
	}
}

void ScrBlit24S(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u8 *pscr;
	u32 *ptr;
	int x, y, cy;
	int xpos, xinc;
	int ypos, yinc;

	xinc = (gsScr->w << 16) / conf.mode.width;

	ypos=0;
	yinc = (gsScr->h << 16) / conf.mode.height;

	if (conf.fps) { y = 15; ypos+= yinc * 15; }
	else y = 0;

	for (; y<conf.mode.height; y++, ypos+=yinc) {
		cy=(ypos>>16);

		pscr = (u8*)(scr + y * lPitch);
		ptr = (u32*)dbuf + (cy + gsScr->y) * gsdspfb->fbw + gsScr->x;
		xpos = 0x10000L;
		for(x=0; x<conf.mode.width; x++, xpos+= xinc) {
			u32 c;

			ptr+= xpos>>16;
			xpos-= xpos&0xffff0000;
			c = *ptr;
			*(pscr+0) = (c >> 16) & 0xff;
			*(pscr+1) = (c >>  8) & 0xff;
			*(pscr+2) = c & 0xff;
			pscr+=3;
		}
	}
}

void ScrBlit32S(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u8 *pscr;
	u32 *ptr;
	int x, y, cy;
	int xpos, xinc;
	int ypos, yinc;

	xinc = (gsScr->w << 16) / conf.mode.width;

	ypos=0;
	yinc = (gsScr->h << 16) / conf.mode.height;

	if (conf.fps) { y = 15; ypos+= yinc * 15; }
	else y = 0;

	for (; y<conf.mode.height; y++, ypos+=yinc) {
		cy=(ypos>>16);

		pscr = (u8*)(scr + y * lPitch);
		ptr = (u32*)dbuf + (cy + gsScr->y) * gsdspfb->fbw + gsScr->x;
		xpos = 0x10000L;
		for(x=0; x<conf.mode.width; x++, xpos+= xinc) {
			u32 c;

			ptr+= xpos>>16;
			xpos-= xpos&0xffff0000;
			c = *ptr;
			*(pscr+0) = (c >> 16) & 0xff;
			*(pscr+1) = (c >>  8) & 0xff;
			*(pscr+2) = c & 0xff;
			pscr+=4;
		}
	}
}

void ScrBlit15(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u16 *pscr;
	u32 *ptr;
	int sx, sy;
	int y, x;
	int h, w;

	if (gsScr->w < conf.mode.width) {
		sx = (conf.mode.width - gsScr->w) / 2;
		w = gsScr->w;
	} else {
		sx = 0;
		w = conf.mode.width;
	}

	if (gsScr->h < conf.mode.height) {
		sy = (conf.mode.height - gsScr->h) / 2;
		h = gsScr->h;
	} else {
		sy = 0;
		h = conf.mode.height;
	}

	if (conf.fps) y = sy < 15 ? 15 - sy : 0;
	else y = 0;

	for (; y<h; y++) {
		pscr = (u16*)(scr + (sy + y) * lPitch + sx*2);
		ptr = (u32*)dbuf + (y + gsScr->y) * gsdspfb->fbw + gsScr->x;
		for (x=0; x<w; x++) {
			u32 c = *ptr++;
			c = ((c & 0x0000f8) <<  7) | // 0x7c00
				((c & 0x00f800) >>  6) | // 0x03e0
				((c & 0xf80000) >> 19);  // 0x001f
			*pscr++ = c;
		}
	}
}

void ScrBlit16(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u16 *pscr;
	u32 *ptr;
	int sx, sy;
	int y, x;
	int h, w;

	if (gsScr->w < conf.mode.width) {
		sx = (conf.mode.width - gsScr->w) / 2;
		w = gsScr->w;
	} else {
		sx = 0;
		w = conf.mode.width;
	}

	if (gsScr->h < conf.mode.height) {
		sy = (conf.mode.height - gsScr->h) / 2;
		h = gsScr->h;
	} else {
		sy = 0;
		h = conf.mode.height;
	}

	if (conf.fps) y = sy < 15 ? 15 - sy : 0;
	else y = 0;

	for (; y<h; y++) {
		pscr = (u16*)(scr + (sy + y) * lPitch + sx*2);
		ptr = (u32*)dbuf + (y + gsScr->y) * gsdspfb->fbw + gsScr->x;
		for (x=0; x<w; x++) {
			u32 c = *ptr++;
			c = ((c & 0x0000f8) <<  8) | // 0xf800
				((c & 0x00fc00) >>  5) | // 0x07e0
				((c & 0xf80000) >> 19);  // 0x001f
			*pscr++ = c;
		}
	}
}

void ScrBlit24(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u8 *pscr;
	u32 *ptr;
	int sx, sy;
	int y, x;
	int h, w;

	if (gsScr->w < conf.mode.width) {
		sx = (conf.mode.width - gsScr->w) / 2;
		w = gsScr->w;
	} else {
		sx = 0;
		w = conf.mode.width;
	}

	if (gsScr->h < conf.mode.height) {
		sy = (conf.mode.height - gsScr->h) / 2;
		h = gsScr->h;
	} else {
		sy = 0;
		h = conf.mode.height;
	}

	if (conf.fps) y = sy < 15 ? 15 - sy : 0;
	else y = 0;

	for (; y<h; y++) {
		pscr = (u8*)(scr + (sy + y) * lPitch + sx*3);
		ptr = (u32*)dbuf + (y + gsScr->y) * gsdspfb->fbw + gsScr->x;
		for (x=0; x<w; x++) {
			u32 c = *ptr++;
			*(pscr+0) = (c >> 16) & 0xff;
			*(pscr+1) = (c >>  8) & 0xff;
			*(pscr+2) = c & 0xff;
			pscr+=3;
		}
	}
}

void ScrBlit32(char *scr, char *dbuf, Rect *gsScr, int lPitch) {
	u8 *pscr;
	u32 *ptr;
	int sx, sy;
	int y, x;
	int h, w;

	if (gsScr->w < conf.mode.width) {
		sx = (conf.mode.width - gsScr->w) / 2;
		w = gsScr->w;
	} else {
		sx = 0;
		w = conf.mode.width;
	}

	if (gsScr->h < conf.mode.height) {
		sy = (conf.mode.height - gsScr->h) / 2;
		h = gsScr->h;
	} else {
		sy = 0;
		h = conf.mode.height;
	}

	if (conf.fps) y = sy < 15 ? 15 - sy : 0;
	else y = 0;

	for (; y<h; y++) {
		pscr = (u8*)(scr + (sy + y) * lPitch + sx*3);
		ptr = (u32*)dbuf + (y + gsScr->y) * gsdspfb->fbw + gsScr->x;
		for (x=0; x<w; x++) {
			u32 c = *ptr++;
			*(pscr+0) = (c >> 16) & 0xff;
			*(pscr+1) = (c >>  8) & 0xff;
			*(pscr+2) = c & 0xff;
			pscr+=4;
		}
	}
}
