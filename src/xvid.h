/*****************************************************************************
*
*  XVID MPEG-4 VIDEO CODEC
*  - XviD Main header file -
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
*****************************************************************************/
/*****************************************************************************
*
*  History
*
*  - 2002/06/13 Added legal header, ANSI C comment style (only for this header
*               as it can be included in a ANSI C project).
*
*               ToDo ? : when BFRAMES is defined, the API_VERSION should not
*                        be the same (3.0 ?)
*
*  $Id: xvid.h,v 1.10 2002-06-13 12:42:18 edgomez Exp $
*
*****************************************************************************/


#ifndef _XVID_H_
#define _XVID_H_

#ifdef __cplusplus
*  $Id: xvid.h,v 1.10 2002-06-13 12:42:18 edgomez Exp $
#endif

/*****************************************************************************
 * Global constants
 ****************************************************************************/

/* API Version : 2.1 */
#define API_VERSION ((2 << 16) | (1))


/* Error codes */
#define XVID_ERR_FAIL		-1
#define XVID_ERR_OK			0
#define	XVID_ERR_MEMORY		1
#define XVID_ERR_FORMAT		2


/* Colorspaces */
#define XVID_CSP_RGB24 	0
#define XVID_CSP_YV12	1
#define XVID_CSP_YUY2	2
#define XVID_CSP_UYVY	3
#define XVID_CSP_I420	4
#define XVID_CSP_RGB555	10
#define XVID_CSP_RGB565	11
#define XVID_CSP_USER	12
#define XVID_CSP_EXTERN      1004  // per slice rendering
#define XVID_CSP_YVYU	1002
#define XVID_CSP_RGB32 	1000
#define XVID_CSP_NULL 	9999

#define XVID_CSP_VFLIP	0x80000000	// flip mask


/*****************************************************************************
 ****************************************************************************/

/* CPU flags for XVID_INIT_PARAM.cpu_flags */

#define XVID_CPU_MMX		0x00000001
#define XVID_CPU_MMXEXT		0x00000002
#define XVID_CPU_SSE		0x00000004 
#define XVID_CPU_SSE2		0x00000008
#define XVID_CPU_3DNOW		0x00000010
#define XVID_CPU_3DNOWEXT	0x00000020

#define XVID_CPU_TSC		0x00000040
#define XVID_CPU_IA64		0x00000080

#define XVID_CPU_CHKONLY	0x40000000		/* check cpu only; dont init globals */
#define XVID_CPU_FORCE		0x80000000


 *  Initialization structures
		int cpu_flags;
		int api_version;
		int core_build;
	}
	XVID_INIT_PARAM;

/*****************************************************************************
 *  Initialization entry point
 ****************************************************************************/

	int xvid_init(void *handle,
				  int opt,
				  void *param1,
				  void *param2);


/*****************************************************************************
 * Decoder constants
 ****************************************************************************/

/* Flags for XVID_DEC_FRAME.general */
#define XVID_QUICK_DECODE		0x00000010

/*****************************************************************************
 * Decoder structures
 ****************************************************************************/

	typedef struct
	{
		int width;
		int height;
		void *handle;
	}
	XVID_DEC_PARAM;


	typedef struct
	{
		int general;
		void *bitstream;
		int length;

		void *image;
		int stride;
		int colorspace;
	}
	XVID_DEC_FRAME;


	// This struct is used for per slice rendering
	typedef struct 
	{
		void *y,*u,*v;
		int stride_y, stride_u,stride_v;
	} XVID_DEC_PICTURE;

#define XVID_DEC_DESTROY	2

	int xvid_decore(void *handle,
					int opt,
					void *param1,
					void *param2);


/*****************************************************************************
 * Encoder constants
 ****************************************************************************/

/* Flags for XVID_ENC_PARAM.global */
#define XVID_GLOBAL_PACKED		0x00000001	/* packed bitstream */
#define XVID_GLOBAL_DX50BVOP	0x00000002	/* dx50 bvop compatibility */
#define XVID_GLOBAL_DEBUG		0x00000004	/* print debug info on each frame */

/* Flags for XVID_ENC_FRAME.general */
#define XVID_VALID_FLAGS		0x80000000

#define XVID_CUSTOM_QMATRIX		0x00000004	/* use custom quant matrix */
#define XVID_LATEINTRA			0x00000200

#define XVID_INTERLACING		0x00000400	/* enable interlaced encoding */
#define XVID_TOPFIELDFIRST		0x00000800	/* set top-field-first flag  */
#define XVID_ALTERNATESCAN		0x00001000	/* set alternate vertical scan flag */

#define XVID_HINTEDME_GET		0x00002000	/* receive mv hint data from core (1st pass) */
#define XVID_HINTEDME_SET		0x00004000	/* send mv hint data to core (2nd pass) */

#define XVID_INTER4V			0x00008000

#define XVID_ME_ZERO			0x00010000
#define XVID_ME_LOGARITHMIC		0x00020000
#define XVID_ME_FULLSEARCH		0x00040000
#define XVID_ME_PMVFAST			0x00080000
#define XVID_ME_EPZS			0x00100000


#define XVID_GREYSCALE			0x01000000	/* enable greyscale only mode (even for */
#define XVID_GRAYSCALE			0x01000000      /* color input material chroma is ignored) */


/* Flags for XVID_ENC_FRAME.motion */
#define PMV_ADVANCEDDIAMOND8	0x00004000
#define PMV_ADVANCEDDIAMOND16   0x00008000
#define PMV_EARLYSTOP16	   		0x00080000
#define PMV_QUICKSTOP16	   		0x00100000	/* like early, but without any more refinement */
#define PMV_UNRESTRICTED16   	0x00200000	/* unrestricted ME, not implemented */
#define PMV_OVERLAPPING16   	0x00400000	/* overlapping ME, not implemented */
#define PMV_USESQUARES16		0x00800000

#define PMV_HALFPELDIAMOND8 	0x01000000
#define PMV_HALFPELREFINE8 		0x02000000
#define PMV_EXTSEARCH8 			0x04000000	/* extend PMV by more searches */
#define PMV_EARLYSTOP8	   		0x08000000
#define PMV_QUICKSTOP8	   		0x10000000	/* like early, but without any more refinement */
#define PMV_UNRESTRICTED8   	0x20000000	/* unrestricted ME, not implemented */
#define PMV_OVERLAPPING8   		0x40000000	/* overlapping ME, not implemented */
#define PMV_USESQUARES8			0x80000000


/*****************************************************************************
 * Encoder structures
 ****************************************************************************/

	typedef struct
	{
		int width, height;
		int fincr, fbase;		/* frame increment, fbase. each frame = "fincr/fbase" seconds */
		int rc_bitrate;			/* the bitrate of the target encoded stream, in bits/second */
		int rc_reaction_delay_factor;	/* how fast the rate control reacts - lower values are faster */
		int rc_averaging_period;	/* as above */
		int rc_buffer;			/* as above */
		int max_quantizer;		/* the upper limit of the quantizer */
		int min_quantizer;		/* the lower limit of the quantizer */
		int max_key_interval;	/* the maximum interval between key frames */
#ifdef _SMP
		int num_threads;		/* number of threads */
#endif
#ifdef BFRAMES
		int global;				/* global/debug options */
		int max_bframes;		/* max sequential bframes (0=disable bframes) */
		int bquant_ratio;		/* bframe quantizer multipier (percentage).
								 * used only when bquant < 1
								 * eg. 200 = x2 multiplier
#endif
	}
	XVID_ENC_PARAM;

	typedef struct
	{
		int x;
	}
	VECTOR;

	typedef struct
	{
		int mode;				/* macroblock mode */
		VECTOR mvs[4];
	}
	MVBLOCKHINT;

	typedef struct
	{
		int intra;				/* frame intra choice */
		int fcode;				/* frame fcode */
		MVBLOCKHINT *block;		/* caller-allocated array of block hints (mb_width * mb_height) */
	}
	MVFRAMEHINT;

	typedef struct
	{
		int rawhints;			/* if set, use MVFRAMEHINT, else use compressed buffer */

		MVFRAMEHINT mvhint;
		void *hintstream;		/* compressed hint buffer */
		int hintlength;			/* length of buffer (bytes) */
	}
	HINTINFO;

	typedef struct
	{
		int general;			/* [in] general options */
		int motion;				/* [in] ME options */
		void *bitstream;		/* [in] bitstream ptr */
		int length;				/* [out] bitstream length (bytes) */

		void *image;			/* [in] image ptr */
		int colorspace;			/* [in] source colorspace */

		unsigned char *quant_intra_matrix;	// [in] custom intra qmatrix */
		unsigned char *quant_inter_matrix;	// [in] custom inter qmatrix */
		int quant;				/* [in] frame quantizer (vbr) */
		int intra;				/* [in] force intra frame (vbr only)
								 * [out] intra state
								 */
		HINTINFO hint;			/* [in/out] mv hint information */

#ifdef BFRAMES
		int bquant;				/* [in] bframe quantizer */
#endif

	}
	XVID_ENC_FRAME;


	typedef struct
	{
		int quant;				/* [out] frame quantizer */
		int input_consumed;		/* [out] */
		int hlength;			/* [out] header length (bytes) */
		int kblks, mblks, ublks;	/* [out] */

	}
	XVID_ENC_STATS;


/*****************************************************************************
 * Encoder entry point
 ****************************************************************************/

/* Encoder options */
#define XVID_ENC_ENCODE		0
#define XVID_ENC_CREATE		1
#define XVID_ENC_DESTROY	2

	int xvid_encore(void *handle,
					int opt,
					void *param1,
					void *param2);


#ifdef __cplusplus
}
#endif

#endif
