/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Encoder header -
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
 * $Id: encoder.h,v 1.24 2002-11-16 23:38:16 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "xvid.h"
#include "portab.h"
#include "global.h"
#include "image/image.h"
#include "utils/ratecontrol.h"

/*****************************************************************************
 * Constants
 ****************************************************************************/

/* Quatization type */
#define H263_QUANT	0
#define MPEG4_QUANT	1

/* Indicates no quantizer changes in INTRA_Q/INTER_Q modes */
#define NO_CHANGE 64

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef int bool;

typedef enum
{
	I_VOP = 0,
	P_VOP = 1,
	B_VOP = 2
}
VOP_TYPE;

/*****************************************************************************
 * Structures
 ****************************************************************************/

typedef struct
{
	uint32_t width;
	uint32_t height;

	uint32_t edged_width;
	uint32_t edged_height;
	uint32_t mb_width;
	uint32_t mb_height;

	/* frame rate increment & base */
	uint32_t fincr;
	uint32_t fbase;

#ifdef _SMP
	int num_threads;
#endif

	/* rounding type; alternate 0-1 after each interframe */
	/* 1 <= fixed_code <= 4
	   automatically adjusted using motion vector statistics inside
	 */

	/* vars that not "quite" frame independant */
	uint32_t m_quant_type;
	uint32_t m_rounding_type;
	uint32_t m_fcode;

	HINTINFO *hint;

	uint32_t m_seconds;
	uint32_t m_ticks;

}
MBParam;


typedef struct
{
	uint32_t quant;
	uint32_t motion_flags;
	uint32_t global_flags;

	VOP_TYPE coding_type;
	uint32_t rounding_type;
	uint32_t fcode;
	uint32_t bcode;

	uint32_t seconds;
	uint32_t ticks;

	IMAGE image;

	MACROBLOCK *mbs;

}
FRAMEINFO;

typedef struct
{
	int iTextBits;
	float fMvPrevSigma;
	int iMvSum;
	int iMvCount;
	int kblks;
	int mblks;
	int ublks;
}
Statistics;



typedef struct
{
	MBParam mbParam;

	int iFrameNum;
	int iMaxKeyInterval;
	int bitrate;

	// images

	FRAMEINFO *current;
	FRAMEINFO *reference;

#ifdef _DEBUG_PSNR
	IMAGE sOriginal;
#endif
	IMAGE vInterH;
	IMAGE vInterV;
	IMAGE vInterHV;

	Statistics sStat;
	RateControl rate_control;
}
Encoder;

/*****************************************************************************
 * Inline functions
 ****************************************************************************/

static __inline uint8_t
get_fcode(uint16_t sr)
{
	if (sr <= 16)
		return 1;

	else if (sr <= 32)
		return 2;

	else if (sr <= 64)
		return 3;

	else if (sr <= 128)
		return 4;

	else if (sr <= 256)
		return 5;

	else if (sr <= 512)
		return 6;

	else if (sr <= 1024)
		return 7;

	else
		return 0;
}


/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void init_encoder(uint32_t cpu_flags);

int encoder_create(XVID_ENC_PARAM * pParam);
int encoder_destroy(Encoder * pEnc);
int encoder_encode(Encoder * pEnc,
				   XVID_ENC_FRAME * pFrame,
				   XVID_ENC_STATS * pResult);

int encoder_encode_bframes(Encoder * pEnc,
				   XVID_ENC_FRAME * pFrame,
				   XVID_ENC_STATS * pResult);

#endif
