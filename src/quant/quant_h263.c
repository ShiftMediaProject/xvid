/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- quantization/dequantization -
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
 * $Id: quant_h263.c,v 1.6 2002-12-15 01:21:12 edgomez Exp $
 *
 *************************************************************************/

#include "quant_h263.h"

/*	mutliply+shift division table
*/

#define SCALEBITS	16
#define FIX(X)		((1L << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] = {
	0, FIX(2), FIX(4), FIX(6),
	FIX(8), FIX(10), FIX(12), FIX(14),
	FIX(16), FIX(18), FIX(20), FIX(22),
	FIX(24), FIX(26), FIX(28), FIX(30),
	FIX(32), FIX(34), FIX(36), FIX(38),
	FIX(40), FIX(42), FIX(44), FIX(46),
	FIX(48), FIX(50), FIX(52), FIX(54),
	FIX(56), FIX(58), FIX(60), FIX(62)
};



#define DIV_DIV(a, b) ((a)>0) ? ((a)+((b)>>1))/(b) : ((a)-((b)>>1))/(b)

/* function pointers */
quanth263_intraFuncPtr quant_intra;
quanth263_intraFuncPtr dequant_intra;

quanth263_interFuncPtr quant_inter;
dequanth263_interFuncPtr dequant_inter;



/*	quantize intra-block
*/


void
quant_intra_c(int16_t * coeff,
			  const int16_t * data,
			  const uint32_t quant,
			  const uint32_t dcscalar)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = (uint16_t)(quant << 1);
	uint32_t i;

	coeff[0] = (int16_t)(DIV_DIV(data[0], (int32_t) dcscalar));

	for (i = 1; i < 64; i++) {
		int16_t acLevel = data[i];

		if (acLevel < 0) {
			acLevel = -acLevel;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (int16_t)(((uint32_t)acLevel * mult) >> SCALEBITS);
			coeff[i] = -acLevel;
		} else {
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (int16_t)(((uint32_t)acLevel * mult) >> SCALEBITS);
			coeff[i] = acLevel;
		}
	}
}


/*	quantize inter-block
*/

uint32_t
quant_inter_c(int16_t * coeff,
			  const int16_t * data,
			  const uint32_t quant)
{
	const uint32_t mult = multipliers[quant];
	const uint16_t quant_m_2 = (uint16_t)(quant << 1);
	const uint16_t quant_d_2 = (uint16_t)(quant >> 1);
	int sum = 0;
	uint32_t i;

	for (i = 0; i < 64; i++) {
		int16_t acLevel = data[i];

		if (acLevel < 0) {
			acLevel = (-acLevel) - quant_d_2;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}

			acLevel = (int16_t)(((uint32_t)acLevel * mult) >> SCALEBITS);
			sum += acLevel;		/* sum += |acLevel| */
			coeff[i] = -acLevel;
		} else {
			acLevel -= quant_d_2;
			if (acLevel < quant_m_2) {
				coeff[i] = 0;
				continue;
			}
			acLevel = (int16_t)(((uint32_t)acLevel * mult) >> SCALEBITS);
			sum += acLevel;
			coeff[i] = acLevel;
		}
	}

	return sum;
}


/*	dequantize intra-block & clamp to [-2048,2047]
*/

void
dequant_intra_c(int16_t * data,
				const int16_t * coeff,
				const uint32_t quant,
				const uint32_t dcscalar)
{
	const int32_t quant_m_2 = quant << 1;
	const int32_t quant_add = (quant & 1 ? quant : quant - 1);
	uint32_t i;

	data[0] = (int16_t)(coeff[0] * dcscalar);
	if (data[0] < -2048) {
		data[0] = -2048;
	} else if (data[0] > 2047) {
		data[0] = 2047;
	}


	for (i = 1; i < 64; i++) {
		int32_t acLevel = coeff[i];

		if (acLevel == 0) {
			data[i] = 0;
		} else if (acLevel < 0) {
			acLevel = quant_m_2 * -acLevel + quant_add;
			data[i] = (acLevel <= 2048 ? -acLevel : -2048);
		} else					/*  if (acLevel > 0) { */
		{
			acLevel = quant_m_2 * acLevel + quant_add;
			data[i] = (acLevel <= 2047 ? acLevel : 2047);
		}
	}
}



/* dequantize inter-block & clamp to [-2048,2047]
*/

void
dequant_inter_c(int16_t * data,
				const int16_t * coeff,
				const uint32_t quant)
{
	const uint16_t quant_m_2 = (uint16_t)(quant << 1);
	const uint16_t quant_add = (uint16_t)(quant & 1 ? quant : quant - 1);
	uint32_t i;

	for (i = 0; i < 64; i++) {
		int16_t acLevel = coeff[i];

		if (acLevel == 0) {
			data[i] = 0;
		} else if (acLevel < 0) {
			acLevel = acLevel * quant_m_2 - quant_add;
			data[i] = (acLevel >= -2048 ? acLevel : -2048);
		} else					/* if (acLevel > 0) */
		{
			acLevel = acLevel * quant_m_2 + quant_add;
			data[i] = (acLevel <= 2047 ? acLevel : 2047);
		}
	}
}
