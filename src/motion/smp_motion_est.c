/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	multithreaded motion estimation
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


/*   WARNING !
 *   This is early alpha code, only published for debugging and testing 
 *   I cannot garantee that it will not break compiliing, encoding 
 *	 or your marriage etc. YOU HAVE BEEN WARNED!
 */

#ifdef _SMP

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <signal.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "../encoder.h"
#include "../utils/mbfunctions.h"
#include "../prediction/mbprediction.h"
#include "../global.h"
#include "../utils/timer.h"
#include "motion.h"
#include "sad.h"

#include "smp_motion_est.h"

/* global variables for SMP search control, the are not needed anywhere else than here */

pthread_mutex_t me_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t me_inqueue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t me_corrqueue_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t me_outqueue_cond = PTHREAD_COND_INITIALIZER;


int me_iIntra=0;
int me_inqueue=0;		// input queue
int me_corrqueue=0;		// correction queue
int me_outqueue=0;		// output queue


void SMP_correct_pmv(int x, int y, int iWcount, MACROBLOCK* pMBs)
{
	MACROBLOCK *const pMB = &pMBs[x + y * iWcount];
	VECTOR pmv;
	int k;
	
	switch (pMB->mode) {
		
	case MODE_INTER:
	case MODE_INTER_Q:
		pmv = get_pmv2(pMBs, iWcount, 0, x, y, 0);   
		pMB->pmvs[0].x = pMB->mvs[0].x - pmv.x;
		pMB->pmvs[0].y = pMB->mvs[0].y - pmv.y;
		break;
				
	case MODE_INTER4V:
		for (k=0;k<4;k++) {
			pmv = get_pmv2(pMBs, iWcount, 0, x, y, k);   
			pMB->pmvs[k].x = pMB->mvs[k].x - pmv.x;
			pMB->pmvs[k].y = pMB->mvs[k].y - pmv.y;
		}
		break;
				
	default:
		break; /* e.g. everything without prediction, e.g. MODE_INTRA */
	}
	return;
}

void SMP_MotionEstimationWorker(jobdata *arg)
{
	const VECTOR zeroMV = { 0, 0 };

//	long long time;
	int32_t x, y;
	VECTOR pmv;

	globaldata* gdata;

	MBParam * pParam;
	FRAMEINFO * current;
	FRAMEINFO * reference;
	IMAGE *  pRefH;
	IMAGE *  pRefV;
	IMAGE *  pRefHV;
//	uint32_t iLimit;

	uint32_t iWcount;
	uint32_t iHcount;
	MACROBLOCK * pMBs;
	MACROBLOCK * prevMBs;
	IMAGE * pCurrent;
	IMAGE * pRef;

	int minx = arg->minx;
	int maxx = arg->maxx;
	int miny = arg->miny;
	int maxy = arg->maxy;

//	int run=0; 
	int iIntra;

	pthread_mutex_lock(&me_mutex);
	while (1)
	{	
//		run++;
		iIntra = 0;

//		fprintf(stderr,"[%d,%d] wait inqueue %d init\n",arg->id,run,me_inqueue);
		while (!me_inqueue)
			pthread_cond_wait(&me_inqueue_cond,&me_mutex);

//	fprintf(stderr,"[%d,%d] wait inqueue %d done\n",arg->id,run,me_inqueue);
	
		me_inqueue--;
		pthread_mutex_unlock(&me_mutex);

		gdata = arg->gdata;
		pParam = gdata->pParam;
		current = gdata->current;
		reference = gdata->reference;
		pRefH = gdata->pRefH;
		pRefV = gdata->pRefV;
		pRefHV = gdata->pRefHV;
//		iLimit = gdata->iLimit;

		iWcount = pParam->mb_width;
		iHcount = pParam->mb_height;
		pMBs = current->mbs;
		prevMBs = reference->mbs;
		pCurrent = &current->image;
		pRef = &reference->image;

//		time = read_counter();

		for (y = miny; y < maxy; y++)	{
		for (x = minx; x < maxx; x++)	{

			MACROBLOCK *const pMB = &pMBs[x + y * iWcount];
	
			pMB->sad16 =
				SEARCH16(pRef->y, pRefH->y, pRefV->y, pRefHV->y, pCurrent, 
						 x, y, current->motion_flags, current->quant,
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
					continue;
				}
			}

			pmv = pMB->pmvs[0];
			if (current->global_flags & XVID_INTER4V)
				if ((!(current->global_flags & XVID_LUMIMASKING) ||
					 pMB->dquant == NO_CHANGE)) {
					int32_t sad8 = IMV16X16 * current->quant;

					if (sad8 < pMB->sad16)

						sad8 += pMB->sad8[0] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x, 2 * y, pMB->mv16.x,
									pMB->mv16.y, current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[0],
									&pMB->pmvs[0]);

					if (sad8 < pMB->sad16)
						sad8 += pMB->sad8[1] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x + 1, 2 * y, pMB->mv16.x,
									pMB->mv16.y, current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[1],
									&pMB->pmvs[1]);

					if (sad8 < pMB->sad16)
						sad8 += pMB->sad8[2] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x, 2 * y + 1, pMB->mv16.x,
									pMB->mv16.y, current->motion_flags,
									current->quant, current->fcode, pParam,
									pMBs, prevMBs, &pMB->mvs[2],
									&pMB->pmvs[2]);

					if (sad8 < pMB->sad16)
						sad8 += pMB->sad8[3] =
							SEARCH8(pRef->y, pRefH->y, pRefV->y, pRefHV->y,
									pCurrent, 2 * x + 1, 2 * y + 1,
									pMB->mv16.x, pMB->mv16.y,
									current->motion_flags, current->quant,
									current->fcode, pParam, pMBs, prevMBs,
									&pMB->mvs[3], &pMB->pmvs[3]);

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
			pMB->sad8[0] = pMB->sad8[1] = pMB->sad8[2] = pMB->sad8[3] = pMB->sad16;

			}
		}	/* end of x/y loop */
		
//		fprintf(stderr,"[%d,%d] Full ME %lld ticks \n",arg->id,run,read_counter()-time);

		pthread_mutex_lock(&me_mutex);

		me_corrqueue--;		// the last to finish wakes the others to start correction 
		me_iIntra += iIntra;

		if (me_corrqueue==0)
		{
//			fprintf(stderr,"[%d,%d] corrqueue %d waking neighbours\n",arg->id,run,me_corrqueue);
			pthread_cond_broadcast(&me_corrqueue_cond);
		}

//		fprintf(stderr,"[%d,%d] wait corrqueue %d init\n",arg->id,run,me_corrqueue);

		while (me_corrqueue)
			pthread_cond_wait(&me_corrqueue_cond,&me_mutex);

//		fprintf(stderr,"[%d,%d] wait corrqueue %d done\n",arg->id,run,me_corrqueue);

//		time = read_counter();

//		if (me_iIntra <= iLimit) 
//		{
//			pthread_mutex_unlock(&me_mutex);

			if (minx!=0) 
				for (y=miny; y<maxy; y++)
					SMP_correct_pmv(minx,  y,iWcount,pMBs);
			if (maxx!=iWcount) 
				for (y=miny; y<maxy; y++)
					SMP_correct_pmv(maxx-1,y,iWcount,pMBs);

			if (miny!=0)
				for (x=minx;x<maxx; x++)
					SMP_correct_pmv(x,miny,  iWcount,pMBs);

			if (maxy!=iHcount) 
				for (x=minx;x<maxx; x++)
					SMP_correct_pmv(x,maxy-1,iWcount,pMBs);

//			pthread_mutex_lock(&me_mutex);
//		}

//		fprintf(stderr,"[%d,%d] Full CORR %lld ticks \n",arg->id,run,read_counter()-time);

		me_outqueue--;		

		if (me_outqueue==0)		// the last to finish wakes the master
			pthread_cond_signal(&me_outqueue_cond);

//		fprintf(stderr,"[%d,%d] wait outqueue %d init\n",arg->id,run,me_outqueue);
		
//		while (me_outqueue)
//			pthread_cond_wait(&me_outqueue_cond,&me_mutex);
			
//		fprintf(stderr,"[%d,%d] wait outqueue %d done\n",arg->id,run,me_outqueue);
	
	} /* end of while(1) */
	pthread_mutex_unlock(&me_mutex);
}

bool
SMP_MotionEstimation(MBParam * const pParam,
				 FRAMEINFO * const current,
				 FRAMEINFO * const reference,
				 const IMAGE * const pRefH,
				 const IMAGE * const pRefV,
				 const IMAGE * const pRefHV,
				 const uint32_t iLimit)
{
	int i;
	static int threadscreated=0;
	
	const int iWcount = pParam->mb_width;
	const int iHcount = pParam->mb_height;
	
	static globaldata 	gdata;
	static jobdata 		jdata[MAXNUMTHREADS];
	static pthread_t 	me_thread[MAXNUMTHREADS];
	
	gdata.pParam = pParam;
	gdata.reference = reference;
	gdata.current = current;
	gdata.pRefH = pRefH;
	gdata.pRefV = pRefV;
	gdata.pRefHV = pRefHV;
//	gdata.iLimit = iLimit;

	if (sadInit)
		(*sadInit) ();

	pthread_mutex_lock(&me_mutex);
	me_iIntra=0;
	me_inqueue=pParam->num_threads;
	me_corrqueue=pParam->num_threads;
	me_outqueue=pParam->num_threads;

	if (!threadscreated)
	{
		for (i=0;i<pParam->num_threads;i++) {	/* split domain into NUMTHREADS parts */

			jdata[i].id = i;
			jdata[i].minx = i*iWcount/pParam->num_threads;
			jdata[i].maxx = (i+1)*iWcount/pParam->num_threads;
			jdata[i].miny = 0;
			jdata[i].maxy = iHcount;
			jdata[i].gdata = &gdata;

			pthread_create(&me_thread[i],NULL,
					(void*)SMP_MotionEstimationWorker,(void*)&jdata[i]);
		}
		threadscreated=1;
	}

	pthread_cond_broadcast(&me_inqueue_cond);	// start working

	while (me_outqueue)	
			pthread_cond_wait(&me_outqueue_cond,&me_mutex);

	if (me_iIntra > iLimit)
	{
		pthread_mutex_unlock(&me_mutex);
		return 1;
	}
	
	pthread_mutex_unlock(&me_mutex);
	return 0;
}

#endif
