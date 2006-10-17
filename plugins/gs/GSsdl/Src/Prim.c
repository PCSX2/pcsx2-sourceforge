#include "GS.h"
#include "Soft.h"

__inline void clipVertex(Vertex *vx) {
	if (vx->x < scissor->x0) vx->x = scissor->x0;
	if (vx->x > scissor->x1) vx->x = scissor->x1;
	if (vx->y < scissor->y0) vx->y = scissor->y0;
	if (vx->y > scissor->y1) vx->y = scissor->y1;

	if (vx->x < 0) vx->x = 0;
	if (vx->x >= gsfb->fbw) vx->x = gsfb->fbw - 1;
	if (vx->y < 0) vx->y = 0;
	if (vx->y >= gsfb->fbh) vx->y = gsfb->fbh - 1;
	if (vx->y >= 1024) vx->y = 1024 - 1;
}

__inline void clipVertexT(Vertex *vx) {
	int mw, mh;

	if (vx->x < scissor->x0) {
		vx->u+= scissor->x0 - vx->x;
		vx->x = scissor->x0;
	}
	if (vx->x > scissor->x1) {
		vx->u-= vx->x - scissor->x1;
		vx->x = scissor->x1;
	}
	if (vx->y < scissor->y0) {
		vx->v+= scissor->y0 - vx->y;
		vx->y = scissor->y0;
	}
	if (vx->y > scissor->y1) {
		vx->v-= vx->y - scissor->y1;
		vx->y = scissor->y1;
	}

	if (vx->x < 0) vx->x = 0;
	if (vx->x >= gsfb->fbw) vx->x = gsfb->fbw - 1;
	if (vx->y < 0) vx->y = 0;
	if (vx->y >= gsfb->fbh) vx->y = gsfb->fbh - 1;

//	mw = gsfb->fbw - (tex0->tbp0 % gsfb->fbw);
//	mh = gsfb->fbh - (tex0->tbp0 / gsfb->fbw);
	mw = gsfb->fbw;
	mh = gsfb->fbh;

	if (vx->u < 0) vx->u = 0;
	if (vx->u >= mw) vx->u = mw - 1;
	if (vx->v < 0) vx->v = 0;
	if (vx->v >= mh) vx->v = mh - 1;

//	GS_LOG("clipT gsfb %d,%d: tbp0 %d: max %d,%d\n", gsfb->fbw, gsfb->fbh, tex0->tbp0, mw, mh);
}

__inline void STQtoUV(Vertex *v) {
	if (*(float*)&v->q != 0) {
		v->u = (int)((float)tex0->tw * (*(float*)&v->s / *(float*)&v->q));
		v->v = (int)((float)tex0->th * (*(float*)&v->t / *(float*)&v->q));
	} else v->u = v->v = 0;
}

void primPoint(Vertex *vertex) {
	Vertex v;

	memcpy(&v, vertex, sizeof(Vertex));

#ifdef GS_LOG
	GS_LOG("primPoint %dx%d %lx\n", 
			v.x, v.y, rgba);
#endif

	v.x -= offset->x; v.y -= offset->y;

	if (v.x < scissor->x0) return;
	if (v.y < scissor->y0) return;
	if (v.x > scissor->x1) return;
	if (v.y > scissor->y1) return;

	if (v.x < 0) return;
	if (v.x >= gsfb->fbw) return;
	if (v.y < 0) return;
	if (v.y >= gsfb->fbh) return;

	fBufUL[v.y * gsfb->fbw + v.x] = rgba;
}

void primLine(Vertex *vertex) {
	Vertex v[2];

	memcpy(v, vertex, sizeof(Vertex) * 2);

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;

#ifdef GS_LOG
	GS_LOG("primLine %dx%d %lx, %dx%d %lx\n", 
			v[0].x, v[0].y, v[0].rgba,
			v[1].x, v[1].y, v[1].rgba);
#endif

	clipVertex(&v[0]);
	clipVertex(&v[1]);

	if (prim->iip == 0)
		 drawLineF(v);
	else drawLineG(v);
}

void primTriangle(Vertex *vertex) {
	Vertex v[3];

	memcpy(v, vertex, sizeof(Vertex) * 3);

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;
	v[2].x -= offset->x; v[2].y -= offset->y;

#ifdef GS_LOG
	GS_LOG("primTriangle%s%s %dx%d %lx - %dx%d %lx - %dx%d %lx (offset %dx%d)\n",
			prim->iip == 0 ? "F" : "G",
			prim->tme == 0 ? "" : "T",
			v[0].x, v[0].y, v[0].rgba,
			v[1].x, v[1].y, v[1].rgba,
			v[2].x, v[2].y, v[2].rgba,
			offset->x, offset->y);
#endif

	if (prim->tme) {
		if (prim->fst == 0) {
#ifdef GS_LOG
			GS_LOG("stq: %xx%xx%x - %xx%xx%x - %xx%xx%x (tex0.tw=%d, tex0.th=%d)\n",
				   v[0].s, v[0].t, v[0].q,
				   v[1].s, v[1].t, v[1].q, v[2].s, v[2].t, v[2].q,
				   tex0->tw, tex0->th);
#endif

			STQtoUV(&v[0]);
			STQtoUV(&v[1]);
			STQtoUV(&v[2]);
		}

		clipVertexT(&v[0]);
		clipVertexT(&v[1]);
		clipVertexT(&v[2]);

		if (prim->iip)
			 drawTriangleGT(v);
		else {
			if(test->zte) {
				if(tex0->tfx == TEX_MODULATE)
					drawTriangleFTModulate_Z(v);
				else
					drawTriangleFTDecal_Z(v);
			} else {
				if(tex0->tfx == TEX_MODULATE)
					drawTriangleFTModulate(v);
				else
					drawTriangleFTDecal(v);
			}
		}
	} else {
		clipVertex(&v[0]);
		clipVertex(&v[1]);
		clipVertex(&v[2]);

		if (prim->iip)
			 drawTriangleG(v);
		else {
			if(test->zte)
				drawTriangleF_Z(v);
			else
				drawTriangleF(v);
		}
	}
}

void primSprite(Vertex *vertex) {
	Vertex v[2];

	memcpy(v, vertex, sizeof(Vertex) * 2);

#ifdef GS_LOG
	GS_LOG("primSprite%s %dx%d %lx - %dx%d %lx (offset %dx%d) (tex %dx%d %dx%d)\n", 
			prim->tme == 0 ? "" : "T",
			v[0].x, v[0].y, v[0].rgba,
			v[1].x, v[1].y, v[1].rgba,
			offset->x, offset->y,
			v[0].u, v[0].v,
			v[1].u, v[1].v);
#endif

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;

	if (prim->tme == 0) {
		clipVertex(&v[0]);
		clipVertex(&v[1]);

		if(test->zte)		/* If z buffer enabled */
			drawSprite_Z(v);
		else
			drawSprite(v);
	} else {
		if (prim->fst == 0) {
			STQtoUV(&v[0]);
			STQtoUV(&v[1]);
		}

		clipVertexT(&v[0]);
		clipVertexT(&v[1]);

		drawSpriteT(v);
	}
}

void primNull(Vertex *v) {
}

void (*primTable[8])(Vertex *v) = {
	primPoint, primLine, primLine,
	primTriangle, primTriangle, primTriangle,
	primSprite, primNull
};
