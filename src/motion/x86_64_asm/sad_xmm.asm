;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - K7 optimized SAD operators -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2001 Michael Militzer <isibaar@xvid.org>
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
; * $Id: sad_xmm.asm,v 1.2 2008-08-19 09:06:48 Isibaar Exp $
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

ALIGN 16
mmx_one: times 4 dw 1

;=============================================================================
; Helper macros
;=============================================================================

%macro SAD_16x16_SSE 0
  movq mm0, [rax]
  psadbw mm0, [rdx]
  movq mm1, [rax+8]
  add rax, rcx
  psadbw mm1, [rdx+8]
  paddusw mm5, mm0
  add rdx, rcx
  paddusw mm6, mm1
%endmacro

%macro SAD_8x8_SSE 0
  movq mm0, [rax]
  movq mm1, [rax+rcx]
  psadbw mm0, [rdx]
  psadbw mm1, [rdx+rcx]
  add rax, rbx
  add rdx, rbx
  paddusw mm5, mm0
  paddusw mm6, mm1
%endmacro

%macro SADBI_16x16_SSE 0
  movq mm0, [rax]
  movq mm1, [rax+8]
  movq mm2, [rdx]
  movq mm3, [rdx+8]
  pavgb mm2, [rbx]
  add rdx, rcx
  pavgb mm3, [rbx+8]
  add rbx, rcx
  psadbw mm0, mm2
  add rax, rcx
  psadbw mm1, mm3
  paddusw mm5, mm0
  paddusw mm6, mm1
%endmacro

%macro SADBI_8x8_XMM 0
  movq mm0, [rax]
  movq mm1, [rax+rcx]
  movq mm2, [rdx]
  movq mm3, [rdx+rcx]
  pavgb mm2, [rbx]
  lea rdx, [rdx+2*rcx]
  pavgb mm3, [rbx+rcx]
  lea rbx, [rbx+2*rcx]
  psadbw mm0, mm2
  lea rax, [rax+2*rcx]
  psadbw mm1, mm3
  paddusw mm5, mm0
  paddusw mm6, mm1
%endmacro

%macro MEAN_16x16_SSE 0
  movq mm0, [rax]
  movq mm1, [rax+8]
  psadbw mm0, mm7
  psadbw mm1, mm7
  add rax, rcx
  paddw mm5, mm0
  paddw mm6, mm1
%endmacro

%macro ABS_16x16_SSE 0
  movq mm0, [rax]
  movq mm1, [rax+8]
  psadbw mm0, mm4
  psadbw mm1, mm4
  lea rax, [rax+rcx]
  paddw mm5, mm0
  paddw mm6, mm1
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal sad16_x86_64
cglobal sad8_x86_64
cglobal sad16bi_x86_64
cglobal sad8bi_x86_64
cglobal dev16_x86_64
cglobal sad16v_x86_64

;-----------------------------------------------------------------------------
;
; uint32_t sad16_x86_64(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
;-----------------------------------------------------------------------------

ALIGN 16
sad16_x86_64:
  mov rcx, rdx		; stride
  mov rdx, rsi		; src2 (64bit pointer)
  mov rax, rdi		; src1 (64bit pointer)

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm6,mm5
  movd eax, mm6
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; uint32_t sad8_x86_64(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN 16
sad8_x86_64:
  mov rcx, rdx		; stride
  mov rdx, rsi		; src2
  mov rax, rdi		; src1

  push rbx

  lea rbx, [rcx+rcx]

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_8x8_SSE
  SAD_8x8_SSE
  SAD_8x8_SSE

  movq mm0, [rax]
  movq mm1, [rax+rcx]
  psadbw mm0, [rdx]
  psadbw mm1, [rdx+rcx]

  pop rbx

  paddusw mm5,mm0
  paddusw mm6,mm1

  paddusw mm6,mm5
  movd eax, mm6

  ret
.endfunc

;-----------------------------------------------------------------------------
;
; uint32_t sad16bi_x86_64(const uint8_t * const cur,
;					const uint8_t * const ref1,
;					const uint8_t * const ref2,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN 16
sad16bi_x86_64:
  push rbx
			; rcx is stride (4. arg)
  mov rbx, rdx		; ref2
  mov rdx, rsi		; ref1
  mov rax, rdi		; src

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE

  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE

  paddusw mm6,mm5
  movd eax, mm6
  pop rbx
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; uint32_t sad8bi_x86_64(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN 16
sad8bi_x86_64:
  push rbx
  			; rcx is stride
  mov rbx, rdx		; ref2
  mov rdx, rsi		; ref1
  mov rax, rdi		; src

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2
.Loop
  SADBI_8x8_XMM
  SADBI_8x8_XMM
  SADBI_8x8_XMM
  SADBI_8x8_XMM

  paddusw mm6,mm5
  movd eax, mm6
  pop rbx
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; uint32_t dev16_x86_64(const uint8_t * const cur,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN 16
dev16_x86_64:
  mov rcx, rsi		; stride
  mov rax, rdi		; src

  pxor mm7, mm7 ; zero
  pxor mm5, mm5 ; mean accums
  pxor mm6, mm6

  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE

  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE

  paddusw mm6, mm5

  movq mm4, mm6
  psllq mm4, 32
  paddd mm4, mm6
  psrld mm4, 8	  ; /= (16*16)

  packssdw mm4, mm4
  packuswb mm4, mm4

	; mm4 contains the mean

  mov rax, rdi		; src

  pxor mm5, mm5 ; sums
  pxor mm6, mm6

  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE

  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE

  paddusw mm6, mm5
  movq mm7, mm6
  psllq mm7, 32
  paddd mm6, mm7

  movd eax, mm6
  ret
.endfunc

;-----------------------------------------------------------------------------
;int sad16v_x86_64(const uint8_t * const cur,
;               const uint8_t * const ref,
;               const uint32_t stride,
;               int* sad8);
;-----------------------------------------------------------------------------

ALIGN 16
sad16v_x86_64:
  push rbx

  mov rbx, rcx		; sad ptr (64bit)
  mov rcx, rdx		; stride
  mov rdx, rsi		; src2
  mov rax, rdi		; src1

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2
  pxor mm7, mm7 ; total

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm7, mm5
  paddusw mm7, mm6
  movd [rbx], mm5
  movd [rbx+4], mm6

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm7, mm5
  paddusw mm7, mm6
  movd [rbx+8], mm5
  movd [rbx+12], mm6

  movd eax, mm7
  pop rbx
  ret
.endfunc

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

