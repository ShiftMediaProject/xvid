#ifndef _IDCT_H_
#define _IDCT_H_

void idct_int32_init();
void idct_ia64_init();

typedef void (idctFunc) (short *const block);
typedef idctFunc *idctFuncPtr;

extern idctFuncPtr idct;

idctFunc idct_int32;

idctFunc idct_mmx;			/* AP-992, Peter Gubanov, Michel Lespinasse */
idctFunc idct_xmm;			/* AP-992, Peter Gubanov, Michel Lespinasse */
idctFunc idct_3dne;			/* AP-992, Peter Gubanov, Michel Lespinasse, Jaan Kalda */
idctFunc idct_sse2;			/* Dmitry Rozhdestvensky */
idctFunc simple_idct_c;		/* Michael Niedermayer */
idctFunc simple_idct_mmx;	/* Michael Niedermayer; expects permutated data */
idctFunc simple_idct_mmx2;	/* Michael Niedermayer */

idctFunc idct_altivec;
idctFunc idct_ia64;

#endif							/* _IDCT_H_ */
