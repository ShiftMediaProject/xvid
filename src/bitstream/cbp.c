#include "../portab.h"
#include "cbp.h"

cbpFuncPtr calc_cbp;

/*
 * Returns a field of bits that indicates non zero ac blocks
 * for this macro block
 */

/* naive C */
uint32_t
calc_cbp_plain(const int16_t codes[6 * 64])
{
	int i, j;
	uint32_t cbp = 0;

	for (i = 0; i < 6; i++) {
		for (j=1; j<64;j++) {
			if (codes[64*i+j]) {
            	cbp |= 1 << (5-i);
				break;
			}
		}
	}
	return cbp;
}

/* optimized C */
uint32_t
calc_cbp_c(const int16_t codes[6 * 64])
{
	unsigned int i=6;
	uint32_t cbp = 0;

/* uses fixed relation: 4*codes = 1*codes64 */
/* if prototype is changed (e.g. from int16_t to something like int32) this routine 
   has to be changed! */

	do  {
		uint64_t *codes64 = (uint64_t*)codes;	/* the compiler doesn't really make this */
		uint32_t *codes32 = (uint32_t*)codes;	/* variables, just "addressing modes" */

		cbp += cbp; 
        if (codes[1] || codes32[1]) {
			cbp++;
		}
        else if (codes64[1] | codes64[2] | codes64[3]) {
			cbp++;
		}
        else if (codes64[4] | codes64[5] | codes64[6] | codes64[7]) {
			cbp++;
		}
        else if (codes64[8] | codes64[9] | codes64[10] | codes64[11]) {
			cbp++;
		}
        else if (codes64[12] | codes64[13] | codes64[14] | codes64[15]) {
			cbp++;
		}
		codes += 64;
		i--;
    } while (i != 0);

	return cbp;
}




/* older code maybe better on some plattforms? */
#if (0==1)
	for (i = 5; i >= 0; i--) {
		if (codes[1] | codes[2] | codes[3])
                        cbp |= 1 << i;
		else {
			for (j = 4; j <= 56; j+=4) 	/* [60],[61],[62],[63] are last */
				if (codes[j] | codes[j+1] | codes[j+2] | codes[j+3]) {
				cbp |= 1 << i;
				break;
			}
		}
		codes += 64;
	}

	return cbp;
#endif
