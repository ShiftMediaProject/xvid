/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8x8 block-based halfpel interpolation -
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
 *  Copyright(C) 2002 MinChen <chenm002@163.com>
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
 * $Id: interpolate8x8.c,v 1.9 2003-02-11 21:56:31 edgomez Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "interpolate8x8.h"

/* function pointers */
INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;


/* dst = interpolate(src) */

void
interpolate8x8_halfpel_h_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {

			int32_t tot =
				(int32_t) src[j * stride + i] + (int32_t) src[j * stride + i + 1];

			tot = (tot + 1 - rounding) >> 1;
			dst[j * stride + i] = (uint8_t) tot;
		}
	}
}



void
interpolate8x8_halfpel_v_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int32_t tot = 
				(int32_t)src[j * stride + i] + (int32_t)src[j * stride + i + stride];

			tot = ((tot + 1 - rounding) >> 1);
			dst[j * stride + i] = (uint8_t) tot;
		}
	}
}


void
interpolate8x8_halfpel_hv_c(uint8_t * const dst,
							const uint8_t * const src,
							const uint32_t stride,
							const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int32_t tot =
				(int32_t)src[j * stride + i] + (int32_t)src[j * stride + i + 1] +
				(int32_t)src[j * stride + i + stride] + (int32_t)src[j * stride + i + stride + 1];
			tot = ((tot + 2 - rounding) >> 2);
			dst[j * stride + i] = (uint8_t) tot;
		}
	}
}

/* add by MinChen <chenm001@163.com> */
/* interpolate8x8 two pred block */
void
interpolate8x8_c(uint8_t * const dst,
				 const uint8_t * const src,
				 const uint32_t x,
				 const uint32_t y,
				 const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int32_t tot =
				(((int32_t)src[(y + j) * stride + x + i] +
				  (int32_t)dst[(y + j) * stride + x + i] + 1) >> 1);
			dst[(y + j) * stride + x + i] = (uint8_t) tot;
		}
	}
}
