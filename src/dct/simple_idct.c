/*
 * Simple IDCT
 *
 * Copyright (c) 2001 Michael Niedermayer <michaelni@gmx.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
  based upon some outcommented c code from mpeg2dec (idct_mmx.c
  written by Aaron Holtzman <aholtzma@ess.engr.uvic.ca>) 
 */
#include "../portab.h"
#include "idct.h"

#if 0
#define W1 2841 /* 2048*sqrt (2)*cos (1*pi/16) */
#define W2 2676 /* 2048*sqrt (2)*cos (2*pi/16) */
#define W3 2408 /* 2048*sqrt (2)*cos (3*pi/16) */
#define W4 2048 /* 2048*sqrt (2)*cos (4*pi/16) */
#define W5 1609 /* 2048*sqrt (2)*cos (5*pi/16) */
#define W6 1108 /* 2048*sqrt (2)*cos (6*pi/16) */
#define W7 565  /* 2048*sqrt (2)*cos (7*pi/16) */
#define ROW_SHIFT 8
#define COL_SHIFT 17
#else
#define W1  22725  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W2  21407  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W3  19266  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W4  16383  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W5  12873  //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W6  8867   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define W7  4520   //cos(i*M_PI/16)*sqrt(2)*(1<<14) + 0.5
#define ROW_SHIFT 11
#define COL_SHIFT 20 // 6
#endif

#if defined(ARCH_IS_PPC)

/* signed 16x16 -> 32 multiply add accumulate */
#define MAC16(rt, ra, rb) \
    asm ("maclhw %0, %2, %3" : "=r" (rt) : "0" (rt), "r" (ra), "r" (rb));

/* signed 16x16 -> 32 multiply */
#define MUL16(rt, ra, rb) \
    asm ("mullhw %0, %1, %2" : "=r" (rt) : "r" (ra), "r" (rb));

#else

/* signed 16x16 -> 32 multiply add accumulate */
#define MAC16(rt, ra, rb) rt += (ra) * (rb)

/* signed 16x16 -> 32 multiply */
#define MUL16(rt, ra, rb) rt = (ra) * (rb)

#endif

static __inline void idctRowCondDC (int16_t * const row)
{
	int a0, a1, a2, a3, b0, b1, b2, b3;
#ifdef FAST_64BIT
        uint64_t temp;
#else
        uint32_t temp;
#endif

#ifdef FAST_64BIT
#ifdef ARCH_IS_BIG_ENDIAN
#define ROW0_MASK 0xffff000000000000LL
#else
#define ROW0_MASK 0xffffLL
#endif
	if ( ((((uint64_t *)row)[0] & ~ROW0_MASK) | 
              ((uint64_t *)row)[1]) == 0) {
            temp = (row[0] << 3) & 0xffff;
            temp += temp << 16;
            temp += temp << 32;
            ((uint64_t *)row)[0] = temp;
            ((uint64_t *)row)[1] = temp;
            return;
	}
#else
	if (!(((uint32_t*)row)[1] |
              ((uint32_t*)row)[2] |
              ((uint32_t*)row)[3] | 
              row[1])) {
            temp = (row[0] << 3) & 0xffff;
            temp += temp << 16;
            ((uint32_t*)row)[0]=((uint32_t*)row)[1] =
		((uint32_t*)row)[2]=((uint32_t*)row)[3] = temp;
		return;
	}
#endif

        a0 = (W4 * row[0]) + (1 << (ROW_SHIFT - 1));
	a1 = a0;
	a2 = a0;
	a3 = a0;

        /* no need to optimize : gcc does it */
        a0 += W2 * row[2];
        a1 += W6 * row[2];
        a2 -= W6 * row[2];
        a3 -= W2 * row[2];

        MUL16(b0, W1, row[1]);
        MAC16(b0, W3, row[3]);
        MUL16(b1, W3, row[1]);
        MAC16(b1, -W7, row[3]);
        MUL16(b2, W5, row[1]);
        MAC16(b2, -W1, row[3]);
        MUL16(b3, W7, row[1]);
        MAC16(b3, -W5, row[3]);

#ifdef FAST_64BIT
        temp = ((uint64_t*)row)[1];
#else
        temp = ((uint32_t*)row)[2] | ((uint32_t*)row)[3];
#endif
	if (temp != 0) {
            a0 += W4*row[4] + W6*row[6];
            a1 += - W4*row[4] - W2*row[6];
            a2 += - W4*row[4] + W2*row[6];
            a3 += W4*row[4] - W6*row[6];

            MAC16(b0, W5, row[5]);
            MAC16(b0, W7, row[7]);
            
            MAC16(b1, -W1, row[5]);
            MAC16(b1, -W5, row[7]);
            
            MAC16(b2, W7, row[5]);
            MAC16(b2, W3, row[7]);
            
            MAC16(b3, W3, row[5]);
            MAC16(b3, -W1, row[7]);
	}

	row[0] = (a0 + b0) >> ROW_SHIFT;
	row[7] = (a0 - b0) >> ROW_SHIFT;
	row[1] = (a1 + b1) >> ROW_SHIFT;
	row[6] = (a1 - b1) >> ROW_SHIFT;
	row[2] = (a2 + b2) >> ROW_SHIFT;
	row[5] = (a2 - b2) >> ROW_SHIFT;
	row[3] = (a3 + b3) >> ROW_SHIFT;
	row[4] = (a3 - b3) >> ROW_SHIFT;
}


static __inline void idctSparseCol (int16_t * const col)
{
	int a0, a1, a2, a3, b0, b1, b2, b3;

        /* XXX: I did that only to give same values as previous code */
	a0 = W4 * (col[8*0] + ((1<<(COL_SHIFT-1))/W4));
	a1 = a0;
	a2 = a0;
	a3 = a0;

        a0 +=  + W2*col[8*2];
        a1 +=  + W6*col[8*2];
        a2 +=  - W6*col[8*2];
        a3 +=  - W2*col[8*2];

        MUL16(b0, W1, col[8*1]);
        MUL16(b1, W3, col[8*1]);
        MUL16(b2, W5, col[8*1]);
        MUL16(b3, W7, col[8*1]);

        MAC16(b0, + W3, col[8*3]);
        MAC16(b1, - W7, col[8*3]);
        MAC16(b2, - W1, col[8*3]);
        MAC16(b3, - W5, col[8*3]);

	if(col[8*4]){
            a0 += + W4*col[8*4];
            a1 += - W4*col[8*4];
            a2 += - W4*col[8*4];
            a3 += + W4*col[8*4];
	}

	if (col[8*5]) {
            MAC16(b0, + W5, col[8*5]);
            MAC16(b1, - W1, col[8*5]);
            MAC16(b2, + W7, col[8*5]);
            MAC16(b3, + W3, col[8*5]);
	}

	if(col[8*6]){
            a0 += + W6*col[8*6];
            a1 += - W2*col[8*6];
            a2 += + W2*col[8*6];
            a3 += - W6*col[8*6];
	}

	if (col[8*7]) {
            MAC16(b0, + W7, col[8*7]);
            MAC16(b1, - W5, col[8*7]);
            MAC16(b2, + W3, col[8*7]);
            MAC16(b3, - W1, col[8*7]);
	}

        col[0 ] = ((a0 + b0) >> COL_SHIFT);
        col[8 ] = ((a1 + b1) >> COL_SHIFT);
        col[16] = ((a2 + b2) >> COL_SHIFT);
        col[24] = ((a3 + b3) >> COL_SHIFT);
        col[32] = ((a3 - b3) >> COL_SHIFT);
        col[40] = ((a2 - b2) >> COL_SHIFT);
        col[48] = ((a1 - b1) >> COL_SHIFT);
        col[56] = ((a0 - b0) >> COL_SHIFT);
}

void simple_idct_c(int16_t * const block)
{
    int i;
    for(i=0; i<8; i++)
        idctRowCondDC(block + i*8);
    
    for(i=0; i<8; i++)
        idctSparseCol(block + i);
}


/* Input permutation for the simple_idct_mmx */
static const uint8_t simple_mmx_permutation[64]={
	0x00, 0x08, 0x04, 0x09, 0x01, 0x0C, 0x05, 0x0D, 
	0x10, 0x18, 0x14, 0x19, 0x11, 0x1C, 0x15, 0x1D, 
	0x20, 0x28, 0x24, 0x29, 0x21, 0x2C, 0x25, 0x2D, 
	0x12, 0x1A, 0x16, 0x1B, 0x13, 0x1E, 0x17, 0x1F, 
	0x02, 0x0A, 0x06, 0x0B, 0x03, 0x0E, 0x07, 0x0F, 
	0x30, 0x38, 0x34, 0x39, 0x31, 0x3C, 0x35, 0x3D, 
	0x22, 0x2A, 0x26, 0x2B, 0x23, 0x2E, 0x27, 0x2F, 
	0x32, 0x3A, 0x36, 0x3B, 0x33, 0x3E, 0x37, 0x3F,
};

#if defined(ARCH_IS_IA32)
/* wrapper function, as simple_idct_mmx expects data to be permutated */
void simple_idct_mmx2(int16_t * const block)
{
    int16_t tmp[64];
    int i;

    for(i=0;i<64;i++) tmp[simple_mmx_permutation[i]] = block[i];
    simple_idct_mmx(tmp);
    for(i=0;i<64;i++) block[i] = tmp[i];
}
#endif
