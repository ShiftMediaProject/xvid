/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Macro Block coding functions -
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
 * $Id: mbcoding.c,v 1.36 2003-01-04 04:28:48 suxen_drol Exp $
 *
 ****************************************************************************/

#include <stdlib.h>
#include "../portab.h"
#include "bitstream.h"
#include "zigzag.h"
#include "vlc_codes.h"
#include "mbcoding.h"

#include "../utils/mbfunctions.h"

#define ABS(X) (((X)>0)?(X):-(X))
#define CLIP(X,A) (X > A) ? (A) : (X)

/*****************************************************************************
 * Local data
 ****************************************************************************/

/* msvc sp5+pp gets confused if they globals are made static */
static VLC intra_table[524288];
static VLC inter_table[524288];

static VLC DCT3Dintra[4096];
static VLC DCT3Dinter[4096];

/*****************************************************************************
 * Vector Length Coding Initialization
 ****************************************************************************/

void
init_vlc_tables(void)
{

	int32_t k, l, i, intra, last;
	VLC *vlc[2];
	VLC const **coeff_ptr;
	VLC *vlc1, *vlc2;

	vlc1 = DCT3Dintra;
	vlc2 = DCT3Dinter;

	vlc[0] = intra_table;
	vlc[1] = inter_table;

	/*
	 * Generate encoding vlc lookup tables
	 * the lookup table idea is taken from the excellent fame project
	 * by Vivien Chapellier
	 */
	for (i = 0; i < 4; i++) {
		intra = i % 2;
		last = i / 2;

		coeff_ptr = coeff_vlc[last + 2 * intra];

		for (k = -2047; k < 2048; k++) {	/* level */
			int8_t const *max_level_ptr = max_level[last + 2 * intra];
			int8_t const *max_run_ptr = max_run[last + 2 * intra];

			for (l = 0; l < 64; l++) {	/* run */
				int32_t level = k;
				ptr_t run = l;

				if ((abs(level) <= max_level_ptr[run]) && (run <= (uint32_t) max_run_ptr[abs(level)])) {	/* level < max_level and run < max_run */

					vlc[intra]->code = 0;
					vlc[intra]->len = 0;
					goto loop_end;
				} else {
					if (level > 0)	/* correct level */
						level -= max_level_ptr[run];
					else
						level += max_level_ptr[run];

					if ((abs(level) <= max_level_ptr[run]) &&
						(run <= (uint32_t) max_run_ptr[abs(level)])) {

						vlc[intra]->code = 0x06;
						vlc[intra]->len = 8;
						goto loop_end;
					}

					if (level > 0)	/* still here? */
						level += max_level_ptr[run];	/* restore level */
					else
						level -= max_level_ptr[run];

					run -= max_run_ptr[abs(level)] + 1;	/* and change run */

					if ((abs(level) <= max_level_ptr[run]) &&
						(run <= (uint32_t) max_run_ptr[abs(level)])) {

						vlc[intra]->code = 0x0e;
						vlc[intra]->len = 9;
						goto loop_end;
					}
					run += max_run_ptr[abs(level)] + 1;
				}

				vlc[intra]->code =
					(uint32_t) ((l << 14) | (0x1e + last) << 20) | (1 << 13) |
					((k & 0xfff) << 1) | 1;

				vlc[intra]->len = 30;
				vlc[intra]++;
				continue;

			  loop_end:
				if (level != 0) {
					vlc[intra]->code =
						(vlc[intra]->
						 code << (coeff_ptr[run][abs(level) - 1].len +
								  1)) | (coeff_ptr[run][abs(level) -
														1].code << 1);
					vlc[intra]->len =
						(coeff_ptr[run][abs(level) - 1].len + 1) +
						vlc[intra]->len;

					if (level < 0)
						vlc[intra]->code += 1;
				}

				vlc[intra]++;
			}
		}
	}

	for (i = 0; i < 4096; i++) {
		if (i >= 512) {
			*vlc1 = DCT3Dtab3[(i >> 5) - 16];
			*vlc2 = DCT3Dtab0[(i >> 5) - 16];
		} else if (i >= 128) {
			*vlc1 = DCT3Dtab4[(i >> 2) - 32];
			*vlc2 = DCT3Dtab1[(i >> 2) - 32];
		} else if (i >= 8) {
			*vlc1 = DCT3Dtab5[i - 8];
			*vlc2 = DCT3Dtab2[i - 8];
		} else {
			*vlc1 = ERRtab[i];
			*vlc2 = ERRtab[i];
		}

		vlc1++;
		vlc2++;
	}
	DCT3D[0] = DCT3Dinter;
	DCT3D[1] = DCT3Dintra;

}

/*****************************************************************************
 * Local inlined functions for MB coding
 ****************************************************************************/

static __inline void
CodeVector(Bitstream * bs,
		   int32_t value,
		   int32_t f_code,
		   Statistics * pStat)
{

	const int scale_factor = 1 << (f_code - 1);
	const int cmp = scale_factor << 5;

	if (value < (-1 * cmp))
		value += 64 * scale_factor;

	if (value > (cmp - 1))
		value -= 64 * scale_factor;

	pStat->iMvSum += value * value;
	pStat->iMvCount++;

	if (value == 0) {
		BitstreamPutBits(bs, mb_motion_table[32].code,
						 mb_motion_table[32].len);
	} else {
		uint16_t length, code, mv_res, sign;

		length = 16 << f_code;
		f_code--;

		sign = (value < 0);

		if (value >= length)
			value -= 2 * length;
		else if (value < -length)
			value += 2 * length;

		if (sign)
			value = -value;

		value--;
		mv_res = value & ((1 << f_code) - 1);
		code = ((value - mv_res) >> f_code) + 1;

		if (sign)
			code = -code;

		code += 32;
		BitstreamPutBits(bs, mb_motion_table[code].code,
						 mb_motion_table[code].len);

		if (f_code)
			BitstreamPutBits(bs, mv_res, f_code);
	}

}

static __inline void
CodeCoeff(Bitstream * bs,
		  const int16_t qcoeff[64],
		  VLC * table,
		  const uint16_t * zigzag,
		  uint16_t intra)
{

	uint32_t j, last;
	short v;
	VLC *vlc;

	j = intra;
	last = intra;

	while (j < 64 && (v = qcoeff[zigzag[j]]) == 0)
		j++;

	do {
		vlc = table + 64 * 2047 + (v << 6) + j - last;
		last = ++j;

		/* count zeroes */
		while (j < 64 && (v = qcoeff[zigzag[j]]) == 0)
			j++;

		/* write code */
		if (j != 64) {
			BitstreamPutBits(bs, vlc->code, vlc->len);
		} else {
			vlc += 64 * 4095;
			BitstreamPutBits(bs, vlc->code, vlc->len);
			break;
		}
	} while (1);

}

/*****************************************************************************
 * Local functions
 ****************************************************************************/

static void
CodeBlockIntra(const FRAMEINFO * frame,
			   const MACROBLOCK * pMB,
			   int16_t qcoeff[6 * 64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	uint32_t i, mcbpc, cbpy, bits;

	cbpy = pMB->cbp >> 2;

	/* write mcbpc */
	if (frame->coding_type == I_VOP) {
		mcbpc = ((pMB->mode >> 1) & 3) | ((pMB->cbp & 3) << 2);
		BitstreamPutBits(bs, mcbpc_intra_tab[mcbpc].code,
						 mcbpc_intra_tab[mcbpc].len);
	} else {
		mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
		BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code,
						 mcbpc_inter_tab[mcbpc].len);
	}

	/* ac prediction flag */
	if (pMB->acpred_directions[0])
		BitstreamPutBits(bs, 1, 1);
	else
		BitstreamPutBits(bs, 0, 1);

	/* write cbpy */
	BitstreamPutBits(bs, cbpy_tab[cbpy].code, cbpy_tab[cbpy].len);

	/* write dquant */
	if (pMB->mode == MODE_INTRA_Q)
		BitstreamPutBits(bs, pMB->dquant, 2);

	/* write interlacing */
	if (frame->global_flags & XVID_INTERLACING) {
		BitstreamPutBit(bs, pMB->field_dct);
	}
	/* code block coeffs */
	for (i = 0; i < 6; i++) {
		if (i < 4)
			BitstreamPutBits(bs, dcy_tab[qcoeff[i * 64 + 0] + 255].code,
							 dcy_tab[qcoeff[i * 64 + 0] + 255].len);
		else
			BitstreamPutBits(bs, dcc_tab[qcoeff[i * 64 + 0] + 255].code,
							 dcc_tab[qcoeff[i * 64 + 0] + 255].len);

		if (pMB->cbp & (1 << (5 - i))) {
			bits = BitstreamPos(bs);

			CodeCoeff(bs, &qcoeff[i * 64], intra_table,
					  scan_tables[pMB->acpred_directions[i]], 1);

			bits = BitstreamPos(bs) - bits;
			pStat->iTextBits += bits;
		}
	}

}


static void
CodeBlockInter(const FRAMEINFO * frame,
			   const MACROBLOCK * pMB,
			   int16_t qcoeff[6 * 64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	int32_t i;
	uint32_t bits, mcbpc, cbpy;

	mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
	cbpy = 15 - (pMB->cbp >> 2);

	/* write mcbpc */
	BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code,
					 mcbpc_inter_tab[mcbpc].len);

	/* write cbpy */
	BitstreamPutBits(bs, cbpy_tab[cbpy].code, cbpy_tab[cbpy].len);

	/* write dquant */
	if (pMB->mode == MODE_INTER_Q)
		BitstreamPutBits(bs, pMB->dquant, 2);

	/* interlacing */
	if (frame->global_flags & XVID_INTERLACING) {
		if (pMB->cbp) {
			BitstreamPutBit(bs, pMB->field_dct);
			DPRINTF(DPRINTF_DEBUG, "codep: field_dct: %d", pMB->field_dct);
		}

		/* if inter block, write field ME flag */
		if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q) {
			BitstreamPutBit(bs, pMB->field_pred);
			DPRINTF(DPRINTF_DEBUG, "codep: field_pred: %d", pMB->field_pred);

			/* write field prediction references */
			if (pMB->field_pred) {
				BitstreamPutBit(bs, pMB->field_for_top);
				BitstreamPutBit(bs, pMB->field_for_bot);
			}
		}
	}
	/* code motion vector(s) */
	for (i = 0; i < (pMB->mode == MODE_INTER4V ? 4 : 1); i++) {
		CodeVector(bs, pMB->pmvs[i].x, frame->fcode, pStat);
		CodeVector(bs, pMB->pmvs[i].y, frame->fcode, pStat);
	}

	bits = BitstreamPos(bs);

	/* code block coeffs */
	for (i = 0; i < 6; i++)
		if (pMB->cbp & (1 << (5 - i)))
			CodeCoeff(bs, &qcoeff[i * 64], inter_table, scan_tables[0], 0);

	bits = BitstreamPos(bs) - bits;
	pStat->iTextBits += bits;

}

/*****************************************************************************
 * Macro Block bitstream encoding functions
 ****************************************************************************/

void
MBCoding(const FRAMEINFO * frame,
		 MACROBLOCK * pMB,
		 int16_t qcoeff[6 * 64],
		 Bitstream * bs,
		 Statistics * pStat)
{

	if (frame->coding_type == P_VOP) {
			BitstreamPutBit(bs, 0);	/* coded */
	}

	if (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q)
		CodeBlockIntra(frame, pMB, qcoeff, bs, pStat);
	else
		CodeBlockInter(frame, pMB, qcoeff, bs, pStat);

}


void
MBSkip(Bitstream * bs)
{
	BitstreamPutBit(bs, 1);	/* not coded */
	return;
}

/*****************************************************************************
 * decoding stuff starts here
 ****************************************************************************/

/*
 * For IVOP addbits == 0
 * For PVOP addbits == fcode - 1
 * For BVOP addbits == max(fcode,bcode) - 1
 * returns true or false
 */

int 
check_resync_marker(Bitstream * bs, int addbits)
{
	uint32_t nbits;
	uint32_t code;
	uint32_t nbitsresyncmarker = NUMBITS_VP_RESYNC_MARKER + addbits;

	nbits = BitstreamNumBitsToByteAlign(bs);
	code = BitstreamShowBits(bs, nbits);

	if (code == (((uint32_t)1 << (nbits - 1)) - 1))
	{
		return BitstreamShowBitsFromByteAlign(bs, nbitsresyncmarker) == RESYNC_MARKER;
	}

	return 0;
}



int
get_mcbpc_intra(Bitstream * bs)
{

	uint32_t index;

	index = BitstreamShowBits(bs, 9);
	index >>= 3;

	BitstreamSkip(bs, mcbpc_intra_table[index].len);

	return mcbpc_intra_table[index].code;

}

int
get_mcbpc_inter(Bitstream * bs)
{

	uint32_t index;
	
	index = CLIP(BitstreamShowBits(bs, 9), 256);

	BitstreamSkip(bs, mcbpc_inter_table[index].len);

	return mcbpc_inter_table[index].code;

}

int
get_cbpy(Bitstream * bs,
		 int intra)
{

	int cbpy;
	uint32_t index = BitstreamShowBits(bs, 6);

	BitstreamSkip(bs, cbpy_table[index].len);
	cbpy = cbpy_table[index].code;

	if (!intra)
		cbpy = 15 - cbpy;

	return cbpy;

}

int
get_mv_data(Bitstream * bs)
{

	uint32_t index;

	if (BitstreamGetBit(bs))
		return 0;

	index = BitstreamShowBits(bs, 12);

	if (index >= 512) {
		index = (index >> 8) - 2;
		BitstreamSkip(bs, TMNMVtab0[index].len);
		return TMNMVtab0[index].code;
	}

	if (index >= 128) {
		index = (index >> 2) - 32;
		BitstreamSkip(bs, TMNMVtab1[index].len);
		return TMNMVtab1[index].code;
	}

	index -= 4;

	BitstreamSkip(bs, TMNMVtab2[index].len);
	return TMNMVtab2[index].code;

}

int
get_mv(Bitstream * bs,
	   int fcode)
{

	int data;
	int res;
	int mv;
	int scale_fac = 1 << (fcode - 1);

	data = get_mv_data(bs);

	if (scale_fac == 1 || data == 0)
		return data;

	res = BitstreamGetBits(bs, fcode - 1);
	mv = ((ABS(data) - 1) * scale_fac) + res + 1;

	return data < 0 ? -mv : mv;

}

int
get_dc_dif(Bitstream * bs,
		   uint32_t dc_size)
{

	int code = BitstreamGetBits(bs, dc_size);
	int msb = code >> (dc_size - 1);

	if (msb == 0)
		return (-1 * (code ^ ((1 << dc_size) - 1)));

	return code;

}

int
get_dc_size_lum(Bitstream * bs)
{

	int code, i;

	code = BitstreamShowBits(bs, 11);

	for (i = 11; i > 3; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i + 1;
		}
		code >>= 1;
	}

	BitstreamSkip(bs, dc_lum_tab[code].len);
	return dc_lum_tab[code].code;

}


int
get_dc_size_chrom(Bitstream * bs)
{

	uint32_t code, i;

	code = BitstreamShowBits(bs, 12);

	for (i = 12; i > 2; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i;
		}
		code >>= 1;
	}

	return 3 - BitstreamGetBits(bs, 2);

}

/*****************************************************************************
 * Local inlined function to "decode" written vlc codes
 ****************************************************************************/

static __inline int
get_coeff(Bitstream * bs,
		  int *run,
		  int *last,
		  int intra,
		  int short_video_header)
{

	uint32_t mode;
	const VLC *tab;
	int32_t level;

	if (short_video_header)		/* inter-VLCs will be used for both intra and inter blocks */
		intra = 0;

	tab = &DCT3D[intra][BitstreamShowBits(bs, 12)];

	if (tab->code == -1)
		goto error;

	BitstreamSkip(bs, tab->len);

	if (tab->code != ESCAPE) {
		if (!intra) {
			*run = (tab->code >> 4) & 255;
			level = tab->code & 15;
			*last = (tab->code >> 12) & 1;
		} else {
			*run = (tab->code >> 8) & 255;
			level = tab->code & 255;
			*last = (tab->code >> 16) & 1;
		}
		return BitstreamGetBit(bs) ? -level : level;
	}

	if (short_video_header) {
		/* escape mode 4 - H.263 type, only used if short_video_header = 1  */
		*last = BitstreamGetBit(bs);
		*run = BitstreamGetBits(bs, 6);
		level = BitstreamGetBits(bs, 8);

		if (level == 0 || level == 128)
			DPRINTF(DPRINTF_ERROR, "Illegal LEVEL for ESCAPE mode 4: %d", level);

		return (level >= 128 ? -(256 - level) : level);
	}

	mode = BitstreamShowBits(bs, 2);

	if (mode < 3) {
		BitstreamSkip(bs, (mode == 2) ? 2 : 1);

		tab = &DCT3D[intra][BitstreamShowBits(bs, 12)];
		if (tab->code == -1)
			goto error;

		BitstreamSkip(bs, tab->len);

		if (!intra) {
			*run = (tab->code >> 4) & 255;
			level = tab->code & 15;
			*last = (tab->code >> 12) & 1;
		} else {
			*run = (tab->code >> 8) & 255;
			level = tab->code & 255;
			*last = (tab->code >> 16) & 1;
		}

		if (mode < 2)			/* first escape mode, level is offset */
			level += max_level[*last + (!intra << 1)][*run];	/* need to add back the max level */
		else if (mode == 2)		/* second escape mode, run is offset */
			*run += max_run[*last + (!intra << 1)][level] + 1;

		return BitstreamGetBit(bs) ? -level : level;
	}
	/* third escape mode - fixed length codes */
	BitstreamSkip(bs, 2);
	*last = BitstreamGetBits(bs, 1);
	*run = BitstreamGetBits(bs, 6);
	BitstreamSkip(bs, 1);		/* marker */
	level = BitstreamGetBits(bs, 12);
	BitstreamSkip(bs, 1);		/* marker */

	return (level & 0x800) ? (level | (-1 ^ 0xfff)) : level;

  error:
	*run = VLC_ERROR;
	return 0;

}

/*****************************************************************************
 * MB reading functions
 ****************************************************************************/

void
get_intra_block(Bitstream * bs,
				int16_t * block,
				int direction,
				int coeff)
{

	const uint16_t *scan = scan_tables[direction];
	int level;
	int run;
	int last;

	do {
		level = get_coeff(bs, &run, &last, 1, 0);
		if (run == -1) {
			DPRINTF(DPRINTF_DEBUG, "fatal: invalid run");
			break;
		}
		coeff += run;
		block[scan[coeff]] = level;

		DPRINTF(DPRINTF_COEFF,"block[%i] %i", scan[coeff], level);
		/*DPRINTF(DPRINTF_COEFF,"block[%i] %i %08x", scan[coeff], level, BitstreamShowBits(bs, 32)); */

		if (level < -2047 || level > 2047) {
			DPRINTF(DPRINTF_DEBUG, "warning: intra_overflow: %d", level);
		}
		coeff++;
	} while (!last);

}

void
get_inter_block(Bitstream * bs,
				int16_t * block)
{

	const uint16_t *scan = scan_tables[0];
	int p;
	int level;
	int run;
	int last;

	p = 0;
	do {
		level = get_coeff(bs, &run, &last, 0, 0);
		if (run == -1) {
			DPRINTF(DPRINTF_ERROR, "fatal: invalid run");
			break;
		}
		p += run;

		block[scan[p]] = level;

		DPRINTF(DPRINTF_COEFF,"block[%i] %i", scan[p], level);

		if (level < -2047 || level > 2047) {
			DPRINTF(DPRINTF_DEBUG, "warning: inter_overflow: %d", level);
		}
		p++;
	} while (!last);

}
