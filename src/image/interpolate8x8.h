/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8x8 block-based halfpel interpolation - headers
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
 *
 *  This file is part of XviD, a free MPEG-4 video encoder/decoder
 *
 *  XviD is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Under section 8 of the GNU General Public License, the copyright
 *  holders of XVID explicitly forbid distribution in the following
 *  countries:
 *
 *    - Japan
 *    - United States of America
 *
 *  Linking XviD statically or dynamically with other modules is making a
 *  combined work based on XviD.  Thus, the terms and conditions of the
 *  GNU General Public License cover the whole combination.
 *
 *  As a special exception, the copyright holders of XviD give you
 *  permission to link XviD with independent modules that communicate with
 *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
 *  license terms of these independent modules, and to copy and distribute
 *  the resulting combined work under terms of your choice, provided that
 *  every copy of the combined work is accompanied by a complete copy of
 *  the source code of XviD (the version of XviD used to produce the
 *  combined work), being distributed under the terms of the GNU General
 *  Public License plus this exception.  An independent module is a module
 *  which is not derived from or based on XviD.
 *
 *  Note that people who make modified versions of XviD are not obligated
 *  to grant this special exception for their modified versions; it is
 *  their choice whether to do so.  The GNU General Public License gives
 *  permission to release a modified version without this exception; this
 *  exception also makes it possible to release a modified version which
 *  carries forward this exception.
 *
 * $Id: interpolate8x8.h,v 1.8 2002-11-26 23:44:10 edgomez Exp $
 *
 ****************************************************************************/

#include "../utils/mem_transfer.h"

typedef void (INTERPOLATE8X8) (uint8_t * const dst,
							   const uint8_t * const src,
							   const uint32_t stride,
							   const uint32_t rounding);
typedef INTERPOLATE8X8 *INTERPOLATE8X8_PTR;

extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;

INTERPOLATE8X8 interpolate8x8_halfpel_h_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_c;

INTERPOLATE8X8 interpolate8x8_halfpel_h_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_v_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_mmx;

INTERPOLATE8X8 interpolate8x8_halfpel_h_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_v_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_xmm;

INTERPOLATE8X8 interpolate8x8_halfpel_h_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_v_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_3dn;

INTERPOLATE8X8 interpolate8x8_halfpel_h_ia64;
INTERPOLATE8X8 interpolate8x8_halfpel_v_ia64;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_ia64;

void interpolate8x8_lowpass_h(uint8_t *dst, uint8_t *src, int32_t dst_stride, int32_t src_stride, int32_t rounding);
void interpolate8x8_lowpass_v(uint8_t *dst, uint8_t *src, int32_t dst_stride, int32_t src_stride, int32_t rounding);
void interpolate8x8_lowpass_hv(uint8_t *dst1, uint8_t *dst2, uint8_t *src, int32_t dst1_stride, int32_t dst2_stride, int32_t src_stride, int32_t rounding);
void interpolate8x8_bilinear2(uint8_t *dst, uint8_t *src1, uint8_t *src2, int32_t dst_stride, int32_t src_stride, int32_t rounding);
void interpolate8x8_bilinear4(uint8_t *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int32_t stride, int32_t rounding);

void interpolate8x8_c(uint8_t * const dst,
					  const uint8_t * const src,
					  const uint32_t x,
					  const uint32_t y,
					  const uint32_t stride);

static __inline void
interpolate8x8_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{
	int32_t ddx, ddy;

	switch (((dx & 1) << 1) + (dy & 1))	/* ((dx%2)?2:0)+((dy%2)?1:0) */
	{
	case 0:
		ddx = dx / 2;
		ddy = dy / 2;
		transfer8x8_copy(cur + y * stride + x,
						 refn + (int)((y + ddy) * stride + x + ddx), stride);
		break;

	case 1:
		ddx = dx / 2;
		ddy = (dy - 1) / 2;
		interpolate8x8_halfpel_v(cur + y * stride + x,
								 refn + (int)((y + ddy) * stride + x + ddx), stride,
								 rounding);
		break;

	case 2:
		ddx = (dx - 1) / 2;
		ddy = dy / 2;
		interpolate8x8_halfpel_h(cur + y * stride + x,
								 refn + (int)((y + ddy) * stride + x + ddx), stride,
								 rounding);
		break;

	default:
		ddx = (dx - 1) / 2;
		ddy = (dy - 1) / 2;
		interpolate8x8_halfpel_hv(cur + y * stride + x,
								 refn + (int)((y + ddy) * stride + x + ddx), stride,
								  rounding);
		break;
	}
}


static __inline void interpolate8x8_quarterpel(uint8_t * const cur,
				     uint8_t * const refn,
				     const uint32_t x, const uint32_t y,
					 const int32_t dx,  const int dy,
					 const uint32_t stride,
					 const uint32_t rounding)
{
	const int32_t xRef = x*4 + dx;
	const int32_t yRef = y*4 + dy;

	uint8_t *src, *dst;
	int32_t x_int, y_int, x_frac, y_frac;

	uint8_t halfpel_h[72];
	uint8_t halfpel_v[64];
	uint8_t halfpel_hv[64];
	
	x_int = xRef/4;
	if (xRef < 0 && xRef % 4) 
		x_int--;

	x_frac = xRef - (4*x_int); 

	y_int  = yRef/4;
	if (yRef < 0 && yRef % 4)
		y_int--;

	y_frac = yRef - (4*y_int);
 
	src = refn + y_int * stride + x_int;
	dst = cur + y * stride + x;
	
	switch((y_frac << 2) | (x_frac)) {

	case 0:
		transfer8x8_copy(dst, src, stride);
		break;

	case 1:
		interpolate8x8_lowpass_h(halfpel_h, src, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, src, halfpel_h, stride, stride, rounding);
  		break;
  
	case 2:
	    interpolate8x8_lowpass_h(dst, src, stride, stride, rounding);
  		break;
  
	case 3:
		interpolate8x8_lowpass_h(halfpel_h, src, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, src+1, halfpel_h, stride, stride, rounding);
  		break;
  
	case 4:
		interpolate8x8_lowpass_v(halfpel_v, src, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, src, halfpel_v, stride, stride, rounding);
 		break;
   
	case 5:
		interpolate8x8_lowpass_v(halfpel_v, src, 8, stride, rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear4(dst, src, halfpel_h, halfpel_v, halfpel_hv, stride, rounding);
 		break;
   
	case 6:
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, halfpel_h, halfpel_hv, stride, 8, 1-rounding);
 		break;
   
	case 7:
		interpolate8x8_lowpass_v(halfpel_v, src+1, 8, stride, 16-rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear4(dst, src+1, halfpel_h, halfpel_v, halfpel_hv, stride, rounding);
 		break;
   
	case 8:
	    interpolate8x8_lowpass_v(dst, src, stride, stride, rounding);
		break;

	case 9:
		interpolate8x8_lowpass_v(halfpel_v, src, 8, stride, 16-rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, halfpel_v, halfpel_hv, stride, 8, rounding);
  		break;
  
	case 10:
		interpolate8x8_lowpass_hv(dst, halfpel_h, src, stride, 8, stride, rounding);
		break;
  
	case 11:
		interpolate8x8_lowpass_v(halfpel_v, src+1, 8, stride, 16-rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, halfpel_v, halfpel_hv, stride, 8, rounding);
   		break;
 
	case 12:
		interpolate8x8_lowpass_v(halfpel_v, src, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, src+stride, halfpel_v, stride, stride, rounding);
   		break;
 
	case 13:
		interpolate8x8_lowpass_v(halfpel_v, src, 8, stride, rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear4(dst, src+stride, halfpel_h+8, halfpel_v, halfpel_hv, stride, rounding);
 		break;
   
	case 14:
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear2(dst, halfpel_h+8, halfpel_hv, stride, 8, rounding);
 		break;
   
	case 15:
		interpolate8x8_lowpass_v(halfpel_v, src+1, 8, stride, rounding);
		interpolate8x8_lowpass_hv(halfpel_hv, halfpel_h, src, 8, 8, stride, rounding);
		interpolate8x8_bilinear4(dst, src+stride+1, halfpel_h+8, halfpel_v, halfpel_hv, stride, rounding);
		break;
	}
}
