;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MPEG4 Quantization H263 implementation / MMX optimized -
; *
; *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
; *               2002-2003 Pascal Massimino <skal@planet-d.net>
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
; * $Id: quantize_h263_mmx.asm,v 1.2 2008-08-19 09:06:48 Isibaar Exp $
; *
; ****************************************************************************/

; enable dequant saturate [-2048,2047], test purposes only.
%define SATURATE

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
; Read only Local data
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

ALIGN 16
plus_one:
	times 8 dw 1

;-----------------------------------------------------------------------------
;
; subtract by Q/2 table
;
;-----------------------------------------------------------------------------

ALIGN 16
mmx_sub:
%assign quant 1
%rep 31
	times 4 dw  quant / 2
	%assign quant quant+1
%endrep

;-----------------------------------------------------------------------------
;
; divide by 2Q table
;
; use a shift of 16 to take full advantage of _pmulhw_
; for q=1, _pmulhw_ will overflow so it is treated seperately
; (3dnow2 provides _pmulhuw_ which wont cause overflow)
;
;-----------------------------------------------------------------------------

ALIGN 16
mmx_div:
%assign quant 1
%rep 31
	times 4 dw  (1<<16) / (quant*2) + 1
	%assign quant quant+1
%endrep

;-----------------------------------------------------------------------------
;
; add by (odd(Q) ? Q : Q - 1) table
;
;-----------------------------------------------------------------------------

ALIGN 16
mmx_add:
%assign quant 1
%rep 31
	%if quant % 2 != 0
	times 4 dw  quant
	%else
	times 4 dw quant - 1
	%endif
	%assign quant quant+1
%endrep

;-----------------------------------------------------------------------------
;
; multiple by 2Q table
;
;-----------------------------------------------------------------------------

ALIGN 16
mmx_mul:
%assign quant 1
%rep 31
	times 4 dw  quant*2
	%assign quant quant+1
%endrep

;-----------------------------------------------------------------------------
;
; saturation limits
;
;-----------------------------------------------------------------------------

ALIGN 16
sse2_2047:
	times 8 dw 2047

ALIGN 16
mmx_2047:
	times 4 dw 2047

ALIGN 8
mmx_32768_minus_2048:
	times 4 dw (32768-2048)

mmx_32767_minus_2047:
	times 4 dw (32767-2047)


;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal quant_h263_intra_x86_64
cglobal quant_h263_inter_x86_64
cglobal dequant_h263_intra_x86_64
cglobal dequant_h263_inter_x86_64

;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_x86_64(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint32_t dcscalar,
;                               const uint16_t *mpeg_matrices);
; Port of the 32bit mmx cousin
;-----------------------------------------------------------------------------

ALIGN 16
quant_h263_intra_x86_64:
  mov rax, rdx			; quant
			; rsi is data
			; rdi is coeff
  mov r8, rcx			; save dscalar

  xor rcx, rcx
  cmp rax, 1
  jz .q1loop

  lea r9, [mmx_div wrt rip]
  movq mm7, [r9 + rax * 8 - 8]

ALIGN 16
.loop
  movq mm0, [rsi + 8*rcx]           ; mm0 = [1st]
  movq mm3, [rsi + 8*rcx + 8]
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  pmulhw mm0, mm7                   ; mm0 = (mm0 / 2Q) >> 16
  pmulhw mm3, mm7                   ;
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4                    ;
  movq [rdi + 8*rcx], mm0
  movq [rdi + 8*rcx + 8], mm3

  add rcx, 2
  cmp rcx, 16
  jnz .loop

.done

    ; caclulate  data[0] // (int32_t)dcscalar)
  mov rcx, r8				; dscalar
  mov rdx, rcx
  movsx eax, word [rsi]			; data[0] with sign extend
  shr rdx, 1                    ; edx = dcscalar /2
  cmp eax, 0 
  jg .gtzero

  sub rax, rdx
  jmp short .mul

.gtzero
  add rax, rdx
.mul
  cdq ; expand eax -> edx:eax
  idiv ecx		; eax = edx:eax / dcscalar
  mov [rdi], ax		; coeff[0] = ax

  xor rax, rax      ; return(0);

  ret

ALIGN 16
.q1loop
  movq mm0, [rsi + 8*rcx]           ; mm0 = [1st]
  movq mm3, [rsi + 8*rcx + 8]
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  psrlw mm0, 1                      ; mm0 >>= 1   (/2)
  psrlw mm3, 1                      ;
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  pxor mm3, mm4
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4                    ;
  movq [rdi + 8*rcx], mm0
  movq [rdi + 8*rcx + 8], mm3

  add rcx, 2
  cmp rcx, 16
  jnz .q1loop

  jmp .done
.endfunc


;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_x86_64(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint16_t *mpeg_matrices);
; Port of the 32bit mmx cousin
;-----------------------------------------------------------------------------

ALIGN 16
quant_h263_inter_x86_64:
  mov rax, rdx				; quant
  			; rsi is data
			; rdi is coeff

  xor rcx, rcx

  pxor mm5, mm5                     ; sum
  lea r9, [mmx_sub wrt rip]
  movq mm6, [r9 + rax * 8 - 8] ; sub

  cmp rax, 1
  jz .q1loop

  lea r9, [mmx_div wrt rip]
  movq mm7, [r9 + rax * 8 - 8] ; divider

ALIGN 8
.loop
  movq mm0, [rsi + 8*rcx]           ; mm0 = [1st]
  movq mm3, [rsi + 8*rcx + 8]
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  psubusw mm0, mm6                  ; mm0 -= sub (unsigned, dont go < 0)
  psubusw mm3, mm6                  ;
  pmulhw mm0, mm7                   ; mm0 = (mm0 / 2Q) >> 16
  pmulhw mm3, mm7                   ;
  paddw mm5, mm0                    ; sum += mm0
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  paddw mm5, mm3                    ;
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4
  movq [rdi + 8*rcx], mm0
  movq [rdi + 8*rcx + 8], mm3

  add rcx, 2
  cmp rcx, 16
  jnz .loop

.done
  pmaddwd mm5, [plus_one wrt rip]
  movq mm0, mm5
  psrlq mm5, 32
  paddd mm0, mm5

  movd rax, mm0     ; return sum

  ret

ALIGN 8
.q1loop
  movq mm0, [rsi + 8*rcx]           ; mm0 = [1st]
  movq mm3, [rsi + 8*rcx+ 8]        ;
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  psubusw mm0, mm6                  ; mm0 -= sub (unsigned, dont go < 0)
  psubusw mm3, mm6                  ;
  psrlw mm0, 1                      ; mm0 >>= 1   (/2)
  psrlw mm3, 1                      ;
  paddw mm5, mm0                    ; sum += mm0
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  paddw mm5, mm3                    ;
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4
  movq [rdi + 8*rcx], mm0
  movq [rdi + 8*rcx + 8], mm3

  add rcx, 2
  cmp rcx, 16
  jnz .q1loop

  jmp .done
.endfunc


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_x86_64(int16_t *data,
;                                 const int16_t const *coeff,
;                                 const uint32_t quant,
;                                 const uint32_t dcscalar,
;                                 const uint16_t *mpeg_matrices);
; port of the 32bit xmm cousin
;-----------------------------------------------------------------------------

  ; this is the same as dequant_inter_mmx, except that we're
  ; saturating using 'pminsw' (saves 2 cycles/loop => ~5% faster)

ALIGN 16
dequant_h263_intra_x86_64:

  mov rax, rdx				; quant
  mov [rsp-8], rcx			; save dscalar
  mov rcx, rsi				; coeff
  mov rdx, rdi				; data

  lea r9, [mmx_add wrt rip]
  movq mm6, [r9 + rax*8 - 8]   ; quant or quant-1
  lea r9, [mmx_mul wrt rip]
  movq mm7, [r9 + rax*8 - 8]   ; 2*quant
  mov rax, -16

ALIGN 16
.loop
  movq mm0, [rcx+8*rax+8*16]        ; c  = coeff[i]
  movq mm3, [rcx+8*rax+8*16 + 8]    ; c' = coeff[i+1]
  pxor mm1, mm1
  pxor mm4, mm4
  pcmpgtw mm1, mm0                  ; sign(c)
  pcmpgtw mm4, mm3                  ; sign(c')
  pxor mm2, mm2
  pxor mm5, mm5
  pcmpeqw mm2, mm0                  ; c is zero
  pcmpeqw mm5, mm3                  ; c' is zero
  pandn mm2, mm6                    ; offset = isZero ? 0 : quant_add
  pandn mm5, mm6
  pxor mm0, mm1                     ; negate if negative
  pxor mm3, mm4                     ; negate if negative
  psubw mm0, mm1
  psubw mm3, mm4
  pmullw mm0, mm7                   ; *= 2Q
  pmullw mm3, mm7                   ; *= 2Q
  paddw mm0, mm2                    ; + offset
  paddw mm3, mm5                    ; + offset
  paddw mm0, mm1                    ; negate back
  paddw mm3, mm4                    ; negate back

   ; saturates to +2047
  movq mm2, [mmx_2047 wrt rip]
  pminsw mm0, mm2
  add rax, 2
  pminsw mm3, mm2

  pxor mm0, mm1
  pxor mm3, mm4
  movq [rdx + 8*rax + 8*16   - 2*8], mm0
  movq [rdx + 8*rax + 8*16+8 - 2*8], mm3
  jnz near .loop

    ; deal with DC
  movd mm0, [rcx]
  pmullw mm0, [rsp-8]		; dscalar
  movq mm2, [mmx_32767_minus_2047 wrt rip]
  paddsw mm0, mm2
  psubsw mm0, mm2
  movq mm2, [mmx_32768_minus_2048 wrt rip]
  psubsw mm0, mm2
  paddsw mm0, mm2
  movd rax, mm0
  mov [rdx], ax

  xor rax, rax
  ret
.endfunc


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_x86_64(int16_t * data,
;                                 const int16_t * const coeff,
;                                 const uint32_t quant,
;                                 const uint16_t *mpeg_matrices);
; Port of the 32bit xmm cousin
;-----------------------------------------------------------------------------

  ; this is the same as dequant_inter_mmx,
  ; except that we're saturating using 'pminsw' (saves 2 cycles/loop)

ALIGN 16
dequant_h263_inter_x86_64:

  mov rax, rdx			; quant
  mov rcx, rsi			; coeff
  mov rdx, rdi			; data

  lea r9, [mmx_add wrt rip]
  movq mm6, [r9 + rax*8 - 8]  ; quant or quant-1
  lea r9, [mmx_mul wrt rip]
  movq mm7, [r9 + rax*8 - 8]  ; 2*quant
  mov rax, -16

ALIGN 16
.loop
  movq mm0, [rcx+8*rax+8*16]      ; c  = coeff[i]
  movq mm3, [rcx+8*rax+8*16 + 8]  ; c' = coeff[i+1]
  pxor mm1, mm1
  pxor mm4, mm4
  pcmpgtw mm1, mm0  ; sign(c)
  pcmpgtw mm4, mm3  ; sign(c')
  pxor mm2, mm2
  pxor mm5, mm5
  pcmpeqw mm2, mm0  ; c is zero
  pcmpeqw mm5, mm3  ; c' is zero
  pandn mm2, mm6    ; offset = isZero ? 0 : quant_add
  pandn mm5, mm6
  pxor mm0, mm1     ; negate if negative
  pxor mm3, mm4     ; negate if negative
  psubw mm0, mm1
  psubw mm3, mm4
  pmullw mm0, mm7   ; *= 2Q
  pmullw mm3, mm7   ; *= 2Q
  paddw mm0, mm2    ; + offset
  paddw mm3, mm5    ; + offset
  paddw mm0, mm1    ; start restoring sign
  paddw mm3, mm4    ; start restoring sign
                            ; saturates to +2047
  movq mm2, [mmx_2047 wrt rip]
  pminsw mm0, mm2
  add rax, 2
  pminsw mm3, mm2

  pxor mm0, mm1 ; finish restoring sign
  pxor mm3, mm4 ; finish restoring sign
  movq [rdx + 8*rax + 8*16   - 2*8], mm0
  movq [rdx + 8*rax + 8*16+8 - 2*8], mm3
  jnz near .loop

  xor rax, rax
  ret
.endfunc

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

