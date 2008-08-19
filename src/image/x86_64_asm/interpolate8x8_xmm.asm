;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - mmx 8x8 block-based halfpel interpolation -
; *
; *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
; *               2002 Pascal Massimino <skal@planet-d.net>
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
; ****************************************************************************/

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
mmx_one:
	times 8 db 1

SECTION .text align=16

cglobal interpolate8x8_halfpel_h_x86_64
cglobal interpolate8x8_halfpel_v_x86_64
cglobal interpolate8x8_halfpel_hv_x86_64

cglobal interpolate8x8_halfpel_add_x86_64
cglobal interpolate8x8_halfpel_h_add_x86_64
cglobal interpolate8x8_halfpel_v_add_x86_64
cglobal interpolate8x8_halfpel_hv_add_x86_64

;===========================================================================
;
; void interpolate8x8_halfpel_h_x86_64(uint8_t * const dst,
;						const uint8_t * const src,
;						const uint32_t stride,
;						const uint32_t rounding);
;
;===========================================================================

%macro COPY_H_SSE_RND0 0
  movq mm0,  [rax]
  pavgb mm0, [rax+1]
  movq mm1,  [rax+rdx]
  pavgb mm1, [rax+rdx+1]
  lea rax,[rax+2*rdx]
  movq [rcx],mm0
  movq [rcx+rdx],mm1
%endmacro

%macro COPY_H_SSE_RND1 0
  movq mm0, [rax]
  movq mm1, [rax+rdx]
  movq mm4, mm0
  movq mm5, mm1
  movq mm2, [rax+1]
  movq mm3, [rax+rdx+1]
  pavgb mm0, mm2
  pxor mm2, mm4
  pavgb mm1, mm3
  lea rax, [rax+2*rdx]
  pxor mm3, mm5
  pand mm2, mm7
  pand mm3, mm7
  psubb mm0, mm2
  movq [rcx], mm0
  psubb mm1, mm3
  movq [rcx+rdx], mm1
%endmacro

ALIGN 16
interpolate8x8_halfpel_h_x86_64:

  mov rax, rcx			; rounding
  mov rcx, rdi			; Dst
  test rax,rax
  mov rax, rsi		; src
  		; rdx is stride

  jnz near .rounding1

  COPY_H_SSE_RND0
  lea rcx,[rcx+2*rdx]
  COPY_H_SSE_RND0
  lea rcx,[rcx+2*rdx]
  COPY_H_SSE_RND0
  lea rcx,[rcx+2*rdx]
  COPY_H_SSE_RND0
  ret

.rounding1
 ; we use: (i+j)/2 = ( i+j+1 )/2 - (i^j)&1
  movq mm7, [mmx_one wrt rip]
  COPY_H_SSE_RND1
  lea rcx, [rcx+2*rdx]
  COPY_H_SSE_RND1
  lea rcx,[rcx+2*rdx]
  COPY_H_SSE_RND1
  lea rcx,[rcx+2*rdx]
  COPY_H_SSE_RND1
  ret
.endfunc

;===========================================================================
;
; void interpolate8x8_halfpel_v_x86_64(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;===========================================================================

%macro COPY_V_SSE_RND0 0
  movq mm0, [rax]
  movq mm1, [rax+rdx]
  pavgb mm0, mm1
  pavgb mm1, [rax+2*rdx]
  lea rax, [rax+2*rdx]
  movq [rcx], mm0
  movq [rcx+rdx],mm1
%endmacro

%macro COPY_V_SSE_RND1 0
  movq mm0, mm2
  movq mm1, [rax]
  movq mm2, [rax+rdx]
  lea rax,[rax+2*rdx]
  movq mm4, mm0
  movq mm5, mm1
  pavgb mm0, mm1
  pxor mm4, mm1
  pavgb mm1, mm2
  pxor mm5, mm2
  pand mm4, mm7         ; lsb's of (i^j)...
  pand mm5, mm7         ; lsb's of (i^j)...
  psubb mm0, mm4        ; ...are substracted from result of pavgb
  movq [rcx], mm0
  psubb mm1, mm5        ; ...are substracted from result of pavgb
  movq [rcx+rdx], mm1
%endmacro

ALIGN 16
interpolate8x8_halfpel_v_x86_64:
  mov rax, rcx			; rounding
  mov rcx, rdi			; Dst
  test rax,rax
  mov rax, rsi			; Src
  		; rdx is stride

  ; we process 2 line at a time
  jnz near .rounding1

  COPY_V_SSE_RND0
  lea rcx, [rcx+2*rdx]
  COPY_V_SSE_RND0
  lea rcx, [rcx+2*rdx]
  COPY_V_SSE_RND0
  lea rcx, [rcx+2*rdx]
  COPY_V_SSE_RND0
  ret

.rounding1
 ; we use: (i+j)/2 = ( i+j+1 )/2 - (i^j)&1
  movq mm7, [mmx_one wrt rip]
  movq mm2, [rax]       ; loop invariant
  add rax, rdx

  COPY_V_SSE_RND1
  lea rcx,[rcx+2*rdx]
  COPY_V_SSE_RND1
  lea rcx,[rcx+2*rdx]
  COPY_V_SSE_RND1
  lea rcx,[rcx+2*rdx]
  COPY_V_SSE_RND1
  ret
.endfunc

;===========================================================================
;
; void interpolate8x8_halfpel_hv_xmm(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

; The trick is to correct the result of 'pavgb' with some combination of the
; lsb's of the 4 input values i,j,k,l, and their intermediate 'pavgb' (s and t).
; The boolean relations are:
;   (i+j+k+l+3)/4 = (s+t+1)/2 - (ij&kl)&st
;   (i+j+k+l+2)/4 = (s+t+1)/2 - (ij|kl)&st
;   (i+j+k+l+1)/4 = (s+t+1)/2 - (ij&kl)|st
;   (i+j+k+l+0)/4 = (s+t+1)/2 - (ij|kl)|st
; with  s=(i+j+1)/2, t=(k+l+1)/2, ij = i^j, kl = k^l, st = s^t.

; Moreover, we process 2 lines at a times, for better overlapping (~15% faster).

%macro COPY_HV_SSE_RND0 0
  lea rax, [rax+rdx]

  movq mm0, [rax]
  movq mm1, [rax+1]

  movq mm6, mm0
  pavgb mm0, mm1    ; mm0=(j+k+1)/2. preserved for next step
  lea rax, [rax+rdx]
  pxor mm1, mm6     ; mm1=(j^k).     preserved for next step

  por mm3, mm1      ; ij |= jk
  movq mm6, mm2
  pxor mm6, mm0     ; mm6 = s^t
  pand mm3, mm6     ; (ij|jk) &= st
  pavgb mm2, mm0    ; mm2 = (s+t+1)/2
  pand mm3, mm7     ; mask lsb
  psubb mm2, mm3    ; apply.

  movq [rcx], mm2

  movq mm2, [rax]
  movq mm3, [rax+1]
  movq mm6, mm2
  pavgb mm2, mm3    ; preserved for next iteration
  lea rcx,[rcx+rdx]
  pxor mm3, mm6     ; preserved for next iteration

  por mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  pand mm1, mm6
  pavgb mm0, mm2

  pand mm1, mm7
  psubb mm0, mm1

  movq [rcx], mm0
%endmacro

%macro COPY_HV_SSE_RND1 0
  lea rax, [rax+rdx]

  movq mm0, [rax]
  movq mm1, [rax+1]

  movq mm6, mm0
  pavgb mm0, mm1    ; mm0=(j+k+1)/2. preserved for next step
  lea rax, [rax+rdx]
  pxor mm1, mm6     ; mm1=(j^k).     preserved for next step

  pand mm3, mm1
  movq mm6, mm2
  pxor mm6, mm0
  por mm3, mm6
  pavgb mm2, mm0
  pand mm3, mm7
  psubb mm2, mm3

  movq [rcx], mm2

  movq mm2, [rax]
  movq mm3, [rax+1]
  movq mm6, mm2
  pavgb mm2, mm3     ; preserved for next iteration
  lea rcx,[rcx+rdx]
  pxor mm3, mm6      ; preserved for next iteration

  pand mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  por mm1, mm6
  pavgb mm0, mm2
  pand mm1, mm7
  psubb mm0, mm1

  movq [rcx], mm0
%endmacro

ALIGN 16
interpolate8x8_halfpel_hv_x86_64:
  mov rax, rcx		; rounding
  mov rcx, rdi		; Dst
  test rax, rax
  mov rax, rsi		; Src
  		; rdx is stride

  movq mm7, [mmx_one wrt rip]

    ; loop invariants: mm2=(i+j+1)/2  and  mm3= i^j
  movq mm2, [rax]
  movq mm3, [rax+1]
  movq mm6, mm2
  pavgb mm2, mm3
  pxor mm3, mm6       ; mm2/mm3 ready

  jnz near .rounding1

  COPY_HV_SSE_RND0
  add rcx, rdx
  COPY_HV_SSE_RND0
  add rcx, rdx
  COPY_HV_SSE_RND0
  add rcx, rdx
  COPY_HV_SSE_RND0
  ret

.rounding1
  COPY_HV_SSE_RND1
  add rcx, rdx
  COPY_HV_SSE_RND1
  add rcx, rdx
  COPY_HV_SSE_RND1
  add rcx, rdx
  COPY_HV_SSE_RND1
  ret
.endfunc

;===========================================================================
;
; The next functions combine both source halfpel interpolation step and the
; averaging (with rouding) step to avoid wasting memory bandwidth computing
; intermediate halfpel images and then averaging them.
;
;===========================================================================

%macro PROLOG0 0
; rcx fourth
; rdx third
; r8 fifth
  mov r8, rdx  ; saves rounding
  mov rcx, rdi ; Dst
  mov rax, rsi ; Src
  ; rdx is stride
%endmacro
%macro PROLOG1 0
  PROLOG0
  test r8, 1; Rounding?
%endmacro
%macro EPILOG 0
  ret
%endmacro

;===========================================================================
;
; void interpolate8x8_halfpel_add_xmm(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_FF 2
    movq mm0,  [rax+%1]
    movq mm1,  [rax+%2]
    pavgb mm0, [rcx+%1]
    pavgb mm1, [rcx+%2]
    movq [rcx+%1], mm0
    movq [rcx+%2], mm1
%endmacro

ALIGN 16
interpolate8x8_halfpel_add_x86_64:  ; 23c
  PROLOG1
  ADD_FF 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FF 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FF 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FF 0, rdx
  EPILOG
.endfunc

;===========================================================================
;
; void interpolate8x8_halfpel_h_add_xmm(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================


%macro ADD_FH_RND0 2
    movq mm0,  [rax+%1]
    movq mm1,  [rax+%2]
    pavgb mm0, [rax+%1+1]
    pavgb mm1, [rax+%2+1]
    pavgb mm0, [rcx+%1]
    pavgb mm1, [rcx+%2]
    movq [rcx+%1],mm0
    movq [rcx+%2],mm1
%endmacro

%macro ADD_FH_RND1 2
    movq mm0,  [rax+%1]
    movq mm1,  [rax+%2]
    movq mm4, mm0
    movq mm5, mm1
    movq mm2, [rax+%1+1]
    movq mm3, [rax+%2+1]
    pavgb mm0, mm2
    ; lea ??
    pxor mm2, mm4
    pavgb mm1, mm3
    pxor mm3, mm5
    pand mm2, [mmx_one wrt rip]
    pand mm3, [mmx_one wrt rip]
    psubb mm0, mm2
    psubb mm1, mm3
    pavgb mm0, [rcx+%1]
    pavgb mm1, [rcx+%2]
    movq [rcx+%1],mm0
    movq [rcx+%2],mm1
%endmacro

ALIGN 16
interpolate8x8_halfpel_h_add_x86_64:   ; 32c
  PROLOG1
  jnz near .Loop1
  ADD_FH_RND0 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND0 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND0 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND0 0, rdx
  EPILOG

.Loop1
  ; we use: (i+j)/2 = ( i+j+1 )/2 - (i^j)&1
  ; movq mm7, [mmx_one wrt rip]
  ADD_FH_RND1 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND1 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND1 0, rdx
  lea rax,[rax+2*rdx]
  lea rcx,[rcx+2*rdx]
  ADD_FH_RND1 0, rdx
  EPILOG
.endfunc


;===========================================================================
;
; void interpolate8x8_halfpel_v_add_x86_64(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_8_HF_RND0 0
  movq mm0,  [rax]
  movq mm1,  [rax+rdx]
  pavgb mm0, mm1
  pavgb mm1, [rax+2*rdx]
  lea rax,[rax+2*rdx]
  pavgb mm0, [rcx]
  pavgb mm1, [rcx+rdx]
  movq [rcx],mm0 
  movq [rcx+rdx],mm1
%endmacro

%macro ADD_8_HF_RND1 0
  movq mm1, [rax+rdx]
  movq mm2, [rax+2*rdx]
  lea rax,[rax+2*rdx]
  movq mm4, mm0
  movq mm5, mm1
  pavgb mm0, mm1
  pxor mm4, mm1  
  pavgb mm1, mm2
  pxor mm5, mm2
  pand mm4, mm7    ; lsb's of (i^j)...
  pand mm5, mm7    ; lsb's of (i^j)...
  psubb mm0, mm4 ; ...are substracted from result of pavgb
  pavgb mm0, [rcx]
  movq [rcx], mm0
  psubb mm1, mm5 ; ...are substracted from result of pavgb
  pavgb mm1, [rcx+rdx]
  movq [rcx+rdx], mm1
%endmacro

ALIGN 16
interpolate8x8_halfpel_v_add_x86_64:
  PROLOG1

  jnz near .Loop1
  pxor mm7, mm7   ; this is a NOP

  ADD_8_HF_RND0
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND0
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND0
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND0
  EPILOG

.Loop1
  movq mm0, [rax] ; loop invariant
  movq mm7, [mmx_one wrt rip]

  ADD_8_HF_RND1 
  movq mm0, mm2
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND1 
  movq mm0, mm2
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND1 
  movq mm0, mm2
  lea rcx,[rcx+2*rdx]
  ADD_8_HF_RND1 
  EPILOG
.endfunc

; The trick is to correct the result of 'pavgb' with some combination of the
; lsb's of the 4 input values i,j,k,l, and their intermediate 'pavgb' (s and t).
; The boolean relations are:
;   (i+j+k+l+3)/4 = (s+t+1)/2 - (ij&kl)&st
;   (i+j+k+l+2)/4 = (s+t+1)/2 - (ij|kl)&st
;   (i+j+k+l+1)/4 = (s+t+1)/2 - (ij&kl)|st
;   (i+j+k+l+0)/4 = (s+t+1)/2 - (ij|kl)|st
; with  s=(i+j+1)/2, t=(k+l+1)/2, ij = i^j, kl = k^l, st = s^t.

; Moreover, we process 2 lines at a times, for better overlapping (~15% faster).

;===========================================================================
;
; void interpolate8x8_halfpel_hv_add_x86_64(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_HH_RND0 0
  lea rax,[rax+rdx]

  movq mm0, [rax]
  movq mm1, [rax+1]

  movq mm6, mm0
  pavgb mm0, mm1  ; mm0=(j+k+1)/2. preserved for next step
  lea rax,[rax+rdx]
  pxor mm1, mm6   ; mm1=(j^k).     preserved for next step

  por mm3, mm1    ; ij |= jk
  movq mm6, mm2
  pxor mm6, mm0   ; mm6 = s^t
  pand mm3, mm6   ; (ij|jk) &= st
  pavgb mm2, mm0  ; mm2 = (s+t+1)/2
  pand mm3, mm7   ; mask lsb
  psubb mm2, mm3  ; apply.

  pavgb mm2, [rcx]
  movq [rcx], mm2

  movq mm2, [rax]
  movq mm3, [rax+1]
  movq mm6, mm2
  pavgb mm2, mm3  ; preserved for next iteration
  lea rcx,[rcx+rdx]
  pxor mm3, mm6   ; preserved for next iteration

  por mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  pand mm1, mm6
  pavgb mm0, mm2

  pand mm1, mm7 
  psubb mm0, mm1

  pavgb mm0, [rcx]
  movq [rcx], mm0
%endmacro

%macro ADD_HH_RND1 0
  lea rax,[rax+rdx]

  movq mm0, [rax]
  movq mm1, [rax+1]

  movq mm6, mm0
  pavgb mm0, mm1  ; mm0=(j+k+1)/2. preserved for next step
  lea rax,[rax+rdx]
  pxor mm1, mm6   ; mm1=(j^k).     preserved for next step

  pand mm3, mm1
  movq mm6, mm2
  pxor mm6, mm0
  por mm3, mm6
  pavgb mm2, mm0
  pand mm3, mm7
  psubb mm2, mm3

  pavgb mm2, [rcx]
  movq [rcx], mm2

  movq mm2, [rax]
  movq mm3, [rax+1]
  movq mm6, mm2
  pavgb mm2, mm3  ; preserved for next iteration
  lea rcx,[rcx+rdx]
  pxor mm3, mm6   ; preserved for next iteration

  pand mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  por mm1, mm6
  pavgb mm0, mm2
  pand mm1, mm7
  psubb mm0, mm1

  pavgb mm0, [rcx]
  movq [rcx], mm0
%endmacro

ALIGN 16
interpolate8x8_halfpel_hv_add_x86_64:
  PROLOG1

  movq mm7, [mmx_one wrt rip]

    ; loop invariants: mm2=(i+j+1)/2  and  mm3= i^j
  movq mm2, [rax] 
  movq mm3, [rax+1]
  movq mm6, mm2   
  pavgb mm2, mm3
  pxor mm3, mm6   ; mm2/mm3 ready

  jnz near .Loop1

  ADD_HH_RND0
  add rcx, rdx
  ADD_HH_RND0
  add rcx, rdx
  ADD_HH_RND0
  add rcx, rdx
  ADD_HH_RND0
  EPILOG

.Loop1
  ADD_HH_RND1
  add rcx, rdx
  ADD_HH_RND1
  add rcx, rdx
  ADD_HH_RND1
  add rcx, rdx
  ADD_HH_RND1

  EPILOG
.endfunc

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

