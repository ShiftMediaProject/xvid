/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - cbp function (zero block flags) -
 *
 *  Copyright (C) 2001-2002 - Peter Ross <pross@cs.rmit.edu.au>
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
 * $Id: cbp.c,v 1.6 2002-09-10 22:29:18 edgomez Exp $
 *
 ****************************************************************************/

#include "../portab.h"
#include "cbp.h"

/*****************************************************************************
 * Global function pointer
 ****************************************************************************/

cbpFuncPtr calc_cbp;

/*****************************************************************************
 * Functions
 ****************************************************************************/

/*
 * Returns a field of bits that indicates non zero ac blocks
 * for this macro block
 */
uint32_t
calc_cbp_c(const int16_t codes[6 * 64])
{
	uint32_t i, j;
	uint32_t cbp = 0;

	for (i = 0; i < 6; i++) {
		for (j = 1; j < 61; j += 4) {
			if (codes[i * 64 + j] | codes[i * 64 + j + 1] |
				codes[i * 64 + j + 2] | codes[i * 64 + j + 3]) {
				cbp |= 1 << (5 - i);
				break;
			}
		}

		if (codes[i * 64 + j] | codes[i * 64 + j + 1] | codes[i * 64 + j + 2])
			cbp |= 1 << (5 - i);

	}

	return cbp;

}
