/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	sum of absolute difference
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
 *	14.02.2002	added sad16bi_c()
 *	10.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "../portab.h"
#include "sad.h"

sad16FuncPtr sad16;
sad8FuncPtr sad8;
sad16biFuncPtr sad16bi;
sad8biFuncPtr sad8bi;		// not really sad16, but no difference in prototype
dev16FuncPtr dev16;

sadInitFuncPtr sadInit;

#define ABS(X) (((X)>0)?(X):-(X))

uint32_t
sad8FuncPtr sad8;
		  const uint8_t * const ref,

sad16biFuncPtr sad8bi;		// not really sad16, but no difference in prototype
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
