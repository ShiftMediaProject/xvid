/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Mpeg4 quantization/dequantization header -
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
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
 *  $Id: quant_mpeg4.h,v 1.5 2002-09-21 03:07:56 suxen_drol Exp $
 *
 ****************************************************************************/

#ifndef _QUANT_MPEG4_H_
#define _QUANT_MPEG4_H_

#include "../portab.h"

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

/* intra */
typedef void (quant_intraFunc) (int16_t * coeff,
								const int16_t * data,
								const uint32_t quant,
								const uint32_t dcscalar);

typedef quant_intraFunc *quant_intraFuncPtr;

extern quant_intraFuncPtr quant4_intra;
extern quant_intraFuncPtr dequant4_intra;

quant_intraFunc quant4_intra_c;
quant_intraFunc quant4_intra_mmx;

quant_intraFunc dequant4_intra_c;
quant_intraFunc dequant4_intra_mmx;

/* inter_quant */
typedef uint32_t(quant_interFunc) (int16_t * coeff,
								   const int16_t * data,
								   const uint32_t quant);

typedef quant_interFunc *quant_interFuncPtr;

extern quant_interFuncPtr quant4_inter;

quant_interFunc quant4_inter_c;
quant_interFunc quant4_inter_mmx;

/* inter_dequant */
typedef void (dequant_interFunc) (int16_t * coeff,
								  const int16_t * data,
								  const uint32_t quant);

typedef dequant_interFunc *dequant_interFuncPtr;

extern dequant_interFuncPtr dequant4_inter;

dequant_interFunc dequant4_inter_c;
dequant_interFunc dequant4_inter_mmx;

#endif /* _QUANT_MPEG4_H_ */
