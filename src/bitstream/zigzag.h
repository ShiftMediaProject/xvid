/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - zigzag dct block scanning -
 *
 *  Copyright(C) 2001-2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: zigzag.h,v 1.4 2002-11-17 00:57:57 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _ZIGZAG_H_
#define _ZIGZAG_H_

static const uint16_t scan_tables[3][64] = {
	{							// zig_zag_scan
	 0, 1, 8, 16, 9, 2, 3, 10,
	 17, 24, 32, 25, 18, 11, 4, 5,
	 12, 19, 26, 33, 40, 48, 41, 34,
	 27, 20, 13, 6, 7, 14, 21, 28,
	 35, 42, 49, 56, 57, 50, 43, 36,
	 29, 22, 15, 23, 30, 37, 44, 51,
	 58, 59, 52, 45, 38, 31, 39, 46,
	 53, 60, 61, 54, 47, 55, 62, 63},

	{							// horizontal_scan
	 0, 1, 2, 3, 8, 9, 16, 17,
	 10, 11, 4, 5, 6, 7, 15, 14,
	 13, 12, 19, 18, 24, 25, 32, 33,
	 26, 27, 20, 21, 22, 23, 28, 29,
	 30, 31, 34, 35, 40, 41, 48, 49,
	 42, 43, 36, 37, 38, 39, 44, 45,
	 46, 47, 50, 51, 56, 57, 58, 59,
	 52, 53, 54, 55, 60, 61, 62, 63},

	{							// vertical_scan
	 0, 8, 16, 24, 1, 9, 2, 10,
	 17, 25, 32, 40, 48, 56, 57, 49,
	 41, 33, 26, 18, 3, 11, 4, 12,
	 19, 27, 34, 42, 50, 58, 35, 43,
	 51, 59, 20, 28, 5, 13, 6, 14,
	 21, 29, 36, 44, 52, 60, 37, 45,
	 53, 61, 22, 30, 7, 15, 23, 31,
	 38, 46, 54, 62, 39, 47, 55, 63}
};

#endif							/* _ZIGZAG_H_ */
