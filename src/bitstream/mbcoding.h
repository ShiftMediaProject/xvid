/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Macro Block coding functions -
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
 * $Id: mbcoding.h,v 1.10 2002-11-17 00:57:57 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _MB_CODING_H_
#define _MB_CODING_H_

#include "../portab.h"
#include "../global.h"
#include "bitstream.h"

void init_vlc_tables(void);

int check_resync_marker(Bitstream * bs, int addbits);

int get_mcbpc_intra(Bitstream * bs);
int get_mcbpc_inter(Bitstream * bs);
int get_cbpy(Bitstream * bs,
			 int intra);
int get_mv(Bitstream * bs,
		   int fcode);

int get_dc_dif(Bitstream * bs,
			   uint32_t dc_size);
int get_dc_size_lum(Bitstream * bs);
int get_dc_size_chrom(Bitstream * bs);

void get_intra_block(Bitstream * bs,
					 int16_t * block,
					 int direction,
					 int coeff);
void get_inter_block(Bitstream * bs,
					 int16_t * block);

#endif /* _MB_CODING_H_ */
