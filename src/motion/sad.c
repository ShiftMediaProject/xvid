/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	sum of absolute difference
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
 *	14.02.2002	added sad16bi_c()
 *	10.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "../portab.h"
#include "../global.h"
#include "sad.h"

sad16FuncPtr sad16;
sad8FuncPtr sad8;
sad16biFuncPtr sad16bi;
sad8biFuncPtr sad8bi;		// not really sad16, but no difference in prototype
dev16FuncPtr dev16;
sad16vFuncPtr sad16v;

sadInitFuncPtr sadInit;


uint32_t
sad16_c(const uint8_t * const cur,
		const uint8_t * const ref,
		const uint32_t stride,
		const uint32_t best_sad)
{

	uint32_t sad = 0;
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 16; j++) {
			sad += ABS(ptr_cur[0] - ptr_ref[0]);
			sad += ABS(ptr_cur[1] - ptr_ref[1]);
			sad += ABS(ptr_cur[2] - ptr_ref[2]);
			sad += ABS(ptr_cur[3] - ptr_ref[3]);
			sad += ABS(ptr_cur[4] - ptr_ref[4]);
			sad += ABS(ptr_cur[5] - ptr_ref[5]);
			sad += ABS(ptr_cur[6] - ptr_ref[6]);
			sad += ABS(ptr_cur[7] - ptr_ref[7]);
			sad += ABS(ptr_cur[8] - ptr_ref[8]);
			sad += ABS(ptr_cur[9] - ptr_ref[9]);
			sad += ABS(ptr_cur[10] - ptr_ref[10]);
			sad += ABS(ptr_cur[11] - ptr_ref[11]);
			sad += ABS(ptr_cur[12] - ptr_ref[12]);
			sad += ABS(ptr_cur[13] - ptr_ref[13]);
			sad += ABS(ptr_cur[14] - ptr_ref[14]);
			sad += ABS(ptr_cur[15] - ptr_ref[15]);

			if (sad >= best_sad) 
				return sad;

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
	uint32_t j;
	uint8_t const *ptr_cur = cur;
	uint8_t const *ptr_ref = ref;

	for (j = 0; j < 8; j++) {

		sad += ABS(ptr_cur[0] - ptr_ref[0]);
		sad += ABS(ptr_cur[1] - ptr_ref[1]);
		sad += ABS(ptr_cur[2] - ptr_ref[2]);
		sad += ABS(ptr_cur[3] - ptr_ref[3]);
		sad += ABS(ptr_cur[4] - ptr_ref[4]);
		sad += ABS(ptr_cur[5] - ptr_ref[5]);
		sad += ABS(ptr_cur[6] - ptr_ref[6]);
		sad += ABS(ptr_cur[7] - ptr_ref[7]);
		
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

uint32_t sad16v_c(const uint8_t * const cur, 
			   const uint8_t * const ref, 
			   const uint32_t stride, 
			   int32_t *sad)
{
	sad[0] = sad8(cur, ref, stride);
	sad[1] = sad8(cur + 8, ref + 8, stride);
	sad[2] = sad8(cur + 8*stride, ref + 8*stride, stride);
	sad[3] = sad8(cur + 8*stride + 8, ref + 8*stride + 8, stride);
	
	return sad[0]+sad[1]+sad[2]+sad[3];
}

uint32_t sad32v_c(const uint8_t * const cur, 
			   const uint8_t * const ref, 
			   const uint32_t stride, 
			   int32_t *sad)
{
	sad[0] = sad16(cur, ref, stride, 256*4096);
	sad[1] = sad16(cur + 8, ref + 8, stride, 256*4096);
	sad[2] = sad16(cur + 8*stride, ref + 8*stride, stride, 256*4096);
	sad[3] = sad16(cur + 8*stride + 8, ref + 8*stride + 8, stride, 256*4096);
	
	return sad[0]+sad[1]+sad[2]+sad[3];
}



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


