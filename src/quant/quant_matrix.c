/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Custom matrix quantization functions -
 *
 *  Copyright(C) 2002 Michael Militzer
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
 *  $Id: quant_matrix.c,v 1.7 2002-09-07 14:16:13 edgomez Exp $
 *
 ****************************************************************************/

#include "quant_matrix.h"

#define FIX(X) (1 << 16) / (X) + 1

/*****************************************************************************
 * Local data
 ****************************************************************************/

/* ToDo : remove all this local to make this module thread safe */
uint8_t custom_intra_matrix = 0;
uint8_t custom_inter_matrix = 0;

uint8_t const default_intra_matrix[64] = {
	8, 17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45
};

int16_t intra_matrix[64] = {
	8, 17, 18, 19, 21, 23, 25, 27,
	17, 18, 19, 21, 23, 25, 27, 28,
	20, 21, 22, 23, 24, 26, 28, 30,
	21, 22, 23, 24, 26, 28, 30, 32,
	22, 23, 24, 26, 28, 30, 32, 35,
	23, 24, 26, 28, 30, 32, 35, 38,
	25, 26, 28, 30, 32, 35, 38, 41,
	27, 28, 30, 32, 35, 38, 41, 45
};

int16_t intra_matrix_fix[64] = {
	FIX(8), FIX(17), FIX(18), FIX(19), FIX(21), FIX(23), FIX(25), FIX(27),
	FIX(17), FIX(18), FIX(19), FIX(21), FIX(23), FIX(25), FIX(27), FIX(28),
	FIX(20), FIX(21), FIX(22), FIX(23), FIX(24), FIX(26), FIX(28), FIX(30),
	FIX(21), FIX(22), FIX(23), FIX(24), FIX(26), FIX(28), FIX(30), FIX(32),
	FIX(22), FIX(23), FIX(24), FIX(26), FIX(28), FIX(30), FIX(32), FIX(35),
	FIX(23), FIX(24), FIX(26), FIX(28), FIX(30), FIX(32), FIX(35), FIX(38),
	FIX(25), FIX(26), FIX(28), FIX(30), FIX(32), FIX(35), FIX(38), FIX(41),
	FIX(27), FIX(28), FIX(30), FIX(32), FIX(35), FIX(38), FIX(41), FIX(45)
};

uint8_t const default_inter_matrix[64] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
};

int16_t inter_matrix[64] = {
	16, 17, 18, 19, 20, 21, 22, 23,
	17, 18, 19, 20, 21, 22, 23, 24,
	18, 19, 20, 21, 22, 23, 24, 25,
	19, 20, 21, 22, 23, 24, 26, 27,
	20, 21, 22, 23, 25, 26, 27, 28,
	21, 22, 23, 24, 26, 27, 28, 30,
	22, 23, 24, 26, 27, 28, 30, 31,
	23, 24, 25, 27, 28, 30, 31, 33
};

int16_t inter_matrix_fix[64] = {
	FIX(16), FIX(17), FIX(18), FIX(19), FIX(20), FIX(21), FIX(22), FIX(23),
	FIX(17), FIX(18), FIX(19), FIX(20), FIX(21), FIX(22), FIX(23), FIX(24),
	FIX(18), FIX(19), FIX(20), FIX(21), FIX(22), FIX(23), FIX(24), FIX(25),
	FIX(19), FIX(20), FIX(21), FIX(22), FIX(23), FIX(24), FIX(26), FIX(27),
	FIX(20), FIX(21), FIX(22), FIX(23), FIX(25), FIX(26), FIX(27), FIX(28),
	FIX(21), FIX(22), FIX(23), FIX(24), FIX(26), FIX(27), FIX(28), FIX(30),
	FIX(22), FIX(23), FIX(24), FIX(26), FIX(27), FIX(28), FIX(30), FIX(31),
	FIX(23), FIX(24), FIX(25), FIX(27), FIX(28), FIX(30), FIX(31), FIX(33)
};

/*****************************************************************************
 * Functions
 ****************************************************************************/

uint8_t
get_intra_matrix_status(void)
{
	return custom_intra_matrix;
}

uint8_t
get_inter_matrix_status(void)
{
	return custom_inter_matrix;
}

void
set_intra_matrix_status(uint8_t status)
{
	custom_intra_matrix = status;
}

void
set_inter_matrix_status(uint8_t status)
{
	custom_inter_matrix = status;
}

int16_t *
get_intra_matrix(void)
{
	return intra_matrix;
}

int16_t *
get_inter_matrix(void)
{
	return inter_matrix;
}

uint8_t *
get_default_intra_matrix(void)
{
	return default_intra_matrix;
}

uint8_t *
get_default_inter_matrix(void)
{
	return default_inter_matrix;
}

uint8_t
set_intra_matrix(uint8_t * matrix)
{
	int i, change = 0;

	custom_intra_matrix = 0;

	for (i = 0; i < 64; i++) {
		if ((int16_t) default_intra_matrix[i] != matrix[i])
			custom_intra_matrix = 1;
		if (intra_matrix[i] != matrix[i])
			change = 1;

		intra_matrix[i] = (int16_t) matrix[i];
		intra_matrix_fix[i] = FIX(intra_matrix[i]);
	}
	return custom_intra_matrix | change;
}


uint8_t
set_inter_matrix(uint8_t * matrix)
{
	int i, change = 0;

	custom_inter_matrix = 0;

	for (i = 0; i < 64; i++) {
		if ((int16_t) default_inter_matrix[i] != matrix[i])
			custom_inter_matrix = 1;
		if (inter_matrix[i] != matrix[i])
			change = 1;

		inter_matrix[i] = (int16_t) matrix[i];
		inter_matrix_fix[i] = FIX(inter_matrix[i]);
	}
	return custom_inter_matrix | change;
}
