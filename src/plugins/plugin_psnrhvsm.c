/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - PSNR-HVS-M plugin: computes the PSNR-HVS-M metric -
 *
 *  Copyright(C) 2010 Michael Militzer <michael@xvid.org>
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
 * $Id: plugin_psnrhvsm.c,v 1.2 2010-11-08 20:20:39 Isibaar Exp $
 *
 ****************************************************************************/

/*****************************************************************************
 *
 * The PSNR-HVS-M metric is described in the following paper:
 *
 * "On between-coefficient contrast masking of DCT basis functions", by
 * N. Ponomarenko, F. Silvestri, K. Egiazarian, M. Carli, J. Astola, V. Lukin,
 * in Proceedings of the Third International Workshop on Video Processing and 
 * Quality Metrics for Consumer Electronics VPQM-07, January, 2007, 4 p.
 *
 * http://www.ponomarenko.info/psnrhvsm.htm
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../portab.h"
#include "../xvid.h"
#include "../dct/fdct.h"
#include "../image/image.h"
#include "../utils/mem_transfer.h"
#include "../utils/emms.h"

typedef struct { 

	uint64_t mse_sum; /* for avrg psnrhvsm */
	long frame_cnt;

} psnrhvsm_data_t; /* internal plugin data */

static const float CSF_Coeff[64] = 
	{ 1.608443f, 2.339554f, 2.573509f, 1.608443f, 1.072295f, 0.643377f, 0.504610f, 0.421887f,
	  2.144591f, 2.144591f, 1.838221f, 1.354478f, 0.989811f, 0.443708f, 0.428918f, 0.467911f,
	  1.838221f, 1.979622f, 1.608443f, 1.072295f, 0.643377f, 0.451493f, 0.372972f, 0.459555f,
	  1.838221f, 1.513829f, 1.169777f, 0.887417f, 0.504610f, 0.295806f, 0.321689f, 0.415082f,
	  1.429727f, 1.169777f, 0.695543f, 0.459555f, 0.378457f, 0.236102f, 0.249855f, 0.334222f,
	  1.072295f, 0.735288f, 0.467911f, 0.402111f, 0.317717f, 0.247453f, 0.227744f, 0.279729f,
	  0.525206f, 0.402111f, 0.329937f, 0.295806f, 0.249855f, 0.212687f, 0.214459f, 0.254803f,
	  0.357432f, 0.279729f, 0.270896f, 0.262603f, 0.229778f, 0.257351f, 0.249855f, 0.259950f
	};

static const float Mask_Coeff[64] = 
	{ 0.000000f, 0.826446f, 1.000000f, 0.390625f, 0.173611f, 0.062500f, 0.038447f, 0.026874f,
	  0.694444f, 0.694444f, 0.510204f, 0.277008f, 0.147929f, 0.029727f, 0.027778f, 0.033058f,
	  0.510204f, 0.591716f, 0.390625f, 0.173611f, 0.062500f, 0.030779f, 0.021004f, 0.031888f,
	  0.510204f, 0.346021f, 0.206612f, 0.118906f, 0.038447f, 0.013212f, 0.015625f, 0.026015f,
	  0.308642f, 0.206612f, 0.073046f, 0.031888f, 0.021626f, 0.008417f, 0.009426f, 0.016866f,
	  0.173611f, 0.081633f, 0.033058f, 0.024414f, 0.015242f, 0.009246f, 0.007831f, 0.011815f,
	  0.041649f, 0.024414f, 0.016437f, 0.013212f, 0.009426f, 0.006830f, 0.006944f, 0.009803f,
	  0.019290f, 0.011815f, 0.011080f, 0.010412f, 0.007972f, 0.010000f, 0.009426f, 0.010203f
	};

#if 0 /* Floating-point implementation */

static uint32_t Calc_MSE_H(int16_t *DCT_A, int16_t *DCT_B, uint8_t *IMG_A, uint8_t *IMG_B, int stride)
{
	int x, y, i, j;
	uint32_t Global_A, Global_B, Sum_A = 0, Sum_B = 0;
	uint32_t Local[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t Local_Square[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	float MASK_A = 0.f, MASK_B = 0.f;
	float Mult1 = 1.f, Mult2 = 1.f;
	uint32_t MSE_H = 0;

	/* Step 1: Calculate CSF weighted energy of DCT coefficients */
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			MASK_A += (float)(DCT_A[y*8 + x]*DCT_A[y*8 + x])*Mask_Coeff[y*8 + x];
			MASK_B += (float)(DCT_B[y*8 + x]*DCT_B[y*8 + x])*Mask_Coeff[y*8 + x];
		}
	}

	/* Step 2: Determine local variances compared to entire block variance */
	for (y = 0; y < 2; y++) {
		for (x = 0; x < 2; x++) {
			for (j = 0; j < 4; j++) {
				for (i = 0; i < 4; i++) {
					uint8_t A = IMG_A[(y*4+j)*stride + 4*x + i];
					uint8_t B = IMG_B[(y*4+j)*stride + 4*x + i];

					Local[y*2 + x] += A;
					Local[y*2 + x + 4] += B;
					Local_Square[y*2 + x] += A*A;
					Local_Square[y*2 + x + 4] += B*B;
				}
			}
		}
	}

	Global_A = Local[0] + Local[1] + Local[2] + Local[3];
	Global_B = Local[4] + Local[5] + Local[6] + Local[7];

	for (i = 0; i < 8; i++)
		Local[i] = (Local_Square[i]<<4) - (Local[i]*Local[i]); /* 16*Var(Di) */

	Local_Square[0] += (Local_Square[1] + Local_Square[2] + Local_Square[3]);
	Local_Square[4] += (Local_Square[5] + Local_Square[6] + Local_Square[7]);

	Global_A = (Local_Square[0]<<6) - Global_A*Global_A; /* 64*Var(D) */
	Global_B = (Local_Square[4]<<6) - Global_B*Global_B; /* 64*Var(D) */

	/* Step 3: Calculate contrast masking threshold */
	if (Global_A) 
		Mult1 = (float)(Local[0]+Local[1]+Local[2]+Local[3])/((float)(Global_A)/4.f);

	if (Global_B)
		Mult2 = (float)(Local[4]+Local[5]+Local[6]+Local[7])/((float)(Global_B)/4.f);

	MASK_A = (float)sqrt(MASK_A * Mult1) / 32.f;
	MASK_B = (float)sqrt(MASK_B * Mult2) / 32.f;

	if (MASK_B > MASK_A) MASK_A = MASK_B; /* MAX(MASK_A, MASK_B) */

	/* Step 4: Calculate MSE of DCT coeffs reduced by masking effect */
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			float u = (float)abs(DCT_A[j*8 + i] - DCT_B[j*8 + i]);

			if ((i|j)>0) {
				if (u < (MASK_A / Mask_Coeff[j*8 + i])) 
					u = 0; /* The error is not perceivable */
				else
					u -= (MASK_A / Mask_Coeff[j*8 + i]);
			}
			
			MSE_H += (uint32_t) ((256.f*(u * CSF_Coeff[j*8 + i])*(u * CSF_Coeff[j*8 + i])) + 0.5f);
		}
	}
	return MSE_H; /* Fixed-point value right-shifted by eight */
}

#else /* First draft of a fixed-point implementation.
	     Might serve as a template for MMX/SSE code */

static const uint16_t iMask_Coeff[64] = 
	{     0, 59577, 65535, 40959, 27306, 16384, 12850, 10743,
	  54612, 54612, 46811, 34492, 25206, 11299, 10923, 11915,
      46811, 50412, 40959, 27306, 16384, 11497,  9498, 11703,
      46811, 38550, 29789, 22598, 12850,  7533,  8192, 10570,
      36408, 29789, 17712, 11703,  9637,  6012,  6363,  8511,
      27306, 18724, 11915, 10240,  8091,  6302,  5799,  7123,
      13374, 10240,  8402,  7533,  6363,  5416,  5461,  6489,
	   9102,  7123,  6898,  6687,  5851,  6553,  6363,  6620
	};

static const uint16_t Inv_iMask_Coeff[64] =
	{     0,   310,   256,   655,  1475,  4096,  6659,  9526,
	    369,   369,   502,   924,  1731,  8612,  9216,  7744,
	    502,   433,   655,  1475,  4096,  8317, 12188,  8028,
	    502,   740,  1239,  2153,  6659, 19376, 16384,  9840,
	    829,  1239,  3505,  8028, 11838, 30415, 27159, 15178,
	   1475,  3136,  7744, 10486, 16796, 27688, 32691, 21667,
	   6147, 10486, 15575, 19376, 27159, 37482, 36866, 26114,
	  13271, 21667, 23105, 24587, 32112, 25600, 27159, 25091
	};

static const uint16_t iCSF_Coeff[64] = 
	{ 1647, 2396, 2635, 1647, 1098, 659, 517, 432,
	  2196, 2196, 1882, 1387, 1014, 454, 439, 479,
	  1882, 2027, 1647, 1098,  659, 462, 382, 471,
	  1882, 1550, 1198,  909,  517, 303, 329, 425,
	  1464, 1198,  712,  471,  388, 242, 256, 342,
	  1098,  753,  479,  412,  325, 253, 233, 286,
	   538,  412,  338,  303,  256, 218, 220, 261,
	   366,  286,  277,  269,  235, 264, 256, 266
	};

static __inline uint32_t isqrt(unsigned long n)
{
    uint32_t c = 0x8000;
    uint32_t g = 0x8000;

    for(;;) {
        if(g*g > n)
            g ^= c;
        c >>= 1;
        if(c == 0)
            return g;
        g |= c;
    }
}

static uint32_t Calc_MSE_H(int16_t *DCT_A, int16_t *DCT_B, uint8_t *IMG_A, uint8_t *IMG_B, int stride)
{
	int x, y, i, j;
	uint32_t Global_A, Global_B, Sum_A = 0, Sum_B = 0;
	uint32_t Local[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t Local_Square[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t MASK;
	uint32_t MSE_H = 0;

	/* Step 1: Calculate CSF weighted energy of DCT coefficients */
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			uint16_t A = (abs(DCT_A[y*8 + x]) * iMask_Coeff[y*8 + x] + 4096) >> 13;
			uint16_t B = (abs(DCT_B[y*8 + x]) * iMask_Coeff[y*8 + x] + 4096) >> 13;

			Sum_A += ((A*A) >> 3); /* PMADDWD */
			Sum_B += ((B*B) >> 3);
		}
	}

	/* Step 2: Determine local variances compared to entire block variance */
	for (y = 0; y < 2; y++) {
		for (x = 0; x < 2; x++) {
			for (j = 0; j < 4; j++) {
				for (i = 0; i < 4; i++) {
					uint8_t A = IMG_A[(y*4+j)*stride + 4*x + i];
					uint8_t B = IMG_B[(y*4+j)*stride + 4*x + i];

					Local[y*2 + x] += A;
					Local[y*2 + x + 4] += B;
					Local_Square[y*2 + x] += A*A;
					Local_Square[y*2 + x + 4] += B*B;
				}
			}
		}
	}

	Global_A = Local[0] + Local[1] + Local[2] + Local[3];
	Global_B = Local[4] + Local[5] + Local[6] + Local[7];

	for (i = 0; i < 8; i++)
		Local[i] = (Local_Square[i]<<4) - (Local[i]*Local[i]); /* 16*Var(Di) */

	Local_Square[0] += (Local_Square[1] + Local_Square[2] + Local_Square[3]);
	Local_Square[4] += (Local_Square[5] + Local_Square[6] + Local_Square[7]);

	Global_A = (Local_Square[0]<<6) - Global_A*Global_A; /* 64*Var(D) */
	Global_B = (Local_Square[4]<<6) - Global_B*Global_B; /* 64*Var(D) */

	/* Step 3: Calculate contrast masking threshold */
	{ 
		uint32_t MASK_A, MASK_B;
		uint32_t Mult1 = 64, Mult2 = 64;

		if (Global_A) 
			Mult1 = ((Local[0]+Local[1]+Local[2]+Local[3])<<8) / Global_A;

		if (Global_B)
			Mult2 = ((Local[4]+Local[5]+Local[6]+Local[7])<<8) / Global_B;

		MASK_A = isqrt(2*Sum_A*Mult1) + 16;
		MASK_B = isqrt(2*Sum_B*Mult2) + 16;

		if (MASK_B > MASK_A)  /* MAX(MASK_A, MASK_B) */
			MASK = (uint32_t) MASK_B;
		else
			MASK = (uint32_t) MASK_A;
	}

	/* Step 4: Calculate MSE of DCT coeffs reduced by masking effect */
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint32_t Thresh = (MASK * Inv_iMask_Coeff[j*8 + i] + 128) >> 8;
			uint32_t u = abs(DCT_A[j*8 + i] - DCT_B[j*8 + i]) << 10;

			if ((i|j)>0) {
				if (u < Thresh)
					u = 0; /* The error is not perceivable */
				else
					u -= Thresh;
			}
			
			{
				uint64_t tmp = (u * iCSF_Coeff[j*8 + i] + 512) >> 10;
				MSE_H += (uint32_t) ((tmp * tmp) >> 12);
			}
		}
	}
	return MSE_H;
}

#endif

static void psnrhvsm_after(xvid_plg_data_t *data, psnrhvsm_data_t *psnrhvsm)
{
	DECLARE_ALIGNED_MATRIX(DCT, 2, 64, int16_t, CACHE_LINE);
	uint32_t x, y, stride = data->original.stride[0];
	int16_t *DCT_A = &DCT[0], *DCT_B = &DCT[64];
	uint8_t *IMG_A = (uint8_t *) data->original.plane[0];
	uint8_t *IMG_B = (uint8_t *) data->current.plane[0];
	uint64_t MSE_H = 0;

	for (y = 0; y < data->height; y += 8) {
		for (x = 0; x < data->width; x += 8) {
			int offset = y*stride + x;

			emms();

			/* Transfer data */
			transfer_8to16copy(DCT_A, IMG_A + offset, stride);
			transfer_8to16copy(DCT_B, IMG_B + offset, stride);

			/* Perform DCT */
			fdct(DCT_A);
			fdct(DCT_B);

			emms();

			/* Calculate MSE_H reduced by contrast masking effect */
			MSE_H += Calc_MSE_H(DCT_A, DCT_B, IMG_A + offset, IMG_B + offset, stride);
		}
	}

	x = 4*MSE_H / (data->width * data->height);
	psnrhvsm->mse_sum += x;
	psnrhvsm->frame_cnt++;

	printf("       psnrhvsm: %2.2f\n", sse_to_PSNR(x, 1024));
}

static int psnrhvsm_create(xvid_plg_create_t *create, void **handle)
{
	psnrhvsm_data_t *psnrhvsm;
	psnrhvsm = (psnrhvsm_data_t *) malloc(sizeof(psnrhvsm_data_t));

	psnrhvsm->mse_sum = 0;
	psnrhvsm->frame_cnt = 0;

	*(handle) = (void*) psnrhvsm;
	return 0;
}	

int xvid_plugin_psnrhvsm(void *handle, int opt, void *param1, void *param2)
{
	switch(opt) {
		case(XVID_PLG_INFO):
 			((xvid_plg_info_t *)param1)->flags = XVID_REQORIGINAL;
			break;
		case(XVID_PLG_CREATE):
			psnrhvsm_create((xvid_plg_create_t *)param1,(void **)param2);
			break;
		case(XVID_PLG_BEFORE):
		case(XVID_PLG_FRAME):
			break;
		case(XVID_PLG_AFTER):
			psnrhvsm_after((xvid_plg_data_t *)param1, (psnrhvsm_data_t *)handle);
			break;
		case(XVID_PLG_DESTROY):
			{
				uint32_t MSE_H;
				psnrhvsm_data_t *psnrhvsm = (psnrhvsm_data_t *)handle;
				
				if (psnrhvsm) {
					MSE_H = (uint32_t) (psnrhvsm->mse_sum / psnrhvsm->frame_cnt);

					emms();
					printf("Average psnrhvsm: %2.2f\n", sse_to_PSNR(MSE_H, 1024));
					free(psnrhvsm);
				}
			}
			break;
		default:
			break;
	}
	return 0;
};
