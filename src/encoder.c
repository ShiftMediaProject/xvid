/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  Encoder main module  -
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
 ****************************************************************************/

/*****************************************************************************
 * 
 *  History
 *
 *  10.07.2002  added BFRAMES_DEC_DEBUG support
 *              MinChen <chenm001@163.com>
 *  20.06.2002 bframe patch
 *  08.05.2002 fix some problem in DEBUG mode;
 *             MinChen <chenm001@163.com>
 *  14.04.2002 added FrameCodeB()
 *
 *  $Id: encoder.c,v 1.71 2002-08-04 22:34:49 edgomez Exp $
 *
 ****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "encoder.h"
#include "prediction/mbprediction.h"
#include "global.h"
#include "utils/timer.h"
#include "image/image.h"
#ifdef BFRAMES
#include "image/font.h"
#include "motion/sad.h"
#endif
#include "motion/motion.h"
#include "bitstream/cbp.h"
#include "utils/mbfunctions.h"
#include "bitstream/bitstream.h"
#include "bitstream/mbcoding.h"
#include "utils/ratecontrol.h"
#include "utils/emms.h"
#include "bitstream/mbcoding.h"
#include "quant/adapt_quant.h"
#include "quant/quant_matrix.h"
#include "utils/mem_align.h"

#ifdef _SMP
#include "motion/smp_motion_est.h"
#endif 
/*****************************************************************************
 * Local macros
 ****************************************************************************/

#define ENC_CHECK(X) if(!(X)) return XVID_ERR_FORMAT
#define SWAP(A,B)    { void * tmp = A; A = B; B = tmp; }

/*****************************************************************************
 * Local function prototypes
 ****************************************************************************/

static int FrameCodeI(Encoder * pEnc,
					  Bitstream * bs,
					  uint32_t * pBits);

static int FrameCodeP(Encoder * pEnc,
					  Bitstream * bs,
					  uint32_t * pBits,
					  bool force_inter,
					  bool vol_header);

#ifdef BFRAMES
static void FrameCodeB(Encoder * pEnc,
					   FRAMEINFO * frame,
					   Bitstream * bs,
					   uint32_t * pBits);
#endif

/*****************************************************************************
 * Local data
 ****************************************************************************/

static int DQtab[4] = {
	-1, -2, 1, 2
};

static int iDQtab[5] = {
	1, 0, NO_CHANGE, 2, 3
};


static void __inline
image_null(IMAGE * image)
{
	image->y = image->u = image->v = NULL;
}


/*****************************************************************************
 * Encoder creation
 *
 * This function creates an Encoder instance, it allocates all necessary
 * image buffers (reference, current and bframes) and initialize the internal
 * xvid encoder paremeters according to the XVID_ENC_PARAM input parameter.
 *
 * The code seems to be very long but is very basic, mainly memory allocation
 * and cleaning code.
 *
 * Returned values :
 *    - XVID_ERR_OK     - no errors
 *    - XVID_ERR_MEMORY - the libc could not allocate memory, the function
 *                        cleans the structure before exiting.
 *                        pParam->handle is also set to NULL.
 *
 ****************************************************************************/

int
encoder_create(XVID_ENC_PARAM * pParam)
{
	Encoder *pEnc;
	int i;

	pParam->handle = NULL;

	ENC_CHECK(pParam);

	ENC_CHECK(pParam->width > 0 && pParam->width <= 1920);
	ENC_CHECK(pParam->height > 0 && pParam->height <= 1280);
	ENC_CHECK(!(pParam->width % 2));
	ENC_CHECK(!(pParam->height % 2));

	/* Fps */

	if (pParam->fincr <= 0 || pParam->fbase <= 0) {
		pParam->fincr = 1;
		pParam->fbase = 25;
	}

	/*
	 * Simplify the "fincr/fbase" fraction
	 * (neccessary, since windows supplies us with huge numbers)
	 */

	i = pParam->fincr;
	while (i > 1) {
		if (pParam->fincr % i == 0 && pParam->fbase % i == 0) {
			pParam->fincr /= i;
			pParam->fbase /= i;
			i = pParam->fincr;
			continue;
		}
		i--;
	}

	if (pParam->fbase > 65535) {
		float div = (float) pParam->fbase / 65535;

		pParam->fbase = (int) (pParam->fbase / div);
		pParam->fincr = (int) (pParam->fincr / div);
	}

	/* Bitrate allocator defaults */

	if (pParam->rc_bitrate <= 0)
		pParam->rc_bitrate = 900000;

	if (pParam->rc_reaction_delay_factor <= 0)
		pParam->rc_reaction_delay_factor = 16;

	if (pParam->rc_averaging_period <= 0)
		pParam->rc_averaging_period = 100;

	if (pParam->rc_buffer <= 0)
		pParam->rc_buffer = 100;

	/* Max and min quantizers */

	if ((pParam->min_quantizer <= 0) || (pParam->min_quantizer > 31))
		pParam->min_quantizer = 1;

	if ((pParam->max_quantizer <= 0) || (pParam->max_quantizer > 31))
		pParam->max_quantizer = 31;

	if (pParam->max_quantizer < pParam->min_quantizer)
		pParam->max_quantizer = pParam->min_quantizer;

	/* 1 keyframe each 10 seconds */

	if (pParam->max_key_interval <= 0)
		pParam->max_key_interval = 10 * pParam->fincr / pParam->fbase;

	pEnc = (Encoder *) xvid_malloc(sizeof(Encoder), CACHE_LINE);
	if (pEnc == NULL)
		return XVID_ERR_MEMORY;

	/* Zero the Encoder Structure */

	memset(pEnc, 0, sizeof(Encoder));

	/* Fill members of Encoder structure */

	pEnc->mbParam.width = pParam->width;
	pEnc->mbParam.height = pParam->height;

	pEnc->mbParam.mb_width = (pEnc->mbParam.width + 15) / 16;
	pEnc->mbParam.mb_height = (pEnc->mbParam.height + 15) / 16;

	pEnc->mbParam.edged_width = 16 * pEnc->mbParam.mb_width + 2 * EDGE_SIZE;
	pEnc->mbParam.edged_height = 16 * pEnc->mbParam.mb_height + 2 * EDGE_SIZE;

	pEnc->mbParam.fbase = pParam->fbase;
	pEnc->mbParam.fincr = pParam->fincr;

	pEnc->mbParam.m_quant_type = H263_QUANT;

#ifdef _SMP
	pEnc->mbParam.num_threads = MIN(pParam->num_threads, MAXNUMTHREADS);
#endif

	pEnc->sStat.fMvPrevSigma = -1;

	/* Fill rate control parameters */

	pEnc->bitrate = pParam->rc_bitrate;

	pEnc->iFrameNum = 0;
	pEnc->iMaxKeyInterval = pParam->max_key_interval;

	/* try to allocate frame memory */

	pEnc->current = xvid_malloc(sizeof(FRAMEINFO), CACHE_LINE);
	pEnc->reference = xvid_malloc(sizeof(FRAMEINFO), CACHE_LINE);

	if (pEnc->current == NULL || pEnc->reference == NULL)
		goto xvid_err_memory1;

	/* try to allocate mb memory */

	pEnc->current->mbs =
		xvid_malloc(sizeof(MACROBLOCK) * pEnc->mbParam.mb_width *
					pEnc->mbParam.mb_height, CACHE_LINE);
	pEnc->reference->mbs =
		xvid_malloc(sizeof(MACROBLOCK) * pEnc->mbParam.mb_width *
					pEnc->mbParam.mb_height, CACHE_LINE);

	if (pEnc->current->mbs == NULL || pEnc->reference->mbs == NULL)
		goto xvid_err_memory2;

	/* try to allocate image memory */

#ifdef _DEBUG_PSNR
	image_null(&pEnc->sOriginal);
#endif
#ifdef BFRAMES
	image_null(&pEnc->f_refh);
	image_null(&pEnc->f_refv);
	image_null(&pEnc->f_refhv);
#endif
	image_null(&pEnc->current->image);
	image_null(&pEnc->reference->image);
	image_null(&pEnc->vInterH);
	image_null(&pEnc->vInterV);
	image_null(&pEnc->vInterVf);
	image_null(&pEnc->vInterHV);
	image_null(&pEnc->vInterHVf);

#ifdef _DEBUG_PSNR
	if (image_create
		(&pEnc->sOriginal, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
#endif
#ifdef BFRAMES
	if (image_create
		(&pEnc->f_refh, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->f_refv, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->f_refhv, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
#endif
	if (image_create
		(&pEnc->current->image, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->reference->image, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->vInterH, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->vInterV, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->vInterVf, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->vInterHV, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;
	if (image_create
		(&pEnc->vInterHVf, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;



	/* B Frames specific init */
#ifdef BFRAMES

	pEnc->global = pParam->global;
	pEnc->mbParam.max_bframes = pParam->max_bframes;
	pEnc->bquant_ratio = pParam->bquant_ratio;
	pEnc->frame_drop_ratio = pParam->frame_drop_ratio;
	pEnc->bframes = NULL;

	if (pEnc->mbParam.max_bframes > 0) {
		int n;

		pEnc->bframes =
			xvid_malloc(pEnc->mbParam.max_bframes * sizeof(FRAMEINFO *),
						CACHE_LINE);

		if (pEnc->bframes == NULL)
			goto xvid_err_memory3;

		for (n = 0; n < pEnc->mbParam.max_bframes; n++)
			pEnc->bframes[n] = NULL;


		for (n = 0; n < pEnc->mbParam.max_bframes; n++) {
			pEnc->bframes[n] = xvid_malloc(sizeof(FRAMEINFO), CACHE_LINE);

			if (pEnc->bframes[n] == NULL)
				goto xvid_err_memory4;

			pEnc->bframes[n]->mbs =
				xvid_malloc(sizeof(MACROBLOCK) * pEnc->mbParam.mb_width *
							pEnc->mbParam.mb_height, CACHE_LINE);

			if (pEnc->bframes[n]->mbs == NULL)
				goto xvid_err_memory4;

			image_null(&pEnc->bframes[n]->image);

			if (image_create
				(&pEnc->bframes[n]->image, pEnc->mbParam.edged_width,
				 pEnc->mbParam.edged_height) < 0)
				goto xvid_err_memory4;

		}
	}

	pEnc->bframenum_head = 0;
	pEnc->bframenum_tail = 0;
	pEnc->flush_bframes = 0;
	pEnc->bframenum_dx50bvop = -1;

	pEnc->queue = NULL;


	if (pEnc->mbParam.max_bframes > 0) {
		int n;

		pEnc->queue =
			xvid_malloc(pEnc->mbParam.max_bframes * sizeof(IMAGE),
						CACHE_LINE);

		if (pEnc->queue == NULL)
			goto xvid_err_memory4;

		for (n = 0; n < pEnc->mbParam.max_bframes; n++)
			image_null(&pEnc->queue[n]);

		for (n = 0; n < pEnc->mbParam.max_bframes; n++) {
			if (image_create
				(&pEnc->queue[n], pEnc->mbParam.edged_width,
				 pEnc->mbParam.edged_height) < 0)
				goto xvid_err_memory5;

		}
	}

	pEnc->queue_head = 0;
	pEnc->queue_tail = 0;
	pEnc->queue_size = 0;


	pEnc->mbParam.m_seconds = 0;
	pEnc->mbParam.m_ticks = 0;
	pEnc->m_framenum = 0;
	pEnc->last_pframe = 0;
#endif

	pParam->handle = (void *) pEnc;

	if (pParam->rc_bitrate) {
		RateControlInit(&pEnc->rate_control, pParam->rc_bitrate,
						pParam->rc_reaction_delay_factor,
						pParam->rc_averaging_period, pParam->rc_buffer,
						pParam->fbase * 1000 / pParam->fincr,
						pParam->max_quantizer, pParam->min_quantizer);
	}

	init_timer();

	return XVID_ERR_OK;

	/*
	 * We handle all XVID_ERR_MEMORY here, this makes the code lighter
	 */
#ifdef BFRAMES
  xvid_err_memory5:


	if (pEnc->mbParam.max_bframes > 0) {

		for (i = 0; i < pEnc->mbParam.max_bframes; i++) {
			image_destroy(&pEnc->queue[i], pEnc->mbParam.edged_width,
						  pEnc->mbParam.edged_height);
		}
		xvid_free(pEnc->queue);
	}

  xvid_err_memory4:

	if (pEnc->mbParam.max_bframes > 0) {

		for (i = 0; i < pEnc->mbParam.max_bframes; i++) {

			if (pEnc->bframes[i] == NULL)
				continue;

			image_destroy(&pEnc->bframes[i]->image, pEnc->mbParam.edged_width,
						  pEnc->mbParam.edged_height);
	
			xvid_free(pEnc->bframes[i]->mbs);
	
			xvid_free(pEnc->bframes[i]);

		}	

		xvid_free(pEnc->bframes);
	}

#endif

  xvid_err_memory3:
#ifdef _DEBUG_PSNR
	image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
#endif

#ifdef BFRAMES
	image_destroy(&pEnc->f_refh, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refhv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
#endif

	image_destroy(&pEnc->current->image, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->reference->image, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterVf, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHVf, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);

  xvid_err_memory2:
	xvid_free(pEnc->current->mbs);
	xvid_free(pEnc->reference->mbs);

  xvid_err_memory1:
	xvid_free(pEnc->current);
	xvid_free(pEnc->reference);
	xvid_free(pEnc);

	pParam->handle = NULL;

	return XVID_ERR_MEMORY;
}

/*****************************************************************************
 * Encoder destruction
 *
 * This function destroy the entire encoder structure created by a previous
 * successful encoder_create call.
 *
 * Returned values (for now only one returned value) :
 *    - XVID_ERR_OK     - no errors
 *
 ****************************************************************************/

int
encoder_destroy(Encoder * pEnc)
{
#ifdef BFRAMES
	int i;
#endif
	
	ENC_CHECK(pEnc);

	/* B Frames specific */
#ifdef BFRAMES
	if (pEnc->mbParam.max_bframes > 0) {

		for (i = 0; i < pEnc->mbParam.max_bframes; i++) {
		
			image_destroy(&pEnc->queue[i], pEnc->mbParam.edged_width,
					  pEnc->mbParam.edged_height);
		}
		xvid_free(pEnc->queue);
	}


	if (pEnc->mbParam.max_bframes > 0) {

		for (i = 0; i < pEnc->mbParam.max_bframes; i++) {

			if (pEnc->bframes[i] == NULL)
				continue;

			image_destroy(&pEnc->bframes[i]->image, pEnc->mbParam.edged_width,
					  pEnc->mbParam.edged_height);

			xvid_free(pEnc->bframes[i]->mbs);

			xvid_free(pEnc->bframes[i]);
		}

		xvid_free(pEnc->bframes);
	
	}
#endif

	/* All images, reference, current etc ... */

	image_destroy(&pEnc->current->image, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->reference->image, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterH, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterV, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterVf, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHV, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->vInterHVf, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
#ifdef BFRAMES
	image_destroy(&pEnc->f_refh, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refhv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
#endif
#ifdef _DEBUG_PSNR
	image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
#endif

	/* Encoder structure */

	xvid_free(pEnc->current->mbs);
	xvid_free(pEnc->current);

	xvid_free(pEnc->reference->mbs);
	xvid_free(pEnc->reference);

	xvid_free(pEnc);

	return XVID_ERR_OK;
}


#ifdef BFRAMES
void inc_frame_num(Encoder * pEnc)
{
	pEnc->mbParam.m_ticks += pEnc->mbParam.fincr;

	pEnc->mbParam.m_seconds = pEnc->mbParam.m_ticks / pEnc->mbParam.fbase;
	pEnc->mbParam.m_ticks = pEnc->mbParam.m_ticks % pEnc->mbParam.fbase;
}
#endif


#ifdef BFRAMES
void queue_image(Encoder * pEnc, XVID_ENC_FRAME * pFrame)
{
	if (pEnc->queue_size >= pEnc->mbParam.max_bframes)
	{
		DPRINTF(DPRINTF_DEBUG,"FATAL: QUEUE FULL");
		return;
	}

	DPRINTF(DPRINTF_DEBUG,"*** QUEUE bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);


	start_timer();
	if (image_input
		(&pEnc->queue[pEnc->queue_tail], pEnc->mbParam.width, pEnc->mbParam.height,
		 pEnc->mbParam.edged_width, pFrame->image, pFrame->colorspace))
		return;
	stop_conv_timer();

	pEnc->queue_size++;
	pEnc->queue_tail =  (pEnc->queue_tail + 1) % pEnc->mbParam.max_bframes;
}
#endif


#ifdef BFRAMES
/*****************************************************************************
 * IPB frame encoder entry point
 *
 * Returned values :
 *    - XVID_ERR_OK     - no errors
 *    - XVID_ERR_FORMAT - the image subsystem reported the image had a wrong
 *                        format
 ****************************************************************************/

int
encoder_encode_bframes(Encoder * pEnc,
			   XVID_ENC_FRAME * pFrame,
			   XVID_ENC_STATS * pResult)
{
	uint16_t x, y;
	Bitstream bs;
	uint32_t bits;

	int input_valid = 1;

#ifdef _DEBUG_PSNR
	float psnr;
	char temp[128];
#endif

	ENC_CHECK(pEnc);
	ENC_CHECK(pFrame);
	ENC_CHECK(pFrame->image);

	start_global_timer();

	BitstreamInit(&bs, pFrame->bitstream, 0);

ipvop_loop:

	/*
	 * bframe "flush" code
	 */

	if ((pFrame->image == NULL || pEnc->flush_bframes)
		&& (pEnc->bframenum_head < pEnc->bframenum_tail)) {

		if (pEnc->flush_bframes == 0) {
			/*
			 * we have reached the end of stream without getting
			 * a future reference frame... so encode last final
			 * frame as a pframe
			 */

			DPRINTF(DPRINTF_DEBUG,"*** BFRAME (final frame) bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

			pEnc->bframenum_tail--;
			SWAP(pEnc->current, pEnc->reference);

			SWAP(pEnc->current, pEnc->bframes[pEnc->bframenum_tail]);

			FrameCodeP(pEnc, &bs, &bits, 1, 0);

			BitstreamPad(&bs);
			pFrame->length = BitstreamLength(&bs);
			pFrame->intra = 0;

			return XVID_ERR_OK;
		}

		
		DPRINTF(DPRINTF_DEBUG,"*** BFRAME (flush) bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

		FrameCodeB(pEnc, pEnc->bframes[pEnc->bframenum_head], &bs, &bits);
		pEnc->bframenum_head++;

		BitstreamPad(&bs);
		pFrame->length = BitstreamLength(&bs);
		pFrame->intra = 0;

		if (input_valid)
			queue_image(pEnc, pFrame);

		return XVID_ERR_OK;
	}

	if (pEnc->bframenum_head > 0) {
		pEnc->bframenum_head = pEnc->bframenum_tail = 0;

		if ((pEnc->global & XVID_GLOBAL_PACKED)) {
			
			DPRINTF(DPRINTF_DEBUG,"*** EMPTY bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

			BitstreamWriteVopHeader(&bs, &pEnc->mbParam, pEnc->current, 0);
			BitstreamPad(&bs);
			BitstreamPutBits(&bs, 0x7f, 8);

			pFrame->length = BitstreamLength(&bs);
			pFrame->intra = 0;

			if (input_valid)
				queue_image(pEnc, pFrame);

			return XVID_ERR_OK;
		}
	}


bvop_loop:

	if (pEnc->bframenum_dx50bvop != -1)
	{

		SWAP(pEnc->current, pEnc->reference);
		SWAP(pEnc->current, pEnc->bframes[pEnc->bframenum_dx50bvop]);		

		if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 100, "DX50 IVOP");
		}

		if (input_valid)
		{
			queue_image(pEnc, pFrame);
			input_valid = 0;
		}

	} else if (input_valid) {

		SWAP(pEnc->current, pEnc->reference);

		start_timer();
		if (image_input
			(&pEnc->current->image, pEnc->mbParam.width, pEnc->mbParam.height,
			pEnc->mbParam.edged_width, pFrame->image, pFrame->colorspace))
			return XVID_ERR_FORMAT;
		stop_conv_timer();

		// queue input frame, and dequue next image
		if (pEnc->queue_size > 0)
		{
			image_swap(&pEnc->current->image, &pEnc->queue[pEnc->queue_tail]);
			if (pEnc->queue_head != pEnc->queue_tail)
			{
				image_swap(&pEnc->current->image, &pEnc->queue[pEnc->queue_head]);
			}
			pEnc->queue_head =  (pEnc->queue_head + 1) % pEnc->mbParam.max_bframes;
			pEnc->queue_tail =  (pEnc->queue_tail + 1) % pEnc->mbParam.max_bframes;
		}

	} else if (pEnc->queue_size > 0) {
		
		SWAP(pEnc->current, pEnc->reference);

		image_swap(&pEnc->current->image, &pEnc->queue[pEnc->queue_head]);
		pEnc->queue_head =  (pEnc->queue_head + 1) % pEnc->mbParam.max_bframes;
		pEnc->queue_size--;

	} else if (BitstreamPos(&bs) == 0) {

		DPRINTF(DPRINTF_DEBUG,"*** SKIP bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

		pFrame->intra = 0;

		BitstreamPutBits(&bs, 0x7f, 8);
		BitstreamPad(&bs);
		pFrame->length = BitstreamLength(&bs);
		
		return XVID_ERR_OK;

	} else {

		pFrame->length = BitstreamLength(&bs);
		return XVID_ERR_OK;
	}

	pEnc->flush_bframes = 0;

	/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	 * Well there was a separation here so i put it in ANSI C
	 * comment style :-)
	 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

	emms();

	// only inc frame num, adapt quant, etc. if we havent seen it before
	if (pEnc->bframenum_dx50bvop < 0 )
	{
		if (pFrame->quant == 0)
			pEnc->current->quant = RateControlGetQ(&pEnc->rate_control, 0);
		else
			pEnc->current->quant = pFrame->quant;

/*		if (pEnc->current->quant < 1)
			pEnc->current->quant = 1;

		if (pEnc->current->quant > 31)
			pEnc->current->quant = 31;
*/
		pEnc->current->global_flags = pFrame->general;
		pEnc->current->motion_flags = pFrame->motion;

		/* ToDo : dynamic fcode (in both directions) */
		pEnc->current->fcode = pEnc->mbParam.m_fcode;
		pEnc->current->bcode = pEnc->mbParam.m_fcode;

		pEnc->current->seconds = pEnc->mbParam.m_seconds;
		pEnc->current->ticks = pEnc->mbParam.m_ticks;

		inc_frame_num(pEnc);

#ifdef _DEBUG_PSNR
		image_copy(&pEnc->sOriginal, &pEnc->current->image,
			   pEnc->mbParam.edged_width, pEnc->mbParam.height);
#endif

		emms();

		if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 5, 
				"%i  if:%i  st:%i:%i", pEnc->m_framenum++, pEnc->iFrameNum, pEnc->current->seconds, pEnc->current->ticks);
		}

	/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	 * Luminance masking
	 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */

		if ((pEnc->current->global_flags & XVID_LUMIMASKING)) {
			int *temp_dquants =
				(int *) xvid_malloc(pEnc->mbParam.mb_width *
								pEnc->mbParam.mb_height * sizeof(int),
								CACHE_LINE);

			pEnc->current->quant =
				adaptive_quantization(pEnc->current->image.y,
								  pEnc->mbParam.edged_width, temp_dquants,
								  pEnc->current->quant, pEnc->current->quant,
								  2 * pEnc->current->quant,
								  pEnc->mbParam.mb_width,
								  pEnc->mbParam.mb_height);

			for (y = 0; y < pEnc->mbParam.mb_height; y++) {

#define OFFSET(x,y) ((x) + (y)*pEnc->mbParam.mb_width)

				for (x = 0; x < pEnc->mbParam.mb_width; x++) {
					MACROBLOCK *pMB = &pEnc->current->mbs[OFFSET(x, y)];

					pMB->dquant = iDQtab[temp_dquants[OFFSET(x, y)] + 2];
				}

#undef OFFSET
			}

			xvid_free(temp_dquants);
		}

	}

	/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	 * ivop/pvop/bvop selection
	 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */


	if (pEnc->iFrameNum == 0 || pFrame->intra == 1 || pEnc->bframenum_dx50bvop >= 0 ||
		(pFrame->intra < 0 && pEnc->iMaxKeyInterval > 0 &&
		 pEnc->iFrameNum >= pEnc->iMaxKeyInterval)
		|| image_mad(&pEnc->reference->image, &pEnc->current->image,
					 pEnc->mbParam.edged_width, pEnc->mbParam.width,
					 pEnc->mbParam.height) > 30) {
		/*
		 * This will be coded as an Intra Frame
		 */

		DPRINTF(DPRINTF_DEBUG,"*** IFRAME bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);
		
		if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "IVOP");
		}

		// when we reach an iframe in DX50BVOP mode, encode the last bframe as a pframe

		if ((pEnc->global & XVID_GLOBAL_DX50BVOP) && pEnc->bframenum_tail > 0) {

			pEnc->bframenum_tail--;
			pEnc->bframenum_dx50bvop = pEnc->bframenum_tail;

			SWAP(pEnc->current, pEnc->bframes[pEnc->bframenum_dx50bvop]);
			if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
				image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 100, "DX50 BVOP->PVOP");
			}
			FrameCodeP(pEnc, &bs, &bits, 1, 0);

			pFrame->intra = 0;
			
		} else {

			FrameCodeI(pEnc, &bs, &bits);
			pFrame->intra = 1;

			pEnc->bframenum_dx50bvop = -1;
		}

		pEnc->flush_bframes = 1;

		if ((pEnc->global & XVID_GLOBAL_PACKED) && pEnc->bframenum_tail > 0) {
			BitstreamPad(&bs);
			input_valid = 0;
			goto ipvop_loop;
		}

		/*
		 * NB : sequences like "IIBB" decode fine with msfdam but,
		 *      go screwy with divx 5.00
		 */
	} else if (pEnc->bframenum_tail >= pEnc->mbParam.max_bframes) {
		/*
		 * This will be coded as a Predicted Frame
		 */

		DPRINTF(DPRINTF_DEBUG,"*** PFRAME bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);
		
		if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "PVOP");
		}

		FrameCodeP(pEnc, &bs, &bits, 1, 0);
		pFrame->intra = 0;
		pEnc->flush_bframes = 1;

		if ((pEnc->global & XVID_GLOBAL_PACKED)) {
			BitstreamPad(&bs);
			input_valid = 0;
			goto ipvop_loop;
		}

	} else {
		/*
		 * This will be coded as a Bidirectional Frame
		 */

		if ((pEnc->global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "BVOP");
		}

		if (pFrame->bquant < 1) {
			pEnc->current->quant =
				((pEnc->reference->quant +
				  pEnc->current->quant) * pEnc->bquant_ratio) / 200;
		} else {
			pEnc->current->quant = pFrame->bquant;
		}
                if (pEnc->current->quant < 1)
                        pEnc->current->quant = 1;

                if (pEnc->current->quant > 31)
                        pEnc->current->quant = 31;
 

			DPRINTF(DPRINTF_DEBUG,"*** BFRAME (store) bf: head=%i tail=%i   queue: head=%i tail=%i size=%i  quant=%i\n",
                                pEnc->bframenum_head, pEnc->bframenum_tail,
                                pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size,pEnc->current->quant);



		/* store frame into bframe buffer & swap ref back to current */
		SWAP(pEnc->current, pEnc->bframes[pEnc->bframenum_tail]);
		SWAP(pEnc->current, pEnc->reference);

		pEnc->bframenum_tail++;

		pFrame->intra = 0;
		pFrame->length = 0;

		input_valid = 0;
		goto bvop_loop;
	}

	pEnc->iFrameNum++;

	BitstreamPad(&bs);
	pFrame->length = BitstreamLength(&bs);

	if (pResult) {
		pResult->quant = pEnc->current->quant;
		pResult->hlength = pFrame->length - (pEnc->sStat.iTextBits / 8);
		pResult->kblks = pEnc->sStat.kblks;
		pResult->mblks = pEnc->sStat.mblks;
		pResult->ublks = pEnc->sStat.ublks;
	}

	emms();

#ifdef _DEBUG_PSNR
	psnr =
		image_psnr(&pEnc->sOriginal, &pEnc->current->image,
				   pEnc->mbParam.edged_width, pEnc->mbParam.width,
				   pEnc->mbParam.height);

	snprintf(temp, 127, "PSNR: %f\n", psnr);
	DEBUG(temp);
#endif

	if (pFrame->quant == 0) {
		RateControlUpdate(&pEnc->rate_control, pEnc->current->quant,
						  pFrame->length, pFrame->intra);
	}


	stop_global_timer();
	write_timer();

	return XVID_ERR_OK;
}

#endif



/*****************************************************************************
 * "original" IP frame encoder entry point
 *
 * Returned values :
 *    - XVID_ERR_OK     - no errors
 *    - XVID_ERR_FORMAT - the image subsystem reported the image had a wrong
 *                        format
 ****************************************************************************/

int
encoder_encode(Encoder * pEnc,
			   XVID_ENC_FRAME * pFrame,
			   XVID_ENC_STATS * pResult)
{
	uint16_t x, y;
	Bitstream bs;
	uint32_t bits;
	uint16_t write_vol_header = 0;

#ifdef _DEBUG_PSNR
	float psnr;
	uint8_t temp[128];
#endif

	start_global_timer();

	ENC_CHECK(pEnc);
	ENC_CHECK(pFrame);
	ENC_CHECK(pFrame->bitstream);
	ENC_CHECK(pFrame->image);

	SWAP(pEnc->current, pEnc->reference);

	pEnc->current->global_flags = pFrame->general;
	pEnc->current->motion_flags = pFrame->motion;
#ifdef BFRAMES
	pEnc->current->seconds = pEnc->mbParam.m_seconds;
	pEnc->current->ticks = pEnc->mbParam.m_ticks;
#endif
	pEnc->mbParam.hint = &pFrame->hint;

	start_timer();
	if (image_input
		(&pEnc->current->image, pEnc->mbParam.width, pEnc->mbParam.height,
		 pEnc->mbParam.edged_width, pFrame->image, pFrame->colorspace) < 0)
		return XVID_ERR_FORMAT;
	stop_conv_timer();

#ifdef _DEBUG_PSNR
	image_copy(&pEnc->sOriginal, &pEnc->current->image,
			   pEnc->mbParam.edged_width, pEnc->mbParam.height);
#endif

	emms();

	BitstreamInit(&bs, pFrame->bitstream, 0);

	if (pFrame->quant == 0) {
		pEnc->current->quant = RateControlGetQ(&pEnc->rate_control, 0);
	} else {
		pEnc->current->quant = pFrame->quant;
	}

	if ((pEnc->current->global_flags & XVID_LUMIMASKING)) {
		int *temp_dquants =
			(int *) xvid_malloc(pEnc->mbParam.mb_width *
								pEnc->mbParam.mb_height * sizeof(int),
								CACHE_LINE);

		pEnc->current->quant =
			adaptive_quantization(pEnc->current->image.y,
								  pEnc->mbParam.edged_width, temp_dquants,
								  pEnc->current->quant, pEnc->current->quant,
								  2 * pEnc->current->quant,
								  pEnc->mbParam.mb_width,
								  pEnc->mbParam.mb_height);

		for (y = 0; y < pEnc->mbParam.mb_height; y++) {

#define OFFSET(x,y) ((x) + (y)*pEnc->mbParam.mb_width)

			for (x = 0; x < pEnc->mbParam.mb_width; x++) {


				MACROBLOCK *pMB = &pEnc->current->mbs[OFFSET(x, y)];

				pMB->dquant = iDQtab[temp_dquants[OFFSET(x, y)] + 2];
			}

#undef OFFSET
		}

		xvid_free(temp_dquants);
	}

	if (pEnc->current->global_flags & XVID_H263QUANT) {
		if (pEnc->mbParam.m_quant_type != H263_QUANT)
			write_vol_header = 1;
		pEnc->mbParam.m_quant_type = H263_QUANT;
	} else if (pEnc->current->global_flags & XVID_MPEGQUANT) {
		int matrix1_changed, matrix2_changed;

		matrix1_changed = matrix2_changed = 0;

		if (pEnc->mbParam.m_quant_type != MPEG4_QUANT)
			write_vol_header = 1;

		pEnc->mbParam.m_quant_type = MPEG4_QUANT;

		if ((pEnc->current->global_flags & XVID_CUSTOM_QMATRIX) > 0) {
			if (pFrame->quant_intra_matrix != NULL)
				matrix1_changed = set_intra_matrix(pFrame->quant_intra_matrix);
			if (pFrame->quant_inter_matrix != NULL)
				matrix2_changed = set_inter_matrix(pFrame->quant_inter_matrix);
		} else {
			matrix1_changed = set_intra_matrix(get_default_intra_matrix());
			matrix2_changed = set_inter_matrix(get_default_inter_matrix());
		}
		if (write_vol_header == 0)
			write_vol_header = matrix1_changed | matrix2_changed;
	}

	if (pFrame->intra < 0) {
		if ((pEnc->iFrameNum == 0)
			|| ((pEnc->iMaxKeyInterval > 0)
				&& (pEnc->iFrameNum >= pEnc->iMaxKeyInterval))) {
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
		} else {
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 0, write_vol_header);
		}
	} else {
		if (pFrame->intra == 1) {
			pFrame->intra = FrameCodeI(pEnc, &bs, &bits);
		} else {
			pFrame->intra = FrameCodeP(pEnc, &bs, &bits, 1, write_vol_header);
		}

	}

	BitstreamPutBits(&bs, 0xFFFF, 16);
	BitstreamPutBits(&bs, 0xFFFF, 16);
	BitstreamPad(&bs);
	pFrame->length = BitstreamLength(&bs);

	if (pResult) {
		pResult->quant = pEnc->current->quant;
		pResult->hlength = pFrame->length - (pEnc->sStat.iTextBits / 8);
		pResult->kblks = pEnc->sStat.kblks;
		pResult->mblks = pEnc->sStat.mblks;
		pResult->ublks = pEnc->sStat.ublks;
	}

	emms();

	if (pFrame->quant == 0) {
		RateControlUpdate(&pEnc->rate_control, pEnc->current->quant,
						  pFrame->length, pFrame->intra);
	}
#ifdef _DEBUG_PSNR
	psnr =
		image_psnr(&pEnc->sOriginal, &pEnc->current->image,
				   pEnc->mbParam.edged_width, pEnc->mbParam.width,
				   pEnc->mbParam.height);

	snprintf(temp, 127, "PSNR: %f\n", psnr);
	DEBUG(temp);
#endif

#ifdef BFRAMES
	inc_frame_num(pEnc);
#endif
	pEnc->iFrameNum++;

	stop_global_timer();
	write_timer();

	return XVID_ERR_OK;
}


static __inline void
CodeIntraMB(Encoder * pEnc,
			MACROBLOCK * pMB)
{

	pMB->mode = MODE_INTRA;

	/* zero mv statistics */
	pMB->mvs[0].x = pMB->mvs[1].x = pMB->mvs[2].x = pMB->mvs[3].x = 0;
	pMB->mvs[0].y = pMB->mvs[1].y = pMB->mvs[2].y = pMB->mvs[3].y = 0;
	pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = 0;
	pMB->sad16 = 0;

	if ((pEnc->current->global_flags & XVID_LUMIMASKING)) {
		if (pMB->dquant != NO_CHANGE) {
			pMB->mode = MODE_INTRA_Q;
			pEnc->current->quant += DQtab[pMB->dquant];

			if (pEnc->current->quant > 31)
				pEnc->current->quant = 31;
			if (pEnc->current->quant < 1)
				pEnc->current->quant = 1;
		}
	}

	pMB->quant = pEnc->current->quant;
}


#define FCODEBITS	3
#define MODEBITS	5

void
HintedMESet(Encoder * pEnc,
			int *intra)
{
	HINTINFO *hint;
	Bitstream bs;
	int length, high;
	uint32_t x, y;

	hint = pEnc->mbParam.hint;

	if (hint->rawhints) {
		*intra = hint->mvhint.intra;
	} else {
		BitstreamInit(&bs, hint->hintstream, hint->hintlength);
		*intra = BitstreamGetBit(&bs);
	}

	if (*intra) {
		return;
	}

	pEnc->current->fcode =
		(hint->rawhints) ? hint->mvhint.fcode : BitstreamGetBits(&bs,
																 FCODEBITS);

	length = pEnc->current->fcode + 5;
	high = 1 << (length - 1);

	for (y = 0; y < pEnc->mbParam.mb_height; ++y) {
		for (x = 0; x < pEnc->mbParam.mb_width; ++x) {
			MACROBLOCK *pMB =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];
			MVBLOCKHINT *bhint =
				&hint->mvhint.block[x + y * pEnc->mbParam.mb_width];
			VECTOR pred;
			VECTOR tmp;
			int vec;

			pMB->mode =
				(hint->rawhints) ? bhint->mode : BitstreamGetBits(&bs,
																  MODEBITS);

			pMB->mode = (pMB->mode == MODE_INTER_Q) ? MODE_INTER : pMB->mode;
			pMB->mode = (pMB->mode == MODE_INTRA_Q) ? MODE_INTRA : pMB->mode;

			if (pMB->mode == MODE_INTER) {
				tmp.x =
					(hint->rawhints) ? bhint->mvs[0].x : BitstreamGetBits(&bs,
																		  length);
				tmp.y =
					(hint->rawhints) ? bhint->mvs[0].y : BitstreamGetBits(&bs,
																		  length);
				tmp.x -= (tmp.x >= high) ? high * 2 : 0;
				tmp.y -= (tmp.y >= high) ? high * 2 : 0;

				pred = get_pmv2(pEnc->current->mbs,pEnc->mbParam.mb_width,0,x,y,0);

				for (vec = 0; vec < 4; ++vec) {
					pMB->mvs[vec].x = tmp.x;
					pMB->mvs[vec].y = tmp.y;
					pMB->pmvs[vec].x = pMB->mvs[0].x - pred.x;
					pMB->pmvs[vec].y = pMB->mvs[0].y - pred.y;
				}
			} else if (pMB->mode == MODE_INTER4V) {
				for (vec = 0; vec < 4; ++vec) {
					tmp.x =
						(hint->rawhints) ? bhint->mvs[vec].
						x : BitstreamGetBits(&bs, length);
					tmp.y =
						(hint->rawhints) ? bhint->mvs[vec].
						y : BitstreamGetBits(&bs, length);
					tmp.x -= (tmp.x >= high) ? high * 2 : 0;
					tmp.y -= (tmp.y >= high) ? high * 2 : 0;

					pred = get_pmv2(pEnc->current->mbs,pEnc->mbParam.mb_width,0,x,y,vec);

					pMB->mvs[vec].x = tmp.x;
					pMB->mvs[vec].y = tmp.y;
					pMB->pmvs[vec].x = pMB->mvs[vec].x - pred.x;
					pMB->pmvs[vec].y = pMB->mvs[vec].y - pred.y;
				}
			} else				// intra / stuffing / not_coded
			{
				for (vec = 0; vec < 4; ++vec) {
					pMB->mvs[vec].x = pMB->mvs[vec].y = 0;
				}
			}

			if (pMB->mode == MODE_INTER4V &&
				(pEnc->current->global_flags & XVID_LUMIMASKING)
				&& pMB->dquant != NO_CHANGE) {
				pMB->mode = MODE_INTRA;

				for (vec = 0; vec < 4; ++vec) {
					pMB->mvs[vec].x = pMB->mvs[vec].y = 0;
				}
			}
		}
	}
}


void
HintedMEGet(Encoder * pEnc,
			int intra)
{
	HINTINFO *hint;
	Bitstream bs;
	uint32_t x, y;
	int length, high;

	hint = pEnc->mbParam.hint;

	if (hint->rawhints) {
		hint->mvhint.intra = intra;
	} else {
		BitstreamInit(&bs, hint->hintstream, 0);
		BitstreamPutBit(&bs, intra);
	}

	if (intra) {
		if (!hint->rawhints) {
			BitstreamPad(&bs);
			hint->hintlength = BitstreamLength(&bs);
		}
		return;
	}

	length = pEnc->current->fcode + 5;
	high = 1 << (length - 1);

	if (hint->rawhints) {
		hint->mvhint.fcode = pEnc->current->fcode;
	} else {
		BitstreamPutBits(&bs, pEnc->current->fcode, FCODEBITS);
	}

	for (y = 0; y < pEnc->mbParam.mb_height; ++y) {
		for (x = 0; x < pEnc->mbParam.mb_width; ++x) {
			MACROBLOCK *pMB =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];
			MVBLOCKHINT *bhint =
				&hint->mvhint.block[x + y * pEnc->mbParam.mb_width];
			VECTOR tmp;

			if (hint->rawhints) {
				bhint->mode = pMB->mode;
			} else {
				BitstreamPutBits(&bs, pMB->mode, MODEBITS);
			}

			if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q) {
				tmp.x = pMB->mvs[0].x;
				tmp.y = pMB->mvs[0].y;
				tmp.x += (tmp.x < 0) ? high * 2 : 0;
				tmp.y += (tmp.y < 0) ? high * 2 : 0;

				if (hint->rawhints) {
					bhint->mvs[0].x = tmp.x;
					bhint->mvs[0].y = tmp.y;
				} else {
					BitstreamPutBits(&bs, tmp.x, length);
					BitstreamPutBits(&bs, tmp.y, length);
				}
			} else if (pMB->mode == MODE_INTER4V) {
				int vec;

				for (vec = 0; vec < 4; ++vec) {
					tmp.x = pMB->mvs[vec].x;
					tmp.y = pMB->mvs[vec].y;
					tmp.x += (tmp.x < 0) ? high * 2 : 0;
					tmp.y += (tmp.y < 0) ? high * 2 : 0;

					if (hint->rawhints) {
						bhint->mvs[vec].x = tmp.x;
						bhint->mvs[vec].y = tmp.y;
					} else {
						BitstreamPutBits(&bs, tmp.x, length);
						BitstreamPutBits(&bs, tmp.y, length);
					}
				}
			}
		}
	}

	if (!hint->rawhints) {
		BitstreamPad(&bs);
		hint->hintlength = BitstreamLength(&bs);
	}
}


static int
FrameCodeI(Encoder * pEnc,
		   Bitstream * bs,
		   uint32_t * pBits)
{

	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff, 6, 64, int16_t, CACHE_LINE);

	uint16_t x, y;

	pEnc->iFrameNum = 0;
	pEnc->mbParam.m_rounding_type = 1;
	pEnc->current->rounding_type = pEnc->mbParam.m_rounding_type;
	pEnc->current->coding_type = I_VOP;

	BitstreamWriteVolHeader(bs, &pEnc->mbParam, pEnc->current);
#ifdef BFRAMES
#define DIVX501B481P "DivX501b481p"
	if ((pEnc->global & XVID_GLOBAL_PACKED)) {
		BitstreamWriteUserData(bs, DIVX501B481P, strlen(DIVX501B481P));
	}
#endif
	BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 1);

	*pBits = BitstreamPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.kblks = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height;
	pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

	for (y = 0; y < pEnc->mbParam.mb_height; y++)
		for (x = 0; x < pEnc->mbParam.mb_width; x++) {
			MACROBLOCK *pMB =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];

			CodeIntraMB(pEnc, pMB);

			MBTransQuantIntra(&pEnc->mbParam, pEnc->current, pMB, x, y,
							  dct_codes, qcoeff);

			start_timer();
			MBPrediction(pEnc->current, x, y, pEnc->mbParam.mb_width, qcoeff);
			stop_prediction_timer();

			start_timer();
			if (pEnc->current->global_flags & XVID_GREYSCALE)
			{	pMB->cbp &= 0x3C;		/* keep only bits 5-2 */
				qcoeff[4*64+0]=0;		/* zero, because for INTRA MBs DC value is saved */
				qcoeff[5*64+0]=0;
			}
			MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->sStat);
			stop_coding_timer();
		}

	emms();

	*pBits = BitstreamPos(bs) - *pBits;
	pEnc->sStat.fMvPrevSigma = -1;
	pEnc->sStat.iMvSum = 0;
	pEnc->sStat.iMvCount = 0;
	pEnc->mbParam.m_fcode = 2;

	if (pEnc->current->global_flags & XVID_HINTEDME_GET) {
		HintedMEGet(pEnc, 1);
	}

	return 1;					// intra
}


#define INTRA_THRESHOLD 0.5
#define BFRAME_SKIP_THRESHHOLD 16

static int
FrameCodeP(Encoder * pEnc,
		   Bitstream * bs,
		   uint32_t * pBits,
		   bool force_inter,
		   bool vol_header)
{
	float fSigma;

	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff, 6, 64, int16_t, CACHE_LINE);

	int iLimit;
	int x, y, k;
	int iSearchRange;
	int bIntra;
	
	/* IMAGE *pCurrent = &pEnc->current->image; */
	IMAGE *pRef = &pEnc->reference->image;

	start_timer();
	image_setedges(pRef, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
				   pEnc->mbParam.width, pEnc->mbParam.height,
				   pEnc->current->global_flags & XVID_INTERLACING);
	stop_edges_timer();

	pEnc->mbParam.m_rounding_type = 1 - pEnc->mbParam.m_rounding_type;
	pEnc->current->rounding_type = pEnc->mbParam.m_rounding_type;
	pEnc->current->fcode = pEnc->mbParam.m_fcode;

	if (!force_inter)
		iLimit =
			(int) (pEnc->mbParam.mb_width * pEnc->mbParam.mb_height *
				   INTRA_THRESHOLD);
	else
		iLimit = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height + 1;

	if ((pEnc->current->global_flags & XVID_HALFPEL)) {
		start_timer();
		image_interpolate(pRef, &pEnc->vInterH, &pEnc->vInterV,
						  &pEnc->vInterHV, pEnc->mbParam.edged_width,
						  pEnc->mbParam.edged_height,
						  pEnc->current->rounding_type);
		stop_inter_timer();
	}

	start_timer();
	if (pEnc->current->global_flags & XVID_HINTEDME_SET) {
		HintedMESet(pEnc, &bIntra);
	} else {

#ifdef _SMP
	if (pEnc->mbParam.num_threads > 1)
		bIntra =
			SMP_MotionEstimation(&pEnc->mbParam, pEnc->current, pEnc->reference,
						 &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
						 iLimit);
	else
#endif
		bIntra =
			MotionEstimation(&pEnc->mbParam, pEnc->current, pEnc->reference,
                         &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
                         iLimit);
			
	}
	stop_motion_timer();

	if (bIntra == 1) {
		return FrameCodeI(pEnc, bs, pBits);
	}

	pEnc->current->coding_type = P_VOP;

	if (vol_header)
		BitstreamWriteVolHeader(bs, &pEnc->mbParam, pEnc->current);

	BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 1);

	*pBits = BitstreamPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.iMvSum = 0;
	pEnc->sStat.iMvCount = 0;
	pEnc->sStat.kblks = pEnc->sStat.mblks = pEnc->sStat.ublks = 0;

	for (y = 0; y < pEnc->mbParam.mb_height; y++) {
		for (x = 0; x < pEnc->mbParam.mb_width; x++) {
			MACROBLOCK *pMB =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];

			bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

			if (!bIntra) {
				start_timer();
				MBMotionCompensation(pMB, x, y, &pEnc->reference->image,
									 &pEnc->vInterH, &pEnc->vInterV,
									 &pEnc->vInterHV, &pEnc->current->image,
									 dct_codes, pEnc->mbParam.width,
									 pEnc->mbParam.height,
									 pEnc->mbParam.edged_width,
									 pEnc->current->rounding_type);
				stop_comp_timer();

				if ((pEnc->current->global_flags & XVID_LUMIMASKING)) {
					if (pMB->dquant != NO_CHANGE) {
						pMB->mode = MODE_INTER_Q;
						pEnc->current->quant += DQtab[pMB->dquant];
						if (pEnc->current->quant > 31)
							pEnc->current->quant = 31;
						else if (pEnc->current->quant < 1)
							pEnc->current->quant = 1;
					}
				}
				pMB->quant = pEnc->current->quant;

				pMB->field_pred = 0;

				pMB->cbp =
					MBTransQuantInter(&pEnc->mbParam, pEnc->current, pMB, x, y,
									  dct_codes, qcoeff);
			} else {
				CodeIntraMB(pEnc, pMB);
				MBTransQuantIntra(&pEnc->mbParam, pEnc->current, pMB, x, y,
								  dct_codes, qcoeff);
			}

			start_timer();
			MBPrediction(pEnc->current, x, y, pEnc->mbParam.mb_width, qcoeff);
			stop_prediction_timer();

			if (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q) {
				pEnc->sStat.kblks++;
			} else if (pMB->cbp || pMB->mvs[0].x || pMB->mvs[0].y ||
					   pMB->mvs[1].x || pMB->mvs[1].y || pMB->mvs[2].x ||
					   pMB->mvs[2].y || pMB->mvs[3].x || pMB->mvs[3].y) {
				pEnc->sStat.mblks++;
			}  else {
				pEnc->sStat.ublks++;
			} 

			start_timer();

			/* Finished processing the MB, now check if to CODE or SKIP */

			if (pMB->cbp == 0 && pMB->mode == MODE_INTER && pMB->mvs[0].x == 0 &&
				pMB->mvs[0].y == 0) {

/* This is a candidate for SKIPping, but check intermediate B-frames first */

#ifdef BFRAMES
				int iSAD=BFRAME_SKIP_THRESHHOLD;
				int bSkip=1;
				
				for (k=pEnc->bframenum_head; k< pEnc->bframenum_tail; k++)
				{		
					iSAD = sad16(pEnc->reference->image.y + 16*y*pEnc->mbParam.edged_width + 16*x,
								pEnc->bframes[k]->image.y + 16*y*pEnc->mbParam.edged_width + 16*x,
								pEnc->mbParam.edged_width,BFRAME_SKIP_THRESHHOLD);
					if (iSAD >= BFRAME_SKIP_THRESHHOLD)
					{	bSkip = 0; 
						break;
					}
				}
				if (!bSkip) 
				{	
					if (pEnc->current->global_flags & XVID_GREYSCALE)
					{	pMB->cbp &= 0x3C;		/* keep only bits 5-2 */
						qcoeff[4*64+0]=0;		/* zero, because DC for INTRA MBs DC value is saved */
						qcoeff[5*64+0]=0;
					}
					MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->sStat);
					pMB->cbp = 0x80;		/* trick! so cbp!=0, but still nothing is written to bs */
				}
				else 
					MBSkip(bs);

#else
					MBSkip(bs);	/* without B-frames, no precautions are needed */

#endif

			} else {
				if (pEnc->current->global_flags & XVID_GREYSCALE)
				{	pMB->cbp &= 0x3C;		/* keep only bits 5-2 */
					qcoeff[4*64+0]=0;		/* zero, because DC for INTRA MBs DC value is saved */
					qcoeff[5*64+0]=0;
				}
				MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->sStat);
			}

			stop_coding_timer();
		}
	}

	emms();

	if (pEnc->current->global_flags & XVID_HINTEDME_GET) {
		HintedMEGet(pEnc, 0);
	}

	if (pEnc->sStat.iMvCount == 0)
		pEnc->sStat.iMvCount = 1;

	fSigma = (float) sqrt((float) pEnc->sStat.iMvSum / pEnc->sStat.iMvCount);

	iSearchRange = 1 << (3 + pEnc->mbParam.m_fcode);

	if ((fSigma > iSearchRange / 3)
		&& (pEnc->mbParam.m_fcode <= 3))	// maximum search range 128
	{
		pEnc->mbParam.m_fcode++;
		iSearchRange *= 2;
	} else if ((fSigma < iSearchRange / 6)
			   && (pEnc->sStat.fMvPrevSigma >= 0)
			   && (pEnc->sStat.fMvPrevSigma < iSearchRange / 6)
			   && (pEnc->mbParam.m_fcode >= 2))	// minimum search range 16
	{
		pEnc->mbParam.m_fcode--;
		iSearchRange /= 2;
	}

	pEnc->sStat.fMvPrevSigma = fSigma;

#ifdef BFRAMES
	/* frame drop code */
	// DPRINTF(DPRINTF_DEBUG, "kmu %i %i %i", pEnc->sStat.kblks, pEnc->sStat.mblks, pEnc->sStat.ublks);
	if (pEnc->sStat.kblks + pEnc->sStat.mblks <
		(pEnc->frame_drop_ratio * pEnc->mbParam.mb_width * pEnc->mbParam.mb_height) / 100)
	{
		pEnc->sStat.kblks = pEnc->sStat.mblks = 0;
		pEnc->sStat.ublks = pEnc->mbParam.mb_width * pEnc->mbParam.mb_height;

		BitstreamReset(bs);
		BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 0);

		// copy reference frame details into the current frame
		pEnc->current->quant = pEnc->reference->quant;
		pEnc->current->motion_flags = pEnc->reference->motion_flags;
		pEnc->current->rounding_type = pEnc->reference->rounding_type;
		pEnc->current->fcode = pEnc->reference->fcode;
		pEnc->current->bcode = pEnc->reference->bcode;
		image_copy(&pEnc->current->image, &pEnc->reference->image, pEnc->mbParam.edged_width, pEnc->mbParam.height);
		memcpy(pEnc->current->mbs, pEnc->reference->mbs, sizeof(MACROBLOCK) * pEnc->mbParam.mb_width * pEnc->mbParam.mb_height);

	}
#endif

	*pBits = BitstreamPos(bs) - *pBits;

#ifdef BFRAMES
	pEnc->time_pp = ((int32_t)pEnc->mbParam.fbase - (int32_t)pEnc->last_pframe + (int32_t)pEnc->current->ticks) %
			(int32_t)pEnc->mbParam.fbase;
	pEnc->last_pframe = pEnc->current->ticks;
#endif

	return 0;					// inter
}


#ifdef BFRAMES
static void
FrameCodeB(Encoder * pEnc,
		   FRAMEINFO * frame,
		   Bitstream * bs,
		   uint32_t * pBits)
{
	int16_t dct_codes[6 * 64];
	int16_t qcoeff[6 * 64];
	uint32_t x, y;
	VECTOR forward;
	VECTOR backward;

	IMAGE *f_ref = &pEnc->reference->image;
	IMAGE *b_ref = &pEnc->current->image;

#ifdef BFRAMES_DEC_DEBUG
	FILE *fp;
	static char first=0;
#define BFRAME_DEBUG  	if (!first && fp){ \
		fprintf(fp,"Y=%3d   X=%3d   MB=%2d   CBP=%02X\n",y,x,mb->mode,mb->cbp); \
	}

	if (!first){
		fp=fopen("C:\\XVIDDBGE.TXT","w");
	}
#endif

	// forward 
	image_setedges(f_ref, pEnc->mbParam.edged_width,
				   pEnc->mbParam.edged_height, pEnc->mbParam.width,
				   pEnc->mbParam.height,
				   frame->global_flags & XVID_INTERLACING);
	start_timer();
	image_interpolate(f_ref, &pEnc->f_refh, &pEnc->f_refv, &pEnc->f_refhv,
					  pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
					  0);
	stop_inter_timer();

	// backward
	image_setedges(b_ref, pEnc->mbParam.edged_width,
				   pEnc->mbParam.edged_height, pEnc->mbParam.width,
				   pEnc->mbParam.height,
				   frame->global_flags & XVID_INTERLACING);
	start_timer();
	image_interpolate(b_ref, &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
					  pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
					  0);
	stop_inter_timer();

	start_timer();
	MotionEstimationBVOP(&pEnc->mbParam, frame, 
		((int32_t)pEnc->mbParam.fbase + pEnc->last_pframe - frame->ticks) % pEnc->mbParam.fbase,
						pEnc->time_pp, 
						pEnc->reference->mbs, f_ref,
						 &pEnc->f_refh, &pEnc->f_refv, &pEnc->f_refhv,
						 pEnc->current->mbs, b_ref, &pEnc->vInterH,
						 &pEnc->vInterV, &pEnc->vInterHV);


	stop_motion_timer();

	/*if (test_quant_type(&pEnc->mbParam, pEnc->current))
	   {
	   BitstreamWriteVolHeader(bs, pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.quant_type);
	   } */

	frame->coding_type = B_VOP;
	BitstreamWriteVopHeader(bs, &pEnc->mbParam, frame, 1);

	*pBits = BitstreamPos(bs);

	pEnc->sStat.iTextBits = 0;
	pEnc->sStat.iMvSum = 0;
	pEnc->sStat.iMvCount = 0;
	pEnc->sStat.kblks = pEnc->sStat.mblks = pEnc->sStat.ublks = 0;


	for (y = 0; y < pEnc->mbParam.mb_height; y++) {
		// reset prediction

		forward.x = 0;
		forward.y = 0;
		backward.x = 0;
		backward.y = 0;

		for (x = 0; x < pEnc->mbParam.mb_width; x++) {
			MACROBLOCK *f_mb =
				&pEnc->reference->mbs[x + y * pEnc->mbParam.mb_width];
			MACROBLOCK *b_mb =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];
			MACROBLOCK *mb = &frame->mbs[x + y * pEnc->mbParam.mb_width];

			// decoder ignores mb when refence block is INTER(0,0), CBP=0 
			if (mb->mode == MODE_NOT_CODED) {
				mb->mvs[0].x = 0;
				mb->mvs[0].y = 0;

				mb->cbp = 0;
#ifdef BFRAMES_DEC_DEBUG
	BFRAME_DEBUG
#endif
				continue;
			}

			MBMotionCompensationBVOP(&pEnc->mbParam, mb, x, y, &frame->image,
									 f_ref, &pEnc->f_refh, &pEnc->f_refv,
									 &pEnc->f_refhv, b_ref, &pEnc->vInterH,
									 &pEnc->vInterV, &pEnc->vInterHV,
									 dct_codes);

			mb->quant = frame->quant;
			mb->cbp =
				MBTransQuantInter(&pEnc->mbParam, frame, mb, x, y, dct_codes,
								  qcoeff);
			//mb->cbp = MBTransQuantBVOP(&pEnc->mbParam, x, y, dct_codes, qcoeff, &frame->image, frame->quant);

			if ( (mb->mode == MODE_DIRECT) && (mb->cbp == 0)
				&& (mb->deltamv.x == 0) && (mb->deltamv.y == 0) ) {
				mb->mode = MODE_DIRECT_NONE_MV;	// skipped
			}

/* update predictors for forward and backward vectors */
			if (mb->mode == MODE_INTERPOLATE || mb->mode == MODE_FORWARD) {
				mb->pmvs[0].x = mb->mvs[0].x - forward.x;
				mb->pmvs[0].y = mb->mvs[0].y - forward.y;
				forward.x = mb->mvs[0].x;
				forward.y = mb->mvs[0].y;
			}

			if (mb->mode == MODE_INTERPOLATE || mb->mode == MODE_BACKWARD) {
				mb->b_pmvs[0].x = mb->b_mvs[0].x - backward.x;
				mb->b_pmvs[0].y = mb->b_mvs[0].y - backward.y;
				backward.x = mb->b_mvs[0].x;
				backward.y = mb->b_mvs[0].y;
			}
			
//			DPRINTF("%05i : [%i %i] M=%i CBP=%i MVS=%i,%i forward=%i,%i", pEnc->m_framenum, x, y, mb->mode, mb->cbp, mb->mvs[0].x, mb->mvs[0].y, forward.x, forward.y);

#ifdef BFRAMES_DEC_DEBUG
	BFRAME_DEBUG
#endif
			start_timer();
			MBCodingBVOP(mb, qcoeff, frame->fcode, frame->bcode, bs,
						 &pEnc->sStat);
			stop_coding_timer();
		}
	}

	emms();

	// TODO: dynamic fcode/bcode ???

	*pBits = BitstreamPos(bs) - *pBits;

#ifdef BFRAMES_DEC_DEBUG
	if (!first){
		first=1;
		if (fp)
			fclose(fp);
	}
#endif
}
#endif


/*	in case internal output is needed somewhere... */
/*	{
        FILE *filehandle;
        filehandle=fopen("last-b.pgm","wb");
        if (filehandle)
        {
                fprintf(filehandle,"P5\n\n");           //
                fprintf(filehandle,"%d %d 255\n",pEnc->mbParam.edged_width,pEnc->mbParam.edged_height);
                fwrite(frame->image.y,pEnc->mbParam.edged_width,pEnc->mbParam.edged_height,filehandle);
                fclose(filehandle);
		}
	}
*/
