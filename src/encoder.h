/**************************************************************************
 *
 *  Modifications:
 *
 *  22.08.2001 added support for EXT_MODE encoding mode
 *             support for EXTENDED API
 *  22.08.2001 fixed bug in iDQtab
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCODER_H_
#define _ENCODER_H_


#include "xvid.h"

#include "portab.h"
#include "global.h"
#include "image/image.h"


#define H263_QUANT	0
#define MPEG4_QUANT	1


typedef uint32_t bool;


typedef enum
{
    I_VOP = 0,
    P_VOP = 1,
	B_VOP = 2
}
VOP_TYPE;

/***********************************

       Encoding Parameters

************************************/ 

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

    /* rounding type; alternate 0-1 after each interframe */
	/* 1 <= fixed_code <= 4
	   automatically adjusted using motion vector statistics inside
	 */

	/* vars that not "quite" frame independant */
	uint32_t m_quant_type;
	uint32_t m_rounding_type;
	uint32_t m_fcode;

	HINTINFO * hint;

#ifdef BFRAMES
	uint32_t m_seconds;
	uint32_t m_ticks;
#endif

} MBParam;


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

	MACROBLOCK * mbs;

} FRAMEINFO;

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

	FRAMEINFO * current;
	FRAMEINFO * reference;

#ifdef _DEBUG
	IMAGE sOriginal;
#endif
    IMAGE vInterH;
    IMAGE vInterV;
	IMAGE vInterVf;
    IMAGE vInterHV;
	IMAGE vInterHVf;

#ifdef BFRAMES
	/* constants */
	int max_bframes;
	int bquant_ratio;
	/* vars */
	int bframenum_head;
	int bframenum_tail;
	int flush_bframes;

	FRAMEINFO ** bframes;
    IMAGE f_refh;
    IMAGE f_refv;
    IMAGE f_refhv;
#endif
    Statistics sStat;
}
Encoder;


// indicates no quantizer changes in INTRA_Q/INTER_Q modes
#define NO_CHANGE 64

void init_encoder(uint32_t cpu_flags);

int encoder_create(XVID_ENC_PARAM * pParam);
int encoder_destroy(Encoder * pEnc);
int encoder_encode(Encoder * pEnc, XVID_ENC_FRAME * pFrame, XVID_ENC_STATS * pResult);
		
static __inline uint8_t get_fcode(uint16_t sr)
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

#endif /* _ENCODER_H_ */
