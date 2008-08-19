;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - Interlacing Field test -
; *
; *  Copyright(C) 2002 Daniel Smith <danielsmith@astroboymail.com>
; *		  2004 Andre Werthmann <wertmann@aei.mpg.de>
; *
; *  This program is free software ; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation ; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program ; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: interlacing_mmx.asm,v 1.4 2008-08-19 09:17:17 Isibaar Exp $
; *
; ***************************************************************************/

BITS 64

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

;=============================================================================
; Read only data
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

; advances to next block on right
ALIGN 16
nexts:
	dd 0, 0, 8, 120, 8

; multiply word sums into dwords
ALIGN 16
ones:
	times 4 dw 1

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal MBFieldTest_x86_64

; neater
%define	line0	rsi
%define	line1	rsi+16
%define	line2	rsi+32
%define	line3	rsi+48
%define	line4	rsi+64
%define	line5	rsi+80
%define	line6	rsi+96
%define	line7	rsi+112
%define	line8	rdi
%define	line9	rdi+16
%define	line10	rdi+32
%define	line11	rdi+48
%define	line12	rdi+64
%define	line13	rdi+80
%define	line14	rdi+96
%define	line15	rdi+112

; keep from losing track which reg holds which line - these never overlap
%define	m00		mm0
%define	m01		mm1
%define	m02		mm2
%define	m03		mm0
%define	m04		mm1
%define	m05		mm2
%define	m06		mm0
%define	m07		mm1
%define	m08		mm2
%define	m09		mm0
%define	m10		mm1
%define	m11		mm2
%define	m12		mm0
%define	m13		mm1
%define	m14		mm2
%define	m15		mm0

; gets diff between three lines low(%2),mid(%3),hi(%4): frame = mid-low, field = hi-low
%macro ABS8 4
  movq %4, [%1]         ; m02 = hi
  movq mm3, %2          ; mm3 = low copy

  pxor mm4, mm4         ; mm4 = 0
  pxor mm5, mm5         ; mm5 = 0

  psubw %2,  %3         ; diff(med,low) for frame
  psubw mm3, %4         ; diff(hi,low) for field

  pcmpgtw mm4, %2       ; if (diff<0), mm4 will be all 1's, else all 0's
  pcmpgtw mm5, mm3
  pxor %2,  mm4         ; this will get abs(), but off by 1 if (diff<0)
  pxor mm3, mm5
  psubw %2,  mm4        ; correct abs being off by 1 when (diff<0)
  psubw mm3, mm5

  paddw mm6, %2         ; add to totals
  paddw mm7, mm3
%endmacro

;-----------------------------------------------------------------------------
;
; uint32_t MBFieldTest_mmx(int16_t * const data);
;
;-----------------------------------------------------------------------------

ALIGN 16
MBFieldTest_x86_64:
  mov rsi, rdi				; rsi = top left block
					; rdi is already data[]
  add rdi, 256                  ; rdi = bottom left block

  pxor mm6, mm6                 ; frame total
  pxor mm7, mm7                 ; field total

  mov rax, 4                    ; we do left 8 bytes of data[0*64], then right 8 bytes
                                ; then left 8 bytes of data[1*64], then last 8 bytes
.loop:
  movq m00, [line0]             ; line0
  movq m01, [line1]             ; line1

  ABS8 line2, m00, m01, m02     ; frame += (line2-line1), field += (line2-line0)
  ABS8 line3, m01, m02, m03
  ABS8 line4, m02, m03, m04
  ABS8 line5, m03, m04, m05
  ABS8 line6, m04, m05, m06
  ABS8 line7, m05, m06, m07
  ABS8 line8, m06, m07, m08

  movq m09, [line9]             ; line9-line7, no frame comp for line9-line8!
  pxor mm4, mm4
  psubw m07, m09
  pcmpgtw mm4, mm1
  pxor m07, mm4
  psubw m07, mm4
  paddw mm7, m07                ; add to field total

  ABS8 line10, m08, m09, m10    ; frame += (line10-line9), field += (line10-line8)
  ABS8 line11, m09, m10, m11
  ABS8 line12, m10, m11, m12
  ABS8 line13, m11, m12, m13
  ABS8 line14, m12, m13, m14
  ABS8 line15, m13, m14, m15

  pxor mm4, mm4                 ; line15-line14, we're done with field comps!
  psubw m14, m15
  pcmpgtw mm4, m14
  pxor m14, mm4
  psubw m14, mm4
  paddw mm6, m14                ; add to frame total

  lea r9, [nexts wrt rip]	; pic code
  mov ecx, [r9+rax*4]           ; move rsi/rdi 8 pixels to the right
  add rsi, rcx
  add rdi, rcx

  dec rax
  jnz near .loop

.decide:
  movq mm0, [ones wrt rip]      ; add packed words into single dwords
  pmaddwd mm6, mm0
  pmaddwd mm7, mm0

  movq mm0, mm6                 ; ecx will be frame total, edx field
  movq mm1, mm7
  psrlq mm0, 32
  psrlq mm1, 32
  paddd mm0, mm6
  paddd mm1, mm7
  movd rcx, mm0
  movd rdx, mm1

  add rdx, 350                  ; add bias against field decision
  cmp rcx, rdx
  jb .end                       ; if frame<field, don't use field dct
  inc rax                       ; if frame>=field, use field dct (return 1)

.end:
  ret
.endfunc


%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

