/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  Decoder main module  -
 *
 *  Copyright(C) 2002 MinChen <chenm001@163.com>
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
 * $Id: decoder.c,v 1.44 2002-11-26 23:44:09 edgomez Exp $
 *
 *************************************************************************/

#include <stdlib.h>
#include <string.h>

#ifdef BFRAMES_DEC_DEBUG
	#define BFRAMES_DEC
#endif

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
	/* add by chenm001 <chenm001@163.com> */
	/* for support B-frame to reference last 2 frame */
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

	/* add by chenm001 <chenm001@163.com> */
	/* for skip MB flag */
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

	/* add by chenm001 <chenm001@163.com> */
	/* for support B-frame to save reference frame's time */
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




/* decode an intra macroblock */

void
decoder_mbintra(DECODER * dec,
				MACROBLOCK * pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				const uint32_t acpred_flag,
				const uint32_t cbp,
				Bitstream * bs,
				const uint32_t quant,
				const uint32_t intra_dc_threshold,
				const unsigned int bound)
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

	memset(block, 0, 6 * 64 * sizeof(int16_t));	/* clear */

	for (i = 0; i < 6; i++) {
		uint32_t iDcScaler = get_dc_scaler(iQuant, i < 4);
		int16_t predictors[8];
		int start_coeff;

		start_timer();
		predict_acdc(dec->mbs, x_pos, y_pos, dec->mb_width, i, &block[i * 64],
					 iQuant, iDcScaler, predictors, bound);
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
				BitstreamSkip(bs, 1);	/* marker */
			}

			block[i * 64 + 0] = dc_dif;
			start_coeff = 1;

			DPRINTF(DPRINTF_COEFF,"block[0] %i", dc_dif);
		} else {
			start_coeff = 0;
		}

		start_timer();
		if (cbp & (1 << (5 - i)))	/* coded */
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


/* decode an inter macroblock */

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

		if (dec->quarterpel)
		{
			uv_dx = (uv_dx >> 1) | (uv_dx & 1);
			uv_dy = (uv_dy >> 1) | (uv_dy & 1);
		}

		uv_dx = (uv_dx & 3) ? (uv_dx >> 1) | 1 : uv_dx / 2;
		uv_dy = (uv_dy & 3) ? (uv_dy >> 1) | 1 : uv_dy / 2;
	} else {
		int sum;
		sum = pMB->mvs[0].x + pMB->mvs[1].x + pMB->mvs[2].x + pMB->mvs[3].x;

		if (dec->quarterpel)
		{
			sum /= 2;
		}

		uv_dx = (sum == 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2));

		sum = pMB->mvs[0].y + pMB->mvs[1].y + pMB->mvs[2].y + pMB->mvs[3].y;

		if (dec->quarterpel)
		{
			sum /= 2;
		}

		uv_dy = (sum == 0 ? 0 : SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2));
	}

	start_timer();
	if(dec->quarterpel) {
		DPRINTF(DPRINTF_DEBUG, "QUARTERPEL\n");
		interpolate8x8_quarterpel(dec->cur.y, dec->refn[0].y, 16*x_pos, 16*y_pos,
								  pMB->mvs[0].x, pMB->mvs[0].y, stride,  rounding);
		interpolate8x8_quarterpel(dec->cur.y, dec->refn[0].y, 16*x_pos + 8, 16*y_pos,
								  pMB->mvs[1].x, pMB->mvs[1].y, stride,  rounding);
		interpolate8x8_quarterpel(dec->cur.y, dec->refn[0].y, 16*x_pos, 16*y_pos + 8,
								  pMB->mvs[2].x, pMB->mvs[2].y, stride,  rounding);
		interpolate8x8_quarterpel(dec->cur.y, dec->refn[0].y, 16*x_pos + 8, 16*y_pos + 8,
								  pMB->mvs[3].x, pMB->mvs[3].y, stride,  rounding);
	}
	else {
		interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16*x_pos, 16*y_pos,
							  pMB->mvs[0].x, pMB->mvs[0].y, stride,  rounding);
		interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16*x_pos + 8, 16*y_pos,
							  pMB->mvs[1].x, pMB->mvs[1].y, stride,  rounding);
		interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16*x_pos, 16*y_pos + 8,
							  pMB->mvs[2].x, pMB->mvs[2].y, stride,  rounding);
		interpolate8x8_switch(dec->cur.y, dec->refn[0].y, 16*x_pos + 8, 16*y_pos + 8, 
							  pMB->mvs[3].x, pMB->mvs[3].y, stride,  rounding);
	}

	interpolate8x8_switch(dec->cur.u, dec->refn[0].u, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, rounding);
	interpolate8x8_switch(dec->cur.v, dec->refn[0].v, 8 * x_pos, 8 * y_pos,
						  uv_dx, uv_dy, stride2, rounding);
	stop_comp_timer();

	for (i = 0; i < 6; i++) {
		if (cbp & (1 << (5 - i)))	/* coded */
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	/* clear */

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
	uint32_t bound;
	uint32_t x, y;

	bound = 0;

	for (y = 0; y < dec->mb_height; y++) {
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb;
			uint32_t mcbpc;
			uint32_t cbpc;
			uint32_t acpred_flag;
			uint32_t cbpy;
			uint32_t cbp;

			while (BitstreamShowBits(bs, 9) == 1)
				BitstreamSkip(bs, 9);

			if (check_resync_marker(bs, 0))
			{
				bound = read_video_packet_header(bs, 0, &quant);
				x = bound % dec->mb_width;
				y = bound / dec->mb_width;
			}
			mb = &dec->mbs[y * dec->mb_width + x];

			DPRINTF(DPRINTF_MB, "macroblock (%i,%i) %08x", x, y, BitstreamShowBits(bs, 32));

			mcbpc = get_mcbpc_intra(bs);
			mb->mode = mcbpc & 7;
			cbpc = (mcbpc >> 4);

			acpred_flag = BitstreamGetBit(bs);

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
			mb->mvs[0].x = mb->mvs[0].y =
			mb->mvs[1].x = mb->mvs[1].y =
			mb->mvs[2].x = mb->mvs[2].y =
			mb->mvs[3].x = mb->mvs[3].y =0;

			if (dec->interlacing) {
				mb->field_dct = BitstreamGetBit(bs);
				DPRINTF(DPRINTF_DEBUG, "deci: field_dct: %d", mb->field_dct);
			}

			decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant,
							intra_dc_threshold, bound);
		}
		if(dec->out_frm)
		  output_slice(&dec->cur, dec->edged_width,dec->width,dec->out_frm,0,y,dec->mb_width);

	}

}


void
get_motion_vector(DECODER * dec,
				  Bitstream * bs,
				  int x,
				  int y,
				  int k,
				  VECTOR * mv,
				  int fcode,
				  const int bound)
{

	int scale_fac = 1 << (fcode - 1);
	int high = (32 * scale_fac) - 1;
	int low = ((-32) * scale_fac);
	int range = (64 * scale_fac);

	VECTOR pmv;
	int mv_x, mv_y;

	pmv = get_pmv2(dec->mbs, dec->mb_width, bound, x, y, k);

	mv_x = get_mv(bs, fcode);
	mv_y = get_mv(bs, fcode);

	DPRINTF(DPRINTF_MV,"mv_diff (%i,%i) pred (%i,%i)", mv_x, mv_y, pmv.x, pmv.y);

	mv_x += pmv.x;
	mv_y += pmv.y;

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
	uint32_t bound;
	int cp_mb, st_mb;

	start_timer();
	image_setedges(&dec->refn[0], dec->edged_width, dec->edged_height,
				   dec->width, dec->height);
	stop_edges_timer();

	bound = 0;

	for (y = 0; y < dec->mb_height; y++) {
		cp_mb = st_mb = 0;
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb;

			/* skip stuffing */
			while (BitstreamShowBits(bs, 10) == 1)
				BitstreamSkip(bs, 10);

			if (check_resync_marker(bs, fcode - 1))
			{
				bound = read_video_packet_header(bs, fcode - 1, &quant);
				x = bound % dec->mb_width;
				y = bound / dec->mb_width;
			}
			mb = &dec->mbs[y * dec->mb_width + x];

			DPRINTF(DPRINTF_MB, "macroblock (%i,%i) %08x", x, y, BitstreamShowBits(bs, 32));

			/*if (!(dec->mb_skip[y*dec->mb_width + x]=BitstreamGetBit(bs)))          not_coded */
			if (!(BitstreamGetBit(bs)))	/* not_coded */
			{
				uint32_t mcbpc;
				uint32_t cbpc;
				uint32_t acpred_flag;
				uint32_t cbpy;
				uint32_t cbp;
				uint32_t intra;

				cp_mb++;
				mcbpc = get_mcbpc_inter(bs);
				mb->mode = mcbpc & 7;
				cbpc = (mcbpc >> 4);
				
				DPRINTF(DPRINTF_MB, "mode %i", mb->mode);
				DPRINTF(DPRINTF_MB, "cbpc %i", cbpc);
				acpred_flag = 0;

				intra = (mb->mode == MODE_INTRA || mb->mode == MODE_INTRA_Q);

				if (intra) {
					acpred_flag = BitstreamGetBit(bs);
				}

				cbpy = get_cbpy(bs, intra);
				DPRINTF(DPRINTF_MB, "cbpy %i", cbpy);

				cbp = (cbpy << 2) | cbpc;

				if (mb->mode == MODE_INTER_Q || mb->mode == MODE_INTRA_Q) {
					int dquant = dquant_table[BitstreamGetBits(bs, 2)];
					DPRINTF(DPRINTF_MB, "dquant %i", dquant);
					quant += dquant;
					if (quant > 31) {
						quant = 31;
					} else if (quant < 1) {
						quant = 1;
					}
					DPRINTF(DPRINTF_MB, "quant %i", quant);
				}
				mb->quant = quant;

				if (dec->interlacing) {
					if (cbp || intra) {
						mb->field_dct = BitstreamGetBit(bs);
						DPRINTF(DPRINTF_DEBUG, "decp: field_dct: %d", mb->field_dct);
					}

					if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) {
						mb->field_pred = BitstreamGetBit(bs);
						DPRINTF(DPRINTF_DEBUG, "decp: field_pred: %d", mb->field_pred);

						if (mb->field_pred) {
							mb->field_for_top = BitstreamGetBit(bs);
							DPRINTF(DPRINTF_DEBUG, "decp: field_for_top: %d", mb->field_for_top);
							mb->field_for_bot = BitstreamGetBit(bs);
							DPRINTF(DPRINTF_DEBUG, "decp: field_for_bot: %d", mb->field_for_bot);
						}
					}
				}

				if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q) {
					if (dec->interlacing && mb->field_pred) {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0],
										  fcode, bound);
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[1],
										  fcode, bound);
					} else {
						get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0],
										  fcode, bound);
						mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
							mb->mvs[0].x;
						mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
							mb->mvs[0].y;
					}
				} else if (mb->mode == MODE_INTER4V ) {

					get_motion_vector(dec, bs, x, y, 0, &mb->mvs[0], fcode, bound);
					get_motion_vector(dec, bs, x, y, 1, &mb->mvs[1], fcode, bound);
					get_motion_vector(dec, bs, x, y, 2, &mb->mvs[2], fcode, bound);
					get_motion_vector(dec, bs, x, y, 3, &mb->mvs[3], fcode, bound);
				} else			/* MODE_INTRA, MODE_INTRA_Q */
				{
					mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x =
						0;
					mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y =
						0;
					decoder_mbintra(dec, mb, x, y, acpred_flag, cbp, bs, quant,
									intra_dc_threshold, bound);
					continue;
				}

				decoder_mbinter(dec, mb, x, y, acpred_flag, cbp, bs, quant,
								rounding);
			} else				/* not coded */
			{
				DPRINTF(DPRINTF_DEBUG, "P-frame MB at (X,Y)=(%d,%d)", x, y);

				mb->mode = MODE_NOT_CODED;
				mb->mvs[0].x = mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = 0;
				mb->mvs[0].y = mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = 0;

				/* copy macroblock directly from ref to cur */

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
				if(dec->out_frm && cp_mb > 0) {
				  output_slice(&dec->cur, dec->edged_width,dec->width,dec->out_frm,st_mb,y,cp_mb);
				  cp_mb = 0;
				}
				st_mb = x+1;
			}
		}
		if(dec->out_frm && cp_mb > 0)
		  output_slice(&dec->cur, dec->edged_width,dec->width,dec->out_frm,st_mb,y,cp_mb);
	}
}


/* add by MinChen <chenm001@163.com> */
/* decode B-frame motion vector */
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


/* add by MinChen <chenm001@163.com> */
/* decode an B-frame forward & backward inter macroblock */
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
		if (cbp & (1 << (5 - i)))	/* coded */
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	/* clear */

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


/* add by MinChen <chenm001@163.com> */
/* decode an B-frame direct &  inter macroblock */
void
decoder_bf_interpolate_mbinter(DECODER * dec,
							   IMAGE forward,
							   IMAGE backward,
							   const MACROBLOCK * pMB,
							   const uint32_t x_pos,
							   const uint32_t y_pos,
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
    const uint32_t cbp = pMB->cbp;

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
		if (cbp & (1 << (5 - i)))	/* coded */
		{
			memset(&block[i * 64], 0, 64 * sizeof(int16_t));	/* clear */

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


/* add by MinChen <chenm001@163.com> */
/* for decode B-frame dbquant */
int32_t __inline
get_dbquant(Bitstream * bs)
{
	if (!BitstreamGetBit(bs))	/* '0' */
		return (0);
	else if (!BitstreamGetBit(bs))	/* '10' */
		return (-2);
	else
		return (2);				/* '11' */
}

/* add by MinChen <chenm001@163.com> */
/* for decode B-frame mb_type */
/* bit   ret_value */
/* 1        0 */
/* 01       1 */
/* 001      2 */
/* 0001     3 */
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
	VECTOR mv;
	const VECTOR zeromv = {0,0};
#ifdef BFRAMES_DEC_DEBUG
	FILE *fp;
	static char first=0;
#define BFRAME_DEBUG  	if (!first && fp){ \
		fprintf(fp,"Y=%3d   X=%3d   MB=%2d   CBP=%02X\n",y,x,mb->mb_type,mb->cbp); \
	}
#endif

	start_timer();
	image_setedges(&dec->refn[0], dec->edged_width, dec->edged_height,
				   dec->width, dec->height);
	image_setedges(&dec->refn[1], dec->edged_width, dec->edged_height,
				   dec->width, dec->height);
	stop_edges_timer();

#ifdef BFRAMES_DEC_DEBUG
	if (!first){
		fp=fopen("C:\\XVIDDBG.TXT","w");
	}
#endif

	for (y = 0; y < dec->mb_height; y++) {
		/* Initialize Pred Motion Vector */
		dec->p_fmv = dec->p_bmv = zeromv;
		for (x = 0; x < dec->mb_width; x++) {
			MACROBLOCK *mb = &dec->mbs[y * dec->mb_width + x];
			MACROBLOCK *last_mb = &dec->last_mbs[y * dec->mb_width + x];

			mv =
			mb->b_mvs[0] = mb->b_mvs[1] = mb->b_mvs[2] = mb->b_mvs[3] =
			mb->mvs[0] = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] = zeromv;

			/* the last P_VOP is skip macroblock ? */
			if (last_mb->mode == MODE_NOT_CODED) {
				/*DEBUG2("Skip MB in B-frame at (X,Y)=!",x,y); */
				mb->cbp = 0;
#ifdef BFRAMES_DEC_DEBUG
				mb->mb_type = MODE_NOT_CODED;
	BFRAME_DEBUG
#endif
				mb->mb_type = MODE_FORWARD;
				mb->quant = last_mb->quant;
				/*mb->mvs[1].x = mb->mvs[2].x = mb->mvs[3].x = mb->mvs[0].x; */
				/*mb->mvs[1].y = mb->mvs[2].y = mb->mvs[3].y = mb->mvs[0].y; */

				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, mb->quant, 1);
				continue;
			}

			if (!BitstreamGetBit(bs)) {	/* modb=='0' */
				const uint8_t modb2 = BitstreamGetBit(bs);

				mb->mb_type = get_mbtype(bs);

				if (!modb2) {	/* modb=='00' */
					mb->cbp = BitstreamGetBits(bs, 6);
				} else {
					mb->cbp = 0;
				}
				if (mb->mb_type && mb->cbp) {
					quant += get_dbquant(bs);

					if (quant > 31) {
						quant = 31;
					} else if (quant < 1) {
						quant = 1;
					}
				}
			} else {
				mb->mb_type = MODE_DIRECT_NONE_MV;
				mb->cbp = 0;
			}

			mb->quant = quant;
			mb->mode = MODE_INTER4V;
			/*DEBUG1("Switch bm_type=",mb->mb_type); */

#ifdef BFRAMES_DEC_DEBUG
	BFRAME_DEBUG
#endif

			switch (mb->mb_type) {
			case MODE_DIRECT:
				get_b_motion_vector(dec, bs, x, y, &mv, 1, zeromv);

			case MODE_DIRECT_NONE_MV:
				{	
					const int64_t TRB = dec->time_pp - dec->time_bp, TRD = dec->time_pp;
					int i;

					for (i = 0; i < 4; i++) {
						mb->mvs[i].x = (int32_t) ((TRB * last_mb->mvs[i].x)
							              / TRD + mv.x);
						mb->b_mvs[i].x = (int32_t) ((mv.x == 0)
										? ((TRB - TRD) * last_mb->mvs[i].x)
										  / TRD
										: mb->mvs[i].x - last_mb->mvs[i].x);
						mb->mvs[i].y = (int32_t) ((TRB * last_mb->mvs[i].y)
							              / TRD + mv.y);
						mb->b_mvs[i].y = (int32_t) ((mv.y == 0)
										? ((TRB - TRD) * last_mb->mvs[i].y)
										  / TRD
									    : mb->mvs[i].y - last_mb->mvs[i].y);
					}
					/*DEBUG("B-frame Direct!\n"); */
				}
				decoder_bf_interpolate_mbinter(dec, dec->refn[1], dec->refn[0],
											   mb, x, y, bs);
				break;

			case MODE_INTERPOLATE:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_forward,
									dec->p_fmv);
				dec->p_fmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				get_b_motion_vector(dec, bs, x, y, &mb->b_mvs[0],
									fcode_backward, dec->p_bmv);
				dec->p_bmv = mb->b_mvs[1] = mb->b_mvs[2] =
					mb->b_mvs[3] = mb->b_mvs[0];

				decoder_bf_interpolate_mbinter(dec, dec->refn[1], dec->refn[0],
											   mb, x, y, bs);
				/*DEBUG("B-frame Bidir!\n"); */
				break;

			case MODE_BACKWARD:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_backward,
									dec->p_bmv);
				dec->p_bmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				mb->mode = MODE_INTER;
				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, quant, 0);
				/*DEBUG("B-frame Backward!\n"); */
				break;

			case MODE_FORWARD:
				get_b_motion_vector(dec, bs, x, y, &mb->mvs[0], fcode_forward,
									dec->p_fmv);
				dec->p_fmv = mb->mvs[1] = mb->mvs[2] = mb->mvs[3] =	mb->mvs[0];

				mb->mode = MODE_INTER;
				decoder_bf_mbinter(dec, mb, x, y, mb->cbp, bs, quant, 1);
				/*DEBUG("B-frame Forward!\n"); */
				break;

			default:
				DPRINTF(DPRINTF_ERROR, "Not support B-frame mb_type = %d", mb->mb_type);
			}

		}						/* end of FOR */
	}
#ifdef BFRAMES_DEC_DEBUG
	if (!first){
		first=1;
		if (fp)
			fclose(fp);
	}
#endif
}

/* swap two MACROBLOCK array */
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

	dec->out_frm = (frame->colorspace == XVID_CSP_EXTERN) ? frame->image : NULL;

	BitstreamInit(&bs, frame->bitstream, frame->length);

	/* add by chenm001 <chenm001@163.com> */
	/* for support B-frame to reference last 2 frame */
	dec->frames++;
	vop_type =
		BitstreamReadHeaders(&bs, dec, &rounding, &quant, &fcode_forward,
							 &fcode_backward, &intra_dc_threshold);

	dec->p_bmv.x = dec->p_bmv.y = dec->p_fmv.y = dec->p_fmv.y = 0;	/* init pred vector to 0 */

	switch (vop_type) {
	case P_VOP:
		decoder_pframe(dec, &bs, rounding, quant, fcode_forward,
					   intra_dc_threshold);
#ifdef BFRAMES_DEC
		DPRINTF(DPRINTF_DEBUG, "P_VOP  Time=%d", dec->time);
#endif
		break;

	case I_VOP:
		decoder_iframe(dec, &bs, quant, intra_dc_threshold);
#ifdef BFRAMES_DEC
		DPRINTF(DPRINTF_DEBUG, "I_VOP  Time=%d", dec->time);
#endif
		break;

	case B_VOP:
#ifdef BFRAMES_DEC
		if (dec->time_pp > dec->time_bp) {
			DPRINTF(DPRINTF_DEBUG, "B_VOP  Time=%d", dec->time);
			decoder_bframe(dec, &bs, quant, fcode_forward, fcode_backward);
		} else {
			DPRINTF(DPRINTF_DEBUG, "Broken B_VOP");
		}
#else
		image_copy(&dec->cur, &dec->refn[0], dec->edged_width, dec->height);
#endif
		break;

	case N_VOP:				/* vop not coded */
		/* when low_delay==0, N_VOP's should interpolate between the past and future frames */
		image_copy(&dec->cur, &dec->refn[0], dec->edged_width, dec->height);
		break;

	default:
		return XVID_ERR_FAIL;
	}

#ifdef BFRAMES_DEC_DEBUG
	if (frame->length != BitstreamPos(&bs) / 8){
		DPRINTF(DPRINTF_DEBUG, "InLen: %d / UseLen: %d", frame->length, BitstreamPos(&bs) / 8);
	}
#endif
	frame->length = BitstreamPos(&bs) / 8;


#ifdef BFRAMES_DEC
	/* test if no B_VOP */
        if (dec->low_delay || dec->frames == 0) {
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

		/* swap MACROBLOCK */
                /* the Divx will not set the low_delay flage some times */
                /* so follow code will wrong to not swap at that time */
                /* this will broken bitstream! so I'm change it, */
                /* But that is not the best way! can anyone tell me how */
                /* to do another way? */
                /* 18-07-2002   MinChen<chenm001@163.com> */
                /*if (!dec->low_delay && vop_type == P_VOP) */
                if (vop_type == P_VOP)
			mb_swap(&dec->mbs, &dec->last_mbs);
	}

	emms();

	stop_global_timer();

	return XVID_ERR_OK;
}
