/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion estimation fuctions header file -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: mbfunctions.h,v 1.14 2002-10-19 11:41:11 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _MBFUNCTIONS_H
#define _MBFUNCTIONS_H

#include "../encoder.h"
#include "../bitstream/bitstream.h"

/*****************************************************************************
 * Prototypes
 ****************************************************************************/


/* MotionEstimation */

bool MotionEstimation(MBParam * const pParam,
					  FRAMEINFO * const current,
					  FRAMEINFO * const reference,
					  const IMAGE * const pRefH,
					  const IMAGE * const pRefV,
					  const IMAGE * const pRefHV,
					  const uint32_t iLimit);


bool SMP_MotionEstimation(MBParam * const pParam,
						  FRAMEINFO * const current,
						  FRAMEINFO * const reference,
						  const IMAGE * const pRefH,
						  const IMAGE * const pRefV,
						  const IMAGE * const pRefHV,
						  const uint32_t iLimit);



/* MBMotionCompensation */

void MBMotionCompensation(MACROBLOCK * const pMB,
						  const uint32_t j,
						  const uint32_t i,
						  const IMAGE * const pRef,
						  const IMAGE * const pRefH,
						  const IMAGE * const pRefV,
						  const IMAGE * const pRefHV,
						  IMAGE * const pCurrent,
						  int16_t dct_codes[6 * 64],
						  const uint32_t width,
						  const uint32_t height,
						  const uint32_t edged_width,
						  const uint32_t rounding);


/* MBTransQuant.c */

void MBTransQuantIntra(const MBParam * pParam,
					   FRAMEINFO * frame,
					   MACROBLOCK * pMB,
					   const uint32_t x_pos,    /* <-- The x position of the MB to be searched */
					   const uint32_t y_pos,    /* <-- The y position of the MB to be searched */
					   int16_t data[6 * 64],    /* <-> the data of the MB to be coded */
					   int16_t qcoeff[6 * 64]); /* <-> the quantized DCT coefficients */
	


void MBTransQuantIntra2(const MBParam * pParam,
						FRAMEINFO * frame,
						MACROBLOCK * pMB,
						const uint32_t x_pos,    /* <-- The x position of the MB to be searched */
						const uint32_t y_pos,    /* <-- The y position of the MB to be searched */
						int16_t data[6 * 64],    /* <-> the data of the MB to be coded */
						int16_t qcoeff[6 * 64]); /* <-> the quantized DCT coefficients */
	


uint8_t MBTransQuantInter(const MBParam * pParam,
						  FRAMEINFO * frame,
						  MACROBLOCK * pMB,
						  const uint32_t x_pos,
						  const uint32_t y_pos,
						  int16_t data[6 * 64],
						  int16_t qcoeff[6 * 64]);


uint8_t MBTransQuantInter2(const MBParam * pParam,
						   FRAMEINFO * frame,
						   MACROBLOCK * pMB,
						   const uint32_t x_pos,
						   const uint32_t y_pos,	
						   int16_t data[6 * 64],
						   int16_t qcoeff[6 * 64]);

uint8_t MBTransQuantInterBVOP(const MBParam * pParam,
							  FRAMEINFO * frame,
							  MACROBLOCK * pMB,
							  int16_t data[6 * 64],
							  int16_t qcoeff[6 * 64]);

void MBTrans(const MBParam * pParam,
			 FRAMEINFO * frame,
			 MACROBLOCK * pMB,
			 const uint32_t x_pos,
			 const uint32_t y_pos,
			 int16_t data[6 * 64]);

void MBfDCT(const MBParam * pParam,
			FRAMEINFO * frame,
			MACROBLOCK * pMB,
			int16_t data[6 * 64]);

uint8_t MBQuantInter(const MBParam * pParam,
					 const int iQuant,
					 int16_t data[6 * 64],
					 int16_t qcoeff[6 * 64]);

void MBQuantDeQuantIntra(const MBParam * pParam,
					  	 FRAMEINFO * frame,
						 MACROBLOCK *pMB,
				  		 int16_t qcoeff[6 * 64],
  				  		 int16_t data[6*64]);

void MBQuantIntra(const MBParam * pParam,
				  FRAMEINFO * frame,
				  MACROBLOCK *pMB,
				  int16_t data[6*64],
				  int16_t qcoeff[6 * 64]);

void MBDeQuantIntra(const MBParam * pParam,
			   		const int iQuant,
				  	int16_t qcoeff[6 * 64],
				  	int16_t data[6*64]);

void MBDeQuantInter(const MBParam * pParam,
					const int iQuant,
					int16_t data[6 * 64],
					int16_t qcoeff[6 * 64],
				  	const uint8_t cbp);


void MBiDCT(int16_t data[6 * 64], 
			const uint8_t cbp);


void MBTransAdd(const MBParam * pParam,
				FRAMEINFO * frame,
				MACROBLOCK * pMB,
				const uint32_t x_pos,
				const uint32_t y_pos,
				int16_t data[6 * 64],
				const uint8_t cbp);



/* interlacing */

uint32_t MBDecideFieldDCT(int16_t data[6 * 64]); /* <- decide whether to use field-based DCT for interlacing */

void MBFrameToField(int16_t data[6 * 64]);       /* de-interlace vertical Y blocks */


/* MBCoding.c */

void MBSkip(Bitstream * bs); /* just the bitstream. Since MB is skipped, no info is needed */


void MBCoding(const FRAMEINFO * frame, /* <-- the parameter for coding of the bitstream */
			  MACROBLOCK * pMB,        /* <-- Info of the MB to be coded */
			  int16_t qcoeff[6 * 64],  /* <-- the quantized DCT coefficients */
			  Bitstream * bs,          /* <-> the bitstream */
			  Statistics * pStat);     /* <-> statistical data collected for current frame */

#endif
