/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - h263 quantization header -
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
 *  $Id: quant_h263.h,v 1.8 2002-10-19 11:41:11 edgomez Exp $
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
