/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8x8 block-based halfpel interpolation
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
 *  Copyright(C) 2002 MinChen <chenm002@163.com>
 *
 *  This program is an implementation of a part of one or more MPEG-4
 *  Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *  to use this software module in hardware or software products are
 *  advised that its use may infringe existing patents or copyrights, and
 *  any such use would be at such party's own risk.  The original
 *  developer of this software module and his/her company, and subsequent
 *  editors and their companies, will have no liability for use of this
 *  software or modifications or derivatives thereof.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
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
 ****************************************************************************/

#include "../portab.h"
#include "interpolate8x8.h"

// function pointers
INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;


// dst = interpolate(src)

void
interpolate8x8_halfpel_h_c(uint8_t * const dst,
						   const uint8_t * const src,
						   const uint32_t stride,
						   const uint32_t rounding)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {

			int16_t tot =
				(int32_t) src[j * stride + i] + (int32_t) src[j * stride + i +
															  1];

			tot = (int32_t) ((tot + 1 - rounding) >> 1);
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
			int16_t tot = src[j * stride + i] + src[j * stride + i + stride];

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
			int16_t tot =
				src[j * stride + i] + src[j * stride + i + 1] +
				src[j * stride + i + stride] + src[j * stride + i + stride +
												   1];
			tot = ((tot + 2 - rounding) >> 2);
			dst[j * stride + i] = (uint8_t) tot;
		}
	}
}

// add by MinChen <chenm001@163.com>
// interpolate8x8 two pred block
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
				((src[(y + j) * stride + x + i] +
				  dst[(y + j) * stride + x + i] + 1) >> 1);
			dst[(y + j) * stride + x + i] = (uint8_t) tot;
		}
	}
}

/*************************************************************
 * QPEL STUFF STARTS HERE                                    *
 *************************************************************/

#define CLIP(X,A,B) (X < A) ? (A) : ((X > B) ? (B) : (X))

void interpolate8x8_lowpass_h(uint8_t *dst, uint8_t *src, int32_t dst_stride, int32_t src_stride, int32_t rounding)
{
    int32_t i;

    for(i = 0; i < 8; i++)
    {
        dst[0] = CLIP((((src[0] + src[1]) * 160 - (src[0] + src[2]) * 48 + (src[1] + src[3]) * 24 - (src[2] + src[4]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[1] = CLIP((((src[1] + src[2]) * 160 - (src[0] + src[3]) * 48 + (src[0] + src[4]) * 24 - (src[1] + src[5]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[2] = CLIP((((src[2] + src[3]) * 160 - (src[1] + src[4]) * 48 + (src[0] + src[5]) * 24 - (src[0] + src[6]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[3] = CLIP((((src[3] + src[4]) * 160 - (src[2] + src[5]) * 48 + (src[1] + src[6]) * 24 - (src[0] + src[7]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[4] = CLIP((((src[4] + src[5]) * 160 - (src[3] + src[6]) * 48 + (src[2] + src[7]) * 24 - (src[1] + src[8]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[5] = CLIP((((src[5] + src[6]) * 160 - (src[4] + src[7]) * 48 + (src[3] + src[8]) * 24 - (src[2] + src[8]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[6] = CLIP((((src[6] + src[7]) * 160 - (src[5] + src[8]) * 48 + (src[4] + src[8]) * 24 - (src[3] + src[7]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[7] = CLIP((((src[7] + src[8]) * 160 - (src[6] + src[8]) * 48 + (src[5] + src[7]) * 24 - (src[4] + src[6]) * 8 + (128 - rounding)) / 256), 0, 255);

        dst += dst_stride;
        src += src_stride;
    }
}

void interpolate8x8_lowpass_v(uint8_t *dst, uint8_t *src, int32_t dst_stride, int32_t src_stride, int32_t rounding)
{
    int32_t i;

    for(i = 0; i < 8; i++)
    {
        int32_t src0 = src[0];
        int32_t src1 = src[src_stride];
        int32_t src2 = src[2 * src_stride];
        int32_t src3 = src[3 * src_stride];
        int32_t src4 = src[4 * src_stride];
        int32_t src5 = src[5 * src_stride];
        int32_t src6 = src[6 * src_stride];
        int32_t src7 = src[7 * src_stride];
        int32_t src8 = src[8 * src_stride];
        
		dst[0] = CLIP((((src0 + src1) * 160 - (src0 + src2) * 48 + (src1 + src3) * 24 - (src2 + src4) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[dst_stride] = CLIP((((src1 + src2) * 160 - (src0 + src3) * 48 + (src0 + src4) * 24 - (src1 + src5) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[2 * dst_stride] = CLIP((((src2 + src3) * 160 - (src1 + src4) * 48 + (src0 + src5) * 24 - (src0 + src6) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[3 * dst_stride] = CLIP((((src3 + src4) * 160 - (src2 + src5) * 48 + (src1 + src6) * 24 - (src0 + src7) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[4 * dst_stride] = CLIP((((src4 + src5) * 160 - (src3 + src6) * 48 + (src2 + src7) * 24 - (src1 + src8) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[5 * dst_stride] = CLIP((((src5 + src6) * 160 - (src4 + src7) * 48 + (src3 + src8) * 24 - (src2 + src8) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[6 * dst_stride] = CLIP((((src6 + src7) * 160 - (src5 + src8) * 48 + (src4 + src8) * 24 - (src3 + src7) * 8 + (128 - rounding)) / 256), 0, 255);
        dst[7 * dst_stride] = CLIP((((src7 + src8) * 160 - (src6 + src8) * 48 + (src5 + src7) * 24 - (src4 + src6) * 8 + (128 - rounding)) / 256), 0, 255);
        
		dst++;
        src++;
    }
}

void interpolate8x8_lowpass_hv(uint8_t *dst1, uint8_t *dst2, uint8_t *src, int32_t dst1_stride, int32_t dst2_stride, int32_t src_stride, int32_t rounding)
{
    uint8_t data[72];

	int32_t i;


    for(i = 0; i < 9; i++)
    {
        dst2[0] = data[8 * i + 0] = CLIP((((src[0] + src[1]) * 160 - (src[0] + src[2]) * 48 + (src[1] + src[3]) * 24 - (src[2] + src[4]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[1] = data[8 * i + 1] = CLIP((((src[1] + src[2]) * 160 - (src[0] + src[3]) * 48 + (src[0] + src[4]) * 24 - (src[1] + src[5]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[2] = data[8 * i + 2] = CLIP((((src[2] + src[3]) * 160 - (src[1] + src[4]) * 48 + (src[0] + src[5]) * 24 - (src[0] + src[6]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[3] = data[8 * i + 3] = CLIP((((src[3] + src[4]) * 160 - (src[2] + src[5]) * 48 + (src[1] + src[6]) * 24 - (src[0] + src[7]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[4] = data[8 * i + 4] = CLIP((((src[4] + src[5]) * 160 - (src[3] + src[6]) * 48 + (src[2] + src[7]) * 24 - (src[1] + src[8]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[5] = data[8 * i + 5] = CLIP((((src[5] + src[6]) * 160 - (src[4] + src[7]) * 48 + (src[3] + src[8]) * 24 - (src[2] + src[8]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[6] = data[8 * i + 6] = CLIP((((src[6] + src[7]) * 160 - (src[5] + src[8]) * 48 + (src[4] + src[8]) * 24 - (src[3] + src[7]) * 8 + (128 - rounding)) / 256), 0, 255);
        dst2[7] = data[8 * i + 7] = CLIP((((src[7] + src[8]) * 160 - (src[6] + src[8]) * 48 + (src[5] + src[7]) * 24 - (src[4] + src[6]) * 8 + (128 - rounding)) / 256), 0, 255);

        src += src_stride;
		dst2 += dst2_stride;
    }

    for(i = 0; i < 8; i++)
    {
        int32_t src0 = data[i];
        int32_t src1 = data[8 + i];
        int32_t src2 = data[2 * 8 + i];
        int32_t src3 = data[3 * 8 + i];
        int32_t src4 = data[4 * 8 + i];
        int32_t src5 = data[5 * 8 + i];
        int32_t src6 = data[6 * 8 + i];
        int32_t src7 = data[7 * 8 + i];
        int32_t src8 = data[8 * 8 + i];
        
		dst1[0] = CLIP((((src0 + src1) * 160 - (src0 + src2) * 48 + (src1 + src3) * 24 - (src2 + src4) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[dst1_stride] = CLIP((((src1 + src2) * 160 - (src0 + src3) * 48 + (src0 + src4) * 24 - (src1 + src5) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[2 * dst1_stride] = CLIP((((src2 + src3) * 160 - (src1 + src4) * 48 + (src0 + src5) * 24 - (src0 + src6) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[3 * dst1_stride] = CLIP((((src3 + src4) * 160 - (src2 + src5) * 48 + (src1 + src6) * 24 - (src0 + src7) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[4 * dst1_stride] = CLIP((((src4 + src5) * 160 - (src3 + src6) * 48 + (src2 + src7) * 24 - (src1 + src8) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[5 * dst1_stride] = CLIP((((src5 + src6) * 160 - (src4 + src7) * 48 + (src3 + src8) * 24 - (src2 + src8) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[6 * dst1_stride] = CLIP((((src6 + src7) * 160 - (src5 + src8) * 48 + (src4 + src8) * 24 - (src3 + src7) * 8 + (128 - rounding)) / 256), 0, 255);
        dst1[7 * dst1_stride] = CLIP((((src7 + src8) * 160 - (src6 + src8) * 48 + (src5 + src7) * 24 - (src4 + src6) * 8 + (128 - rounding)) / 256), 0, 255);
        
		dst1++;
    }
}

void interpolate8x8_bilinear2(uint8_t *dst, uint8_t *src1, uint8_t *src2, int32_t dst_stride, int32_t src_stride, int32_t rounding)
{
    int32_t i;

    for(i = 0; i < 8; i++)
    {
        dst[0] = (src1[0] + src2[0] + (1 - rounding)) >> 1;
        dst[1] = (src1[1] + src2[1] + (1 - rounding)) >> 1;
        dst[2] = (src1[2] + src2[2] + (1 - rounding)) >> 1;
        dst[3] = (src1[3] + src2[3] + (1 - rounding)) >> 1;
        dst[4] = (src1[4] + src2[4] + (1 - rounding)) >> 1;
        dst[5] = (src1[5] + src2[5] + (1 - rounding)) >> 1;
        dst[6] = (src1[6] + src2[6] + (1 - rounding)) >> 1;
        dst[7] = (src1[7] + src2[7] + (1 - rounding)) >> 1;

        dst += dst_stride;
        src1 += src_stride;
        src2 += 8;
    }
}

void interpolate8x8_bilinear4(uint8_t *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int32_t stride, int32_t rounding)
{
    int32_t i;

    for(i = 0; i < 8; i++)
    {
        dst[0] = (src1[0] + src2[0] + src3[0] + src4[0] + (2 - rounding)) >> 2;
        dst[1] = (src1[1] + src2[1] + src3[1] + src4[1] + (2 - rounding)) >> 2;
        dst[2] = (src1[2] + src2[2] + src3[2] + src4[2] + (2 - rounding)) >> 2;
        dst[3] = (src1[3] + src2[3] + src3[3] + src4[3] + (2 - rounding)) >> 2;
        dst[4] = (src1[4] + src2[4] + src3[4] + src4[4] + (2 - rounding)) >> 2;
        dst[5] = (src1[5] + src2[5] + src3[5] + src4[5] + (2 - rounding)) >> 2;
        dst[6] = (src1[6] + src2[6] + src3[6] + src4[6] + (2 - rounding)) >> 2;
        dst[7] = (src1[7] + src2[7] + src3[7] + src4[7] + (2 - rounding)) >> 2;
        
		dst += stride;
        src1 += stride;
        src2 += 8;
        src3 += 8;
        src4 += 8;
    }
}
