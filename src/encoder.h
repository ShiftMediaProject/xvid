/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Encoder header -
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
 *  - 13.06.2002 Added legal header
 *  - 22.08.2001 Added support for EXT_MODE encoding mode
 *               support for EXTENDED API
 *  - 22.08.2001 fixed bug in iDQtab
 *
 *  $Id: encoder.h,v 1.27 2003-02-15 15:22:17 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ENCODER_H_
#define _ENCODER_H_

#include "xvid.h"
#include "portab.h"
#include "global.h"
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
	B_VOP = 2,
	S_VOP = 3
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

	/* constants */
	int global;
	int bquant_ratio;
	int bquant_offset;
	int frame_drop_ratio;

	int iMaxKeyInterval;
	int max_bframes;

	/* rounding type; alternate 0-1 after each interframe */
	/* 1 <= fixed_code <= 4
	   automatically adjusted using motion vector statistics inside
	 */

	/* vars that not "quite" frame independant */
	uint32_t m_quant_type;
	uint32_t m_rounding_type;
	uint32_t m_fcode;
	uint32_t m_quarterpel;
	int m_reduced_resolution;	/* reduced_resolution_enable */

	HINTINFO *hint;

	int64_t m_stamp;
}
MBParam;


typedef struct
{
	int iTextBits;
	int iMvSum;
	int iMvCount;
	int kblks;
	int mblks;
	int ublks;
	int gblks;
}
Statistics;


typedef struct
{
	uint32_t quant;
	uint32_t motion_flags;
	uint32_t global_flags;

	VOP_TYPE coding_type;
	uint32_t rounding_type;
	uint32_t quarterpel;
	uint32_t fcode;
	uint32_t bcode;

	uint32_t seconds;
	uint32_t ticks;
	int64_t stamp;

	IMAGE image;

	MACROBLOCK *mbs;

	WARPPOINTS warp;		// as in bitstream
	GMC_DATA gmc_data;		// common data for all MBs
		
	Statistics sStat;
}
FRAMEINFO;


typedef struct
{
	MBParam mbParam;

	int iFrameNum;
	int bitrate;

	// images

	FRAMEINFO *current;
	FRAMEINFO *reference;

	IMAGE sOriginal;
	IMAGE vInterH;
	IMAGE vInterV;
	IMAGE vInterVf;
	IMAGE vInterHV;
	IMAGE vInterHVf;

	IMAGE vGMC;

	/* image queue */
	int queue_head;
	int queue_tail;
	int queue_size;
	IMAGE *queue;

	/* bframe buffer */
	int bframenum_head;
	int bframenum_tail;
	int flush_bframes;

	FRAMEINFO **bframes;
	IMAGE f_refh;
	IMAGE f_refv;
	IMAGE f_refhv;
	int bframenum_dx50bvop;

	int m_framenum; /* debug frame num counter; unlike iFrameNum, does not reset at ivop */

	RateControl rate_control;

	float fMvPrevSigma;
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
