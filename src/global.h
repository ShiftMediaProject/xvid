/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Global structures, constants -
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
 * $Id: global.h,v 1.18 2002-12-15 01:21:12 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "xvid.h"
#include "portab.h"

/* --- macroblock modes --- */

#define MODE_INTER		0
#define MODE_INTER_Q	1
#define MODE_INTER4V	2
#define	MODE_INTRA		3
#define MODE_INTRA_Q	4
#define MODE_STUFFING	7
#define MODE_NOT_CODED	16

/* --- bframe specific --- */

#define MODE_DIRECT			0
#define MODE_INTERPOLATE	1
#define MODE_BACKWARD		2
#define MODE_FORWARD		3
#define MODE_DIRECT_NONE_MV	4


typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
}
Bitstream;


#define MBPRED_SIZE  15


typedef struct
{
	/* decoder/encoder  */
	VECTOR mvs[4];

	short int pred_values[6][MBPRED_SIZE];
	int acpred_directions[6];

	int mode;
	int quant;					/* absolute quant */

	int field_dct;
	int field_pred;
	int field_for_top;
	int field_for_bot;

	/* encoder specific */

	VECTOR mv16;
	VECTOR pmvs[4];

	int32_t sad8[4];			/* SAD values for inter4v-VECTORs */
	int32_t sad16;				/* SAD value for inter-VECTOR */

	int dquant;
	int cbp;

	/* bframe stuff */

	VECTOR b_mvs[4];
	VECTOR b_pmvs[4];

	/* bframe direct mode */

	VECTOR directmv[4];
	VECTOR deltamv;

	int mb_type;
	int dbquant;

	/* stuff for block based ME (needed for Qpel ME) */
	/* backup of last integer ME vectors/sad */
	
	VECTOR i_mv16;
	VECTOR i_mvs[4];

	int32_t i_sad8[4];	/* SAD values for inter4v-VECTORs */
	int32_t i_sad16;	/* SAD value for inter-VECTOR */

}
MACROBLOCK;

static __inline int8_t
get_dc_scaler(uint32_t quant,
			  uint32_t lum)
{
	if (quant < 5)
		return 8;

	if (quant < 25 && !lum)
		return (int8_t)((quant + 13) / 2);

	if (quant < 9)
		return (int8_t)(2 * quant);

	if (quant < 25)
		return (int8_t)(quant + 8);

	if (lum)
		return (int8_t)(2 * quant - 16);
	else
		return (int8_t)(quant - 6);
}

/* useful macros */

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X)    (((X)>0)?(X):-(X))
#define SIGN(X)   (((X)>0)?1:-1)


#endif /* _GLOBAL_H_ */
