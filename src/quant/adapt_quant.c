/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Adaptive quantization functions -
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
 *               2002 Christoph Lampert <gruel@web.de>
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
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  $Id: adapt_quant.c,v 1.11 2002-10-19 12:20:33 edgomez Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "adapt_quant.h"

#include <stdlib.h>				/* free, malloc */

#define MAX(a,b)      (((a) > (b)) ? (a) : (b))
#define RDIFF(a,b)    ((int)(a+0.5)-(int)(b+0.5))

/*****************************************************************************
 * Functions
 ****************************************************************************/

int
normalize_quantizer_field(float *in,
						  int *out,
						  int num,
						  int min_quant,
						  int max_quant)
{
	int i;
	int finished;

	do {
		finished = 1;
		for (i = 1; i < num; i++) {
			if (RDIFF(in[i], in[i - 1]) > 2) {
				in[i] -= (float) 0.5;
				finished = 0;
			} else if (RDIFF(in[i], in[i - 1]) < -2) {
				in[i - 1] -= (float) 0.5;
				finished = 0;
			}

			if (in[i] > max_quant) {
				in[i] = (float) max_quant;
				finished = 0;
			}
			if (in[i] < min_quant) {
				in[i] = (float) min_quant;
				finished = 0;
			}
			if (in[i - 1] > max_quant) {
				in[i - 1] = (float) max_quant;
				finished = 0;
			}
			if (in[i - 1] < min_quant) {
				in[i - 1] = (float) min_quant;
				finished = 0;
			}
		}
	} while (!finished);

	out[0] = 0;
	for (i = 1; i < num; i++)
		out[i] = RDIFF(in[i], in[i - 1]);

	return (int) (in[0] + 0.5);
}

int
adaptive_quantization(unsigned char *buf,
					  int stride,
					  int *intquant,
					  int framequant,
					  int min_quant,
					  int max_quant,
					  int mb_width,
					  int mb_height)	// no qstride because normalization
{
	int i, j, k, l;

	float *quant;
	unsigned char *ptr;
	float *val;
	float global = 0.;
	uint32_t mid_range = 0;

	const float DarkAmpl = 14 / 2;
	const float BrightAmpl = 10 / 2;
	const float DarkThres = 70;
	const float BrightThres = 200;

	const float GlobalDarkThres = 60;
	const float GlobalBrightThres = 170;

	const float MidRangeThres = 20;
	const float UpperLimit = 200;
	const float LowerLimit = 25;


	if (!(quant = (float *) malloc(mb_width * mb_height * sizeof(float))))
		return(-1);

	if(!(val = (float *) malloc(mb_width * mb_height * sizeof(float)))) {
		free(quant);
		return(-1);
	}

	for (k = 0; k < mb_height; k++) {
		for (l = 0; l < mb_width; l++)	// do this for all macroblocks individually 
		{
			quant[k * mb_width + l] = (float) framequant;

			// calculate luminance-masking
			ptr = &buf[16 * k * stride + 16 * l];	// address of MB

			val[k * mb_width + l] = 0.;

			for (i = 0; i < 16; i++)
				for (j = 0; j < 16; j++)
					val[k * mb_width + l] += ptr[i * stride + j];
			val[k * mb_width + l] /= 256.;
			global +=val[k * mb_width + l];

			if ((val[k * mb_width + l] > LowerLimit) &&
				(val[k * mb_width + l] < UpperLimit))
				mid_range++;
		}
	}

	global /=mb_width * mb_height;

	if (((global <GlobalBrightThres) &&(global >GlobalDarkThres))
		|| (mid_range < MidRangeThres)) {
		for (k = 0; k < mb_height; k++) {
			for (l = 0; l < mb_width; l++)	// do this for all macroblocks individually 
			{
				if (val[k * mb_width + l] < DarkThres)
					quant[k * mb_width + l] +=
						DarkAmpl * (DarkThres -
									val[k * mb_width + l]) / DarkThres;
				else if (val[k * mb_width + l] > BrightThres)
					quant[k * mb_width + l] +=
						BrightAmpl * (val[k * mb_width + l] -
									  BrightThres) / (255 - BrightThres);
			}
		}
	}

	i = normalize_quantizer_field(quant, intquant,
								  mb_width * mb_height,
								  min_quant, max_quant);

	free(val);
	free(quant);

	return(i);

}
