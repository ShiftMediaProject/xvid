/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Decoder header -
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
 * $Id: decoder.h,v 1.11 2002-11-16 23:38:16 edgomez Exp $
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

	XVID_DEC_PICTURE* out_frm;                // This is used for slice rendering
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
