;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *	 mmx 8x8 block-based halfpel interpolation
; *
; *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
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
; * $Id: interpolate8x8_mmx.asm,v 1.11 2002-11-17 00:20:30 edgomez Exp $
; *
; ****************************************************************************/

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

;===========================================================================
; (1 - r) rounding table
;===========================================================================

rounding1_mmx
times 4 dw 1
times 4 dw 0

;===========================================================================
; (2 - r) rounding table  
;===========================================================================

rounding2_mmx
times 4 dw 2
times 4 dw 1

mmx_one
times 8 db 1

section .text

%macro  CALC_AVG 6
	punpcklbw %3, %6
	punpckhbw %4, %6

	paddusw %1, %3		; mm01 += mm23
	paddusw %2, %4
	paddusw %1, %5		; mm01 += rounding
	paddusw %2, %5
		
	psrlw %1, 1			; mm01 >>= 1
	psrlw %2, 1

%endmacro


;===========================================================================
;
; void interpolate8x8_halfpel_h_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t stride,
;						const uint32_t rounding);
;
;===========================================================================

%macro COPY_H_MMX 0
		movq mm0, [esi]
		movq mm2, [esi + 1]
		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm6	; mm01 = [src]
		punpckhbw mm1, mm6	; mm23 = [src + 1]

		CALC_AVG mm0, mm1, mm2, mm3, mm7, mm6

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, edx		; src += stride
		add edi, edx		; dst += stride
%endmacro

align 16
cglobal interpolate8x8_halfpel_h_mmx
interpolate8x8_halfpel_h_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 16]		; rounding

interpolate8x8_halfpel_h_mmx.start
		movq mm7, [rounding1_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src
		mov	edx, [esp + 8 + 12]	; stride

		pxor	mm6, mm6		; zero

		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX
		COPY_H_MMX

		pop edi
		pop esi

		ret


;===========================================================================
;
; void interpolate8x8_halfpel_v_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t stride,
;						const uint32_t rounding);
;
;===========================================================================

%macro COPY_V_MMX 0
		movq mm0, [esi]
		movq mm2, [esi + edx]
		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm6	; mm01 = [src]
		punpckhbw mm1, mm6	; mm23 = [src + 1]

		CALC_AVG mm0, mm1, mm2, mm3, mm7, mm6

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, edx		; src += stride
		add edi, edx		; dst += stride
%endmacro

align 16
cglobal interpolate8x8_halfpel_v_mmx
interpolate8x8_halfpel_v_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 16]		; rounding

interpolate8x8_halfpel_v_mmx.start
		movq mm7, [rounding1_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src
		mov	edx, [esp + 8 + 12]	; stride

		pxor	mm6, mm6		; zero

		
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX
		COPY_V_MMX

		pop edi
		pop esi

		ret


;===========================================================================
;
; void interpolate8x8_halfpel_hv_mmx(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t stride, 
;						const uint32_t rounding);
;
;
;===========================================================================

%macro COPY_HV_MMX 0
		; current row

		movq mm0, [esi]
		movq mm2, [esi + 1]

		movq mm1, mm0
		movq mm3, mm2

		punpcklbw mm0, mm6		; mm01 = [src]
		punpcklbw mm2, mm6		; mm23 = [src + 1]
		punpckhbw mm1, mm6
		punpckhbw mm3, mm6

		paddusw mm0, mm2		; mm01 += mm23
		paddusw mm1, mm3

		; next row

		movq mm4, [esi + edx]
		movq mm2, [esi + edx + 1]
		
		movq mm5, mm4
		movq mm3, mm2
		
		punpcklbw mm4, mm6		; mm45 = [src + stride]
		punpcklbw mm2, mm6		; mm23 = [src + stride + 1]
		punpckhbw mm5, mm6
		punpckhbw mm3, mm6

		paddusw mm4, mm2		; mm45 += mm23
		paddusw mm5, mm3

		; add current + next row

		paddusw mm0, mm4		; mm01 += mm45
		paddusw mm1, mm5
		paddusw mm0, mm7		; mm01 += rounding2
		paddusw mm1, mm7
		
		psrlw mm0, 2			; mm01 >>= 2
		psrlw mm1, 2

		packuswb mm0, mm1
		movq [edi], mm0			; [dst] = mm01

		add esi, edx		; src += stride
		add edi, edx		; dst += stride
%endmacro

align 16
cglobal interpolate8x8_halfpel_hv_mmx
interpolate8x8_halfpel_hv_mmx

		push	esi
		push	edi

		mov	eax, [esp + 8 + 16]		; rounding
interpolate8x8_halfpel_hv_mmx.start

		movq mm7, [rounding2_mmx + eax * 8]

		mov	edi, [esp + 8 + 4]		; dst
		mov	esi, [esp + 8 + 8]		; src

		mov eax, 8

		pxor	mm6, mm6		; zero
		
		mov edx, [esp + 8 + 12]	; stride		
		
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX
		COPY_HV_MMX

		pop edi
		pop esi

		ret
