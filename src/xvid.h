/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - XviD Main header file -
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
 * $Id: xvid.h,v 1.24 2002-11-26 23:44:10 edgomez Exp $
 *
 *****************************************************************************/

#ifndef _XVID_H_
#define _XVID_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \defgroup global_grp   Global constants used in both encoder and decoder.
 *
 * This module describe all constants used in both the encoder and the decoder.
 * @{
 */

/*****************************************************************************
 *  API version number
 ****************************************************************************/

/**
 * \defgroup api_grp API version
 * @{
 */

#define API_VERSION ((2 << 16) | (1))/**< This constant tells you what XviD's
                                      *   version this header defines.
				      *
 * You can use it to check if the host XviD library API is the same as the one
 * you used to build you client program. If versions mismatch, then it is
 * highly possible that your application will segfault because the host XviD
 * library and your application use different structures.
 * 
 */

/** @} */


/*****************************************************************************
 *  Error codes
 ****************************************************************************/


/**
 * \defgroup error_grp Error codes returned by XviD API entry points.
 * @{
 */

#define XVID_ERR_FAIL   -1 /**< Operation failed.
			    *
 * The requested XviD operation failed. If this error code is returned from :
 * <ul>
 * <li>the xvid_init function : you must not try to use an XviD's instance from
 *                              this point of the code. Clean all instances you
 *                              already created and exit the program cleanly.
 * <li>xvid_encore or xvid_decore : something was wrong and en/decoding
 *                                  operation was not completed sucessfully.
 *                                  you can stop the en/decoding process or just 
 *									ignore and go on.
 * <li>xvid_stop : you can safely ignore it if you call this function at the
 *                 end of your program.
 * </ul>
 */

#define XVID_ERR_OK      0 /**< Operation succeed.
			    *
 * The requested XviD operation succeed, you can continue to use XviD's
 * functions.
 */

#define XVID_ERR_MEMORY  1 /**< Operation failed.
			    *
 * Insufficent memory was available on the host system.
 */

#define XVID_ERR_FORMAT  2 /**< Operation failed.
			    *
 * The format of the parameters or input stream were incorrect.
 */

/** @} */


/*****************************************************************************
 *  Color space constants
 ****************************************************************************/


/**
 * \defgroup csp_grp Colorspaces constants.
 * @{
 */

#define XVID_CSP_RGB24  0  /**< 24-bit RGB colorspace (b,g,r packed) */
#define XVID_CSP_YV12   1  /**< YV12 colorspace (y,v,u planar) */
#define XVID_CSP_YUY2   2  /**< YUY2 colorspace (y,u,y,v packed) */
#define XVID_CSP_UYVY   3  /**< UYVY colorspace (u,y,v,y packed) */
#define XVID_CSP_I420   4  /**< I420 colorsapce (y,u,v planar) */
#define XVID_CSP_RGB555 10 /**< 16-bit RGB555 colorspace */
#define XVID_CSP_RGB565 11 /**< 16-bit RGB565 colorspace */
#define XVID_CSP_USER   12 /**< user colorspace format, where the image buffer points
                            *   to a DEC_PICTURE (y,u,v planar) structure.
							*
							*   For encoding, image is read from the DEC_PICTURE
							*   parameter values. For decoding, the DEC_PICTURE 
                            *   parameters are set, pointing to the internal XviD
                            *   image buffer. */
#define XVID_CSP_EXTERN 1004 /**< Special colorspace used for slice rendering
                              *
                              * The application provides an external buffer to XviD.
                              * This way, XviD works directly into the final rendering
                              * buffer, no need to specify this is a speed boost feature.
                              * This feature is only used by mplayer at the moment, refer
                              * to mplayer code to see how it can be used. */
#define XVID_CSP_YVYU   1002 /**< YVYU colorspace (y,v,y,u packed) */
#define XVID_CSP_RGB32  1000 /**< 32-bit RGB colorspace (b,g,r,a packed) */
#define XVID_CSP_NULL   9999 /**< NULL colorspace; no conversion is performed */

#define XVID_CSP_VFLIP  0x80000000 /**< (flag) Flip frame vertically during conversion */

/** @} */

/** @} */

/**
 * \defgroup init_grp Initialization constants, structures and functions.
 *
 * This section describes all the constants, structures and functions used to
 * initialize the XviD core library
 *
 * @{
 */


/*****************************************************************************
 *  CPU flags
 ****************************************************************************/


/**
 * \defgroup cpu_grp Flags for XVID_INIT_PARAM.cpu_flags.
 *
 * This section describes all constants that show host cpu available features,
 * and allow a client application to force usage of some cpu instructions sets.
 * @{
 */


/**
 * \defgroup x86_grp x86 specific cpu flags
 * @{
 */

#define XVID_CPU_MMX      0x00000001 /**< use/has MMX instruction set */
#define XVID_CPU_MMXEXT   0x00000002 /**< use/has MMX-ext (pentium3) instruction set */
#define XVID_CPU_SSE      0x00000004 /**< use/has SSE (pentium3) instruction set */
#define XVID_CPU_SSE2     0x00000008 /**< use/has SSE2 (pentium4) instruction set */
#define XVID_CPU_3DNOW    0x00000010 /**< use/has 3dNOW (k6-2) instruction set */
#define XVID_CPU_3DNOWEXT 0x00000020 /**< use/has 3dNOW-ext (athlon) instruction set */
#define XVID_CPU_TSC      0x00000040 /**< has TimeStampCounter instruction */

/** @} */

/**
 * \defgroup ia64_grp ia64 specific cpu flags.
 * @{
 */

#define XVID_CPU_IA64     0x00000080 /**< Forces ia64 optimized code usage
 *
 * This flags allow client applications to force IA64 optimized functions.
 * This feature is considered exeperimental and should be treated as is.
 */

/** @} */

/**
 * \defgroup iniflags_grp Initialization commands.
 *
 * @{
 */

#define XVID_CPU_CHKONLY  0x40000000 /**< Check cpu features
				      *
 * When this flag is set, the xvid_init function performs just a cpu feature
 * checking and then fills the cpu field. This flag is usefull when client
 * applications want to know what instruction sets the host cpu supports.
 */

#define XVID_CPU_FORCE    0x80000000 /**< Force input flags to be used
				      *
 * When this flag is set, client application forces XviD to use other flags
 * set in cpu_flags. \b Use this at your own risk.
 */

/** @} */

/** @} */

/*****************************************************************************
 *  Initialization structures
 ****************************************************************************/

/** Structure used in xvid_init function. */
	typedef struct
	{
		int cpu_flags;   /**< [in/out]
				  *
				  * Filled with desired[in] or available[out]
				  * cpu instruction sets.
				  */
		int api_version; /**< [out]
				  *
				  * xvid_init will initialize this field with
				  * the API_VERSION used in this XviD core
				  * library
				  */
		int core_build;  /**< [out]
				  * \todo Unused.
				  */
	}
	XVID_INIT_PARAM;

/*****************************************************************************
 *  Initialization entry point
 ****************************************************************************/

/**
 * \defgroup inientry_grp Initialization entry point.
 * @{
 */

/**
 * \brief Initialization entry point.
 *
 * This is the XviD's initialization entry point, it is only used to initialize
 * the XviD internal data (function pointers, vector length code tables,
 * rgb2yuv lookup tables).
 *
 * \param handle Reserved for future use.
 * \param opt Reserved for future use (set it to 0).
 * \param param1 Used to pass an XVID_INIT_PARAM parameter.
 * \param param2 Reserved for future use.
 */
	int xvid_init(void *handle,
		      int opt,
		      void *param1,
		      void *param2);

/** @} */

/** @} */

/*****************************************************************************
 * Decoder constant
 ****************************************************************************/

/**
 * \defgroup decoder_grp Decoder related functions and structures.
 *
 *  This part describes all the structures/functions from XviD's API needed for
 *  decoding a MPEG4 compliant streams.
 *  @{
 */

/**
 * \defgroup decframe_grp Flags for XVID_DEC_FRAME.general
 *
 * Flags' description for the XVID_DEC_FRAME.general member.
 *
 * @{
 */

/** Not used at the moment */
#define XVID_QUICK_DECODE		0x00000010


/**
 * @}
 */

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


	/* This struct is used for per slice rendering */
	typedef struct 
	{
		void *y,*u,*v;
		int stride_y, stride_u,stride_v;
	} XVID_DEC_PICTURE;


/*****************************************************************************
 * Decoder entry point
 ****************************************************************************/

/**
 * \defgroup  decops_grp Decoder operations
 *
 * These are all the operations XviD's decoder can perform.
 *
 * @{
 */

#define XVID_DEC_DECODE		0 /**< Decodes a frame
				   *
 * This operation constant is used when client application wants to decode a
 * frame. Client application must also fill XVID_DEC_FRAME appropriately.
 */

#define XVID_DEC_CREATE		1 /**< Creates a decoder instance
				   *
 * This operation constant is used by a client application in order to create
 * a decoder instance. Decoder instances are independant from each other, and
 * can be safely threaded.
 */

#define XVID_DEC_DESTROY	2 /**< Destroys a decoder instance
				   *
 * This operation constant is used by the client application to destroy a
 * previously created decoder instance.
 */

/**
 * @}
 */

/**
 * \defgroup  decentry_grp Decoder entry point
 *
 * @{
 */

/**
 * \brief Decoder entry point.
 *
 * This is the XviD's decoder entry point. The possible operations are
 * described in the \ref decops_grp section.
 *
 * \param handle Decoder instance handle.
 * \param opt Decoder option constant
 * \param param1 Used to pass a XVID_DEC_PARAM or XVID_DEC_FRAME structure
 * \param param2 Reserved for future use.
 */

	int xvid_decore(void *handle,
			int opt,
			void *param1,
			void *param2);

/** @} */

/** @} */

/**
 * \defgroup encoder_grp Encoder related functions and structures.
 *
 * @{
 */

/*****************************************************************************
 * Encoder constants
 ****************************************************************************/

/**
 * \defgroup encgenflags_grp  Flags for XVID_ENC_FRAME.general
 * @{
 */

#define XVID_VALID_FLAGS		0x80000000 /**< Reserved for future use */

#define XVID_CUSTOM_QMATRIX		0x00000004 /**< Use custom quantization matrices
											*
 * This flag forces XviD to use custom matrices passed to encoder in
 * XVID_ENC_FRAME structure (members quant_intra_matrix and quant_inter_matrix) */
#define XVID_H263QUANT			0x00000010 /**< Use H263 quantization
											*
 * This flag forces XviD to use H263  quantization type */
#define XVID_MPEGQUANT			0x00000020 /**< Use MPEG4 quantization.
											*
 * This flag forces XviD to use MPEG4 quantization type */
#define XVID_HALFPEL			0x00000040 /**< Halfpel motion estimation
											*
* informs xvid to perform a half pixel  motion estimation. */
#define XVID_ADAPTIVEQUANT		0x00000080/**< Adaptive quantization
											*
* informs xvid to perform an adaptative quantization using a Luminance
* masking algorithm */
#define XVID_LUMIMASKING		0x00000100/**< Lumimasking flag
											*
											* \deprecated This flag is no longer used. */
#define XVID_LATEINTRA			0x00000200/**< Unknown
											*
											* \deprecated This flag is no longer used. */
#define XVID_INTERLACING		0x00000400/**< MPEG4 interlacing mode.
											*
											* Enables interlacing encoding mode */
#define XVID_TOPFIELDFIRST		0x00000800/**< Unknown
                                            *
											* \deprecated This flag is no longer used. */
#define XVID_ALTERNATESCAN		0x00001000/**<
											*
											* \deprecated This flag is no longer used. */
#define XVID_HINTEDME_GET		0x00002000/**< Gets Motion vector data from ME system.
											*
* informs  xvid to  return  Motion Estimation vectors from the ME encoder
* algorithm. Used during a first pass. */
#define XVID_HINTEDME_SET		0x00004000/**< Gives Motion vectors hint to ME system.
											*
* informs xvid to  use the user  given motion estimation vectors as hints
* for the encoder ME algorithms. Used during a 2nd pass. */
#define XVID_INTER4V			0x00008000/**< Inter4V mode.
											*
* forces XviD to search a vector for each 8x8 block within the 16x16  Macro
* Block. This mode should  be used only if the XVID_HALFPEL mode is  activated
* (this  could change  in the future). */
#define XVID_ME_ZERO			0x00010000/**< Unused
											*
* Do not use this flag (reserved for future use) */
#define XVID_ME_LOGARITHMIC		0x00020000/**< Unused
											*
* Do not use this flag (reserved for future use) */
#define XVID_ME_FULLSEARCH		0x00040000/**< Unused
											*
* Do not use this flag (reserved for future use) */
#define XVID_ME_PMVFAST			0x00080000/**< Use PMVfast ME algorithm.
											*
* Switches XviD ME algorithm to PMVfast */
#define XVID_ME_EPZS			0x00100000/**< Use EPZS ME algorithm.
											*
* Switches XviD ME algorithm to EPZS */
#define XVID_GREYSCALE			0x01000000/**< Discard chroma data.
											*
* This flags forces XviD to discard chroma data, this is not mpeg4 greyscale
* mode, it simply drops chroma MBs using cbp == 0 for these blocks */
#define XVID_GRAYSCALE			XVID_GREYSCALE /**< XVID_GREYSCALE alias 
											*
* United States locale support. */

/** @} */

/**
 * \defgroup encmotionflags_grp  Flags for XVID_ENC_FRAME.motion
 * @{
 */

#define PMV_ADVANCEDDIAMOND8	0x00004000/**< Uses advanced diamonds for 8x8 blocks
											*
* Same as its 16x16 companion option
*/
#define PMV_ADVANCEDDIAMOND16   0x00008000/**< Uses advanced diamonds for 16x16 blocks
											*
* */
#define PMV_HALFPELDIAMOND16 	0x00010000/**< Turns on halfpel precision for 16x16 blocks
											*
* switches the search algorithm from 1 or 2 full pixels precision to 1 or 2 half pixel precision.
*/
#define PMV_HALFPELREFINE16 	0x00020000/**< Turns on halfpel refinement step
											*
* After normal diamond search, an extra halfpel refinement step is  performed. Should always be used if
* XVID_HALFPEL is on, because it gives a rather big increase in quality.
*/
#define PMV_EXTSEARCH16 		0x00040000/**< Extends search for 16x16 blocks
											*
* Normal PMVfast predicts one  start vector and  does diamond search around this position. EXTSEARCH means that 2
* more  start vectors  are used:  (0,0) and  median  predictor and diamond search  is done for  those, too.  Makes
* search slightly slower, but quality sometimes gets better.
*/
#define PMV_EARLYSTOP16	   		0x00080000/**< Dynamic ME thresholding
											*
* PMVfast and EPZS stop search  if current best is  below some dynamic  threshhold. No  diamond search  is done,
* only halfpel  refinement (if active).  Without EARLYSTOP diamond search is always done. That would be much slower,
* but not really lead to better quality.
*/
#define PMV_QUICKSTOP16	   		0x00100000/**< Dynamic ME thresholding
											*
* like  EARLYSTOP, but not even halfpel refinement is  done. Normally worse quality, so it defaults to
* off. Might be removed, too.
*/
#define PMV_UNRESTRICTED16   	0x00200000/**< Not implemented
											*
* "unrestricted  ME"   is  a   feature  of MPEG4. It's not  implemented, so this flag is  ignored (not even
* checked).
*/
#define PMV_OVERLAPPING16   	0x00400000/**< Not implemented
											*
* Same as above
*/
#define PMV_USESQUARES16		0x00800000/**< Use square pattern
											*
* Replace  the  diamond search  with a  square search. 
*/
#define PMV_HALFPELDIAMOND8 	0x01000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_HALFPELREFINE8 		0x02000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_EXTSEARCH8 			0x04000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_EARLYSTOP8	   		0x08000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_QUICKSTOP8	   		0x10000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_UNRESTRICTED8   	0x20000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_OVERLAPPING8   		0x40000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */
#define PMV_USESQUARES8			0x80000000/**< see 16x16 equivalent
											*
* Same as its 16x16 companion option */

/** @} */

/*****************************************************************************
 * Encoder structures
 ****************************************************************************/

	/** Structure used for encoder instance creation */
	typedef struct
	{
		int width;                    /**< [in]
									   *
									   * Input frame width. */
		int height;                   /**< [in]
									   *
									   * Input frame height. */
		int fincr;                    /**< [in]
									   *
									   * Time increment (fps = increment/base). */
		int fbase;                    /**< [in]
									   *
									   * Time base (fps = increment/base). */
		int rc_bitrate;               /**< [in]
									   *
									   * Sets the target bitrate of the encoded stream, in bits/second. **/
		int rc_reaction_delay_factor; /**< [in]
									   *
									   * Tunes how fast the rate control reacts - lower values are faster. */
		int rc_averaging_period;      /**< [in]
									   *
									   * Tunes how fast the rate control reacts - lower values are faster. */
		int rc_buffer;	              /**< [in]
									   *
									   * Tunes how fast the rate control reacts - lower values are faster. */
		int max_quantizer;            /**< [in]
									   *
									   * Sets the upper limit of the quantizer. */
		int min_quantizer;            /**< [in]
									   *
									   * Sets the lower limit of the quantizer. */
		int max_key_interval;         /**< [in]
									   *
									   * Sets the maximum interval between key frames. */
		void *handle;                 /**< [out]
									   *
									   * XviD core lib will set this with the creater encoder instance. */
	}
	XVID_ENC_PARAM;

	typedef struct
	{
		int x;
		int y;
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

	/** Structure used to pass a frame to the encoder */
	typedef struct
	{
		int general;                       /**< [in]
											*
											* Sets general options flag (See \ref encgenflags_grp) */
		int motion;                        /**< [in]
											*
											* Sets Motion Estimation options */
		void *bitstream;                   /**< [out]
											*
											* Output MPEG4 bitstream buffer pointer */
		int length;                        /**< [out]
											*
											* Output MPEG4 bitstream length (bytes) */
		void *image;                       /**< [in]
											*
											* Input frame */
		int colorspace;                    /**< [in]
											*
											* input frame colorspace */
		unsigned char *quant_intra_matrix; /**< [in]
											*
											* Custom intra quantization matrix */
		unsigned char *quant_inter_matrix; /**< [in]
											*
											* Custom inter quantization matrix */
		int quant;                         /**< [in]
											*
											* Frame quantizer :
											* <ul>
											* <li> 0 (zero) : Then the  rate controler chooses the right quantizer
											*                 for you.  Typically used in ABR encoding, or first pass of a VBR
											*                 encoding session.
											* <li> !=  0  :  Then you  force  the  encoder  to use  this  specific
											*                  quantizer   value.     It   is   clamped    in   the   interval
											*                  [1..31]. Tipically used  during the 2nd pass of  a VBR encoding
											*                  session. 
											* </ul> */
		int intra;                         /**< [in/out]
											*
											* <ul>
											* <li> [in] : tells XviD if the frame must be encoded as an intra frame
											*     <ul>
											*     <li> 1: forces the encoder  to create a keyframe. Mainly used during
											*              a VBR 2nd pass.
											*     <li> 0:  forces the  encoder not to  create a keyframe.  Minaly used
											*               during a VBR second pass
											*     <li> -1: let   the  encoder   decide  (based   on   contents  and
											*              max_key_interval). Mainly  used in ABR  mode and during  a 1st
											*              VBR pass. 
											*     </ul>
											* <li> [out] : When first set to -1, the encoder returns the effective keyframe state
											*              of the frame. 
											* </ul>
                                            */
		HINTINFO hint;                     /**< [in/out]
											*
											* mv hint information */

	}
	XVID_ENC_FRAME;


	/** Encoding statistics */
	typedef struct
	{
		int quant;               /**< [out]
								  *
								  * Frame quantizer used during encoding */
		int hlength;             /**< [out]
								  *
								  * Header bytes in the resulting MPEG4 stream */
		int kblks;               /**< [out]
								  *
								  * Number of intra macro blocks  */
		int mblks;               /**< [out]
								  *
								  * Number of inter macro blocks */
		int ublks;               /**< [out]
								  *
								  * Number of skipped macro blocks */
	}
	XVID_ENC_STATS;


/*****************************************************************************
 * Encoder entry point
 ****************************************************************************/

/**
 * \defgroup  encops_grp Encoder operations
 *
 * These are all the operations XviD's encoder can perform.
 *
 * @{
 */

#define XVID_ENC_ENCODE		0 /**< Encodes a frame
				   *
 * This operation constant is used when client application wants to encode a
 * frame. Client application must also fill XVID_ENC_FRAME appropriately.
 */

#define XVID_ENC_CREATE		1 /**< Creates a decoder instance
				   *
 * This operation constant is used by a client application in order to create
 * an encoder instance. Encoder instances are independant from each other.
 */

#define XVID_ENC_DESTROY	2 /**< Destroys a encoder instance
				   *
 * This operation constant is used by the client application to destroy a
 * previously created encoder instance.
 */


/** @} */

/**
 * \defgroup  encentry_grp Encoder entry point
 *
 * @{
 */

/**
 * \brief Encoder entry point.
 *
 * This is the XviD's encoder entry point. The possible operations are
 * described in the \ref encops_grp section.
 *
 * \param handle Encoder instance handle
 * \param opt Encoder option constant
 * \param param1 Used to pass XVID_ENC_PARAM or XVID_ENC_FRAME structures.
 * \param param2 Optionally used to pass the XVID_ENC_STATS structure.
 */
	int xvid_encore(void *handle,
					int opt,
					void *param1,
					void *param2);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
