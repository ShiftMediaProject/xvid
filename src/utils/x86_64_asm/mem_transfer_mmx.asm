;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 8<->16 bit transfer functions -
; *
; *  Copyright (C) 2001 Peter Ross <pross@xvid.org>
; *                2001 Michael Militzer <isibaar@xvid.org>
; *                2002 Pascal Massimino <skal@planet-d.net>
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
; * $Id: mem_transfer_mmx.asm,v 1.1 2005-01-05 23:02:15 edgomez Exp $
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

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

cglobal transfer_8to16copy_x86_64
cglobal transfer_16to8copy_x86_64
cglobal transfer_8to16sub_x86_64
cglobal transfer_8to16subro_x86_64
cglobal transfer_8to16sub2_x86_64
cglobal transfer_8to16sub2ro_x86_64
cglobal transfer_16to8add_x86_64
cglobal transfer8x8_copy_x86_64

;-----------------------------------------------------------------------------
;
; void transfer_8to16copy_x86_64(int16_t * const dst,
;							const uint8_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16 1
  movq mm0, [rax]
  movq mm1, [rax+rdx]
  movq mm2, mm0
  movq mm3, mm1
  punpcklbw mm0, mm7
  movq [rcx+%1*32], mm0
  punpcklbw mm1, mm7
  movq [rcx+%1*32+16], mm1
  punpckhbw mm2, mm7
  punpckhbw mm3, mm7
  lea rax, [rax+2*rdx]
  movq [rcx+%1*32+8], mm2
  movq [rcx+%1*32+24], mm3
%endmacro

ALIGN 16
transfer_8to16copy_x86_64:
  ; rdx is Stride
  mov rax, rsi		; Src
  mov rcx, rdi		; Dst
		
  pxor mm7, mm7

  COPY_8_TO_16 0
  COPY_8_TO_16 1
  COPY_8_TO_16 2
  COPY_8_TO_16 3
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer_16to8copy_x86_64(uint8_t * const dst,
;							const int16_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_16_TO_8 1
  movq mm0, [rax+%1*32]
  movq mm1, [rax+%1*32+8]
  packuswb mm0, mm1
  movq [rcx], mm0
  movq mm2, [rax+%1*32+16]
  movq mm3, [rax+%1*32+24]
  packuswb mm2, mm3
  movq [rcx+rdx], mm2
%endmacro

ALIGN 16
transfer_16to8copy_x86_64:
  ; rdx is Stride
  mov rax, rsi		; Src
  mov rcx, rdi		; Dst

  COPY_16_TO_8 0
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8 1
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8 2
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8 3
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub_x86_64(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;-----------------------------------------------------------------------------

; when second argument == 1, reference (ebx) block is to current (eax)
%macro COPY_8_TO_16_SUB 2
  movq mm0, [rax]      ; cur
  movq mm2, [rax+rdx]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [rbx]      ; ref
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [rbx+rdx]  ; ref

  movq mm6, mm4
%if %2 == 1
  movq [rax], mm4
  movq [rax+rdx], mm5
%endif
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea rax, [rax+2*rdx]
  psubsw mm3, mm6
  lea rbx,[rbx+2*rdx]

  movq [rcx+%1*32+ 0], mm0 ; dst
  movq [rcx+%1*32+ 8], mm1
  movq [rcx+%1*32+16], mm2
  movq [rcx+%1*32+24], mm3
%endmacro

ALIGN 16
transfer_8to16sub_x86_64:
  push rbx

  mov rax, rsi		; Cur
  mov rbx, rdx		; Ref
  mov rdx, rcx		; Stride
  mov rcx, rdi		; Dst
  
  pxor mm7, mm7

  COPY_8_TO_16_SUB 0, 1
  COPY_8_TO_16_SUB 1, 1
  COPY_8_TO_16_SUB 2, 1
  COPY_8_TO_16_SUB 3, 1

  pop rbx
  ret
.endfunc

ALIGN 16
transfer_8to16subro_x86_64:
  push rbx

  mov rax, rsi		; Cur
  mov rbx, rdx		; Ref
  mov rdx, rcx		; Stride
  mov rcx, rdi		; Dst

  pxor mm7, mm7

  COPY_8_TO_16_SUB 0, 0
  COPY_8_TO_16_SUB 1, 0
  COPY_8_TO_16_SUB 2, 0
  COPY_8_TO_16_SUB 3, 0

  pop rbx
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2_x86_64(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2_SSE 1
  movq mm0, [rax]      ; cur
  movq mm2, [rax+rdx]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [rbx]     ; ref1
  pavgb mm4, [rsi]     ; ref2
  movq [rax], mm4
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [rbx+rdx] ; ref
  pavgb mm5, [rsi+rdx] ; ref2
  movq [rax+rdx], mm5

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  lea rsi, [rsi+2*rdx]
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea rax, [rax+2*rdx]
  psubsw mm3, mm6
  lea rbx, [rbx+2*rdx]

  movq [rcx+%1*32+ 0], mm0 ; dst
  movq [rcx+%1*32+ 8], mm1
  movq [rcx+%1*32+16], mm2
  movq [rcx+%1*32+24], mm3
%endmacro

ALIGN 16
transfer_8to16sub2_x86_64:
  push rbx

  mov rax, rsi		; Cur
  mov rbx, rdx		; Ref1
  mov rdx, r8		; Stride
  mov rsi, rcx		; Ref2
  mov rcx, rdi		; Dst

  pxor mm7, mm7

  COPY_8_TO_16_SUB2_SSE 0
  COPY_8_TO_16_SUB2_SSE 1
  COPY_8_TO_16_SUB2_SSE 2
  COPY_8_TO_16_SUB2_SSE 3

  pop rbx
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2ro_x86_64(int16_t * const dct,
;				const uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2RO_SSE 1
  movq mm0, [rsi]      ; cur
  movq mm2, [rsi+r8]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [rdx]     ; ref1
  pavgb mm4, [rcx]     ; ref2
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [rdx+r8] ; ref
  pavgb mm5, [rcx+r8] ; ref2

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  lea rcx, [rcx+2*r8]
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea rsi, [rsi+2*r8]
  psubsw mm3, mm6
  lea rdx, [rdx+2*r8]

  movq [rdi+%1*32+ 0], mm0 ; dst
  movq [rdi+%1*32+ 8], mm1
  movq [rdi+%1*32+16], mm2
  movq [rdi+%1*32+24], mm3
%endmacro

ALIGN 16
transfer_8to16sub2ro_x86_64:
  pxor mm7, mm7

  COPY_8_TO_16_SUB2RO_SSE 0
  COPY_8_TO_16_SUB2RO_SSE 1
  COPY_8_TO_16_SUB2RO_SSE 2
  COPY_8_TO_16_SUB2RO_SSE 3

  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer_16to8add_x86_64(uint8_t * const dst,
;						const int16_t * const src,
;						uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_16_TO_8_ADD 1
  movq mm0, [rcx]
  movq mm2, [rcx+rdx]
  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  paddsw mm0, [rax+%1*32+ 0]
  paddsw mm1, [rax+%1*32+ 8]
  paddsw mm2, [rax+%1*32+16]
  paddsw mm3, [rax+%1*32+24]
  packuswb mm0, mm1
  movq [rcx], mm0
  packuswb mm2, mm3
  movq [rcx+rdx], mm2
%endmacro


ALIGN 16
transfer_16to8add_x86_64:
  ; rdx is Stride
  mov rax, rsi		; Src
  mov rcx, rdi		; Dst

  pxor mm7, mm7

  COPY_16_TO_8_ADD 0
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8_ADD 1
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8_ADD 2
  lea rcx,[rcx+2*rdx]
  COPY_16_TO_8_ADD 3
  ret
.endfunc

;-----------------------------------------------------------------------------
;
; void transfer8x8_copy_x86_64(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_8 0
  movq mm0, [rax]
  movq mm1, [rax+rdx]
  movq [rcx], mm0
  lea rax, [rax+2*rdx]
  movq [rcx+rdx], mm1
%endmacro

ALIGN 16
transfer8x8_copy_x86_64:
  ; rdx is Stride
  mov rax, rsi		; Src
  mov rcx, rdi		; Dst

  COPY_8_TO_8
  lea rcx,[rcx+2*rdx]
  COPY_8_TO_8
  lea rcx,[rcx+2*rdx]
  COPY_8_TO_8
  lea rcx,[rcx+2*rdx]
  COPY_8_TO_8
  ret
.endfunc
