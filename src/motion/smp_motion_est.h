/**************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  -  SMP Motion search header  -
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
 *  $Id: smp_motion_est.h,v 1.1 2002-07-06 17:03:08 chl Exp $
 *
 ***************************************************************************/

#ifndef _SMP_MOTION_EST_H
#define _SMP_MOTION_EST_H

#ifdef _SMP

#define MAXNUMTHREADS 16

#define NUMTHREADS 2

typedef struct 
{

		MBParam * pParam;

		FRAMEINFO* reference;
		FRAMEINFO* current;
		
		IMAGE* pRef;
		IMAGE* pRefH;
		IMAGE* pRefV;
		IMAGE* pRefHV;

//		int iLimit;		/* currently unused */

		MACROBLOCK * pMBs;
		MACROBLOCK * prevMBs;

} globaldata;		/* this data is the same for all threads */

typedef struct 
{
		int id;

		int minx;
		int maxx;
		int miny;
		int maxy;
		
		globaldata *gdata;

} jobdata;			/* every thread get it's personal version of these */


void 
SMP_correct_pmv(int x, int y, int iWcount, MACROBLOCK* pMBs);

void 
SMP_MotionEstimationWorker(jobdata *arg);

bool
SMP_MotionEstimation(MBParam * const pParam,
				 FRAMEINFO * const current,
				 FRAMEINFO * const reference,
				 const IMAGE * const pRefH,
				 const IMAGE * const pRefV,
				 const IMAGE * const pRefHV,
				 const uint32_t iLimit);

#endif

#endif
