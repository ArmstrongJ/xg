//==============================================================================
//
// colormap.h
//
// Copyright (C) 2000,2001 Ralph Lowinski <AltF4@freemint.de>
//------------------------------------------------------------------------------
// 2000-12-14 - Module released for beta state.
// 2000-07-27 - Initial Version.
//==============================================================================
//
#ifndef __COLORMAP_H__
# define __COLORMAP_H__

#include <stdio.h>
#include <stdint.h>
#include <gem.h>

typedef struct {
	CARD16 r, g, b;
} RGB;


void CmapInit(void);

void            CmapPalette   (CARD16 handle);
extern CARD32 (*CmapLookup)   (RGB * dst, const RGB * src);
CARD16          CmapRevertIdx (CARD16 idx);

static inline CARD32 CmapPixelIdx (CARD32 pixel, CARD16 depth)
{
	if (GRPH_Depth > 8) {
		extern CARD32 (*Cmap_PixelIdx) (CARD32 );
		if (depth == 1) pixel =               ~pixel;
		else            pixel = Cmap_PixelIdx (pixel);
	}
	return (pixel & ((1uL << depth) -1));
}

#define RED1000(x) 	(int16_t)((0xFF & (x >> 16)) * 4)
#define GRN1000(x) 	(int16_t)((0xFF & (x >> 8)) * 4)
#define BLU1000(x) 	(int16_t)((0xFF & x) * 4)

static inline void vsx_color(short (*func)(VdiHdl, short), int16_t cindex, CARD16 hdl, CARD32 color)
{
static int incindex;

	if(GRPH_Depth > 8) {
		int16_t rgb[3];
		int16_t i;
		rgb[0] = RED1000(color);
		rgb[1] = GRN1000(color);
		rgb[2] = BLU1000(color);
		for(i=0;i<3;i++) rgb[i] = (rgb[i] > 1000 ? 1000 : rgb[i]);
		
		/* printf("vsx_color: %lx => %d,%d,%d - index %d\n", 
         *        color, rgb[0], rgb[1], rgb[2], cindex);
		 */

		incindex++; if(incindex > 255 || incindex < 100) incindex = 100;

		vs_color(hdl, incindex, (int16_t *)rgb);
		func(hdl, incindex);
	} else {
		func(hdl, (int16_t)color);
	}
}

#define VSF_COLOR(h, c)		vsx_color(vsf_color, 199, h, c)
#define VSL_COLOR(h, c)		vsx_color(vsl_color, 198, h, c)
#define VSM_COLOR(h, c)		vsx_color(vsm_color, 197, h, c)
#define VST_COLOR(h, c)		vsx_color(vst_color, 196, h, c)

#endif __COLORMAP_H__
