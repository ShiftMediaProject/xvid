 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  bitstream.c                                                               *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  Revision history:                                                         *
  *                                                                            *
  *  05.01.2003	GMC support - gruel                                            *
  *  04.10.2002	qpel support - Isibaar	                                       *
  *  11.07.2002 add VOP width & height return to dec when dec->width           *
  *             or dec->height is 0  (for use in examples/ex1.c)               *
  *             MinChen <chenm001@163.com>                                     *
  *  22.05.2002 bs_put_matrix fix                                              *
  *  20.05.2002 added BitstreamWriteUserData                                   *
  *  19.06.2002 Fix a little bug in use custom quant matrix                    *
  *             MinChen <chenm001@163.com>                                     *
  *  08.05.2002 add low_delay support for B_VOP decode                         *
  *             MinChen <chenm001@163.com>                                     *
  *  06.05.2002 low_delay                                                      *
  *  06.05.2002 fixed fincr/fbase error                                        *
  *  01.05.2002 added BVOP support to BitstreamWriteVopHeader                  *
  *  15.04.2002 rewrite log2bin use asm386  By MinChen <chenm001@163.com>      *
  *  26.03.2002 interlacing support                                            *
  *  03.03.2002 qmatrix writing                                                *
  *  03.03.2002 merged BITREADER and BITWRITER                                 *
  *  30.02.2002 intra_dc_threshold support                                     *
  *  04.12.2001 support for additional headers                                 *
  *  16.12.2001 inital version                                                 *
  *																			   *
  ******************************************************************************/


#include <string.h>
#include <stdio.h>

#include "bitstream.h"
#include "zigzag.h"
#include "../quant/quant_matrix.h"
#include "mbcoding.h"


static uint32_t __inline
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
#if !defined(_MSC_VER)
	int n = 0;

	while (value) {
		value >>= 1;
		n++;
	}
	return n;
#else
	__asm {
		bsr eax, value 
		inc eax
	}
#endif
}


static const uint32_t intra_dc_threshold_table[] = {
	32,							/* never use */
	13,
	15,
	17,
	19,
	21,
	23,
	1,
};


void
bs_get_matrix(Bitstream * bs,
			  uint8_t * matrix)
{
	int i = 0;
	int last, value = 0;

	do {
		last = value;
		value = BitstreamGetBits(bs, 8);
		matrix[scan_tables[0][i++]] = value;
	}
	while (value != 0 && i < 64);
        i--;    // fix little bug at coeff not full

	while (i < 64) {
		matrix[scan_tables[0][i++]] = last;
	}
}



// for PVOP addbits == fcode - 1
// for BVOP addbits == max(fcode,bcode) - 1
// returns mbpos
int
read_video_packet_header(Bitstream *bs, 
						DECODER * dec, 
						const int addbits, 
						int * quant, 
						int * fcode_forward,
						int  * fcode_backward,
						int * intra_dc_threshold)
{
	int startcode_bits = NUMBITS_VP_RESYNC_MARKER + addbits;
	int mbnum_bits = log2bin(dec->mb_width *  dec->mb_height - 1);
	int mbnum;
	int hec = 0;

	BitstreamSkip(bs, BitstreamNumBitsToByteAlign(bs));
	BitstreamSkip(bs, startcode_bits);

	DPRINTF(DPRINTF_STARTCODE, "<video_packet_header>");

	if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
	{
		hec = BitstreamGetBit(bs);		/* header_extension_code */
		if (hec && !(dec->sprite_enable == SPRITE_STATIC /* && current_coding_type = I_VOP */)) 
		{
			BitstreamSkip(bs, 13);			/* vop_width */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_height */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_horizontal_mc_spatial_ref */
			READ_MARKER();
			BitstreamSkip(bs, 13);			/* vop_vertical_mc_spatial_ref */
			READ_MARKER();
		}
	}

	mbnum = BitstreamGetBits(bs, mbnum_bits);		/* macroblock_number */
	DPRINTF(DPRINTF_HEADER, "mbnum %i", mbnum);

	if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
	{
		*quant = BitstreamGetBits(bs, dec->quant_bits);	/* quant_scale */	
		DPRINTF(DPRINTF_HEADER, "quant %i", *quant);
	}

	if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR)
		hec = BitstreamGetBit(bs);		/* header_extension_code */


	DPRINTF(DPRINTF_HEADER, "header_extension_code %i", hec);
	if (hec)
	{
		int time_base;
		int time_increment;
		int coding_type;

		for (time_base=0; BitstreamGetBit(bs)!=0; time_base++);		/* modulo_time_base */
		READ_MARKER();
		if (dec->time_inc_bits)
			time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	/* vop_time_increment */
		READ_MARKER();
		DPRINTF(DPRINTF_HEADER,"time %i:%i", time_base, time_increment);

		coding_type = BitstreamGetBits(bs, 2);
		DPRINTF(DPRINTF_HEADER,"coding_type %i", coding_type);
	
		if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
		{
			BitstreamSkip(bs, 1);	/* change_conv_ratio_disable */
			if (coding_type != I_VOP)
				BitstreamSkip(bs, 1);	/* vop_shape_coding_type */
		}

		if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
		{
			*intra_dc_threshold = intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

			if (dec->sprite_enable == SPRITE_GMC && coding_type == S_VOP &&
				dec->sprite_warping_points > 0)
			{
				// TODO: sprite trajectory
			}
			if (dec->reduced_resolution_enable && 
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP))
			{
				BitstreamSkip(bs, 1); /* XXX: vop_reduced_resolution */
			}

			if (coding_type != I_VOP && fcode_forward)
			{
				*fcode_forward = BitstreamGetBits(bs, 3);
				DPRINTF(DPRINTF_HEADER,"fcode_forward %i", *fcode_forward);
			}

			if (coding_type == B_VOP && fcode_backward)
			{
				*fcode_backward = BitstreamGetBits(bs, 3);
				DPRINTF(DPRINTF_HEADER,"fcode_backward %i", *fcode_backward);
			}
		}

	}

	if (dec->newpred_enable)
	{
		int vop_id;
		int vop_id_for_prediction;
		
		vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
		DPRINTF(DPRINTF_HEADER, "vop_id %i", vop_id);
		if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
		{
			vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
			DPRINTF(DPRINTF_HEADER, "vop_id_for_prediction %i", vop_id_for_prediction);
		}
		READ_MARKER();
	}

	return mbnum;
}




/* vol estimation header */
static void
read_vol_complexity_estimation_header(Bitstream * bs, DECODER * dec)
{
	ESTIMATION * e = &dec->estimation;

	e->method = BitstreamGetBits(bs, 2);	/* estimation_method */
	DPRINTF(DPRINTF_HEADER,"+ complexity_estimation_header; method=%i", e->method);

	if (e->method == 0 || e->method == 1)		
	{
		if (!BitstreamGetBit(bs))		/* shape_complexity_estimation_disable */
		{
			e->opaque = BitstreamGetBit(bs);		/* opaque */
			e->transparent = BitstreamGetBit(bs);		/* transparent */
			e->intra_cae = BitstreamGetBit(bs);		/* intra_cae */
			e->inter_cae = BitstreamGetBit(bs);		/* inter_cae */
			e->no_update = BitstreamGetBit(bs);		/* no_update */
			e->upsampling = BitstreamGetBit(bs);		/* upsampling */
		}

		if (!BitstreamGetBit(bs))	/* texture_complexity_estimation_set_1_disable */
		{
			e->intra_blocks = BitstreamGetBit(bs);		/* intra_blocks */
			e->inter_blocks = BitstreamGetBit(bs);		/* inter_blocks */
			e->inter4v_blocks = BitstreamGetBit(bs);		/* inter4v_blocks */
			e->not_coded_blocks = BitstreamGetBit(bs);		/* not_coded_blocks */
		}
	}

	READ_MARKER();

	if (!BitstreamGetBit(bs))		/* texture_complexity_estimation_set_2_disable */
	{
		e->dct_coefs = BitstreamGetBit(bs);		/* dct_coefs */
		e->dct_lines = BitstreamGetBit(bs);		/* dct_lines */
		e->vlc_symbols = BitstreamGetBit(bs);		/* vlc_symbols */
		e->vlc_bits = BitstreamGetBit(bs);		/* vlc_bits */
	}

	if (!BitstreamGetBit(bs))		/* motion_compensation_complexity_disable */
	{
		e->apm = BitstreamGetBit(bs);		/* apm */
		e->npm = BitstreamGetBit(bs);		/* npm */
		e->interpolate_mc_q = BitstreamGetBit(bs);		/* interpolate_mc_q */
		e->forw_back_mc_q = BitstreamGetBit(bs);		/* forw_back_mc_q */
		e->halfpel2 = BitstreamGetBit(bs);		/* halfpel2 */
		e->halfpel4 = BitstreamGetBit(bs);		/* halfpel4 */
	}

	READ_MARKER();

	if (e->method == 1)
	{
		if (!BitstreamGetBit(bs))	/* version2_complexity_estimation_disable */
		{
			e->sadct = BitstreamGetBit(bs);		/* sadct */
			e->quarterpel = BitstreamGetBit(bs);		/* quarterpel */
		}
	}
}


/* vop estimation header */
static void
read_vop_complexity_estimation_header(Bitstream * bs, DECODER * dec, int coding_type)
{
	ESTIMATION * e = &dec->estimation;

	if (e->method == 0 || e->method == 1)
	{
		if (coding_type == I_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* dcecs_opaque */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
		}
	
		if (coding_type == P_VOP) {
			if (e->opaque) BitstreamSkip(bs, 8);		/* */
			if (e->transparent) BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling) BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols) BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}
		if (coding_type == B_VOP) {
			if (e->opaque)		BitstreamSkip(bs, 8);	/* */
			if (e->transparent)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_cae)	BitstreamSkip(bs, 8);	/* */
			if (e->no_update)	BitstreamSkip(bs, 8);	/* */
			if (e->upsampling)	BitstreamSkip(bs, 8);	/* */
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
			if (e->sadct)		BitstreamSkip(bs, 8);	/* */
			if (e->quarterpel)	BitstreamSkip(bs, 8);	/* */
		}

		if (coding_type == S_VOP && dec->sprite_enable == SPRITE_STATIC) {
			if (e->intra_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->not_coded_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->dct_coefs)	BitstreamSkip(bs, 8);	/* */
			if (e->dct_lines)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_symbols)	BitstreamSkip(bs, 8);	/* */
			if (e->vlc_bits)	BitstreamSkip(bs, 8);	/* */
			if (e->inter_blocks) BitstreamSkip(bs, 8);	/* */
			if (e->inter4v_blocks)	BitstreamSkip(bs, 8);	/* */
			if (e->apm)			BitstreamSkip(bs, 8);	/* */
			if (e->npm)			BitstreamSkip(bs, 8);	/* */
			if (e->forw_back_mc_q)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel2)	BitstreamSkip(bs, 8);	/* */
			if (e->halfpel4)	BitstreamSkip(bs, 8);	/* */
			if (e->interpolate_mc_q) BitstreamSkip(bs, 8);	/* */
		}
	}
}





/*
decode headers
returns coding_type, or -1 if error
*/

#define VIDOBJ_START_CODE_MASK		0x0000001f
#define VIDOBJLAY_START_CODE_MASK	0x0000000f

int
BitstreamReadHeaders(Bitstream * bs,
					 DECODER * dec,
					 uint32_t * rounding,
					 uint32_t * reduced_resolution,
					 uint32_t * quant,
					 uint32_t * fcode_forward,
					 uint32_t * fcode_backward,
					 uint32_t * intra_dc_threshold,
					 WARPPOINTS *gmc_warp)
{
	uint32_t vol_ver_id;
	uint32_t coding_type;
	uint32_t start_code;
	uint32_t time_incr = 0;
	int32_t time_increment = 0;
	int resize = 0;

	do {

		BitstreamByteAlign(bs);
		start_code = BitstreamShowBits(bs, 32);

		if (start_code == VISOBJSEQ_START_CODE) {

			int profile;

			DPRINTF(DPRINTF_STARTCODE, "<visual_object_sequence>");

			BitstreamSkip(bs, 32);	// visual_object_sequence_start_code
			profile = BitstreamGetBits(bs, 8);	// profile_and_level_indication

			DPRINTF(DPRINTF_HEADER, "profile_and_level_indication %i", profile);

		} else if (start_code == VISOBJSEQ_STOP_CODE) {

			BitstreamSkip(bs, 32);	// visual_object_sequence_stop_code

			DPRINTF(DPRINTF_STARTCODE, "</visual_object_sequence>");

		} else if (start_code == VISOBJ_START_CODE) {
			int visobj_ver_id;

			DPRINTF(DPRINTF_STARTCODE, "<visual_object>");

			BitstreamSkip(bs, 32);	// visual_object_start_code
			if (BitstreamGetBit(bs))	// is_visual_object_identified
			{
				visobj_ver_id = BitstreamGetBits(bs, 4);	// visual_object_ver_id
				DPRINTF(DPRINTF_HEADER,"visobj_ver_id %i", visobj_ver_id);
				BitstreamSkip(bs, 3);	// visual_object_priority
			} else {
				visobj_ver_id = 1;
			}

			if (BitstreamShowBits(bs, 4) != VISOBJ_TYPE_VIDEO)	// visual_object_type
			{
				DPRINTF(DPRINTF_ERROR, "visual_object_type != video");
				return -1;
			}
			BitstreamSkip(bs, 4);

			// video_signal_type

			if (BitstreamGetBit(bs))	// video_signal_type
			{
				DPRINTF(DPRINTF_HEADER,"+ video_signal_type");
				BitstreamSkip(bs, 3);	// video_format
				BitstreamSkip(bs, 1);	// video_range
				if (BitstreamGetBit(bs))	// color_description
				{
					DPRINTF(DPRINTF_HEADER,"+ color_description");
					BitstreamSkip(bs, 8);	// color_primaries
					BitstreamSkip(bs, 8);	// transfer_characteristics
					BitstreamSkip(bs, 8);	// matrix_coefficients
				}
			}
		} else if ((start_code & ~VIDOBJ_START_CODE_MASK) == VIDOBJ_START_CODE) {

			DPRINTF(DPRINTF_STARTCODE, "<video_object>");
			DPRINTF(DPRINTF_HEADER, "vo id %i", start_code & VIDOBJ_START_CODE_MASK);

			BitstreamSkip(bs, 32);	// video_object_start_code

		} else if ((start_code & ~VIDOBJLAY_START_CODE_MASK) == VIDOBJLAY_START_CODE) {

			DPRINTF(DPRINTF_STARTCODE, "<video_object_layer>");
			DPRINTF(DPRINTF_HEADER, "vol id %i", start_code & VIDOBJLAY_START_CODE_MASK);

			BitstreamSkip(bs, 32);	// video_object_layer_start_code

			BitstreamSkip(bs, 1);	// random_accessible_vol

			// video_object_type_indication
			if (BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_SIMPLE && 
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_CORE && 
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_MAIN && 
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_ACE && 
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_ART_SIMPLE &&
				BitstreamShowBits(bs, 8) != 0)	// BUGGY DIVX
			{
				DPRINTF(DPRINTF_ERROR,"video_object_type_indication %i not supported ",
					BitstreamShowBits(bs, 8));
				return -1;
			}
			BitstreamSkip(bs, 8);


			if (BitstreamGetBit(bs))	// is_object_layer_identifier
			{
				DPRINTF(DPRINTF_HEADER, "+ is_object_layer_identifier");
				vol_ver_id = BitstreamGetBits(bs, 4);	// video_object_layer_verid
				DPRINTF(DPRINTF_HEADER,"ver_id %i", vol_ver_id);
				BitstreamSkip(bs, 3);	// video_object_layer_priority
			} else {
				vol_ver_id = 1;
			}

			dec->aspect_ratio = BitstreamGetBits(bs, 4);

			if (dec->aspect_ratio == VIDOBJLAY_AR_EXTPAR)	// aspect_ratio_info
			{
				DPRINTF(DPRINTF_HEADER, "+ aspect_ratio_info");
				dec->par_width = BitstreamGetBits(bs, 8);	// par_width
				dec->par_height = BitstreamGetBits(bs, 8);	// par_height
			}

			if (BitstreamGetBit(bs))	// vol_control_parameters
			{
				DPRINTF(DPRINTF_HEADER, "+ vol_control_parameters");
				BitstreamSkip(bs, 2);	// chroma_format
				dec->low_delay = BitstreamGetBit(bs);	// low_delay
				DPRINTF(DPRINTF_HEADER, "low_delay %i", dec->low_delay);
				if (BitstreamGetBit(bs))	// vbv_parameters
				{
					unsigned int bitrate;
					unsigned int buffer_size;
					unsigned int occupancy;

					DPRINTF(DPRINTF_HEADER,"+ vbv_parameters");

					bitrate = BitstreamGetBits(bs,15) << 15;	// first_half_bit_rate
					READ_MARKER();
					bitrate |= BitstreamGetBits(bs,15);		// latter_half_bit_rate
					READ_MARKER();
					
					buffer_size = BitstreamGetBits(bs, 15) << 3;	// first_half_vbv_buffer_size
					READ_MARKER();
					buffer_size |= BitstreamGetBits(bs, 3);		// latter_half_vbv_buffer_size

					occupancy = BitstreamGetBits(bs, 11) << 15;	// first_half_vbv_occupancy
					READ_MARKER();
					occupancy |= BitstreamGetBits(bs, 15);	// latter_half_vbv_occupancy
					READ_MARKER();

					DPRINTF(DPRINTF_HEADER,"bitrate %d (unit=400 bps)", bitrate);
					DPRINTF(DPRINTF_HEADER,"buffer_size %d (unit=16384 bits)", buffer_size);
					DPRINTF(DPRINTF_HEADER,"occupancy %d (unit=64 bits)", occupancy);
				}
			}else{
				dec->low_delay = dec->low_delay_default;
			}

			dec->shape = BitstreamGetBits(bs, 2);	// video_object_layer_shape

			DPRINTF(DPRINTF_HEADER, "shape %i", dec->shape);
			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
			{
				DPRINTF(DPRINTF_ERROR,"non-rectangular shapes are not supported");
			}

			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
				BitstreamSkip(bs, 4);	// video_object_layer_shape_extension
			}

			READ_MARKER();

// *************************** for decode B-frame time ***********************
			dec->time_inc_resolution = BitstreamGetBits(bs, 16);	// vop_time_increment_resolution
			DPRINTF(DPRINTF_HEADER,"vop_time_increment_resolution %i", dec->time_inc_resolution);

//			dec->time_inc_resolution--;

			if (dec->time_inc_resolution > 0) {
				dec->time_inc_bits = log2bin(dec->time_inc_resolution-1);
			} else {
				// dec->time_inc_bits = 0;
				// for "old" xvid compatibility, set time_inc_bits = 1
				dec->time_inc_bits = 1;
			}

			READ_MARKER();

			if (BitstreamGetBit(bs))	// fixed_vop_rate
			{
				DPRINTF(DPRINTF_HEADER, "+ fixed_vop_rate");
				BitstreamSkip(bs, dec->time_inc_bits);	// fixed_vop_time_increment
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR) {
					uint32_t width, height;

					READ_MARKER();
					width = BitstreamGetBits(bs, 13);	// video_object_layer_width
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);	// video_object_layer_height
					READ_MARKER();

					DPRINTF(DPRINTF_HEADER, "width %i", width);
					DPRINTF(DPRINTF_HEADER, "height %i", height);

					if (dec->width != width || dec->height != height)
					{
						if (dec->fixed_dimensions)
						{
							DPRINTF(DPRINTF_ERROR, "XVID_DEC_PARAM width/height does not match bitstream");
							return -1;
						}
						resize = 1;
						dec->width = width;
						dec->height = height;
					}
				}

				dec->interlacing = BitstreamGetBit(bs);
				DPRINTF(DPRINTF_HEADER, "interlacing %i", dec->interlacing);

				if (!BitstreamGetBit(bs))	// obmc_disable
				{
					DPRINTF(DPRINTF_ERROR, "obmc_disabled==false not supported");
					// TODO
					// fucking divx4.02 has this enabled
				}

				dec->sprite_enable = BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2));	// sprite_enable

				if (dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable == SPRITE_GMC)
				{
					int low_latency_sprite_enable;

					if (dec->sprite_enable != SPRITE_GMC)
					{
						int sprite_width;
						int sprite_height;
						int sprite_left_coord;
						int sprite_top_coord;
						sprite_width = BitstreamGetBits(bs, 13);		// sprite_width
						READ_MARKER();
						sprite_height = BitstreamGetBits(bs, 13);	// sprite_height
						READ_MARKER();
						sprite_left_coord = BitstreamGetBits(bs, 13);	// sprite_left_coordinate
						READ_MARKER();
						sprite_top_coord = BitstreamGetBits(bs, 13);	// sprite_top_coordinate
						READ_MARKER();
					}
					dec->sprite_warping_points = BitstreamGetBits(bs, 6);		// no_of_sprite_warping_points
					dec->sprite_warping_accuracy = BitstreamGetBits(bs, 2);		// sprite_warping_accuracy
					dec->sprite_brightness_change = BitstreamGetBits(bs, 1);		// brightness_change
					if (dec->sprite_enable != SPRITE_GMC)
					{
						low_latency_sprite_enable = BitstreamGetBits(bs, 1);		// low_latency_sprite_enable
					}
				}

				if (vol_ver_id != 1 &&
					dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
					BitstreamSkip(bs, 1);	// sadct_disable
				}

				if (BitstreamGetBit(bs))	// not_8_bit
				{
					DPRINTF(DPRINTF_HEADER, "not_8_bit==true (ignored)");
					dec->quant_bits = BitstreamGetBits(bs, 4);	// quant_precision
					BitstreamSkip(bs, 4);	// bits_per_pixel
				} else {
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
					BitstreamSkip(bs, 1);	// no_gray_quant_update
					BitstreamSkip(bs, 1);	// composition_method
					BitstreamSkip(bs, 1);	// linear_composition
				}

				dec->quant_type = BitstreamGetBit(bs);	// quant_type
				DPRINTF(DPRINTF_HEADER, "quant_type %i", dec->quant_type);

				if (dec->quant_type) {
					if (BitstreamGetBit(bs))	// load_intra_quant_mat
					{
						uint8_t matrix[64];

						DPRINTF(DPRINTF_HEADER, "load_intra_quant_mat");

						bs_get_matrix(bs, matrix);
						set_intra_matrix(matrix);
					} else
						set_intra_matrix(get_default_intra_matrix());

					if (BitstreamGetBit(bs))	// load_inter_quant_mat
					{
						uint8_t matrix[64];
						
						DPRINTF(DPRINTF_HEADER, "load_inter_quant_mat");

						bs_get_matrix(bs, matrix);
						set_inter_matrix(matrix);
					} else
						set_inter_matrix(get_default_inter_matrix());

					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE) {
						DPRINTF(DPRINTF_ERROR, "greyscale matrix not supported");
						return -1;
					}

				}


				if (vol_ver_id != 1) {
					dec->quarterpel = BitstreamGetBit(bs);	// quarter_sample
					DPRINTF(DPRINTF_HEADER,"quarterpel %i", dec->quarterpel);
				}
				else
					dec->quarterpel = 0;
				

				dec->complexity_estimation_disable = BitstreamGetBit(bs);	/* complexity estimation disable */
				if (!dec->complexity_estimation_disable)
				{
					read_vol_complexity_estimation_header(bs, dec);
				}

				BitstreamSkip(bs, 1);	// resync_marker_disable

				if (BitstreamGetBit(bs))	// data_partitioned
				{
					DPRINTF(DPRINTF_ERROR, "data_partitioned not supported");
					BitstreamSkip(bs, 1);	// reversible_vlc
				}

				if (vol_ver_id != 1) {
					dec->newpred_enable = BitstreamGetBit(bs);
					if (dec->newpred_enable)	// newpred_enable
					{
						DPRINTF(DPRINTF_HEADER, "+ newpred_enable");
						BitstreamSkip(bs, 2);	// requested_upstream_message_type
						BitstreamSkip(bs, 1);	// newpred_segment_type
					}
					dec->reduced_resolution_enable = BitstreamGetBit(bs);	/* reduced_resolution_vop_enable */
					DPRINTF(DPRINTF_HEADER, "reduced_resolution_enable %i", dec->reduced_resolution_enable);
				}
				else
				{
					dec->newpred_enable = 0;
					dec->reduced_resolution_enable = 0;
				}

				dec->scalability = BitstreamGetBit(bs);	/* scalability */
				if (dec->scalability)	
				{
					DPRINTF(DPRINTF_ERROR, "scalability not supported");
					BitstreamSkip(bs, 1);	/* hierarchy_type */
					BitstreamSkip(bs, 4);	/* ref_layer_id */
					BitstreamSkip(bs, 1);	/* ref_layer_sampling_direc */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
					BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
					BitstreamSkip(bs, 1);	/* enhancement_type */
					if(dec->shape == VIDOBJLAY_SHAPE_BINARY /* && hierarchy_type==0 */) {
						BitstreamSkip(bs, 1);	/* use_ref_shape */
						BitstreamSkip(bs, 1);	/* use_ref_texture */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* shape_vert_sampling_factor_m */
					}
					return -1;
				}
			} else				// dec->shape == BINARY_ONLY
			{
				if (vol_ver_id != 1) {
					dec->scalability = BitstreamGetBit(bs); /* scalability */
					if (dec->scalability)	
					{
						DPRINTF(DPRINTF_ERROR, "scalability not supported");
						BitstreamSkip(bs, 4);	/* ref_layer_id */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* hor_sampling_factor_m */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_n */
						BitstreamSkip(bs, 5);	/* vert_sampling_factor_m */
						return -1;
					}
				}
				BitstreamSkip(bs, 1);	// resync_marker_disable

			}

			return (resize ? -3 : -2 );	/* VOL */

		} else if (start_code == GRPOFVOP_START_CODE) {

			DPRINTF(DPRINTF_STARTCODE, "<group_of_vop>");

			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;

				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);
				
				DPRINTF(DPRINTF_HEADER, "time %ih%im%is", hours,minutes,seconds);
			}
			BitstreamSkip(bs, 1);	// closed_gov
			BitstreamSkip(bs, 1);	// broken_link

		} else if (start_code == VOP_START_CODE) {

			DPRINTF(DPRINTF_STARTCODE, "<vop>");

			BitstreamSkip(bs, 32);	// vop_start_code

			coding_type = BitstreamGetBits(bs, 2);	// vop_coding_type
			DPRINTF(DPRINTF_HEADER, "coding_type %i", coding_type);

// *************************** for decode B-frame time ***********************
			while (BitstreamGetBit(bs) != 0)	// time_base
				time_incr++;

			READ_MARKER();

			if (dec->time_inc_bits) {
				time_increment = (BitstreamGetBits(bs, dec->time_inc_bits));	// vop_time_increment
			}

			DPRINTF(DPRINTF_HEADER, "time_base %i", time_incr);
			DPRINTF(DPRINTF_HEADER, "time_increment %i", time_increment);

			DPRINTF(DPRINTF_TIMECODE, "%c %i:%i", 
				coding_type == I_VOP ? 'I' : coding_type == P_VOP ? 'P' : coding_type == B_VOP ? 'B' : 'S',
				time_incr, time_increment);

			if (coding_type != B_VOP) {
				dec->last_time_base = dec->time_base;
				dec->time_base += time_incr;
				dec->time = time_increment;

/*					dec->time_base * dec->time_inc_resolution +
					time_increment;
*/				dec->time_pp = (uint32_t) 
					(dec->time_inc_resolution + dec->time - dec->last_non_b_time)%dec->time_inc_resolution;
				dec->last_non_b_time = dec->time;
			} else {
				dec->time = time_increment; 
/*
					(dec->last_time_base +
					 time_incr) * dec->time_inc_resolution + time_increment; 
*/
				dec->time_bp = (uint32_t) 
					(dec->time_inc_resolution + dec->last_non_b_time - dec->time)%dec->time_inc_resolution;
			}
			DPRINTF(DPRINTF_HEADER,"time_pp=%i", dec->time_pp);
			DPRINTF(DPRINTF_HEADER,"time_bp=%i", dec->time_bp);

			READ_MARKER();

			if (!BitstreamGetBit(bs))	// vop_coded
			{
				DPRINTF(DPRINTF_HEADER, "vop_coded==false");
				return N_VOP;
			}

			if (dec->newpred_enable)
			{
				int vop_id;
				int vop_id_for_prediction;
				
				vop_id = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
				DPRINTF(DPRINTF_HEADER, "vop_id %i", vop_id);
				if (BitstreamGetBit(bs))	/* vop_id_for_prediction_indication */
				{
					vop_id_for_prediction = BitstreamGetBits(bs, MIN(dec->time_inc_bits + 3, 15));
					DPRINTF(DPRINTF_HEADER, "vop_id_for_prediction %i", vop_id_for_prediction);
				}
				READ_MARKER();
			}

		

			// fix a little bug by MinChen <chenm002@163.com>
			if ((dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) &&
				( (coding_type == P_VOP) || (coding_type == S_VOP && dec->sprite_enable == SPRITE_GMC) ) ) {
				*rounding = BitstreamGetBit(bs);	// rounding_type
				DPRINTF(DPRINTF_HEADER, "rounding %i", *rounding);
			}

			if (dec->reduced_resolution_enable &&
				dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR &&
				(coding_type == P_VOP || coding_type == I_VOP)) {

				*reduced_resolution = BitstreamGetBit(bs);
				DPRINTF(DPRINTF_HEADER, "reduced_resolution %i", *reduced_resolution);
			}
			else
			{
				*reduced_resolution = 0;
			}

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
				if(!(dec->sprite_enable == SPRITE_STATIC && coding_type == I_VOP)) {

					uint32_t width, height;
					uint32_t horiz_mc_ref, vert_mc_ref;

					width = BitstreamGetBits(bs, 13);
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);
					READ_MARKER();
					horiz_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();
					vert_mc_ref = BitstreamGetBits(bs, 13);
					READ_MARKER();

					DPRINTF(DPRINTF_HEADER, "width %i", width);
					DPRINTF(DPRINTF_HEADER, "height %i", height);
					DPRINTF(DPRINTF_HEADER, "horiz_mc_ref %i", horiz_mc_ref);
					DPRINTF(DPRINTF_HEADER, "vert_mc_ref %i", vert_mc_ref);
				}

				BitstreamSkip(bs, 1);	// change_conv_ratio_disable
				if (BitstreamGetBit(bs))	// vop_constant_alpha
				{
					BitstreamSkip(bs, 8);	// vop_constant_alpha_value
				}
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {

				if (!dec->complexity_estimation_disable)
				{
					read_vop_complexity_estimation_header(bs, dec, coding_type);
				}

				// intra_dc_vlc_threshold
				*intra_dc_threshold =
					intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

				dec->top_field_first = 0;
				dec->alternate_vertical_scan = 0;

				if (dec->interlacing) {
					dec->top_field_first = BitstreamGetBit(bs);
					DPRINTF(DPRINTF_HEADER, "interlace top_field_first %i", dec->top_field_first);
					dec->alternate_vertical_scan = BitstreamGetBit(bs);
					DPRINTF(DPRINTF_HEADER, "interlace alternate_vertical_scan %i", dec->alternate_vertical_scan);
					
				}
			}

			if ((dec->sprite_enable == SPRITE_STATIC || dec->sprite_enable== SPRITE_GMC) && coding_type == S_VOP) {

				int i;

				for (i = 0 ; i < dec->sprite_warping_points; i++)
				{
					int length;
					int x = 0, y = 0;

					/* sprite code borowed from ffmpeg; thx Michael Niedermayer <michaelni@gmx.at> */
					length = bs_get_spritetrajectory(bs);
					if(length){
						x= BitstreamGetBits(bs, length);
						if ((x >> (length - 1)) == 0) /* if MSB not set it is negative*/
							x = - (x ^ ((1 << length) - 1));
					}
					READ_MARKER();
        
					length = bs_get_spritetrajectory(bs);
					if(length){
						y = BitstreamGetBits(bs, length);
						if ((y >> (length - 1)) == 0) /* if MSB not set it is negative*/
							y = - (y ^ ((1 << length) - 1));
					}
					READ_MARKER();

					gmc_warp->duv[i].x = x;
					gmc_warp->duv[i].y = y;

					DPRINTF(DPRINTF_HEADER,"sprite_warping_point[%i] xy=(%i,%i)", i, x, y);
				}

				if (dec->sprite_brightness_change)
				{
					// XXX: brightness_change_factor()
				}
				if (dec->sprite_enable == SPRITE_STATIC)
				{
					// XXX: todo
				}

			}

			if ((*quant = BitstreamGetBits(bs, dec->quant_bits)) < 1)	// vop_quant
				*quant = 1;
			DPRINTF(DPRINTF_HEADER, "quant %i", *quant);

			if (coding_type != I_VOP) {
				*fcode_forward = BitstreamGetBits(bs, 3);	// fcode_forward
				DPRINTF(DPRINTF_HEADER, "fcode_forward %i", *fcode_forward);
			}

			if (coding_type == B_VOP) {
				*fcode_backward = BitstreamGetBits(bs, 3);	// fcode_backward
				DPRINTF(DPRINTF_HEADER, "fcode_backward %i", *fcode_backward);
			}
			if (!dec->scalability) {
				if ((dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) &&
					(coding_type != I_VOP)) {
					BitstreamSkip(bs, 1);	// vop_shape_coding_type
				}
			}
			return coding_type;

		} else if (start_code == USERDATA_START_CODE) {
			char tmp[256];
		    int i, version, build;
			char packed;

			BitstreamSkip(bs, 32);	// user_data_start_code

			tmp[0] = BitstreamShowBits(bs, 8);
    
			for(i = 1; i < 256; i++){
				tmp[i] = (BitstreamShowBits(bs, 16) & 0xFF);
				
				if(tmp[i] == 0) 
					break;
				
				BitstreamSkip(bs, 8);
			}

			DPRINTF(DPRINTF_STARTCODE, "<user_data>: %s\n", tmp);
			
			/* read xvid bitstream version */
			if(strncmp(tmp, "XviD", 4) == 0) {
				sscanf(tmp, "XviD%d", &dec->bs_version);
				DPRINTF(DPRINTF_HEADER, "xvid bitstream version=%i", dec->bs_version);
			}

		    /* divx detection */
			i = sscanf(tmp, "DivX%dBuild%d%c", &version, &build, &packed);
			if (i < 2)
				i = sscanf(tmp, "DivX%db%d%c", &version, &build, &packed);
			
			if (i >= 2)
			{
				dec->packed_mode = (i == 3 && packed == 'p');
				DPRINTF(DPRINTF_HEADER, "divx version=%i, build=%i packed=%i",
						version, build, dec->packed_mode);
			}

		} else					// start_code == ?
		{
			if (BitstreamShowBits(bs, 24) == 0x000001) {
				DPRINTF(DPRINTF_STARTCODE, "<unknown: %x>", BitstreamShowBits(bs, 32));
			}
			BitstreamSkip(bs, 8);
		}
	}
	while ((BitstreamPos(bs) >> 3) < bs->length);

	//DPRINTF("*** WARNING: no vop_start_code found");
	return -1;					/* ignore it */
}


/* write custom quant matrix */

static void
bs_put_matrix(Bitstream * bs,
			  const int16_t * matrix)
{
	int i, j;
	const int last = matrix[scan_tables[0][63]];

	for (j = 63; j > 0 && matrix[scan_tables[0][j - 1]] == last; j--);

	for (i = 0; i <= j; i++) {
		BitstreamPutBits(bs, matrix[scan_tables[0][i]], 8);
	}

	if (j < 63) {
		BitstreamPutBits(bs, 0, 8);
	}
}


/*
	write vol header
*/
void
BitstreamWriteVolHeader(Bitstream * const bs,
						const MBParam * pParam,
						const FRAMEINFO * const frame)
{
	static const unsigned int vo_id = 0;
	static const unsigned int vol_id = 0;
	int vol_ver_id=1;
	int profile = 0x03;	/* simple profile/level 3 */

	if ( pParam->m_quarterpel ||  (frame->global_flags & XVID_GMC) || 
		 (pParam->global & XVID_GLOBAL_REDUCED))
		vol_ver_id = 2;

	if ((pParam->global & XVID_GLOBAL_REDUCED))
		profile = 0x93;	/* advanced realtime simple profile/level 3 */

	if (pParam->m_quarterpel ||  (frame->global_flags & XVID_GMC))
		profile = 0xf3;	/* advanced simple profile/level 2 */

	// visual_object_sequence_start_code
//	BitstreamPad(bs);
/* no padding here, anymore. You have to make sure that you are 
   byte aligned, and that always 1-8 padding bits have been written */

	BitstreamPutBits(bs, VISOBJSEQ_START_CODE, 32);
	BitstreamPutBits(bs, profile, 8);

	// visual_object_start_code
	BitstreamPad(bs);
	BitstreamPutBits(bs, VISOBJ_START_CODE, 32);
	BitstreamPutBits(bs, 0, 1);		// is_visual_object_identifier
	BitstreamPutBits(bs, VISOBJ_TYPE_VIDEO, 4);		// visual_object_type
	
	// video object_start_code & vo_id
	BitstreamPad(bs);
	BitstreamPutBits(bs, VIDOBJ_START_CODE|(vo_id&0x5), 32);

	// video_object_layer_start_code & vol_id
	BitstreamPad(bs);
	BitstreamPutBits(bs, VIDOBJLAY_START_CODE|(vol_id&0x4), 32);

	BitstreamPutBit(bs, 0);		// random_accessible_vol
	BitstreamPutBits(bs, 0, 8);	// video_object_type_indication

	if (vol_ver_id == 1)
	{
		BitstreamPutBit(bs, 0);				// is_object_layer_identified (0=not given)
	}
	else
	{
		BitstreamPutBit(bs, 1);		// is_object_layer_identified
		BitstreamPutBits(bs, vol_ver_id, 4);	// vol_ver_id == 2
		BitstreamPutBits(bs, 4, 3); // vol_ver_priority (1==lowest, 7==highest) ??
	}

	BitstreamPutBits(bs, 1, 4);	// aspect_ratio_info (1=1:1)

	BitstreamPutBit(bs, 1);	// vol_control_parameters
	BitstreamPutBits(bs, 1, 2);	// chroma_format 1="4:2:0"

	if (pParam->max_bframes > 0) {
		BitstreamPutBit(bs, 0);	// low_delay
	} else
	{
		BitstreamPutBit(bs, 1);	// low_delay
	}
	BitstreamPutBit(bs, 0);	// vbv_parameters (0=not given)

	BitstreamPutBits(bs, 0, 2);	// video_object_layer_shape (0=rectangular)

	WRITE_MARKER();

	/* time_inc_resolution; ignored by current decore versions
	   eg. 2fps     res=2       inc=1
	   25fps        res=25      inc=1
	   29.97fps res=30000   inc=1001
	 */
	BitstreamPutBits(bs, pParam->fbase, 16);

	WRITE_MARKER();

	BitstreamPutBit(bs, 1);		// fixed_vop_rate = 1
	BitstreamPutBits(bs, pParam->fincr, log2bin(pParam->fbase));	// fixed_vop_time_increment

	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->width, 13);	// width
	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->height, 13);	// height
	WRITE_MARKER();

	BitstreamPutBit(bs, frame->global_flags & XVID_INTERLACING);	// interlace
	BitstreamPutBit(bs, 1);		// obmc_disable (overlapped block motion compensation)

	if (vol_ver_id != 1) 
	{	if (frame->global_flags & XVID_GMC)
		{	BitstreamPutBits(bs, 2, 2);		// sprite_enable=='GMC'
			BitstreamPutBits(bs, 2, 6);		// no_of_sprite_warping_points
			BitstreamPutBits(bs, 3, 2);		// sprite_warping_accuracy 0==1/2, 1=1/4, 2=1/8, 3=1/16
			BitstreamPutBit(bs, 0);			// sprite_brightness_change (not supported)

/* currently we use no_of_sprite_warping_points==2, sprite_warping_accuracy==3 
   for DivX5 compatability */

		} else
			BitstreamPutBits(bs, 0, 2);		// sprite_enable==off
	}
	else
		BitstreamPutBit(bs, 0);		// sprite_enable==off

	BitstreamPutBit(bs, 0);		// not_8_bit

	// quant_type   0=h.263  1=mpeg4(quantizer tables)
	BitstreamPutBit(bs, pParam->m_quant_type);

	if (pParam->m_quant_type) {
		BitstreamPutBit(bs, get_intra_matrix_status());	// load_intra_quant_mat
		if (get_intra_matrix_status()) {
			bs_put_matrix(bs, get_intra_matrix());
		}

		BitstreamPutBit(bs, get_inter_matrix_status());	// load_inter_quant_mat
		if (get_inter_matrix_status()) {
			bs_put_matrix(bs, get_inter_matrix());
		}

	}

	if (vol_ver_id != 1) {
		if (pParam->m_quarterpel)
			BitstreamPutBit(bs, 1);	 	//  quarterpel 
		else
			BitstreamPutBit(bs, 0);		// no quarterpel
	}

	BitstreamPutBit(bs, 1);		// complexity_estimation_disable
	BitstreamPutBit(bs, 1);		// resync_marker_disable
	BitstreamPutBit(bs, 0);		// data_partitioned

	if (vol_ver_id != 1) 
	{
		BitstreamPutBit(bs, 0);		// newpred_enable
		
		BitstreamPutBit(bs, (pParam->global & XVID_GLOBAL_REDUCED)?1:0);	
									/* reduced_resolution_vop_enabled */
	}
	
	BitstreamPutBit(bs, 0);		// scalability

	/* fake divx5 id, to ensure compatibility with divx5 decoder */
#define DIVX5_ID "DivX501b481p"
	if (pParam->max_bframes > 0 && (pParam->global & XVID_GLOBAL_PACKED)) {
		BitstreamWriteUserData(bs, DIVX5_ID, strlen(DIVX5_ID));
	}

	/* xvid id */
#define XVID_ID "XviD" XVID_BS_VERSION
	BitstreamWriteUserData(bs, XVID_ID, strlen(XVID_ID));
}


/*
  write vop header
*/
void
BitstreamWriteVopHeader(
						Bitstream * const bs,
						const MBParam * pParam,
						const FRAMEINFO * const frame,
						int vop_coded)
{
	uint32_t i;

//	BitstreamPad(bs);
/* no padding here, anymore. You have to make sure that you are 
   byte aligned, and that always 1-8 padding bits have been written */

	BitstreamPutBits(bs, VOP_START_CODE, 32);

	BitstreamPutBits(bs, frame->coding_type, 2);
	DPRINTF(DPRINTF_HEADER, "coding_type = %i", frame->coding_type);

	for (i = 0; i < frame->seconds; i++) {
		BitstreamPutBit(bs, 1);
	}
	BitstreamPutBit(bs, 0);

	WRITE_MARKER();

	// time_increment: value=nth_of_sec, nbits = log2(resolution)

	BitstreamPutBits(bs, frame->ticks, log2bin(pParam->fbase));
	/*DPRINTF("[%i:%i] %c", frame->seconds, frame->ticks,
			frame->coding_type == I_VOP ? 'I' : frame->coding_type ==
			P_VOP ? 'P' : 'B');*/

	WRITE_MARKER();

	if (!vop_coded) {
		BitstreamPutBits(bs, 0, 1);
		return;
	}

	BitstreamPutBits(bs, 1, 1);	// vop_coded

	if ( (frame->coding_type == P_VOP) || (frame->coding_type == S_VOP) )
		BitstreamPutBits(bs, frame->rounding_type, 1);

	if ((pParam->global & XVID_GLOBAL_REDUCED))
		BitstreamPutBit(bs, (frame->global_flags & XVID_REDUCED)?1:0);

	BitstreamPutBits(bs, 0, 3);	// intra_dc_vlc_threshold

	if (frame->global_flags & XVID_INTERLACING) {
		BitstreamPutBit(bs, (frame->global_flags & XVID_TOPFIELDFIRST));
		BitstreamPutBit(bs, (frame->global_flags & XVID_ALTERNATESCAN));
	}
	
	if (frame->coding_type == S_VOP) {
		if (1)	{		// no_of_sprite_warping_points>=1 (we use 2!)
			int k;
			for (k=0;k<2;k++)
			{
				bs_put_spritetrajectory(bs, frame->warp.duv[k].x ); // du[k] 
				WRITE_MARKER();
			
				bs_put_spritetrajectory(bs, frame->warp.duv[k].y ); // dv[k] 
				WRITE_MARKER();

			if (pParam->m_quarterpel)
			{
				DPRINTF(DPRINTF_HEADER,"sprite_warping_point[%i] xy=(%i,%i) *QPEL*", k, frame->warp.duv[k].x/2, frame->warp.duv[k].y/2);
			}
			else
			{
				DPRINTF(DPRINTF_HEADER,"sprite_warping_point[%i] xy=(%i,%i)", k, frame->warp.duv[k].x, frame->warp.duv[k].y);
			}
			}
		}
	}
	

	DPRINTF(DPRINTF_HEADER, "quant = %i", frame->quant);

	BitstreamPutBits(bs, frame->quant, 5);	// quantizer

	if (frame->coding_type != I_VOP)
		BitstreamPutBits(bs, frame->fcode, 3);	// forward_fixed_code

	if (frame->coding_type == B_VOP)
		BitstreamPutBits(bs, frame->bcode, 3);	// backward_fixed_code

}

void 
BitstreamWriteUserData(Bitstream * const bs, 
						uint8_t * data, 
						const int length)
{
	int i;

	BitstreamPad(bs);
	BitstreamPutBits(bs, USERDATA_START_CODE, 32);

	for (i = 0; i < length; i++) {
		BitstreamPutBits(bs, data[i], 8);
	}

}
