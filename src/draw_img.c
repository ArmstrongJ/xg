//==============================================================================
//
// draw_img.c
//
// Copyright (C) 2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2001-02-03 - Initial Version.
//==============================================================================
//
#include "main.h"
#include "clnt.h"
#include "pixmap.h"
#include "window.h"
#include "event.h"
#include "gcontext.h"
#include "grph.h"
#include "x_gem.h"

#include <stdlib.h>
#include <stdio.h>

#include <X11/X.h>


//==============================================================================
//
// Callback Functions

#include "Request.h"

//------------------------------------------------------------------------------
void
RQ_PutImage (CLIENT * clnt, xPutImageReq * q)
{
	p_DRAWABLE draw = DrawFind (q->drawable);
	GC       * gc   = GcntFind (q->gc);
	
	if (!draw.p) {
		Bad(Drawable, q->drawable, PutImage,);
		
	} else if (!gc) {
		Bad(GC, q->gc, PutImage,);
	
	} else if ((q->format == XYBitmap  &&  q->depth != 1) ||
	           (q->format != XYBitmap  &&  q->depth != draw.p->Depth)) {
		Bad(Match,, PutImage, /* q->depth */);
	
	} else if ((q->format == ZPixmap  &&  q->leftPad) ||
	           q->leftPad >= PADD_BITS) {
		Bad(Match,, PutImage, /* q->leftPad */);
	
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(Value, (short)((short)q->width <= 0 ? q->width : q->height),
		           PutImage," width = %i height = %i",
		           (short)q->width, (short)q->height);
	
	} else {
		GRECT r[2] = { {0, 0, q->width, q->height} };
		
		if (q->dstX < 0) { r[1].x = 0; r[0].w -= r[0].x = -q->dstX; }
		else             { r[1].x = q->dstX; }
		if (q->dstY < 0) { r[1].y = 0; r[0].h -= r[0].y = -q->dstY; }
		else             { r[1].y = q->dstY; }
		if (r[0].x + r[0].w > draw.p->W) r[0].w = draw.p->W - r[0].x;
		if (r[0].y + r[0].h > draw.p->H) r[0].h = draw.p->H - r[0].y;
		
		if (r[0].w > 0  &&  r[0].h > 0) {
			MFDB mfdb = { (q +1), q->width, q->height,
			              (q->width + q->leftPad + PADD_BITS -1) /16,
			              0, q->depth, 0,0,0 };
			
			r[0].x += q->leftPad;
			r[1].w =  r[0].w;
			r[1].h =  r[0].h;
			
			if (q->depth == 1) {   // all possible formats are matching
				
				DEBUG (PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
				       "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
				       (q->format == XYBitmap ? "Bitmap" :
				        q->format == XYPixmap ? "XYPixmap" :
				        q->format == ZPixmap  ? "ZPixmap"  : "???"),
				       (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
				       q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
				       (q->length *4) - sizeof (xPutImageReq),
				       r[0].x,r[0].y,r[0].w,r[0].h, r[1].x,r[1].y,r[1].w,r[1].h);
				
				if (draw.p->isWind) WindPutMono (draw.Window, gc, r, &mfdb);
				else                PmapPutMono (draw.Pixmap, gc, r, &mfdb);
			
			} else if (q->format == XYPixmap) {
				
				PRINT (- X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
				       "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
				       "XYPixmap", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
				       q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
				       (q->length *4) - sizeof (xPutImageReq),
				       r[0].x,r[0].y,r[0].w,r[0].h, r[1].x,r[1].y,r[1].w,r[1].h);
				
			} else if (q->depth == 16) {
				if (draw.p->isWind) WindPutColor (draw.Window, gc, r, &mfdb);
				else                PmapPutColor (draw.Pixmap, gc, r, &mfdb);
				
			} else if (!(mfdb.fd_addr
			               = malloc (mfdb.fd_wdwidth *2 * q->height * q->depth))) {
				printf ("Can't allocate buffer.\n");
				
			} else { // q->format == ZPixmap
				
				DEBUG (PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
				       "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
				       "ZPixmap", (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
				       q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
				       (q->length *4) - sizeof (xPutImageReq),
				       r[0].x,r[0].y,r[0].w,r[0].h, r[1].x,r[1].y,r[1].w,r[1].h);
				
				if (GRPH_Depth == 4) {
					GrphRasterI4 (mfdb.fd_addr, (char*)(q +1), q->width, q->height);
				} else if (GRPH_Format == SCRN_INTERLEAVED) {
					GrphRasterI8 (mfdb.fd_addr, (char*)(q +1), q->width, q->height);
				} else { // (GRPH_Format == SCRN_PACKEDPIXEL)
					GrphRasterP8 (mfdb.fd_addr, (char*)(q +1), q->width, q->height);
				}
				if (draw.p->isWind) WindPutColor (draw.Window, gc, r, &mfdb);
				else                PmapPutColor (draw.Pixmap, gc, r, &mfdb);
				
				free (mfdb.fd_addr);
			}
		
		} else {
			PRINT (- X_PutImage, " '%s' %c:%lX G:%lX [%i+%i,%i/%u,%u*%u] = %lu\n"
			       "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]",
			       (q->format == XYBitmap ? "Bitmap" :
			        q->format == XYPixmap ? "XYPixmap" :
			        q->format == ZPixmap  ? "ZPixmap"  : "???"),
			       (draw.p->isWind ? 'W' : 'P'), q->drawable, q->gc,
			       q->dstX, q->leftPad, q->dstY, q->width, q->height, q->depth,
			       (q->length *4) - sizeof (xPutImageReq),
			       r[0].x,r[0].y,r[0].w,r[0].h, r[1].x,r[1].y,r[1].w,r[1].h);
		}
	}
}

//------------------------------------------------------------------------------
void
RQ_GetImage (CLIENT * clnt, xGetImageReq * q)
{
	//
	// fully faked atm
	//
	
	p_DRAWABLE draw = DrawFind(q->drawable);
	
	ClntReplyPtr (GetImage, r);
	
	size_t size = (q->width * draw.p->Depth +7) /8 * q->height;
	
	PRINT (- X_GetImage,
	       " D:%lX [%i,%i/%u,%u] form=%i mask=%lX -> dpth = %i size = %lu",
	       q->drawable, q->x,q->y, q->width, q->height, q->format, q->planeMask,
	       draw.p->Depth, size);
	
	r->depth  = draw.p->Depth;
	r->visual = DFLT_VISUAL;
	
	ClntReply (GetImage, size, NULL);
}


//------------------------------------------------------------------------------
#define CPsrcP 0x01
#define CPsrcW 0x02
#define CPdstP 0x10
#define CPdstW 0x20
//------------------------------------------------------------------------------
static short
clip_dst (GRECT * r, p_DRAWABLE draw, const short * coord)
{
	// coord[0,1] src x/y   r[0] src clip
	// coord[2,3] dst x/y   r[1] dst clip
	// coord[4,5] w/h
	
	short w = coord[4];
	short h = coord[5];
	short t;
	
	if (!draw.p->isWind)                t = CPdstP;
	else if (WindVisible (draw.Window)) t = CPdstW;
	else                                return 0;
	
	*(PXY*)&r[0] = *(PXY*)(coord +0); // src
	*(PXY*)&r[1] = *(PXY*)(coord +2); // dst
	
	if (r[1].x     < 0)         { r[0].x -= r[1].x; w += r[1].x;    r[1].x = 0; }
	if (r[1].x + w > draw.p->W)                     w = draw.p->W - r[1].x;
	if (r[1].y     < 0)         { r[0].y -= r[1].y; h += r[1].y;    r[1].y = 0; }
	if (r[1].y + h > draw.p->H)                     h = draw.p->H - r[1].y;
	
	if (w <= 0  ||  h <= 0) return 0;
	
	r[0].w = r[1].w = w;
	r[0].h = r[1].h = h;
	
	return t;
}

//------------------------------------------------------------------------------
static short
clip_src (GRECT * r, p_DRAWABLE draw)
{
	// r[0]   src clip
	// r[1]   dst clip
	// r[2..] dst to be tiled
	short   num   = 0;   // -1: no copy, 0: no tiles, 1..4: num of tiles in r
	GRECT * tile  = r +2;
	BOOL    below = xFalse;
	short   x     = r[1].x;
	short   y     = r[1].y;
	short   w     = r[1].w;
	short   h     = r[1].h;
	short   d;
	
	#define SET(t, _x,_y,_w,_h)   t->x = _x; t->y = _y; t->w = _w; t->h = _h
	
	if (h > (d = draw.p->H - r[0].y)) { // below src
		below = xTrue;
		SET ((r +5), x, y + d, w, h - d);
		h = d;
		if (h <= 0) return -1;
	}
	if (r[0].y < 0) { // above src
		if ((h += r[0].y) <= 0) return -1;
		num++;
		SET (tile, x, y, w, -r[0].y); tile++;
		y      -= r[0].y;
		r[0].y =  0;
	}
	if (r[0].x < 0) { // left of src
		if ((w += r[0].x) <= 0) return -1;
		num++;
		SET (tile, x, y, -r[0].x, h); tile++;
		x      -= r[0].x;
		r[0].x =  0;
	}
	if (w > (d = draw.p->W - r[0].x)) { // right of src
		num++;
		SET (tile, x + d, y, w - d, h); tile++;
		w = d;
		if (w <= 0) return -1;
	}
	if (below && (++num < 4)) {
		*tile = r[5];
	}
	r[1].x          = x;
	r[1].y          = y;
	r[1].w = r[0].w = w;
	r[1].h = r[0].h = h;
	
	#undef SET
	
	return num;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
RQ_CopyArea (CLIENT * clnt, xCopyAreaReq * q)
{
	p_DRAWABLE src_d = DrawFind(q->srcDrawable);
	p_DRAWABLE dst_d = DrawFind(q->dstDrawable);
	GC       * gc    = GcntFind(q->gc);
	
	if (!src_d.p) {
		Bad(Drawable, q->srcDrawable, CopyArea,);
		
	} else if (!dst_d.p) {
		Bad(Drawable, q->dstDrawable, CopyArea,);
		
	} else if (!gc) {
		Bad(GC, q->gc, CopyArea,);
	
//	} else if (src_d.p->Depth != dst_d.p->Depth) {
//		Bad(Match,, CopyArea,"(%c:%X,%c:%X):\n           depth %u != %u.",
//		            (src_d.p->isWind ? 'W' : 'P'), src_d.p->Id,
//		            (dst_d.p->isWind ? 'W' : 'P'), dst_d.p->Id,
//		            src_d.p->Depth, dst_d.p->Depth);
	
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(Value, (short)((short)q->width <= 0 ? q->width : q->height),
		           CopyArea," width = %i height = %i",
		           (short)q->width, (short)q->height);
	
	} else {
		BOOL    debug = xTrue;
		GRECT   rect[6], work;
		GRECT * exps = rect +2;
		short   nExp   = -1;
		PRECT * sect   = NULL;
		short   nSct   = 0;
		PXY     orig;
		short   action;
		
		if (gc->ClipNum < 0 || !(action = clip_dst (rect, dst_d, &q->srcX))) {
			// the area doesn't intersects the destination drawable or the
			// drawable isn't viewable
			//
			PRINT (- X_CopyArea," G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX (%i,%i) %c",
			       q->gc, (src_d.p->isWind ? 'W' : 'P'), q->srcDrawable,
			       q->srcX, q->srcY, q->width, q->height,
			       (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable,
			       q->dstX, q->dstY, (gc->GraphExpos ? '*' : '-'));
			if (gc->GraphExpos) {
				EvntNoExposure (clnt, dst_d.p->Id, X_CopyArea);
			}
			return;
		}
		if (!src_d.p->isWind) {
			action |= CPsrcP;
		} else if (src_d.p == dst_d.p
		           || (action == CPdstP && WindVisible (src_d.Window))) {
			action |= CPsrcW;
		}
		
		if (action == (CPsrcW|CPdstW)) {
			// copy inside one window means scroll a part of this window
			//
			PXY diff = { rect[1].x - rect[0].x, rect[1].y - rect[0].y };
			if (diff.x || diff.y) {
				WINDOW * wind = dst_d.Window;
				GrphCombine (&rect[0], &rect[1]);
				if ((nSct = WindClipLockP (wind, 0, rect,1, &orig, &sect))) {
					exps = (gc->GraphExpos ? (GRECT*)(sect + nSct +1) : NULL);
					nExp = WindScroll (wind, gc, rect, diff,
							              orig, sect, nSct, exps);
					debug = xFalse;
				}
			}
			PRINT (CopyArea," W->W");
		
		} else {
			work = rect[1]; // save the destination area
		
			if (action & (CPsrcP|CPsrcW)) {
				// the action is _not_ copy window to _other_ window but one of:
				// CPsrcP|CPdstP, CPsrcP|CPdstW, CPsrcW|CPdstP
				//
				nExp = clip_src (rect, src_d);
			}
			if (nExp) {
				if (nExp < 0) {
					// no part of the source intersects the destination, or both are
					// different windows, so only generate events for the visible parts
					//
					exps   =  &rect[1];
					nExp   =  1;
					action &= ~(CPsrcP|CPsrcW);
				}
				if (gc->ClipNum > 0) {
					// intersects destination and clipping rectangels for events
					//
					GRECT * r = exps;
					exps      = alloca (sizeof(GRECT) * nExp * gc->ClipNum);
					nExp      = GrphInterList (exps, gc->ClipRect, gc->ClipNum, r,nExp);
					if (!nExp && !(action & (CPsrcP|CPsrcW))) {
						action = 0x00;
					}
				}
			}
			if (action & CPdstW) {
				WINDOW * wind = dst_d.Window;
				if (!(nSct = WindClipLockP (wind, 0, &work,1, &orig, &sect))) {
					action = 0x00;
				
				} else {
					if (nExp) {
						GRECT * r = (gc->GraphExpos ? (GRECT*)(sect + nSct) : NULL);
						short   e = 0;
						if (wind->hasBackGnd) {
							if (!wind->hasBackPix) {
								vswr_mode (GRPH_Vdi, MD_REPLACE);
								vsf_color (GRPH_Vdi, wind->Back.Pixel);
							}
							do {
								PRECT area;
								int   n;
								area.rd.x = (area.lu.x = orig.x + exps->x) + exps->w -1;
								area.rd.y = (area.lu.y = orig.y + exps->y) + exps->h -1;
								n = WindDrawBgnd (wind, orig, &area, sect, nSct, r);
								if (n && r) {
									e += n;
									r += n;
								}
								exps++;
							} while (--nExp);
							
						} else if (gc->GraphExpos) {
							PRECT * s = sect;
							short   n = nSct;
							do {
								GRECT a = { s->lu.x - orig.x,     s->lu.y - orig.y,
								            s->rd.x - s->lu.x +1, s->rd.y - s->lu.y +1 };
								e += GrphInterList (r + e, &a, 1, exps, nExp);
								s++;
							} while (--n);
						}
						nExp = e;
						exps = (GRECT*)(sect + nSct);
					}
				}
			}
			switch (action) {
				// case 0x00|0x00, 0x00|CPdstP: do nothing
				
				case CPsrcP|CPdstP: {
					if (src_d.p->Depth == 1) {
						PmapPutMono (dst_d.Pixmap, gc, rect, PmapMFDB(src_d.Pixmap));
					} else {
						PmapPutColor (dst_d.Pixmap, gc, rect, PmapMFDB(src_d.Pixmap));
					}
					debug = xFalse;
				}	break;
				
				case CPsrcP|CPdstW: {
					WindPutImg (dst_d.Window, gc, rect, PmapMFDB(src_d.Pixmap),
					            orig, sect, nSct);
					debug = xFalse;
				}	break;
				
				case CPsrcW|CPdstP: PRINT (CopyArea," W->P"); break;
				
				case CPsrcW|CPdstW: {/*
					PXY diff = { rect[1].x - rect[0].x, rect[1].y - rect[0].y };
					if (diff.x || diff.y) {
						if (gc->GraphExpos) {
							nExp += copy_scroll (dst_d.Window, *(PXY*)rect, diff,
							                     sect, nSct, exps + nExp);
						} else {
							copy_scroll (dst_d.Window, *(PXY*)rect, diff,
							             sect, nSct, NULL);
						}
					}
					PRINT (CopyArea," W->W");
				*/}	break;
				
				default:            PRINT (CopyArea," 0x%02x", action); debug = xFalse;
			}
		}
		
		if (gc->GraphExpos) {
			if (nExp) EvntGraphExp (clnt, dst_d, X_CopyArea, nExp, exps);
			else      EvntNoExposure (clnt, dst_d.p->Id, X_CopyArea);
		}
		
		if (sect) WindClipOff();
		
		if (debug) {
			PRINT (- X_CopyArea," G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX (%i,%i)\n"
				    "          [%i,%i/%i,%i] -> [%i,%i/%i,%i]  %c",
			       q->gc, (src_d.p->isWind ? 'W' : 'P'), q->srcDrawable,
			       q->srcX, q->srcY, q->width, q->height,
			       (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable,
			       q->dstX, q->dstY,
				    rect[0].x,rect[0].y,rect[0].w,rect[0].h,
				    rect[1].x,rect[1].y,rect[1].w,rect[1].h,
				    (gc->GraphExpos ? '*' : '-'));
		}
	}
}

//------------------------------------------------------------------------------
static int
_mask2plane (CARD32 mask, CARD16 depth)
{
	int plane = -1;
	
	if (mask) while (++plane < depth) {
		if (mask & 1) {
			mask &= ~1uL;
			break;
		}
		mask >>= 1;
	}
	return (mask ? -1 : plane);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void
RQ_CopyPlane (CLIENT * clnt, xCopyPlaneReq * q)
{
	p_DRAWABLE src_d = DrawFind(q->srcDrawable);
	p_DRAWABLE dst_d = DrawFind(q->dstDrawable);
	GC       * gc    = GcntFind (q->gc);
	int        plane;
	
	if (!src_d.p) {
		Bad(Drawable, q->srcDrawable, CopyPlane,);
		
	} else if (!dst_d.p) {
		Bad(Drawable, q->dstDrawable, CopyPlane,);
		
	} else if (!gc) {
		Bad(GC, q->gc, CopyPlane,);
	
	} else if ((plane = _mask2plane(q->bitPlane, src_d.p->Depth)) < 0) {
		Bad(Value, q->bitPlane, CopyPlane," bit-plane = %lX", q->bitPlane);
	
	} else if ((short)q->width <= 0  ||  (short)q->height <= 0) {
		Bad(Value, (short)((short)q->width <= 0 ? q->width : q->height),
		           CopyPlane," width = %i height = %i",
		           (short)q->width, (short)q->height);
	
	} else {
		BOOL debug = xTrue;
		GRECT r[4] = { {0,       0,       src_d.p->W, src_d.p->H},
		               {0,       0,       dst_d.p->W, dst_d.p->H},
		               {q->srcX, q->srcY, q->width,   q->height},
		               {q->dstX, q->dstY, q->width,   q->height} };
		
		if (GrphIntersect (&r[0], &r[2]) && GrphIntersect (&r[1], &r[3])) {
			if (r[0].w > r[1].w) r[0].w = r[1].w;
			else                 r[1].w = r[0].w;
			if (r[0].h > r[1].h) r[0].h = r[1].h;
			else                 r[1].h = r[0].h;
			
			if (src_d.p->isWind) {
				//
				
			} else { // src_d is Pixmap
				MFDB mfdb = *PmapMFDB(src_d.Pixmap);
				
				DEBUG (CopyPlane," #%i(0x%lx)"
				       " G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX %i,%i",
				       plane, q->bitPlane, q->gc, (src_d.p->isWind ? 'W' : 'P'),
				       q->srcDrawable, q->srcX, q->srcY, q->width, q->height,
			   	    (dst_d.p->isWind ? 'W' : 'P'),
			   	    q->dstDrawable, q->dstX, q->dstY);
				
				if (src_d.p->Depth > 1) {
					if (plane) {
						mfdb.fd_addr = src_d.Pixmap->Mem
						           + src_d.Pixmap->nPads *2 * src_d.Pixmap->H * plane;
					}
					mfdb.fd_nplanes = 1;
				}
				if (dst_d.p->isWind) WindPutMono (dst_d.Window, gc, r, &mfdb);
				else                 PmapPutMono (dst_d.Pixmap, gc, r, &mfdb);
				debug = xFalse;
			}
			
		}
		if (debug) {
			PRINT (- X_CopyPlane," #%i(0x%lx)"
			       " G:%lX %c:%lX [%i,%i/%u,%u] to %c:%lX %i,%i",
			       plane, q->bitPlane, q->gc, (src_d.p->isWind ? 'W' : 'P'),
			       q->srcDrawable, q->srcX, q->srcY, q->width, q->height,
			       (dst_d.p->isWind ? 'W' : 'P'), q->dstDrawable, q->dstX, q->dstY);
		}
		/*
		 *  Hacking: GraphicsExposure should be generated if necessary instead!
		 */
		if (gc->GraphExpos) {
			EvntNoExposure (clnt, dst_d.p->Id, X_CopyPlane);
		}
	}
}
