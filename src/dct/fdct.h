#ifndef _FDCT_H_
#define _FDCT_H_

typedef void (fdctFunc) (short *const block);
typedef fdctFunc *fdctFuncPtr;

extern fdctFuncPtr fdct;

fdctFunc fdct_int32;

fdctFunc fdct_mmx;
fdctFunc fdct_sse2;

fdctFunc fdct_altivec;
fdctFunc fdct_ia64;

#endif							/* _FDCT_H_ */
