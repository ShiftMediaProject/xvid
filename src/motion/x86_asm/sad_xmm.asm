;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  xmm (extended mmx) sum of absolute difference
; *
; *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
; *  Copyright(C) 2002 Michael Militzer <michael@xvid.org>
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
; *    - Japan
; *    - United States of America
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
; * $Id: sad_xmm.asm,v 1.5 2002-11-17 00:32:06 edgomez Exp $
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

section .data

align 16
mmx_one	times 4	dw 1

section .text

cglobal  sad16_xmm
cglobal  sad8_xmm
cglobal  sad16bi_xmm
cglobal  sad8bi_xmm
cglobal  dev16_xmm

;===========================================================================
;
; uint32_t sad16_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
;===========================================================================

%macro SAD_16x16_SSE 0
    movq mm0, [eax]
    psadbw mm0, [edx]
    movq mm1, [eax+8]
    add eax, ecx
    psadbw mm1, [edx+8]
    paddusw mm5,mm0
    add edx, ecx
    paddusw mm6,mm1
%endmacro

align 16
sad16_xmm:

    mov eax, [esp+ 4] ; Src1
    mov edx, [esp+ 8] ; Src2
    mov ecx, [esp+12] ; Stride

    pxor mm5, mm5 ; accum1
    pxor mm6, mm6 ; accum2

    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE

    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE
    SAD_16x16_SSE

    paddusw mm6,mm5
    movd eax, mm6
    ret


;===========================================================================
;
; uint32_t sad8_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;===========================================================================

%macro SAD_8x8_SSE 0
    movq mm0, [eax]
    movq mm1, [eax+ecx]

    psadbw mm0, [edx]
    psadbw mm1, [edx+ecx]
    add eax, ebx
    add edx, ebx

    paddusw mm5,mm0
    paddusw mm6,mm1
%endmacro

align 16
sad8_xmm:

    mov eax, [esp+ 4] ; Src1
    mov edx, [esp+ 8] ; Src2
    mov ecx, [esp+12] ; Stride
    push ebx
    lea ebx, [ecx+ecx]
    
    pxor mm5, mm5 ; accum1
    pxor mm6, mm6 ; accum2

    SAD_8x8_SSE
    SAD_8x8_SSE
    SAD_8x8_SSE

    movq mm0, [eax]
    movq mm1, [eax+ecx]
    psadbw mm0, [edx]
    psadbw mm1, [edx+ecx]

    pop ebx

    paddusw mm5,mm0
    paddusw mm6,mm1

    paddusw mm6,mm5
    movd eax, mm6

    ret


;===========================================================================
;
; uint32_t sad16bi_xmm(const uint8_t * const cur,
;					const uint8_t * const ref1,
;					const uint8_t * const ref2,
;					const uint32_t stride);
;
;===========================================================================

%macro SADBI_16x16_SSE 0
    movq mm0, [eax]
    movq mm1, [eax+8]

    movq mm2, [edx]
    movq mm3, [edx+8]

    pavgb mm2, [ebx]
    add edx, ecx

    pavgb mm3, [ebx+8]
    add ebx, ecx

    psadbw mm0, mm2
    add eax, ecx

    psadbw mm1, mm3
    paddusw mm5,mm0

    paddusw mm6,mm1    
%endmacro

align 16
sad16bi_xmm:
    push ebx
    mov eax, [esp+4+ 4] ; Src
    mov edx, [esp+4+ 8] ; Ref1
    mov ebx, [esp+4+12] ; Ref2
    mov ecx, [esp+4+16] ; Stride

    pxor mm5, mm5 ; accum1
    pxor mm6, mm6 ; accum2

    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE

    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE
    SADBI_16x16_SSE

    paddusw mm6,mm5
    movd eax, mm6
    pop ebx
    ret

;=========================================================================== 
; 
; uint32_t sad8bi_xmm(const uint8_t * const cur, 
; const uint8_t * const ref1, 
; const uint8_t * const ref2, 
; const uint32_t stride); 
; 
;=========================================================================== 

%macro SADBI_8x8_XMM 0 
   movq mm0, [eax] 
   movq mm1, [eax+ecx] 

   movq mm2, [edx] 
   movq mm3, [edx+ecx] 

   pavgb mm2, [ebx] 
   lea edx, [edx+2*ecx] 

   pavgb mm3, [ebx+ecx] 
   lea ebx, [ebx+2*ecx] 

   psadbw mm0, mm2 
   lea eax, [eax+2*ecx] 

   psadbw mm1, mm3 
   paddusw mm5,mm0 

   paddusw mm6,mm1 
%endmacro 

align 16 
sad8bi_xmm: 
   push ebx 
   mov eax, [esp+4+ 4] ; Src 
   mov edx, [esp+4+ 8] ; Ref1 
   mov ebx, [esp+4+12] ; Ref2 
   mov ecx, [esp+4+16] ; Stride 

   pxor mm5, mm5 ; accum1 
   pxor mm6, mm6 ; accum2 
.Loop 
   SADBI_8x8_XMM 
   SADBI_8x8_XMM 
   SADBI_8x8_XMM 
   SADBI_8x8_XMM 

   paddusw mm6,mm5 
   movd eax, mm6 
   pop ebx 
   ret 


;===========================================================================
;
; uint32_t dev16_xmm(const uint8_t * const cur,
;					const uint32_t stride);
;
;===========================================================================

%macro MEAN_16x16_SSE 0
    movq mm0, [eax]
    movq mm1, [eax+8]
    psadbw mm0, mm7
    psadbw mm1, mm7
    add eax, ecx
    paddw mm5, mm0 
    paddw mm6, mm1
%endmacro
                            
%macro ABS_16x16_SSE 0
    movq mm0, [eax]
    movq mm1, [eax+8]
    psadbw mm0, mm4
    psadbw mm1, mm4
    lea eax,[eax+ecx]
    paddw mm5, mm0
    paddw mm6, mm1
%endmacro

align 16
dev16_xmm:

    mov eax, [esp+ 4] ; Src
    mov ecx, [esp+ 8] ; Stride
    
    pxor mm7, mm7 ; zero
    pxor mm5, mm5 ; mean accums
    pxor mm6, mm6

    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE

    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE
    MEAN_16x16_SSE

    paddusw mm6, mm5

	movq mm4, mm6
	psllq mm4, 32
	paddd mm4, mm6
	psrld mm4, 8      ; /= (16*16)

	packssdw mm4, mm4
	packuswb mm4, mm4

    ; mm4 contains the mean

    mov eax, [esp+ 4] ; Src

    pxor mm5, mm5 ; sums
    pxor mm6, mm6

    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE

    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE
    ABS_16x16_SSE

    paddusw mm6, mm5
	movq mm7, mm6
	psllq mm7, 32 
	paddd mm6, mm7

    movd eax, mm6
    ret
