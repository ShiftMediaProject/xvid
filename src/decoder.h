#ifndef _DECODER_H_
#define _DECODER_H_

#include "xvid.h"

#include "portab.h"
#include "global.h"
#include "image/image.h"


typedef struct
{
	// bitstream

	uint32_t shape;
	uint32_t time_inc_bits;
	uint32_t quant_bits;
	uint32_t quant_type;
	uint32_t quarterpel;

	uint32_t interlacing;
	uint32_t top_field_first;
	uint32_t alternate_vertical_scan;

	// image

	uint32_t width;
	uint32_t height;
	uint32_t edged_width;
	uint32_t edged_height;

	IMAGE cur;
	IMAGE refn[3];				// 0   -- last I or P VOP
	// 1   -- first I or P
	// 2   -- for interpolate mode B-frame
	IMAGE refh;
	IMAGE refv;
	IMAGE refhv;

	// macroblock

	uint32_t mb_width;
	uint32_t mb_height;
	MACROBLOCK *mbs;

	// for B-frame
	int32_t frames;				// total frame number
	int8_t scalability;
	VECTOR p_fmv, p_bmv;		// pred forward & backward motion vector
	MACROBLOCK *last_mbs;		// last MB
	int64_t time;				// for record time
	int64_t time_base;
	int64_t last_time_base;
	int64_t last_non_b_time;
	uint32_t time_pp;
	uint32_t time_bp;
	uint8_t low_delay;			// low_delay flage (1 means no B_VOP)
}
DECODER;

void init_decoder(uint32_t cpu_flags);

int decoder_create(XVID_DEC_PARAM * param);
int decoder_destroy(DECODER * dec);
int decoder_decode(DECODER * dec,
				   XVID_DEC_FRAME * frame);


#endif							/* _DECODER_H_ */
