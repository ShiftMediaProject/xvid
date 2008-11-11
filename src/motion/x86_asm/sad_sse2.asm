;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 optimized SAD operators -
; *
; *  Copyright(C) 2003 Pascal Massimino <skal@planet-d.net>
; *
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
; * $Id: sad_sse2.asm,v 1.15 2008-11-11 20:46:24 Isibaar Exp $
; *
; ***************************************************************************/

BITS 32

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

ALIGN 64
zero    times 4   dd 0

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal  sad16_sse2
cglobal  dev16_sse2

cglobal  sad16_sse3
cglobal  dev16_sse3

;-----------------------------------------------------------------------------
; uint32_t sad16_sse2 (const uint8_t * const cur, <- assumed aligned!
;                      const uint8_t * const ref,
;	                   const uint32_t stride,
;                      const uint32_t /*ignored*/);
;-----------------------------------------------------------------------------


%macro SAD_16x16_SSE2 1
  %1  xmm0, [edx]
  %1  xmm1, [edx+ecx]
  lea edx,[edx+2*ecx]
  movdqa  xmm2, [eax]
  movdqa  xmm3, [eax+ecx]
  lea eax,[eax+2*ecx]
  psadbw  xmm0, xmm2
  paddusw xmm6,xmm0
  psadbw  xmm1, xmm3
  paddusw xmm6,xmm1
%endmacro

%macro SAD16_SSE2_SSE3 1
  mov eax, [esp+ 4] ; cur (assumed aligned)
  mov edx, [esp+ 8] ; ref
  mov ecx, [esp+12] ; stride

  pxor xmm6, xmm6 ; accum

  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1

  pshufd  xmm5, xmm6, 00000010b
  paddusw xmm6, xmm5
  pextrw  eax, xmm6, 0
  ret
%endmacro

ALIGN 16
sad16_sse2:
  SAD16_SSE2_SSE3 movdqu
ENDFUNC


ALIGN 16
sad16_sse3:
  SAD16_SSE2_SSE3 lddqu
ENDFUNC


;-----------------------------------------------------------------------------
; uint32_t dev16_sse2(const uint8_t * const cur, const uint32_t stride);
;-----------------------------------------------------------------------------

%macro MEAN_16x16_SSE2 1  ; eax: src, ecx:stride, mm7: zero or mean => mm6: result
  %1 xmm0, [eax]
  %1 xmm1, [eax+ecx]
  lea eax, [eax+2*ecx]    ; + 2*stride
  psadbw xmm0, xmm7
  paddusw xmm6, xmm0
  psadbw xmm1, xmm7
  paddusw xmm6, xmm1
%endmacro


%macro MEAN16_SSE2_SSE3 1
  mov eax, [esp+ 4]   ; src
  mov ecx, [esp+ 8]   ; stride

  pxor xmm6, xmm6     ; accum
  pxor xmm7, xmm7     ; zero

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  mov eax, [esp+ 4]       ; src again

  pshufd   xmm7, xmm6, 10b
  paddusw  xmm7, xmm6
  pxor     xmm6, xmm6     ; zero accum
  psrlw    xmm7, 8        ; => Mean
  pshuflw  xmm7, xmm7, 0  ; replicate Mean
  packuswb xmm7, xmm7
  pshufd   xmm7, xmm7, 00000000b

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  pshufd   xmm7, xmm6, 10b
  paddusw  xmm7, xmm6
  pextrw eax, xmm7, 0
  ret
%endmacro

ALIGN 16
dev16_sse2:
  MEAN16_SSE2_SSE3 movdqu
ENDFUNC

ALIGN 16
dev16_sse3:
  MEAN16_SSE2_SSE3 lddqu
ENDFUNC

 
%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

