//==============================================================================
//
// wind_pntr.c
//
// Copyright (C) 2000 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-09-28 - Initial Version.
//==============================================================================
//
#include "window_P.h"
#include "event.h"
#include "grph.h"
#include "Cursor.h"
#include "wmgr.h"
#include "x_gem.h"

#include <stdio.h>

#include <X11/Xproto.h>


#ifdef TRACE
#	define _TRACE_POINTER
#	define _TRACE_FOCUS
#endif


WINDOW * _WIND_PointerRoot = &WIND_Root;


#define BMOTION_MASK (Button1MotionMask|Button2MotionMask| \
                      Button3MotionMask|Button4MotionMask|Button5MotionMask)
#define ALL_MOTION_MASK (PointerMotionMask|ButtonMotionMask|BMOTION_MASK)


//------------------------------------------------------------------------------
#if defined(_TRACE_POINTER) || defined(_TRACE_FOCUS)
static const char *
_M2STR (CARD8 mode)
{
	static char inval[16];
	
	switch (mode) {
		case NotifyAncestor:           return "ancestor";
		case NotifyVirtual:            return "virtual";
		case NotifyInferior:           return "inferior";
		case NotifyNonlinear:          return "nonlinear";
		case NotifyNonlinearVirtual:   return "nonl.virt.";
		case NotifyPointer:            return "pointer";
		case NotifyPointerRoot:        return "ptr.root";
		case NotifyDetailNone:         return "no_detail";
	}
	sprintf (inval, "<%u>", mode);
	
	return inval;
}

# define TRACEP(w,el,m,p,id,r) printf ("W:%X " el " %s [%i,%i] W:%lX [%i,%i] \n", \
                                       w->Id, _M2STR(m), p.x, p.y, id, r.x, r.y)
# define TRACEF(w,el,m) printf ("W:%X " el " %s \n", w->Id, _M2STR(m))

#else
# define TRACEP(w,el,m,p,id,r)
# define TRACEF(w,el,m)
#endif

//==============================================================================
void
WindPointerWatch (BOOL movedNreorg)
{
	CARD8    evnt, next, last;
	GRECT    sect;
	WINDOW * stack[32] = { _WIND_PointerRoot }, * widget = NULL;
	int      anc = 0, top, bot;
	short    hdl;
	PXY      e_p = // pointer coordinates relative to the event windows origin
			         *MAIN_PointerPos,
			   r_p = // pointer coordinates relative to the new pointer root origin
			         { MAIN_PointerPos->x - WIND_Root.Rect.x,
			           MAIN_PointerPos->y - WIND_Root.Rect.y };
	CARD32   r_id = -1;
	
	BOOL watch = (0 != (WIND_Root.u.List.AllMasks & ALL_MOTION_MASK));
	
	/*--- flatten the leave path to the stack list ---*/
	
	if (!*stack) {
		// noting to leave
		e_p.x -= WIND_Root.Rect.x;
		e_p.y -= WIND_Root.Rect.y;
		stack[++anc] = &WIND_Root;
		
	} else while (1) {
		WINDOW * w = stack[anc];
		e_p.x -= w->Rect.x;
		e_p.y -= w->Rect.y;
		if (w->Parent) stack[++anc] = w->Parent;
		else           break;
	}
	top = anc;
	
	/*--- find the gem window the pointer is over ---*/
	
	if ((hdl = wind_find (MAIN_PointerPos->x, MAIN_PointerPos->y)) < 0) {
		// the pointer is outside of all windows, e.g. in the title bar
		
		puts("*PLONK*");
		sect  = WIND_Root.Rect;
		watch = xFalse;
		if (!anc || stack[anc-1]) stack[top] = NULL;
		else                      top        = anc -1;
		
	} else {
		r_id = (ROOT_WINDOW|hdl);
		if (hdl > 0) {
			stack[++top] = NULL;
			if (_WIND_OpenCounter) {
				WINDOW * w = WIND_Root.StackTop;
				do if (w->Handle == hdl) {
					// over x window
					if (w->isMapped) {
						watch |= (0 != (w->u.List.AllMasks & ALL_MOTION_MASK));
						r_p.x -= w->Rect.x;
						r_p.y -= w->Rect.y;
						stack[top] = w;
					}
					break;
				} while ((w = w->PrevSibl));
			}
			// else over some gem window
			
			if (stack[anc-1] == stack[top]) {
				top = --anc;
			}
		}
		// else (hdl == 0), over root window
		
		/*--- find the windows section the pointer is in ---*/
		
		if (!stack[top]  &&  hdl == wind_get_top()) {
			// over the top gem window, can be watched including widgets
			
			wind_get_curr (hdl, &sect);
			watch = xFalse;
			
		} else {
			// find section
			wind_get_first (hdl, &sect);
			while (sect.w && sect.h && !PXYinRect (MAIN_PointerPos, &sect)) {
				wind_get_next (hdl, &sect);
			}
			if (!sect.w || !sect.h) {
				// the pointer is outside of all work areas, so it's over a gem widget
				
				*(PXY*)&sect = *MAIN_PointerPos;
				sect.w = sect.h = 1;
				watch = xFalse;
				
				if (!hdl) {
					top++;
				} else if (stack[top]  &&  anc == top) {
					stack[++anc] = &WIND_Root;
					top = anc +1;
				}
				if (!anc || stack[anc-1]) stack[top] = NULL;
				else                      top        = anc -1;
			
			} else if (hdl && stack[top]) {
				
				/*--- find the subwindow and its section the pointer is in ---*/
				
				GRECT o = // the origin of the window, absolute coordinate
				          { WIND_Root.Rect.x + stack[top]->Rect.x,
				            WIND_Root.Rect.y + stack[top]->Rect.y };
				
				if (r_p.x < 0  ||  r_p.x >= stack[top]->Rect.w  ||
				    r_p.y < 0  ||  r_p.y >= stack[top]->Rect.h) {
					// the pointer is over the border of the main window
					
					WINDOW * w = stack[top];
					short    b = w->GwmDecor;
					if (b) widget = w;
					else   b      = w->BorderWidth;
					if      (r_p.x <  0)         { o.x -= b;         o.w = b; }
					else if (r_p.x >= w->Rect.w) { o.x += w->Rect.w; o.w = b; }
					else                         { o.w =  w->Rect.w;  }
					if      (r_p.y <  0)         { o.y -= b;         o.h = b; }
					else if (r_p.y >= w->Rect.h) { o.y += w->Rect.h; o.h = b; }
					else                         { o.h =  w->Rect.h;  }
					GrphIntersect (&sect, &o);
					watch = xFalse;
					if (stack[anc-1]) {
						top = anc;
						stack[++top] = NULL;
					} else {
						top = --anc;
					}
				
				} else {
					WINDOW * w = stack[top]->StackTop;
					while (w) {
						if (w->isMapped && PXYinRect (&r_p, &w->Rect)) {
							if (anc &&  w == stack[anc-1]) {
								top = --anc;
							} else if (top == sizeof(stack) / sizeof(*stack)) {
								break;
							} else {
								stack[++top] = w;
							}
							watch |= (0 != (w->u.List.AllMasks & ALL_MOTION_MASK));
							// set origin to inferiors value
							o.x += w->Rect.x;
							o.y += w->Rect.y;
							// update relative pointer position for new origin
							r_p.x -= w->Rect.x;
							r_p.y -= w->Rect.y;
							w   =  w->StackTop;
							continue;
						}
						w = w->PrevSibl;
					}
					r_id = stack[top]->Id;
					o.w  = stack[top]->Rect.w;
					o.h  = stack[top]->Rect.h;
					GrphIntersect (&sect, &o);
					if ((w = stack[top]->StackBot)) {
						sect.w += sect.x;
						sect.h += sect.y;
						do {
							int q;
							if        ((q =  w->Rect.x) >  r_p.x) {
									 if (q <= sect.w - o.x) sect.w = q + o.x;
							} else if ((q += w->Rect.w) <= r_p.x) {
									 if (q >  sect.x - o.x) sect.x = q + o.x;
							} else if ((q =  w->Rect.y) >  r_p.y) {
									 if (q <= sect.h - o.y) sect.h = q + o.y;
							} else if ((q += w->Rect.h) <= r_p.y) {
									 if (q >  sect.y - o.y) sect.y = q + o.y;
							}
						} while ((w = w->NextSibl));
						sect.w -= sect.x;
						sect.h -= sect.y;
					}
				}
			}
		}
	}
	_WIND_PointerRoot = stack[top];
	if (_WIND_OpenCounter) {
		MainSetWatch (&sect, MO_LEAVE);
		MainSetMove  (watch);
	} else {
		MainClrWatch (0);
		MainSetMove  (xFalse);
	}
	
	if (!widget  &&  stack[0] == stack[top]) {
		if (WMGR_Cursor) {
			WmgrWidgetOff (NULL);
		}
		if (movedNreorg && stack[top]) {
			TRACEP (stack[top], "      moved", -1, e_p, r_id, r_p);
			WindPointerMove (&r_p);
		}
		return;
	}
	
	/*--- generate events ---*/
	
	if (anc == 0) {
		evnt = NotifyInferior;
		next = NotifyVirtual;
		last = NotifyAncestor;
	} else if (anc == top) {
		evnt = NotifyAncestor;
		next = NotifyVirtual;
		last = NotifyInferior;
	} else {
		evnt = NotifyNonlinear;
		next = NotifyNonlinearVirtual;
		last = NotifyNonlinear;
	}
	
	// notify enter/in events
	
	if (stack[0]) {
		TRACEP (stack[0], "Leave", evnt, e_p, r_id, r_p);
		if (stack[0]->u.List.AllMasks & LeaveWindowMask) {
			EvntLeaveNotify (stack[0], r_id, None, r_p, e_p,
			                 NotifyNormal, ELFlagSameScreen, evnt);
		}
		if (stack[0]->u.List.AllMasks & FocusChangeMask) {
			TRACEF (stack[0], "FocusOut", evnt);
			EvntFocusOut (stack[0], NotifyNormal, evnt);
		}
		if (anc > 0) {
			e_p.x += stack[0]->Rect.x;
			e_p.y += stack[0]->Rect.y;
		}
	}
	for (bot = 1; bot < anc; ++bot) {
		TRACEP (stack[bot], "Leave", next, e_p, r_id, r_p);
		if (stack[bot]->u.List.AllMasks & LeaveWindowMask) {
			EvntLeaveNotify (stack[bot], r_id, None, r_p, e_p,
			                 NotifyNormal, ELFlagSameScreen, next);
		}
		if (stack[bot]->u.List.AllMasks & FocusChangeMask) {
			TRACEF (stack[bot], "FocusOut", next);
			EvntFocusOut (stack[bot], NotifyNormal, next);
		}
		e_p.x += stack[bot]->Rect.x;
		e_p.y += stack[bot]->Rect.y;
	}
	
	if (widget) {
		if (WMGR_Active) {
			TRACEP (widget, "      widget", -1, e_p, r_id, r_p);
			WmgrWidget (widget, &r_p);
		}
	
	} else { // notify leave/out events
		CURSOR * crsr = NULL;
		
		for (bot = anc +1; bot < top; ++bot) {
			e_p.x -= stack[bot]->Rect.x;
			e_p.y -= stack[bot]->Rect.y;
			TRACEP (stack[bot], "Enter", next, e_p, r_id, r_p);
			if (stack[bot]->u.List.AllMasks & EnterWindowMask) {
				EvntEnterNotify (stack[bot], r_id, None, r_p, e_p,
				                 NotifyNormal, ELFlagSameScreen, next);
			}
			if (stack[bot]->u.List.AllMasks & FocusChangeMask) {
				TRACEF (stack[bot], "FocusIn", next);
				EvntFocusIn (stack[bot], NotifyNormal, next);
			}
		}
		if (stack[top]) {
			if (anc < top) {
				e_p.x -= stack[top]->Rect.x;
				e_p.y -= stack[top]->Rect.y;
			}
			TRACEP (stack[top], "Enter", last, e_p, r_id, r_p);
			if (stack[top]->u.List.AllMasks & EnterWindowMask) {
				EvntEnterNotify (stack[top], r_id, None, r_p, e_p,
				                 NotifyNormal, ELFlagSameScreen, last);
			}
			if (stack[top]->u.List.AllMasks & FocusChangeMask) {
				TRACEF (stack[top], "FocusIn", last);
				EvntFocusIn (stack[top], NotifyNormal, last);
			}
			crsr = stack[top]->Cursor;
		}
		
		if (WMGR_Cursor) WmgrWidgetOff (crsr);
		else             CrsrSelect    (crsr);
	}
}

//==============================================================================
void
WindPointerMove (const p_PXY pointer_xy)
{
	PXY r_xy, e_xy;
	WINDOW * wind = _WIND_PointerRoot;
	CARD32   r_id;
	CARD32 msk =  MAIN_KeyButMask & BMOTION_MASK;
	       msk |= (msk ? PointerMotionMask|ButtonMotionMask : PointerMotionMask);
	
	if (!wind) {
		puts("*BANG*");
		MainSetMove (xFalse);
		return;
	}
	if (pointer_xy) {   // called internal, not from main
		r_xy = *pointer_xy;
	
	} else if (wind) {
		r_xy = *MAIN_PointerPos;
		do {
			r_xy.x -= wind->Rect.x;
			r_xy.y -= wind->Rect.y;
		} while ((wind = wind->Parent));
		wind = _WIND_PointerRoot;
	}
	e_xy = r_xy;
	r_id = wind->Id;
	
	do {
		if (msk & wind->u.List.AllMasks) {
			EvntMotionNotify (wind, msk, r_id, r_id, r_xy, e_xy);
		#	ifdef _TRACE_POINTER
			printf ("moved on W:%X [%i,%i] \n", wind->Id, e_xy.x, e_xy.y);
		#	endif
			msk &= !wind->u.List.AllMasks;
		}
		if (msk &= wind->PropagateMask) {
			e_xy.x += wind->Rect.x;
			e_xy.y += wind->Rect.y;
		} else {
			break;
		}
	} while ((wind = wind->Parent));
}


//==============================================================================
//
// Callback Functions

#include "Request.h"

//------------------------------------------------------------------------------
void
RQ_QueryPointer (CLIENT * clnt, xQueryPointerReq * q)
{
	// Reply:
	// BOOL   sameScreen:
	// Window root:
	// Window child:
	// INT16  rootX, rootY:
	// INT16  winX,  winY:
	// CARD16 mask:
	
	WINDOW * wind = WindFind (q->id);
	
	if (!wind) {
		Bad(Window, q->id, QueryPointer,);
	
	} else {
		ClntReplyPtr (QueryPointer, r);
		
		DEBUG (QueryPointer," W:%lX", q->id);
		
		r->sameScreen = xTrue;
		r->mask       = MAIN_KeyButMask;
		r->child      = None;
		
		if (!_WIND_PointerRoot) {
			GRECT work;
			short hdl = wind_get_top();
			wind_get_work (hdl, &work);
			r->root  = ROOT_WINDOW|hdl;
			r->rootX = MAIN_PointerPos->x - work.x;
			r->rootY = MAIN_PointerPos->y - work.y;
		
		} else if (_WIND_PointerRoot == &WIND_Root) {
			r->root  = ROOT_WINDOW;
			r->rootX = MAIN_PointerPos->x - WIND_Root.Rect.x;
			r->rootY = MAIN_PointerPos->y - WIND_Root.Rect.y;
		
		} else {
			PXY pxy;
			WindOrigin (wind, &pxy);
			r->root  = _WIND_PointerRoot->Id;
			r->rootX = MAIN_PointerPos->x - pxy.x;
			r->rootY = MAIN_PointerPos->y - pxy.y;
		}
		
		if (q->id == r->root) {
			r->winX  = r->rootX;
			r->winY  = r->rootY;
		
		} else if (q->id == ROOT_WINDOW) {
			r->winX = MAIN_PointerPos->x - WIND_Root.Rect.x;
			r->winY = MAIN_PointerPos->y - WIND_Root.Rect.y;
			r->child = r->root;
		
		} else {
			PXY pxy;
			WindOrigin (wind, &pxy);
			r->winX = MAIN_PointerPos->x - pxy.x;
			r->winY = MAIN_PointerPos->y - pxy.y;
			
			if ((wind = _WIND_PointerRoot)) {
				while (wind != &WIND_Root) {
					if (wind->Id == q->id) {
						r->child = _WIND_PointerRoot->Id;
						break;
					}
					wind = wind->Parent;
				}
			}
		}
		ClntReply (QueryPointer,, "wwPP.");
	}
}

//------------------------------------------------------------------------------
void
RQ_TranslateCoords (CLIENT * clnt, xTranslateCoordsReq * q)
{
	// Window srcWid:
	// Window dstWid:
	// INT16 srcX, srcY:
	//
	// Reply:
	// BOOL sameScreen:
	// Window child:
	// INT16 dstX, dstY:
	//...........................................................................
	
	WINDOW * wsrc = NULL, * wdst = NULL;
	BOOL     ok   = xTrue;
	PXY      p_src, p_dst;
	
	if (q->srcWid & ~RID_MASK) {
		if ((ok = ((wsrc = WindFind(q->srcWid)) != NULL))) {
			WindOrigin (wsrc, &p_src);
		}
	} else {
		GRECT work;
		if ((ok = wind_get_work (q->srcWid & 0x7FFF, &work))) {
			p_src.x = work.x - WIND_Root.Rect.x;
			p_src.y = work.y - WIND_Root.Rect.y;
		}
	}
	if (!ok) {
		Bad(Window, q->srcWid, TranslateCoords,"(): source not found.");
	
	} else if (q->dstWid & ~RID_MASK) {
		if ((ok = ((wdst = WindFind(q->dstWid)) != NULL))) {
			WindOrigin (wdst, &p_dst);
		}
	} else {
		GRECT work;
		if ((ok = wind_get_work (q->dstWid & 0x7FFF, &work))) {
			p_dst.x = work.x - WIND_Root.Rect.x;
			p_dst.y = work.y - WIND_Root.Rect.y;
		}
	}
	if (!ok) {
		Bad(Window, q->dstWid, TranslateCoords,"(): destination not found.");
	
	} else { //..................................................................
		ClntReplyPtr (TranslateCoords, r);
		
		DEBUG (TranslateCoords," %i/%i for W:%lX on W:%lX",
		       q->srcX, q->srcY, q->dstWid, q->srcWid);
		
		r->sameScreen = xTrue;
		r->dstX       = q->srcX + p_src.x - p_dst.x;
		r->dstY       = q->srcY + p_src.y - p_dst.y;
		r->child      = None;
		if (wdst  &&  r->dstX >= 0  &&  r->dstX < wdst->Rect.w
		          &&  r->dstY >= 0  &&  r->dstY < wdst->Rect.h
		          &&  wdst->StackTop  &&  WindVisible (wdst)) {
			p_dst = *(PXY*)&r->dstX;
			wdst  = wdst->StackTop;
			do {
				if (PXYinRect (&p_dst, &wdst->Rect) && wdst->isMapped) {
					r->child = wdst->Id;
					p_dst.x -= wdst->Rect.x;
					p_dst.y -= wdst->Rect.y;
					if (!(wdst = wdst->StackTop)) break;
					else                          continue;
				}
			} while ((wdst = wdst->PrevSibl));
		}
		ClntReply (TranslateCoords,, "wP");
	}
}

//------------------------------------------------------------------------------
void
RQ_WarpPointer (CLIENT * clnt, xWarpPointerReq * q)
{
	PRINT (- X_WarpPointer," from W:%lX [%i,%i/%u,%u] to W:%lX %i/%i",
	       q->srcWid, q->srcX, q->srcY, q->srcWidth, q->srcHeight, q->dstWid,
	       q->dstX, q->dstY);
}