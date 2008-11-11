;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - K7 optimized SAD operators -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2002 Pascal Massimino <skal@planet-d.net>
; *               2004 Andre Werthmann <wertmann@aei.mpg.de>
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
; * $Id: sad_mmx.asm,v 1.3 2008-11-11 20:46:24 Isibaar Exp $
; *
; ***************************************************************************/

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

;=============================================================================
; Read only data
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal sse8_16bit_x86_64
cglobal sse8_8bit_x86_64

;-----------------------------------------------------------------------------
;
; uint32_t sse8_16bit_x86_64x(const int16_t *b1,
;                         const int16_t *b2,
;                         const uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro ROW_SSE_16Bit_MMX 2
  movq mm0, [%1]
  movq mm1, [%1+8]
  psubw mm0, [%2]
  psubw mm1, [%2+8]
  pmaddwd mm0, mm0
  pmaddwd mm1, mm1
  paddd mm2, mm0
  paddd mm2, mm1
%endmacro	
	
sse8_16bit_x86_64:

  ; rdx is stride
  ; rsi is b2
  ; rdi is b1

  ;; Reset the sse accumulator
  pxor mm2, mm2

  ;; Let's go
%rep 8
  ROW_SSE_16Bit_MMX rsi, rdi
  lea rsi, [rsi+rdx]
  lea rdi, [rdi+rdx]
%endrep

  ;; Finish adding each dword of the accumulator
  movq mm3, mm2
  psrlq mm2, 32
  paddd mm2, mm3
  movd eax, mm2

  ;; All done
  ret
ENDFUNC
  
;-----------------------------------------------------------------------------
;
; uint32_t sse8_8bit_x86_64(const int8_t *b1,
;                        const int8_t *b2,
;                        const uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro ROW_SSE_8bit_MMX 2
  movq mm0, [%1] ; load a row
  movq mm2, [%2] ; load a row

  movq mm1, mm0  ; copy row
  movq mm3, mm2  ; copy row

  punpcklbw mm0, mm7 ; turn the 4low elements into 16bit
  punpckhbw mm1, mm7 ; turn the 4high elements into 16bit

  punpcklbw mm2, mm7 ; turn the 4low elements into 16bit
  punpckhbw mm3, mm7 ; turn the 4high elements into 16bit

  psubw mm0, mm2 ; low  part of src-dst
  psubw mm1, mm3 ; high part of src-dst

  pmaddwd mm0, mm0 ; compute the square sum
  pmaddwd mm1, mm1 ; compute the square sum

  paddd mm6, mm0 ; add to the accumulator
  paddd mm6, mm1 ; add to the accumulator
%endmacro

sse8_8bit_x86_64:
  ;; Reset the sse accumulator
  pxor mm6, mm6

  ;; Used to interleave 8bit data with 0x00 values
  pxor mm7, mm7

  ;; Let's go
%rep 8
  ROW_SSE_8bit_MMX rsi, rdi
  lea rsi, [rsi+rdx]
  lea rdi, [rdi+rdx]
%endrep

  ;; Finish adding each dword of the accumulator
  movq mm7, mm6
  psrlq mm6, 32
  paddd mm6, mm7
  movd eax, mm6

  ;; All done
  ret
ENDFUNC

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

