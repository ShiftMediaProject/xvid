;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *	 mmx yuyv/uyvy to yuv planar conversion 
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
; * $Id: yuyv_to_yv12_mmx.asm,v 1.3 2002-11-17 00:20:30 edgomez Exp $
; *
; ****************************************************************************/

bits 32


section .data

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

align 16


section .data


;===========================================================================
; masks for extracting yuv components
;===========================================================================
;			y     u     y     v     y     u     y     v

mask1	db	0xff, 0,    0xff, 0,    0xff, 0,    0xff, 0
mask2	db	0,    0xff, 0,    0xff, 0,    0xff, 0,    0xff


section .text

;===========================================================================
;
;	void yuyv_to_yv12_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride);
;
;	width must be multiple of 8
;	does not flip
;	~30% faster than plain c
;
;===========================================================================

align 16
cglobal yuyv_to_yv12_mmx
yuyv_to_yv12_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; some global consants

		mov ecx, [esp + 20 + 20]	; width
		mov eax, [esp + 20 + 28]	; stride
		sub eax, ecx				; eax = stride - width
		mov edx, eax
		add edx, [esp + 20 + 28]	; edx = y_dif + stride
		push edx					; [esp + 12] = y_dif

		shr eax, 1
		push eax					; [esp + 8] = uv_dif

		shr ecx, 3
		push ecx					; [esp + 4] = width/8

		sub	esp, 4					; [esp + 0] = tmp_height_counter
						; STACK_BASE = 36
		
		movq mm6, [mask1]
		movq mm7, [mask2]

		mov edi, [esp + 36 + 4]		; y_out
		mov ebx, [esp + 36 + 8]		; u_out
		mov edx, [esp + 36 + 12]	; v_out
		mov esi, [esp + 36 + 16]	; src

		mov eax, [esp + 36 + 20]
		mov ebp, [esp + 36 + 24]
		mov ecx, [esp + 36 + 28]	; ecx = stride
		shr ebp, 1					; ebp = height /= 2
		add eax, eax				; eax = 2 * width

.yloop
		mov [esp], ebp
		mov ebp, [esp + 4]			; width/8
.xloop
		movq mm2, [esi]				; y 1st row
		movq mm3, [esi + 8]
		movq mm0, mm2
		movq mm1, mm3
		pand mm2, mm6 ; mask1
		pand mm3, mm6 ; mask1
		pand mm0, mm7 ; mask2
		pand mm1, mm7 ; mask2
		packuswb mm2, mm3
		psrlq mm0, 8
		psrlq mm1, 8
		movq [edi], mm2

		movq mm4, [esi + eax]		; y 2nd row
		movq mm5, [esi + eax + 8]
		movq mm2, mm4
		movq mm3, mm5
		pand mm4, mm6 ; mask1
		pand mm5, mm6 ; mask1
		pand mm2, mm7 ; mask2
		pand mm3, mm7 ; mask2
		packuswb mm4, mm5
		psrlq mm2, 8
		psrlq mm3, 8
		movq [edi + ecx], mm4

		paddw mm0, mm2			; uv avg 1st & 2nd
		paddw mm1, mm3
		psrlw mm0, 1
		psrlw mm1, 1
		packuswb mm0, mm1
		movq mm2, mm0
		pand mm0, mm6 ; mask1
		pand mm2, mm7 ; mask2
		packuswb mm0, mm0
		psrlq mm2, 8
		movd [ebx], mm0
		packuswb mm2, mm2
		movd [edx], mm2

		add	esi, 16
		add	edi, 8
		add	ebx, 4
		add	edx, 4
		dec ebp
		jnz near .xloop

		mov ebp, [esp]

		add esi, eax			; += width2
		add edi, [esp + 12]		; += y_dif + stride
		add ebx, [esp + 8]		; += uv_dif
		add edx, [esp + 8]		; += uv_dif

		dec ebp
		jnz near .yloop

		emms

		add esp, 16
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret


;===========================================================================
;
;	void uyvy_to_yv12_mmx(uint8_t * const y_out,
;						uint8_t * const u_out,
;						uint8_t * const v_out,
;						const uint8_t * const src,
;						const uint32_t width,
;						const uint32_t height,
;						const uint32_t stride);
;
;	width must be multiple of 8
;	does not flip
;	~30% faster than plain c
;
;===========================================================================

align 16
cglobal uyvy_to_yv12_mmx
uyvy_to_yv12_mmx

		push ebx
		push ecx
		push esi
		push edi			
		push ebp		; STACK BASE = 20

		; some global consants

		mov ecx, [esp + 20 + 20]	; width
		mov eax, [esp + 20 + 28]	; stride
		sub eax, ecx				; eax = stride - width
		mov edx, eax
		add edx, [esp + 20 + 28]	; edx = y_dif + stride
		push edx					; [esp + 12] = y_dif

		shr eax, 1
		push eax					; [esp + 8] = uv_dif

		shr ecx, 3
		push ecx					; [esp + 4] = width/8

		sub	esp, 4					; [esp + 0] = tmp_height_counter
						; STACK_BASE = 36
		
		movq mm6, [mask1]
		movq mm7, [mask2]

		mov edi, [esp + 36 + 4]		; y_out
		mov ebx, [esp + 36 + 8]		; u_out
		mov edx, [esp + 36 + 12]	; v_out
		mov esi, [esp + 36 + 16]	; src

		mov eax, [esp + 36 + 20]
		mov ebp, [esp + 36 + 24]
		mov ecx, [esp + 36 + 28]	; ecx = stride
		shr ebp, 1					; ebp = height /= 2
		add eax, eax				; eax = 2 * width

.yloop
		mov [esp], ebp
		mov ebp, [esp + 4]			; width/8
.xloop
		movq mm2, [esi]				; y 1st row
		movq mm3, [esi + 8]
		movq mm0, mm2
		movq mm1, mm3
		pand mm2, mm7 ; mask2
		pand mm3, mm7 ; mask2
		psrlq mm2, 8
		psrlq mm3, 8
		pand mm0, mm6 ; mask1
		pand mm1, mm6 ; mask1
		packuswb mm2, mm3
		movq [edi], mm2

		movq mm4, [esi + eax]		; y 2nd row
		movq mm5, [esi + eax + 8]
		movq mm2, mm4
		movq mm3, mm5
		pand mm4, mm7 ; mask2
		pand mm5, mm7 ; mask2
		psrlq mm4, 8
		psrlq mm5, 8
		pand mm2, mm6 ; mask1
		pand mm3, mm6 ; mask1
		packuswb mm4, mm5
		movq [edi + ecx], mm4

		paddw mm0, mm2			; uv avg 1st & 2nd
		paddw mm1, mm3
		psrlw mm0, 1
		psrlw mm1, 1
		packuswb mm0, mm1
		movq mm2, mm0
		pand mm0, mm6 ; mask1
		pand mm2, mm7 ; mask2
		packuswb mm0, mm0
		psrlq mm2, 8
		movd [ebx], mm0
		packuswb mm2, mm2
		movd [edx], mm2

		add	esi, 16
		add	edi, 8
		add	ebx, 4
		add	edx, 4
		dec ebp
		jnz near .xloop

		mov ebp, [esp]

		add esi, eax			; += width2
		add edi, [esp + 12]		; += y_dif + stride
		add ebx, [esp + 8]		; += uv_dif
		add edx, [esp + 8]		; += uv_dif

		dec ebp
		jnz near .yloop

		emms

		add esp, 16
		pop ebp
		pop edi
		pop esi
		pop ecx
		pop ebx

		ret
