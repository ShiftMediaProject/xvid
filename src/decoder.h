/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Decoder header -
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
 ****************************************************************************/
/*****************************************************************************
 *
 *  History
 *
 *  - 13.06.2002 Added legal header - Cosmetic
 *
 *  $Id: decoder.h,v 1.9 2002-07-09 01:37:22 chenm001 Exp $
 *
 ****************************************************************************/

#ifndef _DECODER_H_
#define _DECODER_H_

#include "xvid.h"
#include "portab.h"
#include "global.h"
#include "image/image.h"

/*****************************************************************************
 * Structures
 ****************************************************************************/

typedef struct
{
	// bitstream

	uint32_t shape;
	uint32_t time_inc_bits;
	uint32_t quant_bits;
	uint32_t quant_type;
	uint32_t quarterpel;

	uint32_t interlacing;
	uint32_t top_field_first;
	uint32_t alternate_vertical_scan;

	// image

	uint32_t width;
	uint32_t height;
	uint32_t edged_width;
	uint32_t edged_height;

	IMAGE cur;
	IMAGE refn[3];				// 0   -- last I or P VOP
	// 1   -- first I or P
	// 2   -- for interpolate mode B-frame
	IMAGE refh;
	IMAGE refv;
	IMAGE refhv;

	// macroblock

	uint32_t mb_width;
	uint32_t mb_height;
	MACROBLOCK *mbs;

	// for B-frame
	int32_t frames;				// total frame number
	int8_t scalability;
	VECTOR p_fmv, p_bmv;		// pred forward & backward motion vector
	MACROBLOCK *last_mbs;		// last MB
	int64_t time;				// for record time
	int64_t time_base;
	int64_t last_time_base;
	int64_t last_non_b_time;
	uint32_t time_pp;
	uint32_t time_bp;
	uint8_t low_delay;			// low_delay flage (1 means no B_VOP)
}
DECODER;

/*****************************************************************************
 * Decoder prototypes
 ****************************************************************************/

void init_decoder(uint32_t cpu_flags);

int decoder_create(XVID_DEC_PARAM * param);
int decoder_destroy(DECODER * dec);
int decoder_decode(DECODER * dec,
				   XVID_DEC_FRAME * frame);


#endif
