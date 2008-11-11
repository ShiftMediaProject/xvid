;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - simple de-interlacer
; *  Copyright(C) 2006 Pascal Massimino <skal@xvid.org>
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
; * $Id: deintl_sse.asm,v 1.3 2008-11-11 20:46:24 Isibaar Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * Oct 13 2006:  initial version
; *
; *************************************************************************/

bits 32

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

;//////////////////////////////////////////////////////////////////////

cglobal xvid_deinterlace_sse

;//////////////////////////////////////////////////////////////////////

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

align 16
Mask_6b  times 16 db 0x3f
Rnd_3b:  times 16 db 3

SECTION .text

;//////////////////////////////////////////////////////////////////////
;// sse version

align 16
xvid_deinterlace_sse:

  mov eax, [esp+ 4]  ; Pix
  mov ecx, [esp+12]  ; Height
  mov edx, [esp+16]  ; BpS

  push ebx
  mov ebx, [esp+4+ 8] ; Width

  add ebx, 7
  shr ecx, 1
  shr ebx, 3        ; Width /= 8
  dec ecx

  movq mm6, [Mask_6b]

.Loop_x:
  push eax
  movq mm1, [eax      ]
  movq mm2, [eax+  edx]
  lea  eax, [eax+  edx]
  movq mm0, mm2

  push ecx

.Loop:
  movq    mm3, [eax+  edx]
  movq    mm4, [eax+2*edx]
  movq    mm5, mm2
  pavgb   mm0, mm4
  pavgb   mm1, mm3
  movq    mm7, mm2
  psubusb mm2, mm0
  psubusb mm0, mm7
  paddusb mm0, [Rnd_3b]
  psrlw   mm2, 2
  psrlw   mm0, 2
  pand    mm2, mm6
  pand    mm0, mm6
  paddusb mm1, mm2
  psubusb mm1, mm0
  movq   [eax], mm1
  lea  eax, [eax+2*edx]
  movq mm0, mm5
  movq mm1, mm3
  movq mm2, mm4
  dec ecx
  jg .Loop

  pavgb mm0, mm2     ; p0 += p2
  pavgb mm1, mm1     ; p1 += p1
  movq    mm7, mm2
  psubusb mm2, mm0
  psubusb mm0, mm7
  paddusb mm0, [Rnd_3b]
  psrlw   mm2, 2
  psrlw   mm0, 2
  pand    mm2, mm6
  pand    mm0, mm6
  paddusb mm1, mm2
  psubusb mm1, mm0
  movq   [eax], mm1

  pop ecx
  pop eax
  add eax, 8

  dec ebx
  jg .Loop_x

  pop ebx
  ret
ENDFUNC

;//////////////////////////////////////////////////////////////////////

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif
