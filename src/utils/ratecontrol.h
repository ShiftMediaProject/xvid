/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 1 Pass rate controler header file -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: ratecontrol.h,v 1.8 2002-11-17 00:51:11 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _RATECONTROL_H_
#define _RATECONTROL_H_

#include "../portab.h"

typedef struct
{
	int64_t size;
	int32_t count;
}
QuantInfo;

typedef struct
{
	int32_t rtn_quant;
	int64_t frames;
	int64_t total_size;
	double framerate;
	int32_t target_rate;
	int16_t max_quant;
	int16_t min_quant;
	int64_t last_change;
	int64_t quant_sum;
	double quant_error[32];
	double avg_framesize;
	double target_framesize;
	double sequence_quality;
	int32_t averaging_period;
	int32_t reaction_delay_factor;
	int32_t buffer;
}
RateControl;

void RateControlInit(RateControl * rate_control,
					 uint32_t target_rate,
					 uint32_t reaction_delay_factor,
					 uint32_t averaging_period,
					 uint32_t buffer,
					 int framerate,
					 int max_quant,
					 int min_quant);

int RateControlGetQ(RateControl * rate_control,
					int keyframe);

void RateControlUpdate(RateControl * rate_control,
					   int16_t quant,
					   int frame_size,
					   int keyframe);

#endif /* _RATECONTROL_H_ */
