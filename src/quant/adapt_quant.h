/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Adaptive quantization header file -
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
 * $Id: adapt_quant.h,v 1.5 2002-10-19 11:41:11 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ADAPT_QUANT_H_
#define _ADAPT_QUANT_H_

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int adaptive_quantization(unsigned char *buf,
						  int stride,
						  int *intquant,
						  int framequant,
						  int min_quant,
						  int max_quant,
						  int mb_width,
						  int mb_height);	// no stride because of normalization

#endif /* _ADAPT_QUANT_H_ */
