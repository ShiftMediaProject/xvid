/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  Decoder main module  -
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
 *************************************************************************/

/**************************************************************************
 *
 *  History:
 *
 *	22.06.2002	added primative N_VOP support
 *				#define BFRAMES_DEC now enables Minchenm's bframe decoder
 *  08.05.2002  add low_delay support for B_VOP decode
 *              MinChen <chenm001@163.com>
 *  05.05.2002  fix some B-frame decode problem
 *  02.05.2002  add B-frame decode support(have some problem);
 *              MinChen <chenm001@163.com>
 *  22.04.2002  add some B-frame decode support;  chenm001 <chenm001@163.com>
 *  29.03.2002  interlacing fix - compensated block wasn't being used when
 *              reconstructing blocks, thus artifacts
 *              interlacing speedup - used transfers to re-interlace
 *              interlaced decoding should be as fast as progressive now
 *  26.03.2002  interlacing support - moved transfers outside decode loop
 *  26.12.2001  decoder_mbinter: dequant/idct moved within if(coded) block
 *  22.12.2001  lock based interpolation
 *  01.12.2001  inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *  $Id: decoder.c,v 1.21 2002-06-22 07:23:09 suxen_drol Exp $
 *
 *************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "xvid.h"
#include "portab.h"

#include "decoder.h"
#include "bitstream/bitstream.h"
#include "bitstream/mbcoding.h"

#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "utils/mem_transfer.h"
#include "image/interpolate8x8.h"

#include "bitstream/mbcoding.h"
#include "prediction/mbprediction.h"
#include "utils/timer.h"
#include "utils/emms.h"

#include "image/image.h"
#include "image/colorspace.h"
#include "utils/mem_align.h"

int
decoder_create(XVID_DEC_PARAM * param)
{
	DECODER *dec;

	dec = xvid_malloc(sizeof(DECODER), CACHE_LINE);
	if (dec == NULL) {
		return XVID_ERR_MEMORY;
	}
	param->handle = dec;

	dec->width = param->width;
	dec->height = param->height;

	dec->mb_width = (dec->width + 15) / 16;
	dec->mb_height = (dec->height + 15) / 16;

	dec->edged_width = 16 * dec->mb_width + 2 * EDGE_SIZE;
	dec->edged_height = 16 * dec->mb_height + 2 * EDGE_SIZE;
	dec->low_delay = 0;

	if (image_create(&dec->cur, dec->edged_width, dec->edged_height)) {
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	if (image_create(&dec->refn[0], dec->edged_width, dec->edged_height)) {
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}
	// add by chenm001 <chenm001@163.com>
	// for support B-frame to reference last 2 frame
	if (image_create(&dec->refn[1], dec->edged_width, dec->edged_height)) {
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[0], dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}
	if (image_create(&dec->refn[2], dec->edged_width, dec->edged_height)) {
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[0], dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[1], dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	dec->mbs =
		xvid_malloc(sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height,
					CACHE_LINE);
	if (dec->mbs == NULL) {
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[0], dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[1], dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[2], dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	memset(dec->mbs, 0, sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height);

	// add by chenm001 <chenm001@163.com>
	// for skip MB flag
	dec->last_mbs =
		xvid_malloc(sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height,
					CACHE_LINE);
	if (dec->last_mbs == NULL) {
		xvid_free(dec->mbs);
		image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[0], dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[1], dec->edged_width, dec->edged_height);
		image_destroy(&dec->refn[2], dec->edged_width, dec->edged_height);
		xvid_free(dec);
		return XVID_ERR_MEMORY;
	}

	memset(dec->last_mbs, 0, sizeof(MACROBLOCK) * dec->mb_width * dec->mb_height);

	init_timer();

	// add by chenm001 <chenm001@163.com>
	// for support B-frame to save reference frame's time
	dec->frames = -1;
	dec->time = dec->time_base = dec->last_time_base = 0;

	return XVID_ERR_OK;
}


int
decoder_destroy(DECODER * dec)
{
	xvid_free(dec->last_mbs);
	xvid_free(dec->mbs);
	image_destroy(&dec->refn[0], dec->edged_width, dec->edged_height);
	image_destroy(&dec->refn[1], dec->edged_width, dec->edged_height);
	image_destroy(&dec->refn[2], dec->edged_width, dec->edged_height);
	image_destroy(&dec->cur, dec->edged_width, dec->edged_height);
	xvid_free(dec);

	write_timer();
	return XVID_ERR_OK;
}



static const int32_t dquant_table[4] = {
	-1, -2, 1, 2
};




// decode an intra macroblock

void
decoder_mbintra(DECODER * dec,
				MACROBLOCK * pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				const uint32_t acpred_flag,
				const uint32_t cbp,
				Bitstream * bs,
				const uint32_t quant,
				const uint32_t intra_dc_threshold)
{

	DECLARE_ALIGNED_MATRIX(block, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data, 6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	memset(block, 0, 6 * 64 * sizeof(int16_t));	// clear

	for (i = 0; i < 6; i++) {
		uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);
		int16_t predictors[8];
		int start_coeff;

		start_timer();
		predict_acdc(dec->mbs, x_pos, y_pos, dec->mb_width, i, &block[i * 64],
					 iQuant, iDcScaler, predictors);
		if (!acpred_flag) {
			pMB->acpred_directions[i] = 0;
		}
		stop_prediction_timer();

		if (quant < intra_dc_threshold) {
			int dc_size;
			int dc_dif;

			dc_size = i < 4 ? get_dc_size_lum(bs) : get_dc_size_chrom(bs);
			dc_dif = dc_size ? get_dc_dif(bs, dc_size) : 0;

			if (dc_size > 8) {
				BitstreamSkip(bs, 1);	// marker
			}

			block[i * 64 + 0] = dc_dif;
			start_coeff = 1;
		} else {
			start_coeff = 0;
		}

		start_timer();
		if (cbp & (1 << (5 - i)))	// coded
		{
			get_intra_block(bs, &block[i * 64], pMB->acpred_directions[i],
							start_coeff);
		}
		stop_coding_timer();

		start_timer();
		add_acdc(pMB, i, &block[i * 64], iDcScaler, predictors);
		stop_prediction_timer();

		start_timer();
		if (dec->quant_type == 0) {
			dequant_intra(&data[i * 64], &block[i * 64], iQuant, iDcScaler);
		} else {
			dequant4_intra(&data[i * 64], &block[i * 64], iQuant, iDcScaler);
		}
		stop_iquant_timer();

		start_timer();
		idct(&data[i * 64]);
		stop_idct_timer();
	}

	if (dec->interlacing && pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	transfer_16to8copy(pY_Cur, &data[0 * 64], stride);
	transfer_16to8copy(pY_Cur + 8, &data[1 * 64], stride);
	transfer_16to8copy(pY_Cur + next_block, &data[2 * 64], stride);
	transfer_16to8copy(pY_Cur + 8 + next_block, &data[3 * 64], stride);
	transfer_16to8copy(pU_Cur, &data[4 * 64], stride2);
	transfer_16to8copy(pV_Cur, &data[5 * 64], stride2);
	stop_transfer_timer();
}





#define SIGN(X) (((X)>0)?1:-1)
#define ABS(X) (((X)>0)?(X):-(X))
static const uint32_t roundtab[16] =
	{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };


// decode an inter macroblock

void
decoder_mbinter(DECODER * dec,
				const MACROBLOCK * pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				const uint32_t acpred_flag,
				const uint32_t cbp,
				Bitstream * bs,
				const uint32_t quant,
				const uint32_t rounding)
{

	DECLARE_ALIGNED_MATRIX(block, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data, 6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	int uv_dx, uv_dy;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q) {
		uv_dx = pMB->mvs[0].x;
		uv_dy = pMB->mvs[0].y;

		uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;
	} else {
		int sum;

		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
		uv_dx =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;
		uv_dy =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));
	}

	start_timer();
	interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16 * x_pos, 16 * y_pos,
						  pMB->mvs[0].x, pMB->mvs[0].y, stride, rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16 * x_pos + 8,
						  16 * y_pos, pMB->mvs[1].x, pMB->mvs[1].y, stride,
						  rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16 * x_pos,
						  16 * y_pos + 8, pMB->mvs[2].x, pMB->mvs[2].y, stride,
						  rounding);
	interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16 * x_pos + 8,
						  16 * y_pos + 8, pMB->mvs[3].x, pMB->mvs[3].y, stride,
						  rounding);
	interpolate8x8_switch(dec->cur.u, dec->refn[0].u, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, rounding);
	interpolate8x8_switch(dec->cur.v, dec->refn[0].v, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, rounding);
	stop_comp_timer();

	for (i = 0; i < 6; i++) {
		if (cbp & (1 << (5 - i)))	// coded
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	// clear

			start_timer();
			get_inter_block(bs, &block[i * 64]);
			stop_coding_timer();

			start_timer();
			if (dec->quant_type == 0) {
				dequant_inter(&data[i * 64], &block[i * 64], iQuant);
			} else {
				dequant4_inter(&data[i * 64], &block[i * 64], iQuant);
			}
			stop_iquant_timer();

			start_timer();
			idct(&data[i * 64]);
			stop_idct_timer();
		}
	}

	if (dec->interlacing && pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if (cbp & 32)
		transfer_16to8add(pY_Cur, &data[0 * 64], stride);
	if (cbp & 16)
		transfer_16to8add(pY_Cur + 8, &data[1 * 64], stride);
	if (cbp & 8)
		transfer_16to8add(pY_Cur + next_block, &data[2 * 64], stride);
	if (cbp & 4)
		transfer_16to8add(pY_Cur + 8 + next_block, &data[3 * 64], stride);
	if (cbp & 2)
		transfer_16to8add(pU_Cur, &data[4 * 64], stride2);
	if (cbp & 1)
		transfer_16to8add(pV_Cur, &data[5 * 64], stride2);
	stop_transfer_timer();
}


void
decoder_iframe(DECODER * dec,
			   Bitstream * bs,
			   int quant,
			   int intra_dc_threshold)
{

	uint32_t x, y;

	for (y = 0; y < dec->mb_height; y++) {
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb = &dec->mbs[y * dec->mb_width + x];

			uint32_t mcbpc;
			uint32_t cbpc;
			uint32_t acpred_flag;
			uint32_t cbpy;
			uint32_t cbp;

			mcbpc = get_mcbpc_intra(bs);
			mb->mode = mcbpc & 7;
			cbpc = (mcbpc >> 4);

			acpred_flag = BitstreamGetBit(bs);

			if (mb->mode == MODE_STUFFING) {
				DEBUG("-- STUFFING ?");
				continue;
			}

			cbpy = get_cbpy(bs, 1);
			cbp = (cbpy << 2) | cbpc;

			if (mb->mode == MODE_INTRA_Q) {
				quant += dquant_table[BitstreamGetBits(bs, 2)];
				if (quant > 31) {
					quant = 31;
				} else if (quant < 1) {
					quant = 1;
				}
			}
			mb->quant = quant;

			if (dec->interlacing) {
				mb->field_dct = BitstreamGetBit(bs);
				DEBUG1("deci: field_dct: ", mb->field_dct);
			}

			decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant,
							intra_dc_threshold);
		}
	}

}


void
get_motion_vector(DECODER * dec,
				  Bitstream * bs,
				  int x,
				  int y,
				  int k,
				  VECTOR * mv,
				  int fcode)
{

	int scale_fac = 1 << (fcode - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	VECTOR pmv[4];
	int32_t psad[4];

	int mv_x, mv_y;
	int pmv_x, pmv_y;


	get_pmvdata(dec->mbs, x, y, dec->mb_width, k, pmv, psad);

	pmv_x = pmv[0].x;
	pmv_y = pmv[0].y;

	mv_x = get_mv(bs, fcode);
	mv_y = get_mv(bs, fcode);

	mv_x += pmv_x;
	mv_y += pmv_y;

	if (mv_x < low) {
		mv_x += range;
	} else if (mv_x > high) {
		mv_x -= range;
	}

	if (mv_y < low) {
		mv_y += range;
	} else if (mv_y > high) {
		mv_y -= range;
	}

	mv->x = mv_x;
	mv->y = mv_y;

}


void
decoder_pframe(DECODER * dec,
			   Bitstream * bs,
			   int rounding,
			   int quant,
			   int fcode,
			   int intra_dc_threshold)
{

	uint32_t x, y;

	start_timer();
	image_setedges(&dec->refn[0], dec->edged_width, dec->edged_height,
				   dec->width, dec->height, dec->interlacing);
	stop_edges_timer();

	for (y = 0; y < dec->mb_height; y++) {
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb = &dec->mbs[y * dec->mb_width + x];

			//if (!(dec->mb_skip[y*dec->mb_width + x]=BitstreamGetBit(bs)))         // not_coded
			if (!(BitstreamGetBit(bs)))	// not_coded
			{
				uint32_t mcbpc;
				uint32_t cbpc;
				uint32_t acpred_flag;
				uint32_t cbpy;
				uint32_t cbp;
				uint32_t intra;

				mcbpc = get_mcbpc_inter(bs);
				mb->mode = mcbpc & 7;
				cbpc = (mcbpc >> 4);
				acpred_flag = 0;

				intra = (mb->mode == MODE_INTRA || mb->mode == MODE_INTRA_Q);

				if (intra) {
					acpred_flag = BitstreamGetBit(bs);
				}

				if (mb->mode == MODE_STUFFING) {
					DEBUG("-- STUFFING ?");
					continue;
				}

				cbpy = get_cbpy(bs, intra);
				cbp = (cbpy << 2) | cbpc;

				if (mb->mode == MODE_INTER_Q || mb->mode == MODE_INTRA_Q) {
					quant += dquant_table[BitstreamGetBits(bs, 2)];
					if (quant > 31) {
						quant = 31;
					} else if (mb->quant < 1) {
						quant = 1;
					}
				}
				mb->quant = quant;

				if (dec->interlacing) {
					mb->field_dct = BitstreamGetBit(bs);
					DEBUG1("decp: field_dct: ", mb->field_dct);

					if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) {
						mb->field_pred = BitstreamGetBit(bs);
						DEBUG1("decp: field_pred: ", mb->field_pred);

						if (mb->field_pred) {
							mb->field_for_top = BitstreamGetBit(bs);
							DEBUG1("decp: field_for_top: ", mb->field_for_top);
							mb->field_for_bot = BitstreamGetBit(bs);
							DEBUG1("decp: field_for_bot: ", mb->field_for_bot);
						}
					}
				}

				if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) {
					if (dec->interlacing && mb->field_pred) {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0],
										  fcode);
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[1],
										  fcode);
					} else {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0],
										  fcode);
						mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
							mb->mvs[0].x;
						mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
							mb->mvs[0].y;
					}
				} else if (mb->mode ==
						   MODE_INTER4V /* || mb->mode == MODE_INTER4V_Q */ ) {
					get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode);
					get_motion_vector(dec, bs, x, y, 1, &mb->mvs[1], fcode);
					get_motion_vector(dec, bs, x, y, 2, &mb->mvs[2], fcode);
					get_motion_vector(dec, bs, x, y, 3, &mb->mvs[3], fcode);
				} else			// MODE_INTRA, MODE_INTRA_Q
				{
					mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
						0;
					mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
						0;
					decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant,
									intra_dc_threshold);
					continue;
				}

				decoder_mbinter(dec, mb, x, y, acpred_flag, cbp, bs, quant,
								rounding);
			} else				// not coded
			{
				//DEBUG2("P-frame MB at (X,Y)=",x,y);
				mb->mode = MODE_NOT_CODED;
				mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
				mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = 0;

				// copy macroblock directly from ref to cur

				start_timer();

				transfer8x8_copy(dec->cur.y + (16 * y) * dec->edged_width +
								 (16 * x),
								 dec->refn[0].y + (16 * y) * dec->edged_width +
								 (16 * x), dec->edged_width);

				transfer8x8_copy(dec->cur.y + (16 * y) * dec->edged_width +
								 (16 * x + 8),
								 dec->refn[0].y + (16 * y) * dec->edged_width +
								 (16 * x + 8), dec->edged_width);

				transfer8x8_copy(dec->cur.y + (16 * y + 8) * dec->edged_width +
								 (16 * x),
								 dec->refn[0].y + (16 * y +
												   8) * dec->edged_width +
								 (16 * x), dec->edged_width);

				transfer8x8_copy(dec->cur.y + (16 * y + 8) * dec->edged_width +
								 (16 * x + 8),
								 dec->refn[0].y + (16 * y +
												   8) * dec->edged_width +
								 (16 * x + 8), dec->edged_width);

				transfer8x8_copy(dec->cur.u + (8 * y) * dec->edged_width / 2 +
								 (8 * x),
								 dec->refn[0].u +
								 (8 * y) * dec->edged_width / 2 + (8 * x),
								 dec->edged_width / 2);

				transfer8x8_copy(dec->cur.v + (8 * y) * dec->edged_width / 2 +
								 (8 * x),
								 dec->refn[0].v +
								 (8 * y) * dec->edged_width / 2 + (8 * x),
								 dec->edged_width / 2);

				stop_transfer_timer();
			}
		}
	}
}


// add by MinChen <chenm001@163.com>
// decode B-frame motion vector
void
get_b_motion_vector(DECODER * dec,
					Bitstream * bs,
					int x,
					int y,
					VECTOR * mv,
					int fcode,
					const VECTOR pmv)
{
	int scale_fac = 1 << (fcode - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	int mv_x, mv_y;
	int pmv_x, pmv_y;

	pmv_x = pmv.x;
	pmv_y = pmv.y;

	mv_x = get_mv(bs, fcode);
	mv_y = get_mv(bs, fcode);

	mv_x += pmv_x;
	mv_y += pmv_y;

	if (mv_x < low) {
		mv_x += range;
	} else if (mv_x > high) {
		mv_x -= range;
	}

	if (mv_y < low) {
		mv_y += range;
	} else if (mv_y > high) {
		mv_y -= range;
	}

	mv->x = mv_x;
	mv->y = mv_y;
}


// add by MinChen <chenm001@163.com>
// decode an B-frame forward & backward inter macroblock
void
decoder_bf_mbinter(DECODER * dec,
				   const MACROBLOCK * pMB,
				   const uint32_t x_pos,
				   const uint32_t y_pos,
				   const uint32_t cbp,
				   Bitstream * bs,
				   const uint32_t quant,
				   const uint8_t ref)
{

	DECLARE_ALIGNED_MATRIX(block, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data, 6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t i;
	uint32_t iQuant = pMB->quant;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;
	int uv_dx, uv_dy;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	if (!(pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)) {
		uv_dx = pMB->mvs[0].x;
		uv_dy = pMB->mvs[0].y;

		uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;
	} else {
		int sum;

		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
		uv_dx =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;
		uv_dy =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));
	}

	start_timer();
	interpolate8x8_switch(dec->cur.y, dec->refn[ref].y, 16 * x_pos, 16 * y_pos,
						  pMB->mvs[0].x, pMB->mvs[0].y, stride, 0);
	interpolate8x8_switch(dec->cur.y, dec->refn[ref].y, 16 * x_pos + 8,
						  16 * y_pos, pMB->mvs[1].x, pMB->mvs[1].y, stride, 0);
	interpolate8x8_switch(dec->cur.y, dec->refn[ref].y, 16 * x_pos,
						  16 * y_pos + 8, pMB->mvs[2].x, pMB->mvs[2].y, stride,
						  0);
	interpolate8x8_switch(dec->cur.y, dec->refn[ref].y, 16 * x_pos + 8,
						  16 * y_pos + 8, pMB->mvs[3].x, pMB->mvs[3].y, stride,
						  0);
	interpolate8x8_switch(dec->cur.u, dec->refn[ref].u, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, 0);
	interpolate8x8_switch(dec->cur.v, dec->refn[ref].v, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, 0);
	stop_comp_timer();

	for (i = 0; i < 6; i++) {
		if (cbp & (1 << (5 - i)))	// coded
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	// clear

			start_timer();
			get_inter_block(bs, &block[i * 64]);
			stop_coding_timer();

			start_timer();
			if (dec->quant_type == 0) {
				dequant_inter(&data[i * 64], &block[i * 64], iQuant);
			} else {
				dequant4_inter(&data[i * 64], &block[i * 64], iQuant);
			}
			stop_iquant_timer();

			start_timer();
			idct(&data[i * 64]);
			stop_idct_timer();
		}
	}

	if (dec->interlacing && pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if (cbp & 32)
		transfer_16to8add(pY_Cur, &data[0 * 64], stride);
	if (cbp & 16)
		transfer_16to8add(pY_Cur + 8, &data[1 * 64], stride);
	if (cbp & 8)
		transfer_16to8add(pY_Cur + next_block, &data[2 * 64], stride);
	if (cbp & 4)
		transfer_16to8add(pY_Cur + 8 + next_block, &data[3 * 64], stride);
	if (cbp & 2)
		transfer_16to8add(pU_Cur, &data[4 * 64], stride2);
	if (cbp & 1)
		transfer_16to8add(pV_Cur, &data[5 * 64], stride2);
	stop_transfer_timer();
}


// add by MinChen <chenm001@163.com>
// decode an B-frame direct &  inter macroblock
void
decoder_bf_interpolate_mbinter(DECODER * dec,
							   IMAGE forward,
							   IMAGE backward,
							   const MACROBLOCK * pMB,
							   const uint32_t x_pos,
							   const uint32_t y_pos,
							   const uint32_t cbp,
							   Bitstream * bs)
{

	DECLARE_ALIGNED_MATRIX(block, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(data, 6, 64, int16_t, CACHE_LINE);

	uint32_t stride = dec->edged_width;
	uint32_t stride2 = stride / 2;
	uint32_t next_block = stride * 8;
	uint32_t iQuant = pMB->quant;
	int uv_dx, uv_dy;
	int b_uv_dx, b_uv_dy;
	uint32_t i;
	uint8_t *pY_Cur, *pU_Cur, *pV_Cur;

	pY_Cur = dec->cur.y + (y_pos << 4) * stride + (x_pos << 4);
	pU_Cur = dec->cur.u + (y_pos << 3) * stride2 + (x_pos << 3);
	pV_Cur = dec->cur.v + (y_pos << 3) * stride2 + (x_pos << 3);

	if ((pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)) {
		uv_dx = pMB->mvs[0].x;
		uv_dy = pMB->mvs[0].y;

		uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;

		b_uv_dx = pMB->b_mvs[0].x;
		b_uv_dy = pMB->b_mvs[0].y;

		b_uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		b_uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;
	} else {
		int sum;

		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;
		uv_dx =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;
		uv_dy =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));

		sum =
			pMB->b_mvs[0].x + pMB->b_mvs[1].x + pMB->b_mvs[2].x +
			pMB->b_mvs[3].x;
		b_uv_dx =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));

		sum =
			pMB->b_mvs[0].y + pMB->b_mvs[1].y + pMB->b_mvs[2].y +
			pMB->b_mvs[3].y;
		b_uv_dy =
			(sum ==
			 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] +
								  (ABS(sum) / 16) * 2));
	}


	start_timer();
	interpolate8x8_switch(dec->cur.y, forward.y, 16 * x_pos, 16 * y_pos,
						  pMB->mvs[0].x, pMB->mvs[0].y, stride, 0);
	interpolate8x8_switch(dec->cur.y, forward.y, 16 * x_pos + 8, 16 * y_pos,
						  pMB->mvs[1].x, pMB->mvs[1].y, stride, 0);
	interpolate8x8_switch(dec->cur.y, forward.y, 16 * x_pos, 16 * y_pos + 8,
						  pMB->mvs[2].x, pMB->mvs[2].y, stride, 0);
	interpolate8x8_switch(dec->cur.y, forward.y, 16 * x_pos + 8,
						  16 * y_pos + 8, pMB->mvs[3].x, pMB->mvs[3].y, stride,
						  0);
	interpolate8x8_switch(dec->cur.u, forward.u, 8 * x_pos, 8 * y_pos, uv_dx,
						  uv_dy, stride2, 0);
	interpolate8x8_switch(dec->cur.v, forward.v, 8 * x_pos, 8 * y_pos, uv_dx,
						  uv_dy, stride2, 0);


	interpolate8x8_switch(dec->refn[2].y, backward.y, 16 * x_pos, 16 * y_pos,
						  pMB->b_mvs[0].x, pMB->b_mvs[0].y, stride, 0);
	interpolate8x8_switch(dec->refn[2].y, backward.y, 16 * x_pos + 8,
						  16 * y_pos, pMB->b_mvs[1].x, pMB->b_mvs[1].y, stride,
						  0);
	interpolate8x8_switch(dec->refn[2].y, backward.y, 16 * x_pos,
						  16 * y_pos + 8, pMB->b_mvs[2].x, pMB->b_mvs[2].y,
						  stride, 0);
	interpolate8x8_switch(dec->refn[2].y, backward.y, 16 * x_pos + 8,
						  16 * y_pos + 8, pMB->b_mvs[3].x, pMB->b_mvs[3].y,
						  stride, 0);
	interpolate8x8_switch(dec->refn[2].u, backward.u, 8 * x_pos, 8 * y_pos,
						  b_uv_dx, b_uv_dy, stride2, 0);
	interpolate8x8_switch(dec->refn[2].v, backward.v, 8 * x_pos, 8 * y_pos,
						  b_uv_dx, b_uv_dy, stride2, 0);

	interpolate8x8_c(dec->cur.y, dec->refn[2].y, 16 * x_pos, 16 * y_pos,
					 stride);
	interpolate8x8_c(dec->cur.y, dec->refn[2].y, 16 * x_pos + 8, 16 * y_pos,
					 stride);
	interpolate8x8_c(dec->cur.y, dec->refn[2].y, 16 * x_pos, 16 * y_pos + 8,
					 stride);
	interpolate8x8_c(dec->cur.y, dec->refn[2].y, 16 * x_pos + 8,
					 16 * y_pos + 8, stride);
	interpolate8x8_c(dec->cur.u, dec->refn[2].u, 8 * x_pos, 8 * y_pos,
					 stride2);
	interpolate8x8_c(dec->cur.v, dec->refn[2].v, 8 * x_pos, 8 * y_pos,
					 stride2);

	stop_comp_timer();

	for (i = 0; i < 6; i++) {
		if (cbp & (1 << (5 - i)))	// coded
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	// clear

			start_timer();
			get_inter_block(bs, &block[i * 64]);
			stop_coding_timer();

			start_timer();
			if (dec->quant_type == 0) {
				dequant_inter(&data[i * 64], &block[i * 64], iQuant);
			} else {
				dequant4_inter(&data[i * 64], &block[i * 64], iQuant);
			}
			stop_iquant_timer();

			start_timer();
			idct(&data[i * 64]);
			stop_idct_timer();
		}
	}

	if (dec->interlacing && pMB->field_dct) {
		next_block = stride;
		stride *= 2;
	}

	start_timer();
	if (cbp & 32)
		transfer_16to8add(pY_Cur, &data[0 * 64], stride);
	if (cbp & 16)
		transfer_16to8add(pY_Cur + 8, &data[1 * 64], stride);
	if (cbp & 8)
		transfer_16to8add(pY_Cur + next_block, &data[2 * 64], stride);
	if (cbp & 4)
		transfer_16to8add(pY_Cur + 8 + next_block, &data[3 * 64], stride);
	if (cbp & 2)
		transfer_16to8add(pU_Cur, &data[4 * 64], stride2);
	if (cbp & 1)
		transfer_16to8add(pV_Cur, &data[5 * 64], stride2);
	stop_transfer_timer();
}


// add by MinChen <chenm001@163.com>
// for decode B-frame dbquant
int32_t __inline
get_dbquant(Bitstream * bs)
{
	if (!BitstreamGetBit(bs))	// '0'
		return (0);
	else if (!BitstreamGetBit(bs))	// '10'
		return (-2);
	else
		return (2);				// '11'
}

// add by MinChen <chenm001@163.com>
// for decode B-frame mb_type
// bit   ret_value
// 1        0
// 01       1
// 001      2
// 0001     3
int32_t __inline
get_mbtype(Bitstream * bs)
{
	int32_t mb_type;

	for (mb_type = 0; mb_type <= 3; mb_type++) {
		if (BitstreamGetBit(bs))
			break;
	}

	if (mb_type <= 3)
		return (mb_type);
	else
		return (-1);
}

void
decoder_bframe(DECODER * dec,
			   Bitstream * bs,
			   int quant,
			   int fcode_forward,
			   int fcode_backward)
{

	uint32_t x, y;
	VECTOR mv, zeromv;

	start_timer();
	image_setedges(&dec->refn[0], dec->edged_width, dec->edged_height,
				   dec->width, dec->height, dec->interlacing);
	//image_setedges(&dec->refn[1], dec->edged_width, dec->edged_height, dec->width, dec->height, dec->interlacing);
	stop_edges_timer();


	for (y = 0; y < dec->mb_height; y++) {
		// Initialize Pred Motion Vector
		dec->p_fmv.x = dec->p_fmv.y = dec->p_bmv.x = dec->p_bmv.y = 0;
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb = &dec->mbs[y * dec->mb_width + x];
			MACROBLOCK *last_mb = &dec->last_mbs[y * dec->mb_width + x];

			mb->mvs[0].x = mb->mvs[0].y = zeromv.x = zeromv.y = mv.x = mv.y =
				0;

			// the last P_VOP is skip macroblock ?
			if (last_mb->mode == MODE_NOT_CODED) {
				//DEBUG2("Skip MB in B-frame at (X,Y)=!",x,y);
				mb->mb_type = MODE_FORWARD;
				mb->cbp = 0;
				mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = mb->mvs[0].x;
				mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = mb->mvs[0].y;
				mb->quant = 8;

				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, quant, 1);
				continue;
			}
			//t=BitstreamShowBits(bs,32);

			if (!BitstreamGetBit(bs)) {	// modb=='0'
				const uint8_t modb2 = BitstreamGetBit(bs);

				mb->mb_type = get_mbtype(bs);

				if (!modb2) {	// modb=='00'
					mb->cbp = BitstreamGetBits(bs, 6);
				} else {
					mb->cbp = 0;
				}
				if (mb->mb_type && mb->cbp) {
					quant += get_dbquant(bs);

					if (quant > 31) {
						quant = 31;
					} else if (mb->quant < 1) {
						quant = 1;
					}
				} else {
					quant = 8;
				}
				mb->quant = quant;
			} else {
				mb->mb_type = MODE_DIRECT_NONE_MV;
				mb->cbp = 0;
			}

			mb->mode = MODE_INTER;
			//DEBUG1("Switch bm_type=",mb->mb_type);

			switch (mb->mb_type) {
			case MODE_DIRECT:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], 1, zeromv);

			case MODE_DIRECT_NONE_MV:
				{				// Because this file is a C file not C++ so I use '{' to define var
					const int64_t TRB = dec->time_pp - dec->time_bp, TRD =
						dec->time_pp;
					int i;

					for (i = 0; i < 4; i++) {
						mb->mvs[i].x =
							(int32_t) ((TRB * last_mb->mvs[i].x) / TRD +
									   mb->mvs[0].x);
						mb->b_mvs[i].x =
							(int32_t) ((mb->mvs[0].x ==
										0) ? ((TRB -
											   TRD) * last_mb->mvs[i].x) /
									   TRD : mb->mvs[i].x - last_mb->mvs[i].x);
						mb->mvs[i].y =
							(int32_t) ((TRB * last_mb->mvs[i].y) / TRD +
									   mb->mvs[0].y);
						mb->b_mvs[i].y =
							(int32_t) ((mb->mvs[0].y ==
										0) ? ((TRB -
											   TRD) * last_mb->mvs[i].y) /
									   TRD : mb->mvs[i].y - last_mb->mvs[i].y);
					}
					//DEBUG("B-frame Direct!\n");
				}
				mb->mode = MODE_INTER4V;
				decoder_bf_interpolate_mbinter(dec, dec->refn[1], dec->refn[0],
											   mb, x, y, mb->cbp, bs);
				break;

			case MODE_INTERPOLATE:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_forward,
									dec->p_fmv);
				dec->p_fmv.x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
					mb->mvs[0].x;
				dec->p_fmv.y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
					mb->mvs[0].y;

				get_b_motion_vector(dec, bs, x, y, &mb->b_mvs[0],
									fcode_backward, dec->p_bmv);
				dec->p_bmv.x = mb->b_mvs[1].x = mb->b_mvs[2].x =
					mb->b_mvs[3].x = mb->b_mvs[0].x;
				dec->p_bmv.y = mb->b_mvs[1].y = mb->b_mvs[2].y =
					mb->b_mvs[3].y = mb->b_mvs[0].y;

				decoder_bf_interpolate_mbinter(dec, dec->refn[1], dec->refn[0],
											   mb, x, y, mb->cbp, bs);
				//DEBUG("B-frame Bidir!\n");
				break;

			case MODE_BACKWARD:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_backward,
									dec->p_bmv);
				dec->p_bmv.x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
					mb->mvs[0].x;
				dec->p_bmv.y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
					mb->mvs[0].y;

				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, quant, 0);
				//DEBUG("B-frame Backward!\n");
				break;

			case MODE_FORWARD:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_forward,
									dec->p_fmv);
				dec->p_fmv.x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
					mb->mvs[0].x;
				dec->p_fmv.y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
					mb->mvs[0].y;

				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, quant, 1);
				//DEBUG("B-frame Forward!\n");
				break;

			default:
				DEBUG1("Not support B-frame mb_type =", mb->mb_type);
			}

		}						// end of FOR
	}
}

// swap two MACROBLOCK array
void
mb_swap(MACROBLOCK ** mb1,
		MACROBLOCK ** mb2)
{
	MACROBLOCK *temp = *mb1;

	*mb1 = *mb2;
	*mb2 = temp;
}

int
decoder_decode(DECODER * dec,
			   XVID_DEC_FRAME * frame)
{

	Bitstream bs;
	uint32_t rounding;
	uint32_t quant;
	uint32_t fcode_forward;
	uint32_t fcode_backward;
	uint32_t intra_dc_threshold;
	uint32_t vop_type;

	start_global_timer();

	BitstreamInit(&bs, frame->bitstream, frame->length);

	// add by chenm001 <chenm001@163.com>
	// for support B-frame to reference last 2 frame
	dec->frames++;
	vop_type =
		BitstreamReadHeaders(&bs, dec, &rounding, &quant, &fcode_forward,
							 &fcode_backward, &intra_dc_threshold);

	dec->p_bmv.x = dec->p_bmv.y = dec->p_fmv.y = dec->p_fmv.y = 0;	// init pred vector to 0

	switch (vop_type) {
	case P_VOP:
		decoder_pframe(dec, &bs, rounding, quant, fcode_forward,
					   intra_dc_threshold);
		DEBUG1("P_VOP  Time=", dec->time);
		break;

	case I_VOP:
		decoder_iframe(dec, &bs, quant, intra_dc_threshold);
		DEBUG1("I_VOP  Time=", dec->time);
		break;

	case B_VOP:
#ifdef BFRAMES_DEC
		if (dec->time_pp > dec->time_bp) {
			DEBUG1("B_VOP  Time=", dec->time);
			decoder_bframe(dec, &bs, quant, fcode_forward, fcode_backward);
		} else {
			DEBUG("broken B-frame!");
		}
#else
		image_copy(&dec->cur, &dec->refn[0], dec->edged_width, dec->height);
#endif
		break;

	case N_VOP:				// vop not coded
		// when low_delay==0, N_VOP's should interpolate between the past and future frames
		image_copy(&dec->cur, &dec->refn[0], dec->edged_width, dec->height);
		break;

	default:
		return XVID_ERR_FAIL;
	}

	frame->length = BitstreamPos(&bs) / 8;

#ifdef BFRAMES_DEC
	// test if no B_VOP
	if (dec->low_delay) {
#endif
		image_output(&dec->cur, dec->width, dec->height, dec->edged_width,
					 frame->image, frame->stride, frame->colorspace);
#ifdef BFRAMES_DEC
	} else {
		if (dec->frames >= 1) {
			start_timer();
			if ((vop_type == I_VOP || vop_type == P_VOP)) {
				image_output(&dec->refn[0], dec->width, dec->height,
							 dec->edged_width, frame->image, frame->stride,
							 frame->colorspace);
			} else if (vop_type == B_VOP) {
				image_output(&dec->cur, dec->width, dec->height,
							 dec->edged_width, frame->image, frame->stride,
							 frame->colorspace);
			}
			stop_conv_timer();
		}
	}
#endif

	if (vop_type == I_VOP || vop_type == P_VOP) {
		image_swap(&dec->refn[0], &dec->refn[1]);
		image_swap(&dec->cur, &dec->refn[0]);
		// swap MACROBLOCK
		if (dec->low_delay && vop_type == P_VOP)
			mb_swap(&dec->mbs, &dec->last_mbs);
	}

	emms();

	stop_global_timer();

	return XVID_ERR_OK;
}
