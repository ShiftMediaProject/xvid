/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Simple profiling functions header file -
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
 * $Id: timer.h,v 1.7 2002-11-17 00:51:11 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

#ifdef _PROFILING_

#include "../portab.h"

uint64_t count_frames;

/* Here we declare external symbols defined in timer.h */
extern void start_timer();
extern void start_global_timer();
extern void stop_dct_timer();
extern void stop_idct_timer();
extern void stop_motion_timer();
extern void stop_comp_timer();
extern void stop_edges_timer();
extern void stop_inter_timer();
extern void stop_quant_timer();
extern void stop_iquant_timer();
extern void stop_conv_timer();
extern void stop_transfer_timer();
extern void stop_coding_timer();
extern void stop_prediction_timer();
extern void stop_interlacing_timer();
extern void stop_global_timer();
extern void init_timer();
extern void write_timer();

#else

/*
 * We define dummy inline functions, a good compiler should just
 *  remove the function call
 */

static __inline void
start_timer()
{
}
static __inline void
start_global_timer()
{
}
static __inline void
stop_dct_timer()
{
}
static __inline void
stop_idct_timer()
{
}
static __inline void
stop_motion_timer()
{
}
static __inline void
stop_comp_timer()
{
}
static __inline void
stop_edges_timer()
{
}
static __inline void
stop_inter_timer()
{
}
static __inline void
stop_quant_timer()
{
}
static __inline void
stop_iquant_timer()
{
}
static __inline void
stop_conv_timer()
{
}
static __inline void
stop_transfer_timer()
{
}
static __inline void
init_timer()
{
}
static __inline void
write_timer()
{
}
static __inline void
stop_coding_timer()
{
}
static __inline void
stop_interlacing_timer()
{
}
static __inline void
stop_prediction_timer()
{
}
static __inline void
stop_global_timer()
{
}

#endif /* _PROFILING_ */

#endif /* _TIMER_H_ */
