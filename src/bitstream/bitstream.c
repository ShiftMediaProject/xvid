/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Bitstream reader/writer functions -
 *
 *  Copyright (C) 2001-2002 - Peter Ross <pross@cs.rmit.edu.au>
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
 * $Id: bitstream.c,v 1.31 2002-09-19 19:25:06 edgomez Exp $
 *
 ****************************************************************************/

#include "bitstream.h"
#include "zigzag.h"
#include "../quant/quant_matrix.h"

/*****************************************************************************
 * Functions
 ****************************************************************************/

static uint32_t __inline
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
#ifndef WIN32
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
read_video_packet_header(Bitstream *bs, const int addbits, int * quant)
{
	int nbits;
	int mbnum;
	int hec;
	
	nbits = NUMBITS_VP_RESYNC_MARKER + addbits;

	BitstreamSkip(bs, BitstreamNumBitsToByteAlign(bs));
	BitstreamSkip(bs, nbits);

	DPRINTF(DPRINTF_STARTCODE, "<video_packet_header>");

	// if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
		// hec
		// vop_width
		// marker_bit
		// vop_height
		// marker_bit

	//}

	mbnum = BitstreamGetBits(bs, 9);
	DPRINTF(DPRINTF_HEADER, "mbnum %i", mbnum);

	// if (dec->shape != VIDOBJLAY_SHAPE_BINARYONLY)
	*quant = BitstreamGetBits(bs, 5);
	DPRINTF(DPRINTF_HEADER, "quant %i", *quant);

	// if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
	hec = BitstreamGetBit(bs);
	DPRINTF(DPRINTF_HEADER, "header_extension_code %i", hec);
	// if (hec)
	//   .. decoder hec-header ...

	return mbnum;
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
					 uint32_t * quant,
					 uint32_t * fcode_forward,
					 uint32_t * fcode_backward,
					 uint32_t * intra_dc_threshold)
{
	uint32_t vol_ver_id;
	static uint32_t time_increment_resolution;
	uint32_t coding_type;
	uint32_t start_code;
	uint32_t time_incr = 0;
	int32_t time_increment = 0;

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

			DPRINTF(DPRINTF_STARTCODE, "<visual_object>");

			BitstreamSkip(bs, 32);	// visual_object_start_code
			if (BitstreamGetBit(bs))	// is_visual_object_identified
			{
				vol_ver_id = BitstreamGetBits(bs, 4);	// visual_object_ver_id
				DPRINTF(DPRINTF_HEADER,"ver_id %i", vol_ver_id);
				BitstreamSkip(bs, 3);	// visual_object_priority
			} else {
				vol_ver_id = 1;
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
			if (BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_SIMPLE && BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_CORE && BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_MAIN && BitstreamShowBits(bs, 8) != 0)	// BUGGY DIVX
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

			if (BitstreamGetBits(bs, 4) == VIDOBJLAY_AR_EXTPAR)	// aspect_ratio_info
			{
				DPRINTF(DPRINTF_HEADER, "+ aspect_ratio_info");
				BitstreamSkip(bs, 8);	// par_width
				BitstreamSkip(bs, 8);	// par_height
			}

			if (BitstreamGetBit(bs))	// vol_control_parameters
			{
				DPRINTF(DPRINTF_HEADER, "+ vol_control_parameters");
				BitstreamSkip(bs, 2);	// chroma_format
				dec->low_delay = BitstreamGetBit(bs);	// low_delay
				DPRINTF(DPRINTF_HEADER, "low_delay %i", dec->low_delay);
				if (BitstreamGetBit(bs))	// vbv_parameters
				{
					DPRINTF(DPRINTF_HEADER,"+ vbv_parameters");
					BitstreamSkip(bs, 15);	// first_half_bitrate
					READ_MARKER();
					BitstreamSkip(bs, 15);	// latter_half_bitrate
					READ_MARKER();
					BitstreamSkip(bs, 15);	// first_half_vbv_buffer_size
					READ_MARKER();
					BitstreamSkip(bs, 3);	// latter_half_vbv_buffer_size
					BitstreamSkip(bs, 11);	// first_half_vbv_occupancy
					READ_MARKER();
					BitstreamSkip(bs, 15);	// latter_half_vbv_occupancy
					READ_MARKER();

				}
			}

			dec->shape = BitstreamGetBits(bs, 2);	// video_object_layer_shape
			DPRINTF(DPRINTF_HEADER, "shape %i", dec->shape);

			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1) {
				BitstreamSkip(bs, 4);	// video_object_layer_shape_extension
			}

			READ_MARKER();

// *************************** for decode B-frame time ***********************
			time_increment_resolution = BitstreamGetBits(bs, 16);	// vop_time_increment_resolution

			DPRINTF(DPRINTF_HEADER,"vop_time_increment_resolution %i", time_increment_resolution);

//			time_increment_resolution--;

			if (time_increment_resolution > 0) {
				dec->time_inc_bits = log2bin(time_increment_resolution-1);
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

					// for auto set width & height
					if (dec->width == 0)
						dec->width = width;
					if (dec->height == 0)
						dec->height = height;

					if (width != dec->width || height != dec->height) {
						DPRINTF(DPRINTF_ERROR, "XVID_DEC_PARAM width/height does not match bitstream");
						return -1;
					}

				}

				dec->interlacing = BitstreamGetBit(bs);
				DPRINTF(DPRINTF_HEADER, "interlace", dec->interlacing);

				if (!BitstreamGetBit(bs))	// obmc_disable
				{
					DPRINTF(DPRINTF_ERROR, "obmc_disabled==false not supported");
					// TODO
					// fucking divx4.02 has this enabled
				}

				if (BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2)))	// sprite_enable
				{
					DPRINTF(DPRINTF_ERROR, "spriate_enabled not supported");
					return -1;
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
					DEBUG("QUARTERPEL BITSTREAM");
					dec->quarterpel = BitstreamGetBit(bs);	// quarter_sample
				}
				else
					dec->quarterpel = 0;
				

				if (!BitstreamGetBit(bs))	// complexity_estimation_disable
				{
					DPRINTF(DPRINTF_ERROR, "complexity_estimation not supported");
					return -1;
				}

				BitstreamSkip(bs, 1);	// resync_marker_disable

				if (BitstreamGetBit(bs))	// data_partitioned
				{
					DPRINTF(DPRINTF_ERROR, "data_partitioned not supported");
					BitstreamSkip(bs, 1);	// reversible_vlc
				}

				if (vol_ver_id != 1) {
					if (BitstreamGetBit(bs))	// newpred_enable
					{
						DPRINTF(DPRINTF_HEADER, "+ newpred_enable");
						BitstreamSkip(bs, 2);	// requested_upstream_message_type
						BitstreamSkip(bs, 1);	// newpred_segment_type
					}
					if (BitstreamGetBit(bs))	// reduced_resolution_vop_enable
					{
						DPRINTF(DPRINTF_ERROR, "reduced_resolution_vop not supported");
						return -1;
					}
				}

				if ((dec->scalability = BitstreamGetBit(bs)))	// scalability
				{
					DPRINTF(DPRINTF_ERROR, "scalability not supported");
					return -1;
				}
			} else				// dec->shape == BINARY_ONLY
			{
				if (vol_ver_id != 1) {
					if (BitstreamGetBit(bs))	// scalability
					{
					DPRINTF(DPRINTF_ERROR, "scalability not supported");
						return -1;
					}
				}
				BitstreamSkip(bs, 1);	// resync_marker_disable

			}

		} else if (start_code == GRPOFVOP_START_CODE) {

			DPRINTF(DPRINTF_STARTCODE, "<group_of_vop>");

			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;

				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);
				
				DPRINTF(DPRINTF_HEADER, "time %ih%im%is", hours);
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
				coding_type == I_VOP ? 'I' : coding_type == P_VOP ? 'P' : 'B',
				time_incr, time_increment);

			if (coding_type != B_VOP) {
				dec->last_time_base = dec->time_base;
				dec->time_base += time_incr;
				dec->time = time_increment;

/*					dec->time_base * time_increment_resolution +
					time_increment;
*/				dec->time_pp = (uint32_t) 
					(time_increment_resolution + dec->time - dec->last_non_b_time)%time_increment_resolution;
				dec->last_non_b_time = dec->time;
			} else {
				dec->time = time_increment; 
/*
					(dec->last_time_base +
					 time_incr) * time_increment_resolution + time_increment; 
*/
				dec->time_bp = (uint32_t) 
					(time_increment_resolution + dec->last_non_b_time - dec->time)%time_increment_resolution;
			}

			READ_MARKER();

			if (!BitstreamGetBit(bs))	// vop_coded
			{
				DPRINTF(DPRINTF_HEADER, "vop_coded==false");
				return N_VOP;
			}

			/* if (newpred_enable)
			   {
			   }
			 */

			// fix a little bug by MinChen <chenm002@163.com>
			if ((dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) &&
				(coding_type == P_VOP)) {
				*rounding = BitstreamGetBit(bs);	// rounding_type
				DPRINTF(DPRINTF_HEADER, "rounding %i", *rounding);
			}

			/* if (reduced_resolution_enable)
			   {
			   }
			 */

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR) {
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

				BitstreamSkip(bs, 1);	// change_conv_ratio_disable
				if (BitstreamGetBit(bs))	// vop_constant_alpha
				{
					BitstreamSkip(bs, 8);	// vop_constant_alpha_value
				}
			}


			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY) {
				// intra_dc_vlc_threshold
				*intra_dc_threshold =
					intra_dc_threshold_table[BitstreamGetBits(bs, 3)];

				if (dec->interlacing) {
					dec->top_field_first = BitstreamGetBit(bs);
					DPRINTF(DPRINTF_HEADER, "interlace top_field_first %i", dec->top_field_first);
					dec->alternate_vertical_scan = BitstreamGetBit(bs);
					DPRINTF(DPRINTF_HEADER, "interlace alternate_vertical_scan %i", dec->alternate_vertical_scan);
					
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

			DPRINTF(DPRINTF_STARTCODE, "<user_data>");

			BitstreamSkip(bs, 32);	// user_data_start_code

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
						const FRAMEINFO * frame)
{
	// video object_start_code & vo_id
	BitstreamPad(bs);
	BitstreamPutBits(bs, VO_START_CODE, 27);
	BitstreamPutBits(bs, 0, 5);

	// video_object_layer_start_code & vol_id
	BitstreamPutBits(bs, VOL_START_CODE, 28);
	BitstreamPutBits(bs, 0, 4);

	BitstreamPutBit(bs, 0);		// random_accessible_vol
	BitstreamPutBits(bs, 0, 8);	// video_object_type_indication
	BitstreamPutBit(bs, 0);		// is_object_layer_identified (0=not given)
	BitstreamPutBits(bs, 1, 4);	// aspect_ratio_info (1=1:1)

	BitstreamPutBit(bs, 1);	// vol_control_parameters
	BitstreamPutBits(bs, 1, 2);	// chroma_format 1="4:2:0"

	BitstreamPutBit(bs, 1);	// low_delay

	BitstreamPutBit(bs, 0);	// vbv_parameters (0=not given)

	BitstreamPutBits(bs, 0, 2);	// video_object_layer_shape (0=rectangular)

	WRITE_MARKER();

	/*
	 * time_increment_resolution; ignored by current decore versions
	 *  eg. 2fps     res=2       inc=1
	 *      25fps    res=25      inc=1
	 *      29.97fps res=30000   inc=1001
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
	BitstreamPutBit(bs, 0);		// sprite_enable
	BitstreamPutBit(bs, 0);		// not_in_bit

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

	BitstreamPutBit(bs, 1);		// complexity_estimation_disable
	BitstreamPutBit(bs, 1);		// resync_marker_disable
	BitstreamPutBit(bs, 0);		// data_partitioned
	BitstreamPutBit(bs, 0);		// scalability
}


/*
  write vop header

  NOTE: doesnt handle bother with time_base & time_inc
  time_base = n seconds since last resync (eg. last iframe)
  time_inc = nth of a second since last resync
  (decoder uses these values to determine precise time since last resync)
*/
void
BitstreamWriteVopHeader(Bitstream * const bs,
						const MBParam * pParam,
						const FRAMEINFO * frame,
						int vop_coded)
{
	uint32_t i;

	BitstreamPad(bs);
	BitstreamPutBits(bs, VOP_START_CODE, 32);

	BitstreamPutBits(bs, frame->coding_type, 2);

	// time_base = 0  write n x PutBit(1), PutBit(0)
	for (i = 0; i < frame->seconds; i++) {
		BitstreamPutBit(bs, 1);
	}
	BitstreamPutBit(bs, 0);

	WRITE_MARKER();

	// time_increment: value=nth_of_sec, nbits = log2(resolution)
	BitstreamPutBits(bs, frame->ticks, log2bin(pParam->fbase));

	WRITE_MARKER();

	if (!vop_coded) {
		BitstreamPutBits(bs, 0, 1);
		return;
	}

	BitstreamPutBits(bs, 1, 1);	// vop_coded

	if (frame->coding_type == P_VOP)
		BitstreamPutBits(bs, frame->rounding_type, 1);

	BitstreamPutBits(bs, 0, 3);	// intra_dc_vlc_threshold

	if (frame->global_flags & XVID_INTERLACING) {
		BitstreamPutBit(bs, 1);	// top field first
		BitstreamPutBit(bs, 0);	// alternate vertical scan
	}

	BitstreamPutBits(bs, frame->quant, 5);	// quantizer

	if (frame->coding_type != I_VOP)
		BitstreamPutBits(bs, frame->fcode, 3);	// forward_fixed_code

	if (frame->coding_type == B_VOP)
		BitstreamPutBits(bs, frame->bcode, 3);	// backward_fixed_code

}
