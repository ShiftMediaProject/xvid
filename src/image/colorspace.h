/*****************************************************************************
 *
 *  XVID MPEG4 Codec
 *  - Colorspace functions - header files
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
 * $Id: colorspace.h,v 1.4 2002-11-17 00:20:30 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _COLORSPACE_H

#define _COLORSPACE_H

#include "../portab.h"
#include "../divx4.h"

/* initialize tables */

void colorspace_init(void);


/* input color conversion functions (encoder) */

typedef void (color_inputFunc) (uint8_t * y_out,
								uint8_t * u_out,
								uint8_t * v_out,
								uint8_t * src,
								int width,
								int height,
								int stride);

typedef color_inputFunc *color_inputFuncPtr;

extern color_inputFuncPtr rgb555_to_yv12;
extern color_inputFuncPtr rgb565_to_yv12;
extern color_inputFuncPtr rgb24_to_yv12;
extern color_inputFuncPtr rgb32_to_yv12;
extern color_inputFuncPtr yuv_to_yv12;
extern color_inputFuncPtr yuyv_to_yv12;
extern color_inputFuncPtr uyvy_to_yv12;

/* plain c */
color_inputFunc rgb555_to_yv12_c;
color_inputFunc rgb565_to_yv12_c;
color_inputFunc rgb24_to_yv12_c;
color_inputFunc rgb32_to_yv12_c;
color_inputFunc yuv_to_yv12_c;
color_inputFunc yuyv_to_yv12_c;
color_inputFunc uyvy_to_yv12_c;

/* mmx */
color_inputFunc rgb24_to_yv12_mmx;
color_inputFunc rgb32_to_yv12_mmx;
color_inputFunc yuv_to_yv12_mmx;
color_inputFunc yuyv_to_yv12_mmx;
color_inputFunc uyvy_to_yv12_mmx;

/* xmm */
color_inputFunc yuv_to_yv12_xmm;


/* output color conversion functions (decoder) */

typedef void (color_outputFunc) (uint8_t * dst,
								 int dst_stride,
								 uint8_t * y_src,
								 uint8_t * v_src,
								 uint8_t * u_src,
								 int y_stride,
								 int uv_stride,
								 int width,
								 int height);

typedef color_outputFunc *color_outputFuncPtr;

extern color_outputFuncPtr yv12_to_rgb555;
extern color_outputFuncPtr yv12_to_rgb565;
extern color_outputFuncPtr yv12_to_rgb24;
extern color_outputFuncPtr yv12_to_rgb32;
extern color_outputFuncPtr yv12_to_yuv;
extern color_outputFuncPtr yv12_to_yuyv;
extern color_outputFuncPtr yv12_to_uyvy;

/* plain c */
void init_yuv_to_rgb(void);

color_outputFunc yv12_to_rgb555_c;
color_outputFunc yv12_to_rgb565_c;
color_outputFunc yv12_to_rgb24_c;
color_outputFunc yv12_to_rgb32_c;
color_outputFunc yv12_to_yuv_c;
color_outputFunc yv12_to_yuyv_c;
color_outputFunc yv12_to_uyvy_c;

/* mmx */
color_outputFunc yv12_to_rgb24_mmx;
color_outputFunc yv12_to_rgb32_mmx;
color_outputFunc yv12_to_yuyv_mmx;
color_outputFunc yv12_to_uyvy_mmx;


void user_to_yuv_c(uint8_t * y_out,
				   uint8_t * u_out,
				   uint8_t * v_out,
				   int stride,
				   DEC_PICTURE * picture,
				   int width,
				   int height);

#endif							/* _COLORSPACE_H_ */
