/*****************************************************************************
 *
 *  XVID MPEG4 Codec
 *  - Image functions - header files
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
 * $Id: image.h,v 1.10 2002-11-17 00:20:30 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "../portab.h"
#include "colorspace.h"
#include "../xvid.h"

#define EDGE_SIZE  32


typedef struct
{
	uint8_t *y;
	uint8_t *u;
	uint8_t *v;
}
IMAGE;

void init_image(uint32_t cpu_flags);

int32_t image_create(IMAGE * image,
					 uint32_t edged_width,
					 uint32_t edged_height);
void image_destroy(IMAGE * image,
				   uint32_t edged_width,
				   uint32_t edged_height);

void image_swap(IMAGE * image1,
				IMAGE * image2);
void image_copy(IMAGE * image1,
				IMAGE * image2,
				uint32_t edged_width,
				uint32_t height);
void image_setedges(IMAGE * image,
					uint32_t edged_width,
					uint32_t edged_height,
					uint32_t width,
					uint32_t height);
void image_interpolate(const IMAGE * refn,
					   IMAGE * refh,
					   IMAGE * refv,
					   IMAGE * refhv,
					   uint32_t edged_width,
					   uint32_t edged_height,
					   uint32_t rounding);

float image_psnr(IMAGE * orig_image,
				 IMAGE * recon_image,
				 uint16_t stride,
				 uint16_t width,
				 uint16_t height);


int image_input(IMAGE * image,
				uint32_t width,
				int height,
				uint32_t edged_width,
				uint8_t * src,
				int csp);

int image_output(IMAGE * image,
				 uint32_t width,
				 int height,
				 uint32_t edged_width,
				 uint8_t * dst,
				 uint32_t dst_stride,
				 int csp);



int image_dump_yuvpgm(const IMAGE * image,
					  const uint32_t edged_width,
					  const uint32_t width,
					  const uint32_t height,
					  char *filename);

float image_mad(const IMAGE * img1,
				const IMAGE * img2,
				uint32_t stride,
				uint32_t width,
				uint32_t height);

void
output_slice(IMAGE * cur, int edged_width, int width, XVID_DEC_PICTURE* out_frm, int mbx, int mby,int mbl);

#endif							/* _IMAGE_H_ */
