/**************************************************************************
 *
 *      XVID MPEG-4 VIDEO CODEC - Unit tests and benches
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/************************************************************************
 *                            
 *  'Reference' output is at the end of file.
 *  Don't take the checksums and crc too seriouly, they aren't
 *  bullet-proof...
 *
 *   compiles with something like:
 *   gcc -o xvid_bench xvid_bench.c  -I../src/ -lxvidcore -lm
 *
 *	History:
 *
 *	06.06.2002  initial coding      -Skal-
 *
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>  // for gettimeofday
#include <string.h>    // for memset
#include <assert.h>

#include "xvid.h"

// inner guts
#include "dct/idct.h"
#include "dct/fdct.h"
#include "image/colorspace.h"
#include "image/interpolate8x8.h"
#include "utils/mem_transfer.h"
#include "quant/quant_h263.h"
#include "quant/quant_mpeg4.h"
#include "motion/sad.h"
#include "utils/emms.h"
#include "utils/timer.h"
#include "quant/quant_matrix.c"
#include "bitstream/cbp.h"

const int speed_ref = 100;  // on slow machines, decrease this value

/*********************************************************************
 * misc
 *********************************************************************/

 /* returns time in micro-s*/
double gettime_usec()
{    
  struct timeval  tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec*1.0e6 + tv.tv_usec;
}

 /* returns squared deviates (mean(v*v)-mean(v)^2) of a 8x8 block */
double sqr_dev(uint8_t v[8*8])
{
  double sum=0.;
  double sum2=0.;
  int n;
  for (n=0;n<8*8;n++)
  {
    sum  += v[n];
    sum2 += v[n]*v[n];
  }
  sum2 /= n;
  sum /= n;
  return sum2-sum*sum;
}

/*********************************************************************
 * cpu init
 *********************************************************************/

typedef struct {
  const char *name;
  unsigned int cpu;
} CPU;

CPU cpu_list[] = 
{ { "PLAINC", 0 }
, { "MMX   ", XVID_CPU_MMX }
, { "MMXEXT", XVID_CPU_MMXEXT | XVID_CPU_MMX }
, { "SSE2  ", XVID_CPU_SSE2 | XVID_CPU_MMX }
, { "3DNOW ", XVID_CPU_3DNOW }
, { "3DNOWE", XVID_CPU_3DNOWEXT }

//, { "TSC   ", XVID_CPU_TSC }
, { 0, 0 } }

, cpu_short_list[] =
{ { "PLAINC", 0 }
, { "MMX   ", XVID_CPU_MMX }
, { "MMXEXT", XVID_CPU_MMXEXT | XVID_CPU_MMX }
, { "IA64  ", XVID_CPU_IA64 }
, { 0, 0 } }

, cpu_short_list2[] = 
{ { "PLAINC", 0 }
, { "MMX   ", XVID_CPU_MMX }
, { "SSE2  ", XVID_CPU_SSE2 | XVID_CPU_MMX }
, { 0, 0 } };


int init_cpu(CPU *cpu)
{
  int xerr, cpu_type;
  XVID_INIT_PARAM xinit;

  cpu_type = check_cpu_features() & cpu->cpu;
  xinit.cpu_flags = cpu_type | XVID_CPU_FORCE;
  //    xinit.cpu_flags = XVID_CPU_MMX | XVID_CPU_FORCE;
  xerr = xvid_init(NULL, 0, &xinit, NULL);
  if (cpu->cpu>0 && (cpu_type==0 || xerr!=XVID_ERR_OK)) {
    printf( "%s - skipped...\n", cpu->name );
    return 0;
  }
  return 1;
}

/*********************************************************************
 * test DCT
 *********************************************************************/

#define ABS(X)  ((X)<0 ? -(X) : (X))

void test_dct()
{
  const int nb_tests = 300*speed_ref;
  int tst;
  CPU *cpu;
  int i;
  short iDst0[8*8], iDst[8*8], fDst[8*8];
  double overhead;

  printf( "\n ===== test fdct/idct =====\n" );

  for(i=0; i<8*8; ++i) iDst0[i] = (i*7-i*i) & 0x7f;
  overhead = gettime_usec();
  for(tst=0; tst<nb_tests; ++tst)
  {
    for(i=0; i<8*8; ++i) fDst[i] = iDst0[i];
    for(i=0; i<8*8; ++i) iDst[i] = fDst[i];
  }
  overhead = gettime_usec() - overhead;

  for(cpu = cpu_list; cpu->name!=0; ++cpu)
  {
    double t;
    int iCrc, fCrc;

    if (!init_cpu(cpu))
      continue;

    t = gettime_usec();
    emms();
    for(tst=0; tst<nb_tests; ++tst)
    {
      for(i=0; i<8*8; ++i) fDst[i] = iDst0[i];
      fdct(fDst);
      for(i=0; i<8*8; ++i) iDst[i] = fDst[i];
      idct(iDst);
    }
    emms();
    t = (gettime_usec() - t - overhead) / nb_tests;
    iCrc=0; fCrc=0;
    for(i=0; i<8*8; ++i) {
      iCrc += ABS(iDst[i] - iDst0[i]);
      fCrc += fDst[i]^i;
    }
    printf( "%s -  %.3f usec       iCrc=%d  fCrc=%d\n",
      cpu->name, t, iCrc, fCrc );
      // the norm tolerates ~1 bit of diff per coeff
    if (ABS(iCrc)>=64) printf( "*** CRC ERROR! ***\n" );
  }
}

/*********************************************************************
 * test SAD
 *********************************************************************/

void test_sad()
{
  const int nb_tests = 2000*speed_ref;
  int tst;
  CPU *cpu;
  int i;
  uint8_t Cur[16*16], Ref1[16*16], Ref2[16*16];

  printf( "\n ======  test SAD ======\n" );
  for(i=0; i<16*16;++i) {
    Cur[i] = (i/5) ^ 0x05;
    Ref1[i] = (i + 0x0b) & 0xff;
    Ref2[i] = i ^ 0x76;
  }

  for(cpu = cpu_list; cpu->name!=0; ++cpu)
  {
    double t;
    uint32_t s;
    if (!init_cpu(cpu))
      continue;

    t = gettime_usec();
    emms();
    for(tst=0; tst<nb_tests; ++tst) s = sad8(Cur, Ref1, 16);
    emms();
    t = (gettime_usec() - t) / nb_tests;
    printf( "%s - sad8    %.3f usec       sad=%d\n", cpu->name, t, s );
    if (s!=3776) printf( "*** CRC ERROR! ***\n" );

    t = gettime_usec();
    emms();
    for(tst=0; tst<nb_tests; ++tst) s = sad16(Cur, Ref1, 16, -1);
    emms();
    t = (gettime_usec() - t) / nb_tests;
    printf( "%s - sad16   %.3f usec       sad=%d\n", cpu->name, t, s );
    if (s!=27214) printf( "*** CRC ERROR! ***\n" );

    t = gettime_usec();
    emms();
    for(tst=0; tst<nb_tests; ++tst) s = sad16bi(Cur, Ref1, Ref2, 16);
    emms();
    t = (gettime_usec() - t) / nb_tests;
    printf( "%s - sad16bi %.3f usec       sad=%d\n", cpu->name, t, s );
    if (s!=26274) printf( "*** CRC ERROR! ***\n" );

    t = gettime_usec();
    emms();
    for(tst=0; tst<nb_tests; ++tst) s = dev16(Cur, 16);
    emms();
    t = (gettime_usec() - t) / nb_tests;
    printf( "%s - dev16   %.3f usec       sad=%d\n", cpu->name, t, s );
    if (s!=3344) printf( "*** CRC ERROR! ***\n" );

    printf( " --- \n" );
  }
}

/*********************************************************************
 * test interpolation
 *********************************************************************/

#define ENTER \
    for(i=0; i<16*8; ++i) Dst[i] = 0;   \
    t = gettime_usec();                   \
    emms();

#define LEAVE \
    emms();                             \
    t = (gettime_usec() - t) / nb_tests;  \
    iCrc = 0;                           \
    for(i=0; i<16*8; ++i) { iCrc += Dst[i]^i; }

#define TEST_MB(FUNC, R)                \
    ENTER                               \
    for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst, Src0, 16, (R)); \
    LEAVE

#define TEST_MB2(FUNC)                  \
    ENTER                               \
    for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst, Src0, 16); \
    LEAVE


void test_mb()
{
  const int nb_tests = 2000*speed_ref;
  CPU *cpu;
  const uint8_t Src0[16*9] = {
        // try to have every possible combinaison of rounding...
      0, 0, 1, 0, 2, 0, 3, 0, 4             ,0,0,0, 0,0,0,0
    , 0, 1, 1, 1, 2, 1, 3, 1, 3             ,0,0,0, 0,0,0,0
    , 0, 2, 1, 2, 2, 2, 3, 2, 2             ,0,0,0, 0,0,0,0
    , 0, 3, 1, 3, 2, 3, 3, 3, 1             ,0,0,0, 0,0,0,0
    , 1, 3, 0, 2, 1, 0, 2, 3, 4             ,0,0,0, 0,0,0,0
    , 2, 2, 1, 2, 0, 1, 3, 5, 3             ,0,0,0, 0,0,0,0
    , 3, 1, 2, 3, 1, 2, 2, 6, 2             ,0,0,0, 0,0,0,0
    , 1, 0, 1, 3, 0, 3, 1, 6, 1             ,0,0,0, 0,0,0,0
    , 4, 3, 2, 1, 2, 3, 4, 0, 3             ,0,0,0, 0,0,0,0
  };
  uint8_t Dst[16*8] = {0};

  printf( "\n ===  test block motion ===\n" );

  for(cpu = cpu_list; cpu->name!=0; ++cpu)
  {
    double t;
    int tst, i, iCrc;

    if (!init_cpu(cpu))
      continue;

    TEST_MB(interpolate8x8_halfpel_h, 0);
    printf( "%s - interp- h-round0 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8107) printf( "*** CRC ERROR! ***\n" );

    TEST_MB(interpolate8x8_halfpel_h, 1);
    printf( "%s -           round1 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8100) printf( "*** CRC ERROR! ***\n" );


    TEST_MB(interpolate8x8_halfpel_v, 0);
    printf( "%s - interp- v-round0 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8108) printf( "*** CRC ERROR! ***\n" );

    TEST_MB(interpolate8x8_halfpel_v, 1);
    printf( "%s -           round1 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8105) printf( "*** CRC ERROR! ***\n" );


    TEST_MB(interpolate8x8_halfpel_hv, 0);
    printf( "%s - interp-hv-round0 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8112) printf( "*** CRC ERROR! ***\n" );

    TEST_MB(interpolate8x8_halfpel_hv, 1);
    printf( "%s -           round1 %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8103) printf( "*** CRC ERROR! ***\n" );

    printf( " --- \n" );
  }
}

/*********************************************************************
 * test transfer
 *********************************************************************/

#define INIT_TRANSFER \
  for(i=0; i<8*32; ++i) {             \
    Src8[i] = i; Src16[i] = i;        \
    Dst8[i] = 0; Dst16[i] = 0;        \
    Ref1[i] = i^0x27;                 \
    Ref2[i] = i^0x51;                 \
  }

#define TEST_TRANSFER_BEGIN(DST)              \
    INIT_TRANSFER                             \
    overhead = -gettime_usec();               \
    for(tst=0; tst<nb_tests; ++tst) {         \
      for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
    }                                         \
    overhead += gettime_usec();               \
    t = gettime_usec();                       \
    emms();                                   \
    for(tst=0; tst<nb_tests; ++tst) {         \
      for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;


#define TEST_TRANSFER_END(DST)                \
    }                                         \
    emms();                                   \
    t = (gettime_usec()-t -overhead) / nb_tests;\
    s = 0; for(i=0; i<8*32; ++i) { s += (DST)[i]^i; }

#define TEST_TRANSFER(FUNC, DST, SRC)         \
    TEST_TRANSFER_BEGIN(DST);                 \
      (FUNC)((DST), (SRC), 32);               \
    TEST_TRANSFER_END(DST)


#define TEST_TRANSFER2_BEGIN(DST, SRC)        \
    INIT_TRANSFER                             \
    overhead = -gettime_usec();               \
    for(tst=0; tst<nb_tests; ++tst) {         \
      for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
      for(i=0; i<8*32; ++i) (SRC)[i] = i^0x3e;\
    }                                         \
    overhead += gettime_usec();               \
    t = gettime_usec();                       \
    emms();                                   \
    for(tst=0; tst<nb_tests; ++tst) {         \
      for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
      for(i=0; i<8*32; ++i) (SRC)[i] = i^0x3e;

#define TEST_TRANSFER2_END(DST)               \
    }                                         \
    emms();                                   \
    t = (gettime_usec()-t -overhead) / nb_tests;\
    s = 0; for(i=0; i<8*32; ++i) { s += (DST)[i]; }

#define TEST_TRANSFER2(FUNC, DST, SRC, R1)    \
    TEST_TRANSFER2_BEGIN(DST,SRC);            \
      (FUNC)((DST), (SRC), (R1), 32);         \
    TEST_TRANSFER2_END(DST)

#define TEST_TRANSFER3(FUNC, DST, SRC, R1, R2)\
    TEST_TRANSFER_BEGIN(DST);                 \
      (FUNC)((DST), (SRC), (R1), (R2), 32);   \
    TEST_TRANSFER_END(DST)

void test_transfer()
{
  const int nb_tests = 4000*speed_ref;
  int i;
  CPU *cpu;
  uint8_t  Src8[8*32], Dst8[8*32], Ref1[8*32], Ref2[8*32];
  int16_t Src16[8*32], Dst16[8*32];

  printf( "\n ===  test transfer ===\n" );

  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    double t, overhead;
    int tst, s;

    if (!init_cpu(cpu))
      continue;

    TEST_TRANSFER(transfer_8to16copy, Dst16, Src8);
    printf( "%s - 8to16     %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=28288) printf( "*** CRC ERROR! ***\n" );

    TEST_TRANSFER(transfer_16to8copy, Dst8, Src16);
    printf( "%s - 16to8     %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=28288) printf( "*** CRC ERROR! ***\n" );

    TEST_TRANSFER(transfer8x8_copy, Dst8, Src8);
    printf( "%s - 8to8      %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=20352) printf( "*** CRC ERROR! ***\n" );

    TEST_TRANSFER(transfer_16to8add, Dst8, Src16);
    printf( "%s - 16to8add  %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=25536) printf( "*** CRC ERROR! ***\n" );

    TEST_TRANSFER2(transfer_8to16sub, Dst16, Src8, Ref1);
    printf( "%s - 8to16sub  %.3f usec       crc1=%d ", cpu->name, t, s );
    if (s!=28064) printf( "*** CRC ERROR! ***\n" );
    s = 0; for(i=0; i<8*32; ++i) { s += (Src8[i]-Ref1[i])&i; }
    printf( "crc2=%d\n", s);
    if (s!=16256) printf( "*** CRC ERROR! ***\n" );

    TEST_TRANSFER3(transfer_8to16sub2, Dst16, Src8, Ref1, Ref2);
    printf( "%s - 8to16sub2 %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=20384) printf( "*** CRC ERROR! ***\n" );

    printf( " --- \n" );
  }
}

/*********************************************************************
 * test quantization
 *********************************************************************/

#define TEST_QUANT(FUNC, DST, SRC)            \
    t = gettime_usec();                       \
    emms();                                   \
    for(tst=0; tst<nb_tests; ++tst)           \
      for(s=0, q=1; q<=max_Q; ++q) {          \
        (FUNC)((DST), (SRC), q);              \
        for(i=0; i<64; ++i) s+=(DST)[i]^i;    \
      }                                       \
    emms();                                   \
    t = (gettime_usec()-t-overhead)/nb_tests;

#define TEST_QUANT2(FUNC, DST, SRC, MULT)     \
    t = gettime_usec();                       \
    emms();                                   \
    for(tst=0; tst<nb_tests; ++tst)           \
      for(s=0, q=1; q<=max_Q; ++q) {          \
        (FUNC)((DST), (SRC), q, MULT);        \
        for(i=0; i<64; ++i) s+=(DST)[i]^i;    \
      }                                       \
    emms();                                   \
    t = (gettime_usec()-t-overhead)/nb_tests;

void test_quant()
{
  const int nb_tests = 150*speed_ref;
  const int max_Q = 31;
  int i;
  CPU *cpu;
  int16_t  Src[8*8], Dst[8*8];

  printf( "\n =====  test quant =====\n" );

  for(i=0; i<64; ++i) {
    Src[i] = i-32;
    Dst[i] = 0;
  }


  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    double t, overhead;
    int tst, s, q;

    if (!init_cpu(cpu))
      continue;

    set_inter_matrix( get_default_inter_matrix() );
    set_intra_matrix( get_default_intra_matrix() );
    overhead = -gettime_usec();
    for(tst=0; tst<nb_tests; ++tst)
      for(s=0, q=1; q<=max_Q; ++q)
        for(i=0; i<64; ++i) s+=Dst[i]^i;
    overhead += gettime_usec();

    TEST_QUANT2(quant4_intra, Dst, Src, 7);
    printf( "%s -   quant4_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=55827) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(quant4_inter, Dst, Src);
    printf( "%s -   quant4_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=58201) printf( "*** CRC ERROR! ***\n" );


    TEST_QUANT2(dequant4_intra, Dst, Src, 7);
    printf( "%s - dequant4_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=193340) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(dequant4_inter, Dst, Src);
    printf( "%s - dequant4_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=116483) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT2(quant_intra, Dst, Src, 7);
    printf( "%s -    quant_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=56885) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(quant_inter, Dst, Src);
    printf( "%s -    quant_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=58056) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT2(dequant_intra, Dst, Src, 7);
    printf( "%s -  dequant_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=-7936) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(dequant_inter, Dst, Src);
    printf( "%s -  dequant_inter %.3f usec       crc=%d\n", cpu->name, t, s );
//    { int k,l; for(k=0; k<8; ++k) { for(l=0; l<8; ++l) printf( "[%.4d]", Dst[k*8+l]); printf("\n"); } }
    if (s!=-33217) printf( "*** CRC ERROR! ***\n" );

    printf( " --- \n" );
  }
}

/*********************************************************************
 * test non-zero AC counting
 *********************************************************************/

#define TEST_CBP(FUNC, SRC)                   \
    t = gettime_usec();                       \
    emms();                                   \
    for(tst=0; tst<nb_tests; ++tst) {         \
      cbp = (FUNC)((SRC));                    \
    }                                         \
    emms();                                   \
    t = (gettime_usec()-t ) / nb_tests;

void test_cbp()
{
  const int nb_tests = 10000*speed_ref;
  int i;
  CPU *cpu;
  int16_t  Src1[6*64], Src2[6*64], Src3[6*64], Src4[6*64];

  printf( "\n =====  test cbp =====\n" );

  for(i=0; i<6*64; ++i) {
    Src1[i] = (i*i*3/8192)&(i/64)&1;  // 'random'
    Src2[i] = (i<3*64);               // half-full
    Src3[i] = ((i+32)>3*64);
    Src4[i] = (i==(3*64+2) || i==(5*64+9));
  }
  
  for(cpu = cpu_short_list2; cpu->name!=0; ++cpu)
  {
    double t;
    int tst, cbp;

    if (!init_cpu(cpu))
      continue;

    TEST_CBP(calc_cbp, Src1);
    printf( "%s -   calc_cbp#1 %.3f usec       cbp=0x%x\n", cpu->name, t, cbp );
    if (cbp!=0x15) printf( "*** CRC ERROR! ***\n" );
    TEST_CBP(calc_cbp, Src2);
    printf( "%s -   calc_cbp#2 %.3f usec       cbp=0x%x\n", cpu->name, t, cbp );
    if (cbp!=0x38) printf( "*** CRC ERROR! ***\n" );
    TEST_CBP(calc_cbp, Src3);
    printf( "%s -   calc_cbp#3 %.3f usec       cbp=0x%x\n", cpu->name, t, cbp );
    if (cbp!=0x0f) printf( "*** CRC ERROR! ***\n" );
    TEST_CBP(calc_cbp, Src4);
    printf( "%s -   calc_cbp#4 %.3f usec       cbp=0x%x\n", cpu->name, t, cbp );
    if (cbp!=0x05) printf( "*** CRC ERROR! ***\n" );
    printf( " --- \n" );
  }
}

/*********************************************************************
 * measure raw decoding speed
 *********************************************************************/

void test_dec(const char *name, int width, int height, int with_chksum)
{
  FILE *f = 0;
  void *dechandle = 0;
  int xerr;
	XVID_INIT_PARAM xinit;
	XVID_DEC_PARAM xparam;
	XVID_DEC_FRAME xframe;
	double t = 0.;
	int nb = 0;
  uint8_t *buf = 0;
  uint8_t *rgb_out = 0;
  int buf_size, pos;
  uint32_t chksum = 0;

	xinit.cpu_flags = 0;
	xvid_init(NULL, 0, &xinit, NULL);
	printf( "API version: %d, core build:%d\n", xinit.api_version, xinit.core_build);


	xparam.width = width;
	xparam.height = height;
	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &xparam, NULL);
	if (xerr!=XVID_ERR_OK) {
	  printf("can't init decoder (err=%d)\n", xerr);
	  return;
	}
	dechandle = xparam.handle;


	f = fopen(name, "rb");
  if (f==0) {
    printf( "can't open file '%s'\n", name);
    return;
  }
  fseek(f, 0, SEEK_END);
  buf_size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (buf_size<=0) {
    printf("error while stating file\n");
    goto End;
  }
  else printf( "Input size: %d\n", buf_size);

  buf = malloc(buf_size); // should be enuf'
  rgb_out = calloc(4, width*height);  // <-room for _RGB24
  if (buf==0 || rgb_out==0) {
    printf( "malloc failed!\n" );
    goto End;
  }

  if (fread(buf, buf_size, 1, f)!=1) {
    printf( "file-read failed\n" );
    goto End;
  }

  nb = 0;
  pos = 0;
  t = -gettime_usec();
  while(1) {
    xframe.bitstream = buf + pos;
    xframe.length = buf_size - pos;
    xframe.image = rgb_out;
    xframe.stride = width;
    xframe.colorspace = XVID_CSP_RGB24;
    xerr = xvid_decore(dechandle, XVID_DEC_DECODE, &xframe, 0);
    nb++;
    pos += xframe.length;
    if (with_chksum) {
      int k = width*height;
      uint32_t *ptr = (uint32_t *)rgb_out;
      while(k-->0) chksum += *ptr++;
    }
    if (pos==buf_size)
      break;
    if (xerr!=XVID_ERR_OK) {
  	  printf("decoding failed for frame #%d (err=%d)!\n", nb, xerr);
  	  break;
  	}
  }
  t += gettime_usec();
  if (t>0.)
    printf( "%d frames decoded in %.3f s -> %.1f FPS\n", nb, t*1.e-6f, (float)(nb*1.e6f/t) );
  if (with_chksum)
    printf("checksum: 0x%.8x\n", chksum);

End:
  if (rgb_out!=0) free(rgb_out);
  if (buf!=0) free(buf);
  if (dechandle!=0) {
    xerr= xvid_decore(dechandle, XVID_DEC_DESTROY, NULL, NULL);
    if (xerr!=XVID_ERR_OK)
	    printf("destroy-decoder failed (err=%d)!\n", xerr);
  }
  if (f!=0) fclose(f);
}

/*********************************************************************
 * non-regression tests
 *********************************************************************/

void test_bugs1()
{
  CPU *cpu;

  printf( "\n =====  (de)quant4_intra saturation bug? =====\n" );

  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    int i;
    int16_t  Src[8*8], Dst[8*8];

    if (!init_cpu(cpu))
      continue;

    for(i=0; i<64; ++i) Src[i] = i-32;
    set_intra_matrix( get_default_intra_matrix() );
    dequant4_intra(Dst, Src, 32, 5);
    printf( "dequant4_intra with CPU=%s:  ", cpu->name);
    printf( "  Out[]= " );
    for(i=0; i<64; ++i) printf( "[%d]", Dst[i]);
    printf( "\n" );
  }

  printf( "\n =====  (de)quant4_inter saturation bug? =====\n" );

  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    int i;
    int16_t  Src[8*8], Dst[8*8];

    if (!init_cpu(cpu))
      continue;

    for(i=0; i<64; ++i) Src[i] = i-32;
    set_inter_matrix( get_default_inter_matrix() );
    dequant4_inter(Dst, Src, 32);
    printf( "dequant4_inter with CPU=%s:  ", cpu->name);
    printf( "  Out[]= " );
    for(i=0; i<64; ++i) printf( "[%d]", Dst[i]);
    printf( "\n" ); 
  }
}

void test_dct_precision_diffs()
{
  CPU *cpu;
  short Blk[8*8], Blk0[8*8];

  printf( "\n =====  fdct/idct saturation diffs =====\n" );

  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    int i;

    if (!init_cpu(cpu))
      continue;

    for(i=0; i<8*8; ++i) {
      Blk0[i] = (i*7-i*i) & 0x7f;
      Blk[i] = Blk0[i];
    }

    fdct(Blk);
    idct(Blk);
    printf( " fdct+idct diffs with CPU=%s: \n", cpu->name );
    for(i=0; i<8; ++i) {
      int j;
      for(j=0; j<8; ++j) printf( " %d ", Blk[i*8+j]-Blk0[i*8+j]); 
      printf("\n"); 
    }
    printf("\n"); 
  }
}


/*********************************************************************
 * main
 *********************************************************************/

int main(int argc, char *argv[])
{
  int what = 0;
  if (argc>1) what = atoi(argv[1]);
  if (what==0 || what==1) test_dct();
  if (what==0 || what==2) test_mb();
  if (what==0 || what==3) test_sad();
  if (what==0 || what==4) test_transfer();
  if (what==0 || what==5) test_quant();
  if (what==0 || what==6) test_cbp();

  if (what==8) {
    int width, height;
    if (argc<5) {
      printf("usage: %s %d [bitstream] [width] [height]\n", argv[0], what);
      return 1;
    }
    width = atoi(argv[3]);
    height = atoi(argv[4]);
    test_dec(argv[2], width, height, (argc>5));
  }

  if (what==-1) {
    test_bugs1();
    test_dct_precision_diffs();
  }
  return 0;
}

/*********************************************************************
 * 'Reference' output (except for timing) on a PIII 1.13Ghz/linux
 *********************************************************************/
/*

 ===== test fdct/idct =====
PLAINC -  2.631 usec       iCrc=3  fCrc=-85
MMX    -  0.596 usec       iCrc=3  fCrc=-67
MMXEXT -  0.608 usec       iCrc=3  fCrc=-67
SSE2   -  0.605 usec       iCrc=3  fCrc=-67
3DNOW  - skipped...
3DNOWE - skipped...

 ===  test block motion ===
PLAINC - interp- h-round0 1.031 usec       iCrc=8107
PLAINC -           round1 1.022 usec       iCrc=8100
PLAINC - interp- v-round0 1.002 usec       iCrc=8108
PLAINC -           round1 1.011 usec       iCrc=8105
PLAINC - interp-hv-round0 1.623 usec       iCrc=8112
PLAINC -           round1 1.621 usec       iCrc=8103
PLAINC - interpolate8x8_c 0.229 usec       iCrc=8107
 --- 
MMX    - interp- h-round0 0.105 usec       iCrc=8107
MMX    -           round1 0.105 usec       iCrc=8100
MMX    - interp- v-round0 0.106 usec       iCrc=8108
MMX    -           round1 0.107 usec       iCrc=8105
MMX    - interp-hv-round0 0.145 usec       iCrc=8112
MMX    -           round1 0.145 usec       iCrc=8103
MMX    - interpolate8x8_c 0.229 usec       iCrc=8107
 --- 
MMXEXT - interp- h-round0 0.027 usec       iCrc=8107
MMXEXT -           round1 0.041 usec       iCrc=8100
MMXEXT - interp- v-round0 0.027 usec       iCrc=8108
MMXEXT -           round1 0.040 usec       iCrc=8105
MMXEXT - interp-hv-round0 0.070 usec       iCrc=8112
MMXEXT -           round1 0.066 usec       iCrc=8103
MMXEXT - interpolate8x8_c 0.027 usec       iCrc=8107
 --- 
SSE2   - interp- h-round0 0.106 usec       iCrc=8107
SSE2   -           round1 0.105 usec       iCrc=8100
SSE2   - interp- v-round0 0.106 usec       iCrc=8108
SSE2   -           round1 0.106 usec       iCrc=8105
SSE2   - interp-hv-round0 0.145 usec       iCrc=8112
SSE2   -           round1 0.145 usec       iCrc=8103
SSE2   - interpolate8x8_c 0.237 usec       iCrc=8107
 --- 
3DNOW  - skipped...
3DNOWE - skipped...

 ======  test SAD ======
PLAINC - sad8    0.296 usec       sad=3776
PLAINC - sad16   1.599 usec       sad=27214
PLAINC - sad16bi 2.350 usec       sad=26274
PLAINC - dev16   1.610 usec       sad=3344
 --- 
MMX    - sad8    0.057 usec       sad=3776
MMX    - sad16   0.178 usec       sad=27214
MMX    - sad16bi 2.381 usec       sad=26274
MMX    - dev16   0.312 usec       sad=3344
 --- 
MMXEXT - sad8    0.036 usec       sad=3776
MMXEXT - sad16   0.106 usec       sad=27214
MMXEXT - sad16bi 0.182 usec       sad=26274
MMXEXT - dev16   0.193 usec       sad=3344
 --- 
SSE2   - sad8    0.057 usec       sad=3776
SSE2   - sad16   0.178 usec       sad=27214
SSE2   - sad16bi 2.427 usec       sad=26274
SSE2   - dev16   0.313 usec       sad=3344
 --- 
3DNOW  - skipped...
3DNOWE - skipped...

 ===  test transfer ===
PLAINC - 8to16     0.124 usec       crc=28288
PLAINC - 16to8     0.753 usec       crc=28288
PLAINC - 8to8      0.041 usec       crc=20352
PLAINC - 16to8add  0.916 usec       crc=25536
PLAINC - 8to16sub  0.812 usec       crc1=28064 crc2=16256
PLAINC - 8to16sub2 0.954 usec       crc=20384
 --- 
MMX    - 8to16     0.037 usec       crc=28288
MMX    - 16to8     0.016 usec       crc=28288
MMX    - 8to8      0.018 usec       crc=20352
MMX    - 16to8add  0.044 usec       crc=25536
MMX    - 8to16sub  0.065 usec       crc1=28064 crc2=16256
MMX    - 8to16sub2 0.110 usec       crc=20384
 --- 
MMXEXT - 8to16     0.032 usec       crc=28288
MMXEXT - 16to8     0.023 usec       crc=28288
MMXEXT - 8to8      0.018 usec       crc=20352
MMXEXT - 16to8add  0.041 usec       crc=25536
MMXEXT - 8to16sub  0.065 usec       crc1=28064 crc2=16256
MMXEXT - 8to16sub2 0.069 usec       crc=20384
 --- 

 =====  test quant =====
PLAINC -   quant4_intra 78.889 usec       crc=55827
PLAINC -   quant4_inter 71.957 usec       crc=58201
PLAINC - dequant4_intra 34.968 usec       crc=193340
PLAINC - dequant4_inter 40.792 usec       crc=116483
PLAINC -    quant_intra 30.845 usec       crc=56885
PLAINC -    quant_inter 34.842 usec       crc=58056
PLAINC -  dequant_intra 33.211 usec       crc=-7936
PLAINC -  dequant_inter 45.486 usec       crc=-33217
 --- 
MMX    -   quant4_intra 9.030 usec       crc=55827
MMX    -   quant4_inter 8.234 usec       crc=58201
MMX    - dequant4_intra 18.330 usec       crc=193340
MMX    - dequant4_inter 19.181 usec       crc=116483
MMX    -    quant_intra 7.124 usec       crc=56885
MMX    -    quant_inter 6.861 usec       crc=58056
MMX    -  dequant_intra 9.048 usec       crc=-7936
MMX    -  dequant_inter 8.203 usec       crc=-33217
 --- 
MMXEXT -   quant4_intra 9.045 usec       crc=55827
MMXEXT -   quant4_inter 8.232 usec       crc=58201
MMXEXT - dequant4_intra 18.250 usec       crc=193340
MMXEXT - dequant4_inter 19.256 usec       crc=116483
MMXEXT -    quant_intra 7.121 usec       crc=56885
MMXEXT -    quant_inter 6.855 usec       crc=58056
MMXEXT -  dequant_intra 9.034 usec       crc=-7936
MMXEXT -  dequant_inter 8.202 usec       crc=-33217
 --- 

 =====  test cbp =====
PLAINC -   calc_cbp#1 0.545 usec       cbp=0x15
PLAINC -   calc_cbp#2 0.540 usec       cbp=0x38
PLAINC -   calc_cbp#3 0.477 usec       cbp=0xf
PLAINC -   calc_cbp#4 0.739 usec       cbp=0x5
 --- 
MMX    -   calc_cbp#1 0.136 usec       cbp=0x15
MMX    -   calc_cbp#2 0.131 usec       cbp=0x38
MMX    -   calc_cbp#3 0.132 usec       cbp=0xf
MMX    -   calc_cbp#4 0.135 usec       cbp=0x5
 --- 
SSE2   -   calc_cbp#1 0.135 usec       cbp=0x15
SSE2   -   calc_cbp#2 0.131 usec       cbp=0x38
SSE2   -   calc_cbp#3 0.134 usec       cbp=0xf
SSE2   -   calc_cbp#4 0.136 usec       cbp=0x5
 --- 
*/
