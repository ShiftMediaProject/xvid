;/**************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  mmx cbp calc
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
; * $Id: cbp_mmx.asm,v 1.7 2002-11-17 00:57:57 edgomez Exp $
; *
; *************************************************************************/

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

ignore_dc	dw		0, -1, -1, -1, -1, -1, -1, -1

section .text

cglobal calc_cbp_mmx

;===========================================================================
;
; uint32_t calc_cbp_mmx(const int16_t coeff[6][64]);
;
;===========================================================================

align 16
calc_cbp_mmx:
  push	ebx
  push	esi

  mov       esi, [esp + 8 + 4]	; coeff
  xor		eax, eax			; cbp = 0
  mov		edx, (1 << 5)

  movq		mm7, [ignore_dc]

.loop
  movq		mm0, [esi]
  movq		mm1, [esi+8]
  pand		mm0, mm7

  por     	mm0, [esi+16]
  por     	mm1, [esi+24]

  por     	mm0, [esi+32]
  por     	mm1, [esi+40]

  por     	mm0, [esi+48]
  por     	mm1, [esi+56]

  por     	mm0, [esi+64]
  por     	mm1, [esi+72]

  por     	mm0, [esi+80]
  por     	mm1, [esi+88]

  por     	mm0, [esi+96]
  por     	mm1, [esi+104]

  por     	mm0, [esi+112]
  por     	mm1, [esi+120]

  por     	mm0, mm1
  movq    	mm1, mm0
  psrlq   	mm1, 32
  lea		esi, [esi + 128]

  por     	mm0, mm1
  movd		ebx, mm0

  test 		ebx, ebx
  jz		.next
  or 		eax, edx     ; cbp |= 1 << (5-i)

.next
  shr 		edx,1
  jnc		.loop

  pop		esi
  pop		ebx
				
  ret