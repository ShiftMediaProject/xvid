/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - OpenDivx API wrapper -
 *
 *  Copyright(C) 2001-2002 Peter Ross <pross@xvid.org>
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
 * $Id: divx4.c,v 1.20 2002-11-16 23:38:16 edgomez Exp $
 *
 *************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "xvid.h"
#include "divx4.h"
#include "decoder.h"
#include "encoder.h"

#define EMULATED_DIVX_VERSION 20011001

/**************************************************************************
 * Divx Instance Structure
 *
 * This chain list datatype allows XviD do instanciate multiples divx4
 * sessions.
 *
 * ToDo : The way this chain list is used does not guarantee reentrance
 *        because they are not protected by any kind of mutex to allow
 *        only one modifier. We should add a mutex for each element in
 *        the chainlist.
 *************************************************************************/


typedef struct DINST
{
	unsigned long key;
	struct DINST *next;

	void *handle;
	XVID_DEC_FRAME xframe;

}
DINST;

typedef struct EINST
{
	struct EINST *next;

	void *handle;
	int quality;

}
EINST;

/**************************************************************************
 * Global data (needed to emulate correctly exported symbols from divx4)
 *************************************************************************/

/* This is not used in this module but is required by some divx4 encoders*/
int quiet_encore = 1;

/**************************************************************************
 * Local data
 *************************************************************************/

/* The Divx4 instance chainlist */
static DINST *dhead = NULL;
static EINST *ehead = NULL;

/* Divx4 quality to XviD encoder motion flag presets */
static int const divx4_motion_presets[7] = {
	0,

	PMV_EARLYSTOP16,

	PMV_EARLYSTOP16 | PMV_ADVANCEDDIAMOND16,

	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16,

	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 |
		PMV_HALFPELREFINE8,

	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EARLYSTOP8 |
		PMV_HALFPELREFINE8,

	PMV_EARLYSTOP16 | PMV_HALFPELREFINE16 | PMV_EXTSEARCH16 | PMV_EARLYSTOP8 |
		PMV_HALFPELREFINE8
};


/* Divx4 quality to general encoder flag presets */
static int const divx4_general_presets[7] = {
	0,
	XVID_H263QUANT,
	XVID_H263QUANT,
	XVID_H263QUANT | XVID_HALFPEL,
	XVID_H263QUANT | XVID_INTER4V | XVID_HALFPEL,
	XVID_H263QUANT | XVID_INTER4V | XVID_HALFPEL,
	XVID_H263QUANT | XVID_INTER4V | XVID_HALFPEL
};

/**************************************************************************
 * Local Prototypes
 *************************************************************************/

/* Chain list helper functions */
static DINST *dinst_find(unsigned long key);
static DINST *dinst_add(unsigned long key);
static void dinst_remove(unsigned long key);

static EINST *einst_find(void *handle);
static EINST *einst_add(void *handle);
static void einst_remove(void *handle);

/* Converts divx4 colorspaces codes to xvid codes */
static int xvid_to_opendivx_dec_csp(int csp);
static int xvid_to_opendivx_enc_csp(int csp);

/**************************************************************************
 * decore part
 *
 * decore is the divx4 entry point used to decompress the mpeg4 bitstream
 * into a user defined image format.
 *************************************************************************/

int
decore(unsigned long key,
	   unsigned long opt,
	   void *param1,
	   void *param2)
{

	int xerr;

	switch (opt) {

	case DEC_OPT_MEMORY_REQS:
		{
			memset(param2, 0, sizeof(DEC_MEM_REQS));
			return DEC_OK;
		}

	case DEC_OPT_INIT:
		{
			XVID_INIT_PARAM xinit;
			XVID_DEC_PARAM xparam;
			DINST *dcur;
			DEC_PARAM *dparam = (DEC_PARAM *) param1;

			/* Find the divx4 instance */
			if ((dcur = dinst_find(key)) == NULL) {
				dcur = dinst_add(key);
			}

			/*
			 * XviD initialization
			 * XviD will detect the host cpu type and activate optimized
			 * functions according to the host cpu features.
			 */
			xinit.cpu_flags = 0;
			xvid_init(NULL, 0, &xinit, NULL);

			/* XviD decoder initialization for this instance */
			xparam.width = dparam->x_dim;
			xparam.height = dparam->y_dim;
			dcur->xframe.colorspace =
				xvid_to_opendivx_dec_csp(dparam->output_format);

			xerr = decoder_create(&xparam);

			/* Store the xvid handle into the divx4 instance chainlist */
			dcur->handle = xparam.handle;

			break;
		}

	case DEC_OPT_RELEASE:
		{
			DINST *dcur;

			/* Find the divx4 instance into the chain list */
			if ((dcur = dinst_find(key)) == NULL) {
				return DEC_EXIT;
			}

			/* Destroy the XviD decoder attached to this divx4 instance */
			xerr = decoder_destroy(dcur->handle);

			/* Remove the divx4 instance from the chainlist */
			dinst_remove(key);

			break;
		}

	case DEC_OPT_SETPP:
		{
			DINST *dcur;

			/* Find the divx4 instance into the chain list */
			if ((dcur = dinst_find(key)) == NULL) {
				return DEC_EXIT;
			}

			/*
			 * We return DEC_OK but XviD has no postprocessing implemented
			 * in core.
			 */
			return DEC_OK;
		}

	case DEC_OPT_SETOUT:
		{
			DINST *dcur;
			DEC_PARAM *dparam = (DEC_PARAM *) param1;

			if ((dcur = dinst_find(key)) == NULL) {
				return DEC_EXIT;
			}

			/* Change the output colorspace */
			dcur->xframe.colorspace =
				xvid_to_opendivx_dec_csp(dparam->output_format);

			return DEC_OK;
		}

	case DEC_OPT_FRAME:
		{
			int csp_tmp = 0;
			DINST *dcur;
			DEC_FRAME *dframe = (DEC_FRAME *) param1;

			if ((dcur = dinst_find(key)) == NULL) {
				return DEC_EXIT;
			}

			/* Copy the divx4 fields to the XviD decoder structure */
			dcur->xframe.bitstream = dframe->bitstream;
			dcur->xframe.length = dframe->length;
			dcur->xframe.image = dframe->bmp;
			dcur->xframe.stride = dframe->stride;

			/* Does the frame need to be skipped ? */
			if (!dframe->render_flag) {
				/*
				 * Then we use the null colorspace to force XviD to
				 * skip the frame. The original colorspace will be
				 * restored after the decoder call
				 */
				csp_tmp = dcur->xframe.colorspace;
				dcur->xframe.colorspace = XVID_CSP_NULL;
			}

			/* Decode the bitstream */
			xerr = decoder_decode(dcur->handle, &dcur->xframe);

			/* Restore the real colorspace for this instance */
			if (!dframe->render_flag) {
				dcur->xframe.colorspace = csp_tmp;
			}

			break;
		}

	case DEC_OPT_FRAME_311:
		/* XviD does not handle Divx ;-) 3.11 yet */
		return DEC_EXIT;

	case DEC_OPT_VERSION:
		return EMULATED_DIVX_VERSION;

	default:
		return DEC_EXIT;
	}


	/* XviD error code -> Divx4 */
	switch (xerr) {
	case XVID_ERR_OK:
		return DEC_OK;
	case XVID_ERR_MEMORY:
		return DEC_MEMORY;
	case XVID_ERR_FORMAT:
		return DEC_BAD_FORMAT;
	default:
		return DEC_EXIT;
	}
}

/**************************************************************************
 * Encore Part
 *
 * encore is the divx4 entry point used to compress a frame to a mpeg4
 * bitstream.
 *************************************************************************/

#define FRAMERATE_INCR		1001

int
encore(void *handle,
	   int opt,
	   void *param1,
	   void *param2)
{

	int xerr;

	switch (opt) {
	case ENC_OPT_INIT:
		{
			EINST *ecur;
			ENC_PARAM *eparam = (ENC_PARAM *) param1;
			XVID_INIT_PARAM xinit;
			XVID_ENC_PARAM xparam;

			/* Init XviD which will detect host cpu features */
			xinit.cpu_flags = 0;
			xvid_init(NULL, 0, &xinit, NULL);

			/* Settings are copied to the XviD encoder structure */
			xparam.width = eparam->x_dim;
			xparam.height = eparam->y_dim;
			if ((eparam->framerate - (int) eparam->framerate) == 0) {
				xparam.fincr = 1;
				xparam.fbase = (int) eparam->framerate;
			} else {
				xparam.fincr = FRAMERATE_INCR;
				xparam.fbase = (int) (FRAMERATE_INCR * eparam->framerate);
			}
			xparam.rc_bitrate = eparam->bitrate;
			xparam.rc_reaction_delay_factor = 16;
			xparam.rc_averaging_period = 100;
			xparam.rc_buffer = 100;
			xparam.min_quantizer = eparam->min_quantizer;
			xparam.max_quantizer = eparam->max_quantizer;
			xparam.max_key_interval = eparam->max_key_interval;

			/* Create the encoder session */
			xerr = encoder_create(&xparam);

			eparam->handle = xparam.handle;

			/* Create an encoder instance in the chainlist */
			if ((ecur = einst_find(xparam.handle)) == NULL) {
				ecur = einst_add(xparam.handle);

				if (ecur == NULL) {
					encoder_destroy((Encoder *) xparam.handle);
					return ENC_MEMORY;
				}

			}

			ecur->quality = eparam->quality;
			if (ecur->quality < 0)
				ecur->quality = 0;
			if (ecur->quality > 6)
				ecur->quality = 6;

			break;
		}

	case ENC_OPT_RELEASE:
		{
			EINST *ecur;

			if ((ecur = einst_find(handle)) == NULL) {
				return ENC_FAIL;
			}

			einst_remove(handle);
			xerr = encoder_destroy((Encoder *) handle);

			break;
		}

	case ENC_OPT_ENCODE:
	case ENC_OPT_ENCODE_VBR:
		{
			EINST *ecur;

			ENC_FRAME *eframe = (ENC_FRAME *) param1;
			ENC_RESULT *eresult = (ENC_RESULT *) param2;
			XVID_ENC_FRAME xframe;
			XVID_ENC_STATS xstats;

			if ((ecur = einst_find(handle)) == NULL) {
				return ENC_FAIL;
			}

			/* Copy the divx4 info into the xvid structure */
			xframe.bitstream = eframe->bitstream;
			xframe.length = eframe->length;
			xframe.motion = divx4_motion_presets[ecur->quality];
			xframe.general = divx4_general_presets[ecur->quality];

			xframe.image = eframe->image;
			xframe.colorspace = xvid_to_opendivx_enc_csp(eframe->colorspace);

			if (opt == ENC_OPT_ENCODE_VBR) {
				xframe.intra = eframe->intra;
				xframe.quant = eframe->quant;
			} else {
				xframe.intra = -1;
				xframe.quant = 0;
			}

			/* Encode the frame */
			xerr =
				encoder_encode((Encoder *) handle, &xframe,
							   (eresult ? &xstats : NULL));

			/* Copy back the xvid structure to the divx4 one */
			if (eresult) {
				eresult->is_key_frame = xframe.intra;
				eresult->quantizer = xstats.quant;
				eresult->total_bits = xframe.length * 8;
				eresult->motion_bits = xstats.hlength * 8;
				eresult->texture_bits =
					eresult->total_bits - eresult->motion_bits;
			}

			eframe->length = xframe.length;

			break;
		}

	default:
		return ENC_FAIL;
	}

	/* XviD Error code  -> Divx4 error code */
	switch (xerr) {
	case XVID_ERR_OK:
		return ENC_OK;
	case XVID_ERR_MEMORY:
		return ENC_MEMORY;
	case XVID_ERR_FORMAT:
		return ENC_BAD_FORMAT;
	default:
		return ENC_FAIL;
	}
}

/**************************************************************************
 * Local Functions
 *************************************************************************/

/***************************************
 * DINST chainlist helper functions    *
 ***************************************/

/* Find an element in the chainlist according to its key value */
static DINST *
dinst_find(unsigned long key)
{
	DINST *dcur = dhead;

	while (dcur) {
		if (dcur->key == key) {
			return dcur;
		}
		dcur = dcur->next;
	}

	return NULL;
}


/* Add an element to the chainlist */
static DINST *
dinst_add(unsigned long key)
{
	DINST *dnext = dhead;

	dhead = malloc(sizeof(DINST));
	if (dhead == NULL) {
		dhead = dnext;
		return NULL;
	}

	dhead->key = key;
	dhead->next = dnext;

	return dhead;
}


/* Remove an elmement from the chainlist */
static void
dinst_remove(unsigned long key)
{
	DINST *dcur = dhead;

	if (dhead == NULL) {
		return;
	}

	if (dcur->key == key) {
		dhead = dcur->next;
		free(dcur);
		return;
	}

	while (dcur->next) {
		if (dcur->next->key == key) {
			DINST *tmp = dcur->next;

			dcur->next = tmp->next;
			free(tmp);
			return;
		}
		dcur = dcur->next;
	}
}


/***************************************
 * EINST chainlist helper functions    *
 ***************************************/

/* Find an element in the chainlist according to its handle */
static EINST *
einst_find(void *handle)
{
	EINST *ecur = ehead;

	while (ecur) {
		if (ecur->handle == handle) {
			return ecur;
		}
		ecur = ecur->next;
	}

	return NULL;
}


/* Add an element to the chainlist */
static EINST *
einst_add(void *handle)
{
	EINST *enext = ehead;

	ehead = malloc(sizeof(EINST));
	if (ehead == NULL) {
		ehead = enext;
		return NULL;
	}

	ehead->handle = handle;
	ehead->next = enext;

	return ehead;
}


/* Remove an elmement from the chainlist */
static void
einst_remove(void *handle)
{
	EINST *ecur = ehead;

	if (ehead == NULL) {
		return;
	}

	if (ecur->handle == handle) {
		ehead = ecur->next;
		free(ecur);
		return;
	}

	while (ecur->next) {
		if (ecur->next->handle == handle) {
			EINST *tmp = ecur->next;

			ecur->next = tmp->next;
			free(tmp);
			return;
		}
		ecur = ecur->next;
	}
}

/***************************************
 * Colorspace code converter           *
 ***************************************/

static int
xvid_to_opendivx_dec_csp(int csp)
{

	switch (csp) {
	case DEC_YV12:
		return XVID_CSP_YV12;
	case DEC_420:
		return XVID_CSP_I420;
	case DEC_YUY2:
		return XVID_CSP_YUY2;
	case DEC_UYVY:
		return XVID_CSP_UYVY;
	case DEC_RGB32:
		return XVID_CSP_VFLIP | XVID_CSP_RGB32;
	case DEC_RGB24:
		return XVID_CSP_VFLIP | XVID_CSP_RGB24;
	case DEC_RGB565:
		return XVID_CSP_VFLIP | XVID_CSP_RGB565;
	case DEC_RGB555:
		return XVID_CSP_VFLIP | XVID_CSP_RGB555;
	case DEC_RGB32_INV:
		return XVID_CSP_RGB32;
	case DEC_RGB24_INV:
		return XVID_CSP_RGB24;
	case DEC_RGB565_INV:
		return XVID_CSP_RGB565;
	case DEC_RGB555_INV:
		return XVID_CSP_RGB555;
	case DEC_USER:
		return XVID_CSP_USER;
	default:
		return -1;
	}
}

static int
xvid_to_opendivx_enc_csp(int csp)
{

	switch (csp) {
	case ENC_CSP_RGB24:
		return XVID_CSP_VFLIP | XVID_CSP_RGB24;
	case ENC_CSP_YV12:
		return XVID_CSP_YV12;
	case ENC_CSP_YUY2:
		return XVID_CSP_YUY2;
	case ENC_CSP_UYVY:
		return XVID_CSP_UYVY;
	case ENC_CSP_I420:
		return XVID_CSP_I420;
	default:
		return -1;
	}
}
