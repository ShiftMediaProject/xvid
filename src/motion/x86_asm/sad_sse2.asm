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
; * $Id: sad_sse2.asm,v 1.19 2008-12-04 14:41:50 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
zero    times 4   dd 0

;=============================================================================
; Code
;=============================================================================

TEXT

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
  %1  xmm0, [TMP1]
  %1  xmm1, [TMP1+TMP0]
  lea TMP1,[TMP1+2*TMP0]
  movdqa  xmm2, [_EAX]
  movdqa  xmm3, [_EAX+TMP0]
  lea _EAX,[_EAX+2*TMP0]
  psadbw  xmm0, xmm2
  paddusw xmm4,xmm0
  psadbw  xmm1, xmm3
  paddusw xmm4,xmm1
%endmacro

%macro SAD16_SSE2_SSE3 1
  mov _EAX, prm1 ; cur (assumed aligned)
  mov TMP1, prm2 ; ref
  mov TMP0, prm3 ; stride

  pxor xmm4, xmm4 ; accum

  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1

  pshufd  xmm5, xmm4, 00000010b
  paddusw xmm4, xmm5
  pextrw  eax, xmm4, 0

  ret
%endmacro

ALIGN SECTION_ALIGN
sad16_sse2:
  SAD16_SSE2_SSE3 movdqu
ENDFUNC


ALIGN SECTION_ALIGN
sad16_sse3:
  SAD16_SSE2_SSE3 lddqu
ENDFUNC


;-----------------------------------------------------------------------------
; uint32_t dev16_sse2(const uint8_t * const cur, const uint32_t stride);
;-----------------------------------------------------------------------------

%macro MEAN_16x16_SSE2 1  ; _EAX: src, TMP0:stride, mm7: zero or mean => mm6: result
  %1 xmm0, [_EAX]
  %1 xmm1, [_EAX+TMP0]
  lea _EAX, [_EAX+2*TMP0]    ; + 2*stride
  psadbw xmm0, xmm5
  paddusw xmm4, xmm0
  psadbw xmm1, xmm5
  paddusw xmm4, xmm1
%endmacro


%macro MEAN16_SSE2_SSE3 1
  mov _EAX, prm1   ; src
  mov TMP0, prm2   ; stride

  pxor xmm4, xmm4     ; accum
  pxor xmm5, xmm5     ; zero

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  mov _EAX, prm1       ; src again

  pshufd   xmm5, xmm4, 10b
  paddusw  xmm5, xmm4
  pxor     xmm4, xmm4     ; zero accum
  psrlw    xmm5, 8        ; => Mean
  pshuflw  xmm5, xmm5, 0  ; replicate Mean
  packuswb xmm5, xmm5
  pshufd   xmm5, xmm5, 00000000b

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  pshufd   xmm5, xmm4, 10b
  paddusw  xmm5, xmm4
  pextrw eax, xmm5, 0

  ret
%endmacro

ALIGN SECTION_ALIGN
dev16_sse2:
  MEAN16_SSE2_SSE3 movdqu
ENDFUNC

ALIGN SECTION_ALIGN
dev16_sse3:
  MEAN16_SSE2_SSE3 lddqu
ENDFUNC

 
%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

