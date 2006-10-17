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
#include <stdio.h>

void GSsnapFrame()
{
	// Thanks linuzappz ^^

	FILE *bmpfile;
	char filename[256];
	u8 header[0x36];
	u8 *line;
	s32 size,i,j;
	u8 empty[2]={0,0};
	u32 color;
	u32 snapshotnr = 0;
	u32 *screen;

	size = config.video.width*config.video.height*3 + 0x38;
 
	memset(header,0,0x36);
	header[0]    = 'B';
	header[1]    = 'M';
	header[2]    = size&0xff;
	header[3]    = (size>>8)&0xff;
	header[4]    = (size>>16)&0xff;
	header[5]    = (size>>24)&0xff;
	header[0x0a] = 0x36;
	header[0x0e] = 0x28;
	header[0x12] = config.video.width%256;
	header[0x13] = config.video.width/256;
	header[0x16] = config.video.height%256;
	header[0x17] = config.video.height/256;
	header[0x1a] = 0x01;
	header[0x1c] = 0x18;
	header[0x26] = 0x12;
	header[0x27] = 0x0B;
	header[0x2A] = 0x12;
	header[0x2B] = 0x0B;
 
	for (;;) {
		snapshotnr++;

#ifdef _WINDOWS
		sprintf(filename,"%s\\snap%03d.bmp",snap_path,snapshotnr);
#else
		sprintf(filename,"%s/snap%03ld.bmp",snap_path,snapshotnr);
#endif

		bmpfile = fopen(filename,"rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	}

	if(!(bmpfile = fopen(filename,"wb")))
		return;

	screen = (u32*)malloc(config.video.width*config.video.height*sizeof(u32));
	glReadPixels(0,0,config.video.width,config.video.height,GL_RGBA,GL_UNSIGNED_BYTE,screen);

	line = (u8*)malloc(config.video.width*3);

	fwrite(header,0x36,1,bmpfile);
	for(i=0; i<config.video.height; i++) {
		for(j=0; j<config.video.width; j++) {
			color = screen[i*config.video.width+j];
			line[j*3+2] = (u8)(color    )&0xff;
			line[j*3+1] = (u8)(color>> 8)&0xff;
			line[j*3+0] = (u8)(color>>16)&0xff;
		}
		fwrite(line,config.video.width*3,1,bmpfile);
	}
	fwrite(empty,0x2,1,bmpfile);
	fclose(bmpfile);

	free(line);
	free(screen);
}

void GSsnapVram()
{
	// Thanks linuzappz ^^

	FILE *bmpfile;
	char filename[256];
	u8 header[0x36];
	u8 *line;
	s32 size,i,j;
	u8 empty[2]={0,0};
	u32 color;
	u32 snapshotnr = 0;
	u32 h = (1024*1024/dispfb->fbw);

	size = dispfb->fbw*h*3 + 0x38;
 
	memset(header,0,0x36);
	header[0]    = 'B';
	header[1]    = 'M';
	header[2]    = size&0xff;
	header[3]    = (size>>8)&0xff;
	header[4]    = (size>>16)&0xff;
	header[5]    = (size>>24)&0xff;
	header[0x0a] = 0x36;
	header[0x0e] = 0x28;
	header[0x12] = dispfb->fbw%256;
	header[0x13] = dispfb->fbw/256;
	header[0x16] = h%256;
	header[0x17] = h/256;
	header[0x1a] = 0x01;
	header[0x1c] = 0x18;
	header[0x26] = 0x12;
	header[0x27] = 0x0B;
	header[0x2A] = 0x12;
	header[0x2B] = 0x0B;
 
	for (;;) {
		snapshotnr++;

#ifdef _WINDOWS
		sprintf(filename,"%s\\vsnap%03d.bmp",snap_path,snapshotnr);
#else
		sprintf(filename,"%s/vsnap%03ld.bmp",snap_path,snapshotnr);
#endif

		bmpfile = fopen(filename,"rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	}

	if(!(bmpfile = fopen(filename,"wb")))
		return;

	line = (u8*)malloc(dispfb->fbw*3);

	fwrite(header,0x36,1,bmpfile);
	for(i=h-1; i>=0; i--) {
		for(j=0; j<dispfb->fbw; j++) {
			color = vRamUL[i*dispfb->fbw+j];
			line[j*3+2] = (u8)(color    )&0xff;
			line[j*3+1] = (u8)(color>> 8)&0xff;
			line[j*3+0] = (u8)(color>>16)&0xff;
		}
		fwrite(line,dispfb->fbw*3,1,bmpfile);
	}
	fwrite(empty,0x2,1,bmpfile);
	fclose(bmpfile);

	free(line);
}

void GSupdateView()
{
	float r,s;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	switch (config.video.ratio) {
		case 0: // 1:1
			break;

		case 1: { // stretched to full window size

			r = (float)config.video.width/(float)display->w;
			s = (float)config.video.height/(float)display->h;

			if (display->w > display->h) {
				glPointSize(s);
				glLineWidth(s);
			} else { // display->w <= display->h
				glPointSize(r);
				glLineWidth(r);
			}

			if (smode2.intm && smode2.ffmd) s *= 2.f;

			glScalef(r,s,1);

			break; }

		case 2: { // stretched but keep the ps2 ratio

			r = (float)config.video.width/(float)display->w;
			s = (float)config.video.height/(float)display->h;

			if (r > s) {
				glPointSize(s);
				glLineWidth(s);

				if (smode2.intm && smode2.ffmd) r = 2.f;
				else                            r = 1.f;

				glScalef(s,r*s,1);
			} else {
				glPointSize(r);
				glLineWidth(r);

				if (smode2.intm && smode2.ffmd) s = 2.f;
				else                            s = 1.f;

				glScalef(r,s*r,1);
			}
			break; }
	}
}