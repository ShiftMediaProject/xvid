/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SAD Routines header -
 *
 *  Copyright(C) 2002 Michael Militzer
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
 *  Copyright(C) 2002 Michael Militzer
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
 *
 ****************************************************************************/

#ifndef _ENCODER_SAD_H_
#define _ENCODER_SAD_H_

#include "../portab.h"

typedef void (sadInitFunc) (void);
typedef sadInitFunc *sadInitFuncPtr;

extern sadInitFuncPtr sadInit;
sadInitFunc sadInit_altivec;


typedef uint32_t(sad16Func) (const uint8_t * const cur,
							 const uint8_t * const ref,
							 const uint32_t stride,
							 const uint32_t best_sad);
typedef sad16Func *sad16FuncPtr;
extern sad16FuncPtr sad16;
sad16Func sad16_c;
sad16Func sad16_mmx;
sad16Func sad16_xmm;
sad16Func sad16_sse2;
sad16Func sad16_altivec;
sad16Func sad16_ia64;

sad16Func mrsad16_c;


typedef uint32_t(sad8Func) (const uint8_t * const cur,
							const uint8_t * const ref,
							const uint32_t stride);
typedef sad8Func *sad8FuncPtr;
extern sad8FuncPtr sad8;
sad8Func sad8_c;
sad8Func sad8_mmx;
sad8Func sad8_xmm;
sad8Func sad8_altivec;
sad8Func sad8_ia64;


typedef uint32_t(sad16biFunc) (const uint8_t * const cur,
							   const uint8_t * const ref1,
							   const uint8_t * const ref2,
							   const uint32_t stride);
typedef sad16biFunc *sad16biFuncPtr;
extern sad16biFuncPtr sad16bi;
sad16biFunc sad16bi_c;
sad16biFunc sad16bi_ia64;
sad16biFunc sad16bi_mmx;
sad16biFunc sad16bi_xmm;
sad16biFunc sad16bi_3dn;


typedef uint32_t(sad8biFunc) (const uint8_t * const cur,
							   const uint8_t * const ref1,
							   const uint8_t * const ref2,
							   const uint32_t stride);
typedef sad8biFunc *sad8biFuncPtr;
extern sad8biFuncPtr sad8bi;
sad8biFunc sad8bi_c;
sad8biFunc sad8bi_mmx;
sad8biFunc sad8bi_xmm;
sad8biFunc sad8bi_3dn;


typedef uint32_t(dev16Func) (const uint8_t * const cur,
							 const uint32_t stride);
typedef dev16Func *dev16FuncPtr;
extern dev16FuncPtr dev16;
dev16Func dev16_c;
dev16Func dev16_mmx;
dev16Func dev16_xmm;
dev16Func dev16_sse2;
dev16Func dev16_altivec;
dev16Func dev16_ia64;

/* plain c */
/*

uint32_t sad16(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16(const uint8_t * const cur,
				const uint32_t stride);
*/
/* mmx */
/*

uint32_t sad16_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);


uint32_t dev16_mmx(const uint8_t * const cur,
				const uint32_t stride);

*/
/* xmm */
/*
uint32_t sad16_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16_xmm(const uint8_t * const cur,
				const uint32_t stride);
*/

#endif							/* _ENCODER_SAD_H_ */
