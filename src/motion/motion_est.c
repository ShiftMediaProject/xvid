/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	motion estimation 
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *  Modifications:
 *
 *	01.05.2002	updated MotionEstimationBVOP
 *	25.04.2002 partial prevMB conversion
 *  22.04.2002 remove some compile warning by chenm001 <chenm001@163.com>
 *  14.04.2002 added MotionEstimationBVOP()
 *  02.04.2002 add EPZS(^2) as ME algorithm, use PMV_USESQUARES to choose between 
 *             EPZS and EPZS^2
 *  08.02.2002 split up PMVfast into three routines: PMVFast, PMVFast_MainLoop
 *             PMVFast_Refine to support multiple searches with different start points
 *  07.01.2002 uv-block-based interpolation
 *  06.01.2002 INTER/INTRA-decision is now done before any SEARCH8 (speedup)
 *             changed INTER_BIAS to 150 (as suggested by suxen_drol)
 *             removed halfpel refinement step in PMVfastSearch8 + quality=5
 *             added new quality mode = 6 which performs halfpel refinement
 *             filesize difference between quality 5 and 6 is smaller than 1%
 *             (Isibaar)
 *  31.12.2001 PMVfastSearch16 and PMVfastSearch8 (gruel)
 *  30.12.2001 get_range/MotionSearchX simplified; blue/green bug fix
 *  22.12.2001 commented best_point==99 check
 *  19.12.2001 modified get_range (purple bug fix)
 *  15.12.2001 moved pmv displacement from mbprediction
 *  02.12.2001 motion estimation/compensation split (Isibaar)
 *  16.11.2001 rewrote/tweaked search algorithms; pross@cs.rmit.edu.au
 *  10.11.2001 support for sad16/sad8 functions
 *  28.08.2001 reactivated MODE_INTER4V for EXT_MODE
 *  24.08.2001 removed MODE_INTER4V_Q, disabled MODE_INTER4V for EXT_MODE
 *  22.08.2001 added MODE_INTER4V_Q			
 *  20.08.2001 added pragma to get rid of internal compiler error with VC6
 *             idea by Cyril. Thanks.
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../encoder.h"
#include "../utils/mbfunctions.h"
#include "../prediction/mbprediction.h"
#include "../global.h"
#include "../utils/timer.h"
#include "motion.h"
#include "sad.h"



static int32_t lambda_vec16[32] =	/* rounded values for lambda param for weight of motion bits as in modified H.26L */
{ 0, (int) (1.00235 + 0.5), (int) (1.15582 + 0.5), (int) (1.31976 + 0.5),
		(int) (1.49591 + 0.5), (int) (1.68601 + 0.5),
	(int) (1.89187 + 0.5), (int) (2.11542 + 0.5), (int) (2.35878 + 0.5),
		(int) (2.62429 + 0.5), (int) (2.91455 + 0.5),
	(int) (3.23253 + 0.5), (int) (3.58158 + 0.5), (int) (3.96555 + 0.5),
		(int) (4.38887 + 0.5), (int) (4.85673 + 0.5),
	(int) (5.37519 + 0.5), (int) (5.95144 + 0.5), (int) (6.59408 + 0.5),
		(int) (7.31349 + 0.5), (int) (8.12242 + 0.5),
	(int) (9.03669 + 0.5), (int) (10.0763 + 0.5), (int) (11.2669 + 0.5),
		(int) (12.6426 + 0.5), (int) (14.2493 + 0.5),
	(int) (16.1512 + 0.5), (int) (18.442 + 0.5), (int) (21.2656 + 0.5),
		(int) (24.8580 + 0.5), (int) (29.6436 + 0.5),
	(int) (36.4949 + 0.5)
};

static int32_t *lambda_vec8 = lambda_vec16;	/* same table for INTER and INTER4V for now */



// mv.length table
static const uint32_t mvtab[33] = {
	1, 2, 3, 4, 6, 7, 7, 7,
	9, 9, 9, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10,
	10, 11, 11, 11, 11, 11, 11, 12, 12
};


static __inline uint32_t
mv_bits(int32_t component,
		const uint32_t iFcode)
{
	if (component == 0)
		return 1;

	if (component < 0)
		component = -component;

	if (iFcode == 1) {
		if (component > 32)
			component = 32;

		return mvtab[component] + 1;
	}

	component += (1 << (iFcode - 1)) - 1;
	component >>= (iFcode - 1);

	if (component > 32)
		component = 32;

	return mvtab[component] + 1 + iFcode - 1;
}


static __inline uint32_t
calc_delta_16(const int32_t dx,
			  const int32_t dy,
			  const uint32_t iFcode,
			  const uint32_t iQuant)
{
	return NEIGH_TEND_16X16 * lambda_vec16[iQuant] * (mv_bits(dx, iFcode) +
													  mv_bits(dy, iFcode));
}

static __inline uint32_t
calc_delta_8(const int32_t dx,
			 const int32_t dy,
			 const uint32_t iFcode,
			 const uint32_t iQuant)
{
	return NEIGH_TEND_8X8 * lambda_vec8[iQuant] * (mv_bits(dx, iFcode) +
												   mv_bits(dy, iFcode));
}
 
bool
MotionEstimation(MBParam * const pParam,
				 FRAMEINFO * const current,
				 FRAMEINFO * const reference,
				 const IMAGE * const pRefH,
				 const IMAGE * const pRefV,
				 const IMAGE * const pRefHV,
				 const uint32_t iLimit)
{
	const uint32_t iWcount = pParam->mb_width;
	const uint32_t iHcount = pParam->mb_height;
	MACROBLOCK *const pMBs = current->mbs;
	MACROBLOCK *const prevMBs = reference->mbs;
	const IMAGE *const pCurrent = &current->image;
	const IMAGE *const pRef = &reference->image;

	static const VECTOR zeroMV = { 0, 0 };
	VECTOR predMV;

	int32_t x, y;
	int32_t iIntra = 0;
	VECTOR pmv;

	if (sadInit)
		(*sadInit) ();

	for (y = 0; y < iHcount; y++)	{
		for (x = 0; x < iWcount; x ++)	{
	
			MACROBLOCK *const pMB = &pMBs[x + y * iWcount];

			predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 0);    

			pMB->sad16 =
				SEARCH16(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						 x, y, predMV.x, predMV.y, predMV.x, predMV.y, 
						 current->motion_flags, current->quant,
						 current->fcode, pParam, pMBs, prevMBs, &pMB->mv16,
						 &pMB->pmvs[0]);

			if (0 < (pMB->sad16 - MV16_INTER_BIAS)) {
				int32_t deviation;

				deviation =
					dev16(pCurrent->y + x * 16 + y * 16 * pParam->edged_width,
						  pParam->edged_width);

				if (deviation < (pMB->sad16 - MV16_INTER_BIAS)) {
					pMB->mode = MODE_INTRA;
					pMB->mv16 = pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] =
						pMB->mvs[3] = zeroMV;
					pMB->sad16 = pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] =
						pMB->sad8[3] = 0;

					iIntra++;
					if (iIntra >= iLimit)
						return 1;

					continue;
				}
			}

			pmv = pMB->pmvs[0];
			if (current->global_flags & XVID_INTER4V)
				if ((!(current->global_flags & XVID_LUMIMASKING) ||
					 pMB->dquant == NO_CHANGE)) {
					int32_t sad8 = IMV16X16 * current->quant;

					if (sad8 < pMB->sad16) {
						sad8 += pMB->sad8[0] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x, 2 * y, 
									pMB->mv16.x, pMB->mv16.y, predMV.x, predMV.y, 
									current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[0],
									&pMB->pmvs[0]);
					}
					if (sad8 < pMB->sad16) {

						predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 1);    
						sad8 += pMB->sad8[1] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x + 1, 2 * y,
									pMB->mv16.x, pMB->mv16.y, predMV.x, predMV.y, 
									current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[1],
									&pMB->pmvs[1]);
					}
					if (sad8 < pMB->sad16) {
						predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 2);    
						sad8 += pMB->sad8[2] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x, 2 * y + 1, 
									pMB->mv16.x, pMB->mv16.y, predMV.x, predMV.y, 
									current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[2],
									&pMB->pmvs[2]);
					}
					if (sad8 < pMB->sad16) {
						predMV = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 3);  
						sad8 += pMB->sad8[3] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x + 1, 2 * y + 1,
									pMB->mv16.x, pMB->mv16.y, predMV.x, predMV.y, 
									current->motion_flags, 
									current->quant, current->fcode, pParam, 
									pMBs, prevMBs,
									&pMB->mvs[3], 
									&pMB->pmvs[3]);
					}
					
					/* decide: MODE_INTER or MODE_INTER4V
					   mpeg4:   if (sad8 < pMB->sad16 - nb/2+1) use_inter4v
					 */

					if (sad8 < pMB->sad16) {
						pMB->mode = MODE_INTER4V;
						pMB->sad8[0] *= 4;
						pMB->sad8[1] *= 4;
						pMB->sad8[2] *= 4;
						pMB->sad8[3] *= 4;
						continue;
					}

				}

			pMB->mode = MODE_INTER;
			pMB->pmvs[0] = pmv;	/* pMB->pmvs[1] = pMB->pmvs[2] = pMB->pmvs[3]  are not needed for INTER */
			pMB->mvs[0] = pMB->mvs[1] = pMB->mvs[2] = pMB->mvs[3] = pMB->mv16;
			pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] =
				pMB->sad16;
			}
			}

	return 0;
}


#define CHECK_MV16_ZERO {\
  if ( (0 <= max_dx) && (0 >= min_dx) \
    && (0 <= max_dy) && (0 >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, 0, 0 , iEdgedWidth), iEdgedWidth, MV_MAX_ERROR); \
    iSAD += calc_delta_16(-center_x, -center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=0; currMV->y=0; }  }	\
}

#define NOCHECK_MV16_CANDIDATE(X,Y) { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } \
}

#define CHECK_MV16_CANDIDATE(X,Y) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}

#define CHECK_MV16_CANDIDATE_DIR(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}

#define CHECK_MV16_CANDIDATE_FOUND(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}


#define CHECK_MV8_ZERO {\
  iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, 0, 0 , iEdgedWidth), iEdgedWidth); \
  iSAD += calc_delta_8(-center_x, -center_y, (uint8_t)iFcode, iQuant);\
  if (iSAD < iMinSAD) \
  { iMinSAD=iSAD; currMV->x=0; currMV->y=0; } \
}

#define NOCHECK_MV8_CANDIDATE(X,Y) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-center_x, (Y)-center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } \
}

#define CHECK_MV8_CANDIDATE(X,Y) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-center_x, (Y)-center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}

#define CHECK_MV8_CANDIDATE_DIR(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-center_x, (Y)-center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}

#define CHECK_MV8_CANDIDATE_FOUND(X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad8( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 8, (X), (Y), iEdgedWidth),iEdgedWidth); \
    iSAD += calc_delta_8((X)-center_x, (Y)-center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}

/* too slow and not fully functional at the moment */
/*
int32_t ZeroSearch16(
					const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const IMAGE * const pCur,
					const int x, const int y,
					const uint32_t MotionFlags, 				
					const uint32_t iQuant,
					const uint32_t iFcode,
					MBParam * const pParam,
					const MACROBLOCK * const pMBs,				
					const MACROBLOCK * const prevMBs,
					VECTOR * const currMV,
					VECTOR * const currPMV)
{
	const int32_t iEdgedWidth = pParam->edged_width; 
	const uint8_t * cur = pCur->y + x*16 + y*16*iEdgedWidth;
	int32_t iSAD;
	VECTOR pred;
	

	pred = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 0);    

	iSAD = sad16( cur, 
		get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, 0,0, iEdgedWidth),
		iEdgedWidth, MV_MAX_ERROR);
	if (iSAD <= iQuant * 96)	
	   	iSAD -= MV16_00_BIAS; 

	currMV->x = 0;
	currMV->y = 0;
	currPMV->x = -pred.x;
	currPMV->y = -pred.y;

	return iSAD;

}
*/

int32_t
Diamond16_MainSearch(const uint8_t * const pRef,
					 const uint8_t * const pRefH,
					 const uint8_t * const pRefV,
					 const uint8_t * const pRefHV,
					 const uint8_t * const cur,
					 const int x,
					 const int y,
				   const int start_x,
				   const int start_y,
				   int iMinSAD,
				   VECTOR * const currMV,
				   const int center_x,
				   const int center_y,
					 const int32_t min_dx,
					 const int32_t max_dx,
					 const int32_t min_dy,
					 const int32_t max_dy,
					 const int32_t iEdgedWidth,
					 const int32_t iDiamondSize,
					 const int32_t iFcode,
					 const int32_t iQuant,
					 int iFound)
{
/* Do a diamond search around given starting point, return SAD of best */

	int32_t iDirection = 0;
	int32_t iSAD;
	VECTOR backupMV;

	backupMV.x = start_x;
	backupMV.y = start_y;

/* It's one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y, 1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y, 2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize, 3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize, 4);

	if (iDirection)
		while (!iFound) {
			iFound = 1;
			backupMV = *currMV;

			if (iDirection != 2)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x - iDiamondSize,
										   backupMV.y, 1);
			if (iDirection != 1)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x + iDiamondSize,
										   backupMV.y, 2);
			if (iDirection != 4)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x,
										   backupMV.y - iDiamondSize, 3);
			if (iDirection != 3)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x,
										   backupMV.y + iDiamondSize, 4);
	} else {
		currMV->x = start_x;
		currMV->y = start_y;
	}
	return iMinSAD;
}

int32_t
Square16_MainSearch(const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const uint8_t * const cur,
					const int x,
					const int y,
				   const int start_x,
				   const int start_y,
				   int iMinSAD,
				   VECTOR * const currMV,
				   const int center_x,
				   const int center_y,
					const int32_t min_dx,
					const int32_t max_dx,
					const int32_t min_dy,
					const int32_t max_dy,
					const int32_t iEdgedWidth,
					const int32_t iDiamondSize,
					const int32_t iFcode,
					const int32_t iQuant,
					int iFound)
{
/* Do a square search around given starting point, return SAD of best */

	int32_t iDirection = 0;
	int32_t iSAD;
	VECTOR backupMV;

	backupMV.x = start_x;
	backupMV.y = start_y;

/* It's one search with full square pattern, and new parts for all following diamonds */

/*   new direction are extra, so 1-4 is normal diamond
      537
      1*2
      648  
*/

	CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y, 1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y, 2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize, 3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize, 4);

	CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
							 backupMV.y - iDiamondSize, 5);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
							 backupMV.y + iDiamondSize, 6);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
							 backupMV.y - iDiamondSize, 7);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
							 backupMV.y + iDiamondSize, 8);


	if (iDirection)
		while (!iFound) {
			iFound = 1;
			backupMV = *currMV;

			switch (iDirection) {
			case 1:
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x - iDiamondSize,
										   backupMV.y, 1);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				break;
			case 2:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y,
										 2);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);
				break;

			case 3:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize,
										 4);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);
				break;

			case 4:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize,
										 3);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				break;

			case 5:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y,
										 1);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize,
										 3);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				break;

			case 6:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y,
										 2);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize,
										 3);

				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);

				break;

			case 7:
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x - iDiamondSize,
										   backupMV.y, 1);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize,
										 4);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);
				break;

			case 8:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y,
										 2);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize,
										 4);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);
				break;
			default:
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y,
										 1);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y,
										 2);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize,
										 3);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize,
										 4);

				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y - iDiamondSize, 5);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize,
										 backupMV.y + iDiamondSize, 6);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y - iDiamondSize, 7);
				CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize,
										 backupMV.y + iDiamondSize, 8);
				break;
			}
	} else {
		currMV->x = start_x;
		currMV->y = start_y;
	}
	return iMinSAD;
}


int32_t
Full16_MainSearch(const uint8_t * const pRef,
				  const uint8_t * const pRefH,
				  const uint8_t * const pRefV,
				  const uint8_t * const pRefHV,
				  const uint8_t * const cur,
				  const int x,
				  const int y,
				   const int start_x,
				   const int start_y,
				   int iMinSAD,
				   VECTOR * const currMV,
				   const int center_x,
				   const int center_y,
				  const int32_t min_dx,
				  const int32_t max_dx,
				  const int32_t min_dy,
				  const int32_t max_dy,
				  const int32_t iEdgedWidth,
				  const int32_t iDiamondSize,
				  const int32_t iFcode,
				  const int32_t iQuant,
				  int iFound)
{
	int32_t iSAD;
	int32_t dx, dy;
	VECTOR backupMV;

	backupMV.x = start_x;
	backupMV.y = start_y;

	for (dx = min_dx; dx <= max_dx; dx += iDiamondSize)
		for (dy = min_dy; dy <= max_dy; dy += iDiamondSize)
			NOCHECK_MV16_CANDIDATE(dx, dy);

	return iMinSAD;
}

int32_t
AdvDiamond16_MainSearch(const uint8_t * const pRef,
						const uint8_t * const pRefH,
						const uint8_t * const pRefV,
						const uint8_t * const pRefHV,
						const uint8_t * const cur,
						const int x,
						const int y,
					   int start_x,
					   int start_y,
					   int iMinSAD,
					   VECTOR * const currMV,
					   const int center_x,
					   const int center_y,
						const int32_t min_dx,
						const int32_t max_dx,
						const int32_t min_dy,
						const int32_t max_dy,
						const int32_t iEdgedWidth,
						const int32_t iDiamondSize,
						const int32_t iFcode,
						const int32_t iQuant,
						int iDirection)
{

	int32_t iSAD;

/* directions: 1 - left (x-1); 2 - right (x+1), 4 - up (y-1); 8 - down (y+1) */

	if (iDirection) {
		CHECK_MV16_CANDIDATE(start_x - iDiamondSize, start_y);
		CHECK_MV16_CANDIDATE(start_x + iDiamondSize, start_y);
		CHECK_MV16_CANDIDATE(start_x, start_y - iDiamondSize);
		CHECK_MV16_CANDIDATE(start_x, start_y + iDiamondSize);
	} else {
		int bDirection = 1 + 2 + 4 + 8;

		do {
			iDirection = 0;
			if (bDirection & 1)	//we only want to check left if we came from the right (our last motion was to the left, up-left or down-left)
				CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize, start_y, 1);

			if (bDirection & 2)
				CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize, start_y, 2);

			if (bDirection & 4)
				CHECK_MV16_CANDIDATE_DIR(start_x, start_y - iDiamondSize, 4);

			if (bDirection & 8)
				CHECK_MV16_CANDIDATE_DIR(start_x, start_y + iDiamondSize, 8);

			/* now we're doing diagonal checks near our candidate */

			if (iDirection)		//checking if anything found
			{
				bDirection = iDirection;
				iDirection = 0;
				start_x = currMV->x;
				start_y = currMV->y;
				if (bDirection & 3)	//our candidate is left or right
				{
					CHECK_MV16_CANDIDATE_DIR(start_x, start_y + iDiamondSize, 8);
					CHECK_MV16_CANDIDATE_DIR(start_x, start_y - iDiamondSize, 4);
				} else			// what remains here is up or down
				{
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize, start_y, 2);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize, start_y, 1);
				}

				if (iDirection) {
					bDirection += iDirection;
					start_x = currMV->x;
					start_y = currMV->y;
				}
			} else				//about to quit, eh? not so fast....
			{
				switch (bDirection) {
				case 2:
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					break;
				case 1:
	
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					break;
				case 2 + 4:
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					break;
				case 4:
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					break;
				case 8:
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					break;
				case 1 + 4:
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					break;
				case 2 + 8:
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					break;
				case 1 + 8:
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					break;
				default:		//1+2+4+8 == we didn't find anything at all
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y - iDiamondSize, 1 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x - iDiamondSize,
											 start_y + iDiamondSize, 1 + 8);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y - iDiamondSize, 2 + 4);
					CHECK_MV16_CANDIDATE_DIR(start_x + iDiamondSize,
											 start_y + iDiamondSize, 2 + 8);
					break;
				}
				if (!iDirection)
					break;		//ok, the end. really
				else {
					bDirection = iDirection;
					start_x = currMV->x;
					start_y = currMV->y;
				}
			}
		}
		while (1);				//forever
	}
	return iMinSAD;
}


#define CHECK_MV16_F_INTERPOL(X,Y,BX,BY) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}

#define CHECK_MV16_F_INTERPOL_DIR(X,Y,BX,BY,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}

#define CHECK_MV16_F_INTERPOL_FOUND(X,Y,BX,BY,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}


#define CHECK_MV16_B_INTERPOL(FX,FY,X,Y) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); } } \
}


#define CHECK_MV16_B_INTERPOL_DIR(FX,FY,X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); } } \
}


#define CHECK_MV16_B_INTERPOL_FOUND(FX,FY,X,Y,D) { \
  if ( ((X) <= max_dx) && ((X) >= min_dx) \
    && ((Y) <= max_dy) && ((Y) >= min_dy) ) \
  { \
    iSAD = sad16( cur, get_ref(pRef, pRefH, pRefV, pRefHV, x, y, 16, X, Y, iEdgedWidth),iEdgedWidth, iMinSAD); \
    iSAD += calc_delta_16((X) - center_x, (Y) - center_y, (uint8_t)iFcode, iQuant);\
    if (iSAD < iMinSAD) \
    {  iMinSAD=iSAD; currMV->x=(X); currMV->y=(Y); iDirection=(D); iFound=0; } } \
}


#if (0==1)
int32_t
Diamond16_InterpolMainSearch(
					const uint8_t * const f_pRef,
					 const uint8_t * const f_pRefH,
					 const uint8_t * const f_pRefV,
					 const uint8_t * const f_pRefHV,
					 const uint8_t * const cur,

					const uint8_t * const b_pRef,
					 const uint8_t * const b_pRefH,
					 const uint8_t * const b_pRefV,
					 const uint8_t * const b_pRefHV,

					 const int x,
					 const int y,

				   const int f_start_x,
				   const int f_start_y,
				   const int b_start_x,
				   const int b_start_y,

				   int iMinSAD,
				   VECTOR * const f_currMV,
				   VECTOR * const b_currMV,

				   const int f_center_x,
				   const int f_center_y,
				   const int b_center_x,
				   const int b_center_y,

					 const int32_t min_dx,
					 const int32_t max_dx,
					 const int32_t min_dy,
					 const int32_t max_dy,
					 const int32_t iEdgedWidth,
					 const int32_t iDiamondSize,

					 const int32_t f_iFcode,
					 const int32_t b_iFcode,

					 const int32_t iQuant,
					 int iFound)
{
/* Do a diamond search around given starting point, return SAD of best */

	int32_t f_iDirection = 0;
	int32_t b_iDirection = 0;
	int32_t iSAD;

	VECTOR f_backupMV;
	VECTOR b_backupMV;

	f_backupMV.x = start_x;
	f_backupMV.y = start_y;
	b_backupMV.x = start_x;
	b_backupMV.y = start_y;

/* It's one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	CHECK_MV16_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y, 1);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y, 2);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize, 3);
	CHECK_MV16_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize, 4);

	if (iDirection)
		while (!iFound) {
			iFound = 1;
			backupMV = *currMV;

			if (iDirection != 2)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x - iDiamondSize,
										   backupMV.y, 1);
			if (iDirection != 1)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x + iDiamondSize,
										   backupMV.y, 2);
			if (iDirection != 4)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x,
										   backupMV.y - iDiamondSize, 3);
			if (iDirection != 3)
				CHECK_MV16_CANDIDATE_FOUND(backupMV.x,
										   backupMV.y + iDiamondSize, 4);
	} else {
		currMV->x = start_x;
		currMV->y = start_y;
	}
	return iMinSAD;
}
#endif


int32_t
AdvDiamond8_MainSearch(const uint8_t * const pRef,
					   const uint8_t * const pRefH,
					   const uint8_t * const pRefV,
					   const uint8_t * const pRefHV,
					   const uint8_t * const cur,
					   const int x,
					   const int y,
					   int start_x,
					   int start_y,
					   int iMinSAD,
					   VECTOR * const currMV,
					   const int center_x,
					   const int center_y,
					   const int32_t min_dx,
					   const int32_t max_dx,
					   const int32_t min_dy,
					   const int32_t max_dy,
					   const int32_t iEdgedWidth,
					   const int32_t iDiamondSize,
					   const int32_t iFcode,
					   const int32_t iQuant,
					   int iDirection)
{

	int32_t iSAD;

/* directions: 1 - left (x-1); 2 - right (x+1), 4 - up (y-1); 8 - down (y+1) */

	if (iDirection) {
		CHECK_MV8_CANDIDATE(start_x - iDiamondSize, start_y);
		CHECK_MV8_CANDIDATE(start_x + iDiamondSize, start_y);
		CHECK_MV8_CANDIDATE(start_x, start_y - iDiamondSize);
		CHECK_MV8_CANDIDATE(start_x, start_y + iDiamondSize);
	} else {
		int bDirection = 1 + 2 + 4 + 8;

		do {
			iDirection = 0;
			if (bDirection & 1)	//we only want to check left if we came from the right (our last motion was to the left, up-left or down-left)
				CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize, start_y, 1);

			if (bDirection & 2)
				CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize, start_y, 2);

			if (bDirection & 4)
				CHECK_MV8_CANDIDATE_DIR(start_x, start_y - iDiamondSize, 4);

			if (bDirection & 8)
				CHECK_MV8_CANDIDATE_DIR(start_x, start_y + iDiamondSize, 8);

			/* now we're doing diagonal checks near our candidate */

			if (iDirection)		//checking if anything found
			{
				bDirection = iDirection;
				iDirection = 0;
				start_x = currMV->x;
				start_y = currMV->y;
				if (bDirection & 3)	//our candidate is left or right
				{
					CHECK_MV8_CANDIDATE_DIR(start_x, start_y + iDiamondSize, 8);
					CHECK_MV8_CANDIDATE_DIR(start_x, start_y - iDiamondSize, 4);
				} else			// what remains here is up or down
				{
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize, start_y, 2);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize, start_y, 1);
				}

				if (iDirection) {
					bDirection += iDirection;
					start_x = currMV->x;
					start_y = currMV->y;
				}
			} else				//about to quit, eh? not so fast....
			{
				switch (bDirection) {
				case 2:
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					break;
				case 1:
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					break;
				case 2 + 4:
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					break;
				case 4:
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					break;
				case 8:
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					break;
				case 1 + 4:
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					break;
				case 2 + 8:
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					break;
				case 1 + 8:
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					break;
				default:		//1+2+4+8 == we didn't find anything at all
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y - iDiamondSize, 1 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x - iDiamondSize,
											start_y + iDiamondSize, 1 + 8);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y - iDiamondSize, 2 + 4);
					CHECK_MV8_CANDIDATE_DIR(start_x + iDiamondSize,
											start_y + iDiamondSize, 2 + 8);
					break;
				}
				if (!(iDirection))
					break;		//ok, the end. really
				else {
					bDirection = iDirection;
					start_x = currMV->x;
					start_y = currMV->y;
				}
			}
		}
		while (1);				//forever
	}
	return iMinSAD;
}


int32_t
Full8_MainSearch(const uint8_t * const pRef,
				 const uint8_t * const pRefH,
				 const uint8_t * const pRefV,
				 const uint8_t * const pRefHV,
				 const uint8_t * const cur,
				 const int x,
				 const int y,
			   const int start_x,
			   const int start_y,
			   int iMinSAD,
			   VECTOR * const currMV,
			   const int center_x,
			   const int center_y,
				 const int32_t min_dx,
				 const int32_t max_dx,
				 const int32_t min_dy,
				 const int32_t max_dy,
				 const int32_t iEdgedWidth,
				 const int32_t iDiamondSize,
				 const int32_t iFcode,
				 const int32_t iQuant,
				 int iFound)
{
	int32_t iSAD;
	int32_t dx, dy;
	VECTOR backupMV;

	backupMV.x = start_x;
	backupMV.y = start_y;

	for (dx = min_dx; dx <= max_dx; dx += iDiamondSize)
		for (dy = min_dy; dy <= max_dy; dy += iDiamondSize)
			NOCHECK_MV8_CANDIDATE(dx, dy);

	return iMinSAD;
}

Halfpel8_RefineFuncPtr Halfpel8_Refine;

int32_t
Halfpel16_Refine(const uint8_t * const pRef,
				 const uint8_t * const pRefH,
				 const uint8_t * const pRefV,
				 const uint8_t * const pRefHV,
				 const uint8_t * const cur,
				 const int x,
				 const int y,
				 VECTOR * const currMV,
				 int32_t iMinSAD,
			   const int center_x,
			   const int center_y,
				 const int32_t min_dx,
				 const int32_t max_dx,
				 const int32_t min_dy,
				 const int32_t max_dy,
				 const int32_t iFcode,
				 const int32_t iQuant,
				 const int32_t iEdgedWidth)
{
/* Do a half-pel refinement (or rather a "smallest possible amount" refinement) */

	int32_t iSAD;
	VECTOR backupMV = *currMV;

	CHECK_MV16_CANDIDATE(backupMV.x - 1, backupMV.y - 1);
	CHECK_MV16_CANDIDATE(backupMV.x, backupMV.y - 1);
	CHECK_MV16_CANDIDATE(backupMV.x + 1, backupMV.y - 1);
	CHECK_MV16_CANDIDATE(backupMV.x - 1, backupMV.y);
	CHECK_MV16_CANDIDATE(backupMV.x + 1, backupMV.y);
	CHECK_MV16_CANDIDATE(backupMV.x - 1, backupMV.y + 1);
	CHECK_MV16_CANDIDATE(backupMV.x, backupMV.y + 1);
	CHECK_MV16_CANDIDATE(backupMV.x + 1, backupMV.y + 1);

	return iMinSAD;
}

#define PMV_HALFPEL16 (PMV_HALFPELDIAMOND16|PMV_HALFPELREFINE16)



int32_t
PMVfastSearch16(const uint8_t * const pRef,
				const uint8_t * const pRefH,
				const uint8_t * const pRefV,
				const uint8_t * const pRefHV,
				const IMAGE * const pCur,
				const int x,
				const int y,
				const int start_x,
				const int start_y,
				const int center_x,
				const int center_y,
				const uint32_t MotionFlags,
				const uint32_t iQuant,
				const uint32_t iFcode,
				const MBParam * const pParam,
				const MACROBLOCK * const pMBs,
				const MACROBLOCK * const prevMBs,
				VECTOR * const currMV,
				VECTOR * const currPMV)
{
	const uint32_t iWcount = pParam->mb_width;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width;

	const uint8_t *cur = pCur->y + x * 16 + y * 16 * iEdgedWidth;

	int32_t iDiamondSize;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;

	int32_t iFound;

	VECTOR newMV;
	VECTOR backupMV;			/* just for PMVFAST */

	VECTOR pmv[4];
	int32_t psad[4];

	MainSearch16FuncPtr MainSearchPtr;

	const MACROBLOCK *const prevMB = prevMBs + x + y * iWcount;

	int32_t threshA, threshB;
	int32_t bPredEq;
	int32_t iMinSAD, iSAD;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy, x, y, 16, iWidth, iHeight,
			  iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPEL16)) {
		min_dx = EVEN(min_dx);
		max_dx = EVEN(max_dx);
		min_dy = EVEN(min_dy);
		max_dy = EVEN(max_dy);
	}

	/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
	//bPredEq = get_pmvdata(pMBs, x, y, iWcount, 0, pmv, psad);
	bPredEq = get_pmvdata2(pMBs, iWcount, 0, x, y, 0, pmv, psad);

	if ((x == 0) && (y == 0)) {
		threshA = 512;
		threshB = 1024;
	} else {
		threshA = psad[0];
		threshB = threshA + 256;
		if (threshA < 512)
			threshA = 512;
		if (threshA > 1024)
			threshA = 1024;
		if (threshB > 1792)
			threshB = 1792;
	}

	iFound = 0;

/* Step 4: Calculate SAD around the Median prediction. 
   MinSAD=SAD 
   If Motion Vector equal to Previous frame motion vector 
   and MinSAD<PrevFrmSAD goto Step 10. 
   If SAD<=256 goto Step 10. 
*/

	currMV->x = start_x;		
	currMV->y = start_y;		

	if (!(MotionFlags & PMV_HALFPEL16)) {	/* This should NOT be necessary! */
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}

	if (currMV->x > max_dx) {
		currMV->x = max_dx;
	}
	if (currMV->x < min_dx) {
		currMV->x = min_dx;
	}
	if (currMV->y > max_dy) {
		currMV->y = max_dy;
	}
	if (currMV->y < min_dy) {
		currMV->y = min_dy;
	}

	iMinSAD =
		sad16(cur,
			  get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 16, currMV,
						 iEdgedWidth), iEdgedWidth, MV_MAX_ERROR);
	iMinSAD +=
		calc_delta_16(currMV->x - center_x, currMV->y - center_y,
					  (uint8_t) iFcode, iQuant);

	if ((iMinSAD < 256) ||
		((MVequal(*currMV, prevMB->mvs[0])) &&
		 ((int32_t) iMinSAD < prevMB->sad16))) {
		if (iMinSAD < 2 * iQuant)	// high chances for SKIP-mode
		{
			if (!MVzero(*currMV)) {
				iMinSAD += MV16_00_BIAS;
				CHECK_MV16_ZERO;	// (0,0) saves space for letterboxed pictures
				iMinSAD -= MV16_00_BIAS;
			}
		}

		if (MotionFlags & PMV_QUICKSTOP16)
			goto PMVfast16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast16_Terminate_with_Refine;
	}


/* Step 2 (lazy eval): Calculate Distance= |MedianMVX| + |MedianMVY| where MedianMV is the motion 
   vector of the median. 
   If PredEq=1 and MVpredicted = Previous Frame MV, set Found=2  
*/

	if ((bPredEq) && (MVequal(pmv[0], prevMB->mvs[0])))
		iFound = 2;

/* Step 3 (lazy eval): If Distance>0 or thresb<1536 or PredEq=1 Select small Diamond Search. 
   Otherwise select large Diamond Search. 
*/

	if ((!MVzero(pmv[0])) || (threshB < 1536) || (bPredEq))
		iDiamondSize = 1;		// halfpel!
	else
		iDiamondSize = 2;		// halfpel!

	if (!(MotionFlags & PMV_HALFPELDIAMOND16))
		iDiamondSize *= 2;

/* 
   Step 5: Calculate SAD for motion vectors taken from left block, top, top-right, and Previous frame block. 
   Also calculate (0,0) but do not subtract offset. 
   Let MinSAD be the smallest SAD up to this point. 
   If MV is (0,0) subtract offset. 
*/

// (0,0) is always possible

	if (!MVzero(pmv[0]))
		CHECK_MV16_ZERO;

// previous frame MV is always possible

	if (!MVzero(prevMB->mvs[0]))
		if (!MVequal(prevMB->mvs[0], pmv[0]))
			CHECK_MV16_CANDIDATE(prevMB->mvs[0].x, prevMB->mvs[0].y);

// left neighbour, if allowed

	if (!MVzero(pmv[1]))
		if (!MVequal(pmv[1], prevMB->mvs[0]))
			if (!MVequal(pmv[1], pmv[0])) {
				if (!(MotionFlags & PMV_HALFPEL16)) {
					pmv[1].x = EVEN(pmv[1].x);
					pmv[1].y = EVEN(pmv[1].y);
				}

				CHECK_MV16_CANDIDATE(pmv[1].x, pmv[1].y);
			}
// top neighbour, if allowed
	if (!MVzero(pmv[2]))
		if (!MVequal(pmv[2], prevMB->mvs[0]))
			if (!MVequal(pmv[2], pmv[0]))
				if (!MVequal(pmv[2], pmv[1])) {
					if (!(MotionFlags & PMV_HALFPEL16)) {
						pmv[2].x = EVEN(pmv[2].x);
						pmv[2].y = EVEN(pmv[2].y);
					}
					CHECK_MV16_CANDIDATE(pmv[2].x, pmv[2].y);

// top right neighbour, if allowed
					if (!MVzero(pmv[3]))
						if (!MVequal(pmv[3], prevMB->mvs[0]))
							if (!MVequal(pmv[3], pmv[0]))
								if (!MVequal(pmv[3], pmv[1]))
									if (!MVequal(pmv[3], pmv[2])) {
										if (!(MotionFlags & PMV_HALFPEL16)) {
											pmv[3].x = EVEN(pmv[3].x);
											pmv[3].y = EVEN(pmv[3].y);
										}
										CHECK_MV16_CANDIDATE(pmv[3].x,
															 pmv[3].y);
									}
				}

	if ((MVzero(*currMV)) &&
		(!MVzero(pmv[0])) /* && (iMinSAD <= iQuant * 96) */ )
		iMinSAD -= MV16_00_BIAS;


/* Step 6: If MinSAD <= thresa goto Step 10. 
   If Motion Vector equal to Previous frame motion vector and MinSAD<PrevFrmSAD goto Step 10. 
*/

	if ((iMinSAD <= threshA) ||
		(MVequal(*currMV, prevMB->mvs[0]) &&
		 ((int32_t) iMinSAD < prevMB->sad16))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto PMVfast16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast16_Terminate_with_Refine;
	}


/************ (Diamond Search)  **************/
/* 
   Step 7: Perform Diamond search, with either the small or large diamond. 
   If Found=2 only examine one Diamond pattern, and afterwards goto step 10 
   Step 8: If small diamond, iterate small diamond search pattern until motion vector lies in the center of the diamond. 
   If center then goto step 10. 
   Step 9: If large diamond, iterate large diamond search pattern until motion vector lies in the center. 
   Refine by using small diamond and goto step 10. 
*/

	if (MotionFlags & PMV_USESQUARES16)
		MainSearchPtr = Square16_MainSearch;
	else if (MotionFlags & PMV_ADVANCEDDIAMOND16)
		MainSearchPtr = AdvDiamond16_MainSearch;
	else
		MainSearchPtr = Diamond16_MainSearch;

	backupMV = *currMV;			/* save best prediction, actually only for EXTSEARCH */


/* default: use best prediction as starting point for one call of PMVfast_MainSearch */
	iSAD =
		(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 
						  currMV->x, currMV->y, iMinSAD, &newMV, center_x, center_y, 
						  min_dx, max_dx,
						  min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode,
						  iQuant, iFound);

	if (iSAD < iMinSAD) {
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH16) {
/* extended: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0], backupMV))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y,
								  center_x, center_y, iMinSAD, &newMV, center_x, center_y,
								  min_dx, max_dx, min_dy, max_dy, iEdgedWidth,
								  iDiamondSize, iFcode, iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}

		if ((!(MVzero(pmv[0]))) && (!(MVzero(backupMV)))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 0, 0,
								  iMinSAD, &newMV, center_x, center_y, 
								  min_dx, max_dx, min_dy, max_dy, 
								  iEdgedWidth, iDiamondSize, iFcode,
								  iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/* 
   Step 10:  The motion vector is chosen according to the block corresponding to MinSAD.
*/

  PMVfast16_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE16)	// perform final half-pel step 
		iMinSAD =
			Halfpel16_Refine(pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV,
							 iMinSAD, center_x, center_y, min_dx, max_dx, min_dy, max_dy,
							 iFcode, iQuant, iEdgedWidth);

  PMVfast16_Terminate_without_Refine:
	currPMV->x = currMV->x - center_x;
	currPMV->y = currMV->y - center_y;
	return iMinSAD;
}






int32_t
Diamond8_MainSearch(const uint8_t * const pRef,
					const uint8_t * const pRefH,
					const uint8_t * const pRefV,
					const uint8_t * const pRefHV,
					const uint8_t * const cur,
					const int x,
					const int y,
					int32_t start_x,
					int32_t start_y,
					int32_t iMinSAD,
					VECTOR * const currMV,
				   const int center_x,
				   const int center_y,
					const int32_t min_dx,
					const int32_t max_dx,
					const int32_t min_dy,
					const int32_t max_dy,
					const int32_t iEdgedWidth,
					const int32_t iDiamondSize,
					const int32_t iFcode,
					const int32_t iQuant,
					int iFound)
{
/* Do a diamond search around given starting point, return SAD of best */

	int32_t iDirection = 0;
	int32_t iSAD;
	VECTOR backupMV;

	backupMV.x = start_x;
	backupMV.y = start_y;

/* It's one search with full Diamond pattern, and only 3 of 4 for all following diamonds */

	CHECK_MV8_CANDIDATE_DIR(backupMV.x - iDiamondSize, backupMV.y, 1);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x + iDiamondSize, backupMV.y, 2);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x, backupMV.y - iDiamondSize, 3);
	CHECK_MV8_CANDIDATE_DIR(backupMV.x, backupMV.y + iDiamondSize, 4);

	if (iDirection)
		while (!iFound) {
			iFound = 1;
			backupMV = *currMV;	// since iDirection!=0, this is well defined!

			if (iDirection != 2)
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x - iDiamondSize,
										  backupMV.y, 1);
			if (iDirection != 1)
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x + iDiamondSize,
										  backupMV.y, 2);
			if (iDirection != 4)
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x,
										  backupMV.y - iDiamondSize, 3);
			if (iDirection != 3)
				CHECK_MV8_CANDIDATE_FOUND(backupMV.x,
										  backupMV.y + iDiamondSize, 4);
	} else {
		currMV->x = start_x;
		currMV->y = start_y;
	}
	return iMinSAD;
}

int32_t
Halfpel8_Refine_c(const uint8_t * const pRef,
				const uint8_t * const pRefH,
				const uint8_t * const pRefV,
				const uint8_t * const pRefHV,
				const uint8_t * const cur,
				const int x,
				const int y,
				VECTOR * const currMV,
				int32_t iMinSAD,
			   const int center_x,
			   const int center_y,
				const int32_t min_dx,
				const int32_t max_dx,
				const int32_t min_dy,
				const int32_t max_dy,
				const int32_t iFcode,
				const int32_t iQuant,
				const int32_t iEdgedWidth)
{
/* Do a half-pel refinement (or rather a "smallest possible amount" refinement) */

	int32_t iSAD;
	VECTOR backupMV = *currMV;

	CHECK_MV8_CANDIDATE(backupMV.x - 1, backupMV.y - 1);
	CHECK_MV8_CANDIDATE(backupMV.x, backupMV.y - 1);
	CHECK_MV8_CANDIDATE(backupMV.x + 1, backupMV.y - 1);
	CHECK_MV8_CANDIDATE(backupMV.x - 1, backupMV.y);
	CHECK_MV8_CANDIDATE(backupMV.x + 1, backupMV.y);
	CHECK_MV8_CANDIDATE(backupMV.x - 1, backupMV.y + 1);
	CHECK_MV8_CANDIDATE(backupMV.x, backupMV.y + 1);
	CHECK_MV8_CANDIDATE(backupMV.x + 1, backupMV.y + 1);

	return iMinSAD;
}


#define PMV_HALFPEL8 (PMV_HALFPELDIAMOND8|PMV_HALFPELREFINE8)

int32_t
PMVfastSearch8(const uint8_t * const pRef,
			   const uint8_t * const pRefH,
			   const uint8_t * const pRefV,
			   const uint8_t * const pRefHV,
			   const IMAGE * const pCur,
			   const int x,
			   const int y,
			   const int start_x,
			   const int start_y,
				const int center_x,
				const int center_y,
			   const uint32_t MotionFlags,
			   const uint32_t iQuant,
			   const uint32_t iFcode,
			   const MBParam * const pParam,
			   const MACROBLOCK * const pMBs,
			   const MACROBLOCK * const prevMBs,
			   VECTOR * const currMV,
			   VECTOR * const currPMV)
{
	const uint32_t iWcount = pParam->mb_width;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width;

	const uint8_t *cur = pCur->y + x * 8 + y * 8 * iEdgedWidth;

	int32_t iDiamondSize;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;

	VECTOR pmv[4];
	int32_t psad[4];
	VECTOR newMV;
	VECTOR backupMV;
	VECTOR startMV;

//  const MACROBLOCK * const pMB = pMBs + (x>>1) + (y>>1) * iWcount;
	const MACROBLOCK *const prevMB = prevMBs + (x >> 1) + (y >> 1) * iWcount;

	 int32_t threshA, threshB;
	int32_t iFound, bPredEq;
	int32_t iMinSAD, iSAD;

	int32_t iSubBlock = (y & 1) + (y & 1) + (x & 1);

	MainSearch8FuncPtr MainSearchPtr;

	/* Init variables */
	startMV.x = start_x;
	startMV.y = start_y;

	/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy, x, y, 8, iWidth, iHeight,
			  iFcode);

	if (!(MotionFlags & PMV_HALFPELDIAMOND8)) {
		min_dx = EVEN(min_dx);
		max_dx = EVEN(max_dx);
		min_dy = EVEN(min_dy);
		max_dy = EVEN(max_dy);
	}

	/* because we might use IF (dx>max_dx) THEN dx=max_dx; */
	//bPredEq = get_pmvdata(pMBs, (x >> 1), (y >> 1), iWcount, iSubBlock, pmv, psad);
	bPredEq = get_pmvdata2(pMBs, iWcount, 0, (x >> 1), (y >> 1), iSubBlock, pmv, psad);

	if ((x == 0) && (y == 0)) {
		threshA = 512 / 4;
		threshB = 1024 / 4;

	} else {
		threshA = psad[0] / 4;	/* good estimate? */
		threshB = threshA + 256 / 4;
		if (threshA < 512 / 4)
			threshA = 512 / 4;
		if (threshA > 1024 / 4)
			threshA = 1024 / 4;
		if (threshB > 1792 / 4)
			threshB = 1792 / 4;
	}

	iFound = 0;

/* Step 4: Calculate SAD around the Median prediction. 
   MinSAD=SAD 
   If Motion Vector equal to Previous frame motion vector 
   and MinSAD<PrevFrmSAD goto Step 10. 
   If SAD<=256 goto Step 10. 
*/


// Prepare for main loop 

//  if (MotionFlags & PMV_USESQUARES8)
//      MainSearchPtr = Square8_MainSearch;
//  else

	if (MotionFlags & PMV_ADVANCEDDIAMOND8)
		MainSearchPtr = AdvDiamond8_MainSearch;
	else
		MainSearchPtr = Diamond8_MainSearch;


	*currMV = startMV;

	iMinSAD =
		sad8(cur,
			 get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 8, currMV,
						iEdgedWidth), iEdgedWidth);
	iMinSAD +=
		calc_delta_8(currMV->x - center_x, currMV->y - center_y,
					 (uint8_t) iFcode, iQuant);

	if ((iMinSAD < 256 / 4) || ((MVequal(*currMV, prevMB->mvs[iSubBlock]))
								&& ((int32_t) iMinSAD <
									prevMB->sad8[iSubBlock]))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto PMVfast8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast8_Terminate_with_Refine;
	}

/* Step 2 (lazy eval): Calculate Distance= |MedianMVX| + |MedianMVY| where MedianMV is the motion 
   vector of the median. 
   If PredEq=1 and MVpredicted = Previous Frame MV, set Found=2  
*/

	if ((bPredEq) && (MVequal(pmv[0], prevMB->mvs[iSubBlock])))
		iFound = 2;

/* Step 3 (lazy eval): If Distance>0 or thresb<1536 or PredEq=1 Select small Diamond Search. 
   Otherwise select large Diamond Search. 
*/

	if ((!MVzero(pmv[0])) || (threshB < 1536 / 4) || (bPredEq))
		iDiamondSize = 1;		// 1 halfpel!
	else
		iDiamondSize = 2;		// 2 halfpel = 1 full pixel!

	if (!(MotionFlags & PMV_HALFPELDIAMOND8))
		iDiamondSize *= 2;


/* 
   Step 5: Calculate SAD for motion vectors taken from left block, top, top-right, and Previous frame block. 
   Also calculate (0,0) but do not subtract offset. 
   Let MinSAD be the smallest SAD up to this point. 
   If MV is (0,0) subtract offset. 
*/

// the median prediction might be even better than mv16

	if (!MVequal(pmv[0], startMV))
		CHECK_MV8_CANDIDATE(center_x, center_y);

// (0,0) if needed
	if (!MVzero(pmv[0]))
		if (!MVzero(startMV))
			CHECK_MV8_ZERO;

// previous frame MV if needed
	if (!MVzero(prevMB->mvs[iSubBlock]))
		if (!MVequal(prevMB->mvs[iSubBlock], startMV))
			if (!MVequal(prevMB->mvs[iSubBlock], pmv[0]))
				CHECK_MV8_CANDIDATE(prevMB->mvs[iSubBlock].x,
									prevMB->mvs[iSubBlock].y);

	if ((iMinSAD <= threshA) ||
		(MVequal(*currMV, prevMB->mvs[iSubBlock]) &&
		 ((int32_t) iMinSAD < prevMB->sad8[iSubBlock]))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto PMVfast8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast8_Terminate_with_Refine;
	}

// left neighbour, if allowed and needed
	if (!MVzero(pmv[1]))
		if (!MVequal(pmv[1], startMV))
			if (!MVequal(pmv[1], prevMB->mvs[iSubBlock]))
				if (!MVequal(pmv[1], pmv[0])) {
					if (!(MotionFlags & PMV_HALFPEL8)) {
						pmv[1].x = EVEN(pmv[1].x);
						pmv[1].y = EVEN(pmv[1].y);
					}
					CHECK_MV8_CANDIDATE(pmv[1].x, pmv[1].y);
				}
// top neighbour, if allowed and needed
	if (!MVzero(pmv[2]))
		if (!MVequal(pmv[2], startMV))
			if (!MVequal(pmv[2], prevMB->mvs[iSubBlock]))
				if (!MVequal(pmv[2], pmv[0]))
					if (!MVequal(pmv[2], pmv[1])) {
						if (!(MotionFlags & PMV_HALFPEL8)) {
							pmv[2].x = EVEN(pmv[2].x);
							pmv[2].y = EVEN(pmv[2].y);
						}
						CHECK_MV8_CANDIDATE(pmv[2].x, pmv[2].y);

// top right neighbour, if allowed and needed
						if (!MVzero(pmv[3]))
							if (!MVequal(pmv[3], startMV))
								if (!MVequal(pmv[3], prevMB->mvs[iSubBlock]))
									if (!MVequal(pmv[3], pmv[0]))
										if (!MVequal(pmv[3], pmv[1]))
											if (!MVequal(pmv[3], pmv[2])) {
												if (!
													(MotionFlags &
													 PMV_HALFPEL8)) {
													pmv[3].x = EVEN(pmv[3].x);
													pmv[3].y = EVEN(pmv[3].y);
												}
												CHECK_MV8_CANDIDATE(pmv[3].x,
																	pmv[3].y);
											}
					}

	if ((MVzero(*currMV)) &&
		(!MVzero(pmv[0])) /* && (iMinSAD <= iQuant * 96) */ )
		iMinSAD -= MV8_00_BIAS;


/* Step 6: If MinSAD <= thresa goto Step 10. 
   If Motion Vector equal to Previous frame motion vector and MinSAD<PrevFrmSAD goto Step 10. 
*/

	if ((iMinSAD <= threshA) ||
		(MVequal(*currMV, prevMB->mvs[iSubBlock]) &&
		 ((int32_t) iMinSAD < prevMB->sad8[iSubBlock]))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto PMVfast8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfast8_Terminate_with_Refine;
	}

/************ (Diamond Search)  **************/
/* 
   Step 7: Perform Diamond search, with either the small or large diamond. 
   If Found=2 only examine one Diamond pattern, and afterwards goto step 10 
   Step 8: If small diamond, iterate small diamond search pattern until motion vector lies in the center of the diamond. 
   If center then goto step 10. 
   Step 9: If large diamond, iterate large diamond search pattern until motion vector lies in the center. 
   Refine by using small diamond and goto step 10. 
*/

	backupMV = *currMV;			/* save best prediction, actually only for EXTSEARCH */

/* default: use best prediction as starting point for one call of PMVfast_MainSearch */
	iSAD =
		(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV->x,
						  currMV->y, iMinSAD, &newMV, center_x, center_y, min_dx, max_dx,
						  min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode,
						  iQuant, iFound);

	if (iSAD < iMinSAD) {
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH8) {
/* extended: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0], backupMV))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y,
								  pmv[0].x, pmv[0].y, iMinSAD, &newMV, center_x, center_y,
								  min_dx, max_dx, min_dy, max_dy, iEdgedWidth,
								  iDiamondSize, iFcode, iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}

		if ((!(MVzero(pmv[0]))) && (!(MVzero(backupMV)))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 0, 0,
								  iMinSAD, &newMV, center_x, center_y, min_dx, max_dx, min_dy,
								  max_dy, iEdgedWidth, iDiamondSize, iFcode,
								  iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/* Step 10: The motion vector is chosen according to the block corresponding to MinSAD.
   By performing an optional local half-pixel search, we can refine this result even further.
*/

  PMVfast8_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE8)	// perform final half-pel step 
		iMinSAD =
			Halfpel8_Refine(pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV,
							iMinSAD, center_x, center_y, min_dx, max_dx, min_dy, max_dy,
							iFcode, iQuant, iEdgedWidth);


  PMVfast8_Terminate_without_Refine:
	currPMV->x = currMV->x - center_x;
	currPMV->y = currMV->y - center_y;

	return iMinSAD;
}

int32_t
EPZSSearch16(const uint8_t * const pRef,
			 const uint8_t * const pRefH,
			 const uint8_t * const pRefV,
			 const uint8_t * const pRefHV,
			 const IMAGE * const pCur,
			 const int x,
			 const int y,
			const int start_x,
			const int start_y,
			const int center_x,
			const int center_y,
			 const uint32_t MotionFlags,
			 const uint32_t iQuant,
			 const uint32_t iFcode,
			 const MBParam * const pParam,
			 const MACROBLOCK * const pMBs,
			 const MACROBLOCK * const prevMBs,
			 VECTOR * const currMV,
			 VECTOR * const currPMV)
{
	const uint32_t iWcount = pParam->mb_width;
	const uint32_t iHcount = pParam->mb_height;

	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width;

	const uint8_t *cur = pCur->y + x * 16 + y * 16 * iEdgedWidth;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;

	VECTOR newMV;
	VECTOR backupMV;

	VECTOR pmv[4];
	int32_t psad[8];

	static MACROBLOCK *oldMBs = NULL;

//  const MACROBLOCK * const pMB = pMBs + x + y * iWcount;
	const MACROBLOCK *const prevMB = prevMBs + x + y * iWcount;
	MACROBLOCK *oldMB = NULL;

	 int32_t thresh2;
	int32_t bPredEq;
	int32_t iMinSAD, iSAD = 9999;

	MainSearch16FuncPtr MainSearchPtr;

	if (oldMBs == NULL) {
		oldMBs = (MACROBLOCK *) calloc(iWcount * iHcount, sizeof(MACROBLOCK));
//      fprintf(stderr,"allocated %d bytes for oldMBs\n",iWcount*iHcount*sizeof(MACROBLOCK));
	}
	oldMB = oldMBs + x + y * iWcount;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy, x, y, 16, iWidth, iHeight,
			  iFcode);

	if (!(MotionFlags & PMV_HALFPEL16)) {
		min_dx = EVEN(min_dx);
		max_dx = EVEN(max_dx);
		min_dy = EVEN(min_dy);
		max_dy = EVEN(max_dy);
	}
	/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
	//bPredEq = get_pmvdata(pMBs, x, y, iWcount, 0, pmv, psad);
	bPredEq = get_pmvdata2(pMBs, iWcount, 0, x, y, 0, pmv, psad);

/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/

// Prepare for main loop 

	currMV->x = start_x;		
	currMV->y = start_y;		

	if (!(MotionFlags & PMV_HALFPEL16)) {
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}

	if (currMV->x > max_dx)
		currMV->x = max_dx;
	if (currMV->x < min_dx)
		currMV->x = min_dx;
	if (currMV->y > max_dy)
		currMV->y = max_dy;
	if (currMV->y < min_dy)
		currMV->y = min_dy;

/***************** This is predictor SET A: only median prediction ******************/

	iMinSAD =
		sad16(cur,
			  get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 16, currMV,
						 iEdgedWidth), iEdgedWidth, MV_MAX_ERROR);
	iMinSAD +=
		calc_delta_16(currMV->x - center_x, currMV->y - center_y,
					  (uint8_t) iFcode, iQuant);

// thresh1 is fixed to 256 
	if ((iMinSAD < 256) ||
		((MVequal(*currMV, prevMB->mvs[0])) &&
		 ((int32_t) iMinSAD < prevMB->sad16))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto EPZS16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto EPZS16_Terminate_with_Refine;
	}

/************** This is predictor SET B: (0,0), prev.frame MV, neighbours **************/

// previous frame MV 
	CHECK_MV16_CANDIDATE(prevMB->mvs[0].x, prevMB->mvs[0].y);

// set threshhold based on Min of Prediction and SAD of collocated block
// CHECK_MV16 always uses iSAD for the SAD of last vector to check, so now iSAD is what we want

	if ((x == 0) && (y == 0)) {
		thresh2 = 512;
	} else {
/* T_k = 1.2 * MIN(SAD_top,SAD_left,SAD_topleft,SAD_coll) +128;   [Tourapis, 2002] */

		thresh2 = MIN(psad[0], iSAD) * 6 / 5 + 128;
	}

// MV=(0,0) is often a good choice

	CHECK_MV16_ZERO;


// left neighbour, if allowed
	if (x != 0) {
		if (!(MotionFlags & PMV_HALFPEL16)) {
			pmv[1].x = EVEN(pmv[1].x);
			pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV16_CANDIDATE(pmv[1].x, pmv[1].y);
	}
// top neighbour, if allowed
	if (y != 0) {
		if (!(MotionFlags & PMV_HALFPEL16)) {
			pmv[2].x = EVEN(pmv[2].x);
			pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV16_CANDIDATE(pmv[2].x, pmv[2].y);

// top right neighbour, if allowed
		if ((uint32_t) x != (iWcount - 1)) {
			if (!(MotionFlags & PMV_HALFPEL16)) {
				pmv[3].x = EVEN(pmv[3].x);
				pmv[3].y = EVEN(pmv[3].y);
			}
			CHECK_MV16_CANDIDATE(pmv[3].x, pmv[3].y);
		}
	}

/* Terminate if MinSAD <= T_2 
   Terminate if MV[t] == MV[t-1] and MinSAD[t] <= MinSAD[t-1] 
*/

	if ((iMinSAD <= thresh2)
		|| (MVequal(*currMV, prevMB->mvs[0]) &&
			((int32_t) iMinSAD <= prevMB->sad16))) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto EPZS16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto EPZS16_Terminate_with_Refine;
	}

/***** predictor SET C: acceleration MV (new!), neighbours in prev. frame(new!) ****/

	backupMV = prevMB->mvs[0];	// collocated MV
	backupMV.x += (prevMB->mvs[0].x - oldMB->mvs[0].x);	// acceleration X
	backupMV.y += (prevMB->mvs[0].y - oldMB->mvs[0].y);	// acceleration Y 

	CHECK_MV16_CANDIDATE(backupMV.x, backupMV.y);

// left neighbour
	if (x != 0)
		CHECK_MV16_CANDIDATE((prevMB - 1)->mvs[0].x, (prevMB - 1)->mvs[0].y);

// top neighbour 
	if (y != 0)
		CHECK_MV16_CANDIDATE((prevMB - iWcount)->mvs[0].x,
							 (prevMB - iWcount)->mvs[0].y);

// right neighbour, if allowed (this value is not written yet, so take it from   pMB->mvs 

	if ((uint32_t) x != iWcount - 1)
		CHECK_MV16_CANDIDATE((prevMB + 1)->mvs[0].x, (prevMB + 1)->mvs[0].y);

// bottom neighbour, dito
	if ((uint32_t) y != iHcount - 1)
		CHECK_MV16_CANDIDATE((prevMB + iWcount)->mvs[0].x,
							 (prevMB + iWcount)->mvs[0].y);

/* Terminate if MinSAD <= T_3 (here T_3 = T_2)  */
	if (iMinSAD <= thresh2) {
		if (MotionFlags & PMV_QUICKSTOP16)
			goto EPZS16_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP16)
			goto EPZS16_Terminate_with_Refine;
	}

/************ (if Diamond Search)  **************/

	backupMV = *currMV;			/* save best prediction, actually only for EXTSEARCH */

	if (MotionFlags & PMV_USESQUARES16)
		MainSearchPtr = Square16_MainSearch;
	else
	 if (MotionFlags & PMV_ADVANCEDDIAMOND16)
		MainSearchPtr = AdvDiamond16_MainSearch;
	else
		MainSearchPtr = Diamond16_MainSearch;

/* default: use best prediction as starting point for one call of PMVfast_MainSearch */

	iSAD =
		(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV->x,
						  currMV->y, iMinSAD, &newMV, center_x, center_y, min_dx, max_dx,
						  min_dy, max_dy, iEdgedWidth, 2, iFcode, iQuant, 0);

	if (iSAD < iMinSAD) {
		*currMV = newMV;
		iMinSAD = iSAD;
	}


	if (MotionFlags & PMV_EXTSEARCH16) {
/* extended mode: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0], backupMV))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y,
								  pmv[0].x, pmv[0].y, iMinSAD, &newMV, center_x, center_y,
								  min_dx, max_dx, min_dy, max_dy, iEdgedWidth,
								  2, iFcode, iQuant, 0);
		}

		if (iSAD < iMinSAD) {
			*currMV = newMV;
			iMinSAD = iSAD;
		}

		if ((!(MVzero(pmv[0]))) && (!(MVzero(backupMV)))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 0, 0,
								  iMinSAD, &newMV, center_x, center_y, min_dx, max_dx, min_dy,
								  max_dy, iEdgedWidth, 2, iFcode, iQuant, 0);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/*************** 	Choose best MV found     **************/

  EPZS16_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE16)	// perform final half-pel step 
		iMinSAD =
			Halfpel16_Refine(pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV,
							 iMinSAD, center_x, center_y, min_dx, max_dx, min_dy, max_dy,
							 iFcode, iQuant, iEdgedWidth);

  EPZS16_Terminate_without_Refine:

	*oldMB = *prevMB;

	currPMV->x = currMV->x - center_x;
	currPMV->y = currMV->y - center_y;
	return iMinSAD;
}


int32_t
EPZSSearch8(const uint8_t * const pRef,
			const uint8_t * const pRefH,
			const uint8_t * const pRefV,
			const uint8_t * const pRefHV,
			const IMAGE * const pCur,
			const int x,
			const int y,
			const int start_x,
			const int start_y,
			const int center_x,
			const int center_y,
			const uint32_t MotionFlags,
			const uint32_t iQuant,
			const uint32_t iFcode,
			const MBParam * const pParam,
			const MACROBLOCK * const pMBs,
			const MACROBLOCK * const prevMBs,
			VECTOR * const currMV,
			VECTOR * const currPMV)
{
/* Please not that EPZS might not be a good choice for 8x8-block motion search ! */

	const uint32_t iWcount = pParam->mb_width;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width;

	const uint8_t *cur = pCur->y + x * 8 + y * 8 * iEdgedWidth;

	int32_t iDiamondSize = 1;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;

	VECTOR newMV;
	VECTOR backupMV;

	VECTOR pmv[4];
	int32_t psad[8];

	const int32_t iSubBlock = ((y & 1) << 1) + (x & 1);

//  const MACROBLOCK * const pMB = pMBs + (x>>1) + (y>>1) * iWcount;
	const MACROBLOCK *const prevMB = prevMBs + (x >> 1) + (y >> 1) * iWcount;

	int32_t bPredEq;
	int32_t iMinSAD, iSAD = 9999;

	MainSearch8FuncPtr MainSearchPtr;

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy, x, y, 8, iWidth, iHeight,
			  iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if (!(MotionFlags & PMV_HALFPEL8)) {
		min_dx = EVEN(min_dx);
		max_dx = EVEN(max_dx);
		min_dy = EVEN(min_dy);
		max_dy = EVEN(max_dy);
	}
	/* because we might use something like IF (dx>max_dx) THEN dx=max_dx; */
	//bPredEq = get_pmvdata(pMBs, x >> 1, y >> 1, iWcount, iSubBlock, pmv[0].x, pmv[0].y, psad);
	bPredEq = get_pmvdata2(pMBs, iWcount, 0, x >> 1, y >> 1, iSubBlock, pmv, psad);


/* Step 4: Calculate SAD around the Median prediction. 
        MinSAD=SAD 
        If Motion Vector equal to Previous frame motion vector 
		and MinSAD<PrevFrmSAD goto Step 10. 
        If SAD<=256 goto Step 10. 
*/

// Prepare for main loop 


	if (!(MotionFlags & PMV_HALFPEL8)) {
		currMV->x = EVEN(currMV->x);
		currMV->y = EVEN(currMV->y);
	}

	if (currMV->x > max_dx)
		currMV->x = max_dx;
	if (currMV->x < min_dx)
		currMV->x = min_dx;
	if (currMV->y > max_dy)
		currMV->y = max_dy;
	if (currMV->y < min_dy)
		currMV->y = min_dy;

/***************** This is predictor SET A: only median prediction ******************/


	iMinSAD =
		sad8(cur,
			 get_ref_mv(pRef, pRefH, pRefV, pRefHV, x, y, 8, currMV,
						iEdgedWidth), iEdgedWidth);
	iMinSAD +=
		calc_delta_8(currMV->x - center_x, currMV->y - center_y,
					 (uint8_t) iFcode, iQuant);


// thresh1 is fixed to 256 
	if (iMinSAD < 256 / 4) {
		if (MotionFlags & PMV_QUICKSTOP8)
			goto EPZS8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP8)
			goto EPZS8_Terminate_with_Refine;
	}

/************** This is predictor SET B: (0,0), prev.frame MV, neighbours **************/


// MV=(0,0) is often a good choice
	CHECK_MV8_ZERO;

// previous frame MV 
	CHECK_MV8_CANDIDATE(prevMB->mvs[iSubBlock].x, prevMB->mvs[iSubBlock].y);

// left neighbour, if allowed
	if (psad[1] != MV_MAX_ERROR) {
		if (!(MotionFlags & PMV_HALFPEL8)) {
			pmv[1].x = EVEN(pmv[1].x);
			pmv[1].y = EVEN(pmv[1].y);
		}
		CHECK_MV8_CANDIDATE(pmv[1].x, pmv[1].y);
	}
// top neighbour, if allowed
	if (psad[2] != MV_MAX_ERROR) {
		if (!(MotionFlags & PMV_HALFPEL8)) {
			pmv[2].x = EVEN(pmv[2].x);
			pmv[2].y = EVEN(pmv[2].y);
		}
		CHECK_MV8_CANDIDATE(pmv[2].x, pmv[2].y);

// top right neighbour, if allowed
		if (psad[3] != MV_MAX_ERROR) {
			if (!(MotionFlags & PMV_HALFPEL8)) {
				pmv[3].x = EVEN(pmv[3].x);
				pmv[3].y = EVEN(pmv[3].y);
			}
			CHECK_MV8_CANDIDATE(pmv[3].x, pmv[3].y);
		}
	}

/*  // this bias is zero anyway, at the moment! 

    	if ( (MVzero(*currMV)) && (!MVzero(pmv[0])) ) // && (iMinSAD <= iQuant * 96) 
		iMinSAD -= MV8_00_BIAS;		

*/

/* Terminate if MinSAD <= T_2 
   Terminate if MV[t] == MV[t-1] and MinSAD[t] <= MinSAD[t-1] 
*/

	if (iMinSAD < 512 / 4) {	/* T_2 == 512/4 hardcoded */
		if (MotionFlags & PMV_QUICKSTOP8)
			goto EPZS8_Terminate_without_Refine;
		if (MotionFlags & PMV_EARLYSTOP8)
			goto EPZS8_Terminate_with_Refine;
	}

/************ (Diamond Search)  **************/

	backupMV = *currMV;			/* save best prediction, actually only for EXTSEARCH */

	if (!(MotionFlags & PMV_HALFPELDIAMOND8))
		iDiamondSize *= 2;

/* default: use best prediction as starting point for one call of EPZS_MainSearch */

// there is no EPZS^2 for inter4v at the moment 

//  if (MotionFlags & PMV_USESQUARES8)
//      MainSearchPtr = Square8_MainSearch;
//  else

	if (MotionFlags & PMV_ADVANCEDDIAMOND8)
		MainSearchPtr = AdvDiamond8_MainSearch;
	else
		MainSearchPtr = Diamond8_MainSearch;

	iSAD =
		(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV->x,
						  currMV->y, iMinSAD, &newMV, center_x, center_y, min_dx, max_dx,
						  min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode,
						  iQuant, 0);


	if (iSAD < iMinSAD) {
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH8) {
/* extended mode: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0], backupMV))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y,
								  pmv[0].x, pmv[0].y, iMinSAD, &newMV, center_x, center_y,
								  min_dx, max_dx, min_dy, max_dy, iEdgedWidth,
								  iDiamondSize, iFcode, iQuant, 0);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}

		if ((!(MVzero(pmv[0]))) && (!(MVzero(backupMV)))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 0, 0,
								  iMinSAD, &newMV, center_x, center_y, min_dx, max_dx, min_dy,
								  max_dy, iEdgedWidth, iDiamondSize, iFcode,
								  iQuant, 0);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/*************** 	Choose best MV found     **************/

  EPZS8_Terminate_with_Refine:
	if (MotionFlags & PMV_HALFPELREFINE8)	// perform final half-pel step 
		iMinSAD =
			Halfpel8_Refine(pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV,
							iMinSAD, center_x, center_y, min_dx, max_dx, min_dy, max_dy,
							iFcode, iQuant, iEdgedWidth);

  EPZS8_Terminate_without_Refine:

	currPMV->x = currMV->x - center_x;
	currPMV->y = currMV->y - center_y;
	return iMinSAD;
}



int32_t
PMVfastIntSearch16(const uint8_t * const pRef,
				const uint8_t * const pRefH,
				const uint8_t * const pRefV,
				const uint8_t * const pRefHV,
				const IMAGE * const pCur,
				const int x,
				const int y,
			const int start_x,
			const int start_y,
			const int center_x,
			const int center_y,
				const uint32_t MotionFlags,
				const uint32_t iQuant,
				const uint32_t iFcode,
				const MBParam * const pParam,
				const MACROBLOCK * const pMBs,
				const MACROBLOCK * const prevMBs,
				VECTOR * const currMV,
				VECTOR * const currPMV)
{
	const uint32_t iWcount = pParam->mb_width;
	const int32_t iWidth = pParam->width;
	const int32_t iHeight = pParam->height;
	const int32_t iEdgedWidth = pParam->edged_width;

	const uint8_t *cur = pCur->y + x * 16 + y * 16 * iEdgedWidth;
	const VECTOR zeroMV = { 0, 0 };

	int32_t iDiamondSize;

	int32_t min_dx;
	int32_t max_dx;
	int32_t min_dy;
	int32_t max_dy;

	int32_t iFound;

	VECTOR newMV;
	VECTOR backupMV;			/* just for PMVFAST */

	VECTOR pmv[4];
	int32_t psad[4];

	MainSearch16FuncPtr MainSearchPtr;

	const MACROBLOCK *const prevMB = prevMBs + x + y * iWcount;
	MACROBLOCK *const pMB = pMBs + x + y * iWcount;

	int32_t threshA, threshB;
	int32_t bPredEq;
	int32_t iMinSAD, iSAD;
	

/* Get maximum range */
	get_range(&min_dx, &max_dx, &min_dy, &max_dy, x, y, 16, iWidth, iHeight,
			  iFcode);

/* we work with abs. MVs, not relative to prediction, so get_range is called relative to 0,0 */

	if ((x == 0) && (y == 0)) {
		threshA = 512;
		threshB = 1024;

		bPredEq = 0;
		psad[0] = psad[1] = psad[2] = psad[3] = 0;
		*currMV = pmv[0] = pmv[1] = pmv[2] = pmv[3] = zeroMV;

	} else {
		threshA = psad[0];
		threshB = threshA + 256;
		if (threshA < 512)
			threshA = 512;
		if (threshA > 1024)
			threshA = 1024;
		if (threshB > 1792)
			threshB = 1792;

		bPredEq = get_ipmvdata(pMBs, iWcount, 0, x, y, 0, pmv, psad);
		*currMV = pmv[0];			/* current best := prediction */
	}

	iFound = 0;

/* Step 4: Calculate SAD around the Median prediction. 
   MinSAD=SAD 
   If Motion Vector equal to Previous frame motion vector 
   and MinSAD<PrevFrmSAD goto Step 10. 
   If SAD<=256 goto Step 10. 
*/

	if (currMV->x > max_dx) {
		currMV->x = EVEN(max_dx);
	}
	if (currMV->x < min_dx) {
		currMV->x = EVEN(min_dx);
	}
	if (currMV->y > max_dy) {
		currMV->y = EVEN(max_dy);
	}
	if (currMV->y < min_dy) {
		currMV->y = EVEN(min_dy);
	}

	iMinSAD =
		sad16(cur,
			  get_iref_mv(pRef, x, y, 16, currMV,
						 iEdgedWidth), iEdgedWidth, MV_MAX_ERROR);
	iMinSAD +=
		calc_delta_16(currMV->x - center_x, currMV->y - center_y,
					  (uint8_t) iFcode, iQuant);

	if ((iMinSAD < 256) ||
		((MVequal(*currMV, prevMB->i_mvs[0])) &&
		 ((int32_t) iMinSAD < prevMB->i_sad16))) {
		if (iMinSAD < 2 * iQuant)	// high chances for SKIP-mode
		{
			if (!MVzero(*currMV)) {
				iMinSAD += MV16_00_BIAS;
				CHECK_MV16_ZERO;	// (0,0) saves space for letterboxed pictures
				iMinSAD -= MV16_00_BIAS;
			}
		}

		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfastInt16_Terminate_with_Refine;
	}


/* Step 2 (lazy eval): Calculate Distance= |MedianMVX| + |MedianMVY| where MedianMV is the motion 
   vector of the median. 
   If PredEq=1 and MVpredicted = Previous Frame MV, set Found=2  
*/

	if ((bPredEq) && (MVequal(pmv[0], prevMB->i_mvs[0])))
		iFound = 2;

/* Step 3 (lazy eval): If Distance>0 or thresb<1536 or PredEq=1 Select small Diamond Search. 
   Otherwise select large Diamond Search. 
*/

	if ((!MVzero(pmv[0])) || (threshB < 1536) || (bPredEq))
		iDiamondSize = 2;		// halfpel units!
	else
		iDiamondSize = 4;		// halfpel units!

/* 
   Step 5: Calculate SAD for motion vectors taken from left block, top, top-right, and Previous frame block. 
   Also calculate (0,0) but do not subtract offset. 
   Let MinSAD be the smallest SAD up to this point. 
   If MV is (0,0) subtract offset. 
*/

// (0,0) is often a good choice

	if (!MVzero(pmv[0]))
		CHECK_MV16_ZERO;

// previous frame MV is always possible

	if (!MVzero(prevMB->i_mvs[0]))
		if (!MVequal(prevMB->i_mvs[0], pmv[0]))
			CHECK_MV16_CANDIDATE(prevMB->i_mvs[0].x, prevMB->i_mvs[0].y);

// left neighbour, if allowed

	if (!MVzero(pmv[1]))
		if (!MVequal(pmv[1], prevMB->i_mvs[0]))
			if (!MVequal(pmv[1], pmv[0])) 
				CHECK_MV16_CANDIDATE(pmv[1].x, pmv[1].y);

// top neighbour, if allowed
	if (!MVzero(pmv[2]))
		if (!MVequal(pmv[2], prevMB->i_mvs[0]))
			if (!MVequal(pmv[2], pmv[0]))
				if (!MVequal(pmv[2], pmv[1])) 
					CHECK_MV16_CANDIDATE(pmv[2].x, pmv[2].y);

// top right neighbour, if allowed
					if (!MVzero(pmv[3]))
						if (!MVequal(pmv[3], prevMB->i_mvs[0]))
							if (!MVequal(pmv[3], pmv[0]))
								if (!MVequal(pmv[3], pmv[1]))
									if (!MVequal(pmv[3], pmv[2])) 
										CHECK_MV16_CANDIDATE(pmv[3].x,
															 pmv[3].y);

	if ((MVzero(*currMV)) &&
		(!MVzero(pmv[0])) /* && (iMinSAD <= iQuant * 96) */ )
		iMinSAD -= MV16_00_BIAS;


/* Step 6: If MinSAD <= thresa goto Step 10. 
   If Motion Vector equal to Previous frame motion vector and MinSAD<PrevFrmSAD goto Step 10. 
*/

	if ((iMinSAD <= threshA) ||
		(MVequal(*currMV, prevMB->i_mvs[0]) &&
		 ((int32_t) iMinSAD < prevMB->i_sad16))) {

		if (MotionFlags & PMV_EARLYSTOP16)
			goto PMVfastInt16_Terminate_with_Refine;
	}


/************ (Diamond Search)  **************/
/* 
   Step 7: Perform Diamond search, with either the small or large diamond. 
   If Found=2 only examine one Diamond pattern, and afterwards goto step 10 
   Step 8: If small diamond, iterate small diamond search pattern until motion vector lies in the center of the diamond. 
   If center then goto step 10. 
   Step 9: If large diamond, iterate large diamond search pattern until motion vector lies in the center. 
   Refine by using small diamond and goto step 10. 
*/

	if (MotionFlags & PMV_USESQUARES16)
		MainSearchPtr = Square16_MainSearch;
	else if (MotionFlags & PMV_ADVANCEDDIAMOND16)
		MainSearchPtr = AdvDiamond16_MainSearch;
	else
		MainSearchPtr = Diamond16_MainSearch;

	backupMV = *currMV;			/* save best prediction, actually only for EXTSEARCH */


/* default: use best prediction as starting point for one call of PMVfast_MainSearch */
	iSAD =
		(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV->x,
						  currMV->y, iMinSAD, &newMV, center_x, center_y, min_dx, max_dx,
						  min_dy, max_dy, iEdgedWidth, iDiamondSize, iFcode,
						  iQuant, iFound);

	if (iSAD < iMinSAD) {
		*currMV = newMV;
		iMinSAD = iSAD;
	}

	if (MotionFlags & PMV_EXTSEARCH16) {
/* extended: search (up to) two more times: orignal prediction and (0,0) */

		if (!(MVequal(pmv[0], backupMV))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y,
								  pmv[0].x, pmv[0].y, iMinSAD, &newMV, center_x, center_y,
								  min_dx, max_dx, min_dy, max_dy, iEdgedWidth,
								  iDiamondSize, iFcode, iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}

		if ((!(MVzero(pmv[0]))) && (!(MVzero(backupMV)))) {
			iSAD =
				(*MainSearchPtr) (pRef, pRefH, pRefV, pRefHV, cur, x, y, 0, 0,
								  iMinSAD, &newMV, center_x, center_y, min_dx, max_dx, min_dy,
								  max_dy, iEdgedWidth, iDiamondSize, iFcode,
								  iQuant, iFound);

			if (iSAD < iMinSAD) {
				*currMV = newMV;
				iMinSAD = iSAD;
			}
		}
	}

/* 
   Step 10:  The motion vector is chosen according to the block corresponding to MinSAD.
*/

PMVfastInt16_Terminate_with_Refine:

	pMB->i_mvs[0] = pMB->i_mvs[1] = pMB->i_mvs[2] = pMB->i_mvs[3] = pMB->i_mv16 = *currMV;
	pMB->i_sad8[0] = pMB->i_sad8[1] = pMB->i_sad8[2] = pMB->i_sad8[3] = pMB->i_sad16 = iMinSAD;
	
	if (MotionFlags & PMV_HALFPELREFINE16)	// perform final half-pel step 
		iMinSAD =
			Halfpel16_Refine(pRef, pRefH, pRefV, pRefHV, cur, x, y, currMV,
							 iMinSAD, center_x, center_y, min_dx, max_dx, min_dy, max_dy,
							 iFcode, iQuant, iEdgedWidth);

  	pmv[0] = get_pmv2(pMBs, pParam->mb_width, 0, x, y, 0);  	// get _REAL_ prediction (halfpel possible)

PMVfastInt16_Terminate_without_Refine:
	currPMV->x = currMV->x - center_x;
	currPMV->y = currMV->y - center_y;
	return iMinSAD;
}



/* ***********************************************************
	bvop motion estimation 
// TODO: need to incorporate prediction here (eg. sad += calc_delta_16)
***************************************************************/


#define DIRECT_PENALTY 0	
#define DIRECT_UPPERLIMIT 256	// never use direct mode if SAD is larger than this

void
MotionEstimationBVOP(MBParam * const pParam,
					 FRAMEINFO * const frame,
					 const int32_t time_bp,
					 const int32_t time_pp,
					 // forward (past) reference 
					 const MACROBLOCK * const f_mbs,
					 const IMAGE * const f_ref,
					 const IMAGE * const f_refH,
					 const IMAGE * const f_refV,
					 const IMAGE * const f_refHV,
					 // backward (future) reference
					 const MACROBLOCK * const b_mbs,
					 const IMAGE * const b_ref,
					 const IMAGE * const b_refH,
					 const IMAGE * const b_refV,
					 const IMAGE * const b_refHV)
{
	const int mb_width = pParam->mb_width;
	const int mb_height = pParam->mb_height;
	const int edged_width = pParam->edged_width;

	int i, j, k;

	static const VECTOR zeroMV={0,0};
	
	int f_sad16;	/* forward (as usual) search */
	int b_sad16;	/* backward (only in b-frames) search */
	int i_sad16;	/* interpolated (both direction, b-frames only) */
	int d_sad16;	/* direct mode (assume linear motion) */
	int dnv_sad16;	/* direct mode (assume linear motion) without correction vector */

	int best_sad;

	VECTOR f_predMV, b_predMV;	/* there is no direct prediction */
	VECTOR pmv_dontcare;

	int f_count=0;
	int b_count=0;
	int i_count=0;
	int d_count=0;
	int dnv_count=0;
	int s_count=0;
	const int64_t TRB = (int32_t)time_pp - (int32_t)time_bp;
    const int64_t TRD = (int32_t)time_pp;

	// fprintf(stderr,"TRB = %lld  TRD = %lld  time_bp =%d time_pp =%d\n\n",TRB,TRD,time_bp,time_pp);
	// note: i==horizontal, j==vertical
	for (j = 0; j < mb_height; j++) {
	
		f_predMV = zeroMV; 	/* prediction is reset at left boundary */
		b_predMV = zeroMV; 
		
		for (i = 0; i < mb_width; i++) {
			MACROBLOCK *mb = &frame->mbs[i + j * mb_width];
			const MACROBLOCK *f_mb = &f_mbs[i + j * mb_width];
			const MACROBLOCK *b_mb = &b_mbs[i + j * mb_width];

			VECTOR directMV;
			VECTOR deltaMV=zeroMV;

/* special case, if collocated block is SKIPed: encoding is forward(0,0)  */

#ifndef _DISABLE_SKIP
			if (b_mb->mode == MODE_INTER && b_mb->cbp == 0 &&
				b_mb->mvs[0].x == 0 && b_mb->mvs[0].y == 0) {	
				mb->mode = MODE_NOT_CODED;
				mb->mvs[0].x = 0;
				mb->mvs[0].y = 0;
				mb->b_mvs[0].x = 0;
				mb->b_mvs[0].y = 0;
				continue;
			}
#endif

			dnv_sad16 = DIRECT_PENALTY;

			if (b_mb->mode == MODE_INTER4V)
			{

			/* same method of scaling as in decoder.c, so we copy from there */
	            for (k = 0; k < 4; k++) {
		
					directMV = b_mb->mvs[k];

					mb->mvs[k].x = (int32_t) ((TRB * directMV.x) / TRD + deltaMV.x);
            	    mb->b_mvs[k].x = (int32_t) ((deltaMV.x == 0) 
										? ((TRB - TRD) * directMV.x) / TRD
	                                    : mb->mvs[k].x - directMV.x);
    	            mb->mvs[k].y = (int32_t) ((TRB * directMV.y) / TRD + deltaMV.y);
        	        mb->b_mvs[k].y = (int32_t) ((deltaMV.y == 0) 
										? ((TRB - TRD) * directMV.y) / TRD
                	                    : mb->mvs[k].y - directMV.y);

					dnv_sad16 += 
						sad8bi(frame->image.y + 2*(i+(k&1))*8 + 2*(j+(k>>1))*8*edged_width,
						  get_ref_mv(f_ref->y, f_refH->y, f_refV->y, f_refHV->y,
								2*(i+(k&1)), 2*(j+(k>>1)), 8, &mb->mvs[k], edged_width), 
						  get_ref_mv(b_ref->y, b_refH->y, b_refV->y, b_refHV->y,
								2*(i+(k&1)), 2*(j+(k>>1)), 8, &mb->b_mvs[k], edged_width),
						  edged_width);
				}
			}
			else
			{
				directMV = b_mb->mvs[0];

				mb->mvs[0].x = (int32_t) ((TRB * directMV.x) / TRD + deltaMV.x);
            	mb->b_mvs[0].x = (int32_t) ((deltaMV.x == 0) 
								? ((TRB - TRD) * directMV.x) / TRD
	                            : mb->mvs[0].x - directMV.x);
    	        mb->mvs[0].y = (int32_t) ((TRB * directMV.y) / TRD + deltaMV.y);
        	    mb->b_mvs[0].y = (int32_t) ((deltaMV.y == 0) 
								? ((TRB - TRD) * directMV.y) / TRD
                                : mb->mvs[0].y - directMV.y);

				dnv_sad16 = DIRECT_PENALTY + 
					sad16bi(frame->image.y + i * 16 + j * 16 * edged_width,
						  get_ref_mv(f_ref->y, f_refH->y, f_refV->y, f_refHV->y,
								i, j, 16, &mb->mvs[0], edged_width), 
						  get_ref_mv(b_ref->y, b_refH->y, b_refV->y, b_refHV->y,
								i, j, 16, &mb->b_mvs[0], edged_width),
						  edged_width);


            }

			// forward search
			f_sad16 = SEARCH16(f_ref->y, f_refH->y, f_refV->y, f_refHV->y,
						&frame->image, i, j, 
						mb->mvs[0].x, mb->mvs[0].y,			/* start point f_directMV */
						f_predMV.x, f_predMV.y, 			/* center is f-prediction */
						frame->motion_flags & (~(PMV_EARLYSTOP16|PMV_QUICKSTOP16)),
						frame->quant, frame->fcode, pParam, 
						f_mbs, f_mbs, 
						&mb->mvs[0], &pmv_dontcare);


			// backward search
			b_sad16 = SEARCH16(b_ref->y, b_refH->y, b_refV->y, b_refHV->y, 
						&frame->image, i, j, 
						mb->b_mvs[0].x, mb->b_mvs[0].y,		/* start point b_directMV */
						b_predMV.x, b_predMV.y, 			/* center is b-prediction */
						frame->motion_flags & (~(PMV_EARLYSTOP16|PMV_QUICKSTOP16)),
						frame->quant, frame->bcode, pParam, 
						b_mbs, b_mbs,	
						&mb->b_mvs[0], &pmv_dontcare);

			i_sad16 =
				sad16bi(frame->image.y + i * 16 + j * 16 * edged_width,
						  get_ref_mv(f_ref->y, f_refH->y, f_refV->y, f_refHV->y,
								i, j, 16, &mb->mvs[0], edged_width), 
						  get_ref_mv(b_ref->y, b_refH->y, b_refV->y, b_refHV->y,
								i, j, 16, &mb->b_mvs[0], edged_width),
						  edged_width);
		    i_sad16 += calc_delta_16(mb->mvs[0].x-f_predMV.x, mb->mvs[0].y-f_predMV.y, 
								frame->fcode, frame->quant);
		    i_sad16 += calc_delta_16(mb->b_mvs[0].y-b_predMV.y, mb->b_mvs[0].y-b_predMV.y, 
								frame->bcode, frame->quant);

			// TODO: direct search
			// predictor + delta vector in range [-32,32] (fcode=1)
			
//			i_sad16 = 65535;
//			f_sad16 = 65535;
//			b_sad16 = 65535;

			if (f_sad16 < b_sad16) {
				best_sad = f_sad16;
				mb->mode = MODE_FORWARD;
			} else {
				best_sad = b_sad16;
				mb->mode = MODE_BACKWARD;
			}

			if (i_sad16 < best_sad) {
				best_sad = i_sad16;
				mb->mode = MODE_INTERPOLATE;
			}

			if (dnv_sad16 < best_sad) {

				if (dnv_sad16 > DIRECT_UPPERLIMIT)
				{	
					/* if SAD value is too large, try same vector with MODE_INTERPOLATE 
					   instead (interpolate has residue encoding, direct mode without MV 
					   doesn't) 
					   
						This has to be replaced later by "real" direct mode, including delta 
						vector and (if needed) residue encoding 
					   
				 	*/
				
					directMV = b_mb->mvs[0];

					mb->mvs[0].x = (int32_t) ((TRB * directMV.x) / TRD + deltaMV.x);
    	            mb->b_mvs[0].x = (int32_t) ((deltaMV.x == 0) 
										? ((TRB - TRD) * directMV.x) / TRD
            	                        : mb->mvs[0].x - directMV.x);
                	mb->mvs[0].y = (int32_t) ((TRB * directMV.y) / TRD + deltaMV.y);
	                mb->b_mvs[0].y = (int32_t) ((deltaMV.y == 0) 
										? ((TRB - TRD) * directMV.y) / TRD
        	                            : mb->mvs[0].y - directMV.y);

					dnv_sad16 =
						sad16bi(frame->image.y + i * 16 + j * 16 * edged_width,
						  get_ref_mv(f_ref->y, f_refH->y, f_refV->y, f_refHV->y,
								i, j, 16, &mb->mvs[0], edged_width), 
						  get_ref_mv(b_ref->y, b_refH->y, b_refV->y, b_refHV->y,
								i, j, 16, &mb->b_mvs[0], edged_width),
						  edged_width);
		    		dnv_sad16 += calc_delta_16(mb->mvs[0].x-f_predMV.x, mb->mvs[0].y-f_predMV.y, 
								frame->fcode, frame->quant);
				    dnv_sad16 += calc_delta_16(mb->b_mvs[0].y-b_predMV.y, mb->b_mvs[0].y-b_predMV.y, 
								frame->bcode, frame->quant);
	
					if (dnv_sad16 < best_sad)
					{
						best_sad = dnv_sad16;
						mb->mode = MODE_INTERPOLATE;

/*						fprintf(stderr,"f_sad16 = %d, b_sad16 = %d, i_sad16 = %d, dnv_sad16 = %d\n",
							f_sad16,b_sad16,i_sad16,dnv_sad16); 
*/					}
				}
				else
				{
					best_sad = dnv_sad16;
					mb->mode = MODE_DIRECT_NONE_MV;
				}
			}

			switch (mb->mode)
			{
				case MODE_FORWARD:
					f_count++; 
					f_predMV = mb->mvs[0];
					break;
				case MODE_BACKWARD:
					b_count++; 
					b_predMV = mb->b_mvs[0];

					break;
				case MODE_INTERPOLATE:
					i_count++; 
					f_predMV = mb->mvs[0];
					b_predMV = mb->b_mvs[0];
					break;
				case MODE_DIRECT:
					d_count++; break;
				case MODE_DIRECT_NONE_MV:
					dnv_count++; break;
				default:
					s_count++; break;
			}
			
		}
	}
	
#ifdef _DEBUG_BFRAME_STAT
	fprintf(stderr,"B-Stat: F: %04d   B: %04d   I: %04d  D0: %04d   D: %04d   S: %04d\n",
				f_count,b_count,i_count,dnv_count,d_count,s_count);
#endif

}
