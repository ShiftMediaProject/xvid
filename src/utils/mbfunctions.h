/**************************************************************************
 *
 *  Modifications:
 *
 *  29.03.2002 removed MBFieldToFrame - no longer used (transfers instead)
 *  26.03.2002 interlacing support
 *  02.12.2001 motion estimation/compensation split
 *  16.11.2001 const/uint32_t changes to MBMotionEstComp()
 *  26.08.2001 added inter4v_mode parameter to MBMotionEstComp()
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_BLOCK_H
#define _ENCORE_BLOCK_H

#include "../encoder.h"
#include "../bitstream/bitstream.h"

/** MotionEstimation **/

bool MotionEstimation(MBParam * const pParam,
					FRAMEINFO * const current,
					FRAMEINFO * const reference,
					const IMAGE * const pRefH,
					const IMAGE * const pRefV,
					const IMAGE * const pRefHV,
					const uint32_t iLimit);

/** MBMotionCompensation **/

void
MBMotionCompensation(MACROBLOCK * const mb,
					const uint32_t i,
					const uint32_t j,
					const IMAGE * const ref,
					const IMAGE * const refh,
					const IMAGE * const refv,
					const IMAGE * const refhv,
					const IMAGE * const refGMC,
					IMAGE * const cur,
					int16_t * dct_codes,
					const uint32_t width,
					const uint32_t height,
					const uint32_t edged_width,
					const int32_t quarterpel,
					const int reduced_resolution,
					const int32_t rounding);

/** MBTransQuant.c **/


void MBTransQuantIntra(const MBParam * const pParam,
					FRAMEINFO * const frame,
					MACROBLOCK * const pMB,
					const uint32_t x_pos,	/* <-- The x position of the MB to be searched */
					const uint32_t y_pos,	/* <-- The y position of the MB to be searched */
					int16_t data[6 * 64],	/* <-> the data of the MB to be coded */
					int16_t qcoeff[6 * 64]);	/* <-> the quantized DCT coefficients */

uint8_t MBTransQuantInter(const MBParam * const pParam,
						FRAMEINFO * const frame,
						MACROBLOCK * const pMB,
						const uint32_t x_pos,
						const uint32_t y_pos,
						int16_t data[6 * 64],
						int16_t qcoeff[6 * 64]);

uint8_t MBTransQuantInterBVOP(const MBParam * pParam,
						FRAMEINFO * frame,
						MACROBLOCK * pMB,
						int16_t data[6 * 64],
						int16_t qcoeff[6 * 64]);

/** interlacing **/

uint32_t MBDecideFieldDCT(int16_t data[6 * 64]);	/* <- decide whether to use field-based DCT
														for interlacing */

typedef uint32_t (MBFIELDTEST) (int16_t data[6 * 64]);	/* function pointer for field test */
typedef MBFIELDTEST *MBFIELDTEST_PTR;

/* global field test pointer for xvid.c */
extern MBFIELDTEST_PTR MBFieldTest;

/* field test implementations */
MBFIELDTEST MBFieldTest_c;
MBFIELDTEST MBFieldTest_mmx;

void MBFrameToField(int16_t data[6 * 64]);	/* de-interlace vertical Y blocks */


/** MBCoding.c **/

void MBCoding(const FRAMEINFO * const frame,	/* <-- the parameter for coding of the bitstream */
			MACROBLOCK * pMB,	/* <-- Info of the MB to be coded */
			int16_t qcoeff[6 * 64],	/* <-- the quantized DCT coefficients */
			Bitstream * bs,	/* <-> the bitstream */
			Statistics * pStat);	/* <-> statistical data collected for current frame */

#endif
