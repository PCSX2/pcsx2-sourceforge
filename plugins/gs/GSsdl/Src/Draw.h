#ifndef __DRAW_H__
#define __DRAW_H__

#include "GS.h"

void ScrBlit15S(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit16S(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit24S(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit32S(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit15(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit16(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit24(char *scr, char *dbuf, Rect *gsScr, int lPitch);
void ScrBlit32(char *scr, char *dbuf, Rect *gsScr, int lPitch);

#endif /* __DRAW_H__ */
