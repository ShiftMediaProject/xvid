/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	image stuff
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *  05.10.2002	support for interpolated images in qpel mode - Isibaar
 *	01.05.2002	BFRAME image-based u,v interpolation
 *  22.04.2002  added some B-frame support
 *	14.04.2002	added image_dump_yuvpgm(), added image_mad()
 *              XVID_CSP_USER input support
 *  09.04.2002  PSNR calculations - Isibaar
 *	06.04.2002	removed interlaced edging from U,V blocks (as per spec)
 *  26.03.2002  interlacing support (field-based edging in set_edges)
 *	26.01.2002	rgb555, rgb565
 *	07.01.2001	commented u,v interpolation (not required for uv-block-based)
 *  23.12.2001  removed #ifdefs, added function pointers + init_common()
 *	22.12.2001	cpu #ifdefs
 *  19.12.2001  image_dump(); useful for debugging
 *	 6.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/

#include <stdlib.h>
#include <string.h>				// memcpy, memset
#include <math.h>

#include "../portab.h"
#include "../global.h"			// XVID_CSP_XXX's
#include "../xvid.h"			// XVID_CSP_XXX's
#include "image.h"
#include "colorspace.h"
#include "interpolate8x8.h"
#include "reduced.h"
#include "../divx4.h"
#include "../utils/mem_align.h"

#include "font.h"		// XXX: remove later

#define SAFETY	64
#define EDGE_SIZE2  (EDGE_SIZE/2)


int32_t
image_create(IMAGE * image,
			 uint32_t edged_width,
			 uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t edged_height2 = edged_height / 2;
	uint32_t i;

	image->y =
		xvid_malloc(edged_width * (edged_height + 1) + SAFETY, CACHE_LINE);
	if (image->y == NULL) {
		return -1;
	}

	for (i = 0; i < edged_width * edged_height + SAFETY; i++) {
		image->y[i] = 0;
	}

	image->u = xvid_malloc(edged_width2 * edged_height2 + SAFETY, CACHE_LINE);
	if (image->u == NULL) {
		xvid_free(image->y);
		return -1;
	}
	image->v = xvid_malloc(edged_width2 * edged_height2 + SAFETY, CACHE_LINE);
	if (image->v == NULL) {
		xvid_free(image->u);
		xvid_free(image->y);
		return -1;
	}

	image->y += EDGE_SIZE * edged_width + EDGE_SIZE;
	image->u += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;
	image->v += EDGE_SIZE2 * edged_width2 + EDGE_SIZE2;

	return 0;
}



void
image_destroy(IMAGE * image,
			  uint32_t edged_width,
			  uint32_t edged_height)
{
	const uint32_t edged_width2 = edged_width / 2;

	if (image->y) {
		xvid_free(image->y - (EDGE_SIZE * edged_width + EDGE_SIZE));
	}
	if (image->u) {
		xvid_free(image->u - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
	}
	if (image->v) {
		xvid_free(image->v - (EDGE_SIZE2 * edged_width2 + EDGE_SIZE2));
	}
}


void
image_swap(IMAGE * image1,
		   IMAGE * image2)
{
	uint8_t *tmp;

	tmp = image1->y;
	image1->y = image2->y;
	image2->y = tmp;

	tmp = image1->u;
	image1->u = image2->u;
	image2->u = tmp;

	tmp = image1->v;
	image1->v = image2->v;
	image2->v = tmp;
}


void
image_copy(IMAGE * image1,
		   IMAGE * image2,
		   uint32_t edged_width,
		   uint32_t height)
{
	memcpy(image1->y, image2->y, edged_width * height);
	memcpy(image1->u, image2->u, edged_width * height / 4);
	memcpy(image1->v, image2->v, edged_width * height / 4);
}


void
image_setedges(IMAGE * image,
			   uint32_t edged_width,
			   uint32_t edged_height,
			   uint32_t width,
			   uint32_t height)
{
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t width2 = width / 2;
	uint32_t i;
	uint8_t *dst;
	uint8_t *src;


	dst = image->y - (EDGE_SIZE + EDGE_SIZE * edged_width);
	src = image->y;

	for (i = 0; i < EDGE_SIZE; i++) {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, *(src + width - 1),
			   EDGE_SIZE);
		dst += edged_width;
	}

	for (i = 0; i < height; i++) {
		memset(dst, *src, EDGE_SIZE);
		memset(dst + edged_width - EDGE_SIZE, src[width - 1], EDGE_SIZE);
		dst += edged_width;
		src += edged_width;
	}

	src -= edged_width;
	for (i = 0; i < EDGE_SIZE; i++) {
		memset(dst, *src, EDGE_SIZE);
		memcpy(dst + EDGE_SIZE, src, width);
		memset(dst + edged_width - EDGE_SIZE, *(src + width - 1),
				   EDGE_SIZE);
		dst += edged_width;
	}


//U
	dst = image->u - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
	src = image->u;

	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}

	for (i = 0; i < height / 2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
	}
	src -= edged_width2;
	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}


// V
	dst = image->v - (EDGE_SIZE2 + EDGE_SIZE2 * edged_width2);
	src = image->v;

	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}

	for (i = 0; i < height / 2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memset(dst + edged_width2 - EDGE_SIZE2, src[width2 - 1], EDGE_SIZE2);
		dst += edged_width2;
		src += edged_width2;
	}
	src -= edged_width2;
	for (i = 0; i < EDGE_SIZE2; i++) {
		memset(dst, *src, EDGE_SIZE2);
		memcpy(dst + EDGE_SIZE2, src, width2);
		memset(dst + edged_width2 - EDGE_SIZE2, *(src + width2 - 1),
			   EDGE_SIZE2);
		dst += edged_width2;
	}
}

// bframe encoding requires image-based u,v interpolation
void
image_interpolate(const IMAGE * refn,
				  IMAGE * refh,
				  IMAGE * refv,
				  IMAGE * refhv,
				  uint32_t edged_width,
				  uint32_t edged_height,
				  uint32_t quarterpel,
				  uint32_t rounding)
{
	const uint32_t offset = EDGE_SIZE2 * (edged_width + 1); // we only interpolate half of the edge area
	const uint32_t stride_add = 7 * edged_width;
/*
#ifdef BFRAMES
	const uint32_t edged_width2 = edged_width / 2;
	const uint32_t edged_height2 = edged_height / 2;
	const uint32_t offset2 = EDGE_SIZE2 * (edged_width2 + 1);
	const uint32_t stride_add2 = 7 * edged_width2;
#endif
*/
	uint8_t *n_ptr, *h_ptr, *v_ptr, *hv_ptr;
	uint32_t x, y;


	n_ptr = refn->y;
	h_ptr = refh->y;
	v_ptr = refv->y;
	hv_ptr = refhv->y;

	n_ptr -= offset;
	h_ptr -= offset;
	v_ptr -= offset;
	hv_ptr -= offset;

	if(quarterpel) {
		
		for (y = 0; y < (edged_height - EDGE_SIZE); y += 8) {
			for (x = 0; x < (edged_width - EDGE_SIZE); x += 8) {
				interpolate8x8_6tap_lowpass_h(h_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_6tap_lowpass_v(v_ptr, n_ptr, edged_width, rounding);

				n_ptr += 8;
				h_ptr += 8;
				v_ptr += 8;
			}
			
			n_ptr += EDGE_SIZE;
			h_ptr += EDGE_SIZE;
			v_ptr += EDGE_SIZE;

			h_ptr += stride_add;
			v_ptr += stride_add;
			n_ptr += stride_add;
		}

		h_ptr = refh->y;
		h_ptr -= offset;

		for (y = 0; y < (edged_height - EDGE_SIZE); y = y + 8) {
			for (x = 0; x < (edged_width - EDGE_SIZE); x = x + 8) {
				interpolate8x8_6tap_lowpass_v(hv_ptr, h_ptr, edged_width, rounding);
				hv_ptr += 8;
				h_ptr += 8;
			}

			hv_ptr += EDGE_SIZE;
			h_ptr += EDGE_SIZE;

			hv_ptr += stride_add;
			h_ptr += stride_add;
		}
	}
	else {

		for (y = 0; y < (edged_height - EDGE_SIZE); y += 8) {
			for (x = 0; x < (edged_width - EDGE_SIZE); x += 8) {
				interpolate8x8_halfpel_h(h_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_halfpel_v(v_ptr, n_ptr, edged_width, rounding);
				interpolate8x8_halfpel_hv(hv_ptr, n_ptr, edged_width, rounding);

				n_ptr += 8;
				h_ptr += 8;
				v_ptr += 8;
				hv_ptr += 8;
			}
			
			h_ptr += EDGE_SIZE;
			v_ptr += EDGE_SIZE;
			hv_ptr += EDGE_SIZE;
			n_ptr += EDGE_SIZE;

			h_ptr += stride_add;
			v_ptr += stride_add;
			hv_ptr += stride_add;
			n_ptr += stride_add;
		}
	}
/*
#ifdef BFRAMES
	n_ptr = refn->u;
	h_ptr = refh->u;
	v_ptr = refv->u;
	hv_ptr = refhv->u;

	n_ptr -= offset2;
	h_ptr -= offset2;
	v_ptr -= offset2;
	hv_ptr -= offset2;

	for (y = 0; y < edged_height2; y += 8) {
		for (x = 0; x < edged_width2; x += 8) {
			interpolate8x8_halfpel_h(h_ptr, n_ptr, edged_width2, rounding);
			interpolate8x8_halfpel_v(v_ptr, n_ptr, edged_width2, rounding);
			interpolate8x8_halfpel_hv(hv_ptr, n_ptr, edged_width2, rounding);

			n_ptr += 8;
			h_ptr += 8;
			v_ptr += 8;
			hv_ptr += 8;
		}
		h_ptr += stride_add2;
		v_ptr += stride_add2;
		hv_ptr += stride_add2;
		n_ptr += stride_add2;
	}

	n_ptr = refn->v;
	h_ptr = refh->v;
	v_ptr = refv->v;
	hv_ptr = refhv->v;

	n_ptr -= offset2;
	h_ptr -= offset2;
	v_ptr -= offset2;
	hv_ptr -= offset2;

	for (y = 0; y < edged_height2; y = y + 8) {
		for (x = 0; x < edged_width2; x = x + 8) {
			interpolate8x8_halfpel_h(h_ptr, n_ptr, edged_width2, rounding);
			interpolate8x8_halfpel_v(v_ptr, n_ptr, edged_width2, rounding);
			interpolate8x8_halfpel_hv(hv_ptr, n_ptr, edged_width2, rounding);

			n_ptr += 8;
			h_ptr += 8;
			v_ptr += 8;
			hv_ptr += 8;
		}
		h_ptr += stride_add2;
		v_ptr += stride_add2;
		hv_ptr += stride_add2;
		n_ptr += stride_add2;
	}
#endif
*/
	/*
	   interpolate_halfpel_h(
	   refh->y - offset,
	   refn->y - offset, 
	   edged_width, edged_height,
	   rounding);

	   interpolate_halfpel_v(
	   refv->y - offset,
	   refn->y - offset, 
	   edged_width, edged_height,
	   rounding);

	   interpolate_halfpel_hv(
	   refhv->y - offset,
	   refn->y - offset,
	   edged_width, edged_height,
	   rounding);
	 */

	/* uv-image-based compensation
	   offset = EDGE_SIZE2 * (edged_width / 2 + 1);

	   interpolate_halfpel_h(
	   refh->u - offset,
	   refn->u - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);

	   interpolate_halfpel_v(
	   refv->u - offset,
	   refn->u - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);

	   interpolate_halfpel_hv(
	   refhv->u - offset,
	   refn->u - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);


	   interpolate_halfpel_h(
	   refh->v - offset,
	   refn->v - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);

	   interpolate_halfpel_v(
	   refv->v - offset,
	   refn->v - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);

	   interpolate_halfpel_hv(
	   refhv->v - offset,
	   refn->v - offset, 
	   edged_width / 2, edged_height / 2,
	   rounding);
	 */
}


/*
chroma optimize filter, invented by mf
a chroma pixel is average from the surrounding pixels, when the
correpsonding luma pixels are pure black or white.
*/

void
image_chroma_optimize(IMAGE * img, int width, int height, int edged_width)
{
	int x,y;
	int pixels = 0;

	for (y = 1; y < height/2 - 1; y++)
	for (x = 1; x < width/2 - 1; x++)
	{
#define IS_PURE(a)  ((a)<=16||(a)>=235)
#define IMG_Y(Y,X)	img->y[(Y)*edged_width + (X)]
#define IMG_U(Y,X)	img->u[(Y)*edged_width/2 + (X)]
#define IMG_V(Y,X)	img->v[(Y)*edged_width/2 + (X)]

		if (IS_PURE(IMG_Y(y*2  ,x*2  )) && 
			IS_PURE(IMG_Y(y*2  ,x*2+1)) &&
			IS_PURE(IMG_Y(y*2+1,x*2  )) && 
			IS_PURE(IMG_Y(y*2+1,x*2+1)))
		{
			IMG_U(y,x) = (IMG_U(y,x-1) + IMG_U(y-1, x) + IMG_U(y, x+1) + IMG_U(y+1, x)) / 4;
			IMG_V(y,x) = (IMG_V(y,x-1) + IMG_V(y-1, x) + IMG_V(y, x+1) + IMG_V(y+1, x)) / 4;
			pixels++;
		}

#undef IS_PURE
#undef IMG_Y
#undef IMG_U
#undef IMG_V
	}
	
	DPRINTF(DPRINTF_DEBUG,"chroma_optimized_pixels = %i/%i", pixels, width*height/4);
}





/*
  perform safe packed colorspace conversion, by splitting
  the image up into an optimized area (pixel width divisible by 16),
  and two unoptimized/plain-c areas (pixel width divisible by 2)
*/

static void	
safe_packed_conv(uint8_t * x_ptr, int x_stride,
				 uint8_t * y_ptr, uint8_t * u_ptr, uint8_t * v_ptr,
				 int y_stride, int uv_stride,
				 int width, int height, int vflip,
				 packedFunc * func_opt, packedFunc func_c, int size)
{
	int width_opt, width_c;

	if (func_opt != func_c && x_stride < size*((width+15)/16)*16)
	{
		width_opt = width & (~15);
		width_c = width - width_opt;
	}
	else
	{
		width_opt = width;
		width_c = 0;
	}

	func_opt(x_ptr, x_stride,
			y_ptr, u_ptr, v_ptr, y_stride, uv_stride,
			width_opt, height, vflip);

	if (width_c)
	{
		func_c(x_ptr + size*width_opt, x_stride,
			y_ptr + width_opt, u_ptr + width_opt/2, v_ptr + width_opt/2,
			y_stride, uv_stride, width_c, height, vflip);
	}
}



int
image_input(IMAGE * image,
			uint32_t width,
			int height,
			uint32_t edged_width,
			uint8_t * src,
			int src_stride,
			int csp,
			int interlacing)
{
	const int edged_width2 = edged_width/2;
	const int width2 = width/2;
	const int height2 = height/2;
	//const int height_signed = (csp & XVID_CSP_VFLIP) ? -height : height;

	
	//	int src_stride = width;

	// --- xvid 2.1 compatiblity patch ---
	// --- remove when xvid_dec_frame->stride equals real stride
	/*
	if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB555 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB565 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_YUY2 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_YVYU ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_UYVY)
	{
		src_stride *= 2;
	} 
	else if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB24)
	{
		src_stride *= 3;
	}
	else if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB32 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_ABGR ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGBA)
	{
		src_stride *= 4;
	}
	*/
	// ^--- xvid 2.1 compatiblity fix ---^

	switch (csp & ~XVID_CSP_VFLIP) {
	case XVID_CSP_RGB555:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgb555i_to_yv12  :rgb555_to_yv12,
			interlacing?rgb555i_to_yv12_c:rgb555_to_yv12_c, 2);
		break;

	case XVID_CSP_RGB565:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgb565i_to_yv12  :rgb565_to_yv12,
			interlacing?rgb565i_to_yv12_c:rgb565_to_yv12_c, 2);
		break;


	case XVID_CSP_RGB24:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?bgri_to_yv12  :bgr_to_yv12,
			interlacing?bgri_to_yv12_c:bgr_to_yv12_c, 3);
		break;

	case XVID_CSP_RGB32:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?bgrai_to_yv12  :bgra_to_yv12,
			interlacing?bgrai_to_yv12_c:bgra_to_yv12_c, 4);
		break;

	case XVID_CSP_ABGR :
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?abgri_to_yv12  :abgr_to_yv12,
			interlacing?abgri_to_yv12_c:abgr_to_yv12_c, 4);
		break;

	case XVID_CSP_RGBA :
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?rgbai_to_yv12  :rgba_to_yv12,
			interlacing?rgbai_to_yv12_c:rgba_to_yv12_c, 4);
		break;

	case XVID_CSP_YUY2:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
			interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2);
		break;

	case XVID_CSP_YVYU:		/* u/v swapped */
		safe_packed_conv(
			src, src_stride, image->y, image->v, image->y, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yuyvi_to_yv12  :yuyv_to_yv12,
			interlacing?yuyvi_to_yv12_c:yuyv_to_yv12_c, 2);
		break;

	case XVID_CSP_UYVY:
		safe_packed_conv(
			src, src_stride, image->y, image->u, image->v, 
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?uyvyi_to_yv12  :uyvy_to_yv12,
			interlacing?uyvyi_to_yv12_c:uyvy_to_yv12_c, 2);
		break;

	case XVID_CSP_I420:
		yv12_to_yv12(image->y, image->u, image->v, edged_width, edged_width2,
			src, src + src_stride*height, src + src_stride*height + (src_stride/2)*height2,
			src_stride, src_stride/2, width, height, (csp & XVID_CSP_VFLIP));
		break
			;
	case XVID_CSP_YV12:		/* u/v swapped */
		yv12_to_yv12(image->y, image->v, image->u, edged_width, edged_width2,
			src, src + src_stride*height, src + src_stride*height + (src_stride/2)*height2,
			src_stride, src_stride/2, width, height, (csp & XVID_CSP_VFLIP));
		break;

	case XVID_CSP_USER:
		{
			DEC_PICTURE * pic = (DEC_PICTURE*)src;
			yv12_to_yv12(image->y, image->u, image->v, edged_width, edged_width2,
				pic->y, pic->u, pic->v, pic->stride_y, pic->stride_y,
				width, height, (csp & XVID_CSP_VFLIP));
		}
		break;

	case XVID_CSP_NULL:
		break;

	default :
		return -1;
	}


	/* pad out image when the width and/or height is not a multiple of 16 */

	if (width & 15)
	{
		int i;
		int pad_width = 16 - (width&15);
		for (i = 0; i < height; i++)
		{
			memset(image->y + i*edged_width + width, 
				 *(image->y + i*edged_width + width - 1), pad_width);
		}
		for (i = 0; i < height/2; i++)
		{
			memset(image->u + i*edged_width2 + width2, 
				 *(image->u + i*edged_width2 + width2 - 1),pad_width/2);
			memset(image->v + i*edged_width2 + width2, 
				 *(image->v + i*edged_width2 + width2 - 1),pad_width/2);
		}
	}

	if (height & 15)
	{
		int pad_height = 16 - (height&15); 
		int length = ((width+15)/16)*16;
		int i;
		for (i = 0; i < pad_height; i++)
		{
			memcpy(image->y + (height+i)*edged_width,
				   image->y + (height-1)*edged_width,length);
		}

		for (i = 0; i < pad_height/2; i++)
		{
			memcpy(image->u + (height2+i)*edged_width2,
				   image->u + (height2-1)*edged_width2,length/2);
			memcpy(image->v + (height2+i)*edged_width2,
				   image->v + (height2-1)*edged_width2,length/2);
		}
	}

/*
	if (interlacing)
		image_printf(image, edged_width, height, 5,5, "[i]");
	image_dump_yuvpgm(image, edged_width, ((width+15)/16)*16, ((height+15)/16)*16, "\\encode.pgm");
*/
	return 0;
}



int
image_output(IMAGE * image,
			 uint32_t width,
			 int height,
			 uint32_t edged_width,
			 uint8_t * dst,
			 uint32_t dst_stride,
			 int csp,
			 int interlacing)
{
	const int edged_width2 = edged_width/2;
	int height2 = height/2;

/*
	if (interlacing)
		image_printf(image, edged_width, height, 5,100, "[i]=%i,%i",width,height);
	image_dump_yuvpgm(image, edged_width, width, height, "\\decode.pgm");
*/


	// --- xvid 2.1 compatiblity patch ---
	// --- remove when xvid_dec_frame->stride equals real stride
	/*
	if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB555 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB565 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_YUY2 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_YVYU ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_UYVY)
	{
		dst_stride *= 2;
	} 
	else if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB24)
	{
		dst_stride *= 3;
	}
	else if ((csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGB32 ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_ABGR ||
		(csp & ~XVID_CSP_VFLIP) == XVID_CSP_RGBA)
	{
		dst_stride *= 4;
	}
	*/
	// ^--- xvid 2.1 compatiblity fix ---^

	
	switch (csp & ~XVID_CSP_VFLIP) {
	case XVID_CSP_RGB555:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgb555i  :yv12_to_rgb555,
			interlacing?yv12_to_rgb555i_c:yv12_to_rgb555_c, 2);
		return 0;

	case XVID_CSP_RGB565:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgb565i  :yv12_to_rgb565,
			interlacing?yv12_to_rgb565i_c:yv12_to_rgb565_c, 2);
		return 0;

	case XVID_CSP_RGB24:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_bgri  :yv12_to_bgr,
			interlacing?yv12_to_bgri_c:yv12_to_bgr_c, 3);
		return 0;

	case XVID_CSP_RGB32:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_bgrai  :yv12_to_bgra,
			interlacing?yv12_to_bgrai_c:yv12_to_bgra_c, 4);
		return 0;

	case XVID_CSP_ABGR:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_abgri  :yv12_to_abgr,
			interlacing?yv12_to_abgri_c:yv12_to_abgr_c, 4);
		return 0;

	case XVID_CSP_RGBA:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_rgbai  :yv12_to_rgba,
			interlacing?yv12_to_rgbai_c:yv12_to_rgba_c, 4);
		return 0;

	case XVID_CSP_YUY2:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2);
		return 0;

	case XVID_CSP_YVYU:		// u,v swapped
		safe_packed_conv(
			dst, dst_stride, image->y, image->v, image->u,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_yuyvi  :yv12_to_yuyv,
			interlacing?yv12_to_yuyvi_c:yv12_to_yuyv_c, 2);
		return 0;

	case XVID_CSP_UYVY:
		safe_packed_conv(
			dst, dst_stride, image->y, image->u, image->v,
			edged_width, edged_width2, width, height, (csp & XVID_CSP_VFLIP),
			interlacing?yv12_to_uyvyi  :yv12_to_uyvy,
			interlacing?yv12_to_uyvyi_c:yv12_to_uyvy_c, 2);
		return 0;

	case XVID_CSP_I420:
		yv12_to_yv12(dst, dst + dst_stride*height, dst + dst_stride*height + (dst_stride/2)*height2,
			dst_stride, dst_stride/2,
			image->y, image->u, image->v, edged_width, edged_width2,
			width, height, (csp & XVID_CSP_VFLIP));
		return 0;

	case XVID_CSP_YV12:		// u,v swapped
		yv12_to_yv12(dst, dst + dst_stride*height, dst + dst_stride*height + (dst_stride/2)*height2,
			dst_stride, dst_stride/2,
			image->y, image->v, image->u, edged_width, edged_width2,
			width, height, (csp & XVID_CSP_VFLIP));
		return 0;

	case XVID_CSP_USER:
		{
			DEC_PICTURE * pic = (DEC_PICTURE*)dst;
			pic->y = image->y;
			pic->u = image->u;
			pic->v = image->v;
			pic->stride_y = edged_width;
			pic->stride_uv = edged_width / 2;
		}
		return 0;

	case XVID_CSP_NULL:
	case XVID_CSP_EXTERN:
		return 0;

	}

	return -1;
}

float
image_psnr(IMAGE * orig_image,
		   IMAGE * recon_image,
		   uint16_t stride,
		   uint16_t width,
		   uint16_t height)
{
	int32_t diff, x, y, quad = 0;
	uint8_t *orig = orig_image->y;
	uint8_t *recon = recon_image->y;
	float psnr_y;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			diff = *(orig + x) - *(recon + x);
			quad += diff * diff;
		}
		orig += stride;
		recon += stride;
	}

	psnr_y = (float) quad / (float) (width * height);

	if (psnr_y) {
		psnr_y = (float) (255 * 255) / psnr_y;
		psnr_y = 10 * (float) log10(psnr_y);
	} else
		psnr_y = (float) 99.99;

	return psnr_y;
}


float sse_to_PSNR(long sse, int pixels)
{
        if (sse==0)
                return 99.99F;

        return 48.131F - 10*(float)log10((float)sse/(float)(pixels));   // log10(255*255)=4.8131

}

long plane_sse(uint8_t * orig,
		   uint8_t * recon,
		   uint16_t stride,
		   uint16_t width,
		   uint16_t height)
{
	int diff, x, y;
	long sse=0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			diff = *(orig + x) - *(recon + x);
			sse += diff * diff;
		}
		orig += stride;
		recon += stride;
	}
	return sse;
}

/*

#include <stdio.h>
#include <string.h>

int image_dump_pgm(uint8_t * bmp, uint32_t width, uint32_t height, char * filename)
{
	FILE * f;
	char hdr[1024];
	
	f = fopen(filename, "wb");
	if ( f == NULL)
	{
		return -1;
	}
	sprintf(hdr, "P5\n#xvid\n%i %i\n255\n", width, height);
	fwrite(hdr, strlen(hdr), 1, f);
	fwrite(bmp, width, height, f);
	fclose(f);

	return 0;
}


// dump image+edges to yuv pgm files 

int image_dump(IMAGE * image, uint32_t edged_width, uint32_t edged_height, char * path, int number)
{
	char filename[1024];

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'y');
	image_dump_pgm(
		image->y - (EDGE_SIZE * edged_width + EDGE_SIZE),
		edged_width, edged_height, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'u');
	image_dump_pgm(
		image->u - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	sprintf(filename, "%s_%i_%c.pgm", path, number, 'v');
	image_dump_pgm(
		image->v - (EDGE_SIZE2 * edged_width / 2 + EDGE_SIZE2),
		edged_width / 2, edged_height / 2, filename);

	return 0;
}
*/



/* dump image to yuvpgm file */

#include <stdio.h>

int
image_dump_yuvpgm(const IMAGE * image,
				  const uint32_t edged_width,
				  const uint32_t width,
				  const uint32_t height,
				  char *filename)
{
	FILE *f;
	char hdr[1024];
	uint32_t i;
	uint8_t *bmp1;
	uint8_t *bmp2;


	f = fopen(filename, "wb");
	if (f == NULL) {
		return -1;
	}
	sprintf(hdr, "P5\n#xvid\n%i %i\n255\n", width, (3 * height) / 2);
	fwrite(hdr, strlen(hdr), 1, f);

	bmp1 = image->y;
	for (i = 0; i < height; i++) {
		fwrite(bmp1, width, 1, f);
		bmp1 += edged_width;
	}

	bmp1 = image->u;
	bmp2 = image->v;
	for (i = 0; i < height / 2; i++) {
		fwrite(bmp1, width / 2, 1, f);
		fwrite(bmp2, width / 2, 1, f);
		bmp1 += edged_width / 2;
		bmp2 += edged_width / 2;
	}

	fclose(f);
	return 0;
}


float
image_mad(const IMAGE * img1,
		  const IMAGE * img2,
		  uint32_t stride,
		  uint32_t width,
		  uint32_t height)
{
	const uint32_t stride2 = stride / 2;
	const uint32_t width2 = width / 2;
	const uint32_t height2 = height / 2;

	uint32_t x, y;
	uint32_t sum = 0;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
			sum += ABS(img1->y[x + y * stride] - img2->y[x + y * stride]);

	for (y = 0; y < height2; y++)
		for (x = 0; x < width2; x++)
			sum += ABS(img1->u[x + y * stride2] - img2->u[x + y * stride2]);

	for (y = 0; y < height2; y++)
		for (x = 0; x < width2; x++)
			sum += ABS(img1->v[x + y * stride2] - img2->v[x + y * stride2]);

	return (float) sum / (width * height * 3 / 2);
}

void
output_slice(IMAGE * cur, int std, int width, XVID_DEC_PICTURE* out_frm, int mbx, int mby,int mbl) {
  uint8_t *dY,*dU,*dV,*sY,*sU,*sV;
  int std2 = std >> 1;
  int w = mbl << 4, w2,i;

  if(w > width)
    w = width;
  w2 = w >> 1;

  dY = (uint8_t*)out_frm->y + (mby << 4) * out_frm->stride_y + (mbx << 4);
  dU = (uint8_t*)out_frm->u + (mby << 3) * out_frm->stride_u + (mbx << 3);
  dV = (uint8_t*)out_frm->v + (mby << 3) * out_frm->stride_v + (mbx << 3);
  sY = cur->y + (mby << 4) * std + (mbx << 4);
  sU = cur->u + (mby << 3) * std2 + (mbx << 3);
  sV = cur->v + (mby << 3) * std2 + (mbx << 3);

  for(i = 0 ; i < 16 ; i++) {
    memcpy(dY,sY,w);
    dY += out_frm->stride_y;
    sY += std;
  }
  for(i = 0 ; i < 8 ; i++) {
    memcpy(dU,sU,w2);
    dU += out_frm->stride_u;
    sU += std2;
  }
  for(i = 0 ; i < 8 ; i++) {
    memcpy(dV,sV,w2);
    dV += out_frm->stride_v;
    sV += std2;
  }
}


void
image_clear(IMAGE * img, int width, int height, int edged_width,
					int y, int u, int v)
{
	uint8_t * p;
	int i;

	p = img->y;
	for (i = 0; i < height; i++) {
		memset(p, y, width);
		p += edged_width;
	}

	p = img->u;
	for (i = 0; i < height/2; i++) {
		memset(p, u, width/2);
		p += edged_width/2;
	}

	p = img->v;
	for (i = 0; i < height/2; i++) {
		memset(p, v, width/2);
		p += edged_width/2;
	}
}


/* reduced resolution deblocking filter 
	block = block size (16=rrv, 8=full resolution)
	flags = XVID_DEC_YDEBLOCK|XVID_DEC_UVDEBLOCK
*/
void
image_deblock_rrv(IMAGE * img, int edged_width,
				const MACROBLOCK * mbs, int mb_width, int mb_height, int mb_stride,
				int block, int flags)
{
	const int edged_width2 = edged_width /2;
	const int nblocks = block / 8;	/* skals code uses 8pixel block uints */
	int i,j;

	/* luma: j,i in block units */
	if ((flags & XVID_DEC_DEBLOCKY))
	{
		for (j = 1; j < mb_height*2; j++)		/* horizontal deblocking */
		for (i = 0; i < mb_width*2; i++)
		{
			if (mbs[(j-1)/2*mb_stride + (i/2)].mode != MODE_NOT_CODED ||
				mbs[(j+0)/2*mb_stride + (i/2)].mode != MODE_NOT_CODED)
			{
				hfilter_31(img->y + (j*block - 1)*edged_width + i*block,
								  img->y + (j*block + 0)*edged_width + i*block, nblocks);
			}
		}

		for (j = 0; j < mb_height*2; j++)		/* vertical deblocking */
		for (i = 1; i < mb_width*2; i++)
		{
			if (mbs[(j/2)*mb_stride + (i-1)/2].mode != MODE_NOT_CODED ||
				mbs[(j/2)*mb_stride + (i+0)/2].mode != MODE_NOT_CODED)
			{
				vfilter_31(img->y + (j*block)*edged_width + i*block - 1,
						   img->y + (j*block)*edged_width + i*block + 0,
						   edged_width, nblocks);
			}
		}
	}


	/* chroma */
	if ((flags & XVID_DEC_DEBLOCKUV))
	{
		for (j = 1; j < mb_height; j++)		/* horizontal deblocking */
		for (i = 0; i < mb_width; i++)
		{
			if (mbs[(j-1)*mb_stride + i].mode != MODE_NOT_CODED || 
				mbs[(j+0)*mb_stride + i].mode != MODE_NOT_CODED)
			{
				hfilter_31(img->u + (j*block - 1)*edged_width2 + i*block,
						   img->u + (j*block + 0)*edged_width2 + i*block, nblocks);
				hfilter_31(img->v + (j*block - 1)*edged_width2 + i*block,
						   img->v + (j*block + 0)*edged_width2 + i*block, nblocks);
			}
		}

		for (j = 0; j < mb_height; j++)		/* vertical deblocking */	
		for (i = 1; i < mb_width; i++)
		{
			if (mbs[j*mb_stride + i - 1].mode != MODE_NOT_CODED ||
				mbs[j*mb_stride + i + 0].mode != MODE_NOT_CODED) 
			{
				vfilter_31(img->u + (j*block)*edged_width2 + i*block - 1,
						   img->u + (j*block)*edged_width2 + i*block + 0,
						   edged_width2, nblocks);
				vfilter_31(img->v + (j*block)*edged_width2 + i*block - 1,
						   img->v + (j*block)*edged_width2 + i*block + 0,
						   edged_width2, nblocks);
			}
		}
	}

}

