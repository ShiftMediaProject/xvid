/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SAD Routines header -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: sad.h,v 1.17 2002-11-17 00:32:06 edgomez Exp $
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
