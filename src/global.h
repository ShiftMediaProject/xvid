#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "xvid.h"
#include "portab.h"

/* --- macroblock modes --- */

#define MODE_INTER		0
#define MODE_INTER_Q	1
#define MODE_INTER4V	2
#define	MODE_INTRA		3
#define MODE_INTRA_Q	4
#define MODE_STUFFING	7
#define MODE_NOT_CODED	16

/* --- bframe specific --- */

#define MODE_DIRECT			0
#define MODE_INTERPOLATE	1
#define MODE_BACKWARD		2
#define MODE_FORWARD		3
#define MODE_DIRECT_NONE_MV	4


typedef struct
{
	uint32_t bufa;
	uint32_t bufb;
	uint32_t buf;
	uint32_t pos;
	uint32_t *tail;
	uint32_t *start;
	uint32_t length;
} 
Bitstream;


#define MBPRED_SIZE  15


typedef struct
{
	// decoder/encoder 
	VECTOR mvs[4];
	int32_t sad8[4];		// (signed!) SAD values for inter4v-VECTORs
	int32_t sad16;			// (signed!) SAD value for inter-VECTOR

    short int pred_values[6][MBPRED_SIZE];
    int acpred_directions[6];
    
	int mode;
	int quant;		// absolute quant

	int field_dct;
	int field_pred;
	int field_for_top;
	int field_for_bot;

	// encoder specific

	VECTOR pmvs[4];
	int dquant;
	int cbp;

	// bframe stuff

	VECTOR b_mvs[4];
	VECTOR b_pmvs[4];

	int mb_type;
	int dbquant;

} MACROBLOCK;

static __inline int8_t get_dc_scaler(uint32_t quant, uint32_t lum)
{
	if(quant < 5)
        return 8;

	if(quant < 25 && !lum)
        return (quant + 13) / 2;

	if(quant < 9)
        return 2 * quant;

    if(quant < 25)
        return quant + 8;

	if(lum)
		return 2 * quant - 16;
	else
        return quant - 6;
}

// useful macros

#define MIN(X, Y) ((X)<(Y)?(X):(Y))
#define MAX(X, Y) ((X)>(Y)?(X):(Y))
#define ABS(X)    (((X)>0)?(X):-(X))
#define SIGN(X)   (((X)>0)?1:-1)


#endif /* _GLOBAL_H_ */
