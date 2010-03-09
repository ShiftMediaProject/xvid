/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - multithreaded Motion Estimation header -
 *
 *  Copyright(C) 2005 Radoslaw Czyz <xvid@syskin.cjb.net>
 *
 *  significant portions derived from x264 project,
 *  original authors: Trax, Gianluigi Tiesi, Eric Petit
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
 * $Id: motion_smp.h,v 1.7 2010-03-09 10:00:14 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef SMP_MOTION_H
#define SMP_MOTION_H

typedef struct
{
	pthread_t handle;		/* thread's handle */
	const MBParam * pParam;
	const FRAMEINFO * current;
	const FRAMEINFO * reference;
	const IMAGE * pRefH;
	const IMAGE * pRefV;
	const IMAGE * pRefHV;
	const IMAGE * pGMC;
	uint8_t * RefQ;
	int y_step;
	int start_y;
	int * complete_count_self;
	int * complete_count_above;
	
	/* bvop stuff */
	int time_bp, time_pp;
	const MACROBLOCK * f_mbs;
	const IMAGE * pRef;
	const IMAGE * fRef;
	const IMAGE * fRefH;
	const IMAGE * fRefV;
	const IMAGE * fRefHV;

	int MVmax, mvSum, mvCount;		/* out */

	int minfcode, minbcode;
} SMPmotionData;


void MotionEstimateSMP(SMPmotionData * h);
void SMPMotionEstimationBVOP(SMPmotionData * h);

#endif /* SMP_MOTION_H */
