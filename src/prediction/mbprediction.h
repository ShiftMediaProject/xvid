/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  MB prediction header file  -
 *
 *  Copyright(C) 2002 Christoph Lampert <gruel@web.de>
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
 * $Id: mbprediction.h,v 1.18 2002-11-26 23:44:11 edgomez Exp $
 *
 *************************************************************************/

#ifndef _MBPREDICTION_H_
#define _MBPREDICTION_H_

#include "../portab.h"
#include "../decoder.h"
#include "../global.h"

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))

/* very large value */
#define MV_MAX_ERROR	(4096 * 256)

#define MVequal(A,B) ( ((A).x)==((B).x) && ((A).y)==((B).y) )

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void MBPrediction(FRAMEINFO * frame,	/* <-- The parameter for ACDC and MV prediction */

				  uint32_t x_pos,	/* <-- The x position of the MB to be searched  */

				  uint32_t y_pos,	/* <-- The y position of the MB to be searched  */

				  uint32_t x_dim,	/* <-- Number of macroblocks in a row           */

				  int16_t * qcoeff);	/* <-> The quantized DCT coefficients           */

void add_acdc(MACROBLOCK * pMB,
			  uint32_t block,
			  int16_t dct_codes[64],
			  uint32_t iDcScaler,
			  int16_t predictors[8]);


void predict_acdc(MACROBLOCK * pMBs,
				  uint32_t x,
				  uint32_t y,
				  uint32_t mb_width,
				  uint32_t block,
				  int16_t qcoeff[64],
				  uint32_t current_quant,
				  int32_t iDcScaler,
				  int16_t predictors[8],
				const int bound);


/*****************************************************************************
 * Inlined functions
 ****************************************************************************/

/*
 * MODE_INTER, vm18 page 48
 * MODE_INTER4V vm18 page 51
 *
 *   (x,y-1)      (x+1,y-1)
 *   [   |   ]    [   |   ]
 *   [ 2 | 3 ]    [ 2 |   ]
 *
 *   (x-1,y)       (x,y)        (x+1,y)
 *   [   | 1 ]    [ 0 | 1 ]    [ 0 |   ]
 *   [   | 3 ]    [ 2 | 3 ]    [   |   ]
 */

static __inline VECTOR
get_pmv2(const MACROBLOCK * const mbs,
         const int mb_width,
         const int bound,
         const int x,
         const int y,
         const int block)
{
	static const VECTOR zeroMV = { 0, 0 };
    
    int lx, ly, lz;         /* left */
    int tx, ty, tz;         /* top */
    int rx, ry, rz;         /* top-right */
    int lpos, tpos, rpos;
    int num_cand, last_cand;

	VECTOR pmv[4];	/* left neighbour, top neighbour, top-right neighbour */

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

    lpos = lx + ly * mb_width;
    rpos = rx + ry * mb_width;
    tpos = tx + ty * mb_width;
    last_cand = num_cand = 0;

    if (lpos >= bound && lx >= 0) {
        num_cand++;
        last_cand = 1;
        pmv[1] = mbs[lpos].mvs[lz];
    } else {
        pmv[1] = zeroMV;
    }

    if (tpos >= bound) {
        num_cand++;
        last_cand = 2;
        pmv[2] = mbs[tpos].mvs[tz];
    } else {
        pmv[2] = zeroMV;
    }
    
    if (rpos >= bound && rx < mb_width) {
        num_cand++;
        last_cand = 3;
        pmv[3] = mbs[rpos].mvs[rz];
    } else {
        pmv[3] = zeroMV;
    }

    /*
	 * If there're more than one candidate, we return the median vector
	 * edgomez : the special case "no candidates" is handled the same way
	 *           because all vectors are set to zero. So the median vector
	 *           is {0,0}, and this is exactly the vector we must return
	 *           according to the mpeg4 specs.
	 */

	if (num_cand != 1) {
		/* set median */
   
   		pmv[0].x =
			MIN(MAX(pmv[1].x, pmv[2].x),
				MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
		pmv[0].y =
			MIN(MAX(pmv[1].y, pmv[2].y),
				MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
		return pmv[0];
	 }

	 return pmv[last_cand];  /* no point calculating median mv */
}



/*
 * pmv are filled with: 
 *  [0]: Median (or whatever is correct in a special case)
 *  [1]: left neighbour
 *  [2]: top neighbour
 *  [3]: topright neighbour
 * psad are filled with:
 *  [0]: minimum of [1] to [3]
 *  [1]: left neighbour's SAD (NB:[1] to [3] are actually not needed)  
 *  [2]: top neighbour's SAD
 *  [3]: topright neighbour's SAD
 */
	 
static __inline int
get_pmvdata2(const MACROBLOCK * const mbs,
         const int mb_width,
         const int bound,
         const int x,
         const int y,
         const int block,
		 VECTOR * const pmv,
		 int32_t * const psad)
{
	static const VECTOR zeroMV = { 0, 0 };
    
    int lx, ly, lz;         /* left */
    int tx, ty, tz;         /* top */
    int rx, ry, rz;         /* top-right */
    int lpos, tpos, rpos;
    int num_cand, last_cand;

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

    lpos = lx + ly * mb_width;
    rpos = rx + ry * mb_width;
    tpos = tx + ty * mb_width;
    last_cand = num_cand = 0;

    if (lpos >= bound && lx >= 0) {
        num_cand++;
        last_cand = 1;
        pmv[1] = mbs[lpos].mvs[lz];
		psad[1] = mbs[lpos].sad8[lz];
    } else {
        pmv[1] = zeroMV;
		psad[1] = MV_MAX_ERROR;
    }

    if (tpos >= bound) {
        num_cand++;
        last_cand = 2;
        pmv[2]= mbs[tpos].mvs[tz];
        psad[2] = mbs[tpos].sad8[tz];
    } else {
        pmv[2] = zeroMV;
		psad[2] = MV_MAX_ERROR;
    }
    
    if (rpos >= bound && rx < mb_width) {
        num_cand++;
        last_cand = 3;
        pmv[3] = mbs[rpos].mvs[rz];
        psad[3] = mbs[rpos].sad8[rz];
    } else {
        pmv[3] = zeroMV;
		psad[3] = MV_MAX_ERROR;
    }

	/* original pmvdata() compatibility hack */
	if (x == 0 && y == 0 && block == 0)
	{
		pmv[0] = pmv[1] = pmv[2] = pmv[3] = zeroMV;
		psad[0] = 0;
		psad[1] = psad[2] = psad[3] = MV_MAX_ERROR;
		return 0;
	}

    /* if only one valid candidate preictor, the invalid candiates are set to the canidate */
	if (num_cand == 1) {
		pmv[0] = pmv[last_cand];
		psad[0] = psad[last_cand];
        /* return MVequal(pmv[0], zeroMV);  no point calculating median mv and minimum sad  */
		
		/* original pmvdata() compatibility hack */
		return y==0 && block <= 1 ? 0 : MVequal(pmv[0], zeroMV);
	}

	if ((MVequal(pmv[1], pmv[2])) && (MVequal(pmv[1], pmv[3]))) {
		pmv[0] = pmv[1];
		psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);
		return 1;
		/* compatibility patch */
		/*return y==0 && block <= 1 ? 0 : 1; */
	}

	/* set median, minimum */

	pmv[0].x =
		MIN(MAX(pmv[1].x, pmv[2].x),
			MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
	pmv[0].y =
		MIN(MAX(pmv[1].y, pmv[2].y),
			MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));

	psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);

   	return 0;
}

/* copies of get_pmv and get_pmvdata for prediction from integer search */

static __inline VECTOR
get_ipmv(const MACROBLOCK * const mbs,
         const int mb_width,
         const int bound,
         const int x,
         const int y,
         const int block)
{
	static const VECTOR zeroMV = { 0, 0 };
    
	int lx, ly, lz;         /* left */
	int tx, ty, tz;         /* top */
	int rx, ry, rz;         /* top-right */
	int lpos, tpos, rpos;
	int num_cand, last_cand;

	VECTOR pmv[4];	/* left neighbour, top neighbour, top-right neighbour */

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

    lpos = lx + ly * mb_width;
    rpos = rx + ry * mb_width;
    tpos = tx + ty * mb_width;
    last_cand = num_cand = 0;

    if (lpos >= bound && lx >= 0) {
        num_cand++;
        last_cand = 1;
        pmv[1] = mbs[lpos].i_mvs[lz];
    } else {
        pmv[1] = zeroMV;
    }

    if (tpos >= bound) {
        num_cand++;
        last_cand = 2;
        pmv[2] = mbs[tpos].i_mvs[tz];
    } else {
        pmv[2] = zeroMV;
    }
    
    if (rpos >= bound && rx < mb_width) {
        num_cand++;
        last_cand = 3;
        pmv[3] = mbs[rpos].i_mvs[rz];
    } else {
        pmv[3] = zeroMV;
    }

    /* if only one valid candidate predictor, the invalid candiates are set to the canidate */
	if (num_cand != 1) {
		/* set median */
   
   		pmv[0].x =
			MIN(MAX(pmv[1].x, pmv[2].x),
				MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
		pmv[0].y =
			MIN(MAX(pmv[1].y, pmv[2].y),
				MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));
		return pmv[0];
	 }

	 return pmv[last_cand];  /* no point calculating median mv */
}

static __inline int
get_ipmvdata(const MACROBLOCK * const mbs,
         const int mb_width,
         const int bound,
         const int x,
         const int y,
         const int block,
		 VECTOR * const pmv,
		 int32_t * const psad)
{
	static const VECTOR zeroMV = { 0, 0 };
    
    int lx, ly, lz;         /* left */
    int tx, ty, tz;         /* top */
    int rx, ry, rz;         /* top-right */
    int lpos, tpos, rpos;
    int num_cand, last_cand;

	switch (block) {
	case 0:
		lx = x - 1;	ly = y;		lz = 1;
		tx = x;		ty = y - 1;	tz = 2;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 1:
		lx = x;		ly = y;		lz = 0;
		tx = x;		ty = y - 1;	tz = 3;
		rx = x + 1;	ry = y - 1;	rz = 2;
		break;
	case 2:
		lx = x - 1;	ly = y;		lz = 3;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
		break;
	default:
		lx = x;		ly = y;		lz = 2;
		tx = x;		ty = y;		tz = 0;
		rx = x;		ry = y;		rz = 1;
	}

    lpos = lx + ly * mb_width;
    rpos = rx + ry * mb_width;
    tpos = tx + ty * mb_width;
    last_cand = num_cand = 0;

    if (lpos >= bound && lx >= 0) {
        num_cand++;
        last_cand = 1;
        pmv[1] = mbs[lpos].i_mvs[lz];
		psad[1] = mbs[lpos].i_sad8[lz];
    } else {
        pmv[1] = zeroMV;
		psad[1] = MV_MAX_ERROR;
    }

    if (tpos >= bound) {
        num_cand++;
        last_cand = 2;
        pmv[2]= mbs[tpos].i_mvs[tz];
        psad[2] = mbs[tpos].i_sad8[tz];
    } else {
        pmv[2] = zeroMV;
		psad[2] = MV_MAX_ERROR;
    }
    
    if (rpos >= bound && rx < mb_width) {
        num_cand++;
        last_cand = 3;
        pmv[3] = mbs[rpos].i_mvs[rz];
        psad[3] = mbs[rpos].i_sad8[rz];
    } else {
        pmv[3] = zeroMV;
		psad[3] = MV_MAX_ERROR;
    }

	/* original pmvdata() compatibility hack */
	if (x == 0 && y == 0 && block == 0)
	{
		pmv[0] = pmv[1] = pmv[2] = pmv[3] = zeroMV;
		psad[0] = 0;
		psad[1] = psad[2] = psad[3] = MV_MAX_ERROR;
		return 0;
	}

    /* if only one valid candidate preictor, the invalid candiates are set to the canidate */
	if (num_cand == 1) {
		pmv[0] = pmv[last_cand];
		psad[0] = psad[last_cand];
        /* return MVequal(pmv[0], zeroMV); no point calculating median mv and minimum sad */
		
		/* original pmvdata() compatibility hack */
		return y==0 && block <= 1 ? 0 : MVequal(pmv[0], zeroMV);
	}

	if ((MVequal(pmv[1], pmv[2])) && (MVequal(pmv[1], pmv[3]))) {
		pmv[0] = pmv[1];
		psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);
		return 1;
		/* compatibility patch */
		/*return y==0 && block <= 1 ? 0 : 1; */
	}

	/* set median, minimum */

	pmv[0].x =
		MIN(MAX(pmv[1].x, pmv[2].x),
			MIN(MAX(pmv[2].x, pmv[3].x), MAX(pmv[1].x, pmv[3].x)));
	pmv[0].y =
		MIN(MAX(pmv[1].y, pmv[2].y),
			MIN(MAX(pmv[2].y, pmv[3].y), MAX(pmv[1].y, pmv[3].y)));

	psad[0] = MIN(MIN(psad[1], psad[2]), psad[3]);

   	return 0;
}


#endif							/* _MBPREDICTION_H_ */
