#include "../portab.h"
#include "cbp.h"

cbpFuncPtr calc_cbp;

/*
 * Returns a field of bits that indicates non zero ac blocks
 * for this macro block
 */
uint32_t calc_cbp_c(const int16_t codes[6][64])
{
	uint32_t i, j;
	uint32_t cbp = 0;

	for (i = 0; i < 6; i++) {
		for (j = 1; j < 61; j+=4) {
			if (codes[i][j]  |codes[i][j+1]|
			    codes[i][j+2]|codes[i][j+3]) {
				cbp |= 1 << (5 - i);
				break;
			}
		}

		if(codes[i][j]|codes[i][j+1]|codes[i][j+2])
			cbp |= 1 << (5 - i);

	}

	return cbp;

}
