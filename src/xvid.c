/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Native API implementation  -
 *
 *  Copyright(C) 2001-2002 Peter Ross <pross@cs.rmit.edu.au>
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
 ****************************************************************************/

#include "xvid.h"
#include "decoder.h"
#include "encoder.h"
#include "bitstream/cbp.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "image/colorspace.h"
#include "image/interpolate8x8.h"
#include "utils/mem_transfer.h"
#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "motion/motion.h"
#include "motion/sad.h"
#include "utils/emms.h"
#include "utils/timer.h"
#include "bitstream/mbcoding.h"

#if defined(ARCH_X86) && defined(EXPERIMENTAL_SSE2_CODE)

#ifdef WIN32
#include <windows.h>
#else
#include <signal.h>
#include <setjmp.h>
#endif


#ifndef WIN32

static jmp_buf mark;

static void
sigill_handler(int signal)
{
   longjmp(mark, 1);
}
#endif


/*
 * Calls the funcptr, and returns whether SIGILL (illegal instruction) was signalled
 * Return values:
 * -1 : could not determine
 * 0  : SIGILL was *not* signalled
 * 1  : SIGILL was signalled
 */

int
sigill_check(void (*func)())
{
#ifdef WIN32
	_try {
		func();
	}
	_except(EXCEPTION_EXECUTE_HANDLER) {

		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
			return 1;
	}
	return 0;
#else
    void * old_handler;
    int jmpret;


    old_handler = signal(SIGILL, sigill_handler);
    if (old_handler == SIG_ERR)
    {
        return -1;
    }

    jmpret = setjmp(mark);
    if (jmpret == 0)
    {
        func();
    }

    signal(SIGILL, old_handler);

    return jmpret;
#endif
}
#endif

/*****************************************************************************
 * XviD Init Entry point
 *
 * Well this function initialize all internal function pointers according
 * to the CPU features forced by the library client or autodetected (depending
 * on the XVID_CPU_FORCE flag). It also initializes vlc coding tables and all
 * image colorspace transformation tables.
 * 
 * Returned value : XVID_ERR_OK
 *                  + API_VERSION in the input XVID_INIT_PARAM structure
 *                  + core build  "   "    "       "               "
 *
 ****************************************************************************/

int
xvid_init(void *handle,
		  int opt,
		  void *param1,
		  void *param2)
{
	int cpu_flags;
	XVID_INIT_PARAM *init_param;

	init_param = (XVID_INIT_PARAM *) param1;

	/* Inform the client the API version */
	init_param->api_version = API_VERSION;

	/* Inform the client the core build - unused because we're still alpha */
	init_param->core_build = 1000;

	/* Do we have to force CPU features  ? */
	if ((init_param->cpu_flags & XVID_CPU_FORCE)) {

		cpu_flags = init_param->cpu_flags;

	} else {

		cpu_flags = check_cpu_features();

#if defined(ARCH_X86) && defined(EXPERIMENTAL_SSE2_CODE)
		if ((cpu_flags & XVID_CPU_SSE) && sigill_check(sse_os_trigger))
			cpu_flags &= ~XVID_CPU_SSE;

		if ((cpu_flags & XVID_CPU_SSE2) && sigill_check(sse2_os_trigger))
			cpu_flags &= ~XVID_CPU_SSE2;
#endif
	}

	if ((init_param->cpu_flags & XVID_CPU_CHKONLY))
	{
		init_param->cpu_flags = cpu_flags;
		return XVID_ERR_OK;
	}

	init_param->cpu_flags = cpu_flags;


	/* Initialize the function pointers */
	idct_int32_init();
	init_vlc_tables();

	/* Fixed Point Forward/Inverse DCT transformations */
	fdct = fdct_int32;
	idct = idct_int32;

	/* Only needed on PPC Altivec archs */
	sadInit = 0;

	/* Restore FPU context : emms_c is a nop functions */
	emms = emms_c;

	/* Quantization functions */
	quant_intra   = quant_intra_c;
	dequant_intra = dequant_intra_c;
	quant_inter   = quant_inter_c;
	dequant_inter = dequant_inter_c;

	quant4_intra   = quant4_intra_c;
	dequant4_intra = dequant4_intra_c;
	quant4_inter   = quant4_inter_c;
	dequant4_inter = dequant4_inter_c;

	/* Block transfer related functions */
	transfer_8to16copy = transfer_8to16copy_c;
	transfer_16to8copy = transfer_16to8copy_c;
	transfer_8to16sub  = transfer_8to16sub_c;
	transfer_8to16sub2 = transfer_8to16sub2_c;
	transfer_16to8add  = transfer_16to8add_c;
	transfer8x8_copy   = transfer8x8_copy_c;

	/* Image interpolation related functions */
	interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_c;
	interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_c;
	interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_c;

	/* Initialize internal colorspace transformation tables */
	colorspace_init();

	/* All colorspace transformation functions User Format->YV12 */
	rgb555_to_yv12 = rgb555_to_yv12_c;
	rgb565_to_yv12 = rgb565_to_yv12_c;
	rgb24_to_yv12  = rgb24_to_yv12_c;
	rgb32_to_yv12  = rgb32_to_yv12_c;
	yuv_to_yv12    = yuv_to_yv12_c;
	yuyv_to_yv12   = yuyv_to_yv12_c;
	uyvy_to_yv12   = uyvy_to_yv12_c;

	/* All colorspace transformation functions YV12->User format */
	yv12_to_rgb555 = yv12_to_rgb555_c;
	yv12_to_rgb565 = yv12_to_rgb565_c;
	yv12_to_rgb24  = yv12_to_rgb24_c;
	yv12_to_rgb32  = yv12_to_rgb32_c;
	yv12_to_yuv    = yv12_to_yuv_c;
	yv12_to_yuyv   = yv12_to_yuyv_c;
	yv12_to_uyvy   = yv12_to_uyvy_c;

	/* Functions used in motion estimation algorithms */
	calc_cbp = calc_cbp_c;
	sad16    = sad16_c;
	sad8     = sad8_c;
	sad16bi  = sad16bi_c;
	sad8bi   = sad8bi_c;
	dev16    = dev16_c;
	
	Halfpel8_Refine = Halfpel8_Refine_c;

#ifdef ARCH_X86
	if ((cpu_flags & XVID_CPU_MMX) > 0) {

		/* Forward and Inverse Discrete Cosine Transformation functions */
		fdct = fdct_mmx;
		idct = idct_mmx;

		/* To restore FPU context after mmx use */
		emms = emms_mmx;

		/* Quantization related functions */
		quant_intra   = quant_intra_mmx;
		dequant_intra = dequant_intra_mmx;
		quant_inter   = quant_inter_mmx;
		dequant_inter = dequant_inter_mmx;

		quant4_intra   = quant4_intra_mmx;
		dequant4_intra = dequant4_intra_mmx;
		quant4_inter   = quant4_inter_mmx;
		dequant4_inter = dequant4_inter_mmx;

		/* Block related functions */
		transfer_8to16copy = transfer_8to16copy_mmx;
		transfer_16to8copy = transfer_16to8copy_mmx;
		transfer_8to16sub  = transfer_8to16sub_mmx;
		transfer_8to16sub2 = transfer_8to16sub2_mmx;
		transfer_16to8add  = transfer_16to8add_mmx;
		transfer8x8_copy   = transfer8x8_copy_mmx;


		/* Image Interpolation related functions */
		interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_mmx;
		interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_mmx;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_mmx;

		/* Image RGB->YV12 related functions */
		rgb24_to_yv12 = rgb24_to_yv12_mmx;
		rgb32_to_yv12 = rgb32_to_yv12_mmx;
		yuv_to_yv12   = yuv_to_yv12_mmx;
		yuyv_to_yv12  = yuyv_to_yv12_mmx;
		uyvy_to_yv12  = uyvy_to_yv12_mmx;

		/* Image YV12->RGB related functions */
		yv12_to_rgb24 = yv12_to_rgb24_mmx;
		yv12_to_rgb32 = yv12_to_rgb32_mmx;
		yv12_to_yuyv  = yv12_to_yuyv_mmx;
		yv12_to_uyvy  = yv12_to_uyvy_mmx;

		/* Motion estimation related functions */
		calc_cbp = calc_cbp_mmx;
		sad16    = sad16_mmx;
		sad8     = sad8_mmx;
		sad16bi = sad16bi_mmx;
		sad8bi  = sad8bi_mmx;
		dev16    = dev16_mmx;

	}

	/* these 3dnow functions are faster than mmx, but slower than xmm. */
	if ((cpu_flags & XVID_CPU_3DNOW) > 0) {

		/* ME functions */
		sad16bi = sad16bi_3dn;
		sad8bi  = sad8bi_3dn;
	}


	if ((cpu_flags & XVID_CPU_MMXEXT) > 0) {

		/* Inverse DCT */
		idct = idct_xmm;

		/* Interpolation */
		interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_xmm;
		interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_xmm;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_xmm;

		/* Quantization */
		dequant_intra = dequant_intra_xmm;
		dequant_inter = dequant_inter_xmm;

		/* Buffer transfer */
		transfer_8to16sub2 = transfer_8to16sub2_xmm;

		/* Colorspace transformation */
		yuv_to_yv12 = yuv_to_yv12_xmm;

		/* ME functions */
		sad16 = sad16_xmm;
		sad8  = sad8_xmm;
		sad16bi = sad16bi_xmm;
		sad8bi  = sad8bi_xmm;
		dev16 = dev16_xmm;

	}

	if ((cpu_flags & XVID_CPU_3DNOW) > 0) {

		/* Interpolation */
		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_3dn;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_3dn;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_3dn;
	}

	if ((cpu_flags & XVID_CPU_SSE2) > 0) {
#ifdef EXPERIMENTAL_SSE2_CODE

		calc_cbp = calc_cbp_sse2;

		/* Quantization */
		quant_intra   = quant_intra_sse2;
		dequant_intra = dequant_intra_sse2;
		quant_inter   = quant_inter_sse2;
		dequant_inter = dequant_inter_sse2;

		/* ME */
		sad16    = sad16_sse2;
		dev16    = dev16_sse2;

		/* Forward and Inverse DCT */
		idct  = idct_sse2;
		fdct = fdct_sse2;
#endif
	}

#endif

#ifdef ARCH_IA64
	if ((cpu_flags & XVID_CPU_IA64) > 0) { //use assembler routines?
	  idct_ia64_init();
	  fdct = fdct_ia64;
	  idct = idct_ia64;   //not yet working, crashes
	  interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_ia64;
	  interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_ia64;
	  interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_ia64;
	  sad16 = sad16_ia64;
	  sad16bi = sad16bi_ia64;
	  sad8 = sad8_ia64;
	  dev16 = dev16_ia64;
	  Halfpel8_Refine = Halfpel8_Refine_ia64;
	  quant_intra = quant_intra_ia64;
	  dequant_intra = dequant_intra_ia64;
	  quant_inter = quant_inter_ia64;
	  dequant_inter = dequant_inter_ia64;
	  transfer_8to16copy = transfer_8to16copy_ia64;
	  transfer_16to8copy = transfer_16to8copy_ia64;
	  transfer_8to16sub = transfer_8to16sub_ia64;
	  transfer_8to16sub2 = transfer_8to16sub2_ia64;
	  transfer_16to8add = transfer_16to8add_ia64;
	  transfer8x8_copy = transfer8x8_copy_ia64;
	  DEBUG("Using IA-64 assembler routines.\n");
	}
#endif 

#ifdef ARCH_PPC
#ifdef ARCH_PPC_ALTIVEC
	calc_cbp = calc_cbp_altivec;
	fdct = fdct_altivec;
	idct = idct_altivec;
	sadInit = sadInit_altivec;
	sad16 = sad16_altivec;
	sad8 = sad8_altivec;
	dev16 = dev16_altivec;
#else
	calc_cbp = calc_cbp_ppc;
#endif
#endif

	return XVID_ERR_OK;
}

/*****************************************************************************
 * XviD Native decoder entry point
 *
 * This function is just a wrapper to all the option cases.
 *
 * Returned values : XVID_ERR_FAIL when opt is invalid
 *                   else returns the wrapped function result
 *
 ****************************************************************************/

int
xvid_decore(void *handle,
			int opt,
			void *param1,
			void *param2)
{
	switch (opt) {
	case XVID_DEC_DECODE:
		return decoder_decode((DECODER *) handle, (XVID_DEC_FRAME *) param1);

	case XVID_DEC_CREATE:
		return decoder_create((XVID_DEC_PARAM *) param1);

	case XVID_DEC_DESTROY:
		return decoder_destroy((DECODER *) handle);

	default:
		return XVID_ERR_FAIL;
	}
}


/*****************************************************************************
 * XviD Native encoder entry point
 *
 * This function is just a wrapper to all the option cases.
 *
 * Returned values : XVID_ERR_FAIL when opt is invalid
 *                   else returns the wrapped function result
 *
 ****************************************************************************/

int
xvid_encore(void *handle,
			int opt,
			void *param1,
			void *param2)
{
	switch (opt) {
	case XVID_ENC_ENCODE:
#ifdef BFRAMES
		if (((Encoder *) handle)->mbParam.max_bframes >= 0)
		return encoder_encode_bframes((Encoder *) handle, (XVID_ENC_FRAME *) param1,
							  (XVID_ENC_STATS *) param2);
		else 
#endif
		return encoder_encode((Encoder *) handle, (XVID_ENC_FRAME *) param1,
							  (XVID_ENC_STATS *) param2);

	case XVID_ENC_CREATE:
		return encoder_create((XVID_ENC_PARAM *) param1);

	case XVID_ENC_DESTROY:
		return encoder_destroy((Encoder *) handle);

	default:
		return XVID_ERR_FAIL;
	}
}
