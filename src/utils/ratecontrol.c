/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Rate Controler module -
 *
 *  Copyright(C) 2002 Benjamin Lambert <foxer@hotmail.com>
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
 * $Id: ratecontrol.c,v 1.18 2002-11-17 00:51:11 edgomez Exp $
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
