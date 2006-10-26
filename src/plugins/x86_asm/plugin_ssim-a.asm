;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - optimized SSIM routines -
; *
; *  Copyright(C) 2006 Johannes Reinhardt <johannes.reinhardt@gmx.de>
; *
; *  This program is free software; you can redistribute it and/or modify it
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
; *
; ***************************************************************************/

BITS 32

%macro cglobal 1
	%ifdef PREFIX
		%ifdef MARK_FUNCS
			global _%1:function %1.endfunc-%1
			%define %1 _%1:function %1.endfunc-%1
		%else
			global _%1
			%define %1 _%1
		%endif
	%else
		%ifdef MARK_FUNCS
			global %1:function %1.endfunc-%1
		%else
			global %1
		%endif
	%endif
%endmacro

%macro ACC_ROW 2
	movq %1,[    ecx]
	movq %2,[ecx+edx]
	psadbw %1,mm0
	psadbw %2,mm0
        lea ecx, [ecx+2*edx]
        paddw  %1, %2
%endmacro

	;load a dq from mem to a xmm reg
%macro LOAD_XMM 2
	movdqu %1,[%2]
	;movhps %1,[%2+8]
%endmacro

%macro WRITE_XMM 2
	;movlps [%1],%2
	;movhps [%1+8],%2
	movdqu [%1],%2
%endmacro

%macro CONSIM_1x8_SSE2 0
	LOAD_XMM xmm0,ecx
	LOAD_XMM xmm1,edx
	pxor xmm2,xmm2

	;unpack to words
	punpcklbw xmm0,xmm2
	punpcklbw xmm1,xmm2

	;devo
	psubw xmm0,xmm6
	movaps xmm2,xmm0
	pmaddwd xmm2,xmm0
	paddd xmm3,xmm2

	;devc
	psubw xmm1,xmm7
	movaps xmm2,xmm1
	pmaddwd xmm2,xmm1
	paddd xmm4,xmm2

	;corr
	pmaddwd xmm1,xmm0
	paddd xmm5,xmm1
%endmacro


%macro CONSIM_1x8_MMX 0
	movq mm0,[ecx];orig
	movq mm1,[edx];comp
	pxor mm2,mm2;null vector

	;unpack low half of qw to words
	punpcklbw mm0,mm2
	punpcklbw mm1,mm2

	;devo
	psubw mm0,mm6
	movq mm2,mm0
	pmaddwd	mm2,mm0
	paddd mm3,mm2;
	
	;devc
	psubw mm1,mm7
	movq mm2,mm1
	pmaddwd mm2,mm1
	paddd mm4,mm2

	;corr
	pmaddwd mm1,mm0
	paddd mm5,mm1

	movq mm0,[ecx]
	movq mm1,[edx]
	pxor mm2,mm2;null vector

	;unpack high half of qw to words
	punpckhbw mm0,mm2
	punpckhbw mm1,mm2

	;devo
	psubw mm0,mm6
	movq mm2,mm0
	pmaddwd	mm2,mm0
	paddd mm3,mm2;
	
	;devc
	psubw mm1,mm7
	movq mm2,mm1
	pmaddwd mm2,mm1
	paddd mm4,mm2

	;corr
	pmaddwd mm1,mm0
	paddd mm5,mm1
%endmacro



SECTION .text

cglobal lum_8x8_mmx
cglobal consim_sse2
cglobal consim_mmx

;int lum_8x8_c(uint8_t* ptr, uint32_t stride)

ALIGN 16
lum_8x8_mmx:
	mov ecx, [esp + 4] ;ptr
	mov edx, [esp + 8];stride

	pxor mm0,mm0

	ACC_ROW mm1, mm2
	
	ACC_ROW mm3, mm4

	ACC_ROW mm5, mm6

	ACC_ROW mm7, mm4

	paddw mm1, mm3
	paddw mm5, mm7
	paddw mm1, mm5

	movd eax,mm1
	ret
.endfunc

ALIGN 16
consim_mmx:
	mov ecx,[esp+4] ;ptro
	pxor mm6,mm6;

	mov edx,[esp+8] ;ptrc
	pxor mm3,mm3;devo
	pxor mm4,mm4;devc
	movd mm6,[esp + 16];lumo
	pxor mm7,mm7
	mov eax,[esp+12];stride
	movd mm7,[esp + 20];lumc
	pshufw mm6,mm6,00000000b                ; TODO: remove later! not MMX, but SSE
	pxor mm5,mm5;corr
	pshufw mm7,mm7,00000000b

	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX
	add ecx,eax
	add edx,eax
	CONSIM_1x8_MMX

	pshufw mm0,mm3,01001110b
	paddd mm3,mm0
	pshufw mm1,mm4,01001110b
	paddd mm4,mm1
	pshufw mm2,mm5,01001110b	
	paddd mm5,mm2

	;load target pointer
 	mov ecx,[esp + 24]; pdevo
	movd [ecx],mm3
 	mov edx,[esp + 28]; pdevc
	movd [edx],mm4
 	mov eax,[esp + 32]; corr
	movd [eax],mm5
	emms
	ret
.endfunc

consim_sse2:
	mov ecx,[esp+4] ;ptro
	pxor xmm6,xmm6;
	mov edx,[esp+8] ;ptrc
	pxor xmm3,xmm3;devo
	pxor xmm4,xmm4;devc
	movd xmm6,[esp + 16];lumo
	pxor xmm7,xmm7
	mov eax,[esp+12];stride
	movd xmm7,[esp + 20];lumc
	pxor xmm5,xmm5;corr

	;broadcast lumo/c
	;punpcklbw xmm6,xmm6
	punpcklwd xmm6,xmm6
	pshufd xmm6,xmm6,00000000b;or shufps
	;punpcklbw xmm7,xmm7
	punpcklwd xmm7,xmm7
	pshufd xmm7,xmm7,00000000b

	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2
	add ecx,eax
	add edx,eax
	CONSIM_1x8_SSE2

;accumulate xmm3-5
	pshufd     xmm0, xmm3, 0EH ; Get bit 64-127 from xmm1 (or use movhlps)
	paddd      xmm3, xmm0      ; Sums are in 2 dwords
	pshufd     xmm0, xmm3, 01H ; Get bit 32-63 from xmm0
	paddd      xmm3, xmm0      ; Sum is in one dword

	pshufd     xmm1, xmm4, 0EH ; Get bit 64-127 from xmm1 (or use movhlps)
	paddd      xmm4, xmm1      ; Sums are in 2 dwords
	pshufd     xmm1, xmm4, 01H ; Get bit 32-63 from xmm0
	paddd      xmm4, xmm1      ; Sum is in one dword

	pshufd     xmm2, xmm5, 0EH ; Get bit 64-127 from xmm1 (or use movhlps)
	paddd      xmm5, xmm2      ; Sums are in 2 dwords
	pshufd     xmm2, xmm5, 01H ; Get bit 32-63 from xmm0
	paddd      xmm5, xmm2      ; Sum is in one dword


	;load target pointer
 	mov ecx,[esp + 24]; pdevo
	movd [ecx],xmm3
 	mov edx,[esp + 28]; pdevc
	movd [edx],xmm4
 	mov eax,[esp + 32]; corr
	movd [eax],xmm5
	ret
.endfunc