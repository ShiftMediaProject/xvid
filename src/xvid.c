/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Native API implementation  -
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
 * $Id: xvid.c,v 1.46 2003-06-09 17:07:10 Isibaar Exp $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xvid.h"
#include "decoder.h"
#include "encoder.h"
#include "bitstream/cbp.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "image/colorspace.h"
#include "image/interpolate8x8.h"
#include "image/reduced.h"
#include "utils/mem_transfer.h"
#include "utils/mbfunctions.h"
#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "motion/motion.h"
#include "motion/sad.h"
#include "utils/emms.h"
#include "utils/timer.h"
#include "bitstream/mbcoding.h"

#if defined(ARCH_IS_IA32)

#if defined(_MSC_VER)
#	include <windows.h>
#else
#	include <signal.h>
#	include <setjmp.h>

	static jmp_buf mark;

	static void
	sigill_handler(int signal)
	{
	   longjmp(mark, 1);
	}
#endif


/*
 * Calls the funcptr, and returns whether SIGILL (illegal instruction) was
 * signalled
 *
 * Return values:
 *  -1 : could not determine
 *   0 : SIGILL was *not* signalled
 *   1 : SIGILL was signalled
 */

int
sigill_check(void (*func)())
{
#if defined(_MSC_VER)
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


/* detect cpu flags  */
static unsigned int
detect_cpu_flags()
{
	/* enable native assembly optimizations by default */
	unsigned int cpu_flags = XVID_CPU_ASM;

#if defined(ARCH_IS_IA32)
	cpu_flags |= check_cpu_features();
	if ((cpu_flags & XVID_CPU_SSE) && sigill_check(sse_os_trigger))
		cpu_flags &= ~XVID_CPU_SSE;

	if ((cpu_flags & XVID_CPU_SSE2) && sigill_check(sse2_os_trigger))
		cpu_flags &= ~XVID_CPU_SSE2;
#endif

#if defined(ARCH_IS_PPC)
#if defined(ARCH_IS_PPC_ALTIVEC)
	cpu_flags |= XVID_CPU_ALTIVEC;
#endif
#endif

	return cpu_flags;
}


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


static 
int xvid_init_init(XVID_INIT_PARAM * init_param)
{
	int cpu_flags;

	/* Inform the client the API version */
	init_param->api_version = API_VERSION;

	/* Inform the client the core build - unused because we're still alpha */
	init_param->core_build = 1000;

	/* Do we have to force CPU features  ? */
	if ((init_param->cpu_flags & XVID_CPU_FORCE)) {

		cpu_flags = init_param->cpu_flags;

	} else {

		cpu_flags = detect_cpu_flags();
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
	transfer_8to16subro  = transfer_8to16subro_c;
	transfer_8to16sub2 = transfer_8to16sub2_c;
	transfer_16to8add  = transfer_16to8add_c;
	transfer8x8_copy   = transfer8x8_copy_c;

	/* Interlacing functions */
	MBFieldTest = MBFieldTest_c;

	/* Image interpolation related functions */
	interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_c;
	interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_c;
	interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_c;

	interpolate16x16_lowpass_h = interpolate16x16_lowpass_h_c;
	interpolate16x16_lowpass_v = interpolate16x16_lowpass_v_c;
	interpolate16x16_lowpass_hv = interpolate16x16_lowpass_hv_c;

	interpolate8x8_lowpass_h = interpolate8x8_lowpass_h_c;
	interpolate8x8_lowpass_v = interpolate8x8_lowpass_v_c;
	interpolate8x8_lowpass_hv = interpolate8x8_lowpass_hv_c;

	interpolate8x8_6tap_lowpass_h = interpolate8x8_6tap_lowpass_h_c;
	interpolate8x8_6tap_lowpass_v = interpolate8x8_6tap_lowpass_v_c;

	interpolate8x8_avg2 = interpolate8x8_avg2_c;
	interpolate8x8_avg4 = interpolate8x8_avg4_c;

	/* reduced resoltuion */
	copy_upsampled_8x8_16to8 = xvid_Copy_Upsampled_8x8_16To8_C;
	add_upsampled_8x8_16to8 = xvid_Add_Upsampled_8x8_16To8_C;
	vfilter_31 = xvid_VFilter_31_C;
	hfilter_31 = xvid_HFilter_31_C;
	filter_18x18_to_8x8 = xvid_Filter_18x18_To_8x8_C;
	filter_diff_18x18_to_8x8 = xvid_Filter_Diff_18x18_To_8x8_C;

	/* Initialize internal colorspace transformation tables */
	colorspace_init();

	/* All colorspace transformation functions User Format->YV12 */
	yv12_to_yv12    = yv12_to_yv12_c;
	rgb555_to_yv12  = rgb555_to_yv12_c;
	rgb565_to_yv12  = rgb565_to_yv12_c;
	bgr_to_yv12     = bgr_to_yv12_c;
	bgra_to_yv12    = bgra_to_yv12_c;
	abgr_to_yv12    = abgr_to_yv12_c;
	rgba_to_yv12    = rgba_to_yv12_c;
	yuyv_to_yv12    = yuyv_to_yv12_c;
	uyvy_to_yv12    = uyvy_to_yv12_c;

	rgb555i_to_yv12 = rgb555i_to_yv12_c;
	rgb565i_to_yv12 = rgb565i_to_yv12_c;
	bgri_to_yv12    = bgri_to_yv12_c;
	bgrai_to_yv12   = bgrai_to_yv12_c;
	abgri_to_yv12   = abgri_to_yv12_c;
	rgbai_to_yv12   = rgbai_to_yv12_c;
	yuyvi_to_yv12   = yuyvi_to_yv12_c;
	uyvyi_to_yv12   = uyvyi_to_yv12_c;


	/* All colorspace transformation functions YV12->User format */
	yv12_to_rgb555  = yv12_to_rgb555_c;
	yv12_to_rgb565  = yv12_to_rgb565_c;
	yv12_to_bgr     = yv12_to_bgr_c;
	yv12_to_bgra    = yv12_to_bgra_c;
	yv12_to_abgr    = yv12_to_abgr_c;
	yv12_to_rgba    = yv12_to_rgba_c;
	yv12_to_yuyv    = yv12_to_yuyv_c;
	yv12_to_uyvy    = yv12_to_uyvy_c;
 
	yv12_to_rgb555i = yv12_to_rgb555i_c;
	yv12_to_rgb565i = yv12_to_rgb565i_c;
	yv12_to_bgri    = yv12_to_bgri_c;
	yv12_to_bgrai   = yv12_to_bgrai_c;
	yv12_to_abgri   = yv12_to_abgri_c;
	yv12_to_rgbai   = yv12_to_rgbai_c;
	yv12_to_yuyvi   = yv12_to_yuyvi_c;
	yv12_to_uyvyi   = yv12_to_uyvyi_c;

	/* Functions used in motion estimation algorithms */
	calc_cbp = calc_cbp_c;
	sad16    = sad16_c;
	sad8     = sad8_c;
	sad16bi  = sad16bi_c;
	sad8bi   = sad8bi_c;
	dev16    = dev16_c;
	sad16v	 = sad16v_c;
	
/*	Halfpel8_Refine = Halfpel8_Refine_c; */

#if defined(ARCH_IS_IA32)

	if ((cpu_flags & XVID_CPU_ASM))
	{
		vfilter_31 = xvid_VFilter_31_x86;
		hfilter_31 = xvid_HFilter_31_x86;
	}

	if ((cpu_flags & XVID_CPU_MMX) || (cpu_flags & XVID_CPU_MMXEXT) ||
		(cpu_flags & XVID_CPU_3DNOW) || (cpu_flags & XVID_CPU_3DNOWEXT) ||
		(cpu_flags & XVID_CPU_SSE) || (cpu_flags & XVID_CPU_SSE2))
	{
		/* Restore FPU context : emms_c is a nop functions */
		emms = emms_mmx;
	}

	if ((cpu_flags & XVID_CPU_MMX)) {

		/* Forward and Inverse Discrete Cosine Transformation functions */
		fdct = fdct_mmx;
		idct = simple_idct_mmx; /* use simple idct by default */

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
		transfer_8to16subro  = transfer_8to16subro_mmx;
		transfer_8to16sub2 = transfer_8to16sub2_mmx;
		transfer_16to8add  = transfer_16to8add_mmx;
		transfer8x8_copy   = transfer8x8_copy_mmx;

		/* Interlacing Functions */
		MBFieldTest = MBFieldTest_mmx;

		/* Image Interpolation related functions */
		interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_mmx;
		interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_mmx;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_mmx;

		interpolate8x8_6tap_lowpass_h = interpolate8x8_6tap_lowpass_h_mmx;
		interpolate8x8_6tap_lowpass_v = interpolate8x8_6tap_lowpass_v_mmx;

		interpolate8x8_avg2 = interpolate8x8_avg2_mmx;
		interpolate8x8_avg4 = interpolate8x8_avg4_mmx;

		/* reduced resolution */
		copy_upsampled_8x8_16to8 = xvid_Copy_Upsampled_8x8_16To8_mmx;
		add_upsampled_8x8_16to8 = xvid_Add_Upsampled_8x8_16To8_mmx;
		hfilter_31 = xvid_HFilter_31_mmx;
		filter_18x18_to_8x8 = xvid_Filter_18x18_To_8x8_mmx;
		filter_diff_18x18_to_8x8 = xvid_Filter_Diff_18x18_To_8x8_mmx;

		/* image input xxx_to_yv12 related functions */
		yv12_to_yv12  = yv12_to_yv12_mmx;
		bgr_to_yv12   = bgr_to_yv12_mmx;
		bgra_to_yv12  = bgra_to_yv12_mmx;
		yuyv_to_yv12  = yuyv_to_yv12_mmx;
		uyvy_to_yv12  = uyvy_to_yv12_mmx;

		/* image output yv12_to_xxx related functions */
		yv12_to_bgr   = yv12_to_bgr_mmx;
		yv12_to_bgra  = yv12_to_bgra_mmx;
		yv12_to_yuyv  = yv12_to_yuyv_mmx;
		yv12_to_uyvy  = yv12_to_uyvy_mmx;
		
		yv12_to_yuyvi = yv12_to_yuyvi_mmx;
		yv12_to_uyvyi = yv12_to_uyvyi_mmx;

		/* Motion estimation related functions */
		calc_cbp = calc_cbp_mmx;
		sad16    = sad16_mmx;
		sad8     = sad8_mmx;
		sad16bi = sad16bi_mmx;
		sad8bi  = sad8bi_mmx;
		dev16    = dev16_mmx;
		sad16v	 = sad16v_mmx;
	}

	/* these 3dnow functions are faster than mmx, but slower than xmm. */
	if ((cpu_flags & XVID_CPU_3DNOW)) {

		emms = emms_3dn;

		/* ME functions */
		sad16bi = sad16bi_3dn;
		sad8bi  = sad8bi_3dn;

		yuyv_to_yv12  = yuyv_to_yv12_3dn;
		uyvy_to_yv12  = uyvy_to_yv12_3dn;
	}


	if ((cpu_flags & XVID_CPU_MMXEXT)) {

		/* Inverse DCT */
		/* idct = idct_xmm; Don't use Walken idct anymore! */

		/* Interpolation */
		interpolate8x8_halfpel_h  = interpolate8x8_halfpel_h_xmm;
		interpolate8x8_halfpel_v  = interpolate8x8_halfpel_v_xmm;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_xmm;

		/* reduced resolution */
		copy_upsampled_8x8_16to8 = xvid_Copy_Upsampled_8x8_16To8_xmm;
		add_upsampled_8x8_16to8 = xvid_Add_Upsampled_8x8_16To8_xmm;

		/* Quantization */
		quant4_intra = quant4_intra_xmm;
		quant4_inter = quant4_inter_xmm;

		dequant_intra = dequant_intra_xmm;
		dequant_inter = dequant_inter_xmm;

		/* Buffer transfer */
		transfer_8to16sub2 = transfer_8to16sub2_xmm;

		/* Colorspace transformation */
		yv12_to_yv12  = yv12_to_yv12_xmm;
		yuyv_to_yv12  = yuyv_to_yv12_xmm;
		uyvy_to_yv12  = uyvy_to_yv12_xmm;

		/* ME functions */
		sad16 = sad16_xmm;
		sad8  = sad8_xmm;
		sad16bi = sad16bi_xmm;
		sad8bi  = sad8bi_xmm;
		dev16 = dev16_xmm;
		sad16v	 = sad16v_xmm;
	}

	if ((cpu_flags & XVID_CPU_3DNOW)) {

		/* Interpolation */
		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_3dn;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_3dn;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_3dn;
	}

	if ((cpu_flags & XVID_CPU_3DNOWEXT)) {

		/* Inverse DCT */
		/* idct =  idct_3dne; Don't use Walken idct anymore */

		/* Buffer transfer */
		transfer_8to16copy =  transfer_8to16copy_3dne;
		transfer_16to8copy = transfer_16to8copy_3dne;
		transfer_8to16sub =  transfer_8to16sub_3dne;
		transfer_8to16subro =  transfer_8to16subro_3dne;
		transfer_8to16sub2 =  transfer_8to16sub2_3dne;
		transfer_16to8add = transfer_16to8add_3dne;
		transfer8x8_copy = transfer8x8_copy_3dne;

		/* Quantization */
		dequant4_intra = dequant4_intra_3dne;
		dequant4_inter = dequant4_inter_3dne;
		quant_intra = quant_intra_3dne;
		quant_inter = quant_inter_3dne;
		dequant_intra = dequant_intra_3dne;
		dequant_inter = dequant_inter_3dne;

		/* ME functions */
		calc_cbp = calc_cbp_3dne;
		sad16 = sad16_3dne;
		sad8 = sad8_3dne;
		sad16bi = sad16bi_3dne;
		sad8bi = sad8bi_3dne;
		dev16 = dev16_3dne;

		/* Interpolation */
		interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_3dne;
		interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_3dne;
		interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_3dne;
	}


	if ((cpu_flags & XVID_CPU_SSE2)) {

		calc_cbp = calc_cbp_sse2;

		/* Quantization */
		quant_intra   = quant_intra_sse2;
		dequant_intra = dequant_intra_sse2;
		quant_inter   = quant_inter_sse2;
		dequant_inter = dequant_inter_sse2;

#if defined(EXPERIMENTAL_SSE2_CODE)
		/* ME; slower than xmm */
		sad16    = sad16_sse2;
		dev16    = dev16_sse2;
#endif
		/* Forward and Inverse DCT */
		/* idct  = idct_sse2;
		/* fdct = fdct_sse2; Both are none to be unprecise - better deactivate for now */
	}
#endif

#if defined(ARCH_IS_IA64)
	if ((cpu_flags & XVID_CPU_ASM)) { /* use assembler routines? */
	  idct_ia64_init();
	  fdct = fdct_ia64;
	  idct = idct_ia64;   /*not yet working, crashes */
	  interpolate8x8_halfpel_h = interpolate8x8_halfpel_h_ia64;
	  interpolate8x8_halfpel_v = interpolate8x8_halfpel_v_ia64;
	  interpolate8x8_halfpel_hv = interpolate8x8_halfpel_hv_ia64;
	  sad16 = sad16_ia64;
	  sad16bi = sad16bi_ia64;
	  sad8 = sad8_ia64;
	  dev16 = dev16_ia64;
/*	  Halfpel8_Refine = Halfpel8_Refine_ia64; */
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
	  DPRINTF(DPRINTF_DEBUG, "Using IA-64 assembler routines.");
	}
#endif 

#if defined(ARCH_IS_PPC)
	if ((cpu_flags & XVID_CPU_ASM))
	{
		calc_cbp = calc_cbp_ppc;
	}

	if ((cpu_flags & XVID_CPU_ALTIVEC))
	{
		calc_cbp = calc_cbp_altivec;
		fdct = fdct_altivec;
		idct = idct_altivec;
		sadInit = sadInit_altivec;
		sad16 = sad16_altivec;
		sad8 = sad8_altivec;
		dev16 = dev16_altivec;
	}
#endif

	return XVID_ERR_OK;
}



static int
xvid_init_convert(XVID_INIT_CONVERTINFO* convert)
{
/*
	const int flip1 =
		(convert->input.colorspace & XVID_CSP_VFLIP) ^
		(convert->output.colorspace & XVID_CSP_VFLIP);
*/
	const int width = convert->width;
	const int height = convert->height;
	const int width2 = convert->width/2;
	const int height2 = convert->height/2;
	IMAGE img;

	switch (convert->input.colorspace & ~XVID_CSP_VFLIP)
	{
		case XVID_CSP_YV12 :
			img.y = convert->input.y;
			img.v = (uint8_t*)convert->input.y + width*height; 
			img.u = (uint8_t*)convert->input.y + width*height + width2*height2;
			image_output(&img, width, height, width,
						convert->output.y, convert->output.y_stride,
						convert->output.colorspace, convert->interlacing);
			break;

		default :
			return XVID_ERR_FORMAT;
	}


	emms();
	return XVID_ERR_OK;
}



void fill8(uint8_t * block, int size, int value)
{
	int i;
	for (i = 0; i < size; i++)
		block[i] = value;
}

void fill16(int16_t * block, int size, int value)
{
	int i;
	for (i = 0; i < size; i++)
		block[i] = value;
}

#define RANDOM(min,max) min + (rand() % (max-min))

void random8(uint8_t * block, int size, int min, int max)
{
	int i;
	for (i = 0; i < size; i++)
		block[i] = RANDOM(min,max);
}

void random16(int16_t * block, int size, int min, int max)
{
	int i;
	for (i = 0; i < size; i++)
		block[i] = RANDOM(min,max);
}

int compare16(const int16_t * blockA, const int16_t * blockB, int size)
{
	int i;
	for (i = 0; i < size; i++)
		if (blockA[i] != blockB[i])
			return 1;

	return 0;
}

int diff16(const int16_t * blockA, const int16_t * blockB, int size)
{
	int i, diff = 0;
	for (i = 0; i < size; i++)
		diff += ABS(blockA[i]-blockB[i]);
	return diff;
}


#define XVID_TEST_RANDOM	0x00000001	/* random input data */
#define XVID_TEST_VERBOSE	0x00000002	/* verbose error output */


#define TEST_FORWARD	0x00000001	/* intra */
#define TEST_FDCT  (TEST_FORWARD)
#define TEST_IDCT  (0)

static int test_transform(void * funcA, void * funcB, const char * nameB,
				   int test, int flags)
{
	int i;
	int64_t timeSTART;
	int64_t timeA = 0;
	int64_t timeB = 0;
	DECLARE_ALIGNED_MATRIX(arrayA, 1, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(arrayB, 1, 64, int16_t, CACHE_LINE);
	int min, max;
	int count = 0;
	
	int tmp;
	int min_error = 0x10000*64;
	int max_error = 0;


	if ((test & TEST_FORWARD))	/* forward */
	{
		min = -256;
		max = 255;
	}else{		/* inverse */
		min = -2048;
		max = 2047;
	}

	for (i = 0; i < 64*64; i++)
	{
		if ((flags & XVID_TEST_RANDOM))
		{
			random16(arrayA, 64, min, max);
		}else{
			fill16(arrayA, 64, i);
		}
		memcpy(arrayB, arrayA, 64*sizeof(int16_t));

		if ((test & TEST_FORWARD))
		{
			timeSTART = read_counter();
			((fdctFunc*)funcA)(arrayA);
			timeA += read_counter() - timeSTART;

			timeSTART = read_counter();
			((fdctFunc*)funcB)(arrayB);
			timeB += read_counter() - timeSTART;
		}
		else
		{
			timeSTART = read_counter();
			((idctFunc*)funcA)(arrayA);
			timeA += read_counter() - timeSTART;

			timeSTART = read_counter();
			((idctFunc*)funcB)(arrayB);
			timeB += read_counter() - timeSTART;
		}

		tmp = diff16(arrayA, arrayB, 64) / 64;
		if (tmp > max_error)
			max_error = tmp;
		if (tmp < min_error)
			min_error = tmp;

		count++;
	}

	/* print the "average difference" of best/worst transforms */
	printf("%s:\t%i\t(min_error:%i, max_error:%i)\n", nameB, (int)(timeB / count), min_error, max_error);

	return 0;
}


#define TEST_QUANT	0x00000001	/* forward quantization */
#define TEST_INTRA	0x00000002	/* intra */
#define TEST_QUANT_INTRA	(TEST_QUANT|TEST_INTRA)
#define TEST_QUANT_INTER	(TEST_QUANT)
#define TEST_DEQUANT_INTRA	(TEST_INTRA)
#define TEST_DEQUANT_INTER	(0)

static int test_quant(void * funcA, void * funcB, const char * nameB,
			   int test, int flags)
{
	int q,i;
	int64_t timeSTART;
	int64_t timeA = 0;
	int64_t timeB = 0;
	int retA = 0, retB = 0;
	DECLARE_ALIGNED_MATRIX(arrayX, 1, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(arrayA, 1, 64, int16_t, CACHE_LINE);
	DECLARE_ALIGNED_MATRIX(arrayB, 1, 64, int16_t, CACHE_LINE);
	int min, max;
	int count = 0;
	int errors = 0;

	if ((test & TEST_QUANT))	/* quant */
	{
		min = -2048;
		max = 2047;
	}else{		/* dequant */
		min = -256;
		max = 255;
	}

	for (q = 1; q <= 31; q++)	/* quantizer */
	{
		for (i = min; i < max; i++)	/* input coeff */
		{
			if ((flags & XVID_TEST_RANDOM))
			{
				random16(arrayX, 64, min, max);
			}else{
				fill16(arrayX, 64, i);
			}

			if ((test & TEST_INTRA))	/* intra */
			{
				timeSTART = read_counter();
				((quanth263_intraFunc*)funcA)(arrayA, arrayX, q, q);
				timeA += read_counter() - timeSTART;

				timeSTART = read_counter();
				((quanth263_intraFunc*)funcB)(arrayB, arrayX, q, q);
				timeB += read_counter() - timeSTART;
			}
			else	/* inter */
			{
				timeSTART = read_counter();
				retA = ((quanth263_interFunc*)funcA)(arrayA, arrayX, q);
				timeA += read_counter() - timeSTART;

				timeSTART = read_counter();
				retB = ((quanth263_interFunc*)funcB)(arrayB, arrayX, q);
				timeB += read_counter() - timeSTART;
			}

			/* compare return value from quant_inter, and compare (de)quantiz'd arrays */
			if ( ((test&TEST_QUANT) && !(test&TEST_INTRA) && retA != retB ) ||
				compare16(arrayA, arrayB, 64))
			{
				errors++;
				if ((flags & XVID_TEST_VERBOSE))
					printf("%s error: q=%i, i=%i\n", nameB, q, i);
			}

			count++;
		}
	}

	printf("%s:\t%i", nameB, (int)(timeB / count));
	if (errors>0)
		printf("\t(%i errors out of %i)", errors, count);
	printf("\n");
	
	return 0;
}



int xvid_init_test(int flags)
{
#if defined(ARCH_IS_IA32)
	int cpu_flags;
#endif

	printf("XviD tests\n\n");

#if defined(ARCH_IS_IA32)
	cpu_flags = detect_cpu_flags();
#endif

	idct_int32_init();
	emms();

	srand(time(0));

	/* fDCT test */
	printf("--- fdct ---\n");
		test_transform(fdct_int32, fdct_int32, "c", TEST_FDCT, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_transform(fdct_int32, fdct_mmx, "mmx", TEST_FDCT, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_transform(fdct_int32, fdct_sse2, "sse2", TEST_FDCT, flags);
#endif

	/* iDCT test */
	printf("\n--- idct ---\n");
		test_transform(idct_int32, idct_int32, "c", TEST_IDCT, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_transform(idct_int32, idct_mmx, "mmx", TEST_IDCT, flags);
	if (cpu_flags & XVID_CPU_MMXEXT)
		test_transform(idct_int32, idct_xmm, "xmm", TEST_IDCT, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_transform(idct_int32, idct_3dne, "3dne", TEST_IDCT, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_transform(idct_int32, idct_sse2, "sse2", TEST_IDCT, flags);
#endif

	/* Intra quantization test */
	printf("\n--- quant intra ---\n");
		test_quant(quant_intra_c, quant_intra_c, "c", TEST_QUANT_INTRA, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(quant_intra_c, quant_intra_mmx, "mmx", TEST_QUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(quant_intra_c, quant_intra_3dne, "3dne", TEST_QUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_quant(quant_intra_c, quant_intra_sse2, "sse2", TEST_QUANT_INTRA, flags);
#endif

	/* Inter quantization test */
	printf("\n--- quant inter ---\n");
		test_quant(quant_inter_c, quant_inter_c, "c", TEST_QUANT_INTER, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(quant_inter_c, quant_inter_mmx, "mmx", TEST_QUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(quant_inter_c, quant_inter_3dne, "3dne", TEST_QUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_quant(quant_inter_c, quant_inter_sse2, "sse2", TEST_QUANT_INTER, flags);
#endif

	/* Intra dequantization test */
	printf("\n--- dequant intra ---\n");
		test_quant(dequant_intra_c, dequant_intra_c, "c", TEST_DEQUANT_INTRA, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(dequant_intra_c, dequant_intra_mmx, "mmx", TEST_DEQUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_MMXEXT)
		test_quant(dequant_intra_c, dequant_intra_xmm, "xmm", TEST_DEQUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(dequant_intra_c, dequant_intra_3dne, "3dne", TEST_DEQUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_quant(dequant_intra_c, dequant_intra_sse2, "sse2", TEST_DEQUANT_INTRA, flags);
#endif

	/* Inter dequantization test */
	printf("\n--- dequant inter ---\n");
		test_quant(dequant_inter_c, dequant_inter_c, "c", TEST_DEQUANT_INTER, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(dequant_inter_c, dequant_inter_mmx, "mmx", TEST_DEQUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_MMXEXT)
		test_quant(dequant_inter_c, dequant_inter_xmm, "xmm", TEST_DEQUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(dequant_inter_c, dequant_inter_3dne, "3dne", TEST_DEQUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_SSE2)
		test_quant(dequant_inter_c, dequant_inter_sse2, "sse2", TEST_DEQUANT_INTER, flags);
#endif

	/* Intra quantization test */
	printf("\n--- quant4 intra ---\n");
		test_quant(quant4_intra_c, quant4_intra_c, "c", TEST_QUANT_INTRA, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(quant4_intra_c, quant4_intra_mmx, "mmx", TEST_QUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_MMXEXT)
		test_quant(quant4_intra_c, quant4_intra_xmm, "xmm", TEST_QUANT_INTRA, flags);
#endif

	/* Inter quantization test */
	printf("\n--- quant4 inter ---\n");
		test_quant(quant4_inter_c, quant4_inter_c, "c", TEST_QUANT_INTER, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(quant4_inter_c, quant4_inter_mmx, "mmx", TEST_QUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_MMXEXT)
		test_quant(quant4_inter_c, quant4_inter_xmm, "xmm", TEST_QUANT_INTER, flags);
#endif

	/* Intra dequantization test */
	printf("\n--- dequant4 intra ---\n");
		test_quant(dequant4_intra_c, dequant4_intra_c, "c", TEST_DEQUANT_INTRA, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(dequant4_intra_c, dequant4_intra_mmx, "mmx", TEST_DEQUANT_INTRA, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(dequant4_intra_c, dequant4_intra_3dne, "3dne", TEST_DEQUANT_INTRA, flags);
#endif

	/* Inter dequantization test */
	printf("\n--- dequant4 inter ---\n");
		test_quant(dequant4_inter_c, dequant4_inter_c, "c", TEST_DEQUANT_INTER, flags);

#if defined(ARCH_IS_IA32)
	if (cpu_flags & XVID_CPU_MMX)
		test_quant(dequant4_inter_c, dequant4_inter_mmx, "mmx", TEST_DEQUANT_INTER, flags);
	if (cpu_flags & XVID_CPU_3DNOWEXT)
		test_quant(dequant4_inter_c, dequant4_inter_3dne, "3dne", TEST_DEQUANT_INTER, flags);
#endif

	emms();

	return XVID_ERR_OK;
}


int
xvid_init(void *handle,
		  int opt,
		  void *param1,
		  void *param2)
{
	switch(opt)
	{
		case XVID_INIT_INIT :
			return xvid_init_init((XVID_INIT_PARAM*)param1);

		case XVID_INIT_CONVERT :
			return xvid_init_convert((XVID_INIT_CONVERTINFO*)param1);

		case XVID_INIT_TEST :
		{
			ptr_t flags = (ptr_t)param1;
			return xvid_init_test((int)flags);
		}
		default :
			return XVID_ERR_FAIL;
	}
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
		return decoder_decode((DECODER *) handle, (XVID_DEC_FRAME *) param1, (XVID_DEC_STATS*) param2);

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

		if (((Encoder *) handle)->mbParam.max_bframes >= 0)
			return encoder_encode_bframes((Encoder *) handle,
										  (XVID_ENC_FRAME *) param1,
										  (XVID_ENC_STATS *) param2);
		else 
			return encoder_encode((Encoder *) handle,
								  (XVID_ENC_FRAME *) param1,
								  (XVID_ENC_STATS *) param2);

	case XVID_ENC_CREATE:
		return encoder_create((XVID_ENC_PARAM *) param1);

	case XVID_ENC_DESTROY:
		return encoder_destroy((Encoder *) handle);

	default:
		return XVID_ERR_FAIL;
	}
}
