;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  xmm (extended mmx) sum of absolute difference
; *
; *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
; *  Copyright(C) 2002 Michael Militzer <michael@xvid.org>
; *  Copyright(C) 2002 -Skal-
; *
; *  This program is an implementation of a part of one or more MPEG-4
; *  Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
; *  to use this software module in hardware or software products are
; *  advised that its use may infringe existing patents or copyrights, and
; *  any such use would be at such party's own risk.  The original
; *  developer of this software module and his/her company, and subsequent
; *  editors and their companies, will have no liability for use of this
; *  software or modifications or derivatives thereof.
; *
; *  This program is free software; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
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
