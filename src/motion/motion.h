/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  Motion sad header  -
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
 *  $Id: motion.h,v 1.20 2003-02-19 20:12:43 edgomez Exp $
 *
 ***************************************************************************/

#ifndef _MOTION_H_
#define _MOTION_H_

#include "../portab.h"
#include "../global.h"

// fast ((A)/2)*2
#define EVEN(A)		(((A)<0?(A)+1:(A)) & ~1)

#define MVzero(A) ( ((A).x)==(0) && ((A).y)==(0) )
#define MVequal(A,B) ( ((A).x)==((B).x) && ((A).y)==((B).y) )

/*****************************************************************************
 * Modified rounding tables -- defined in motion_est.c
 * Original tables see ISO spec tables 7-6 -> 7-9
 ****************************************************************************/

extern const uint32_t roundtab[16];

/* K = 4 */
extern const uint32_t roundtab_76[16];
/* K = 2 */
extern const uint32_t roundtab_78[8];
/* K = 1 */
extern const uint32_t roundtab_79[4];


/*
 * getref: calculate reference image pointer
 * the decision to use interpolation h/v/hv or the normal image is
 * based on dx & dy.
 */

static __inline const uint8_t *
get_ref(const uint8_t * const refn,
		const uint8_t * const refh,
		const uint8_t * const refv,
		const uint8_t * const refhv,
		const uint32_t x,
		const uint32_t y,
		const uint32_t block,	/* block dimension, 8 or 16 */

		const int32_t dx,
		const int32_t dy,
		const int32_t stride)
{


	switch (((dx & 1) << 1) + (dy & 1)) {	/* ((dx%2)?2:0)+((dy%2)?1:0) */
	case 0:
		return refn + (int) ((x * block + dx / 2) + (y * block + dy / 2) * stride);
	case 1:
		return refv + (int) ((x * block + dx / 2) + (y * block +
											  (dy - 1) / 2) * stride);
	case 2:
		return refh + (int) ((x * block + (dx - 1) / 2) + (y * block +
													dy / 2) * stride);
	default:
		return refhv + (int) ((x * block + (dx - 1) / 2) + (y * block +
													 (dy - 1) / 2) * stride);
	}

}


/* This is somehow a copy of get_ref, but with MV instead of X,Y */

static __inline const uint8_t *
get_ref_mv(const uint8_t * const refn,
		   const uint8_t * const refh,
		   const uint8_t * const refv,
		   const uint8_t * const refhv,
		   const uint32_t x,
		   const uint32_t y,
		   const uint32_t block,	/* block dimension, 8 or 16 */

		   const VECTOR * mv,	/* measured in half-pel! */

		   const int32_t stride)
{

	switch ((((mv->x) & 1) << 1) + ((mv->y) & 1)) {
	case 0:
		return refn + (int) ((x * block + (mv->x) / 2) + (y * block +
												   (mv->y) / 2) * stride);
	case 1:
		return refv + (int) ((x * block + (mv->x) / 2) + (y * block +
												   ((mv->y) - 1) / 2) * stride);
	case 2:
		return refh + (int) ((x * block + ((mv->x) - 1) / 2) + (y * block +
														 (mv->y) / 2) * stride);
	default:
		return refhv + (int) ((x * block + ((mv->x) - 1) / 2) + (y * block +
														  ((mv->y) -
														   1) / 2) * stride);
	}

}

void MotionEstimationBVOP(MBParam * const pParam,
						  FRAMEINFO * const frame,
						  // forward (past) reference 
						  const int32_t time_bp,
						  const int32_t time_pp,
						  const MACROBLOCK * const f_mbs,
						  const IMAGE * const f_ref,
						  const IMAGE * const f_refH,
						  const IMAGE * const f_refV,
						  const IMAGE * const f_refHV,
						  // backward (future) reference
						  const FRAMEINFO * const b_reference,
						  const IMAGE * const b_ref,
						  const IMAGE * const b_refH,
						  const IMAGE * const b_refV,
						  const IMAGE * const b_refHV);

void MBMotionCompensationBVOP(MBParam * pParam,
							  MACROBLOCK * const mb,
							  const uint32_t i,
							  const uint32_t j,
							  IMAGE * const cur,
							  const IMAGE * const f_ref,
							  const IMAGE * const f_refh,
							  const IMAGE * const f_refv,
							  const IMAGE * const f_refhv,
							  const IMAGE * const b_ref,
							  const IMAGE * const b_refh,
							  const IMAGE * const b_refv,
							  const IMAGE * const b_refhv,
							  int16_t * dct_codes);
							  
							  
/* GMC stuff. Maybe better put it into a separate file */
							  
void 
generate_GMCparameters(	const int num_wp,				// [input]: number of warppoints
						const int res, 					// [input]: resolution 
						const WARPPOINTS *const warp, 	// [input]: warp points  
						const int width, const int height, 	// [input]: without edges!
						GMC_DATA *const gmc); 		// [output] precalculated parameters 

void 
generate_GMCimage(	const GMC_DATA *const gmc_data, 	// [input] precalculated data
					const IMAGE *const pRef,			// [input] 
					const int mb_width, 
					const int mb_height,
					const int stride,
					const int stride2, 
					const int fcode, 					// [input] some parameters...
  					const int32_t quarterpel,			// [input] for rounding avgMV
					const int reduced_resolution,		// [input] ignored
					const int32_t rounding,			// [input] for rounding image data
					MACROBLOCK *const pMBs, 	// [output] average motion vectors
					IMAGE *const pGMC);			// [output] full warped image
					

VECTOR generate_GMCimageMB(	const GMC_DATA *const gmc_data, 	/* [input] all precalc data */
							const IMAGE *const pRef,			// [input] 
							const int mi, const int mj, 		/* [input] MB position  */
							const int stride,					/* [input] Lumi stride */
							const int stride2,					/* [input] chroma stride */
							const int quarterpel, 				/* [input] for rounding of AvgMV */
							const int rounding, 				
							IMAGE *const pGMC);					/* [outut] generate image */



/* Hinted ME */

void
MotionEstimationHinted(	MBParam * const pParam,
						FRAMEINFO * const current,
						FRAMEINFO * const reference,
						const IMAGE * const pRefH,
						const IMAGE * const pRefV,
						const IMAGE * const pRefHV);

int
MEanalysis(	const IMAGE * const pRef,	
			FRAMEINFO * const Current,
			MBParam * const pParam,
			int maxIntra,
			int intraCount,
			int bCount);

int
FindFcode(	const MBParam * const pParam,
			const FRAMEINFO * const current);


int d_amv_penalty(int x, int y, const VECTOR pred, const uint32_t iFcode, const int quant);
				

#endif							/* _MOTION_H_ */
