#ifndef _MB_CODING_H_
#define _MB_CODING_H_

#include "../portab.h"
#include "../global.h"
#include "vlc_codes.h"
#include "bitstream.h"

void init_vlc_tables(void);

int check_resync_marker(Bitstream * bs, int addbits);

void bs_put_spritetrajectory(Bitstream * bs, const int val);
int bs_get_spritetrajectory(Bitstream * bs);

int get_mcbpc_intra(Bitstream * bs);
int get_mcbpc_inter(Bitstream * bs);
int get_cbpy(Bitstream * bs,
			 int intra);
int get_mv(Bitstream * bs,
		   int fcode);

int get_dc_dif(Bitstream * bs,
			   uint32_t dc_size);
int get_dc_size_lum(Bitstream * bs);
int get_dc_size_chrom(Bitstream * bs);

void get_intra_block(Bitstream * bs,
					 int16_t * block,
					 int direction,
					 int coeff);
void get_inter_block(Bitstream * bs,
					 int16_t * block,
					 int direction);


void MBCodingBVOP(const MACROBLOCK * mb,
				  const int16_t qcoeff[6 * 64],
				  const int32_t fcode,
				  const int32_t bcode,
				  Bitstream * bs,
				  Statistics * pStat,
				  int alternate_scan);


static __inline void
MBSkip(Bitstream * bs)
{
	BitstreamPutBit(bs, 1);	// not coded
}


#ifdef BIGLUT
extern VLC *intra_table;
int CodeCoeff_CalcBits(const int16_t qcoeff[64], VLC * table, const uint16_t * zigzag, uint16_t intra);
#else
int CodeCoeffIntra_CalcBits(const int16_t qcoeff[64], const uint16_t * zigzag);
#endif

#endif							/* _MB_CODING_H_ */
