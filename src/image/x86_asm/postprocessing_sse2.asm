;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - sse2 post processing -
; *
; *  Copyright(C) 2004 Peter Ross <pross@xvid.org>
; *               2004 Dcoder <dcoder@alexandria.cc>
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
; *************************************************************************/

BITS 32

%macro cglobal 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%macro FILLBYTES 2
	
	mov [%1], %2
	mov [%1 + 1], %2
	mov [%1 + 2], %2
	mov [%1 + 3], %2
	mov [%1 + 4], %2
	mov [%1 + 5], %2
	mov [%1 + 6], %2
	mov [%1 + 7], %2
	mov [%1 + 8], %2
	mov [%1 + 9], %2
	mov [%1 + 10], %2
	mov [%1 + 11], %2
	mov [%1 + 12], %2
	mov [%1 + 13], %2
	mov [%1 + 14], %2
	mov [%1 + 15], %2

%endmacro


;===========================================================================
; read only data
;===========================================================================

%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata data align=16
%endif

xmm_0x80:
	times 16 db 0x80

offset_xmm:
	times 16 db 0x00

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal image_brightness_sse2


;//////////////////////////////////////////////////////////////////////
;// image_brightness_sse2
;//////////////////////////////////////////////////////////////////////

align 16
image_brightness_sse2:

	push esi
	push edi

	movdqa xmm6, [xmm_0x80]

	mov eax, [esp+8+20] ; offset	
	
	FILLBYTES offset_xmm, al

	movdqa xmm7, [offset_xmm]	

	mov edx, [esp+8+4]  ; Dst
	mov ecx, [esp+8+8]  ; stride
	mov esi, [esp+8+12] ; width
	mov edi, [esp+8+16] ; height

.yloop
	xor	eax, eax

.xloop
	movdqa xmm0, [edx + eax]
	movdqa xmm1, [edx + eax + 16]	; xmm0 = [dst]

	paddb xmm0, xmm6				; unsigned -> signed domain
	paddb xmm1, xmm6
	paddsb xmm0, xmm7
	paddsb xmm1, xmm7				; xmm0 += offset
	psubb xmm0, xmm6
	psubb xmm1, xmm6				; signed -> unsigned domain

	movdqa [edx + eax], xmm0
	movdqa [edx + eax + 16], xmm1	; [dst] = xmm0

	add	eax,32
	cmp	eax,esi	
	jl .xloop

	add edx, ecx				; dst += stride
	sub edi, 1
	jg .yloop

	pop edi
	pop esi

	ret
;//////////////////////////////////////////////////////////////////////
