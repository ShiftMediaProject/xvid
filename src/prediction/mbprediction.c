/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Prediction functions -
 *
 *  Copyright(C) 2001-2002 - Michael Militzer <isibaar@xvid.org>
 *  Copyright(C) 2001-2002 - Peter Ross <pross@xvid.org>
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
 * $Id: mbprediction.c,v 1.12 2002-12-15 01:21:12 edgomez Exp $
 *
 ****************************************************************************/

#include "../encoder.h"
#include "mbprediction.h"
#include "../utils/mbfunctions.h"
#include "../bitstream/cbp.h"


#define ABS(X) (((X)>0)?(X):-(X))
#define DIV_DIV(A,B)    ( (A) > 0 ? ((A)+((B)>>1))/(B) : ((A)-((B)>>1))/(B) )


/*****************************************************************************
 * Local inlined function
 ****************************************************************************/

static int __inline
rescale(int predict_quant,
		int current_quant,
		int coeff)
{
	return (coeff != 0) ? DIV_DIV((coeff) * (predict_quant),
								  (current_quant)) : 0;
}


/*****************************************************************************
 * Local data
 ****************************************************************************/

static const int16_t default_acdc_values[15] = {
	1024,
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0
};


/*****************************************************************************
 * Functions
 ****************************************************************************/


/*	get dc/ac prediction direction for a single block and place
	predictor values into MB->pred_values[j][..]
*/


void
predict_acdc(MACROBLOCK * pMBs,
			 uint32_t x,
			 uint32_t y,
			 uint32_t mb_width,
			 uint32_t block,
			 int16_t qcoeff[64],
			 uint32_t current_quant,
			 int32_t iDcScaler,
			 int16_t predictors[8],
			const int bound)

{
	const int mbpos = (y * mb_width) + x;
	int16_t *left, *top, *diag, *current;

	int32_t left_quant = current_quant;
	int32_t top_quant = current_quant;

	const int16_t *pLeft = default_acdc_values;
	const int16_t *pTop = default_acdc_values;
	const int16_t *pDiag = default_acdc_values;

	uint32_t index = x + y * mb_width;	/* current macroblock */
	int *acpred_direction = &pMBs[index].acpred_directions[block];
	uint32_t i;

	left = top = diag = current = 0;

	/* grab left,top and diag macroblocks */

	/* left macroblock  */

	if (x && mbpos >= bound + 1  &&
		(pMBs[index - 1].mode == MODE_INTRA ||
		 pMBs[index - 1].mode == MODE_INTRA_Q)) {

		left = pMBs[index - 1].pred_values[0];
		left_quant = pMBs[index - 1].quant;
		/*DEBUGI("LEFT", *(left+MBPRED_SIZE)); */
	}
	/* top macroblock */

	if (mbpos >= bound + (int)mb_width &&
		(pMBs[index - mb_width].mode == MODE_INTRA ||
		 pMBs[index - mb_width].mode == MODE_INTRA_Q)) {

		top = pMBs[index - mb_width].pred_values[0];
		top_quant = pMBs[index - mb_width].quant;
	}
	/* diag macroblock  */

	if (x && mbpos >= bound + (int)mb_width + 1 &&
		(pMBs[index - 1 - mb_width].mode == MODE_INTRA ||
		 pMBs[index - 1 - mb_width].mode == MODE_INTRA_Q)) {

		diag = pMBs[index - 1 - mb_width].pred_values[0];
	}

	current = pMBs[index].pred_values[0];

	/* now grab pLeft, pTop, pDiag _blocks_  */

	switch (block) {

	case 0:
		if (left)
			pLeft = left + MBPRED_SIZE;

		if (top)
			pTop = top + (MBPRED_SIZE << 1);

		if (diag)
			pDiag = diag + 3 * MBPRED_SIZE;

		break;

	case 1:
		pLeft = current;
		left_quant = current_quant;

		if (top) {
			pTop = top + 3 * MBPRED_SIZE;
			pDiag = top + (MBPRED_SIZE << 1);
		}
		break;

	case 2:
		if (left) {
			pLeft = left + 3 * MBPRED_SIZE;
			pDiag = left + MBPRED_SIZE;
		}

		pTop = current;
		top_quant = current_quant;

		break;

	case 3:
		pLeft = current + (MBPRED_SIZE << 1);
		left_quant = current_quant;

		pTop = current + MBPRED_SIZE;
		top_quant = current_quant;

		pDiag = current;

		break;

	case 4:
		if (left)
			pLeft = left + (MBPRED_SIZE << 2);
		if (top)
			pTop = top + (MBPRED_SIZE << 2);
		if (diag)
			pDiag = diag + (MBPRED_SIZE << 2);
		break;

	case 5:
		if (left)
			pLeft = left + 5 * MBPRED_SIZE;
		if (top)
			pTop = top + 5 * MBPRED_SIZE;
		if (diag)
			pDiag = diag + 5 * MBPRED_SIZE;
		break;
	}

	/*  determine ac prediction direction & ac/dc predictor */
	/*  place rescaled ac/dc predictions into predictors[] for later use */

	if (ABS(pLeft[0] - pDiag[0]) < ABS(pDiag[0] - pTop[0])) {
		*acpred_direction = 1;	/* vertical */
		predictors[0] = (int16_t)(DIV_DIV(pTop[0], iDcScaler));
		for (i = 1; i < 8; i++) {
			predictors[i] = rescale(top_quant, current_quant, pTop[i]);
		}
	} else {
		*acpred_direction = 2;	/* horizontal */
		predictors[0] = (int16_t)(DIV_DIV(pLeft[0], iDcScaler));
		for (i = 1; i < 8; i++) {
			predictors[i] = rescale(left_quant, current_quant, pLeft[i + 7]);
		}
	}
}


/* decoder: add predictors to dct_codes[] and
   store current coeffs to pred_values[] for future prediction 
*/


void
add_acdc(MACROBLOCK * pMB,
		 uint32_t block,
		 int16_t dct_codes[64],
		 uint32_t iDcScaler,
		 int16_t predictors[8])
{
	uint8_t acpred_direction = pMB->acpred_directions[block];
	int16_t *pCurrent = pMB->pred_values[block];
	uint32_t i;

	DPRINTF(DPRINTF_COEFF,"predictor[0] %i", predictors[0]);

	dct_codes[0] += predictors[0];	/* dc prediction */
	pCurrent[0] = (int16_t)(dct_codes[0] * iDcScaler);

	if (acpred_direction == 1) {
		for (i = 1; i < 8; i++) {
			int level = dct_codes[i] + predictors[i];

			DPRINTF(DPRINTF_COEFF,"predictor[%i] %i",i, predictors[i]);

			dct_codes[i] = level;
			pCurrent[i] = level;
			pCurrent[i + 7] = dct_codes[i * 8];
		}
	} else if (acpred_direction == 2) {
		for (i = 1; i < 8; i++) {
			int level = dct_codes[i * 8] + predictors[i];
			DPRINTF(DPRINTF_COEFF,"predictor[%i] %i",i*8, predictors[i]);

			dct_codes[i * 8] = level;
			pCurrent[i + 7] = level;
			pCurrent[i] = dct_codes[i];
		}
	} else {
		for (i = 1; i < 8; i++) {
			pCurrent[i] = dct_codes[i];
			pCurrent[i + 7] = dct_codes[i * 8];
		}
	}
}



/* ****************************************************************** */
/* ****************************************************************** */

/* encoder: subtract predictors from qcoeff[] and calculate S1/S2

todo: perform [-127,127] clamping after prediction
clamping must adjust the coeffs, so dequant is done correctly
				   
S1/S2 are used  to determine if its worth predicting for AC
S1 = sum of all (qcoeff - prediction)
S2 = sum of all qcoeff
*/

uint32_t
calc_acdc(MACROBLOCK * pMB,
		  uint32_t block,
		  int16_t qcoeff[64],
		  uint32_t iDcScaler,
		  int16_t predictors[8])
{
	int16_t *pCurrent = pMB->pred_values[block];
	uint32_t i;
	uint32_t S1 = 0, S2 = 0;


	/* store current coeffs to pred_values[] for future prediction */

	pCurrent[0] = (int16_t)(qcoeff[0] * iDcScaler);
	for (i = 1; i < 8; i++) {
		pCurrent[i] = qcoeff[i];
		pCurrent[i + 7] = qcoeff[i * 8];
	}

	/* subtract predictors and store back in predictors[] */

	qcoeff[0] = qcoeff[0] - predictors[0];

	if (pMB->acpred_directions[block] == 1) {
		for (i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i];
			S2 += ABS(level);
			level -= predictors[i];
			S1 += ABS(level);
			predictors[i] = level;
		}
	} else						/* acpred_direction == 2 */
	{
		for (i = 1; i < 8; i++) {
			int16_t level;

			level = qcoeff[i * 8];
			S2 += ABS(level);
			level -= predictors[i];
			S1 += ABS(level);
			predictors[i] = level;
		}

	}


	return S2 - S1;
}


/* apply predictors[] to qcoeff */

void
apply_acdc(MACROBLOCK * pMB,
		   uint32_t block,
		   int16_t qcoeff[64],
		   int16_t predictors[8])
{
	uint32_t i;

	if (pMB->acpred_directions[block] == 1) {
		for (i = 1; i < 8; i++) {
			qcoeff[i] = predictors[i];
		}
	} else {
		for (i = 1; i < 8; i++) {
			qcoeff[i * 8] = predictors[i];
		}
	}
}


void
MBPrediction(FRAMEINFO * frame,
			 uint32_t x,
			 uint32_t y,
			 uint32_t mb_width,
			 int16_t qcoeff[6 * 64])
{

	int32_t j;
	int32_t iDcScaler, iQuant = frame->quant;
	int32_t S = 0;
	int16_t predictors[6][8];

	MACROBLOCK *pMB = &frame->mbs[x + y * mb_width];

	if ((pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q)) {

		for (j = 0; j < 6; j++) {
			iDcScaler = get_dc_scaler(iQuant, (j < 4) ? 1 : 0);

			predict_acdc(frame->mbs, x, y, mb_width, j, &qcoeff[j * 64],
						 iQuant, iDcScaler, predictors[j], 0);

			S += calc_acdc(pMB, j, &qcoeff[j * 64], iDcScaler, predictors[j]);

		}

		if (S < 0)				/* dont predict */
		{
			for (j = 0; j < 6; j++) {
				pMB->acpred_directions[j] = 0;
			}
		} else {
			for (j = 0; j < 6; j++) {
				apply_acdc(pMB, j, &qcoeff[j * 64], predictors[j]);
			}
		}
		pMB->cbp = calc_cbp(qcoeff);
	}

}
