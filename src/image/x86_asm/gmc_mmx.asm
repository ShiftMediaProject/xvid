;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - GMC core functions -
; *  Copyright(C) 2006 Pascal Massimino <skal@planet-d.net>
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
; * $Id: gmc_mmx.asm,v 1.2 2006-11-07 19:59:03 Skal Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * Jun 14 2006:  initial version (during Germany/Poland match;)
; *
; *************************************************************************/

bits 32

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

;//////////////////////////////////////////////////////////////////////

cglobal xvid_GMC_Core_Lin_8_mmx
cglobal xvid_GMC_Core_Lin_8_sse2

;//////////////////////////////////////////////////////////////////////

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

align 16
Cst16:
times 8 dw 16

SECTION .text

;//////////////////////////////////////////////////////////////////////
;// mmx version

%macro GMC_4_SSE 2  ; %1: i   %2: out reg (mm5 or mm6)

  pcmpeqw   mm0, mm0
  movq      mm1, [eax+2*(%1)     ]  ; u0 | u1 | u2 | u3
  psrlw     mm0, 12                 ; mask 0x000f
  movq      mm2, [eax+2*(%1)+2*16]  ; v0 | v1 | v2 | v3

  pand      mm1, mm0  ; u0
  pand      mm2, mm0  ; v0

  movq      mm0, [Cst16]
  movq      mm3, mm1    ; u     | ...
  movq      mm4, mm0
  pmullw    mm3, mm2    ; u.v
  psubw     mm0, mm1    ; 16-u
  psubw     mm4, mm2    ; 16-v
  pmullw    mm2, mm0    ; (16-u).v
  pmullw    mm0, mm4    ; (16-u).(16-v)
  pmullw    mm1, mm4    ;     u .(16-v)

  movd      mm4, [ecx+edx  +%1]  ; src2
  movd       %2, [ecx+edx+1+%1]  ; src3
  punpcklbw mm4, mm7
  punpcklbw  %2, mm7
  pmullw    mm2, mm4
  pmullw    mm3,  %2

  movd      mm4, [ecx      +%1]  ; src0
  movd       %2, [ecx    +1+%1]  ; src1
  punpcklbw mm4, mm7
  punpcklbw  %2, mm7
  pmullw    mm4, mm0
  pmullw     %2, mm1

  paddw     mm2, mm3
  paddw      %2, mm4

  paddw      %2, mm2
%endmacro

align 16
xvid_GMC_Core_Lin_8_mmx:
  mov  eax, [esp + 8]  ; Offsets
  mov  ecx, [esp +12]  ; Src0
  mov  edx, [esp +16]  ; BpS

  pxor      mm7, mm7

  GMC_4_SSE 0, mm5
  GMC_4_SSE 4, mm6

;  pshufw   mm4, [esp +20], 01010101b  ; Rounder (bits [16..31])
  movd      mm4, [esp+20]  ; Rounder (bits [16..31])
  mov       eax, [esp + 4]  ; Dst
  punpcklwd mm4, mm4
  punpckhdq mm4, mm4

  paddw    mm5, mm4
  paddw    mm6, mm4
  psrlw    mm5, 8
  psrlw    mm6, 8
  packuswb mm5, mm6
  movq [eax], mm5

  ret
.endfunc

;//////////////////////////////////////////////////////////////////////
;// SSE2 version

%macro GMC_8_SSE2 0
  
  pcmpeqw   xmm0, xmm0
  movdqa    xmm1, [eax     ]  ; u...
  psrlw     xmm0, 12          ; mask = 0x000f
  movdqa    xmm2, [eax+2*16]  ; v...
  pand      xmm1, xmm0
  pand      xmm2, xmm0

  movdqa    xmm0, [Cst16]
  movdqa    xmm3, xmm1    ; u     | ...
  movdqa    xmm4, xmm0
  pmullw    xmm3, xmm2    ; u.v
  psubw     xmm0, xmm1    ; 16-u
  psubw     xmm4, xmm2    ; 16-v
  pmullw    xmm2, xmm0    ; (16-u).v
  pmullw    xmm0, xmm4    ; (16-u).(16-v)
  pmullw    xmm1, xmm4    ;     u .(16-v)

  movq      xmm4, [ecx+edx  ]  ; src2
  movq      xmm5, [ecx+edx+1]  ; src3
  punpcklbw xmm4, xmm7
  punpcklbw xmm5, xmm7
  pmullw    xmm2, xmm4
  pmullw    xmm3, xmm5

  movq      xmm4, [ecx      ]  ; src0
  movq      xmm5, [ecx    +1]  ; src1
  punpcklbw xmm4, xmm7
  punpcklbw xmm5, xmm7
  pmullw    xmm4, xmm0
  pmullw    xmm5, xmm1

  paddw     xmm2, xmm3
  paddw     xmm5, xmm4

  paddw     xmm5, xmm2
%endmacro

align 16
xvid_GMC_Core_Lin_8_sse2:
  mov  eax, [esp + 8]  ; Offsets
  mov  ecx, [esp +12]  ; Src0
  mov  edx, [esp +16]  ; BpS

  pxor     xmm7, xmm7

  GMC_8_SSE2

  movd      xmm4, [esp +20]
  pshuflw   xmm4, xmm4, 01010101b  ; Rounder (bits [16..31])
  punpckldq xmm4, xmm4
  mov  eax, [esp + 4]  ; Dst

  paddw    xmm5, xmm4
  psrlw    xmm5, 8
  packuswb xmm5, xmm5
  movq [eax], xmm5

  ret
.endfunc

;//////////////////////////////////////////////////////////////////////
