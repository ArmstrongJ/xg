#include "x_gem.h"

#include <stddef.h>
#include <stdio.h> // printf


// VDIPB { vdi_control, vdi_intin, vdi_intout, vdi_ptsin, vdi_ptsout };
// AESPB { aes_control, aes_global,
//                         aes_intin, aes_intout, aes_addrin, aes_addrout };


//==============================================================================
void
vs_clip_r (int handle, const GRECT * rect)
{
	if (rect) {
		vdi_intin[0] = 1;
		vdi_ptsin[0] = rect->x;
		vdi_ptsin[1] = rect->y;
		vdi_ptsin[2] = rect->x + rect->w -1;
		vdi_ptsin[3] = rect->y + rect->h -1;
	
	} else {
		vdi_intin[0] = 0;
	}
	
	vdi_control[0] = 129;
	vdi_control[1] = 2;
	vdi_control[3] = 1;
	vdi_control[5] = 0;
	vdi_control[6] = handle;
	vdi(&vdi_params);
}


//==============================================================================
void
vqt_extent_n (int handle, const short * str, int count, short * ext)
{
	vdi_params.intin  = (short*)str;
	vdi_params.ptsout = ext;
	
	vdi_control[0] = 116;
	vdi_control[1] = 0;
	vdi_control[3] = count;
	vdi_control[6] = handle;
	vdi (&vdi_params);
	
	vdi_params.intin  = vdi_intin;
	vdi_params.ptsout = vdi_ptsout;
}


//==============================================================================
void
v_bar_p (int handle, const PXY pxyarray[])
{
	vdi_params.ptsin = (short*)pxyarray;

	vdi_control[0] = 11;
	vdi_control[1] = 2;
	vdi_control[3] = 0;
	vdi_control[5] = 1;
	vdi_control[6] = handle;
	vdi(&vdi_params);
	
	vdi_params.ptsin = vdi_ptsin;
}

//==============================================================================
void
v_fillarea_p (int handle, int count, const PXY pxyarray[])
{
	vdi_params.ptsin = (short*)pxyarray;
	
	vdi_control[0] = 9;
	vdi_control[1] = count;
	vdi_control[3] = 0;
	vdi_control[6] = handle;
	vdi (&vdi_params);
	
	vdi_params.ptsin = vdi_ptsin;
}

//==============================================================================
void
v_gtext_n (int handle, const PXY * p, int count, const char * str) 
{
	short i;
	
	for (i = 0; i < count; ++i) {
		vdi_intin[i] = *(str++);
	}
	*(PXY*)vdi_ptsin = *p;
	
	vdi_control[0] = 8;
	vdi_control[1] = 2;
	vdi_control[3] = i;
	vdi_control[6] = handle;
	vdi (&vdi_params);
}

//==============================================================================
void
v_gtext_arr (int handle, const PXY * p, int count, const short * arr) 
{
	vdi_params.intin = (short*)arr;
	
	*(PXY*)vdi_ptsin = *p;
	
	vdi_control[0] = 8;
	vdi_control[1] = 2;
	vdi_control[3] = count;
	vdi_control[6] = handle;
	vdi (&vdi_params);
	
	vdi_params.intin = vdi_intin;
}

//==============================================================================
void
v_pline_p (int handle, int count, const PXY pxyarray[])
{
	vdi_params.ptsin = (short*)pxyarray;
	
	vdi_control[0] = 6;
	vdi_control[1] = count;
	vdi_control[3] = 0;
	vdi_control[6] = handle;
	vdi (&vdi_params);
	
	vdi_params.ptsin = vdi_ptsin;
}

//==============================================================================
void
v_pmarker_p (int handle, int count, const PXY pxyarray[])
{
	vdi_params.ptsin = (short*)pxyarray;
	
	vdi_control[0] = 7;
	vdi_control[1] = count;
	vdi_control[3] = 0;
	vdi_control[6] = handle;
	vdi (&vdi_params);
	
	vdi_params.ptsin = vdi_ptsin;
}



//==============================================================================
short
evnt_multi_s (const EVMULTI_IN * ev_i, short * msg, EVMULTI_OUT * ev_o)
{
	aes_params.intin  = (short*)ev_i;
	aes_addrin[0]     = (long)msg;
	aes_params.intout = (short*)ev_o;

	aes_control[0] = 25;
	aes_control[1] = 16;
	aes_control[2] = 7;
	aes_control[3] = 1;
	aes_control[4] = 0;
	aes(&aes_params);
	
	aes_params.intin  = aes_intin;
	aes_params.intout = aes_intout;
	
	return ev_o->evo_events;
}


//==============================================================================
void
graf_mkstate_p (PXY * mxy, short * btn, short * meta)
{
	aes_control[0] = 79;
	aes_control[1] = 0;
	aes_control[2] = 5;
	aes_control[3] = 0;
	aes_control[4] = 0;
	aes (&aes_params);
	
	*mxy  = *(PXY*)(aes_intout +1);
	*btn  = aes_intout[3];
	*meta = aes_intout[4];
}


//==============================================================================
short
wind_calc_r (int Type, int Parts, const GRECT * r_in, GRECT * r_out)
{
	aes_intin[0]            = Type;
	aes_intin[1]            = Parts;
	*(GRECT*)(aes_intin +2) = *r_in;
	
   aes_control[0] = 108;
   aes_control[1] = 6;
   aes_control[2] = 5;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes(&aes_params);
   
	*r_out = *(GRECT*)(aes_intout +1);
	
	return aes_intout[0];
}

//==============================================================================
short
wind_create_r (int Parts, const GRECT * curr)
{
	aes_intin[0]            = Parts;
	*(GRECT*)(aes_intin +1) = *curr;
	
   aes_control[0] = 100;
   aes_control[1] = 5;
   aes_control[2] = 1;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes(&aes_params);
   
	return aes_intout[0];
}

//==============================================================================
short
wind_get_r (int WindowHandle, int What, GRECT * rec)
{
	aes_intin[0] = WindowHandle;
	aes_intin[1] = What;
	if(What == WF_DCOLOR || What == WF_COLOR)
		aes_intin[2] = rec->x;
   
   aes_control[0] = 104;
   aes_control[1] = 2;
   aes_control[2] = 5;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes (&aes_params);
	
	*rec = *(GRECT*)(aes_intout +1);
	
	return aes_intout[0];
}

//==============================================================================
short
wind_get_one (int WindowHandle, int What)
{
	aes_intin[0] = WindowHandle;
	aes_intin[1] = What;
   
   aes_control[0] = 104;
   aes_control[1] = 2;
   aes_control[2] = 5;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes (&aes_params);
	
	return (aes_intout[0] ? aes_intout[1] : -1);
}

//==============================================================================
short
wind_open_r (int WindowHandle, const GRECT * curr)
{
	aes_intin[0]            = WindowHandle;
	*(GRECT*)(aes_intin +1) = *curr;
	
   aes_control[0] = 101;
   aes_control[1] = 5;
   aes_control[2] = 1;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes(&aes_params);
	
	return aes_intout[0];
}

//==============================================================================
short
wind_set_r (int WindowHandle, int What, GRECT * rec)
{
	aes_intin[0]            = WindowHandle;
	aes_intin[1]            = What;
	*(GRECT*)(aes_intin +2) = *rec;
   
   aes_control[0] = 105;
   aes_control[1] = 6;
   aes_control[2] = 1;
   aes_control[3] = 0;
   aes_control[4] = 0;
   aes (&aes_params);
	
	return aes_intout[0];
}