/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8bit<->16bit transfer -
 *
 *  Copyright(C) 2001-2002 Peter Ross <pross@xvid.org>
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
 * $Id: mem_transfer.c,v 1.7 2002-11-17 00:51:11 edgomez Exp $
 *
 ****************************************************************************/

#include "../global.h"
#include "mem_transfer.h"

/* Function pointers - Initialized in the xvid.c module */

TRANSFER_8TO16COPY_PTR transfer_8to16copy;
TRANSFER_16TO8COPY_PTR transfer_16to8copy;

TRANSFER_8TO16SUB_PTR  transfer_8to16sub;
TRANSFER_8TO16SUB2_PTR transfer_8to16sub2;
TRANSFER_16TO8ADD_PTR  transfer_16to8add;

TRANSFER8X8_COPY_PTR transfer8x8_copy;


/*****************************************************************************
 *
 * All these functions are used to transfer data from a 8 bit data array
 * to a 16 bit data array.
 *
 * This is typically used during motion compensation, that's why some
 * functions also do the addition/substraction of another buffer during the
 * so called  transfer.
 *
 ****************************************************************************/

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (8bit)  = SRC
 *    DST (16bit) = SRC
 */
void
transfer_8to16copy_c(int16_t * const dst,
					 const uint8_t * const src,
					 uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[j * 8 + i] = (int16_t) src[j * stride + i];
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(SRC, 255), 0)
 */
void
transfer_16to8copy_c(uint8_t * const dst,
					 const int16_t * const src,
					 uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pixel = src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
		}
	}
}




/*
 * C   - the current buffer
 * R   - the reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R   (8bit)  = R
 *    C   (8bit)  = R
 *    DCT (16bit) = C - R
 */
void
transfer_8to16sub_c(int16_t * const dct,
					uint8_t * const cur,
					const uint8_t * ref,
					const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint8_t c = cur[j * stride + i];
			uint8_t r = ref[j * stride + i];

			cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


/*
 * C   - the current buffer
 * R1  - the 1st reference buffer
 * R2  - the 2nd reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R1  (8bit) = R1
 *    R2  (8bit) = R2
 *    C   (8bit) = C
 *    DCT (16bit)= C - min((R1 + R2)/2, 255)
 */
void
transfer_8to16sub2_c(int16_t * const dct,
					 uint8_t * const cur,
					 const uint8_t * ref1,
					 const uint8_t * ref2,
					 const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint8_t c = cur[j * stride + i];
			int r = (ref1[j * stride + i] + ref2[j * stride + i] + 1) / 2;

			if (r > 255) {
				r = 255;
			}
			//cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 16->8 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(DST+SRC, 255), 0)
 */
void
transfer_16to8add_c(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			int16_t pixel = (int16_t) dst[j * stride + i] + src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->8 bit transfer and this serie of operations :
 *
 *    SRC (8bit) = SRC
 *    DST (8bit) = SRC
 */
void
transfer8x8_copy_c(uint8_t * const dst,
				   const uint8_t * const src,
				   const uint32_t stride)
{
	uint32_t i, j;

	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			dst[j * stride + i] = src[j * stride + i];
		}
	}
}
