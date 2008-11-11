;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - mmx 8x8 block-based halfpel interpolation -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2002 Michael Militzer <isibaar@xvid.org>
; *		  2004 Andre Werthmann <wertmann@aei.mpg.de> (amd64 conversation)
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
; ****************************************************************************/

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

;-----------------------------------------------------------------------------
; (16 - r) rounding table
;-----------------------------------------------------------------------------

ALIGN 16
rounding_lowpass_mmx:
	times 4 dw 16
	times 4 dw 15

;-----------------------------------------------------------------------------
; (1 - r) rounding table
;-----------------------------------------------------------------------------

rounding1_mmx:
	times 4 dw 1
	times 4 dw 0

;-----------------------------------------------------------------------------
; (2 - r) rounding table
;-----------------------------------------------------------------------------

rounding2_mmx:
	times 4 dw 2
	times 4 dw 1

mmx_one:
	times 8 db 1

mmx_two:
	times 8 db 2

mmx_three:
	times 8 db 3

mmx_five:
	times 4 dw 5

mmx_mask:
	times 8 db 254

mmx_mask2:
	times 8 db 252

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal interpolate8x8_avg2_x86_64
cglobal interpolate8x8_avg4_x86_64
cglobal interpolate8x8_6tap_lowpass_h_x86_64
cglobal interpolate8x8_6tap_lowpass_v_x86_64

%macro  CALC_AVG 6
  punpcklbw %3, %6
  punpckhbw %4, %6

  paddusw %1, %3    ; mm01 += mm23
  paddusw %2, %4
  paddusw %1, %5    ; mm01 += rounding
  paddusw %2, %5

  psrlw %1, 1   ; mm01 >>= 1
  psrlw %2, 1
%endmacro

;-----------------------------------------------------------------------------
;
; void interpolate8x8_avg2_x86_64(uint8_t const *dst,
;                              const uint8_t * const src1,
;                              const uint8_t * const src2,
;                              const uint32_t stride,
;                              const uint32_t rounding,
;                              const uint32_t height);
;
;-----------------------------------------------------------------------------

%macro AVG2_MMX_RND0 0
  movq mm0, [rax]           ; src1 -> mm0
  movq mm1, [rbx]           ; src2 -> mm1

  movq mm4, [rax+rdx]
  movq mm5, [rbx+rdx]

  movq mm2, mm0             ; src1 -> mm2
  movq mm3, mm1             ; src2 -> mm3

  pand mm2, mm7             ; isolate the lsb
  pand mm3, mm7             ; isolate the lsb

  por mm2, mm3              ; ODD(src1) OR ODD(src2) -> mm2

  movq mm3, mm4
  movq mm6, mm5

  pand mm3, mm7
  pand mm6, mm7

  por mm3, mm6

  pand mm0, [mmx_mask wrt rip]
  pand mm1, [mmx_mask wrt rip]
  pand mm4, [mmx_mask wrt rip]
  pand mm5, [mmx_mask wrt rip]

  psrlq mm0, 1              ; src1 / 2
  psrlq mm1, 1              ; src2 / 2

  psrlq mm4, 1
  psrlq mm5, 1

  paddb mm0, mm1            ; src1/2 + src2/2 -> mm0
  paddb mm0, mm2            ; correct rounding error

  paddb mm4, mm5
  paddb mm4, mm3

  lea rax, [rax+2*rdx]
  lea rbx, [rbx+2*rdx]

  movq [rcx], mm0           ; (src1 + src2 + 1) / 2 -> dst
  movq [rcx+rdx], mm4
%endmacro

%macro AVG2_MMX_RND1 0
  movq mm0, [rax]           ; src1 -> mm0
  movq mm1, [rbx]           ; src2 -> mm1

  movq mm4, [rax+rdx]
  movq mm5, [rbx+rdx]

  movq mm2, mm0             ; src1 -> mm2
  movq mm3, mm1             ; src2 -> mm3

  pand mm2, mm7             ; isolate the lsb
  pand mm3, mm7             ; isolate the lsb

  pand mm2, mm3             ; ODD(src1) AND ODD(src2) -> mm2

  movq mm3, mm4
  movq mm6, mm5

  pand mm3, mm7
  pand mm6, mm7

  pand mm3, mm6

  pand mm0, [mmx_mask wrt rip]
  pand mm1, [mmx_mask wrt rip]
  pand mm4, [mmx_mask wrt rip]
  pand mm5, [mmx_mask wrt rip]

  psrlq mm0, 1              ; src1 / 2
  psrlq mm1, 1              ; src2 / 2

  psrlq mm4, 1
  psrlq mm5, 1

  paddb mm0, mm1            ; src1/2 + src2/2 -> mm0
  paddb mm0, mm2            ; correct rounding error

  paddb mm4, mm5
  paddb mm4, mm3

  lea rax, [rax+2*rdx]
  lea rbx, [rbx+2*rdx]

  movq [rcx], mm0           ; (src1 + src2 + 1) / 2 -> dst
  movq [rcx+rdx], mm4
%endmacro

ALIGN 16
interpolate8x8_avg2_x86_64:

  push rbx

  mov rax, r8			; 5. par
  test rax, rax

  jnz near .rounding1

  mov rax, r9			; 6. par (height)
  sub rax, 8
  test rax, rax

  mov rbx, rdx			; src2
  mov rdx, rcx			; stride
  mov rax, rsi			; src1
  mov rcx, rdi			; dst

  movq mm7, [mmx_one wrt rip]

  jz near .start0

  AVG2_MMX_RND0
  lea rcx, [rcx+2*rdx]

.start0:

  AVG2_MMX_RND0
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND0
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND0
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND0

  pop rbx
  ret

.rounding1:
  mov rax, r9				; 6. par (height)
  sub rax, 8
  test rax, rax

  mov rbx, rdx			; src2
  mov rdx, rcx			; stride
  mov rax, rsi			; src1
  mov rcx, rdi			; dst

  movq mm7, [mmx_one wrt rip]

  jz near .start1

  AVG2_MMX_RND1
  lea rcx, [rcx+2*rdx]

.start1:

  AVG2_MMX_RND1
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND1
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND1
  lea rcx, [rcx+2*rdx]
  AVG2_MMX_RND1

  pop rbx
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x8_avg4_x86_64(uint8_t const *dst,
;                              const uint8_t * const src1,
;                              const uint8_t * const src2,
;                              const uint8_t * const src3,
;                              const uint8_t * const src4,
;                              const uint32_t stride,
;                              const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro AVG4_MMX_RND0 0
  movq mm0, [rax]           ; src1 -> mm0
  movq mm1, [rbx]           ; src2 -> mm1

  movq mm2, mm0
  movq mm3, mm1

  pand mm2, [mmx_three wrt rip]
  pand mm3, [mmx_three wrt rip]

  pand mm0, [mmx_mask2 wrt rip]
  pand mm1, [mmx_mask2 wrt rip]

  psrlq mm0, 2
  psrlq mm1, 2

  lea rax, [rax+rdx]
  lea rbx, [rbx+rdx]

  paddb mm0, mm1
  paddb mm2, mm3

  movq mm4, [rsi]           ; src3 -> mm0
  movq mm5, [rdi]           ; src4 -> mm1

  movq mm1, mm4
  movq mm3, mm5

  pand mm1, [mmx_three wrt rip]
  pand mm3, [mmx_three wrt rip]

  pand mm4, [mmx_mask2 wrt rip]
  pand mm5, [mmx_mask2 wrt rip]

  psrlq mm4, 2
  psrlq mm5, 2

  paddb mm4, mm5
  paddb mm0, mm4

  paddb mm1, mm3
  paddb mm2, mm1

  paddb mm2, [mmx_two wrt rip]
  pand mm2, [mmx_mask2 wrt rip]

  psrlq mm2, 2
  paddb mm0, mm2

  lea rsi, [rsi+rdx]
  lea rdi, [rdi+rdx]

  movq [rcx], mm0           ; (src1 + src2 + src3 + src4 + 2) / 4 -> dst
%endmacro

%macro AVG4_MMX_RND1 0
  movq mm0, [rax]           ; src1 -> mm0
  movq mm1, [rbx]           ; src2 -> mm1

  movq mm2, mm0
  movq mm3, mm1

  pand mm2, [mmx_three wrt rip]
  pand mm3, [mmx_three wrt rip]

  pand mm0, [mmx_mask2 wrt rip]
  pand mm1, [mmx_mask2 wrt rip]

  psrlq mm0, 2
  psrlq mm1, 2

  lea rax,[rax+rdx]
  lea rbx,[rbx+rdx]

  paddb mm0, mm1
  paddb mm2, mm3

  movq mm4, [rsi]           ; src3 -> mm0
  movq mm5, [rdi]           ; src4 -> mm1

  movq mm1, mm4
  movq mm3, mm5

  pand mm1, [mmx_three wrt rip]
  pand mm3, [mmx_three wrt rip]

  pand mm4, [mmx_mask2 wrt rip]
  pand mm5, [mmx_mask2 wrt rip]

  psrlq mm4, 2
  psrlq mm5, 2

  paddb mm4, mm5
  paddb mm0, mm4

  paddb mm1, mm3
  paddb mm2, mm1

  paddb mm2, [mmx_one wrt rip]
  pand mm2, [mmx_mask2 wrt rip]

  psrlq mm2, 2
  paddb mm0, mm2

  lea rsi,[rsi+rdx]
  lea rdi,[rdi+rdx]

  movq [rcx], mm0           ; (src1 + src2 + src3 + src4 + 2) / 4 -> dst
%endmacro

ALIGN 16
interpolate8x8_avg4_x86_64:

  push rbx

  mov rax, [rsp + 8 + 8]	; rounding

  test rax, rax

  mov rbx, rdx				; src2
  mov rdx, r9				; stride
  mov rax, rsi				; src1
  mov rsi, rcx				; src3
  mov rcx, rdi				; dst
  mov rdi, r8				; src4

  movq mm7, [mmx_one wrt rip]

  jnz near .rounding1

  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND0

  pop rbx
  ret

.rounding1:
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1
  lea rcx, [rcx+rdx]
  AVG4_MMX_RND1

  pop rbx
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_6tap_lowpass_h_x86_64(uint8_t const *dst,
;                                        const uint8_t * const src,
;                                        const uint32_t stride,
;                                        const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro LOWPASS_6TAP_H_MMX 0
  movq mm0, [rax]
  movq mm2, [rax+1]

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7

  punpckhbw mm1, mm7
  punpckhbw mm3, mm7

  paddw mm0, mm2
  paddw mm1, mm3

  psllw mm0, 2
  psllw mm1, 2

  movq mm2, [rax-1]
  movq mm4, [rax+2]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  psubsw mm0, mm2
  psubsw mm1, mm3

  pmullw mm0, [mmx_five wrt rip]
  pmullw mm1, [mmx_five wrt rip]

  movq mm2, [rax-2]
  movq mm4, [rax+3]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  paddsw mm0, mm2
  paddsw mm1, mm3

  paddsw mm0, mm6
  paddsw mm1, mm6

  psraw mm0, 5
  psraw mm1, 5

  lea rax, [rax+rdx]
  packuswb mm0, mm1
  movq [rcx], mm0
%endmacro

ALIGN 16
interpolate8x8_6tap_lowpass_h_x86_64:

  mov rax, rcx				; rounding

  lea r8, [rounding_lowpass_mmx wrt rip]
  movq mm6, [r8 + rax * 8]

  	; rdx is stride
  mov rax, rsi				; src
  mov rcx, rdi				; dst

  pxor mm7, mm7

  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_H_MMX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_6tap_lowpass_v_x86_64(uint8_t const *dst,
;                                        const uint8_t * const src,
;                                        const uint32_t stride,
;                                        const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro LOWPASS_6TAP_V_MMX 0
  movq mm0, [rax]
  movq mm2, [rax+rdx]

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7

  punpckhbw mm1, mm7
  punpckhbw mm3, mm7

  paddw mm0, mm2
  paddw mm1, mm3

  psllw mm0, 2
  psllw mm1, 2

  movq mm4, [rax+2*rdx]
  sub rax, rbx
  movq mm2, [rax+2*rdx]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  psubsw mm0, mm2
  psubsw mm1, mm3

  pmullw mm0, [mmx_five wrt rip]
  pmullw mm1, [mmx_five wrt rip]

  movq mm2, [rax+rdx]
  movq mm4, [rax+2*rbx]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  paddsw mm0, mm2
  paddsw mm1, mm3

  paddsw mm0, mm6
  paddsw mm1, mm6

  psraw mm0, 5
  psraw mm1, 5

  lea rax, [rax+4*rdx]
  packuswb mm0, mm1
  movq [rcx], mm0
%endmacro

ALIGN 16
interpolate8x8_6tap_lowpass_v_x86_64:

  push rbx

  mov rax, rcx				; rounding

  lea r8, [rounding_lowpass_mmx wrt rip]
  movq mm6, [r8 + rax * 8]

		; rdx is stride
  mov rax, rsi				; src
  mov rcx, rdi				; dst

  mov rbx, rdx
  shl rbx, 1
  add rbx, rdx

  pxor mm7, mm7

  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX
  lea rcx, [rcx+rdx]
  LOWPASS_6TAP_V_MMX

  pop rbx
  ret
ENDFUNC

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

