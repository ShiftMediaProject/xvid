/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Rate Controler module -
 *
 *  Copyright(C) 2002 Benjamin Lambert <foxer@hotmail.com>
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
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
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
 * $Id: ratecontrol.c,v 1.17 2002-09-26 00:33:00 h Exp $
 *
 ****************************************************************************/

#include <math.h>
#include "ratecontrol.h"

/*****************************************************************************
 * Local prototype - defined at the end of the file
 ****************************************************************************/

static int get_initial_quant(int bpp);

/*****************************************************************************
 * RateControlInit
 *
 * This function initialize the structure members of 'rate_control' argument
 * according to the other arguments.
 *
 * Returned value : None
 *
 ****************************************************************************/

void
RateControlInit(RateControl * rate_control,
				uint32_t target_rate,
				uint32_t reaction_delay_factor,
				uint32_t averaging_period,
				uint32_t buffer,
				int framerate,
				int max_quant,
				int min_quant)
{
	int i;

	rate_control->frames = 0;
	rate_control->total_size = 0;

	rate_control->framerate = framerate / 1000.0;
	rate_control->target_rate = target_rate;

	rate_control->rtn_quant = get_initial_quant(0);
	rate_control->max_quant = max_quant;
	rate_control->min_quant = min_quant;

	for (i = 0; i < 32; ++i) {
		rate_control->quant_error[i] = 0.0;
	}

	rate_control->target_framesize =
		(double) target_rate / 8.0 / rate_control->framerate;
	rate_control->sequence_quality = 2.0 / (double) rate_control->rtn_quant;
	rate_control->avg_framesize = rate_control->target_framesize;

	rate_control->reaction_delay_factor = reaction_delay_factor;
	rate_control->averaging_period = averaging_period;
	rate_control->buffer = buffer;
}

/*****************************************************************************
 * RateControlGetQ
 *
 * Returned value : - the current 'rate_control' quantizer value
 *
 ****************************************************************************/

int
RateControlGetQ(RateControl * rate_control,
				int keyframe)
{
	return rate_control->rtn_quant;
}

/*****************************************************************************
 * RateControlUpdate
 *
 * This function is called once a coded frame to update all internal
 * parameters of 'rate_control'.
 *
 * Returned value : None
 *
 ****************************************************************************/

void
RateControlUpdate(RateControl * rate_control,
				  int16_t quant,
				  int frame_size,
				  int keyframe)
{
	int64_t deviation;
	double overflow, averaging_period, reaction_delay_factor;
	double quality_scale, base_quality, target_quality;
	int32_t rtn_quant;

	rate_control->frames++;
	rate_control->total_size += frame_size;

	deviation =
		(int64_t) ((double) rate_control->total_size -
				   ((double)
					((double) rate_control->target_rate / 8.0 /
					 (double) rate_control->framerate) *
					(double) rate_control->frames));

	DPRINTF(DPRINTF_DEBUG, "CBR: frame: %i, quant: %i, deviation: %i\n",
			(int32_t) (rate_control->frames - 1), rate_control->rtn_quant,
			(int32_t) deviation);

	if (rate_control->rtn_quant >= 2) {
		averaging_period = (double) rate_control->averaging_period;
		rate_control->sequence_quality -=
			rate_control->sequence_quality / averaging_period;
		rate_control->sequence_quality +=
			2.0 / (double) rate_control->rtn_quant / averaging_period;
		if (rate_control->sequence_quality < 0.1)
			rate_control->sequence_quality = 0.1;

		if (!keyframe) {
			reaction_delay_factor =
				(double) rate_control->reaction_delay_factor;
			rate_control->avg_framesize -=
				rate_control->avg_framesize / reaction_delay_factor;
			rate_control->avg_framesize += frame_size / reaction_delay_factor;
		}
	}

	quality_scale =
		rate_control->target_framesize / rate_control->avg_framesize *
		rate_control->target_framesize / rate_control->avg_framesize;

	base_quality = rate_control->sequence_quality;
	if (quality_scale >= 1.0) {
		base_quality = 1.0 - (1.0 - base_quality) / quality_scale;
	} else {
		base_quality = 0.06452 + (base_quality - 0.06452) * quality_scale;
	}

	overflow = -((double) deviation / (double) rate_control->buffer);

	target_quality =
		base_quality + (base_quality -
						0.06452) * overflow / rate_control->target_framesize;

	if (target_quality > 2.0)
		target_quality = 2.0;
	else if (target_quality < 0.06452)
		target_quality = 0.06452;

	rtn_quant = (int32_t) (2.0 / target_quality);

	if (rtn_quant < 31) {
		rate_control->quant_error[rtn_quant] +=
			2.0 / target_quality - rtn_quant;
		if (rate_control->quant_error[rtn_quant] >= 1.0) {
			rate_control->quant_error[rtn_quant] -= 1.0;
			rtn_quant++;
		}
	}

	if (rtn_quant > rate_control->rtn_quant + 1)
		rtn_quant = rate_control->rtn_quant + 1;
	else if (rtn_quant < rate_control->rtn_quant - 1)
		rtn_quant = rate_control->rtn_quant - 1;

	if (rtn_quant > rate_control->max_quant)
		rtn_quant = rate_control->max_quant;
	else if (rtn_quant < rate_control->min_quant)
		rtn_quant = rate_control->min_quant;

	rate_control->rtn_quant = rtn_quant;
}

/*****************************************************************************
 * Local functions
 ****************************************************************************/

static int
get_initial_quant(int bpp)
{
	return 5;
}
