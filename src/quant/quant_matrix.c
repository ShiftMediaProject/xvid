/**************************************************************************
 *
 *    XVID MPEG-4 VIDEO CODEC
 *    mpeg-4 quantization matrix stuff
 *
 *    This program is an implementation of a part of one or more MPEG-4
 *    Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *    to use this software module in hardware or software products are
 *    advised that its use may infringe existing patents or copyrights, and
 *    any such use would be at such party's own risk.  The original
 *    developer of this software module and his/her company, and subsequent
 *    editors and their companies, will have no liability for use of this
 *    software or modifications or derivatives thereof.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *    History:
 *
 *	2.3.2002    inital version <suxen_drol@hotmail.com>
 *
 *************************************************************************/

#include "common.h"


static const int16_t default_intra_matrix[64] = {
     8,17,18,19,21,23,25,27,
    17,18,19,21,23,25,27,28,
    20,21,22,23,24,26,28,30,
    21,22,23,24,26,28,30,32,
    22,23,24,26,28,30,32,35,
    23,24,26,28,30,32,35,38,
    25,26,28,30,32,35,38,41,
    27,28,30,32,35,38,41,45
};

static const int16_t default_inter_matrix[64] = {
    16,17,18,19,20,21,22,23,
    17,18,19,20,21,22,23,24,
    18,19,20,21,22,23,24,25,
    19,20,21,22,23,24,26,27,
    20,21,22,23,25,26,27,28,
    21,22,23,24,26,27,28,30,
    22,23,24,26,27,28,30,31,
    23,24,25,27,28,30,31,33
};


void quant4_intra_init(QMATRIX * qmatrix, int use_default)
{
	if (use_default)
	{
		memcpy(qmatrix->intra, default_intra_matrix, 64 * sizeof (int16_t));
	}

#ifdef ARCH_X86
	// TODO: generate mmx tables
#endif

}


void quant4_inter_init(QMATRIX * qmatrix, int use_default)
{
	if (use_default)
	{
		memcpy(qmatrix->inter, default_inter_matrix, 64 * sizeof (int16_t));
	}

#ifdef ARCH_X86
	// TODO: generate mmx tables
#endif
}
