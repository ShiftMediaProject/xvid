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
 *  bullet-proof (should plug some .md5 here)...
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
#ifdef	WIN32
#include <time.h>  /* for clock */
#else
#include <sys/time.h>  /* for gettimeofday */
#endif
#include <string.h>    /* for memset */
#include <assert.h>

#include "xvid.h"

/* inner guts */
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

#include <math.h>
#ifndef M_PI
#  define M_PI     3.14159265359
#  define M_PI_2   1.5707963268
#endif
const int speed_ref = 100;  /* on slow machines, decrease this value */

/*********************************************************************
 * misc
 *********************************************************************/

 /* returns time in micro-s*/
double gettime_usec()
{    
#ifdef	WIN32
  return clock()*1000;
#else
  struct timeval  tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec*1.0e6 + tv.tv_usec;
#endif
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
, { "IA64  ", XVID_CPU_IA64 }  
/*, { "TSC   ", XVID_CPU_TSC } */
, { 0, 0 } }

, cpu_short_list[] =
{ { "PLAINC", 0 }
, { "MMX   ", XVID_CPU_MMX }
/*, { "MMXEXT", XVID_CPU_MMXEXT | XVID_CPU_MMX } */
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
  /*    xinit.cpu_flags = XVID_CPU_MMX | XVID_CPU_FORCE; */
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
    double t, PSNR, MSE;

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
    MSE = 0.;
    for(i=0; i<8*8; ++i) {
      double delta = 1.0*(iDst[i] - iDst0[i]);
      MSE += delta*delta;
    }
    PSNR = (MSE==0.) ? 1.e6 : -4.3429448*log( MSE/64. );
    printf( "%s -  %.3f usec       PSNR=%.3f  MSE=%.3f\n",
      cpu->name, t, PSNR, MSE );
    if (ABS(MSE)>=64) printf( "*** CRC ERROR! ***\n" );
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
        /* try to have every possible combinaison of rounding... */
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


       /* this is a new function, as of 06.06.2002 */
#if 0
    TEST_MB2(interpolate8x8_avrg);
    printf( "%s - interpolate8x8_c %.3f usec       iCrc=%d\n", cpu->name, t, iCrc );
    if (iCrc!=8107) printf( "*** CRC ERROR! ***\n" );
#endif

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
#if 1
    TEST_TRANSFER3(transfer_8to16sub2, Dst16, Src8, Ref1, Ref2);
    printf( "%s - 8to16sub2 %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=20384) printf( "*** CRC ERROR! ***\n" );
/*    for(i=0; i<64; ++i) printf( "[%d]", Dst16[i]); */
/*    printf("\n"); */
#endif
    printf( " --- \n" );
  }
}

/*********************************************************************
 * test quantization
 *********************************************************************/

#define TEST_QUANT(FUNC, DST, SRC)              \
    t = gettime_usec();                         \
    for(s=0,qm=1; qm<=255; ++qm) {              \
      for(i=0; i<8*8; ++i) Quant[i] = qm;       \
      set_inter_matrix( Quant );                \
      emms();                                   \
      for(q=1; q<=max_Q; ++q) {                 \
        for(tst=0; tst<nb_tests; ++tst)         \
          (FUNC)((DST), (SRC), q);              \
        for(i=0; i<64; ++i) s+=(DST)[i]^i^qm;   \
      }                                         \
      emms();                                   \
    }                                           \
    t = (gettime_usec()-t-overhead)/nb_tests/qm;\
    s = (s&0xffff)^(s>>16)

#define TEST_QUANT2(FUNC, DST, SRC)             \
    t = gettime_usec();                         \
    for(s=0,qm=1; qm<=255; ++qm) {              \
      for(i=0; i<8*8; ++i) Quant[i] = qm;       \
      set_intra_matrix( Quant );                \
      emms();                                   \
      for(q=1; q<=max_Q; ++q) {                 \
        for(tst=0; tst<nb_tests; ++tst)         \
          (FUNC)((DST), (SRC), q, q);           \
        for(i=0; i<64; ++i) s+=(DST)[i]^i^qm;   \
      }                                         \
      emms();                                   \
    }                                           \
    t = (gettime_usec()-t-overhead)/nb_tests/qm;\
    s = (s&0xffff)^(s>>16)

void test_quant()
{
  const int nb_tests = 1*speed_ref;
  const int max_Q = 31;
  int i, qm;
  CPU *cpu;
  int16_t  Src[8*8], Dst[8*8];
  uint8_t Quant[8*8];

  printf( "\n =====  test quant =====\n" );

    /* we deliberately enfringe the norm's specified range [-127,127], */
    /* to test the robustness of the iquant module */
  for(i=0; i<64; ++i) {
    Src[i] = 1 + (i-32) * (i&6);
    Dst[i] = 0;
  }

  for(cpu = cpu_short_list; cpu->name!=0; ++cpu)
  {
    double t, overhead;
    int tst, q;
    uint32_t s;

    if (!init_cpu(cpu))
      continue;

    overhead = -gettime_usec();
    for(s=0,qm=1; qm<=255; ++qm) {
      for(i=0; i<8*8; ++i) Quant[i] = qm;
      set_inter_matrix( Quant );
      for(q=1; q<=max_Q; ++q)
        for(i=0; i<64; ++i) s+=Dst[i]^i^qm;
    }
    overhead += gettime_usec();

#if 1
    TEST_QUANT2(quant4_intra, Dst, Src);
    printf( "%s -   quant4_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=29809) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(quant4_inter, Dst, Src);
    printf( "%s -   quant4_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=12574) printf( "*** CRC ERROR! ***\n" );
#endif
#if 1
    TEST_QUANT2(dequant4_intra, Dst, Src);
    printf( "%s - dequant4_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=24052) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(dequant4_inter, Dst, Src);
    printf( "%s - dequant4_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=63847) printf( "*** CRC ERROR! ***\n" );
#endif
#if 1
    TEST_QUANT2(quant_intra, Dst, Src);
    printf( "%s -    quant_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=25662) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(quant_inter, Dst, Src);
    printf( "%s -    quant_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=23972) printf( "*** CRC ERROR! ***\n" );
#endif
#if 1
    TEST_QUANT2(dequant_intra, Dst, Src);
    printf( "%s -  dequant_intra %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=49900) printf( "*** CRC ERROR! ***\n" );

    TEST_QUANT(dequant_inter, Dst, Src);
    printf( "%s -  dequant_inter %.3f usec       crc=%d\n", cpu->name, t, s );
    if (s!=48899) printf( "*** CRC ERROR! ***\n" );
#endif
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
    Src1[i] = (i*i*3/8192)&(i/64)&1;  /* 'random' */
    Src2[i] = (i<3*64);               /* half-full */
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
 * fdct/idct IEEE1180 compliance
 *********************************************************************/

typedef struct {
  long Errors[64];
  long Sqr_Errors[64];
  long Max_Errors[64];
  long Nb;
} STATS_8x8;

void init_stats(STATS_8x8 *S)
{
  int i;
  for(i=0; i<64; ++i) {
    S->Errors[i]     = 0;
    S->Sqr_Errors[i] = 0;
    S->Max_Errors[i] = 0;
  }
  S->Nb = 0;
}

void store_stats(STATS_8x8 *S, short Blk[64], short Ref[64])
{
  int i;
  for(i=0; i<64; ++i)
  {
    short Err = Blk[i] - Ref[i];
    S->Errors[i] += Err;
    S->Sqr_Errors[i] += Err * Err;
    if (Err<0) Err = -Err;
    if (S->Max_Errors[i]<Err)
      S->Max_Errors[i] = Err;
  }
  S->Nb++;
}

void print_stats(STATS_8x8 *S)
{
  int i;
  double Norm;

  assert(S->Nb>0);
  Norm = 1. / (double)S->Nb;
  printf("\n== Max absolute values of errors ==\n");
  for(i=0; i<64; i++) {
    printf("  %4ld", S->Max_Errors[i]);
    if ((i&7)==7) printf("\n");
  }

  printf("\n== Mean square errors ==\n");
  for(i=0; i<64; i++)
  {
    double Err = Norm * (double)S->Sqr_Errors[i];
    printf(" %.3f", Err);
    if ((i&7)==7) printf("\n");
  }

  printf("\n== Mean errors ==\n");
  for(i=0; i<64; i++)
  {
    double Err = Norm * (double)S->Errors[i];
    printf(" %.3f", Err);
    if ((i&7)==7) printf("\n");
  }
  printf("\n");
}

static const char *CHECK(double v, double l) {
  if (fabs(v)<=l) return "ok";
  else return "FAIL!";
}

void report_stats(STATS_8x8 *S, const double *Limits)
{
  int i;
  double Norm, PE, PMSE, OMSE, PME, OME;

  assert(S->Nb>0);
  Norm = 1. / (double)S->Nb;
  PE = 0.;
  for(i=0; i<64; i++) {
    if (PE<S->Max_Errors[i])
      PE = S->Max_Errors[i];
  }
  
  PMSE = 0.;
  OMSE = 0.;
  for(i=0; i<64; i++)
  {
    double Err = Norm * (double)S->Sqr_Errors[i];
    OMSE += Err;
    if (PMSE < Err) PMSE = Err;
  }
  OMSE /= 64.;

  PME = 0.;
  OME = 0.;
  for(i=0; i<64; i++)
  {
    double Err = Norm * (double)S->Errors[i];
    OME += Err;
    Err = fabs(Err);
    if (PME < Err) PME = Err;
  }
  OME /= 64.;

  printf( "Peak error:   %4.4f\n", PE );
  printf( "Peak MSE:     %4.4f\n", PMSE );
  printf( "Overall MSE:  %4.4f\n", OMSE );
  printf( "Peak ME:      %4.4f\n", PME );
  printf( "Overall ME:   %4.4f\n", OME );

  if (Limits!=0)
  {
    printf( "[PE<=%.4f %s]  ", Limits[0], CHECK(PE,   Limits[0]) );
    printf( "\n" );
    printf( "[PMSE<=%.4f %s]", Limits[1], CHECK(PMSE, Limits[1]) );
    printf( "[OMSE<=%.4f %s]", Limits[2], CHECK(OMSE, Limits[2]) );
    printf( "\n" );
    printf( "[PME<=%.4f %s] ", Limits[3], CHECK(PME , Limits[3]) );
    printf( "[OME<=%.4f %s] ", Limits[4], CHECK(OME , Limits[4]) );
    printf( "\n" );
  }
}

/*//////////////////////////////////////////////////////// */
/* Pseudo-random generator specified by IEEE 1180 */

static long ieee_seed = 1;
static void ieee_reseed(long s) {
  ieee_seed = s;
}
static long ieee_rand(int Min, int Max)
{
  static double z = (double) 0x7fffffff;

  long i,j;
  double x;

  ieee_seed = (ieee_seed * 1103515245) + 12345;
  i = ieee_seed & 0x7ffffffe;
  x = ((double) i) / z;
  x *= (Max-Min+1);
  j = (long)x;
  j = j + Min;
  assert(j>=Min && j<=Max);
  return (short)j;
}

#define CLAMP(x, M)   (x) = ((x)<-(M)) ? (-(M)) : ((x)>=(M) ? ((M)-1) : (x))

static double Cos[8][8];
static void init_ref_dct()
{
  int i, j;
  for(i=0; i<8; i++)
  {
    double scale = (i == 0) ? sqrt(0.125) : 0.5;
    for (j=0; j<8; j++)
      Cos[i][j] = scale*cos( (M_PI/8.0)*i*(j + 0.5) );
  }
}

void ref_idct(short *M)
{
  int i, j, k;
  double Tmp[8][8];

  for(i=0; i<8; i++) {
    for(j=0; j<8; j++)
    {
      double Sum = 0.0;
      for (k=0; k<8; k++) Sum += Cos[k][j]*M[8*i+k];
      Tmp[i][j] = Sum;
    }
  }
  for(i=0; i<8; i++) {
    for(j=0; j<8; j++) {
      double Sum = 0.0;
      for (k=0; k<8; k++) Sum += Cos[k][i]*Tmp[k][j];
      M[8*i+j] = (short)floor(Sum + .5);
    }
  }
}

void ref_fdct(short *M)
{
  int i, j, k;
  double Tmp[8][8];

  for(i=0; i<8; i++) {
    for(j=0; j<8; j++)
    {
      double Sum = 0.0;
      for (k=0; k<8; k++) Sum += Cos[j][k]*M[8*i+k];
      Tmp[i][j] = Sum;
    }
  }
  for(i=0; i<8; i++) {
    for(j=0; j<8; j++) {
      double Sum = 0.0;
      for (k=0; k<8; k++) Sum += Cos[i][k]*Tmp[k][j];
      M[8*i+j] = (short)floor(Sum + 0.5);
    }
  }
}

void test_IEEE1180_compliance(int Min, int Max, int Sign)
{
  static const double ILimits[5] = { 1., 0.06, 0.02, 0.015, 0.0015 };
  int Loops = 10000;
  int i, m, n;
  short Blk0[64];     /* reference */
  short Blk[64], iBlk[64];
  short Ref_FDCT[64];
  short Ref_IDCT[64];

  STATS_8x8 FStats; /* forward dct stats */
  STATS_8x8 IStats; /* inverse dct stats */

  CPU *cpu;

  init_ref_dct();

  for(cpu = cpu_list; cpu->name!=0; ++cpu)
  {
    if (!init_cpu(cpu))
      continue;

    printf( "\n===== IEEE test for %s ==== (Min=%d Max=%d Sign=%d Loops=%d)\n",
      cpu->name, Min, Max, Sign, Loops);

    init_stats(&IStats);
    init_stats(&FStats);

    ieee_reseed(1);
    for(n=0; n<Loops; ++n)
    {
      for(i=0; i<64; ++i)
        Blk0[i] = (short)ieee_rand(Min,Max) * Sign;

        /* hmm, I'm not quite sure this is exactly */
        /* the tests described in the norm. check... */

      memcpy(Ref_FDCT, Blk0, 64*sizeof(short));
      ref_fdct(Ref_FDCT);
      for(i=0; i<64; i++) CLAMP( Ref_FDCT[i], 2048 );

      memcpy(Blk, Blk0, 64*sizeof(short));
      emms(); fdct(Blk); emms();
      for(i=0; i<64; i++) CLAMP( Blk[i], 2048 );

      store_stats(&FStats, Blk, Ref_FDCT);


      memcpy(Ref_IDCT, Ref_FDCT, 64*sizeof(short));
      ref_idct(Ref_IDCT);
      for (i=0; i<64; i++) CLAMP( Ref_IDCT[i], 256 );

      memcpy(iBlk, Ref_FDCT, 64*sizeof(short));
      emms(); idct(iBlk); emms();
      for(i=0; i<64; i++) CLAMP( iBlk[i], 256 );

      store_stats(&IStats, iBlk, Ref_IDCT);
    }


    printf( "\n  -- FDCT report --\n" );
/*    print_stats(&FStats); */
    report_stats(&FStats, 0); /* so far I know, IEEE1180 says nothing for fdct */

    for(i=0; i<64; i++) Blk[i] = 0;
    emms(); fdct(Blk); emms();
    for(m=i=0; i<64; i++) if (Blk[i]!=0) m++;
    printf( "FDCT(0) == 0 ?  %s\n", (m!=0) ? "NOPE!" : "yup." );

    printf( "\n  -- IDCT report --\n" );
/*    print_stats(&IStats); */
    report_stats(&IStats, ILimits);


    for(i=0; i<64; i++) Blk[i] = 0;
    emms(); idct(Blk); emms();
    for(m=i=0; i<64; i++) if (Blk[i]!=0) m++;
    printf( "IDCT(0) == 0 ?  %s\n", (m!=0) ? "NOPE!" : "yup." );
  }
}


void test_dct_saturation(int Min, int Max)
{
    /* test behaviour on input range fringe */

  int i, n, p;
  CPU *cpu;
/*  const short IDCT_MAX =  2047;  // 12bits input */
/*  const short IDCT_MIN = -2048; */
/*  const short IDCT_OUT =   256;  // 9bits ouput */
  const int Partitions = 4;
  const int Loops = 10000 / Partitions;

  init_ref_dct();

  for(cpu = cpu_list; cpu->name!=0; ++cpu)
  {
    short Blk0[64], Blk[64];
    STATS_8x8 Stats;

    if (!init_cpu(cpu))
      continue;

    printf( "\n===== IEEE test for %s Min=%d Max=%d =====\n",
      cpu->name, Min, Max );

              /* FDCT tests // */

    init_stats(&Stats);

      /* test each computation channels separately */
    for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? Max : 0;
    ref_fdct(Blk0);
    emms(); fdct(Blk); emms();
    store_stats(&Stats, Blk, Blk0);

    for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? Min : 0;
    ref_fdct(Blk0);
    emms(); fdct(Blk); emms();
    store_stats(&Stats, Blk, Blk0);

      /* randomly saturated inputs */
    for(p=0; p<Partitions; ++p)
    {
      for(n=0; n<Loops; ++n)
      {
        for(i=0; i<64; ++i)
          Blk0[i] = Blk[i] = (ieee_rand(0,Partitions)>=p)? Max : Min;
        ref_fdct(Blk0);
        emms(); fdct(Blk); emms();
        store_stats(&Stats, Blk, Blk0);
      }
    }
    printf( "\n  -- FDCT saturation report --\n" );
    report_stats(&Stats, 0);


              /* IDCT tests */
#if 0
      /* no finished yet */

    init_stats(&Stats);

    /* test each computation channel separately */
    for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? IDCT_MAX : 0;
    ref_idct(Blk0);
    emms(); idct(Blk); emms();
    for(i=0; i<64; i++) { CLAMP(Blk0[i], IDCT_OUT); CLAMP(Blk[i], IDCT_OUT); }
    store_stats(&Stats, Blk, Blk0);

    for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? IDCT_MIN : 0;
    ref_idct(Blk0);
    emms(); idct(Blk); emms();
    for(i=0; i<64; i++) { CLAMP(Blk0[i], IDCT_OUT); CLAMP(Blk[i], IDCT_OUT); }
    store_stats(&Stats, Blk, Blk0);

      /* randomly saturated inputs */
    for(p=0; p<Partitions; ++p)
    {
      for(n=0; n<Loops; ++n)
      {
        for(i=0; i<64; ++i)
          Blk0[i] = Blk[i] = (ieee_rand(0,Partitions)>=p)? IDCT_MAX : IDCT_MIN;
        ref_idct(Blk0);
        emms(); idct(Blk); emms();
        for(i=0; i<64; i++) { CLAMP(Blk0[i],IDCT_OUT); CLAMP(Blk[i],IDCT_OUT); }
        store_stats(&Stats, Blk, Blk0);
      }
    }

    printf( "\n  -- IDCT saturation report --\n" );
    print_stats(&Stats);
    report_stats(&Stats, 0);
#endif
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

	xinit.cpu_flags = XVID_CPU_MMX | XVID_CPU_FORCE;
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

  buf = malloc(buf_size); /* should be enuf' */
  rgb_out = calloc(4, width*height);  /* <-room for _RGB24 */
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
    dequant4_intra(Dst, Src, 31, 5);
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
    dequant4_inter(Dst, Src, 31);
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

  printf( "\n =====  fdct/idct precision diffs =====\n" );

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

void test_quant_bug()
{
  const int max_Q = 31;
  int i, n, qm, q;
  CPU *cpu;
  int16_t  Src[8*8], Dst[8*8];
  uint8_t Quant[8*8];
  CPU cpu_bug_list[] = { { "PLAINC", 0 }, { "MMX   ", XVID_CPU_MMX }, {0,0} };
  uint16_t Crcs_Inter[2][32];
  uint16_t Crcs_Intra[2][32];
  printf( "\n =====  test MPEG4-quantize bug =====\n" );

  for(i=0; i<64; ++i) Src[i] = 2048*(i-32)/32;

#if 1
  for(qm=1; qm<=255; ++qm)
  {
    for(i=0; i<8*8; ++i) Quant[i] = qm;
    set_inter_matrix( Quant );

    for(n=0, cpu = cpu_bug_list; cpu->name!=0; ++cpu, ++n)
    {
      uint16_t s;

      if (!init_cpu(cpu))
        continue;

      for(q=1; q<=max_Q; ++q) {
        emms();
        quant4_inter( Dst, Src, q );
        emms();
        for(s=0, i=0; i<64; ++i) s+=((uint16_t)Dst[i])^i;
        Crcs_Inter[n][q] = s;
      }
    }

    for(q=1; q<=max_Q; ++q)
      for(i=0; i<n-1; ++i)
        if (Crcs_Inter[i][q]!=Crcs_Inter[i+1][q])
          printf( "Discrepancy Inter: qm=%d, q=%d  -> %d/%d !\n",
            qm, q, Crcs_Inter[i][q], Crcs_Inter[i+1][q]);
  }
#endif

#if 1
  for(qm=1; qm<=255; ++qm)
  {
    for(i=0; i<8*8; ++i) Quant[i] = qm;
    set_intra_matrix( Quant );

    for(n=0, cpu = cpu_bug_list; cpu->name!=0; ++cpu, ++n)
    {
      uint16_t s;

      if (!init_cpu(cpu))
        continue;

      for(q=1; q<=max_Q; ++q) {
        emms();
        quant4_intra( Dst, Src, q, q);
        emms();
        for(s=0, i=0; i<64; ++i) s+=((uint16_t)Dst[i])^i;
        Crcs_Intra[n][q] = s;
      }
    }

    for(q=1; q<=max_Q; ++q)
      for(i=0; i<n-1; ++i)
        if (Crcs_Intra[i][q]!=Crcs_Intra[i+1][q])
          printf( "Discrepancy Intra: qm=%d, q=%d  -> %d/%d!\n",
            qm, q, Crcs_Inter[i][q], Crcs_Inter[i+1][q]);
  }
#endif
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

  if (what==7) {
    test_IEEE1180_compliance(-256, 255, 1);
#if 0
    test_IEEE1180_compliance(-256, 255,-1);
    test_IEEE1180_compliance(  -5,   5, 1);
    test_IEEE1180_compliance(  -5,   5,-1);
    test_IEEE1180_compliance(-300, 300, 1);
    test_IEEE1180_compliance(-300, 300,-1);
#endif
  }
  if (what==8) test_dct_saturation(-256, 255);

  if (what==9) {
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
    test_dct_precision_diffs();
    test_bugs1();
  }
  if (what==-2)
    test_quant_bug();

  return 0;
}

/*********************************************************************
 * 'Reference' output (except for timing) on a PIII 1.13Ghz/linux
 *********************************************************************/

    /* as of 07/01/2002, there's a problem with mpeg4-quantization */
/*

 ===== test fdct/idct =====
PLAINC -  3.312 usec       PSNR=13.291  MSE=3.000
MMX    -  0.591 usec       PSNR=13.291  MSE=3.000
MMXEXT -  0.577 usec       PSNR=13.291  MSE=3.000
SSE2   -  0.588 usec       PSNR=13.291  MSE=3.000
3DNOW  - skipped...
3DNOWE - skipped...

 ===  test block motion ===
PLAINC - interp- h-round0 0.911 usec       iCrc=8107
PLAINC -           round1 0.863 usec       iCrc=8100
PLAINC - interp- v-round0 0.860 usec       iCrc=8108
PLAINC -           round1 0.857 usec       iCrc=8105
PLAINC - interp-hv-round0 2.103 usec       iCrc=8112
PLAINC -           round1 2.050 usec       iCrc=8103
 --- 
MMX    - interp- h-round0 0.105 usec       iCrc=8107
MMX    -           round1 0.106 usec       iCrc=8100
MMX    - interp- v-round0 0.106 usec       iCrc=8108
MMX    -           round1 0.106 usec       iCrc=8105
MMX    - interp-hv-round0 0.145 usec       iCrc=8112
MMX    -           round1 0.145 usec       iCrc=8103
 --- 
MMXEXT - interp- h-round0 0.028 usec       iCrc=8107
MMXEXT -           round1 0.041 usec       iCrc=8100
MMXEXT - interp- v-round0 0.027 usec       iCrc=8108
MMXEXT -           round1 0.041 usec       iCrc=8105
MMXEXT - interp-hv-round0 0.066 usec       iCrc=8112
MMXEXT -           round1 0.065 usec       iCrc=8103
 --- 
SSE2   - interp- h-round0 0.109 usec       iCrc=8107
SSE2   -           round1 0.105 usec       iCrc=8100
SSE2   - interp- v-round0 0.106 usec       iCrc=8108
SSE2   -           round1 0.109 usec       iCrc=8105
SSE2   - interp-hv-round0 0.145 usec       iCrc=8112
SSE2   -           round1 0.145 usec       iCrc=8103
 --- 
3DNOW  - skipped...
3DNOWE - skipped...

 ======  test SAD ======
PLAINC - sad8    0.251 usec       sad=3776
PLAINC - sad16   1.601 usec       sad=27214
PLAINC - sad16bi 2.371 usec       sad=26274
PLAINC - dev16   1.564 usec       sad=3344
 --- 
MMX    - sad8    0.057 usec       sad=3776
MMX    - sad16   0.182 usec       sad=27214
MMX    - sad16bi 2.462 usec       sad=26274
MMX    - dev16   0.311 usec       sad=3344
 --- 
MMXEXT - sad8    0.036 usec       sad=3776
MMXEXT - sad16   0.109 usec       sad=27214
MMXEXT - sad16bi 0.143 usec       sad=26274
MMXEXT - dev16   0.192 usec       sad=3344
 --- 
SSE2   - sad8    0.057 usec       sad=3776
SSE2   - sad16   0.179 usec       sad=27214
SSE2   - sad16bi 2.456 usec       sad=26274
SSE2   - dev16   0.321 usec       sad=3344
 --- 
3DNOW  - skipped...
3DNOWE - skipped...

 ===  test transfer ===
PLAINC - 8to16     0.151 usec       crc=28288
PLAINC - 16to8     1.113 usec       crc=28288
PLAINC - 8to8      0.043 usec       crc=20352
PLAINC - 16to8add  1.069 usec       crc=25536
PLAINC - 8to16sub  0.631 usec       crc1=28064 crc2=16256
PLAINC - 8to16sub2 0.597 usec       crc=20384
 --- 
MMX    - 8to16     0.032 usec       crc=28288
MMX    - 16to8     0.024 usec       crc=28288
MMX    - 8to8      0.020 usec       crc=20352
MMX    - 16to8add  0.043 usec       crc=25536
MMX    - 8to16sub  0.066 usec       crc1=28064 crc2=16256
MMX    - 8to16sub2 0.111 usec       crc=20384
 --- 

 =====  test quant =====
PLAINC -   quant4_intra 74.248 usec       crc=29809
PLAINC -   quant4_inter 70.850 usec       crc=12574
PLAINC - dequant4_intra 40.628 usec       crc=24052
PLAINC - dequant4_inter 45.691 usec       crc=63847
PLAINC -    quant_intra 43.357 usec       crc=25662
PLAINC -    quant_inter 33.410 usec       crc=23972
PLAINC -  dequant_intra 36.384 usec       crc=49900
PLAINC -  dequant_inter 48.930 usec       crc=48899
 --- 
MMX    -   quant4_intra 7.445 usec       crc=3459
*** CRC ERROR! ***
MMX    -   quant4_inter 5.384 usec       crc=51072
*** CRC ERROR! ***
MMX    - dequant4_intra 5.515 usec       crc=24052
MMX    - dequant4_inter 7.745 usec       crc=63847
MMX    -    quant_intra 4.661 usec       crc=25662
MMX    -    quant_inter 4.406 usec       crc=23972
MMX    -  dequant_intra 4.928 usec       crc=49900
MMX    -  dequant_inter 4.532 usec       crc=48899
 --- 

 =====  test cbp =====
PLAINC -   calc_cbp#1 0.371 usec       cbp=0x15
PLAINC -   calc_cbp#2 0.432 usec       cbp=0x38
PLAINC -   calc_cbp#3 0.339 usec       cbp=0xf
PLAINC -   calc_cbp#4 0.506 usec       cbp=0x5
 --- 
MMX    -   calc_cbp#1 0.136 usec       cbp=0x15
MMX    -   calc_cbp#2 0.134 usec       cbp=0x38
MMX    -   calc_cbp#3 0.138 usec       cbp=0xf
MMX    -   calc_cbp#4 0.135 usec       cbp=0x5
 --- 
SSE2   -   calc_cbp#1 0.136 usec       cbp=0x15
SSE2   -   calc_cbp#2 0.133 usec       cbp=0x38
SSE2   -   calc_cbp#3 0.133 usec       cbp=0xf
SSE2   -   calc_cbp#4 0.141 usec       cbp=0x5
 --- 

*/
