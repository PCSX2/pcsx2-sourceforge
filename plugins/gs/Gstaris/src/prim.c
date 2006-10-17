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

gsvertex v_[3];

s32 i;
double u[3],v[3];
float k;

// set_draw_color help:
// there's different case:
// flat = set_draw_color_f
// gouraud = set_draw_color_g
// flat + texture = set_draw_color_ft
// gouraud + texture = set_draw_color_gt
// vc = vertex componant

__inline void set_draw_color_f() {
	glColor4f(rgbac.r,rgbac.g,rgbac.b,rgbac.a);
}

__inline void set_draw_color_g(s32 vc) {
	glColor4f(rgba[vc].r,rgba[vc].g,rgba[vc].b,rgba[vc].a);
}

__inline void set_draw_color_ft() {
	switch (tex0->tfx) {
		case 0: // MODULATE
			if (prim->fst)
				glColor4f(2.f*rgbac.r,2.f*rgbac.g,2.f*rgbac.b,2.f*rgbac.a);
			else
				glColor4f(rgbac.r,rgbac.g,rgbac.b,2.f*rgbac.a);
			break;
		case 1: // DECAL
			glColor4f(1.f,1.f,1.f,1.f);
			break;
		case 2: // HIGHLIGHT
			glColor4f(2.f*rgbac.r+rgbac.a,2.f*rgbac.g+rgbac.a,2.f*rgbac.b+rgbac.a,2.f*rgbac.a+rgbac.a);
			break;
		case 3: // HIGHLIGHT2
			glColor4f(rgbac.r+rgbac.a,rgbac.g+rgbac.a,rgbac.b+rgbac.a,rgbac.a);
			break;
	}
}

__inline void set_draw_color_gt(s32 vc) {
	switch (tex0->tfx) {
		case 0: // MODULATE
			if (prim->fst)
				glColor4f(2*rgba[vc].r,2*rgba[vc].g,2*rgba[vc].b,2*rgba[vc].a);
			else
				glColor4f(rgba[vc].r,rgba[vc].g,rgba[vc].b,rgba[vc].a);
			break;
		case 1: // DECAL
			glColor4f(1.f,1.f,1.f,1.f);
			break;
		case 2: // HIGHLIGHT
			glColor4f(2.f*rgba[vc].r+rgba[vc].a,2.f*rgba[vc].g+rgba[vc].a,2.f*rgba[vc].b+rgba[vc].a,2.f*rgba[vc].a+rgba[vc].a);
			break;
		case 3: // HIGHLIGHT2
			glColor4f(rgba[vc].r+rgba[vc].a,rgba[vc].g+rgba[vc].a,rgba[vc].b+rgba[vc].a,rgba[vc].a);
			break;
	}
}

// set_uv_coords help:

__inline void set_uv_coords_2() {	
	if (prim->fst) { // UV
		u[0] = (double)v_[0].u/(double)tex0->tw;
		v[0] = (double)v_[0].v/(double)tex0->th;
		u[1] = (double)v_[1].u/(double)tex0->tw;
		v[1] = (double)v_[1].v/(double)tex0->th;
	} else { // STQ
		if (v_[0].q == 0) {
			u[0] = 0;
			v[0] = 0;
		} else {
			u[0] = (double)(*(float*)&v_[0].s / *(float*)&v_[0].q);
			v[0] = (double)(*(float*)&v_[0].t / *(float*)&v_[0].q);
		}
		if (v_[1].q == 0) {
			u[1] = 0;
			v[1] = 0;
		} else {
			u[1] = (double)(*(float*)&v_[1].s / *(float*)&v_[1].q);
			v[1] = (double)(*(float*)&v_[1].t / *(float*)&v_[1].q);
		}
	}
}

__inline void set_uv_coords_3() {
	if (prim->fst) { // UV
		u[0] = (double)v_[0].u/(double)tex0->tw;
		v[0] = (double)v_[0].v/(double)tex0->th;
		u[1] = (double)v_[1].u/(double)tex0->tw;
		v[1] = (double)v_[1].v/(double)tex0->th;
		u[2] = (double)v_[2].u/(double)tex0->tw;
		v[2] = (double)v_[2].v/(double)tex0->th;
	} else { // STQ
		if (v_[0].q == 0) {
			u[0] = 0;
			v[0] = 0;
		} else {
			u[0] = (double)(*(float*)&v_[0].s / *(float*)&v_[0].q);
			v[0] = (double)(*(float*)&v_[0].t / *(float*)&v_[0].q);
		}
		if (v_[1].q == 0) {
			u[1] = 0;
			v[1] = 0;
		} else {
			u[1] = (double)(*(float*)&v_[1].s / *(float*)&v_[1].q);
			v[1] = (double)(*(float*)&v_[1].t / *(float*)&v_[1].q);
		}
		if (v_[2].q == 0) {
			u[2] = 0;
			v[2] = 0;
		} else {
			u[2] = (double)(*(float*)&v_[2].s / *(float*)&v_[2].q);
			v[2] = (double)(*(float*)&v_[2].t / *(float*)&v_[2].q);
		}
	}
}

// get_z help
// 

__inline float get_z(s32 i) {
	// need to write a better stuff ^^
	// sure this code sucks but at least it work with all the demos i tried
	if (test->zte) // pow(2,32) = 4294967296 ; 0.5 / pow(2,32) = 8589934592;
		return ( 0.75f - (double)(v_[i].z<<((zbuf->psm == 0) ? 0 : (zbuf->psm == 1) ? 5 : 10)) / 8589934592.f );
	return 0.f;
}

// *************
// point drawing
// *************

void primPoint(gsvertex *v)
{
	if (config.log.prim)
		GSlog("primPoint\n");

	memcpy(v_,v,sizeof(gsvertex));

	v_[0].x -= xyoffset->x;
	v_[0].y -= xyoffset->y;

	if (v_[0].x < scissor->x0) return;
	if (v_[0].x > scissor->x1) return;
	if (v_[0].y < scissor->y0) return;
	if (v_[0].y > scissor->y1) return;

	if (v_[0].x < 0)           return;
	if (v_[0].x >= frame->fbw) return;
	if (v_[0].y < 0)           return;
	if (v_[0].y >= 1024)       return;

	set_draw_color_f();
	glBegin(GL_POINTS);
		glVertex3d((double)v_[0].x,(double)v_[0].y,get_z(0));
	glEnd();
}

// ************
// line drawing
// ************

void drawLineF()
{
	set_draw_color_f();
	glBegin(GL_LINES);
	for (i=0; i<2; i++) {
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void drawLineG()
{
	glBegin(GL_LINES);
	for (i=0; i<2; i++) {
		set_draw_color_g(i);
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void primLine(gsvertex *v)
{
	if (config.log.prim)
		GSlog("primLine\n");

	memcpy(v_,v,sizeof(gsvertex)*2);

	v_[0].x -= xyoffset->x; v_[1].x -= xyoffset->x;
	v_[0].y -= xyoffset->y; v_[1].y -= xyoffset->y;

	for (i=0; i < 2; i++) {
		if (v_[i].x < scissor->x0) v_[i].x = scissor->x0;
		if (v_[i].x > scissor->x1) v_[i].x = scissor->x1;
		if (v_[i].y < scissor->y0) v_[i].y = scissor->y0;
		if (v_[i].y > scissor->y1) v_[i].y = scissor->y1;

		if (v_[i].x < 0)           v_[i].x = 0;
		if (v_[i].x >= frame->fbw) v_[i].x = frame->fbw-1;
		if (v_[i].y < 0)           v_[i].y = 0;
		if (v_[i].y >= 1024)       v_[i].y = 1024-1;
	}

	if (prim->iip) drawLineG();
	else           drawLineF();
}

// ****************
// triangle drawing
// ****************

void drawTriangleF()
{
	set_draw_color_f();
	glBegin(GL_TRIANGLES);
	for (i=0; i<3; i++) {
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void drawTriangleFT()
{
	set_uv_coords_3();
	set_draw_color_ft();

	glBegin(GL_TRIANGLES);
	for (i=0; i<3; i++) {
		glTexCoord2d(u[i],v[i]);
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void drawTriangleG()
{
	glBegin(GL_TRIANGLES);
	for (i=0; i<3; i++) {
		set_draw_color_g(i);
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void drawTriangleGT()
{
	set_uv_coords_3();

	glBegin(GL_TRIANGLES);
	for (i=0; i<3; i++) {
		set_draw_color_gt(i);
		glTexCoord2d(u[i],v[i]);
		glVertex3d((double)v_[i].x,(double)v_[i].y,get_z(i));
	}
	glEnd();
}

void primTriangle(gsvertex *v)
{
	if (config.log.prim)
		GSlog("primTriangle\n");

	memcpy(v_,v,sizeof(gsvertex)*3);

	v_[0].x -= xyoffset->x; v_[1].x -= xyoffset->x; v_[2].x -= xyoffset->x;
	v_[0].y -= xyoffset->y; v_[1].y -= xyoffset->y; v_[2].y -= xyoffset->y;

	// missing stuff fix [1fx, ...]
	if (config.fixes.missing_stuff) {
		v_[0].y -= frame->fbp/frame->fbw; v_[1].y -= frame->fbp/frame->fbw; v_[2].y -= frame->fbp/frame->fbw;
	}

	for (i=0; i < 3; i++) {
		if (v_[i].x < scissor->x0) { v_[i].x = scissor->x0; }
		if (v_[i].x > scissor->x1) { v_[i].x = scissor->x1; }
		if (v_[i].y < scissor->y0) { v_[i].y = scissor->y0; }
		if (v_[i].y > scissor->y1) { v_[i].y = scissor->y1; }

		if (v_[i].x < 0)           v_[i].x = 0;
		if (v_[i].x >= frame->fbw) v_[i].x = frame->fbw-1;
		if (v_[i].y < 0)           v_[i].y = 0;
		if (v_[i].y >= 1024)       v_[i].y = 1024-1;
	}

	if (prim->tme) {
		if (prim->iip) drawTriangleGT();
		else           drawTriangleFT();
	} else {
		if (prim->iip) drawTriangleG();
		else           drawTriangleF();
	}
}

// **************
// sprite drawing
// **************

void drawSprite()
{
	set_draw_color_f();
	glBegin(GL_QUADS);
		glVertex2i(v_[0].x,v_[0].y);
		glVertex2i(v_[1].x,v_[0].y);
		glVertex2i(v_[1].x,v_[1].y);
		glVertex2i(v_[0].x,v_[1].y);
	glEnd();
}

void drawSpriteT()
{
	set_uv_coords_2();
	set_draw_color_ft();

	glBegin(GL_QUADS);
		glTexCoord2d(u[0],v[0]); glVertex2i(v_[0].x,v_[0].y);
		glTexCoord2d(u[1],v[0]); glVertex2i(v_[1].x,v_[0].y);
		glTexCoord2d(u[1],v[1]); glVertex2i(v_[1].x,v_[1].y);
		glTexCoord2d(u[0],v[1]); glVertex2i(v_[0].x,v_[1].y);
	glEnd();
}

void primSprite(gsvertex *v)
{
	if (config.log.prim)
		GSlog("primSprite\n");

	memcpy(v_,v,sizeof(gsvertex)*2);

	v_[0].x -= xyoffset->x; v_[1].x -= xyoffset->x;
	v_[0].y -= xyoffset->y; v_[1].y -= xyoffset->y;

	// this part correct the size of objects which have some parts out of the screen

	if (v_[0].x < scissor->x0) {
		k = ((float)(scissor->x0-v_[0].x))/((float)(v_[1].x-v_[0].x));
		v_[0].u += (int)(k * (float)(v_[1].u-v_[0].u));
	} else if (v_[1].x < scissor->x0) {
		k = ((float)(scissor->x0-v_[1].x))/((float)(v_[0].x-v_[1].x));
		v_[1].u += (int)(k * (float)(v_[0].u-v_[1].u));
	}

	if (v_[0].x > scissor->x1) {
		k = ((float)(v_[0].x-scissor->x1))/((float)(v_[1].x-v_[0].x));
		v_[0].u -= (int)(k * (float)(v_[1].u-v_[0].u));
	} else if (v_[1].x > scissor->x1) {
		k = ((float)(v_[1].x-scissor->x1))/((float)(v_[0].x-v_[1].x));
		v_[1].u -= (int)(k * (float)(v_[0].u-v_[1].u));
	}

	if (v_[0].y < scissor->y0) {
		k = ((float)(scissor->y0-v_[0].y))/((float)(v_[1].y-v_[0].y));
		v_[0].v += (int)(k * (float)(v_[1].v-v_[0].v));
	} else if (v_[1].y < scissor->y0) {
		k = ((float)(scissor->y0-v_[1].y))/((float)(v_[0].y-v_[1].y));
		v_[1].v += (int)(k * (float)(v_[0].v-v_[1].v));
	}

	if (v_[0].y > scissor->y1) {
		k = ((float)(v_[0].y-scissor->y1))/((float)(v_[1].y-v_[0].y));
		v_[0].v -= (int)(k * (float)(v_[1].v-v_[0].v));
	} else if (v_[1].y > scissor->y1) {
		k = ((float)(v_[1].y-scissor->y1))/((float)(v_[0].y-v_[1].y));
		v_[1].v -= (int)(k * (float)(v_[0].v-v_[1].v));
	}

	for (i=0; i<2; i++) {
		if (v_[i].x < scissor->x0) { v_[i].x = scissor->x0; }
		if (v_[i].x > scissor->x1) { v_[i].x = scissor->x1; }
		if (v_[i].y < scissor->y0) { v_[i].y = scissor->y0; }
		if (v_[i].y > scissor->y1) { v_[i].y = scissor->y1; }

		if (v_[i].x < 0)           v_[i].x = 0;
		if (v_[i].x >= frame->fbw) v_[i].x = frame->fbw-1;
		if (v_[i].y < 0)           v_[i].y = 0;
		if (v_[i].y >= 1024)       v_[i].y = 1024-1;
	}

	// depth test is disabled > cause of some problems

	glDisable(GL_DEPTH_TEST);

	if (prim->tme) drawSpriteT();
	else           drawSprite();

	GSsetDepthTest();
}

// *******
// nothing
// *******

void primNothing(gsvertex *v)
{
}