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
  *																			   *	
  *  Revision history:                                                         *
  *                                                                            *
  *  14.04.2002 bframe encoding
  *  08.03.2002 initial version; isibaar					                   *
  *																			   *
  ******************************************************************************/



#include <stdlib.h>
#include "../portab.h"
#include "bitstream.h"
#include "zigzag.h"
#include "vlc_codes.h"
#include "mbcoding.h"

#include "../utils/mbfunctions.h"

#define ABS(X) (((X)>0)?(X):-(X))
#define CLIP(X,A) (X > A) ? (A) : (X)

VLC intra_table[65536]; 
VLC inter_table[65536];

VLC DCT3Dintra[4096];
VLC DCT3Dinter[4096];

static int16_t clip_table[4096];

void init_vlc_tables(void)
{

	int32_t k, l, i, intra, last;
	VLC *vlc[2];
	VLC **coeff_ptr;
	VLC *vlc1, *vlc2;

	vlc1 = DCT3Dintra;
	vlc2 = DCT3Dinter;
	
	vlc[0] = intra_table;
	vlc[1] = inter_table;
	
	// initialize the clipping table
	for(i = -2048; i < 2048; i++) {
		clip_table[i + 2048] = i;
		if(i < -255)
			clip_table[i + 2048] = -255;
		if(i > 255)
			clip_table[i + 2048] = 255;
	}
	
	// generate encoding vlc lookup tables
	for(i = 0; i < 4; i++) {
		intra = i % 2;
		last = i / 2;

		coeff_ptr = coeff_vlc[last + 2 * intra];
			
		for(k = -255; k < 256; k++) { // level
			int8_t *max_level_ptr = max_level[last + 2 * intra];
			int8_t *max_run_ptr = max_run[last + 2 * intra];
	
			for(l = 0; l < 64; l++) { // run
				int32_t level = k;
				uint32_t run = l;
			
				if((abs(level) <= max_level_ptr[run]) &&
				   (run <= (uint32_t)max_run_ptr[abs(level)])) { // level < max_level and run < max_run
													   
						vlc[intra]->code = 0;
						vlc[intra]->len = 0;
						goto loop_end;
				}
				else {
					if(level > 0)					     // correct level
						level -= max_level_ptr[run];
					else
						level += max_level_ptr[run];

					if((abs(level) <= max_level_ptr[run]) &&
					   (run <= (uint32_t) max_run_ptr[abs(level)])) {
						
						vlc[intra]->code = 0x06;
						vlc[intra]->len = 8;
						goto loop_end;
					}

					if(level > 0)					    // still here?
						level += max_level_ptr[run];    // restore level
					else
						level -= max_level_ptr[run];

					run -= max_run_ptr[abs(level)] + 1; // and change run

					if((abs(level) <= max_level_ptr[run]) &&
					   (run <= (uint32_t) max_run_ptr[abs(level)])) {
						
						vlc[intra]->code = 0x0e;
						vlc[intra]->len = 9;
						goto loop_end;
					}
					run += max_run_ptr[abs(level)] + 1;
				}

				vlc[intra]->code = (uint32_t) ((l << 14) | (0x1e + last) << 20) |
											  (1 << 13) | ((k & 0xfff) << 1) | 1;

				vlc[intra]->len = 30;
				vlc[intra]++;
				continue;

loop_end:
				if(level != 0) {
					vlc[intra]->code = (vlc[intra]->code << (coeff_ptr[run][abs(level) - 1].len + 1)) |
									   (coeff_ptr[run][abs(level) - 1].code << 1);
					vlc[intra]->len = (coeff_ptr[run][abs(level) - 1].len + 1) + vlc[intra]->len;

					if(level < 0)
						vlc[intra]->code += 1;
				}

				vlc[intra]++;
			}
		}
	}

	for(i = 0; i < 4096; i++) {
		if(i >= 512) {
			*vlc1 = DCT3Dtab3[(i >> 5) - 16];
			*vlc2 = DCT3Dtab0[(i >> 5) - 16];
		}
		else if(i >= 128) {
			*vlc1 = DCT3Dtab4[(i >> 2) - 32];
			*vlc2 = DCT3Dtab1[(i >> 2) - 32];
		}
		else if(i >= 8) {
			*vlc1 = DCT3Dtab5[i - 8];
			*vlc2 = DCT3Dtab2[i - 8];
		}
		else {
			*vlc1 = ERRtab[i];
			*vlc2 = ERRtab[i];
		}

		vlc1++;
		vlc2++;
	}
	DCT3D[0] = DCT3Dinter;
	DCT3D[1] = DCT3Dintra;

}

static __inline void CodeVector(Bitstream *bs,
				int32_t value,
				int32_t f_code,
				Statistics *pStat)
{

	const int scale_factor = 1 << (f_code - 1);
	const int cmp = scale_factor << 5;

	if(value < (-1 * cmp))
		value += 64 * scale_factor;
	
	if(value > (cmp - 1))
		value -= 64 * scale_factor;

	pStat->iMvSum += value * value;
	pStat->iMvCount++;

	if (value == 0) {
		BitstreamPutBits(bs, mb_motion_table[32].code, mb_motion_table[32].len);
	} else {
		uint16_t length, code, mv_res, sign;
		
		length = 16 << f_code;
		f_code--;
		
		sign = (value < 0);

		if(value >= length)
			value -= 2 * length;
		else if(value < -length)
			value += 2 * length;

		if(sign)
			value = -value;

		value--;
		mv_res = value & ((1 << f_code) - 1);
		code = ((value - mv_res) >> f_code) + 1;

		if(sign)
			code = -code;

		code += 32;
		BitstreamPutBits(bs, mb_motion_table[code].code, mb_motion_table[code].len);
		
		if(f_code)
			BitstreamPutBits(bs, mv_res, f_code);
	}

}


static __inline void CodeCoeff(Bitstream *bs,
			       const int16_t qcoeff[64],
			       VLC *table,
			       const uint16_t *zigzag,
			       uint16_t intra)
{

	uint32_t j, last;
	short v;
	VLC *vlc;
	
	j = intra;
	last = intra;

	while((v = qcoeff[zigzag[j]]) == 0) j++;
    
	do {
		// count zeroes
		vlc = table + 64*255 + (clip_table[2048+v] << 6) + j - last;
		last = ++j;
		while(j < 64 && (v = qcoeff[zigzag[j]]) == 0) j++;
				
		// write code
		if(j != 64) {
			BitstreamPutBits(bs, vlc->code, vlc->len);
		} else {
			vlc += 64*511;
			BitstreamPutBits(bs, vlc->code, vlc->len);
			break;
		}
	} while(1);

}


static void CodeBlockIntra(const FRAMEINFO * frame,
			   const MACROBLOCK *pMB,
			   int16_t qcoeff[6*64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	uint32_t i, mcbpc, cbpy, bits;

	cbpy = pMB->cbp >> 2;

	// write mcbpc
	if(frame->coding_type == I_VOP) {
		mcbpc = ((pMB->mode >> 1) & 3) | ((pMB->cbp & 3) << 2);
		BitstreamPutBits(bs, mcbpc_intra_tab[mcbpc].code, mcbpc_intra_tab[mcbpc].len);
	}
	else {
		mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
		BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code, mcbpc_inter_tab[mcbpc].len);
	}

	// ac prediction flag
	if(pMB->acpred_directions[0])
		BitstreamPutBits(bs, 1, 1);
	else
		BitstreamPutBits(bs, 0, 1);

	// write cbpy
	BitstreamPutBits (bs, cbpy_tab[cbpy].code, cbpy_tab[cbpy].len);

	// write dquant
	if(pMB->mode == MODE_INTRA_Q)
		BitstreamPutBits(bs, pMB->dquant, 2);

	// write interlacing
	if (frame->global_flags & XVID_INTERLACING)
	{
		BitstreamPutBit(bs, pMB->field_dct);
	}

	// code block coeffs
	for(i = 0; i < 6; i++)
	{
		if(i < 4)
			BitstreamPutBits(bs,
					 dcy_tab[qcoeff[i*64 + 0] + 255].code,
					 dcy_tab[qcoeff[i*64 + 0] + 255].len);
		else
			BitstreamPutBits(bs,
					 dcc_tab[qcoeff[i*64 + 0] + 255].code,
			                 dcc_tab[qcoeff[i*64 + 0] + 255].len);
		
		if(pMB->cbp & (1 << (5 - i)))
		{
			bits = BitstreamPos(bs);

			CodeCoeff(bs,
				  &qcoeff[i*64],
				  intra_table,
				  scan_tables[pMB->acpred_directions[i]],
				  1);

			bits = BitstreamPos(bs) - bits;
			pStat->iTextBits += bits;
		}
	}

}


static void CodeBlockInter(const FRAMEINFO * frame,
			   const MACROBLOCK *pMB,
			   int16_t qcoeff[6*64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	int32_t i;
	uint32_t bits, mcbpc, cbpy;

	mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
	cbpy = 15 - (pMB->cbp >> 2);

	// write mcbpc
	BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code, mcbpc_inter_tab[mcbpc].len);

	// write cbpy
	BitstreamPutBits(bs, cbpy_tab[cbpy].code, cbpy_tab[cbpy].len);

	// write dquant
	if(pMB->mode == MODE_INTER_Q)
		BitstreamPutBits(bs, pMB->dquant, 2);
    
	// interlacing
	if (frame->global_flags & XVID_INTERLACING)
	{
		BitstreamPutBit(bs, pMB->field_dct);
		DEBUG1("codep: field_dct: ", pMB->field_dct);

		// if inter block, write field ME flag
		if (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q)
		{
			BitstreamPutBit(bs, pMB->field_pred);
			DEBUG1("codep: field_pred: ", pMB->field_pred);

			// write field prediction references
			if (pMB->field_pred)
			{
				BitstreamPutBit(bs, pMB->field_for_top);
				BitstreamPutBit(bs, pMB->field_for_bot);
			}
		}
	}

	// code motion vector(s)
	for(i = 0; i < (pMB->mode == MODE_INTER4V ? 4 : 1); i++)
	{
		CodeVector(bs, pMB->pmvs[i].x, frame->fcode, pStat);
		CodeVector(bs, pMB->pmvs[i].y, frame->fcode, pStat);
	}

	bits = BitstreamPos(bs);
	
	// code block coeffs
	for(i = 0; i < 6; i++)
		if(pMB->cbp & (1 << (5 - i)))
			CodeCoeff(bs, &qcoeff[i*64], inter_table, scan_tables[0], 0);

	bits = BitstreamPos(bs) - bits;
	pStat->iTextBits += bits;

}


void MBCoding(const FRAMEINFO * frame,
	      MACROBLOCK *pMB,
	      int16_t qcoeff[6*64], 
	      Bitstream * bs,
	      Statistics * pStat)
{

	int intra = (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q);

	if(frame->coding_type == P_VOP) {
		if(pMB->cbp == 0 && pMB->mode == MODE_INTER &&
		   pMB->mvs[0].x == 0 && pMB->mvs[0].y == 0)
		{
			BitstreamPutBit(bs, 1);		// not_coded
			return;
		}
		else
			BitstreamPutBit(bs, 0);		// coded
	}

	if(intra)
		CodeBlockIntra(frame, pMB, qcoeff, bs, pStat);
	else
		CodeBlockInter(frame, pMB, qcoeff, bs, pStat);

}

/***************************************************************
 * bframe encoding start
 ***************************************************************/

/*
	mbtype
	0	1b		direct(h263)		mvdb
	1	01b		interpolate mc+q	dbquant, mvdf, mvdb
	2	001b	backward mc+q		dbquant, mvdb
	3	0001b	forward mc+q		dbquant, mvdf
*/

void put_bvop_mbtype(Bitstream * bs, int value)
{
	switch(value)
	{
	case 0 :	BitstreamPutBit(bs, 1);
				return;

	case 1 :	BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 1);
				return;
	
	case 2 :	BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 1);
				return;
	
	case 3 :	BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 0);
				BitstreamPutBit(bs, 1);
				return;

	default :	; // invalid!

	}
	
}

/*
	dbquant
	-2	10b
	0	0b
	+2	11b
*/

void put_bvop_dbquant(Bitstream *bs, int value)
{
	switch (value)
	{
	case 0 :	BitstreamPutBit(bs, 0);
				return;

	case -2 :	BitstreamPutBit(bs, 1);
				BitstreamPutBit(bs, 0);
				return;
	
	case 2 :	BitstreamPutBit(bs, 1);
				BitstreamPutBit(bs, 1);
				return;

	default : 	; // invalid
	}
}



void MBCodingBVOP(const MACROBLOCK * mb, 
				  const int16_t qcoeff[6*64],
				  const int16_t fcode,
				  const int16_t bcode,
				  Bitstream * bs, 
				  Statistics * pStat)
{
	int i;

/*	------------------------------------------------------------------
		when a block is skipped it is decoded DIRECT(0,)
		hence are interpolated from forward & backward frames
	------------------------------------------------------------------ */

	if (mb->mode == 5)
	{
		BitstreamPutBit(bs, 1);		// skipped
		return;
	}

	BitstreamPutBit(bs, 0);		// not skipped

	if (mb->cbp == 0)
	{
		BitstreamPutBit(bs, 1);		// cbp == 0
	}
	else
	{
		BitstreamPutBit(bs, 0);		// cbp == xxx
	}

	put_bvop_mbtype(bs, mb->mode);

	if (mb->cbp)
	{
		BitstreamPutBits(bs, mb->cbp, 6);
	}

	if (mb->mode != MODE_DIRECT && mb->cbp != 0)
	{
		put_bvop_dbquant(bs, 0); // todo: mb->dquant = 0
	}

	if (mb->mode == MODE_INTERPOLATE || mb->mode == MODE_FORWARD)
	{
	    CodeVector(bs, mb->pmvs[0].x, fcode, pStat);
	    CodeVector(bs, mb->pmvs[0].y, fcode, pStat);
	}

	if (mb->mode == MODE_INTERPOLATE || mb->mode == MODE_BACKWARD)
	{
	    CodeVector(bs, mb->b_pmvs[0].x, bcode, pStat);
	    CodeVector(bs, mb->b_pmvs[0].y, bcode, pStat);
	}

	if (mb->mode == MODE_DIRECT)
	{
		// TODO: direct
	}
	
    for (i = 0; i < 6; i++)
    {
		if (mb->cbp & (1 << (5 - i)))
		{
			CodeCoeff(bs, &qcoeff[i*64], inter_table, scan_tables[0], 0);
		}
    }
}



/***************************************************************
 * decoding stuff starts here                                  *
 ***************************************************************/

int get_mcbpc_intra(Bitstream * bs)
{

	uint32_t index;
	
	while((index = BitstreamShowBits(bs, 9)) == 1)
		BitstreamSkip(bs, 9);

	index >>= 3;

	BitstreamSkip(bs, mcbpc_intra_table[index].len);

	return mcbpc_intra_table[index].code;

}

int get_mcbpc_inter(Bitstream * bs)
{

	uint32_t index;
	
	while((index = CLIP(BitstreamShowBits(bs, 9), 256)) == 1)
		BitstreamSkip(bs, 9);

	BitstreamSkip(bs,  mcbpc_inter_table[index].len);

	return mcbpc_inter_table[index].code;

}

int get_cbpy(Bitstream * bs, int intra)
{

	int cbpy;
	uint32_t index = BitstreamShowBits(bs, 6);

	BitstreamSkip(bs, cbpy_table[index].len);
	cbpy = cbpy_table[index].code;

	if(!intra)
		cbpy = 15 - cbpy;

	return cbpy;

}

int get_mv_data(Bitstream * bs)
{

	uint32_t index;

	if(BitstreamGetBit(bs))
		return 0;
	
	index = BitstreamShowBits(bs, 12);

	if(index >= 512)
	{
		index = (index >> 8) - 2;
		BitstreamSkip(bs, TMNMVtab0[index].len);
		return TMNMVtab0[index].code;
	}
	
	if(index >= 128)
	{
		index = (index >> 2) - 32;
		BitstreamSkip(bs, TMNMVtab1[index].len);
		return TMNMVtab1[index].code;
	}

	index -= 4; 

	BitstreamSkip(bs, TMNMVtab2[index].len);
	return TMNMVtab2[index].code;

}

int get_mv(Bitstream * bs, int fcode)
{

	int data;
	int res;
	int mv;
	int scale_fac = 1 << (fcode - 1);

	data = get_mv_data(bs);
	
	if(scale_fac == 1 || data == 0)
		return data;

	res = BitstreamGetBits(bs, fcode - 1);
	mv = ((ABS(data) - 1) * scale_fac) + res + 1;
	
	return data < 0 ? -mv : mv;

}

int get_dc_dif(Bitstream * bs, uint32_t dc_size)
{

	int code = BitstreamGetBits(bs, dc_size);
	int msb = code >> (dc_size - 1);

	if(msb == 0)
		return (-1 * (code^((1 << dc_size) - 1)));

	return code;

}

int get_dc_size_lum(Bitstream * bs)
{

	int code, i;
	code = BitstreamShowBits(bs, 11);

	for(i = 11; i > 3; i--) {
		if(code == 1) {
			BitstreamSkip(bs, i);
			return i + 1;
		}
		code >>= 1;
	}

	BitstreamSkip(bs, dc_lum_tab[code].len);
	return dc_lum_tab[code].code;

}


int get_dc_size_chrom(Bitstream * bs)
{

	uint32_t code, i;
	code = BitstreamShowBits(bs, 12);

	for(i = 12; i > 2; i--) {
		if(code == 1) {
			BitstreamSkip(bs, i);
			return i;
		}
		code >>= 1;
	}

	return 3 - BitstreamGetBits(bs, 2);

}

void get_intra_block(Bitstream * bs, int16_t * block, int direction, int coeff) 
{

	const uint16_t * scan = scan_tables[ direction ];
	int level;
	int run;
	int last;

	do
	{
		level = get_coeff(bs, &run, &last, 1, 0);
		if (run == -1)
		{
			DEBUG("fatal: invalid run");
			break;
		}
		coeff += run;
		block[ scan[coeff] ] = level;
		if (level < -127 || level > 127)
		{
			DEBUG1("warning: intra_overflow", level);
		}
		coeff++;
	} while (!last);

}

void get_inter_block(Bitstream * bs, int16_t * block) 
{

	const uint16_t * scan = scan_tables[0];
	int p;
	int level;
	int run;
	int last;

	p = 0;
	do
	{
		level = get_coeff(bs, &run, &last, 0, 0);
		if (run == -1)
		{
			DEBUG("fatal: invalid run");
			break;
		}
		p += run;
		block[ scan[p] ] = level;
		if (level < -127 || level > 127)
		{
			DEBUG1("warning: inter_overflow", level);
		}
		p++;
	} while (!last);

}
