;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - mmx post processing -
; *
; *  Copyright(C) 2004 Peter Ross <pross@xvid.org>
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
; * $Id: postprocessing_mmx.asm,v 1.2 2004-07-24 11:46:08 edgomez Exp $
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

;===========================================================================
; read only data
;===========================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

mmx_0x80:
	times 8 db 0x80

mmx_offset:
%assign i -128
%rep 256
	times 8 db i
	%assign i i+1
%endrep


;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal image_brightness_mmx


;//////////////////////////////////////////////////////////////////////
;// image_brightness_mmx
;//////////////////////////////////////////////////////////////////////

align 16
image_brightness_mmx:

	push esi
	push edi

	movq mm6, [mmx_0x80]

	mov eax, [esp+8+20] ; offset
	movq mm7, [mmx_offset + (eax + 128)*8]   ; being lazy

	mov edx, [esp+8+4]  ; Dst
	mov ecx, [esp+8+8]  ; stride
	mov esi, [esp+8+12] ; width
	mov edi, [esp+8+16] ; height

.yloop
	xor	eax, eax

.xloop
	movq mm0, [edx + eax]
	movq mm1, [edx + eax + 8]	; mm0 = [dst]

	paddb mm0, mm6				; unsigned -> signed domain
	paddb mm1, mm6
	paddsb mm0, mm7
	paddsb mm1, mm7				; mm0 += offset
	psubb mm0, mm6
	psubb mm1, mm6				; signed -> unsigned domain

	movq [edx + eax], mm0
	movq [edx + eax + 8], mm1	; [dst] = mm0

	add	eax,16
	cmp	eax,esi	
	jl .xloop

	add edx, ecx				; dst += stride
	sub edi, 1
	jg .yloop

	pop edi
	pop esi

	ret
;//////////////////////////////////////////////////////////////////////
