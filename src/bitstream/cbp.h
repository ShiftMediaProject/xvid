/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - cbp function (zero block flags) -
 *
 *  Copyright(C) 2001-2002  Michael Militzer <isibaar@xvid.org>
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
 * $Id: cbp.h,v 1.7 2002-09-10 22:57:18 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ENCODER_CBP_H_
#define _ENCODER_CBP_H_

#include "../portab.h"

/*****************************************************************************
 * Function type
 ****************************************************************************/

typedef uint32_t(cbpFunc) (const int16_t * codes);

typedef cbpFunc *cbpFuncPtr;

/*****************************************************************************
 * Global Function pointer
 ****************************************************************************/

extern cbpFuncPtr calc_cbp;

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

extern cbpFunc calc_cbp_c;
extern cbpFunc calc_cbp_mmx;
extern cbpFunc calc_cbp_sse2;
extern cbpFunc calc_cbp_ppc;
extern cbpFunc calc_cbp_altivec;

#endif /* _ENCODER_CBP_H_ */
