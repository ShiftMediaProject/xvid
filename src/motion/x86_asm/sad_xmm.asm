;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	xmm sum of absolute difference
; *
; *	This program is free software; you can redistribute it and/or modify
; *	it under the terms of the GNU General Public License as published by
; *	the Free Software Foundation; either version 2 of the License, or
; *	(at your option) any later version.
; *
; *	This program is distributed in the hope that it will be useful,
; *	but WITHOUT ANY WARRANTY; without even the implied warranty of
; *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *	GNU General Public License for more details.
; *
; *	You should have received a copy of the GNU General Public License
; *	along with this program; if not, write to the Free Software
; *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 23.07.2002	sad8bi_xmm; <pross@xvid.org>
; * 04.06.2002  rewrote some funcs (XMM mainly)     -Skal-
; * 17.11.2001  bugfix and small improvement for dev16_xmm,
; *             removed terminate early in sad16_xmm (Isibaar)
; *	12.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
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
;					const uint32_t best_sad);
cglobal  sad8_xmm
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
%macro MEAN_16x16_SSE 0
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