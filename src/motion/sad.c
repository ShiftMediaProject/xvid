/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SAD calculation module (C part) -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *               2002 Peter Ross <pross@xvid.org>
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
 * $Id: sad.c,v 1.12 2002-11-26 23:44:11 edgomez Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "sad.h"

sad16FuncPtr sad16;
sad8FuncPtr sad8;
sad16biFuncPtr sad16bi;
sad8biFuncPtr sad8bi;		/* not really sad16, but no difference in prototype */
dev16FuncPtr dev16;

sadInitFuncPtr sadInit;

#define ABS(X) (((X)>0)?(X):-(X))

#define MRSAD16_CORRFACTOR 8
uint32_t
mrsad16_c(const uint8_t * const cur,
		  const uint8_t * const ref,
		  const uint32_t stride,
		  const uint32_t best_sad)
{

	uint32_t sad = 0;
	int32_t mean = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 16; i++) {
			mean += ((int) *(ptr_cur + i) - (int) *(ptr_ref + i));
		}
		ptr_cur += stride;
		ptr_ref += stride;

	}
	mean /= 256;

	for (j = 0; j < 16; j++) {

		ptr_cur -= stride;
		ptr_ref -= stride;

		for (i = 0; i < 16; i++) {

			sad += ABS(*(ptr_cur + i) - *(ptr_ref + i) - mean);
			if (sad >= best_sad) {
				return MRSAD16_CORRFACTOR * sad;
			}
		}
	}

	return MRSAD16_CORRFACTOR * sad;

}


uint32_t
sad16_c(const uint8_t * const cur,
		const uint8_t * const ref,
		const uint32_t stride,
		const uint32_t best_sad)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++) {

			sad += ABS(*(ptr_cur + i) - *(ptr_ref + i));

			if (sad >= best_sad) {
				return sad;
			}


		}

		ptr_cur += stride;
		ptr_ref += stride;

	}

	return sad;

}



uint32_t
sad16bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}

			sad += ABS(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}

uint32_t
sad8bi_c(const uint8_t * const cur,
		  const uint8_t * const ref1,
		  const uint8_t * const ref2,
		  const uint32_t stride)
{

	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref1 = ref1;
	uint8_t const *ptr_ref2 = ref2;

	for (j = 0; j < 8; j++) {

		for (i = 0; i < 8; i++) {
			int pixel = (ptr_ref1[i] + ptr_ref2[i] + 1) / 2;

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}

			sad += ABS(ptr_cur[i] - pixel);
		}

		ptr_cur += stride;
		ptr_ref1 += stride;
		ptr_ref2 += stride;

	}

	return sad;

}



uint32_t
sad8_c(const uint8_t * const cur,
	   const uint8_t * const ref,
	   const uint32_t stride)
{
	uint32_t sad = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 8; j++) {

		for (i = 0; i < 8; i++) {
			sad += ABS(*(ptr_cur + i) - *(ptr_ref + i));
		}

		ptr_cur += stride;
		ptr_ref += stride;

	}

	return sad;
}




/* average deviation from mean */

uint32_t
dev16_c(const uint8_t * const cur,
		const uint32_t stride)
{

	uint32_t mean = 0;
	uint32_t dev = 0;
	uint32_t i, j;
	uint8_t const *ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			mean += *(ptr_cur + i);

		ptr_cur += stride;

	}

	mean /= (16 * 16);
	ptr_cur = cur;

	for (j = 0; j < 16; j++) {

		for (i = 0; i < 16; i++)
			dev += ABS(*(ptr_cur + i) - (int32_t) mean);

		ptr_cur += stride;

	}

	return dev;
}
