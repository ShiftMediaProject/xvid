;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - Quarter-pixel interpolation -
; *  Copyright(C) 2002 Pascal Massimino <skal@planet-d.net>
; *		  2004 Andre Werthmann <wertmann@aei.mpg.de>
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
; * $Id: qpel_mmx.asm,v 1.4 2008-11-11 20:46:24 Isibaar Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 22.10.2002  initial coding. unoptimized 'proof of concept',
; *             just to heft the qpel filtering. - Skal -
; *
; *************************************************************************/


%define USE_TABLES      ; in order to use xvid_FIR_x_x_x_x tables
                        ; instead of xvid_Expand_mmx...


BITS 64


%macro cglobal 1
	%ifdef PREFIX
		%ifdef MARK_FUNCS
			global _%1:function %1.endfunc-%1
			%define %1 _%1:function %1.endfunc-%1
			%define ENDFUNC .endfunc
		%else
			global _%1
			%define %1 _%1
			%define ENDFUNC
		%endif
	%else
		%ifdef MARK_FUNCS
			global %1:function %1.endfunc-%1
			%define ENDFUNC .endfunc
		%else
			global %1
			%define ENDFUNC
		%endif
	%endif
%endmacro

;//////////////////////////////////////////////////////////////////////
;// Declarations
;//   all signatures are:
;// void XXX(uint8_t *dst, const uint8_t *src,
;//          int32_t length, int32_t stride, int32_t rounding)
;//////////////////////////////////////////////////////////////////////

cglobal xvid_H_Pass_16_x86_64
cglobal xvid_H_Pass_Avrg_16_x86_64
cglobal xvid_H_Pass_Avrg_Up_16_x86_64
cglobal xvid_V_Pass_16_x86_64
cglobal xvid_V_Pass_Avrg_16_x86_64
cglobal xvid_V_Pass_Avrg_Up_16_x86_64
cglobal xvid_H_Pass_8_x86_64
cglobal xvid_H_Pass_Avrg_8_x86_64
cglobal xvid_H_Pass_Avrg_Up_8_x86_64
cglobal xvid_V_Pass_8_x86_64
cglobal xvid_V_Pass_Avrg_8_x86_64
cglobal xvid_V_Pass_Avrg_Up_8_x86_64

cglobal xvid_H_Pass_Add_16_x86_64
cglobal xvid_H_Pass_Avrg_Add_16_x86_64
cglobal xvid_H_Pass_Avrg_Up_Add_16_x86_64
cglobal xvid_V_Pass_Add_16_x86_64
cglobal xvid_V_Pass_Avrg_Add_16_x86_64
cglobal xvid_V_Pass_Avrg_Up_Add_16_x86_64
cglobal xvid_H_Pass_8_Add_x86_64
cglobal xvid_H_Pass_Avrg_8_Add_x86_64
cglobal xvid_H_Pass_Avrg_Up_8_Add_x86_64
cglobal xvid_V_Pass_8_Add_x86_64
cglobal xvid_V_Pass_Avrg_8_Add_x86_64
cglobal xvid_V_Pass_Avrg_Up_8_Add_x86_64

cglobal xvid_Expand_mmx

;%ifdef USE_TABLES

cglobal xvid_FIR_1_0_0_0
cglobal xvid_FIR_3_1_0_0
cglobal xvid_FIR_6_3_1_0
cglobal xvid_FIR_14_3_2_1
cglobal xvid_FIR_20_6_3_1
cglobal xvid_FIR_20_20_6_3
cglobal xvid_FIR_23_19_6_3
cglobal xvid_FIR_7_20_20_6
cglobal xvid_FIR_6_20_20_6
cglobal xvid_FIR_6_20_20_7
cglobal xvid_FIR_3_6_20_20
cglobal xvid_FIR_3_6_19_23
cglobal xvid_FIR_1_3_6_20
cglobal xvid_FIR_1_2_3_14
cglobal xvid_FIR_0_1_3_6
cglobal xvid_FIR_0_0_1_3
cglobal xvid_FIR_0_0_0_1

;%endif

SECTION .data align=32
xvid_Expand_mmx:
times 256*4 dw 0        ; uint16_t xvid_Expand_mmx[256][4]
ENDFUNC

xvid_FIR_1_0_0_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_1_0_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_3_1_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_14_3_2_1:
times 256*4 dw 0
ENDFUNC

xvid_FIR_20_6_3_1:
times 256*4 dw 0
ENDFUNC

xvid_FIR_20_20_6_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_23_19_6_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_7_20_20_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_20_20_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_20_20_7:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_6_20_20:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_6_19_23:
times 256*4 dw 0
ENDFUNC

xvid_FIR_1_3_6_20:
times 256*4 dw 0
ENDFUNC

xvid_FIR_1_2_3_14:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_1_3_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_0_1_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_0_0_1:
times 256*4 dw 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

align 16
Rounder1_MMX:
times 4 dw 1
Rounder0_MMX:
times 4 dw 0

align 16
Rounder_QP_MMX:
times 4 dw 16
times 4 dw 15

%ifndef USE_TABLES

align 16

  ; H-Pass table shared by 16x? and 8x? filters

FIR_R0:  dw 14, -3,  2, -1
align 16
FIR_R1:  dw 23, 19, -6,  3,   -1,  0,  0,  0

FIR_R2:  dw -7, 20, 20, -6,    3, -1,  0,  0

FIR_R3:  dw  3, -6, 20, 20,   -6,  3, -1,  0

FIR_R4:  dw -1,  3, -6, 20,   20, -6,  3, -1

FIR_R5:  dw  0, -1,  3, -6,   20, 20, -6,  3,   -1,  0,  0,  0
align 16
FIR_R6:  dw  0,  0, -1,  3,   -6, 20, 20, -6,    3, -1,  0,  0
align 16
FIR_R7:  dw  0,  0,  0, -1,    3, -6, 20, 20,   -6,  3, -1,  0
align 16
FIR_R8:  dw                   -1,  3, -6, 20,   20, -6,  3, -1

FIR_R9:  dw                    0, -1,  3, -6,   20, 20, -6,  3,   -1,  0,  0,  0
align 16
FIR_R10: dw                    0,  0, -1,  3,   -6, 20, 20, -6,    3, -1,  0,  0
align 16
FIR_R11: dw                    0,  0,  0, -1,    3, -6, 20, 20,   -6,  3, -1,  0
align 16
FIR_R12: dw                                     -1,  3, -6, 20,   20, -6,  3, -1

FIR_R13: dw                                      0, -1,  3, -6,   20, 20, -6,  3

FIR_R14: dw                                      0,  0, -1,  3,   -6, 20, 20, -7

FIR_R15: dw                                      0,  0,  0, -1,    3, -6, 19, 23

FIR_R16: dw                                                       -1,  2, -3, 14

%endif  ; !USE_TABLES

  ; V-Pass taps

align 16
FIR_Cm7: times 4 dw -7
FIR_Cm6: times 4 dw -6
FIR_Cm3: times 4 dw -3
FIR_Cm1: times 4 dw -1
FIR_C2:  times 4 dw  2
FIR_C3:  times 4 dw  3
FIR_C14: times 4 dw 14
FIR_C19: times 4 dw 19
FIR_C20: times 4 dw 20
FIR_C23: times 4 dw 23

SECTION .text align=16

;//////////////////////////////////////////////////////////////////////
;// Here we go with the Q-Pel mess.
;//  For horizontal passes, we process 4 *output* pixel in parallel
;//  For vertical ones, we process 4 *input* pixel in parallel.
;//////////////////////////////////////////////////////////////////////

%macro PROLOG_NO_AVRG 0
  push rbp
  mov rax, r8			; Rnd
  and rax, 1
  mov rbp, rcx			; BpS
  mov rcx, rdx			; Size
  		; rsi is Src
		; rdi is Dst
  lea r9, [Rounder_QP_MMX wrt rip]
  movq mm7, [r9+rax*8]
%endmacro

%macro EPILOG_NO_AVRG 0
  pop rbp
  ret
%endmacro

%macro PROLOG_AVRG 0
  push rbx
  push rbp
  mov rax, r8			; Rnd
  and rax, 1
  mov rbp, rcx			; BpS
  mov rcx, rdx			; Size
  		; rsi is Src
		; rdi is Dst
 
  lea r9, [Rounder_QP_MMX wrt rip]
  movq mm7, [r9+rax*8]
  lea r9, [Rounder1_MMX wrt rip]
  lea rbx, [r9+rax*8]
%endmacro

%macro EPILOG_AVRG 0
  pop rbp
  pop rbx
  ret
%endmacro

;//////////////////////////////////////////////////////////////////////
;//
;// All horizontal passes
;//
;//////////////////////////////////////////////////////////////////////

  ; macros for USE_TABLES

%macro TLOAD 2     ; %1,%2: src pixels
  movzx rax, byte [rsi+%1]
  movzx rdx, byte [rsi+%2]
  lea r9, [xvid_FIR_14_3_2_1 wrt rip]
  movq mm0, [r9 + rax*8 ]
  lea r9, [xvid_FIR_1_2_3_14 wrt rip]
  movq mm3, [r9 + rdx*8 ]
  paddw mm0, mm7
  paddw mm3, mm7
%endmacro

%macro TACCUM2 5   ;%1:src pixel/%2-%3:Taps tables/ %4-%5:dst regs
  movzx rax, byte [rsi+%1]
  lea r9, [%2 wrt rip]
  paddw %4, [r9 + rax*8]
  lea r9, [%3 wrt rip]
  paddw %5, [r9 + rax*8]
%endmacro

%macro TACCUM3 7   ;%1:src pixel/%2-%4:Taps tables/%5-%7:dst regs
  movzx rax, byte [rsi+%1]
  lea r9, [%2 wrt rip]
  paddw %5, [r9 + rax*8]
  lea r9, [%3 wrt rip]
  paddw %6, [r9 + rax*8]
  lea r9, [%4 wrt rip]
  paddw %7, [r9 + rax*8]
%endmacro

;//////////////////////////////////////////////////////////////////////

  ; macros without USE_TABLES

%macro LOAD 2     ; %1,%2: src pixels
  movzx rax, byte [rsi+%1]
  movzx rdx, byte [rsi+%2]
  lea r9, [xvid_Expand_mmx wrt rip]
  movq mm0, [r9 + rax*8]
  movq mm3, [r9 + rdx*8]
  pmullw mm0, [FIR_R0 wrt rip]
  pmullw mm3, [FIR_R16 wrt rip]
  paddw mm0, mm7
  paddw mm3, mm7
%endmacro

%macro ACCUM2 4   ;src pixel/Taps/dst regs #1-#2
  movzx rax, byte [rsi+%1]
  lea r9, [xvid_Expand_mmx wrt rip]
  movq mm4, [r9 + rax*8]
  movq mm5, mm4
  lea r9, [%2 wrt rip]	; data
  pmullw mm4, [r9]
  pmullw mm5, [r9+8]
  paddw %3, mm4
  paddw %4, mm5
%endmacro

%macro ACCUM3 5   ;src pixel/Taps/dst regs #1-#2-#3
  movzx rax, byte [rsi+%1]
  lea r9, [xvid_Expand_mmx wrt rip]
  movq mm4, [r9 + rax*8]
  movq mm5, mm4
  movq mm6, mm5
  lea r9, [%2 wrt rip]	; data
  pmullw mm4, [r9   ]
  pmullw mm5, [r9+ 8]
  pmullw mm6, [r9+16]
  paddw %3, mm4
  paddw %4, mm5
  paddw %5, mm6
%endmacro

;//////////////////////////////////////////////////////////////////////

%macro MIX 3   ; %1:reg, %2:src, %3:rounder
  pxor mm6, mm6
  movq mm4, [%2]
  movq mm1, %1
  movq mm5, mm4
  punpcklbw %1, mm6
  punpcklbw mm4, mm6
  punpckhbw mm1, mm6
  punpckhbw mm5, mm6
  movq mm6, [%3]   ; rounder #2
  paddusw %1, mm4
  paddusw mm1, mm5
  paddusw %1, mm6
  paddusw mm1, mm6
  psrlw %1, 1
  psrlw mm1, 1
  packuswb %1, mm1
%endmacro

;//////////////////////////////////////////////////////////////////////

%macro H_PASS_16  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
   PROLOG_NO_AVRG
%else
   PROLOG_AVRG
%endif

.Loop:

    ;  mm0..mm3 serves as a 4x4 delay line

%ifndef USE_TABLES

  LOAD 0, 16  ; special case for 1rst/last pixel
  movq mm1, mm7
  movq mm2, mm7

  ACCUM2 1,    FIR_R1, mm0, mm1
  ACCUM2 2,    FIR_R2, mm0, mm1
  ACCUM2 3,    FIR_R3, mm0, mm1
  ACCUM2 4,    FIR_R4, mm0, mm1

  ACCUM3 5,    FIR_R5, mm0, mm1, mm2
  ACCUM3 6,    FIR_R6, mm0, mm1, mm2
  ACCUM3 7,    FIR_R7, mm0, mm1, mm2
  ACCUM2 8,    FIR_R8, mm1, mm2
  ACCUM3 9,    FIR_R9, mm1, mm2, mm3
  ACCUM3 10,   FIR_R10,mm1, mm2, mm3
  ACCUM3 11,   FIR_R11,mm1, mm2, mm3

  ACCUM2 12,   FIR_R12, mm2, mm3
  ACCUM2 13,   FIR_R13, mm2, mm3
  ACCUM2 14,   FIR_R14, mm2, mm3
  ACCUM2 15,   FIR_R15, mm2, mm3

%else

  TLOAD 0, 16  ; special case for 1rst/last pixel
  movq mm1, mm7
  movq mm2, mm7

  TACCUM2 1,    xvid_FIR_23_19_6_3, xvid_FIR_1_0_0_0 , mm0, mm1
  TACCUM2 2,    xvid_FIR_7_20_20_6, xvid_FIR_3_1_0_0 , mm0, mm1
  TACCUM2 3,    xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0 , mm0, mm1
  TACCUM2 4,    xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1, mm0, mm1

  TACCUM3 5,    xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, xvid_FIR_1_0_0_0  , mm0, mm1, mm2
  TACCUM3 6,    xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_6, xvid_FIR_3_1_0_0  , mm0, mm1, mm2
  TACCUM3 7,    xvid_FIR_0_0_0_1  , xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0  , mm0, mm1, mm2

  TACCUM2 8,    xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1 ,      mm1, mm2

  TACCUM3 9,    xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, xvid_FIR_1_0_0_0,  mm1, mm2, mm3
  TACCUM3 10,   xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_6, xvid_FIR_3_1_0_0,  mm1, mm2, mm3
  TACCUM3 11,   xvid_FIR_0_0_0_1  , xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0,  mm1, mm2, mm3

  TACCUM2 12,  xvid_FIR_1_3_6_20, xvid_FIR_20_6_3_1 , mm2, mm3
  TACCUM2 13,  xvid_FIR_0_1_3_6 , xvid_FIR_20_20_6_3, mm2, mm3
  TACCUM2 14,  xvid_FIR_0_0_1_3 , xvid_FIR_6_20_20_7, mm2, mm3
  TACCUM2 15,  xvid_FIR_0_0_0_1 , xvid_FIR_3_6_19_23, mm2, mm3

%endif

  psraw mm0, 5
  psraw mm1, 5
  psraw mm2, 5
  psraw mm3, 5
  packuswb mm0, mm1
  packuswb mm2, mm3

%if (%1==1)
  MIX mm0, rsi, rbx
%elif (%1==2)
  MIX mm0, rsi+1, rbx
%endif
%if (%2==1)
  lea r9, [Rounder1_MMX wrt rip]
  MIX mm0, rdi, r9
%endif

%if (%1==1)
  MIX mm2, rsi+8, rbx
%elif (%1==2)
  MIX mm2, rsi+9, rbx
%endif
%if (%2==1)
  lea r9, [Rounder1_MMX wrt rip]
  MIX mm2, rdi+8, r9
%endif

  lea rsi, [rsi+rbp]

  movq [rdi+0], mm0
  movq [rdi+8], mm2

  add rdi, rbp
  dec rcx
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro


;//////////////////////////////////////////////////////////////////////

%macro H_PASS_8  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

.Loop:
    ;  mm0..mm3 serves as a 4x4 delay line

%ifndef USE_TABLES

  LOAD 0, 8  ; special case for 1rst/last pixel
  ACCUM2 1,  FIR_R1,  mm0, mm3
  ACCUM2 2,  FIR_R2,  mm0, mm3
  ACCUM2 3,  FIR_R3,  mm0, mm3
  ACCUM2 4,  FIR_R4,  mm0, mm3

  ACCUM2 5,  FIR_R13,  mm0, mm3
  ACCUM2 6,  FIR_R14,  mm0, mm3
  ACCUM2 7,  FIR_R15,  mm0, mm3

%else

%if 1   ; test with no unrolling

  TLOAD 0, 8  ; special case for 1rst/last pixel
  TACCUM2 1,  xvid_FIR_23_19_6_3, xvid_FIR_1_0_0_0  , mm0, mm3
  TACCUM2 2,  xvid_FIR_7_20_20_6, xvid_FIR_3_1_0_0  , mm0, mm3
  TACCUM2 3,  xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0  , mm0, mm3
  TACCUM2 4,  xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1 , mm0, mm3
  TACCUM2 5,  xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, mm0, mm3
  TACCUM2 6,  xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_7, mm0, mm3
  TACCUM2 7,  xvid_FIR_0_0_0_1  , xvid_FIR_3_6_19_23, mm0, mm3

%else  ; test with unrolling (little faster, but not much)

  movzx rax, byte [rsi]
  movzx rdx, byte [rsi+8]
  movq mm0, [xvid_FIR_14_3_2_1 + rax*8 ]
  movzx rax, byte [rsi+1]
  movq mm3, [xvid_FIR_1_2_3_14 + rdx*8 ]
  paddw mm0, mm7
  paddw mm3, mm7

  movzx rdx, byte [rsi+2]
  lea r9, [xvid_FIR_23_19_6_3 wrt rip]
  paddw mm0, [r9 + rax*8]
  lea r9, [xvid_FIR_1_0_0_0 wrt rip]
  paddw mm3, [r9 + rax*8]

  movzx rax, byte [rsi+3]
  lea r9, [xvid_FIR_7_20_20_6 wrt rip]
  paddw mm0, [r9 + rdx*8]
  lea r9, [xvid_FIR_3_1_0_0 wrt rip]
  paddw mm3, [r9 + rdx*8]

  movzx rdx, byte [rsi+4]
  lea r9, [xvid_FIR_3_6_20_20 wrt rip]
  paddw mm0, [r9 + rax*8]
  lea r9, [xvid_FIR_6_3_1_0 wrt rip]
  paddw mm3, [r9 + rax*8]

  movzx rax, byte [rsi+5]
  lea r9, [xvid_FIR_1_3_6_20 wrt rip]
  paddw mm0, [r9 + rdx*8]
  lea r9, [xvid_FIR_20_6_3_1 wrt rip]
  paddw mm3, [r9 + rdx*8]

  movzx rdx, byte [rsi+6]
  lea r9, [xvid_FIR_0_1_3_6 wrt rip]
  paddw mm0, [r9 + rax*8]
  lea r9, [xvid_FIR_20_20_6_3 wrt rip]
  paddw mm3, [r9 + rax*8]

  movzx rax, byte [rsi+7]
  lea r9, [xvid_FIR_0_0_1_3 wrt rip]
  paddw mm0, [r9 + rdx*8]
  lea r9, [xvid_FIR_6_20_20_7 wrt rip]
  paddw mm3, [r9 + rdx*8]

  lea r9, [xvid_FIR_0_0_0_1 wrt rip]
  paddw mm0, [r9 + rax*8]
  lea r9, [xvid_FIR_3_6_19_23 wrt rip]
  paddw mm3, [r9 + rax*8]

%endif

%endif    ; !USE_TABLES

  psraw mm0, 5
  psraw mm3, 5
  packuswb mm0, mm3

%if (%1==1)
  MIX mm0, rsi, rbx
%elif (%1==2)
  MIX mm0, rsi+1, rbx
%endif
%if (%2==1)
  lea r9, [Rounder1_MMX wrt rip]
  MIX mm0, rdi, r9
%endif

  movq [rdi], mm0

  add rdi, rbp
  add rsi, rbp
  dec rcx
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////
;// 16x? copy Functions

xvid_H_Pass_16_x86_64:
  H_PASS_16 0, 0
ENDFUNC
xvid_H_Pass_Avrg_16_x86_64:
  H_PASS_16 1, 0
ENDFUNC
xvid_H_Pass_Avrg_Up_16_x86_64:
  H_PASS_16 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? copy Functions

xvid_H_Pass_8_x86_64:
  H_PASS_8 0, 0
ENDFUNC
xvid_H_Pass_Avrg_8_x86_64:
  H_PASS_8 1, 0
ENDFUNC
xvid_H_Pass_Avrg_Up_8_x86_64:
  H_PASS_8 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 16x? avrg Functions

xvid_H_Pass_Add_16_x86_64:
  H_PASS_16 0, 1
ENDFUNC
xvid_H_Pass_Avrg_Add_16_x86_64:
  H_PASS_16 1, 1
ENDFUNC
xvid_H_Pass_Avrg_Up_Add_16_x86_64:
  H_PASS_16 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? avrg Functions

xvid_H_Pass_8_Add_x86_64:
  H_PASS_8 0, 1
ENDFUNC
xvid_H_Pass_Avrg_8_Add_x86_64:
  H_PASS_8 1, 1
ENDFUNC
xvid_H_Pass_Avrg_Up_8_Add_x86_64:
  H_PASS_8 2, 1
ENDFUNC


;//////////////////////////////////////////////////////////////////////
;//
;// All vertical passes
;//
;//////////////////////////////////////////////////////////////////////

%macro V_LOAD 1  ; %1=Last?

  movd mm4, [rdx]
  pxor mm6, mm6
%if (%1==0)
  add rdx, rbp
%endif
  punpcklbw mm4, mm6

%endmacro

%macro V_ACC1 2   ; %1:reg; 2:tap
  pmullw mm4, [%2]
  paddw %1, mm4
%endmacro

%macro V_ACC2 4   ; %1-%2: regs, %3-%4: taps
  movq mm5, mm4
  movq mm6, mm4
  pmullw mm5, [%3]
  pmullw mm6, [%4]
  paddw %1, mm5
  paddw %2, mm6
%endmacro

%macro V_ACC2l 4   ; %1-%2: regs, %3-%4: taps
  movq mm5, mm4
  pmullw mm5, [%3]
  pmullw mm4, [%4]
  paddw %1, mm5
  paddw %2, mm4
%endmacro

%macro V_ACC4 8   ; %1-%4: regs, %5-%8: taps
  V_ACC2 %1,%2, %5,%6
  V_ACC2l %3,%4, %7,%8
%endmacro


%macro V_MIX 3  ; %1:dst-reg, %2:src, %3: rounder
  pxor mm6, mm6
  movq mm4, [%2]
  punpcklbw %1, mm6
  punpcklbw mm4, mm6
  paddusw %1, mm4
  paddusw %1, [%3]
  psrlw %1, 1
  packuswb %1, %1
%endmacro

%macro V_STORE 4    ; %1-%2: mix ops, %3: reg, %4:last?

  psraw %3, 5
  packuswb %3, %3

%if (%1==1)
  V_MIX %3, rsi, rbx
  add rsi, rbp
%elif (%1==2)
  add rsi, rbp
  V_MIX %3, rsi, rbx
%endif
%if (%2==1)
  lea r9, [Rounder1_MMX wrt rip]
  V_MIX %3, rdi, r9
%endif

  movd eax, %3
  mov [rdi], eax

%if (%4==0)
  add rdi, rbp
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////

%macro V_PASS_16  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

    ; we process one stripe of 4x16 pixel each time.
    ; the size (3rd argument) is meant to be a multiple of 4
    ;  mm0..mm3 serves as a 4x4 delay line

.Loop:

  push rdi
  push rsi      ; esi is preserved for src-mixing
  mov rdx, rsi	; rsi is Src

    ; ouput rows [0..3], from input rows [0..8]

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0	; add rdx, rbp, rbp is Bps
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14 wrt rip, FIR_Cm3 wrt rip, FIR_C2 wrt rip,  FIR_Cm1 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23 wrt rip, FIR_C19 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1 wrt rip
  V_STORE %1, %2, mm3, 0

    ; ouput rows [4..7], from input rows [1..11] (!!)

;  mov esi, [esp]
  mov rsi, [rsp]	; rsi on stack...
  lea rdx, [rsi+rbp]

  lea rsi, [rsi+4*rbp]  ; for src-mixing
  push rsi              ; this will be the new value for next round

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm0, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2l mm0, mm1, FIR_C3 wrt rip,  FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2 mm0, mm1, FIR_Cm6 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm2, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip, FIR_Cm1 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1 wrt rip
  V_STORE %1, %2, mm3, 0

    ; ouput rows [8..11], from input rows [5..15]

  pop rsi
  lea rdx, [rsi+rbp]

  lea rsi, [rsi+4*rbp]  ; for src-mixing
  push rsi              ; this will be the new value for next round

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm0, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2l mm0, mm1, FIR_C3 wrt rip,  FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2 mm0, mm1, FIR_Cm6 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm2, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip, FIR_Cm1 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip

  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1 wrt rip
  V_STORE %1, %2, mm3, 0


    ; ouput rows [12..15], from input rows [9.16]

  pop rsi
  lea rdx, [rsi+rbp]

%if (%1!=0)
  lea rsi, [rsi+4*rbp]  ; for src-mixing
%endif

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm3, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip,  FIR_C3 wrt rip

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23 wrt rip, FIR_C19 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 1
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14 wrt rip, FIR_Cm3 wrt rip, FIR_C2 wrt rip, FIR_Cm1 wrt rip

  V_STORE %1, %2, mm3, 0
  V_STORE %1, %2, mm2, 0
  V_STORE %1, %2, mm1, 0
  V_STORE %1, %2, mm0, 1

    ; ... next 4 columns

  pop rsi
  pop rdi
  add rsi, 4
  add rdi, 4
  sub rcx, 4
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////

%macro V_PASS_8  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

    ; we process one stripe of 4x8 pixel each time
    ; the size (3rd argument) is meant to be a multiple of 4
    ;  mm0..mm3 serves as a 4x4 delay line
.Loop:

  push rdi
  push rsi      ; esi is preserved for src-mixing
  mov rdx, rsi

    ; ouput rows [0..3], from input rows [0..8]

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14 wrt rip, FIR_Cm3 wrt rip, FIR_C2 wrt rip,  FIR_Cm1 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23 wrt rip, FIR_C19 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip

  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1 wrt rip
  V_STORE %1, %2, mm3, 0

    ; ouput rows [4..7], from input rows [1..9]

  mov rsi, [rsp]	; rsi on stack...
  lea rdx, [rsi+rbp]

%if (%1!=0)
  lea rsi, [rsi+4*rbp]  ; for src-mixing
%endif

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm3, FIR_Cm1 wrt rip

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1 wrt rip,  FIR_C3 wrt rip

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1 wrt rip,  FIR_C3 wrt rip
  V_ACC1 mm3, FIR_Cm6 wrt rip

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1 wrt rip, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3 wrt rip,  FIR_Cm6 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7 wrt rip, FIR_C20 wrt rip, FIR_C20 wrt rip, FIR_Cm6 wrt rip
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23 wrt rip, FIR_C19 wrt rip, FIR_Cm6 wrt rip, FIR_C3 wrt rip
  V_LOAD 1
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14 wrt rip, FIR_Cm3 wrt rip, FIR_C2 wrt rip, FIR_Cm1 wrt rip

  V_STORE %1, %2, mm3, 0
  V_STORE %1, %2, mm2, 0
  V_STORE %1, %2, mm1, 0
  V_STORE %1, %2, mm0, 1

    ; ... next 4 columns

  pop rsi
  pop rdi
  add rsi, 4
  add rdi, 4
  sub rcx, 4
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro


;//////////////////////////////////////////////////////////////////////
;// 16x? copy Functions

xvid_V_Pass_16_x86_64:
  V_PASS_16 0, 0
ENDFUNC
xvid_V_Pass_Avrg_16_x86_64:
  V_PASS_16 1, 0
ENDFUNC
xvid_V_Pass_Avrg_Up_16_x86_64:
  V_PASS_16 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? copy Functions

xvid_V_Pass_8_x86_64:
  V_PASS_8 0, 0
ENDFUNC
xvid_V_Pass_Avrg_8_x86_64:
  V_PASS_8 1, 0
ENDFUNC
xvid_V_Pass_Avrg_Up_8_x86_64:
  V_PASS_8 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 16x? avrg Functions

xvid_V_Pass_Add_16_x86_64:
  V_PASS_16 0, 1
ENDFUNC
xvid_V_Pass_Avrg_Add_16_x86_64:
  V_PASS_16 1, 1
ENDFUNC
xvid_V_Pass_Avrg_Up_Add_16_x86_64:
  V_PASS_16 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? avrg Functions

xvid_V_Pass_8_Add_x86_64:
  V_PASS_8 0, 1
ENDFUNC
xvid_V_Pass_Avrg_8_Add_x86_64:
  V_PASS_8 1, 1
ENDFUNC
xvid_V_Pass_Avrg_Up_8_Add_x86_64:
  V_PASS_8 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

