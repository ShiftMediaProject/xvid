#ifndef _FDCT_H_
#define _FDCT_H_

typedef void (fdctFunc) (short *const block);
typedef fdctFunc *fdctFuncPtr;

extern fdctFuncPtr fdct;

fdctFunc fdct_int32;

fdctFunc fdct_mmx;		/* AP-992, Royce Shih-Wea Liao */
fdctFunc fdct_sse2;		/* Dmitry Rozhdestvensky, Vladimir G. Ivanov */
fdctFunc xvid_fdct_mmx;	/* Pascal Massimino */
fdctFunc xvid_fdct_sse;	/* Pascal Massimino */

fdctFunc fdct_altivec;
fdctFunc fdct_ia64;

#endif							/* _FDCT_H_ */
