//==============================================================================
//
// window_P.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-09-27 - Initial Version.
//==============================================================================
//
#ifndef __WINDOW_P_H__
# define __WINDOW_P_H__

#include "main.h"
#include "clnt.h"
#include "window.h"
#include "tools.h"

#include <X11/X.h>


typedef struct s_BTNGRAB {
	struct s_BTNGRAB * Next;
	BOOL     PntrAsync;
	BOOL     KybdAsync;
	BOOL     OwnrEvents;
	CARD8    Button;
	CARD16   Modifiers;
	CARD16   EventMask;
	p_CLIENT Client;
	p_CURSOR Cursor;
} BTNGRAB;


extern WINDOW * _WIND_PointerRoot;
extern CARD16   _WIND_OpenCounter;

extern CLIENT * _WIND_PgrabClient;
extern WINDOW * _WIND_PgrabWindow;
extern p_CURSOR _WIND_PgrabCursor;
extern CARD32   _WIND_PgrabEvents;
extern BOOL     _WIND_PgrabOwnrEv;
extern BOOL     _WIND_PgrabPassive;

BOOL _Wind_PgrabMatch (p_WINDOW wind, CARD8 but, CARD16 mod);
BOOL _Wind_PgrabClear (p_CLIENT clnt);

// utility functions
BOOL _Wind_IsInferior (p_WINDOW wind, p_WINDOW inferior);
void _Wind_Cursor     (p_WINDOW wind);
int  _Wind_PathStack  (WINDOW ** stack, int * anc, WINDOW * beg, WINDOW * end);


#endif __WINDOW_P_H__
