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
 *  $Id: encoder.h,v 1.11 2002-06-20 14:05:57 suxen_drol Exp $
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

#ifdef BFRAMES
	int max_bframes;
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

#ifdef BFRAMES
	uint32_t m_seconds;
	uint32_t m_ticks;
#endif

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

#ifdef BFRAMES
	uint32_t seconds;
	uint32_t ticks;
#endif

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
	IMAGE vInterVf;
	IMAGE vInterHV;
	IMAGE vInterHVf;

#ifdef BFRAMES
	/* constants */
	int packed;
	int bquant_ratio;

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
#endif

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
