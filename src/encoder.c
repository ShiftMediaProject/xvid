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
 *  $Id: encoder.c,v 1.93 2003-02-17 23:45:21 edgomez Exp $
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
#include "image/font.h"
#include "motion/sad.h"
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

/*****************************************************************************
 * Local macros
 ****************************************************************************/

#define ENC_CHECK(X) if(!(X)) return XVID_ERR_FORMAT
#define SWAP(_T_,A,B)    { _T_ tmp = A; A = B; B = tmp; }

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

static void FrameCodeB(Encoder * pEnc,
					   FRAMEINFO * frame,
					   Bitstream * bs,
					   uint32_t * pBits);

/*****************************************************************************
 * Local data
 ****************************************************************************/

static int DQtab[4] = {
	-1, -2, 1, 2
};

static int iDQtab[5] = {
	1, 0, NO_CHANGE, 2, 3
};


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

	pEnc->fMvPrevSigma = -1;

	/* Fill rate control parameters */

	pEnc->bitrate = pParam->rc_bitrate;

	pEnc->iFrameNum = -1;
	pEnc->mbParam.iMaxKeyInterval = pParam->max_key_interval;

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

	if (pParam->global & XVID_GLOBAL_EXTRASTATS)
		image_null(&pEnc->sOriginal);

	image_null(&pEnc->f_refh);
	image_null(&pEnc->f_refv);
	image_null(&pEnc->f_refhv);

	image_null(&pEnc->current->image);
	image_null(&pEnc->reference->image);
	image_null(&pEnc->vInterH);
	image_null(&pEnc->vInterV);
	image_null(&pEnc->vInterVf);
	image_null(&pEnc->vInterHV);
	image_null(&pEnc->vInterHVf);

	if (pParam->global & XVID_GLOBAL_EXTRASTATS)
	{	if (image_create
			(&pEnc->sOriginal, pEnc->mbParam.edged_width,
			 pEnc->mbParam.edged_height) < 0)
			goto xvid_err_memory3;
	}

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

/* Create full bitplane for GMC, this might be wasteful */
	if (image_create
		(&pEnc->vGMC, pEnc->mbParam.edged_width,
		 pEnc->mbParam.edged_height) < 0)
		goto xvid_err_memory3;



	pEnc->mbParam.global = pParam->global;

	/* B Frames specific init */
	pEnc->mbParam.max_bframes = pParam->max_bframes;
	pEnc->mbParam.bquant_ratio = pParam->bquant_ratio;
	pEnc->mbParam.bquant_offset = pParam->bquant_offset;
	pEnc->mbParam.frame_drop_ratio = pParam->frame_drop_ratio;
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

	pEnc->mbParam.m_stamp = 0;

	pEnc->m_framenum = 0;
	pEnc->current->stamp = 0;
	pEnc->reference->stamp = 0;

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

  xvid_err_memory3:

	if (pEnc->mbParam.global & XVID_GLOBAL_EXTRASTATS)
	{	image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width,
					  pEnc->mbParam.edged_height);
	}

	image_destroy(&pEnc->f_refh, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refhv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);

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

/* destroy GMC image */
	image_destroy(&pEnc->vGMC, pEnc->mbParam.edged_width,
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
	int i;
	
	ENC_CHECK(pEnc);

	/* B Frames specific */
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

	image_destroy(&pEnc->f_refh, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);
	image_destroy(&pEnc->f_refhv, pEnc->mbParam.edged_width,
				  pEnc->mbParam.edged_height);

	if (pEnc->mbParam.global & XVID_GLOBAL_EXTRASTATS)
	{	image_destroy(&pEnc->sOriginal, pEnc->mbParam.edged_width,
					  pEnc->mbParam.edged_height);
	}

	/* Encoder structure */

	xvid_free(pEnc->current->mbs);
	xvid_free(pEnc->current);

	xvid_free(pEnc->reference->mbs);
	xvid_free(pEnc->reference);

	xvid_free(pEnc);

	return XVID_ERR_OK;
}


static __inline void inc_frame_num(Encoder * pEnc)
{
	pEnc->current->stamp = pEnc->mbParam.m_stamp;	/* first frame is zero */
	pEnc->mbParam.m_stamp += pEnc->mbParam.fincr;
}


static __inline void
queue_image(Encoder * pEnc, XVID_ENC_FRAME * pFrame)
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
		 pEnc->mbParam.edged_width, pFrame->image, pFrame->stride, pFrame->colorspace, pFrame->general & XVID_INTERLACING))
		return;
	stop_conv_timer();

	if ((pFrame->general & XVID_CHROMAOPT)) {
		image_chroma_optimize(&pEnc->queue[pEnc->queue_tail], 
			pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.edged_width);
	}

	pEnc->queue_size++;
	pEnc->queue_tail =  (pEnc->queue_tail + 1) % pEnc->mbParam.max_bframes;
}

static __inline void 
set_timecodes(FRAMEINFO* pCur,FRAMEINFO *pRef, int32_t time_base)
{

		pCur->ticks = (int32_t)pCur->stamp % time_base;
		pCur->seconds =  ((int32_t)pCur->stamp / time_base)	- ((int32_t)pRef->stamp / time_base) ;
		
		/* HEAVY DEBUG OUTPUT remove when timecodes prove to be stable */

/*		fprintf(stderr,"WriteVop:   %d - %d \n",
			((int32_t)pCur->stamp / time_base), ((int32_t)pRef->stamp / time_base));
		fprintf(stderr,"set_timecodes: VOP %1d   stamp=%lld ref_stamp=%lld  base=%d\n",
			pCur->coding_type, pCur->stamp, pRef->stamp, time_base);
		fprintf(stderr,"set_timecodes: VOP %1d   seconds=%d   ticks=%d   (ref-sec=%d  ref-tick=%d)\n",
			pCur->coding_type, pCur->seconds, pCur->ticks, pRef->seconds, pRef->ticks);

*/
}



/* convert pFrame->intra to coding_type */
static int intra2coding_type(int intra)
{
	if (intra < 0)  return -1;
	if (intra == 1) return I_VOP;
	if (intra == 2) return B_VOP;

	return P_VOP;
}



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
	int mode;

	int input_valid = 1;
	int bframes_count = 0;

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
			SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);

			SWAP(FRAMEINFO *, pEnc->current, pEnc->bframes[pEnc->bframenum_tail]);

			FrameCodeP(pEnc, &bs, &bits, 1, 0);
			bframes_count = 0;

			BitstreamPadAlways(&bs);
			pFrame->length = BitstreamLength(&bs);
			pFrame->intra = 0;


			emms();

			if (pResult) {
				pResult->quant = pEnc->current->quant;
				pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
				pResult->kblks = pEnc->current->sStat.kblks;
				pResult->mblks = pEnc->current->sStat.mblks;
				pResult->ublks = pEnc->current->sStat.ublks;
			}

			return XVID_ERR_OK;
		}

		
		DPRINTF(DPRINTF_DEBUG,"*** BFRAME (flush) bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

		FrameCodeB(pEnc, pEnc->bframes[pEnc->bframenum_head], &bs, &bits);
		pEnc->bframenum_head++;

		BitstreamPadAlways(&bs);
		pFrame->length = BitstreamLength(&bs);
		pFrame->intra = 2;

		if (pResult) {
			pResult->quant = pEnc->current->quant;
			pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
			pResult->kblks = pEnc->current->sStat.kblks;
			pResult->mblks = pEnc->current->sStat.mblks;
			pResult->ublks = pEnc->current->sStat.ublks;
		}

		if (input_valid)
			queue_image(pEnc, pFrame);

		emms();

		return XVID_ERR_OK;
	}

	if (pEnc->bframenum_head > 0) {
		pEnc->bframenum_head = pEnc->bframenum_tail = 0;

		/* write an empty marker to the bitstream.
		   
		   for divx5 decoder compatibility, this marker must consist
		   of a not-coded p-vop, with a time_base of zero, and time_increment 
		   indentical to the future-referece frame.
		*/

		if ((pEnc->mbParam.global & XVID_GLOBAL_PACKED)) {
			int tmp;
			
			DPRINTF(DPRINTF_DEBUG,"*** EMPTY bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);


			tmp = pEnc->current->seconds;
			pEnc->current->seconds = 0; /* force time_base = 0 */

			BitstreamWriteVopHeader(&bs, &pEnc->mbParam, pEnc->current, 0);
			pEnc->current->seconds = tmp;

			BitstreamPadAlways(&bs);
			pFrame->length = BitstreamLength(&bs);
			pFrame->intra = 4;

			if (pResult) {
				pResult->quant = pEnc->current->quant;
				pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
				pResult->kblks = pEnc->current->sStat.kblks;
				pResult->mblks = pEnc->current->sStat.mblks;
				pResult->ublks = pEnc->current->sStat.ublks;
			}

			if (input_valid)
				queue_image(pEnc, pFrame);

			emms();

			return XVID_ERR_OK;
		}
	}


bvop_loop:

	if (pEnc->bframenum_dx50bvop != -1)
	{

		SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);
		SWAP(FRAMEINFO *, pEnc->current, pEnc->bframes[pEnc->bframenum_dx50bvop]);		

		if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 100, "DX50 IVOP");
		}

		if (input_valid)
		{
			queue_image(pEnc, pFrame);
			input_valid = 0;
		}

	} else if (input_valid) {

		SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);

		start_timer();
		if (image_input
			(&pEnc->current->image, pEnc->mbParam.width, pEnc->mbParam.height,
			pEnc->mbParam.edged_width, pFrame->image, pFrame->stride, pFrame->colorspace, pFrame->general & XVID_INTERLACING))
		{
			emms();
			return XVID_ERR_FORMAT;
		}
		stop_conv_timer();

		if ((pFrame->general & XVID_CHROMAOPT)) {
			image_chroma_optimize(&pEnc->current->image, 
				pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.edged_width);
		}

		/* queue input frame, and dequue next image */
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
		
		SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);

		image_swap(&pEnc->current->image, &pEnc->queue[pEnc->queue_head]);
		pEnc->queue_head =  (pEnc->queue_head + 1) % pEnc->mbParam.max_bframes;
		pEnc->queue_size--;

	} else {

		/* if nothing was encoded, write an 'ignore this frame' flag
		   to the bitstream */

		if (BitstreamPos(&bs) == 0) {

			DPRINTF(DPRINTF_DEBUG,"*** SKIP bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);

			/* That disabled line of code was supposed to inform VirtualDub
			 * that the frame was a dummy delay frame - now disabled (thx god :-)
			 */
			/* BitstreamPutBits(&bs, 0x7f, 8); */
			pFrame->intra = 5;

			if (pResult) {
				/*
				 * We must decide what to put there because i know some apps
				 * are storing statistics about quantizers and just do
				 * stats[quant]++ or stats[quant-1]++
				 * transcode is one of these app with its 2pass module
				 */

				/*
				 * For now i prefer 31 than 0 that could lead to a segfault
				 * in transcode
				 */
				pResult->quant = 31;

				pResult->hlength = 0;
				pResult->kblks = 0;
				pResult->mblks = 0;
				pResult->ublks = 0;
			}

		} else {

			if (pResult) {
				pResult->quant = pEnc->current->quant;
				pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
				pResult->kblks = pEnc->current->sStat.kblks;
				pResult->mblks = pEnc->current->sStat.mblks;
				pResult->ublks = pEnc->current->sStat.ublks;
			}

		}

		pFrame->length = BitstreamLength(&bs);

		emms();

		return XVID_ERR_OK;
	}

	pEnc->flush_bframes = 0;

	emms();

	/* only inc frame num, adapt quant, etc. if we havent seen it before */
	if (pEnc->bframenum_dx50bvop < 0 )
	{
		mode = intra2coding_type(pFrame->intra);
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

		inc_frame_num(pEnc);

		if (pFrame->general & XVID_EXTRASTATS)
		{	image_copy(&pEnc->sOriginal, &pEnc->current->image,
				   pEnc->mbParam.edged_width, pEnc->mbParam.height);
		}

		emms();

		if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 5, 
				"%i  if:%i  st:%i", pEnc->m_framenum++, pEnc->iFrameNum, pEnc->current->stamp);
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
	pEnc->iFrameNum++;

	if (pEnc->iFrameNum == 0 || pEnc->bframenum_dx50bvop >= 0 ||
		(mode < 0 && pEnc->mbParam.iMaxKeyInterval > 0 && 
			pEnc->iFrameNum >= pEnc->mbParam.iMaxKeyInterval))
	{
		mode = I_VOP;
	}else{
		mode = MEanalysis(&pEnc->reference->image, pEnc->current,
					&pEnc->mbParam, pEnc->mbParam.iMaxKeyInterval,
					(mode < 0) ? pEnc->iFrameNum : 0,
					bframes_count++);
	}

	if (mode == I_VOP) {
		/*
		 * This will be coded as an Intra Frame
		 */
		if ((pEnc->current->global_flags & XVID_QUARTERPEL))
			pEnc->mbParam.m_quarterpel = 1;
		else
			pEnc->mbParam.m_quarterpel = 0;

		if (pEnc->current->global_flags & XVID_MPEGQUANT) pEnc->mbParam.m_quant_type = MPEG4_QUANT;

		if ((pEnc->current->global_flags & XVID_CUSTOM_QMATRIX) > 0) {
			if (pFrame->quant_intra_matrix != NULL)
				set_intra_matrix(pFrame->quant_intra_matrix);
			if (pFrame->quant_inter_matrix != NULL)
				set_inter_matrix(pFrame->quant_inter_matrix);
		}


		DPRINTF(DPRINTF_DEBUG,"*** IFRAME bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);
		
		if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "IVOP");
		}

		/* when we reach an iframe in DX50BVOP mode, encode the last bframe as a pframe */

		if ((pEnc->mbParam.global & XVID_GLOBAL_DX50BVOP) && pEnc->bframenum_tail > 0) {

			pEnc->bframenum_tail--;
			pEnc->bframenum_dx50bvop = pEnc->bframenum_tail;

			SWAP(FRAMEINFO *, pEnc->current, pEnc->bframes[pEnc->bframenum_dx50bvop]);
			if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
				image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 100, "DX50 BVOP->PVOP");
			}
			FrameCodeP(pEnc, &bs, &bits, 1, 0);
			bframes_count = 0;
			pFrame->intra = 0;

		} else {

			FrameCodeI(pEnc, &bs, &bits);
			bframes_count = 0;
			pFrame->intra = 1;

			pEnc->bframenum_dx50bvop = -1;
		}

		pEnc->flush_bframes = 1;

		if ((pEnc->mbParam.global & XVID_GLOBAL_PACKED) && pEnc->bframenum_tail > 0) {
			BitstreamPadAlways(&bs);
			input_valid = 0;
			goto ipvop_loop;
		}

		/*
		 * NB : sequences like "IIBB" decode fine with msfdam but,
		 *      go screwy with divx 5.00
		 */
	} else if (mode == P_VOP || mode == S_VOP || pEnc->bframenum_tail >= pEnc->mbParam.max_bframes) {
		/*
		 * This will be coded as a Predicted Frame
		 */

		DPRINTF(DPRINTF_DEBUG,"*** PFRAME bf: head=%i tail=%i   queue: head=%i tail=%i size=%i",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size);
		
		if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "PVOP");
		}

		FrameCodeP(pEnc, &bs, &bits, 1, 0);
		bframes_count = 0;
		pFrame->intra = 0;
		pEnc->flush_bframes = 1;

		if ((pEnc->mbParam.global & XVID_GLOBAL_PACKED) && (pEnc->bframenum_tail > 0)) {
			BitstreamPadAlways(&bs);
			input_valid = 0;
			goto ipvop_loop;
		}

	} else {	/* mode == B_VOP */
		/*
		 * This will be coded as a Bidirectional Frame
		 */

		if ((pEnc->mbParam.global & XVID_GLOBAL_DEBUG)) {
			image_printf(&pEnc->current->image, pEnc->mbParam.edged_width, pEnc->mbParam.height, 5, 200, "BVOP");
		}

		if (pFrame->bquant < 1) {
			pEnc->current->quant = ((((pEnc->reference->quant + pEnc->current->quant) * 
				pEnc->mbParam.bquant_ratio) / 2) + pEnc->mbParam.bquant_offset)/100;

		} else {
			pEnc->current->quant = pFrame->bquant;
		}

		if (pEnc->current->quant < 1)
			pEnc->current->quant = 1;
		else if (pEnc->current->quant > 31)
            pEnc->current->quant = 31;
 
		DPRINTF(DPRINTF_DEBUG,"*** BFRAME (store) bf: head=%i tail=%i   queue: head=%i tail=%i size=%i  quant=%i\n",
				pEnc->bframenum_head, pEnc->bframenum_tail,
				pEnc->queue_head, pEnc->queue_tail, pEnc->queue_size,pEnc->current->quant);

		/* store frame into bframe buffer & swap ref back to current */
		SWAP(FRAMEINFO *, pEnc->current, pEnc->bframes[pEnc->bframenum_tail]);
		SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);

		pEnc->bframenum_tail++;

		/* bframe report by koepi */
		pFrame->intra = 2;
		pFrame->length = 0;

		input_valid = 0;
		goto bvop_loop;
	}

	BitstreamPadAlways(&bs);
	pFrame->length = BitstreamLength(&bs);

	if (pResult) {
		pResult->quant = pEnc->current->quant;
		pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
		pResult->kblks = pEnc->current->sStat.kblks;
		pResult->mblks = pEnc->current->sStat.mblks;
		pResult->ublks = pEnc->current->sStat.ublks;

		if (pFrame->general & XVID_EXTRASTATS)
		{	pResult->sse_y =
				plane_sse( pEnc->sOriginal.y, pEnc->current->image.y,
						   pEnc->mbParam.edged_width, pEnc->mbParam.width,
						   pEnc->mbParam.height);

			pResult->sse_u =
				plane_sse( pEnc->sOriginal.u, pEnc->current->image.u,
						   pEnc->mbParam.edged_width/2, pEnc->mbParam.width/2,
						   pEnc->mbParam.height/2);

			pResult->sse_v =
				plane_sse( pEnc->sOriginal.v, pEnc->current->image.v,
						   pEnc->mbParam.edged_width/2, pEnc->mbParam.width/2,
						   pEnc->mbParam.height/2);		
		}
	}

	emms();

	if (pFrame->quant == 0) {
		RateControlUpdate(&pEnc->rate_control, pEnc->current->quant,
						  pFrame->length, pFrame->intra);
	}

	stop_global_timer();
	write_timer();

	emms();
	return XVID_ERR_OK;
}



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

	float psnr;
	uint8_t temp[128];

	start_global_timer();

	ENC_CHECK(pEnc);
	ENC_CHECK(pFrame);
	ENC_CHECK(pFrame->bitstream);
	ENC_CHECK(pFrame->image);

	SWAP(FRAMEINFO *, pEnc->current, pEnc->reference);

	pEnc->current->global_flags = pFrame->general;
	pEnc->current->motion_flags = pFrame->motion;
	pEnc->mbParam.hint = &pFrame->hint;

	inc_frame_num(pEnc);

	/* disable alternate scan flag if interlacing is not enabled */
	if ((pEnc->current->global_flags & XVID_ALTERNATESCAN) &&
		!(pEnc->current->global_flags & XVID_INTERLACING))
	{
		pEnc->current->global_flags -= XVID_ALTERNATESCAN;
	}

	start_timer();
	if (image_input
		(&pEnc->current->image, pEnc->mbParam.width, pEnc->mbParam.height,
		 pEnc->mbParam.edged_width, pFrame->image, pFrame->stride, pFrame->colorspace, pFrame->general & XVID_INTERLACING) < 0)
		return XVID_ERR_FORMAT;
	stop_conv_timer();

	if ((pFrame->general & XVID_CHROMAOPT)) {
		image_chroma_optimize(&pEnc->current->image, 
			pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.edged_width);
	}

	if (pFrame->general & XVID_EXTRASTATS)
	{	image_copy(&pEnc->sOriginal, &pEnc->current->image,
				   pEnc->mbParam.edged_width, pEnc->mbParam.height);
	}

	emms();

	BitstreamInit(&bs, pFrame->bitstream, 0);

	if (pFrame->quant == 0) {
		pEnc->current->quant = RateControlGetQ(&pEnc->rate_control, 0);
	} else {
		pEnc->current->quant = pFrame->quant;
	}

	if ((pEnc->current->global_flags & XVID_QUARTERPEL))
		pEnc->mbParam.m_quarterpel = 1;
	else
		pEnc->mbParam.m_quarterpel = 0;

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
		if ((pEnc->iFrameNum == -1)
			|| ((pEnc->mbParam.iMaxKeyInterval > 0)
				&& (pEnc->iFrameNum >= pEnc->mbParam.iMaxKeyInterval))) {
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

	/* Relic from OpenDivX - now disabled
	BitstreamPutBits(&bs, 0xFFFF, 16);
	BitstreamPutBits(&bs, 0xFFFF, 16);
	*/

	BitstreamPadAlways(&bs);
	pFrame->length = BitstreamLength(&bs);

	if (pResult) {
		pResult->quant = pEnc->current->quant;
		pResult->hlength = pFrame->length - (pEnc->current->sStat.iTextBits / 8);
		pResult->kblks = pEnc->current->sStat.kblks;
		pResult->mblks = pEnc->current->sStat.mblks;
		pResult->ublks = pEnc->current->sStat.ublks;
	}

	emms();

	if (pFrame->quant == 0) {
		RateControlUpdate(&pEnc->rate_control, pEnc->current->quant,
						  pFrame->length, pFrame->intra);
	}
	if (pFrame->general & XVID_EXTRASTATS)
	{
		psnr =
			image_psnr(&pEnc->sOriginal, &pEnc->current->image,
					   pEnc->mbParam.edged_width, pEnc->mbParam.width,
					   pEnc->mbParam.height);
	
		snprintf(temp, 127, "PSNR: %f\n", psnr);
	}

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
			} else				/* intra / stuffing / not_coded */
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
			BitstreamPadAlways(&bs);
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
	int mb_width = pEnc->mbParam.mb_width;
	int mb_height = pEnc->mbParam.mb_height;

	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff, 6, 64, int16_t, CACHE_LINE);

	uint16_t x, y;

	if ((pEnc->current->global_flags & XVID_REDUCED))
	{
		mb_width = (pEnc->mbParam.width + 31) / 32;
		mb_height = (pEnc->mbParam.height + 31) / 32;

		/* 16x16->8x8 downsample requires 1 additional edge pixel*/
		/* XXX: setedges is overkill */
		start_timer();
		image_setedges(&pEnc->current->image, 
			pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
			pEnc->mbParam.width, pEnc->mbParam.height);
		stop_edges_timer();
	}

	pEnc->iFrameNum = 0;
	pEnc->mbParam.m_rounding_type = 1;
	pEnc->current->rounding_type = pEnc->mbParam.m_rounding_type;
  	pEnc->current->quarterpel =  pEnc->mbParam.m_quarterpel;
	pEnc->current->coding_type = I_VOP;

	BitstreamWriteVolHeader(bs, &pEnc->mbParam, pEnc->current);

	set_timecodes(pEnc->current,pEnc->reference,pEnc->mbParam.fbase);

	BitstreamPadAlways(bs);
	BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 1);

	*pBits = BitstreamPos(bs);

	pEnc->current->sStat.iTextBits = 0;
	pEnc->current->sStat.kblks = mb_width * mb_height;
	pEnc->current->sStat.mblks = pEnc->current->sStat.ublks = 0;

	for (y = 0; y < mb_height; y++)
		for (x = 0; x < mb_width; x++) {
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
			MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->current->sStat);
			stop_coding_timer();
		}

	if ((pEnc->current->global_flags & XVID_REDUCED))
	{
		image_deblock_rrv(&pEnc->current->image, pEnc->mbParam.edged_width, 
			pEnc->current->mbs, mb_width, mb_height, pEnc->mbParam.mb_width,
			16, XVID_DEC_DEBLOCKY|XVID_DEC_DEBLOCKUV);
	}
	emms();

	*pBits = BitstreamPos(bs) - *pBits;
	pEnc->fMvPrevSigma = -1;
	pEnc->mbParam.m_fcode = 2;

	if (pEnc->current->global_flags & XVID_HINTEDME_GET) {
		HintedMEGet(pEnc, 1);
	}

	return 1;					/* intra */
}


#define INTRA_THRESHOLD 0.5
#define BFRAME_SKIP_THRESHHOLD 30


/* FrameCodeP also handles S(GMC)-VOPs */
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

	int mb_width = pEnc->mbParam.mb_width;
	int mb_height = pEnc->mbParam.mb_height;

	int iLimit;
	int x, y, k;
	int iSearchRange;
	int bIntra, skip_possible;
	
	/* IMAGE *pCurrent = &pEnc->current->image; */
	IMAGE *pRef = &pEnc->reference->image;

	if ((pEnc->current->global_flags & XVID_REDUCED))
	{
		mb_width = (pEnc->mbParam.width + 31) / 32;
		mb_height = (pEnc->mbParam.height + 31) / 32;
	}


	start_timer();
	image_setedges(pRef, pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
				   pEnc->mbParam.width, pEnc->mbParam.height);
	stop_edges_timer();

	pEnc->mbParam.m_rounding_type = 1 - pEnc->mbParam.m_rounding_type;
	pEnc->current->rounding_type = pEnc->mbParam.m_rounding_type;
  	pEnc->current->quarterpel =  pEnc->mbParam.m_quarterpel;
	pEnc->current->fcode = pEnc->mbParam.m_fcode;

	if (!force_inter)
		iLimit = (int)(mb_width * mb_height *  INTRA_THRESHOLD);
	else
		iLimit = mb_width * mb_height + 1;

	if ((pEnc->current->global_flags & XVID_HALFPEL)) {
		start_timer();
		image_interpolate(pRef, &pEnc->vInterH, &pEnc->vInterV,
						  &pEnc->vInterHV, pEnc->mbParam.edged_width,
						  pEnc->mbParam.edged_height,
						  pEnc->mbParam.m_quarterpel,
						  pEnc->current->rounding_type);
		stop_inter_timer();
	}

	pEnc->current->coding_type = P_VOP;
	
	start_timer();
	if (pEnc->current->global_flags & XVID_HINTEDME_SET)
		HintedMESet(pEnc, &bIntra);
	else
		bIntra =
			MotionEstimation(&pEnc->mbParam, pEnc->current, pEnc->reference,
                         &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
                         iLimit);

	stop_motion_timer();

	if (bIntra == 1) return FrameCodeI(pEnc, bs, pBits);

	if ( ( pEnc->current->global_flags & XVID_GMC ) 
		&& ( (pEnc->current->warp.duv[1].x != 0) || (pEnc->current->warp.duv[1].y != 0) ) )
	{
		pEnc->current->coding_type = S_VOP;

		generate_GMCparameters(	2, 16, &pEnc->current->warp, 
					pEnc->mbParam.width, pEnc->mbParam.height, 
					&pEnc->current->gmc_data);

		generate_GMCimage(&pEnc->current->gmc_data, &pEnc->reference->image, 
				pEnc->mbParam.mb_width, pEnc->mbParam.mb_height,
				pEnc->mbParam.edged_width, pEnc->mbParam.edged_width/2, 
				pEnc->mbParam.m_fcode, pEnc->mbParam.m_quarterpel, 0, 
				pEnc->current->rounding_type, pEnc->current->mbs, &pEnc->vGMC);

	}

	set_timecodes(pEnc->current,pEnc->reference,pEnc->mbParam.fbase);
	if (vol_header)
	{	BitstreamWriteVolHeader(bs, &pEnc->mbParam, pEnc->current);	
		BitstreamPadAlways(bs);
	}

	BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 1);

	*pBits = BitstreamPos(bs);

	pEnc->current->sStat.iTextBits = pEnc->current->sStat.iMvSum = pEnc->current->sStat.iMvCount = 
		pEnc->current->sStat.kblks = pEnc->current->sStat.mblks = pEnc->current->sStat.ublks = 0;


	for (y = 0; y < mb_height; y++) {
		for (x = 0; x < mb_width; x++) {
			MACROBLOCK *pMB =
				&pEnc->current->mbs[x + y * pEnc->mbParam.mb_width];

/* Mode decision: Check, if the block should be INTRA / INTER or GMC-coded */
/* For a start, leave INTRA decision as is, only choose only between INTER/GMC  - gruel, 9.1.2002 */

			bIntra = (pMB->mode == MODE_INTRA) || (pMB->mode == MODE_INTRA_Q);

			if (bIntra) {
				CodeIntraMB(pEnc, pMB);
				MBTransQuantIntra(&pEnc->mbParam, pEnc->current, pMB, x, y,
								  dct_codes, qcoeff);

				start_timer();
				MBPrediction(pEnc->current, x, y, pEnc->mbParam.mb_width, qcoeff);
				stop_prediction_timer();

				pEnc->current->sStat.kblks++;

				MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->current->sStat);
				stop_coding_timer();
				continue;
			}
				
			if (pEnc->current->coding_type == S_VOP) {

				int32_t iSAD = sad16(pEnc->current->image.y + 16*y*pEnc->mbParam.edged_width + 16*x,
					pEnc->vGMC.y + 16*y*pEnc->mbParam.edged_width + 16*x, 
					pEnc->mbParam.edged_width, 65536);
				
				if (pEnc->current->motion_flags & PMV_CHROMA16) {
					iSAD += sad8(pEnc->current->image.u + 8*y*(pEnc->mbParam.edged_width/2) + 8*x,
					pEnc->vGMC.u + 8*y*(pEnc->mbParam.edged_width/2) + 8*x, pEnc->mbParam.edged_width/2);

					iSAD += sad8(pEnc->current->image.v + 8*y*(pEnc->mbParam.edged_width/2) + 8*x,
					pEnc->vGMC.v + 8*y*(pEnc->mbParam.edged_width/2) + 8*x, pEnc->mbParam.edged_width/2);
				}

				if (iSAD <= pMB->sad16) {		/* mode decision GMC */

					if (pEnc->mbParam.m_quarterpel)
						pMB->qmvs[0] = pMB->qmvs[1] = pMB->qmvs[2] = pMB->qmvs[3] = pMB->amv;
					else
						pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = pMB->amv;

					pMB->mode = MODE_INTER;
					pMB->mcsel = 1;
					pMB->sad16 = iSAD;
				} else {
					pMB->mcsel = 0;
				}
			} else {
				pMB->mcsel = 0;	/* just a precaution */
			}

			start_timer();
			MBMotionCompensation(pMB, x, y, &pEnc->reference->image,
								 &pEnc->vInterH, &pEnc->vInterV,
								 &pEnc->vInterHV, &pEnc->vGMC, 
								 &pEnc->current->image,
								 dct_codes, pEnc->mbParam.width,
								 pEnc->mbParam.height,
								 pEnc->mbParam.edged_width,
								 pEnc->mbParam.m_quarterpel,
								 (pEnc->current->global_flags & XVID_REDUCED),
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

			if (pMB->mode != MODE_NOT_CODED)
			{	pMB->cbp =
					MBTransQuantInter(&pEnc->mbParam, pEnc->current, pMB, x, y,
									  dct_codes, qcoeff);
			}

			if (pMB->cbp || pMB->mvs[0].x || pMB->mvs[0].y ||
				   pMB->mvs[1].x || pMB->mvs[1].y || pMB->mvs[2].x ||
				   pMB->mvs[2].y || pMB->mvs[3].x || pMB->mvs[3].y) {
				pEnc->current->sStat.mblks++;
			}  else {
				pEnc->current->sStat.ublks++;
			}
			
			start_timer();

			/* Finished processing the MB, now check if to CODE or SKIP */

			skip_possible = (pMB->cbp == 0) && (pMB->mode == MODE_INTER) &&
							(pMB->dquant == NO_CHANGE);
			
			if (pEnc->current->coding_type == S_VOP)
				skip_possible &= (pMB->mcsel == 1);
			else if (pEnc->current->coding_type == P_VOP) {
				if (pEnc->mbParam.m_quarterpel)
					skip_possible &= ( (pMB->qmvs[0].x == 0) && (pMB->qmvs[0].y == 0) );
				else 
					skip_possible &= ( (pMB->mvs[0].x == 0) && (pMB->mvs[0].y == 0) );
			}

			if ( (pMB->mode == MODE_NOT_CODED) || (skip_possible)) {

/* This is a candidate for SKIPping, but for P-VOPs check intermediate B-frames first */

				if (pEnc->current->coding_type == P_VOP)	/* special rule for P-VOP's SKIP */
				{
					int bSkip = 1; 
				
					for (k=pEnc->bframenum_head; k< pEnc->bframenum_tail; k++)
					{
						int iSAD;
						iSAD = sad16(pEnc->reference->image.y + 16*y*pEnc->mbParam.edged_width + 16*x,
									pEnc->bframes[k]->image.y + 16*y*pEnc->mbParam.edged_width + 16*x,
								pEnc->mbParam.edged_width,BFRAME_SKIP_THRESHHOLD);
						if (iSAD >= BFRAME_SKIP_THRESHHOLD * pMB->quant)
						{	bSkip = 0; 
							break;
						}
					}
					
					if (!bSkip) {	/* no SKIP, but trivial block */
						if(pEnc->mbParam.m_quarterpel) {
							VECTOR predMV = get_qpmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, 0);
							pMB->pmvs[0].x = - predMV.x; 
							pMB->pmvs[0].y = - predMV.y; 
						}
						else {
							VECTOR predMV = get_pmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, 0);
							pMB->pmvs[0].x = - predMV.x; 
							pMB->pmvs[0].y = - predMV.y; 
						}
						pMB->mode = MODE_INTER;
						pMB->cbp = 0;
						MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->current->sStat);
						stop_coding_timer();

						continue;	/* next MB */
					}
				}
				/* do SKIP */

				pMB->mode = MODE_NOT_CODED;
				MBSkip(bs);
				stop_coding_timer();
				continue;	/* next MB */
			}
			/* ordinary case: normal coded INTER/INTER4V block */

			if (pEnc->current->global_flags & XVID_GREYSCALE)
			{	pMB->cbp &= 0x3C;		/* keep only bits 5-2 */
				qcoeff[4*64+0]=0;		/* zero, because DC for INTRA MBs DC value is saved */
				qcoeff[5*64+0]=0;
			}

			if(pEnc->mbParam.m_quarterpel) {
				VECTOR predMV = get_qpmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, 0);
				pMB->pmvs[0].x = pMB->qmvs[0].x - predMV.x;  
				pMB->pmvs[0].y = pMB->qmvs[0].y - predMV.y; 
				DPRINTF(DPRINTF_MV,"mv_diff (%i,%i) pred (%i,%i) result (%i,%i)", pMB->pmvs[0].x, pMB->pmvs[0].y, predMV.x, predMV.y, pMB->mvs[0].x, pMB->mvs[0].y);
			} else {
				VECTOR predMV = get_pmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, 0);
				pMB->pmvs[0].x = pMB->mvs[0].x - predMV.x; 
				pMB->pmvs[0].y = pMB->mvs[0].y - predMV.y; 
				DPRINTF(DPRINTF_MV,"mv_diff (%i,%i) pred (%i,%i) result (%i,%i)", pMB->pmvs[0].x, pMB->pmvs[0].y, predMV.x, predMV.y, pMB->mvs[0].x, pMB->mvs[0].y);
			}


			if (pMB->mode == MODE_INTER4V)
			{	int k;
				for (k=1;k<4;k++)
				{
					if(pEnc->mbParam.m_quarterpel) {
						VECTOR predMV = get_qpmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, k);
						pMB->pmvs[k].x = pMB->qmvs[k].x - predMV.x;  
						pMB->pmvs[k].y = pMB->qmvs[k].y - predMV.y; 
				DPRINTF(DPRINTF_MV,"mv_diff (%i,%i) pred (%i,%i) result (%i,%i)", pMB->pmvs[k].x, pMB->pmvs[k].y, predMV.x, predMV.y, pMB->mvs[k].x, pMB->mvs[k].y);
					} else {
						VECTOR predMV = get_pmv2(pEnc->current->mbs, pEnc->mbParam.mb_width, 0, x, y, k);
						pMB->pmvs[k].x = pMB->mvs[k].x - predMV.x; 
						pMB->pmvs[k].y = pMB->mvs[k].y - predMV.y; 
				DPRINTF(DPRINTF_MV,"mv_diff (%i,%i) pred (%i,%i) result (%i,%i)", pMB->pmvs[k].x, pMB->pmvs[k].y, predMV.x, predMV.y, pMB->mvs[k].x, pMB->mvs[k].y);
					}

				}
			}
			
			MBCoding(pEnc->current, pMB, qcoeff, bs, &pEnc->current->sStat);
			stop_coding_timer();

		}
	}

	if ((pEnc->current->global_flags & XVID_REDUCED))
	{
		image_deblock_rrv(&pEnc->current->image, pEnc->mbParam.edged_width, 
			pEnc->current->mbs, mb_width, mb_height, pEnc->mbParam.mb_width,
			16, XVID_DEC_DEBLOCKY|XVID_DEC_DEBLOCKUV);
	}

	emms();

	if (pEnc->current->global_flags & XVID_HINTEDME_GET) {
		HintedMEGet(pEnc, 0);
	}

	if (pEnc->current->sStat.iMvCount == 0)
		pEnc->current->sStat.iMvCount = 1;

	fSigma = (float) sqrt((float) pEnc->current->sStat.iMvSum / pEnc->current->sStat.iMvCount);

	iSearchRange = 1 << (3 + pEnc->mbParam.m_fcode);

	if ((fSigma > iSearchRange / 3)
		&& (pEnc->mbParam.m_fcode <= (3 + pEnc->mbParam.m_quarterpel)))	/* maximum search range 128 */
	{
		pEnc->mbParam.m_fcode++;
		iSearchRange *= 2;
	} else if ((fSigma < iSearchRange / 6)
			   && (pEnc->fMvPrevSigma >= 0)
			   && (pEnc->fMvPrevSigma < iSearchRange / 6)
			   && (pEnc->mbParam.m_fcode >= (2 + pEnc->mbParam.m_quarterpel)))	/* minimum search range 16 */
	{
		pEnc->mbParam.m_fcode--;
		iSearchRange /= 2;
	}

	pEnc->fMvPrevSigma = fSigma;

	/* frame drop code */
	DPRINTF(DPRINTF_DEBUG, "kmu %i %i %i", pEnc->current->sStat.kblks, pEnc->current->sStat.mblks, pEnc->current->sStat.ublks);
	if (pEnc->current->sStat.kblks + pEnc->current->sStat.mblks <
		(pEnc->mbParam.frame_drop_ratio * mb_width * mb_height) / 100)
	{
		pEnc->current->sStat.kblks = pEnc->current->sStat.mblks = 0;
		pEnc->current->sStat.ublks = mb_width * mb_height;

		BitstreamReset(bs);

		set_timecodes(pEnc->current,pEnc->reference,pEnc->mbParam.fbase);
		BitstreamWriteVopHeader(bs, &pEnc->mbParam, pEnc->current, 0);

		/* copy reference frame details into the current frame */
		pEnc->current->quant = pEnc->reference->quant;
		pEnc->current->motion_flags = pEnc->reference->motion_flags;
		pEnc->current->rounding_type = pEnc->reference->rounding_type;
	  	pEnc->current->quarterpel =  pEnc->reference->quarterpel;
		pEnc->current->fcode = pEnc->reference->fcode;
		pEnc->current->bcode = pEnc->reference->bcode;
		image_copy(&pEnc->current->image, &pEnc->reference->image, pEnc->mbParam.edged_width, pEnc->mbParam.height);
		memcpy(pEnc->current->mbs, pEnc->reference->mbs, sizeof(MACROBLOCK) * mb_width * mb_height);
	}

	/* XXX: debug
	{
		char s[100];
		sprintf(s, "\\%05i_cur.pgm", pEnc->m_framenum);
		image_dump_yuvpgm(&pEnc->current->image, 
			pEnc->mbParam.edged_width,
			pEnc->mbParam.width, pEnc->mbParam.height, s);
		
		sprintf(s, "\\%05i_ref.pgm", pEnc->m_framenum);
		image_dump_yuvpgm(&pEnc->reference->image, 
			pEnc->mbParam.edged_width,
			pEnc->mbParam.width, pEnc->mbParam.height, s);
	} 
	*/


	*pBits = BitstreamPos(bs) - *pBits;

	return 0;					/* inter */
}


static void
FrameCodeB(Encoder * pEnc,
		   FRAMEINFO * frame,
		   Bitstream * bs,
		   uint32_t * pBits)
{
	DECLARE_ALIGNED_MATRIX(dct_codes, 6, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(qcoeff, 6, 64, int16_t, CACHE_LINE);
	uint32_t x, y;

	IMAGE *f_ref = &pEnc->reference->image;
	IMAGE *b_ref = &pEnc->current->image;

#ifdef BFRAMES_DEC_DEBUG
	FILE *fp;
	static char first=0;
#define BFRAME_DEBUG  	if (!first && fp){ \
		fprintf(fp,"Y=%3d   X=%3d   MB=%2d   CBP=%02X\n",y,x,mb->mode,mb->cbp); \
	}

	pEnc->current->global_flags &= ~XVID_REDUCED;	/* reduced resoltion not yet supported */

	if (!first){
		fp=fopen("C:\\XVIDDBGE.TXT","w");
	}
#endif

  	frame->quarterpel =  pEnc->mbParam.m_quarterpel;
	
	/* forward  */
	image_setedges(f_ref, pEnc->mbParam.edged_width,
				   pEnc->mbParam.edged_height, pEnc->mbParam.width,
				   pEnc->mbParam.height);
	start_timer();
	image_interpolate(f_ref, &pEnc->f_refh, &pEnc->f_refv, &pEnc->f_refhv,
					  pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
					  pEnc->mbParam.m_quarterpel, 0);
	stop_inter_timer();

	/* backward */
	image_setedges(b_ref, pEnc->mbParam.edged_width,
				   pEnc->mbParam.edged_height, pEnc->mbParam.width,
				   pEnc->mbParam.height);
	start_timer();
	image_interpolate(b_ref, &pEnc->vInterH, &pEnc->vInterV, &pEnc->vInterHV,
					  pEnc->mbParam.edged_width, pEnc->mbParam.edged_height,
					  pEnc->mbParam.m_quarterpel, 0);
	stop_inter_timer();

	start_timer();

	MotionEstimationBVOP(&pEnc->mbParam, frame, 
						 ((int32_t)(pEnc->current->stamp - frame->stamp)),				/* time_bp */
						 ((int32_t)(pEnc->current->stamp - pEnc->reference->stamp)), 	/* time_pp */
						 pEnc->reference->mbs, f_ref,
						 &pEnc->f_refh, &pEnc->f_refv, &pEnc->f_refhv,
						 pEnc->current, b_ref, &pEnc->vInterH,
						 &pEnc->vInterV, &pEnc->vInterHV);


	stop_motion_timer();

	/*
	if (test_quant_type(&pEnc->mbParam, pEnc->current)) {
		BitstreamWriteVolHeader(bs, pEnc->mbParam.width, pEnc->mbParam.height, pEnc->mbParam.quant_type);
	}
	*/

	frame->coding_type = B_VOP;

	set_timecodes(frame, pEnc->reference,pEnc->mbParam.fbase);
	BitstreamWriteVopHeader(bs, &pEnc->mbParam, frame, 1);

	*pBits = BitstreamPos(bs);

	frame->sStat.iTextBits = 0;
	frame->sStat.iMvSum = 0;
	frame->sStat.iMvCount = 0;
	frame->sStat.kblks = frame->sStat.mblks = frame->sStat.ublks = 0;


	for (y = 0; y < pEnc->mbParam.mb_height; y++) {
		for (x = 0; x < pEnc->mbParam.mb_width; x++) {
			MACROBLOCK * const mb = &frame->mbs[x + y * pEnc->mbParam.mb_width];
			int direction = pEnc->mbParam.global & XVID_ALTERNATESCAN ? 2 : 0;

			/* decoder ignores mb when refence block is INTER(0,0), CBP=0 */
			if (mb->mode == MODE_NOT_CODED) {
				/* mb->mvs[0].x = mb->mvs[0].y = mb->cbp = 0; */
				continue;
			}

			if (mb->mode != MODE_DIRECT_NONE_MV) {
				MBMotionCompensationBVOP(&pEnc->mbParam, mb, x, y, &frame->image,
									 f_ref, &pEnc->f_refh, &pEnc->f_refv,
									 &pEnc->f_refhv, b_ref, &pEnc->vInterH,
									 &pEnc->vInterV, &pEnc->vInterHV,
									 dct_codes);

				if (mb->mode == MODE_DIRECT_NO4V) mb->mode = MODE_DIRECT;
				mb->quant = frame->quant;
			
				mb->cbp =
					MBTransQuantInterBVOP(&pEnc->mbParam, frame, mb, dct_codes, qcoeff);

				if ( (mb->mode == MODE_DIRECT) && (mb->cbp == 0)
					&& (mb->pmvs[3].x == 0) && (mb->pmvs[3].y == 0) ) {
					mb->mode = MODE_DIRECT_NONE_MV;	/* skipped */
				}
			}

#ifdef BFRAMES_DEC_DEBUG
	BFRAME_DEBUG
#endif
			start_timer();
			MBCodingBVOP(mb, qcoeff, frame->fcode, frame->bcode, bs,
						 &frame->sStat, direction);
			stop_coding_timer();
		}
	}

	emms();

	/* TODO: dynamic fcode/bcode ??? */

	*pBits = BitstreamPos(bs) - *pBits;

#ifdef BFRAMES_DEC_DEBUG
	if (!first){
		first=1;
		if (fp)
			fclose(fp);
	}
#endif
}
