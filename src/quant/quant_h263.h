/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - h263 quantization header -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: quant_h263.h,v 1.9 2002-11-17 00:41:19 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _QUANT_H263_H_
#define _QUANT_H263_H_

#include "../portab.h"

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

/* intra */
typedef void (quanth263_intraFunc) (int16_t * coeff,
									const int16_t * data,
									const uint32_t quant,
									const uint32_t dcscalar);

typedef quanth263_intraFunc *quanth263_intraFuncPtr;

extern quanth263_intraFuncPtr quant_intra;
extern quanth263_intraFuncPtr dequant_intra;

quanth263_intraFunc quant_intra_c;
quanth263_intraFunc quant_intra_mmx;
quanth263_intraFunc quant_intra_sse2;
quanth263_intraFunc quant_intra_ia64;

quanth263_intraFunc dequant_intra_c;
quanth263_intraFunc dequant_intra_mmx;
quanth263_intraFunc dequant_intra_xmm;
quanth263_intraFunc dequant_intra_sse2;
quanth263_intraFunc dequant_intra_ia64;

/* inter_quant */
typedef uint32_t(quanth263_interFunc) (int16_t * coeff,
									   const int16_t * data,
									   const uint32_t quant);

typedef quanth263_interFunc *quanth263_interFuncPtr;

extern quanth263_interFuncPtr quant_inter;

quanth263_interFunc quant_inter_c;
quanth263_interFunc quant_inter_mmx;
quanth263_interFunc quant_inter_sse2;
quanth263_interFunc quant_inter_ia64;

/*inter_dequant */
typedef void (dequanth263_interFunc) (int16_t * coeff,
									  const int16_t * data,
									  const uint32_t quant);

typedef dequanth263_interFunc *dequanth263_interFuncPtr;

extern dequanth263_interFuncPtr dequant_inter;

dequanth263_interFunc dequant_inter_c;
dequanth263_interFunc dequant_inter_mmx;
dequanth263_interFunc dequant_inter_xmm;
dequanth263_interFunc dequant_inter_sse2;
dequanth263_interFunc dequant_inter_ia64;

#endif /* _QUANT_H263_H_ */
