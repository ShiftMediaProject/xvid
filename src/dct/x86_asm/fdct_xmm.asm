;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX/SSE forward discrete cosine transform -
; *  Copyright(C) 2002 Pascal Massimino <skal@planet-d.net>
; *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder
; *
; *  XviD is free software; you can redistribute it and/or modify it
; *  under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; *  Under section 8 of the GNU General Public License, the copyright
; *  holders of XVID explicitly forbid distribution in the following
; *  countries:
; *
; * - Japan
; * - United States of America
; *
; *  Linking XviD statically or dynamically with other modules is making a
; *  combined work based on XviD.  Thus, the terms and conditions of the
; *  GNU General Public License cover the whole combination.
; *
; *  As a special exception, the copyright holders of XviD give you
; *  permission to link XviD with independent modules that communicate with
; *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
; *  license terms of these independent modules, and to copy and distribute
; *  the resulting combined work under terms of your choice, provided that
; *  every copy of the combined work is accompanied by a complete copy of
; *  the source code of XviD (the version of XviD used to produce the
; *  combined work), being distributed under the terms of the GNU General
; *  Public License plus this exception.  An independent module is a module
; *  which is not derived from or based on XviD.
; *
; *  Note that people who make modified versions of XviD are not obligated
; *  to grant this special exception for their modified versions; it is
; *  their choice whether to do so.  The GNU General Public License gives
; *  permission to release a modified version without this exception; this
; *  exception also makes it possible to release a modified version which
; *  carries forward this exception.
; *
; * $Id: fdct_xmm.asm,v 1.2 2003-02-15 15:22:18 edgomez Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 01.10.2002  creation - Skal -
; *
; *************************************************************************/

bits 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

cglobal xvid_fdct_sse
cglobal xvid_fdct_mmx

;//////////////////////////////////////////////////////////////////////
;
; Vertical pass is an implementation of the scheme:
;  Loeffler C., Ligtenberg A., and Moschytz C.S.:
;  Practical Fast 1D DCT Algorithm with Eleven Multiplications,
;  Proc. ICASSP 1989, 988-991.
;
; Horizontal pass is a double 4x4 vector/matrix multiplication,
; (see also Intel's Application Note 922:
;  http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
;  Copyright (C) 1999 Intel Corporation)
;
; Notes:
;  * tan(3pi/16) is greater than 0.5, and would use the
;    sign bit when turned into 16b fixed-point precision. So,
;    we use the trick: x*tan3 = x*(tan3-1)+x
; 
;  * There's only one SSE-specific instruction (pshufw).
;    Porting to SSE2 also seems straightforward.
;
;  * There's still 1 or 2 ticks to save in fLLM_PASS, but
;    I prefer having a readable code, instead of a tightly 
;    scheduled one...
;
;  * Quantization stage (as well as pre-transposition for the
;    idct way back) can be included in the fTab* constants
;    (with induced loss of precision, somehow)
;
;  * Some more details at: http://skal.planet-d.net/coding/dct.html
;
;////////////////////////////////////////////////////////////////////// 
;
;   idct-like IEEE errors:
;
;  =========================
;  Peak error:   1.0000
;  Peak MSE:     0.0365
;  Overall MSE:  0.0201
;  Peak ME:      0.0265
;  Overall ME:   0.0006
;
;  == Mean square errors ==
;   0.000 0.001 0.001 0.002 0.000 0.002 0.001 0.000    [0.001]
;   0.035 0.029 0.032 0.032 0.031 0.032 0.034 0.035    [0.032]
;   0.026 0.028 0.027 0.027 0.025 0.028 0.028 0.025    [0.027]
;   0.037 0.032 0.031 0.030 0.028 0.029 0.026 0.031    [0.030]
;   0.000 0.001 0.001 0.002 0.000 0.002 0.001 0.001    [0.001]
;   0.025 0.024 0.022 0.022 0.022 0.022 0.023 0.023    [0.023]
;   0.026 0.028 0.025 0.028 0.030 0.025 0.026 0.027    [0.027]
;   0.021 0.020 0.020 0.022 0.020 0.022 0.017 0.019    [0.020]
;  
;  == Abs Mean errors ==
;   0.000 0.000 0.000 0.000 0.000 0.000 0.000 0.000    [0.000]
;   0.020 0.001 0.003 0.003 0.000 0.004 0.002 0.003    [0.002]
;   0.000 0.001 0.001 0.001 0.001 0.004 0.000 0.000    [0.000]
;   0.027 0.001 0.000 0.002 0.002 0.002 0.001 0.000    [0.003]
;   0.000 0.000 0.000 0.000 0.000 0.001 0.000 0.001    [-0.000]
;   0.001 0.003 0.001 0.001 0.002 0.001 0.000 0.000    [-0.000]
;   0.000 0.002 0.002 0.001 0.001 0.002 0.001 0.000    [-0.000]
;   0.000 0.002 0.001 0.002 0.001 0.002 0.001 0.001    [-0.000]
;  
;//////////////////////////////////////////////////////////////////////

section .data

align 16
tan1:    dw  0x32ec,0x32ec,0x32ec,0x32ec    ; tan( pi/16)
tan2:    dw  0x6a0a,0x6a0a,0x6a0a,0x6a0a    ; tan(2pi/16)  (=sqrt(2)-1)
tan3:    dw  0xab0e,0xab0e,0xab0e,0xab0e    ; tan(3pi/16)-1
sqrt2:   dw  0x5a82,0x5a82,0x5a82,0x5a82    ; 0.5/sqrt(2)

;//////////////////////////////////////////////////////////////////////

align 16
fTab1:
  dw 0x4000, 0x4000, 0x58c5, 0x4b42,
  dw 0x4000, 0x4000, 0x3249, 0x11a8,
  dw 0x539f, 0x22a3, 0x4b42, 0xee58,
  dw 0xdd5d, 0xac61, 0xa73b, 0xcdb7,
  dw 0x4000, 0xc000, 0x3249, 0xa73b,
  dw 0xc000, 0x4000, 0x11a8, 0x4b42,
  dw 0x22a3, 0xac61, 0x11a8, 0xcdb7,
  dw 0x539f, 0xdd5d, 0x4b42, 0xa73b

fTab2:
  dw 0x58c5, 0x58c5, 0x7b21, 0x6862,
  dw 0x58c5, 0x58c5, 0x45bf, 0x187e,
  dw 0x73fc, 0x300b, 0x6862, 0xe782,
  dw 0xcff5, 0x8c04, 0x84df, 0xba41,
  dw 0x58c5, 0xa73b, 0x45bf, 0x84df,
  dw 0xa73b, 0x58c5, 0x187e, 0x6862,
  dw 0x300b, 0x8c04, 0x187e, 0xba41,
  dw 0x73fc, 0xcff5, 0x6862, 0x84df

fTab3:
  dw 0x539f, 0x539f, 0x73fc, 0x6254,
  dw 0x539f, 0x539f, 0x41b3, 0x1712,
  dw 0x6d41, 0x2d41, 0x6254, 0xe8ee,
  dw 0xd2bf, 0x92bf, 0x8c04, 0xbe4d,
  dw 0x539f, 0xac61, 0x41b3, 0x8c04,
  dw 0xac61, 0x539f, 0x1712, 0x6254,
  dw 0x2d41, 0x92bf, 0x1712, 0xbe4d,
  dw 0x6d41, 0xd2bf, 0x6254, 0x8c04

fTab4:
  dw 0x4b42, 0x4b42, 0x6862, 0x587e,
  dw 0x4b42, 0x4b42, 0x3b21, 0x14c3,
  dw 0x6254, 0x28ba, 0x587e, 0xeb3d,
  dw 0xd746, 0x9dac, 0x979e, 0xc4df,
  dw 0x4b42, 0xb4be, 0x3b21, 0x979e,
  dw 0xb4be, 0x4b42, 0x14c3, 0x587e,
  dw 0x28ba, 0x9dac, 0x14c3, 0xc4df,
  dw 0x6254, 0xd746, 0x587e, 0x979e

align 16
Fdct_Rnd0: dw  6,8,8,8
Fdct_Rnd1: dw  8,8,8,8
Fdct_Rnd2: dw 10,8,8,8
MMX_One:   dw  1,1,1,1

;//////////////////////////////////////////////////////////////////////

section .text

;//////////////////////////////////////////////////////////////////////
;// FDCT LLM vertical pass (~39c)
;//////////////////////////////////////////////////////////////////////

%macro fLLM_PASS 2  ; %1: src/dst, %2:Shift

  movq   mm0, [%1+0*16]   ; In0
  movq   mm2, [%1+2*16]   ; In2
  movq   mm3, mm0
  movq   mm4, mm2
  movq   mm7, [%1+7*16]   ; In7
  movq   mm5, [%1+5*16]   ; In5

  psubsw mm0, mm7         ; t7 = In0-In7
  paddsw mm7, mm3         ; t0 = In0+In7
  psubsw mm2, mm5         ; t5 = In2-In5
  paddsw mm5, mm4         ; t2 = In2+In5

  movq   mm3, [%1+3*16]   ; In3
  movq   mm4, [%1+4*16]   ; In4
  movq   mm1, mm3
  psubsw mm3, mm4         ; t4 = In3-In4
  paddsw mm4, mm1         ; t3 = In3+In4
  movq   mm6, [%1+6*16]   ; In6
  movq   mm1, [%1+1*16]   ; In1
  psubsw mm1, mm6         ; t6 = In1-In6
  paddsw mm6, [%1+1*16]   ; t1 = In1+In6

  psubsw mm7, mm4         ; tm03 = t0-t3
  psubsw mm6, mm5         ; tm12 = t1-t2
  paddsw mm4, mm4         ; 2.t3
  paddsw mm5, mm5         ; 2.t2
  paddsw mm4, mm7         ; tp03 = t0+t3
  paddsw mm5, mm6         ; tp12 = t1+t2

  psllw  mm2, %2+1        ; shift t5 (shift +1 to..
  psllw  mm1, %2+1        ; shift t6  ..compensate cos4/2)
  psllw  mm4, %2          ; shift t3
  psllw  mm5, %2          ; shift t2
  psllw  mm7, %2          ; shift t0
  psllw  mm6, %2          ; shift t1
  psllw  mm3, %2          ; shift t4
  psllw  mm0, %2          ; shift t7

  psubsw mm4, mm5         ; out4 = tp03-tp12
  psubsw mm1, mm2         ; mm1: t6-t5
  paddsw mm5, mm5
  paddsw mm2, mm2
  paddsw mm5, mm4         ; out0 = tp03+tp12
  movq   [%1+4*16], mm4   ; => out4
  paddsw mm2, mm1         ; mm2: t6+t5
  movq   [%1+0*16], mm5   ; => out0

  movq   mm4, [tan2]      ; mm4 <= tan2
  pmulhw mm4, mm7         ; tm03*tan2
  movq   mm5, [tan2]      ; mm5 <= tan2
  psubsw mm4, mm6         ; out6 = tm03*tan2 - tm12
  pmulhw mm5, mm6         ; tm12*tan2
  paddsw mm5, mm7         ; out2 = tm12*tan2 + tm03

  movq   mm6, [sqrt2]  
  movq   mm7, [MMX_One]

  pmulhw mm2, mm6         ; mm2: tp65 = (t6 + t5)*cos4
  por    mm5, mm7         ; correct out2
  por    mm4, mm7         ; correct out6
  pmulhw mm1, mm6         ; mm1: tm65 = (t6 - t5)*cos4
  por    mm2, mm7         ; correct tp65

  movq   [%1+2*16], mm5   ; => out2
  movq   mm5, mm3         ; save t4
  movq   [%1+6*16], mm4   ; => out6
  movq   mm4, mm0         ; save t7
  
  psubsw mm3, mm1         ; mm3: tm465 = t4 - tm65
  psubsw mm0, mm2         ; mm0: tm765 = t7 - tp65
  paddsw mm2, mm4         ; mm2: tp765 = t7 + tp65
  paddsw mm1, mm5         ; mm1: tp465 = t4 + tm65

  movq   mm4, [tan3]      ; tan3 - 1
  movq   mm5, [tan1]      ; tan1

  movq   mm7, mm3         ; save tm465
  pmulhw mm3, mm4         ; tm465*(tan3-1)
  movq   mm6, mm1         ; save tp465
  pmulhw mm1, mm5         ; tp465*tan1

  paddsw mm3, mm7         ; tm465*tan3
  pmulhw mm4, mm0         ; tm765*(tan3-1)
  paddsw mm4, mm0         ; tm765*tan3
  pmulhw mm5, mm2         ; tp765*tan1

  paddsw mm1, mm2         ; out1 = tp765 + tp465*tan1
  psubsw mm0, mm3         ; out3 = tm765 - tm465*tan3
  paddsw mm7, mm4         ; out5 = tm465 + tm765*tan3
  psubsw mm5, mm6         ; out7 =-tp465 + tp765*tan1

  movq   [%1+1*16], mm1   ; => out1
  movq   [%1+3*16], mm0   ; => out3
  movq   [%1+5*16], mm7   ; => out5
  movq   [%1+7*16], mm5   ; => out7

%endmacro

;//////////////////////////////////////////////////////////////////////
;// fMTX_MULT (~20c)
;//////////////////////////////////////////////////////////////////////

%macro fMTX_MULT 4   ; %1=src, %2 = Coeffs, %3/%4=rounders
  movq    mm0, [ecx+%1*16+0]  ; mm0 = [0123]

    ; the 'pshufw' below is the only SSE instruction.
    ; For MMX-only version, it should be emulated with
    ; some 'punpck' soup...

  pshufw  mm1, [ecx+%1*16+8], 00011011b ; mm1 = [7654]
  movq    mm7, mm0

  paddsw  mm0, mm1            ; mm0 = [a0 a1 a2 a3]
  psubsw  mm7, mm1            ; mm7 = [b0 b1 b2 b3]

  movq      mm1, mm0
  punpckldq mm0, mm7          ; mm0 = [a0 a1 b0 b1]
  punpckhdq mm1, mm7          ; mm1 = [b2 b3 a2 a3]

  movq    mm2, qword [%2+ 0]  ;  [   M00    M01      M16    M17]
  movq    mm3, qword [%2+ 8]  ;  [   M02    M03      M18    M19]
  pmaddwd mm2, mm0            ;  [a0.M00+a1.M01 | b0.M16+b1.M17]
  movq    mm4, qword [%2+16]  ;  [   M04    M05      M20    M21]
  pmaddwd mm3, mm1            ;  [a2.M02+a3.M03 | b2.M18+b3.M19]
  movq    mm5, qword [%2+24]  ;  [   M06    M07      M22    M23]
  pmaddwd mm4, mm0            ;  [a0.M04+a1.M05 | b0.M20+b1.M21]
  movq    mm6, qword [%2+32]  ;  [   M08    M09      M24    M25]
  pmaddwd mm5, mm1            ;  [a2.M06+a3.M07 | b2.M22+b3.M23]
  movq    mm7, qword [%2+40]  ;  [   M10    M11      M26    M27]
  pmaddwd mm6, mm0            ;  [a0.M08+a1.M09 | b0.M24+b1.M25]
  paddd   mm2, mm3            ;  [ out0 | out1 ]
  pmaddwd mm7, mm1            ;  [a0.M10+a1.M11 | b0.M26+b1.M27]
  psrad   mm2, 16
  pmaddwd mm0, qword [%2+48]  ;  [a0.M12+a1.M13 | b0.M28+b1.M29]
  paddd   mm4, mm5            ;  [ out2 | out3 ]
  pmaddwd mm1, qword [%2+56]  ;  [a0.M14+a1.M15 | b0.M30+b1.M31]
  psrad   mm4, 16

  paddd   mm6, mm7            ;  [ out4 | out5 ]
  psrad   mm6, 16
  paddd   mm0, mm1            ;  [ out6 | out7 ]  
  psrad   mm0, 16
  
  packssdw mm2, mm4           ;  [ out0|out1|out2|out3 ]
  paddsw  mm2, [%3]           ;  Round
  packssdw mm6, mm0           ;  [ out4|out5|out6|out7 ]
  paddsw  mm6, [%4]           ;  Round

  psraw   mm2, 4               ; => [-2048, 2047]
  psraw   mm6, 4

  movq    [ecx+%1*16+0], mm2
  movq    [ecx+%1*16+8], mm6
%endmacro

align 16
xvid_fdct_sse:    ; ~240c
  mov ecx, [esp+4]

  fLLM_PASS ecx+0, 3
  fLLM_PASS ecx+8, 3
  fMTX_MULT  0, fTab1, Fdct_Rnd0, Fdct_Rnd0
  fMTX_MULT  1, fTab2, Fdct_Rnd2, Fdct_Rnd1
  fMTX_MULT  2, fTab3, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT  3, fTab4, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT  4, fTab1, Fdct_Rnd0, Fdct_Rnd0
  fMTX_MULT  5, fTab4, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT  6, fTab3, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT  7, fTab2, Fdct_Rnd1, Fdct_Rnd1

  ret


;//////////////////////////////////////////////////////////////////////
;// fMTX_MULT_MMX (~26c)
;//////////////////////////////////////////////////////////////////////

%macro fMTX_MULT_MMX 4   ; %1=src, %2 = Coeffs, %3/%4=rounders

      ; MMX-only version (no 'pshufw'. ~10% overall slower than SSE)

  movd    mm1, [ecx+%1*16+8+4]  ; [67..]
  movq    mm0, [ecx+%1*16+0]    ; mm0 = [0123]
  movq    mm7, mm0
  punpcklwd mm1, [ecx+%1*16+8]  ; [6475]
  movq    mm2, mm1
  psrlq   mm1, 32               ; [75..]
  punpcklwd mm1,mm2             ; [7654]

  paddsw  mm0, mm1            ; mm0 = [a0 a1 a2 a3]
  psubsw  mm7, mm1            ; mm7 = [b0 b1 b2 b3]

  movq      mm1, mm0
  punpckldq mm0, mm7          ; mm0 = [a0 a1 b0 b1]
  punpckhdq mm1, mm7          ; mm1 = [b2 b3 a2 a3]

  movq    mm2, qword [%2+ 0]  ;  [   M00    M01      M16    M17]
  movq    mm3, qword [%2+ 8]  ;  [   M02    M03      M18    M19]
  pmaddwd mm2, mm0            ;  [a0.M00+a1.M01 | b0.M16+b1.M17]
  movq    mm4, qword [%2+16]  ;  [   M04    M05      M20    M21]
  pmaddwd mm3, mm1            ;  [a2.M02+a3.M03 | b2.M18+b3.M19]
  movq    mm5, qword [%2+24]  ;  [   M06    M07      M22    M23]
  pmaddwd mm4, mm0            ;  [a0.M04+a1.M05 | b0.M20+b1.M21]
  movq    mm6, qword [%2+32]  ;  [   M08    M09      M24    M25]
  pmaddwd mm5, mm1            ;  [a2.M06+a3.M07 | b2.M22+b3.M23]
  movq    mm7, qword [%2+40]  ;  [   M10    M11      M26    M27]
  pmaddwd mm6, mm0            ;  [a0.M08+a1.M09 | b0.M24+b1.M25]
  paddd   mm2, mm3            ;  [ out0 | out1 ]
  pmaddwd mm7, mm1            ;  [a0.M10+a1.M11 | b0.M26+b1.M27]
  psrad   mm2, 16
  pmaddwd mm0, qword [%2+48]  ;  [a0.M12+a1.M13 | b0.M28+b1.M29]
  paddd   mm4, mm5            ;  [ out2 | out3 ]
  pmaddwd mm1, qword [%2+56]  ;  [a0.M14+a1.M15 | b0.M30+b1.M31]
  psrad   mm4, 16

  paddd   mm6, mm7            ;  [ out4 | out5 ]
  psrad   mm6, 16
  paddd   mm0, mm1            ;  [ out6 | out7 ]  
  psrad   mm0, 16
  
  packssdw mm2, mm4           ;  [ out0|out1|out2|out3 ]
  paddsw  mm2, [%3]           ;  Round
  packssdw mm6, mm0           ;  [ out4|out5|out6|out7 ]
  paddsw  mm6, [%4]           ;  Round

  psraw   mm2, 4               ; => [-2048, 2047]
  psraw   mm6, 4

  movq    [ecx+%1*16+0], mm2
  movq    [ecx+%1*16+8], mm6
%endmacro

align 16
xvid_fdct_mmx:    ; ~269c
  mov ecx, [esp+4]

  fLLM_PASS ecx+0, 3
  fLLM_PASS ecx+8, 3
  fMTX_MULT_MMX  0, fTab1, Fdct_Rnd0, Fdct_Rnd0
  fMTX_MULT_MMX  1, fTab2, Fdct_Rnd2, Fdct_Rnd1
  fMTX_MULT_MMX  2, fTab3, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT_MMX  3, fTab4, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT_MMX  4, fTab1, Fdct_Rnd0, Fdct_Rnd0
  fMTX_MULT_MMX  5, fTab4, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT_MMX  6, fTab3, Fdct_Rnd1, Fdct_Rnd1
  fMTX_MULT_MMX  7, fTab2, Fdct_Rnd1, Fdct_Rnd1

  ret

;//////////////////////////////////////////////////////////////////////
