#include "../portab.h"
#include "bitstream.h"
#include "zigzag.h"
#include "vlc_codes.h"

#include "../utils/mbfunctions.h"

#include <stdlib.h> /* malloc, free */

#define ESCAPE 7167
#define ABS(X) (((X)>0)?(X):-(X))
#define CLIP(X,A) (X > A) ? (A) : (X)

static VLC *DCT3D[2];

VLC *intra_table, *inter_table;
static short clip_table[4096];

void create_vlc_tables(void)
{
	int32_t k, l, i, intra, last;
	VLC *vlc[2];
	VLC **coeff_ptr;
	VLC *vlc1, *vlc2;

	VLC *DCT3Dintra;
	VLC *DCT3Dinter;

	DCT3Dintra = (VLC *) malloc(sizeof(VLC) * 4096);
	DCT3Dinter = (VLC *) malloc(sizeof(VLC) * 4096);

	vlc1 = DCT3Dintra;
	vlc2 = DCT3Dinter;
	
	vlc[0] = intra_table = (VLC *) malloc(128 * 511 * sizeof(VLC));
	vlc[1] = inter_table = (VLC *) malloc(128 * 511 * sizeof(VLC));
	
	// initialize the clipping table
	for(i = -2048; i < 2048; i++) {
		clip_table[i + 2048] = i;
		if(i < -255)
			clip_table[i + 2048] = -255;
		if(i > 255)
			clip_table[i + 2048] = 255;
	}
	
	// generate intra/inter vlc lookup table
	for(i = 0; i < 4; i++) {
		intra = i % 2;
		last = i >> 1;

		coeff_ptr = coeff_vlc[last + (intra << 1)];
			
		for(k = -255; k < 256; k++) { // level
			char *max_level_ptr = max_level[last + (intra << 1)];
			char *max_run_ptr = max_run[last + (intra << 1)];
	
			for(l = 0; l < 64; l++) { // run
				int32_t level = k, run = l;
	
				if(abs(level) <= max_level_ptr[run] && run <= max_run_ptr[abs(level)]) {

					if(level > 0) {
						vlc[intra]->code = (coeff_ptr[run][level - 1].code) << 1;
						vlc[intra]->len = coeff_ptr[run][level - 1].len + 1;
					}
					else if(level < 0) {
						vlc[intra]->code = ((coeff_ptr[run][-level - 1].code) << 1) + 1;
						vlc[intra]->len = coeff_ptr[run][-level - 1].len + 1;
					}
					else {
						vlc[intra]->code = 0;
						vlc[intra]->len = 0;
					}
				} else {
					if(level > 0)
						level -= max_level_ptr[run];
					else
						level += max_level_ptr[run];
					
					if(abs(level) <= max_level_ptr[run] &&
						run <= max_run_ptr[abs(level)]) {
							
						if(level > 0) {
							vlc[intra]->code = (0x06 << (coeff_ptr[run][level - 1].len + 1)) |
								(coeff_ptr[run][level - 1].code << 1);
							vlc[intra]->len = (coeff_ptr[run][level - 1].len + 1) + 8;
						}
						else if(level < 0) {
							vlc[intra]->code = (0x06 << (coeff_ptr[run][-level - 1].len + 1)) |
								((coeff_ptr[run][-level - 1].code << 1) + 1);
							vlc[intra]->len = (coeff_ptr[run][-level - 1].len + 1) + 8;
						}
						else {
							vlc[intra]->code = 0x06;
							vlc[intra]->len = 8;
						}
					} else {
						if(level > 0)
							level += max_level_ptr[run];
						else
							level -= max_level_ptr[run];
						DEBUG1("1) run:", run);
						run -= max_run_ptr[abs(level)] + 1;
						DEBUG1("2) run:", run);
						
						if(abs(level) <= max_level_ptr[run] &&
							run <= max_run_ptr[abs(level)]) {
						
							if(level > 0) {
								vlc[intra]->code = (0x0e << (coeff_ptr[run][level - 1].len + 1)) |
									(coeff_ptr[run][level - 1].code << 1);
								vlc[intra]->len = (coeff_ptr[run][level - 1].len + 1) + 9;
							}
							else if(level < 0) {
								vlc[intra]->code = (0x0e << (coeff_ptr[run][-level - 1].len + 1)) |
									((coeff_ptr[run][-level - 1].code << 1) + 1);
								vlc[intra]->len = (coeff_ptr[run][-level - 1].len + 1) + 9;
							}
							else {
								vlc[intra]->code = 0x0e;
								vlc[intra]->len = 9;
							}
						} else {
							if(level != 0)
								run += max_run_ptr[abs(level)] + 1;
							else
								run++;

							DEBUG1("3) run:", run);

							vlc[intra]->code = (uint32_t) ((0x1e + last) << 20) |
										(l << 14) | (1 << 13) | ((k & 0xfff) << 1) | 1;
								
							vlc[intra]->len = 30;
						}
					}
				}
				vlc[intra]++;
			}
		}
	}
	intra_table += 64*255; // center vlc tables
	inter_table += 64*255; // center vlc tables

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

void destroy_vlc_tables(void) {

	if(intra_table != NULL && inter_table != NULL) {
		intra_table -= 64*255; // uncenter vlc tables
		inter_table -= 64*255; // uncenter vlc tables

		free(intra_table);
		free(inter_table);
	}

	if(DCT3D[0] != NULL && DCT3D[1] != NULL) {
		free(DCT3D[0]);
		free(DCT3D[1]);
	}

}

static __inline void CodeVector(Bitstream *bs, int16_t value, int16_t f_code, Statistics *pStat)
{
	const int scale_factor = 1 << (f_code - 1);
	const int cmp = scale_factor << 5;

	if(value < (-1 * cmp))
		value += 64 * scale_factor;
	
	if(value > (cmp - 1))
		value -= 64 * scale_factor;

    pStat->iMvSum += value * value;
    pStat->iMvCount++;

	if (value == 0)
		BitstreamPutBits(bs, mb_motion_table[32].code, mb_motion_table[32].len);
    else {
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


static __inline void CodeCoeff(Bitstream *bs, int16_t qcoeff[64], VLC *table,
							   const uint16_t *zigzag, uint16_t intra) {
	uint32_t j, last;
	short v;
	VLC *vlc;
	
	j = intra;
	last = 1 + intra;

	while((v = qcoeff[zigzag[j++]]) == 0);
    
	do {
		// count zeroes
		vlc = table + (clip_table[2048+v] << 6) + j - last;
		last = j + 1;
		while(j < 64 && (v = qcoeff[zigzag[j++]]) == 0);
				
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


static void CodeBlockIntra(const MBParam * pParam, const MACROBLOCK *pMB,
								  int16_t qcoeff[][64], Bitstream * bs, Statistics * pStat)
{
	uint32_t i, mcbpc, cbpy, bits;

	cbpy = pMB->cbp >> 2;

    // write mcbpc
	if(pParam->coding_type == I_VOP) {
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

	// code block coeffs
	for(i = 0; i < 6; i++)
	{
		if(i < 4)
			BitstreamPutBits(bs, dcy_tab[qcoeff[i][0] + 255].code,
							 dcy_tab[qcoeff[i][0] + 255].len);
		else
			BitstreamPutBits(bs, dcc_tab[qcoeff[i][0] + 255].code,
			                 dcc_tab[qcoeff[i][0] + 255].len);
		
		if(pMB->cbp & (1 << (5 - i)))
		{
			bits = BitstreamPos(bs);

			CodeCoeff(bs, qcoeff[i], intra_table, scan_tables[pMB->acpred_directions[i]], 1);

			bits = BitstreamPos(bs) - bits;
			pStat->iTextBits += bits;
		}
	}
}


static void CodeBlockInter(const MBParam * pParam, const MACROBLOCK *pMB,
								  int16_t qcoeff[][64], Bitstream * bs, Statistics * pStat)
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
    
	// code motion vector(s)
	for(i = 0; i < (pMB->mode == MODE_INTER4V ? 4 : 1); i++)
	{
		CodeVector(bs, pMB->pmvs[i].x, pParam->fixed_code, pStat);
		CodeVector(bs, pMB->pmvs[i].y, pParam->fixed_code, pStat);
	}

	bits = BitstreamPos(bs);
	
	// code block coeffs
	for(i = 0; i < 6; i++)
		if(pMB->cbp & (1 << (5 - i)))
			CodeCoeff(bs, qcoeff[i], inter_table, scan_tables[0], 0);

	bits = BitstreamPos(bs) - bits;
	pStat->iTextBits += bits;
}


void MBCoding(const MBParam * pParam, MACROBLOCK *pMB,
	      int16_t qcoeff[][64], 
		  Bitstream * bs, Statistics * pStat)
{
	int intra = (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q);

    if(pParam->coding_type == P_VOP) {
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
		CodeBlockIntra(pParam, pMB, qcoeff, bs, pStat);
	else
		CodeBlockInter(pParam, pMB, qcoeff, bs, pStat);
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

int get_coeff(Bitstream * bs, int *run, int *last, int intra, int short_video_header) 
{
    uint32_t mode;
    const VLC *tab;
	int32_t level;

	if(short_video_header) // inter-VLCs will be used for both intra and inter blocks
		intra = 0;
	
	tab = &DCT3D[intra][BitstreamShowBits(bs, 12)];
	
	if(tab->code == -1)
		goto error;

	BitstreamSkip(bs, tab->len);

	if(tab->code != ESCAPE) {
		if(!intra) 
		{
			*run = (tab->code >> 4) & 255;
			level = tab->code & 15;
			*last = (tab->code >> 12) & 1;
		}
	    else 
		{
			*run = (tab->code >> 8) & 255;
			level = tab->code & 255;
			*last = (tab->code >> 16) & 1;
		}
		return BitstreamGetBit(bs) ? -level : level;
	}

	if(short_video_header)
	{
		// escape mode 4 - H.263 type, only used if short_video_header = 1 
		*last = BitstreamGetBit(bs);
		*run = BitstreamGetBits(bs, 6);
		level = BitstreamGetBits(bs, 8);

		if (level == 0 || level == 128) 
			DEBUG1("Illegal LEVEL for ESCAPE mode 4:", level);

		return (level >= 128 ? -(256 - level) : level);
	}
	
	mode = BitstreamShowBits(bs, 2);

	if(mode < 3) {
		BitstreamSkip(bs, (mode == 2) ? 2 : 1);

		tab = &DCT3D[intra][BitstreamShowBits(bs, 12)];
		if (tab->code == -1)
			goto error;

		BitstreamSkip(bs, tab->len);

		if (!intra) {		
			*run = (tab->code >> 4) & 255;
			level = tab->code & 15;
			*last = (tab->code >> 12) & 1;
		}
		else 
		{
			*run = (tab->code >> 8) & 255;
			level = tab->code & 255;
			*last = (tab->code >> 16) & 1;
		}      

		if(mode < 2) // first escape mode, level is offset
			level += max_level[*last + (!intra<<1)][*run]; // need to add back the max level
		else if(mode == 2)  // second escape mode, run is offset
			*run += max_run[*last + (!intra<<1)][level] + 1;
		
		return BitstreamGetBit(bs) ? -level : level;
	} 

	// third escape mode - fixed length codes
	BitstreamSkip(bs, 2);
	*last = BitstreamGetBits(bs, 1);
	*run = BitstreamGetBits(bs, 6);			
	BitstreamSkip(bs, 1);				// marker
	level = BitstreamGetBits(bs, 12);		
	BitstreamSkip(bs, 1);				// marker

	return (level & 0x800) ? (level | (-1 ^ 0xfff)) : level;

error:
	*run = VLC_ERROR;
	return 0;
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
