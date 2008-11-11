;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 3dne Quantization/Dequantization -
; *
; *  Copyright (C) 2002-2003 Peter Ross <pross@xvid.org>
; *                2002      Jaan Kalda
; *		   2004 Andre Werthmann <wertmann@aei.mpg.de>
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
; * $Id: quantize_mpeg_xmm.asm,v 1.3 2008-11-11 20:46:24 Isibaar Exp $
; *
; ***************************************************************************/

; _3dne functions are compatible with iSSE, but are optimized specifically
; for K7 pipelines

%define SATURATE

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
; Local data
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

ALIGN 8
mmzero:
	dd 0,0
mmx_one:
	times 4 dw 1

;-----------------------------------------------------------------------------
; divide by 2Q table
;-----------------------------------------------------------------------------

ALIGN 16
mmx_divs:		;i>2
%assign i 1
%rep 31
	times 4 dw  ((1 << 15) / i + 1)
	%assign i i+1
%endrep

ALIGN 16
mmx_div:		;quant>2
	times 4 dw 65535 ; the div by 2 formula will overflow for the case
	                 ; quant=1 but we don't care much because quant=1
	                 ; is handled by a different piece of code that
	                 ; doesn't use this table.
%assign quant 2
%rep 31
	times 4 dw  ((1 << 16) / quant + 1)
	%assign quant quant+1
%endrep

%macro FIXX 1
dw (1 << 16) / (%1) + 1
%endmacro

%define nop4	db	08Dh, 074h, 026h,0
%define nop3	add	esp, byte 0
%define nop2	mov	esp, esp
%define nop7	db	08dh, 02ch, 02dh,0,0,0,0
%define nop6	add	ebp, dword 0

;-----------------------------------------------------------------------------
; quantd table
;-----------------------------------------------------------------------------

%define VM18P	3
%define VM18Q	4

ALIGN 16
quantd:
%assign i 1
%rep 31
	times 4 dw  (((VM18P*i) + (VM18Q/2)) / VM18Q)
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; multiple by 2Q table
;-----------------------------------------------------------------------------

ALIGN 16
mmx_mul_quant:
%assign i 1
%rep 31
	times 4 dw  i
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; saturation limits
;-----------------------------------------------------------------------------

ALIGN 16
mmx_32767_minus_2047:
	times 4 dw (32767-2047)
mmx_32768_minus_2048:
	times 4 dw (32768-2048)
mmx_2047:
	times 4 dw 2047
mmx_minus_2048:
	times 4 dw (-2048)
zero:
	times 4 dw 0

int_div:
dd 0
%assign i 1
%rep 255
	dd  (1 << 17) / ( i) + 1
	%assign i i+1
%endrep

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal quant_mpeg_intra_x86_64
cglobal quant_mpeg_inter_x86_64
cglobal dequant_mpeg_intra_x86_64
cglobal dequant_mpeg_inter_x86_64

;-----------------------------------------------------------------------------
;
; uint32_t quant_mpeg_intra_x86_64(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint32_t dcscalar,
;                               const uint16_t *mpeg_matrices);
; Ported from its 32bit xmm cousin
;-----------------------------------------------------------------------------

ALIGN 16
quant_mpeg_intra_x86_64:
  mov rax, rsi			; data
  mov r9, rcx			; save dcscalar
  mov rcx, rdx			; quant
  mov rdx, rdi			; coeff

  push rbx
  
  mov rdi, r8				; mpeg_quant_matrices

  mov rsi, -14
  pxor mm0, mm0
  pxor mm3, mm3
  cmp rcx, byte 1
  je near .q1loop
  cmp rcx, byte 19
  jg near .lloop

ALIGN 16
.loop:
  movq mm1, [rax + 8*rsi+112]   ; mm0 = [1st]
  psubw mm0, mm1                ;-mm1
  movq mm4, [rax + 8*rsi + 120] ;
  psubw mm3, mm4                ;-mm4
  pmaxsw mm0, mm1               ;|src|
  pmaxsw mm3,mm4
;  nop2
  psraw mm1, 15     ;sign src
  psraw mm4, 15
  psllw mm0, 4      ;level << 4 ;
  psllw mm3, 4
  paddw mm0, [rdi + 128 + 8*rsi+112]
  paddw mm3, [rdi + 128 + 8*rsi+120]
  movq mm5, [rdi + 384 + 8*rsi+112]
  movq mm7, [rdi + 384 + 8*rsi+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3
;  mov esp, esp
  movq mm2, [rdi + 8*rsi+112]
  movq mm6, [rdi + 8*rsi+120]
  pmullw mm2, mm5
  pmullw mm6, mm7
  psubw mm0, mm2
  psubw mm3, mm6
;  nop4
  lea r11, [quantd wrt rip]
  movq mm2, [r11 + rcx * 8 - 8]
  lea r11, [mmx_divs wrt rip]
  movq mm6, [r11 + rcx * 8 - 8]
  paddw mm5, mm2
  paddw mm7, mm2
;  mov esp, esp
  pmulhuw mm0, [rdi + 256 + 8*rsi+112]
  pmulhuw mm3, [rdi + 256 + 8*rsi+120]
  paddw mm5, mm0
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level + quantd) / quant (0<quant<32)
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [rdx + 8*rsi+112], mm5
  movq [rdx + 8*rsi +120], mm7
  add rsi, byte 2
  jng near .loop

.done:
; calculate  data[0] // (int32_t)dcscalar)
;  mov esi, [esp + 12 + 16]  ; dcscalar
  mov rsi, r9			; dcscalar
  movsx rcx, word [rax]
  mov rdi, rcx
;  mov edx, [esp + 12 + 16]
  mov r11, rdx		; save rdx
  mov rdx, r9		;
  shr edx, 1            ; ebx = dcscalar /2
  sar edi, 31           ; cdq is vectorpath
  xor edx, edi          ; ebx = eax V -eax -1
  sub ecx, edi
  add ecx, edx
;;  mov rdx, [dword esp + 12 + 4]
  mov rdx, r11		; restore rdx
  lea r11, [int_div wrt rip]
  mov rsi, [r11+4*rsi]
  imul ecx, esi
  sar ecx, 17
  lea rbx, [byte rcx + 1]
  cmovs rcx, rbx
  ; idiv    cx          ; ecx = edi:ecx / dcscalar

;  mov ebx, [esp]
;  mov edi, [esp+4]
;  mov esi, [esp+8]
;  add esp, byte 12	; pops...
  pop rbx
;  mov [rdx], rcx     ; coeff[0] = ax
  mov [rdx], cx		; coeff[0] = cx

  xor rax, rax
  ret

ALIGN 16
.q1loop:
  movq mm1, [rax + 8*rsi+112]               ; mm0 = [1st]
  psubw mm0, mm1                            ;-mm1
  movq mm4, [rax + 8*rsi+120]               ;
  psubw mm3, mm4                            ;-mm4
  pmaxsw mm0, mm1                           ;|src|
  pmaxsw mm3, mm4
;  nop2
  psraw mm1, 15                             ;sign src
  psraw mm4, 15
  psllw mm0, 4                              ; level << 4
  psllw mm3, 4
  paddw mm0, [rdi + 128 + 8*rsi+112]    ;mm0 is to be divided
  paddw mm3, [rdi + 128 + 8*rsi+120]    ;intra1 contains fix for division by 1
  movq mm5, [rdi + 384 + 8*rsi+112] ;with rounding down
  movq mm7, [rdi + 384 + 8*rsi+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3      ;mm7: first approx of division
;  mov esp, esp
  movq mm2, [rdi + 8*rsi+112]
  movq mm6, [rdi + 8*rsi+120]      ; divs for q<=16
  pmullw mm2, mm5       ;test value <= original
  pmullw mm6, mm7
  psubw mm0, mm2        ;mismatch
  psubw mm3, mm6
;  nop4
  lea r11, [quantd wrt rip]
  movq mm2, [r11 + rcx * 8 - 8]
  paddw mm5, mm2        ;first approx with quantd
  paddw mm7, mm2
;  mov esp, esp
  pmulhuw mm0, [rdi + 256 + 8*rsi+112]   ;correction
  pmulhuw mm3, [rdi + 256 + 8*rsi+120]
  paddw mm5, mm0        ;final result with quantd
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
;  mov esp, esp
  psrlw mm5, 1          ;  (level + quantd) /2  (quant = 1)
  psrlw mm7, 1
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [rdx + 8*rsi+112], mm5
  movq [rdx + 8*rsi +120], mm7
  add rsi, byte 2
  jng near .q1loop
  jmp near .done

ALIGN 8
.lloop:
  movq mm1, [rax + 8*rsi+112]       ; mm0 = [1st]
  psubw mm0, mm1        ;-mm1
  movq mm4, [rax + 8*rsi+120]
  psubw mm3, mm4        ;-mm4
  pmaxsw mm0, mm1       ;|src|
  pmaxsw mm3, mm4
;  nop2
  psraw mm1, 15         ;sign src
  psraw mm4, 15
  psllw mm0, 4          ; level << 4
  psllw mm3, 4          ;
  paddw mm0, [rdi + 128 + 8*rsi+112] ;mm0 is to be divided intra1 contains fix for division by 1
  paddw mm3, [rdi + 128 + 8*rsi+120]
  movq mm5, [rdi + 384 + 8*rsi+112]
  movq mm7, [rdi + 384 + 8*rsi+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3      ;mm7: first approx of division
;  mov esp, esp
  movq mm2, [rdi + 8*rsi+112]
  movq mm6, [rdi + 8*rsi+120]
  pmullw mm2, mm5       ;test value <= original
  pmullw mm6, mm7
  psubw mm0, mm2        ;mismatch
  psubw mm3, mm6
;  nop4
  lea r11, [quantd wrt rip]
  movq mm2, [r11 + rcx * 8 - 8]
  lea r11, [mmx_div wrt rip]
  movq mm6, [r11 + rcx * 8 - 8] ; divs for q<=16
  paddw mm5, mm2        ;first approx with quantd
  paddw mm7, mm2
;  mov esp, esp
  pmulhuw mm0, [rdi + 256 + 8*rsi+112] ;correction
  pmulhuw mm3, [rdi + 256 + 8*rsi+120]
  paddw mm5, mm0        ;final result with quantd
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
;  mov esp, esp
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level + quantd) / quant (0<quant<32)
  psrlw mm5, 1          ; (level + quantd) / (2*quant)
  psrlw mm7, 1
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [rdx + 8*rsi+112], mm5
  movq [rdx + 8*rsi +120], mm7
  add rsi,byte 2
  jng near .lloop
  jmp near .done
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t quant_mpeg_inter_x86_64(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint16_t *mpeg_matrices);
; Ported from its 32bit xmm cousin
;-----------------------------------------------------------------------------

ALIGN 16
quant_mpeg_inter_x86_64:
  mov rax, rsi			; data
  mov r8, rdi			; save coeff
  mov rdi, rcx			; mpeg_matrices
  mov rcx, rdx			; quant
  mov rdx, r8			; coeff

  push rbx

  mov rsi, -14
  mov rbx, rsp
  sub rsp, byte 24	; 16 would be enough, but it isn't important
  lea rbx, [rsp+8]
  and rbx, byte -8 ;ALIGN 8
  pxor mm0, mm0
  pxor mm3, mm3
  movq [byte rbx],mm0
  movq [rbx+8],mm0
  cmp rcx, byte 1
  je near .q1loop
  cmp rcx, byte 19
  jg near .lloop

ALIGN 16
.loop:
  movq mm1, [rax + 8*rsi+112]       ; mm0 = [1st]
  psubw mm0, mm1 ;-mm1
  movq mm4, [rax + 8*rsi + 120] ;
  psubw mm3, mm4 ;-mm4
  pmaxsw mm0, mm1 ;|src|
  pmaxsw mm3, mm4
;  nop2
  psraw mm1, 15         ;sign src
  psraw mm4, 15
  psllw mm0, 4          ; level << 4
  psllw mm3, 4          ;
  paddw mm0, [rdi + 640 + 8*rsi+112]
  paddw mm3, [rdi + 640 + 8*rsi+120]
  movq mm5, [rdi + 896 + 8*rsi+112]
  movq mm7, [rdi + 896 + 8*rsi+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3
;  mov esp, esp
  movq mm2, [rdi + 512 + 8*rsi+112]
  movq mm6, [rdi + 512 + 8*rsi+120]
  pmullw mm2, mm5
  pmullw mm6, mm7
  psubw mm0, mm2
  psubw mm3, mm6
  movq mm2, [byte rbx]
  lea r11, [mmx_divs wrt rip]
  movq mm6, [r11 + rcx * 8 - 8]
  pmulhuw mm0, [rdi + 768 + 8*rsi+112]
  pmulhuw mm3, [rdi + 768 + 8*rsi+120]
  paddw mm2, [rbx+8]    ;sum
  paddw mm5, mm0
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level ) / quant (0<quant<32)
  add rsi, byte 2
  paddw mm2, mm5        ;sum += x1
  movq [rbx], mm7       ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
;  db 0Fh, 7Fh, 54h, 23h, 08 ;movq   [ebx+8],mm2 ;store sum
  movq [rbx+8], mm2 ;store sum
  movq [rdx + 8*rsi+112-16], mm5
  movq [rdx + 8*rsi +120-16], mm7
  jng near .loop

.done
; calculate  data[0] // (int32_t)dcscalar)
  paddw mm2, [rbx]
  add rsp, byte 24
  pop rbx
  pmaddwd mm2, [mmx_one wrt rip]
  punpckldq mm0, mm2 ;get low dw to mm0:high
  paddd mm0,mm2
  punpckhdq mm0, mm0 ;get result to low
  movd rax, mm0

  ret

ALIGN 16
.q1loop:
  movq mm1, [rax + 8*rsi+112]       ; mm0 = [1st]
  psubw mm0, mm1                    ;-mm1
  movq mm4, [rax + 8*rsi+120]
  psubw mm3, mm4                    ;-mm4
  pmaxsw mm0, mm1                   ;|src|
  pmaxsw mm3, mm4
;  nop2
  psraw mm1, 15                             ; sign src
  psraw mm4, 15
  psllw mm0, 4                              ; level << 4
  psllw mm3, 4
  paddw mm0, [rdi + 640 + 8*rsi+112]    ;mm0 is to be divided
  paddw mm3, [rdi + 640 + 8*rsi+120]    ; inter1 contains fix for division by 1
  movq mm5, [rdi + 896 + 8*rsi+112] ;with rounding down
  movq mm7, [rdi + 896 + 8*rsi+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3                          ;mm7: first approx of division
;  mov esp, esp
  movq mm2, [rdi + 512 + 8*rsi+112]
  movq mm6, [rdi + 512 + 8*rsi+120]      ; divs for q<=16
  pmullw mm2, mm5                           ;test value <= original
  pmullw mm6, mm7
  psubw mm0, mm2                            ;mismatch
  psubw mm3, mm6
  movq mm2, [byte rbx]
  pmulhuw mm0, [rdi + 768 + 8*rsi+112]  ;correction
  pmulhuw mm3, [rdi + 768 + 8*rsi+120]
  paddw mm2, [rbx+8]    ;sum
  paddw mm5, mm0        ;final result
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
  psrlw mm5, 1          ;  (level ) /2  (quant = 1)
  psrlw mm7, 1
  add rsi, byte 2
  paddw mm2, mm5        ;sum += x1
  movq [rbx], mm7       ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [rbx+8], mm2     ;store sum
  movq [rdx + 8*rsi+112-16], mm5
  movq [rdx + 8*rsi +120-16], mm7
  jng near .q1loop
  jmp near .done

ALIGN 8
.lloop:
  movq mm1, [rax + 8*rsi+112]       ; mm0 = [1st]
  psubw mm0,mm1         ;-mm1
  movq mm4, [rax + 8*rsi+120]
  psubw mm3,mm4         ;-mm4
  pmaxsw mm0,mm1        ;|src|
  pmaxsw mm3,mm4
;  nop2
  psraw mm1,15          ;sign src
  psraw mm4,15
  psllw mm0, 4          ; level << 4
  psllw mm3, 4          ;
  paddw mm0, [rdi + 640 + 8*rsi+112] ;mm0 is to be divided inter1 contains fix for division by 1
  paddw mm3, [rdi + 640 + 8*rsi+120]
  movq mm5,[rdi + 896 + 8*rsi+112]
  movq mm7,[rdi + 896 + 8*rsi+120]
  pmulhuw mm5,mm0
  pmulhuw mm7,mm3       ;mm7: first approx of division
;  mov esp,esp
  movq mm2,[rdi + 512 + 8*rsi+112]
  movq mm6,[rdi + 512 + 8*rsi+120]
  pmullw mm2,mm5        ;test value <= original
  pmullw mm6,mm7
  psubw mm0,mm2         ;mismatch
  psubw mm3,mm6
  movq mm2,[byte rbx]
  lea r11, [mmx_div wrt rip]
  movq mm6,[r11 + rcx * 8 - 8]  ; divs for q<=16
  pmulhuw mm0,[rdi + 768 + 8*rsi+112] ;correction
  pmulhuw mm3,[rdi + 768 + 8*rsi+120]
  paddw mm2,[rbx+8]     ;sum
  paddw mm5,mm0         ;final result
  paddw mm7,mm3
  pxor mm0,mm0
  pxor mm3,mm3
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level ) / quant (0<quant<32)
  add rsi,byte 2
  psrlw mm5, 1          ; (level ) / (2*quant)
  paddw mm2,mm5         ;sum += x1
  psrlw mm7, 1
  movq [rbx],mm7        ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
;  db 0Fh, 7Fh, 54h, 23h, 08 ;movq   [ebx+8],mm2 ;store sum
  movq [rbx+8], mm2 ;store sum
  movq [rdx + 8*rsi+112-16], mm5
  movq [rdx + 8*rsi +120-16], mm7
  jng near .lloop
  jmp near .done
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t dequant_mpeg_intra_x86_64(int16_t *data,
;                                  const int16_t const *coeff,
;                                  const uint32_t quant,
;                                  const uint32_t dcscalar,
;                                  const uint16_t *mpeg_matrices);
; Ported from the 32bit 3dne cousin
;-----------------------------------------------------------------------------

  ;   Note: in order to saturate 'easily', we pre-shift the quantifier
  ; by 4. Then, the high-word of (coeff[]*matrix[i]*quant) are used to
  ; build a saturating mask. It is non-zero only when an overflow occured.
  ; We thus avoid packing/unpacking toward double-word.
  ; Moreover, we perform the mult (matrix[i]*quant) first, instead of, e.g.,
  ; (coeff[i]*matrix[i]). This is less prone to overflow if coeff[] are not
  ; checked. Input ranges are: coeff in [-127,127], inter_matrix in [1..255],a
  ; and quant in [1..31].
  ;

%macro DEQUANT4INTRAMMX 1
  movq mm1, [byte rcx+ 16 * %1] ; mm0 = c  = coeff[i]
  movq mm4, [rcx+ 16 * %1 +8]   ; mm3 = c' = coeff[i+1]
  psubw mm0, mm1
  psubw mm3, mm4
  pmaxsw mm0, mm1
  pmaxsw mm3, mm4
  psraw mm1, 15
  psraw mm4, 15
%if %1
  movq mm2, [rsp-16]
  movq mm7, [rsp-16]
%endif
  pmullw mm2, [rdi + 16 * %1 ]     ; matrix[i]*quant
  pmullw mm7, [rdi + 16 * %1 +8]   ; matrix[i+1]*quant
  movq mm5, mm0
  movq mm6, mm3
  pmulhw mm0, mm2   ; high of coeff*(matrix*quant)
  pmulhw mm3, mm7   ; high of coeff*(matrix*quant)
  pmullw mm2, mm5   ; low  of coeff*(matrix*quant)
  pmullw mm7, mm6   ; low  of coeff*(matrix*quant)
  pcmpgtw mm0, [rsp-8]
  pcmpgtw mm3, [rsp-8]
  paddusw mm2, mm0
  paddusw mm7, mm3
  psrlw mm2, 5
  psrlw mm7, 5
  pxor mm2, mm1     ; start negating back
  pxor mm7, mm4     ; start negating back
  psubusw mm1, mm0
  psubusw mm4, mm3
  movq mm0, [rsp-8]
  movq mm3, [rsp-8]
  psubw mm2, mm1    ; finish negating back
  psubw mm7, mm4    ; finish negating back
  movq [byte rdx + 16 * %1], mm2   ; data[i]
  movq [rdx + 16 * %1  +8], mm7   ; data[i+1]
%endmacro

ALIGN 16
dequant_mpeg_intra_x86_64:
  mov rax, rdx		; quant
  mov rdx, rdi		; data
  mov r9, rcx		; dcscalar
  mov rcx, rsi		; coeff

  lea r11, [mmx_mul_quant wrt rip]
  movq mm7, [r11  + rax*8 - 8]
  psllw mm7, 2      ; << 2. See comment.
  push rbx

  movsx ebx, word [rcx]
  pxor mm0, mm0
  pxor mm3, mm3

  movq [rsp-8], mm0
  movq [rsp-16], mm7

  imul ebx, r9d
  movq mm2, mm7
  mov rdi, r8				; mpeg_quant_matrices
ALIGN 4

  DEQUANT4INTRAMMX 0

  mov esi, -2048
;  nop
  cmp ebx, esi

  DEQUANT4INTRAMMX 1

  cmovl ebx, esi
  neg esi
  sub esi, byte 1 ;2047

  DEQUANT4INTRAMMX 2

  cmp ebx, esi
  cmovg ebx, esi

  DEQUANT4INTRAMMX 3

  mov [byte rdx], bx

  DEQUANT4INTRAMMX 4
  DEQUANT4INTRAMMX 5
  DEQUANT4INTRAMMX 6
  DEQUANT4INTRAMMX 7

  pop rbx

  xor rax, rax
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t dequant_mpeg_inter_3dne(int16_t * data,
;                                  const int16_t * const coeff,
;                                  const uint32_t quant,
;                                  const uint16_t *mpeg_matrices);
; Ported from 32bit 3dne cousin
;-----------------------------------------------------------------------------

    ; Note:  We use (2*c + sgn(c) - sgn(-c)) as multiplier
    ; so we handle the 3 cases: c<0, c==0, and c>0 in one shot.
    ; sgn(x) is the result of 'pcmpgtw 0,x':  0 if x>=0, -1 if x<0.
    ; It's mixed with the extraction of the absolute value.

ALIGN 16
dequant_mpeg_inter_x86_64:
  mov rax, rdx			; quant
  mov rdx, rdi			; data
  mov rdi, rcx			; mpeg_matrices
  mov rcx, rsi			; coeff

  lea r11, [mmx_mul_quant wrt rip]
  movq mm7, [r11  + rax*8 - 8]
  mov rax, -14
  paddw mm7, mm7    ; << 1
  pxor mm6, mm6     ; mismatch sum
  pxor mm1, mm1
  pxor mm3, mm3

ALIGN 16
.loop:
  movq mm0, [rcx+8*rax + 7*16   ]   ; mm0 = coeff[i]
  pcmpgtw mm1, mm0  ; mm1 = sgn(c)    (preserved)
  movq mm2, [rcx+8*rax + 7*16 +8]   ; mm2 = coeff[i+1]
  pcmpgtw mm3, mm2  ; mm3 = sgn(c')   (preserved)
  paddsw mm0, mm1   ; c += sgn(c)
  paddsw mm2, mm3   ; c += sgn(c')
  paddw mm0, mm0    ; c *= 2
  paddw mm2, mm2    ; c'*= 2

  movq mm4, [mmzero wrt rip]
  movq mm5, [mmzero wrt rip]
  psubw mm4, mm0    ; -c
  psubw mm5, mm2    ; -c'

  psraw mm4, 16     ; mm4 = sgn(-c)
  psraw mm5, 16     ; mm5 = sgn(-c')
  psubsw mm0, mm4   ; c  -= sgn(-c)
  psubsw mm2, mm5   ; c' -= sgn(-c')
  pxor mm0, mm1     ; finish changing sign if needed
  pxor mm2, mm3     ; finish changing sign if needed

 ; we're short on register, here. Poor pairing...

  movq mm4, mm7     ; (matrix*quant)
;  nop
  pmullw mm4, [rdi + 512 + 8*rax + 7*16]
  movq mm5, mm4
  pmulhw mm5, mm0   ; high of c*(matrix*quant)
  pmullw mm0, mm4   ; low  of c*(matrix*quant)

  movq mm4, mm7     ; (matrix*quant)
  pmullw mm4, [rdi + 512 + 8*rax + 7*16 + 8]
  add rax, byte 2

  pcmpgtw mm5, [mmzero wrt rip]
  paddusw mm0, mm5
  psrlw mm0, 5
  pxor mm0, mm1     ; start restoring sign
  psubusw mm1, mm5

  movq mm5, mm4
  pmulhw mm5, mm2   ; high of c*(matrix*quant)
  pmullw mm2, mm4   ; low  of c*(matrix*quant)
  psubw mm0, mm1    ; finish restoring sign

  pcmpgtw mm5, [mmzero wrt rip]
  paddusw mm2, mm5
  psrlw mm2, 5
  pxor mm2, mm3     ; start restoring sign
  psubusw mm3, mm5
  psubw mm2, mm3    ; finish restoring sign
  movq mm1, [mmzero wrt rip]
  movq mm3, [byte mmzero wrt rip]
  pxor mm6, mm0                             ; mismatch control
  movq [rdx + 8*rax + 7*16 -2*8   ], mm0    ; data[i]
  pxor mm6, mm2                             ; mismatch control
  movq [rdx + 8*rax + 7*16 -2*8 +8], mm2    ; data[i+1]

  jng .loop
;  nop

 ; mismatch control

  pshufw mm0, mm6, 01010101b
  pshufw mm1, mm6, 10101010b
  pshufw mm2, mm6, 11111111b
  pxor mm6, mm0
  pxor mm1, mm2
  pxor mm6, mm1
  movd rax, mm6
  and rax, byte 1
  xor rax, byte 1
  xor word [rdx + 2*63], ax

  xor rax, rax
  ret
ENDFUNC

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

