 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  mbtransquant.c                                                            *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>                  *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  29.03.2002 interlacing speedup - used transfer strides instead of		   *
  *             manual field-to-frame conversion							   *
  *  26.03.2002 interlacing support - moved transfers outside loops			   *
  *  22.12.2001 get_dc_scaler() moved to common.h							   *
  *  19.11.2001 introduced coefficient thresholding (Isibaar)                  *
  *  17.11.2001 initial version                                                *
  *                                                                            *
  ******************************************************************************/

#include <string.h>

#include "../portab.h"
#include "mbfunctions.h"

#include "../global.h"
#include "mem_transfer.h"
#include "timer.h"
#include "../dct/fdct.h"
#include "../dct/idct.h"
#include "../quant/quant_mpeg4.h"
#include "../quant/quant_h263.h"
#include "../encoder.h"

#include "../image/reduced.h"

MBFIELDTEST_PTR MBFieldTest;

#define TOOSMALL_LIMIT 	1	/* skip blocks having a coefficient sum below this value */

static __inline void
MBfDCT(int16_t data[6 * 64])
{
	start_timer();
	fdct(&data[0 * 64]);
	fdct(&data[1 * 64]);
	fdct(&data[2 * 64]);
	fdct(&data[3 * 64]);
	fdct(&data[4 * 64]);
	fdct(&data[5 * 64]);
	stop_dct_timer();
}


static __inline uint32_t 
QuantizeInterBlock(	int16_t qcoeff[64],
					const int16_t data[64],
					const uint32_t iQuant,
					const uint32_t quant_type)
{
	uint32_t sum;

	start_timer();
	if (quant_type == H263_QUANT)
		sum = quant_inter(qcoeff, data, iQuant);
	else
		sum = quant4_inter(qcoeff, data, iQuant);

	stop_quant_timer();
	return sum;
}

void
MBTransQuantIntra(const MBParam * const pParam,
				FRAMEINFO * const frame,
				MACROBLOCK * const pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				int16_t data[6 * 64],
				int16_t qcoeff[6 * 64])
{

	uint32_t stride = pParam->edged_width;
	const uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * ((frame->global_flags & XVID_REDUCED)?16:8);
	int i;
	const uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	const IMAGE * const pCurrent = &frame->image;

	start_timer();
	if ((frame->global_flags & XVID_REDUCED))
	{
		pY_Cur = pCurrent->y + (y_pos << 5) * stride + (x_pos << 5);
		pU_Cur = pCurrent->u + (y_pos << 4) * stride2 + (x_pos << 4);
		pV_Cur = pCurrent->v + (y_pos << 4) * stride2 + (x_pos << 4);

		filter_18x18_to_8x8(&data[0 * 64], pY_Cur, stride);
		filter_18x18_to_8x8(&data[1 * 64], pY_Cur + 16, stride);
		filter_18x18_to_8x8(&data[2 * 64], pY_Cur + next_block, stride);
		filter_18x18_to_8x8(&data[3 * 64], pY_Cur + next_block + 16, stride);
		filter_18x18_to_8x8(&data[4 * 64], pU_Cur, stride2);
		filter_18x18_to_8x8(&data[5 * 64], pV_Cur, stride2);
	} else {
		pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
		pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
		pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);

		transfer_8to16copy(&data[0 * 64], pY_Cur, stride);
		transfer_8to16copy(&data[1 * 64], pY_Cur + 8, stride);
		transfer_8to16copy(&data[2 * 64], pY_Cur + next_block, stride);
		transfer_8to16copy(&data[3 * 64], pY_Cur + next_block + 8, stride);
		transfer_8to16copy(&data[4 * 64], pU_Cur, stride2);
		transfer_8to16copy(&data[5 * 64], pV_Cur, stride2);
	}
	stop_transfer_timer();

	/* XXX: rrv+interlacing is buggy */
	start_timer();
	pMB->field_dct = 0;
	if ((frame->global_flags & XVID_INTERLACING) &&
		(x_pos>0) && (x_pos<pParam->mb_width-1) &&
		(y_pos>0) && (y_pos<pParam->mb_height-1)) {
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	MBfDCT(data);

	for (i = 0; i < 6; i++) {
		const uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);

		start_timer();
		if (pParam->m_quant_type == H263_QUANT)
			quant_intra(&qcoeff[i * 64], &data[i * 64], iQuant, iDcScaler);
		else
			quant4_intra(&qcoeff[i * 64], &data[i * 64], iQuant, iDcScaler);
		stop_quant_timer();

		/* speedup: dont decode when encoding only ivops */
		if (pParam->iMaxKeyInterval != 1 || pParam->max_bframes > 0)
		{
			start_timer();
			if (pParam->m_quant_type == H263_QUANT)
				dequant_intra(&data[i * 64], &qcoeff[i * 64], iQuant, iDcScaler);
			else
				dequant4_intra(&data[i * 64], &qcoeff[i * 64], iQuant, iDcScaler);
			stop_iquant_timer();

			start_timer();
			idct(&data[i * 64]);
			stop_idct_timer();
		}
	}

	/* speedup: dont decode when encoding only ivops */
	if (pParam->iMaxKeyInterval != 1 || pParam->max_bframes > 0)
	{

		if (pMB->field_dct) {
			next_block = stride;
			stride *= 2;
		}

		start_timer();
		if ((frame->global_flags & XVID_REDUCED)) {
			copy_upsampled_8x8_16to8(pY_Cur, &data[0 * 64], stride);
			copy_upsampled_8x8_16to8(pY_Cur + 16, &data[1 * 64], stride);
			copy_upsampled_8x8_16to8(pY_Cur + next_block, &data[2 * 64], stride);
			copy_upsampled_8x8_16to8(pY_Cur + next_block + 16, &data[3 * 64], stride);
			copy_upsampled_8x8_16to8(pU_Cur, &data[4 * 64], stride2);
			copy_upsampled_8x8_16to8(pV_Cur, &data[5 * 64], stride2);
		} else {
			transfer_16to8copy(pY_Cur, &data[0 * 64], stride);
			transfer_16to8copy(pY_Cur + 8, &data[1 * 64], stride);
			transfer_16to8copy(pY_Cur + next_block, &data[2 * 64], stride);
			transfer_16to8copy(pY_Cur + next_block + 8, &data[3 * 64], stride);
			transfer_16to8copy(pU_Cur, &data[4 * 64], stride2);
			transfer_16to8copy(pV_Cur, &data[5 * 64], stride2);
		}
		stop_transfer_timer();
	}

}

uint8_t
MBTransQuantInter(const MBParam * const pParam,
				FRAMEINFO * const frame,
				MACROBLOCK * const pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				int16_t data[6 * 64],
				int16_t qcoeff[6 * 64])
{
	uint32_t stride = pParam->edged_width;
	const uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * ((frame->global_flags & XVID_REDUCED)?16:8);
	int i;
	const uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	int cbp = 0;
	uint32_t sum;
	const IMAGE * const pCurrent = &frame->image;

	if ((frame->global_flags & XVID_REDUCED)) {
		pY_Cur = pCurrent->y + (y_pos << 5) * stride + (x_pos << 5);
		pU_Cur = pCurrent->u + (y_pos << 4) * stride2 + (x_pos << 4);
		pV_Cur = pCurrent->v + (y_pos << 4) * stride2 + (x_pos << 4);
	} else {
		pY_Cur = pCurrent->y + (y_pos << 4) * stride + (x_pos << 4);
		pU_Cur = pCurrent->u + (y_pos << 3) * stride2 + (x_pos << 3);
		pV_Cur = pCurrent->v + (y_pos << 3) * stride2 + (x_pos << 3);
	}

	start_timer();
	pMB->field_dct = 0;
	if ((frame->global_flags & XVID_INTERLACING) &&
		(x_pos>0) && (x_pos<pParam->mb_width-1) &&
		(y_pos>0) && (y_pos<pParam->mb_height-1)) {
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	MBfDCT(data);

	for (i = 0; i < 6; i++) {
		const uint32_t limit = TOOSMALL_LIMIT + ((iQuant == 1) ? 1 : 0);
		/*
		 *  no need to transfer 8->16-bit
		 * (this is performed already in motion compensation)
		 */

		sum = QuantizeInterBlock(&qcoeff[i * 64], &data[i * 64], iQuant, pParam->m_quant_type);

		if (sum >= limit) {

			start_timer();
			if (pParam->m_quant_type == H263_QUANT)
				dequant_inter(&data[i * 64], &qcoeff[i * 64], iQuant);
			else
				dequant4_inter(&data[i * 64], &qcoeff[i * 64], iQuant);
			stop_iquant_timer();

			cbp |= 1 << (5 - i);

			start_timer();
			idct(&data[i * 64]);
			stop_idct_timer();
		}
	}

	if (pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if ((frame->global_flags & XVID_REDUCED)) {
		if (cbp & 32)
			add_upsampled_8x8_16to8(pY_Cur, &data[0 * 64], stride);
		if (cbp & 16)
			add_upsampled_8x8_16to8(pY_Cur + 16, &data[1 * 64], stride);
		if (cbp & 8)
			add_upsampled_8x8_16to8(pY_Cur + next_block, &data[2 * 64], stride);
		if (cbp & 4)
			add_upsampled_8x8_16to8(pY_Cur + 16 + next_block, &data[3 * 64], stride);
		if (cbp & 2)
			add_upsampled_8x8_16to8(pU_Cur, &data[4 * 64], stride2);
		if (cbp & 1)
			add_upsampled_8x8_16to8(pV_Cur, &data[5 * 64], stride2);
	} else {
		if (cbp & 32)
			transfer_16to8add(pY_Cur, &data[0 * 64], stride);
		if (cbp & 16)
			transfer_16to8add(pY_Cur + 8, &data[1 * 64], stride);
		if (cbp & 8)
			transfer_16to8add(pY_Cur + next_block, &data[2 * 64], stride);
		if (cbp & 4)
			transfer_16to8add(pY_Cur + next_block + 8, &data[3 * 64], stride);
		if (cbp & 2)
			transfer_16to8add(pU_Cur, &data[4 * 64], stride2);
		if (cbp & 1)
			transfer_16to8add(pV_Cur, &data[5 * 64], stride2);
	}
	stop_transfer_timer();

	return (uint8_t) cbp;
}

uint8_t
MBTransQuantInterBVOP(const MBParam * pParam,
				  FRAMEINFO * frame,
				  MACROBLOCK * pMB,
				  int16_t data[6 * 64],
				  int16_t qcoeff[6 * 64])
{
	int cbp = 0;
	int i;

/* there is no MBTrans for Inter block, that's done in motion compensation already */

	start_timer();
	pMB->field_dct = 0;
	if ((frame->global_flags & XVID_INTERLACING)) {
		pMB->field_dct = MBDecideFieldDCT(data);
	}
	stop_interlacing_timer();

	MBfDCT(data);

	for (i = 0; i < 6; i++) {
		int codedecision = 0;
		
		int sum = QuantizeInterBlock(&qcoeff[i * 64], &data[i * 64], pMB->quant, pParam->m_quant_type);
		
		if ((sum > 2) || (qcoeff[i*64+1] != 0) || (qcoeff[i*64+8] != 0) ) codedecision = 1;
		else {
			if (pMB->mode == MODE_DIRECT || pMB->mode == MODE_DIRECT_NO4V) {
				// dark blocks prevention for direct mode
				if ( (qcoeff[i*64] < -1) || (qcoeff[i*64] > 0) ) codedecision = 1;
			} else
				if (qcoeff[i*64] != 0) codedecision = 1; // not direct mode
		}

		if (codedecision) cbp |= 1 << (5 - i);
	}

/* we don't have to DeQuant, iDCT and Transfer back data for B-frames if we don't reconstruct this frame */
/* warning: reconstruction not supported yet */
	return (uint8_t) cbp;
}

/* permute block and return field dct choice */

static uint32_t
MBDecideFieldDCT(int16_t data[6 * 64])
{
	const uint32_t field = MBFieldTest(data);
	if (field) MBFrameToField(data);

	return field;
}

/* if sum(diff between field lines) < sum(diff between frame lines), use field dct */

uint32_t
MBFieldTest_c(int16_t data[6 * 64])
{
	const uint8_t blocks[] =
		{ 0 * 64, 0 * 64, 0 * 64, 0 * 64, 2 * 64, 2 * 64, 2 * 64, 2 * 64 };
	const uint8_t lines[] = { 0, 16, 32, 48, 0, 16, 32, 48 };

	int frame = 0, field = 0;
	int i, j;

	for (i = 0; i < 7; ++i) {
		for (j = 0; j < 8; ++j) {
			frame +=
				ABS(data[0 * 64 + (i + 1) * 8 + j] - data[0 * 64 + i * 8 + j]);
			frame +=
				ABS(data[1 * 64 + (i + 1) * 8 + j] - data[1 * 64 + i * 8 + j]);
			frame +=
				ABS(data[2 * 64 + (i + 1) * 8 + j] - data[2 * 64 + i * 8 + j]);
			frame +=
				ABS(data[3 * 64 + (i + 1) * 8 + j] - data[3 * 64 + i * 8 + j]);

			field +=
				ABS(data[blocks[i + 1] + lines[i + 1] + j] -
					data[blocks[i] + lines[i] + j]);
			field +=
				ABS(data[blocks[i + 1] + lines[i + 1] + 8 + j] -
					data[blocks[i] + lines[i] + 8 + j]);
			field +=
				ABS(data[blocks[i + 1] + 64 + lines[i + 1] + j] -
					data[blocks[i] + 64 + lines[i] + j]);
			field +=
				ABS(data[blocks[i + 1] + 64 + lines[i + 1] + 8 + j] -
					data[blocks[i] + 64 + lines[i] + 8 + j]);
		}
	}

	return (frame >= (field + 350));
}


/* deinterlace Y blocks vertically */

#define MOVLINE(X,Y) memcpy(X, Y, sizeof(tmp))
#define LINE(X,Y)    &data[X*64 + Y*8]

void
MBFrameToField(int16_t data[6 * 64])
{
	int16_t tmp[8];

	/* left blocks */

	// 1=2, 2=4, 4=8, 8=1
	MOVLINE(tmp, LINE(0, 1));
	MOVLINE(LINE(0, 1), LINE(0, 2));
	MOVLINE(LINE(0, 2), LINE(0, 4));
	MOVLINE(LINE(0, 4), LINE(2, 0));
	MOVLINE(LINE(2, 0), tmp);

	// 3=6, 6=12, 12=9, 9=3
	MOVLINE(tmp, LINE(0, 3));
	MOVLINE(LINE(0, 3), LINE(0, 6));
	MOVLINE(LINE(0, 6), LINE(2, 4));
	MOVLINE(LINE(2, 4), LINE(2, 1));
	MOVLINE(LINE(2, 1), tmp);

	// 5=10, 10=5
	MOVLINE(tmp, LINE(0, 5));
	MOVLINE(LINE(0, 5), LINE(2, 2));
	MOVLINE(LINE(2, 2), tmp);

	// 7=14, 14=13, 13=11, 11=7
	MOVLINE(tmp, LINE(0, 7));
	MOVLINE(LINE(0, 7), LINE(2, 6));
	MOVLINE(LINE(2, 6), LINE(2, 5));
	MOVLINE(LINE(2, 5), LINE(2, 3));
	MOVLINE(LINE(2, 3), tmp);

	/* right blocks */

	// 1=2, 2=4, 4=8, 8=1
	MOVLINE(tmp, LINE(1, 1));
	MOVLINE(LINE(1, 1), LINE(1, 2));
	MOVLINE(LINE(1, 2), LINE(1, 4));
	MOVLINE(LINE(1, 4), LINE(3, 0));
	MOVLINE(LINE(3, 0), tmp);

	// 3=6, 6=12, 12=9, 9=3
	MOVLINE(tmp, LINE(1, 3));
	MOVLINE(LINE(1, 3), LINE(1, 6));
	MOVLINE(LINE(1, 6), LINE(3, 4));
	MOVLINE(LINE(3, 4), LINE(3, 1));
	MOVLINE(LINE(3, 1), tmp);

	// 5=10, 10=5
	MOVLINE(tmp, LINE(1, 5));
	MOVLINE(LINE(1, 5), LINE(3, 2));
	MOVLINE(LINE(3, 2), tmp);

	// 7=14, 14=13, 13=11, 11=7
	MOVLINE(tmp, LINE(1, 7));
	MOVLINE(LINE(1, 7), LINE(3, 6));
	MOVLINE(LINE(3, 6), LINE(3, 5));
	MOVLINE(LINE(3, 5), LINE(3, 3));
	MOVLINE(LINE(3, 3), tmp);
}
