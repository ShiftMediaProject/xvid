;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx 8bit<->16bit transfers
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
; * $Id: mem_transfer_mmx.asm,v 1.8 2002-11-17 00:51:11 edgomez Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 04.06.2002  speed enhancement (unroll+overlap). -Skal-
; *             + added transfer_8to16sub2_mmx/xmm
; * 07.01.2002	merge functions from compensate_mmx; rename functions
; *	07.11.2001	initial version; (c)2001 peter ross <pross@xvid.org>
; *
; *************************************************************************/


bits 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro


section .text

cglobal transfer_8to16copy_mmx
cglobal transfer_16to8copy_mmx
cglobal transfer_8to16sub_mmx
cglobal transfer_8to16sub2_mmx
cglobal transfer_8to16sub2_xmm
cglobal transfer_16to8add_mmx
cglobal transfer8x8_copy_mmx

;===========================================================================
;
; void transfer_8to16copy_mmx(int16_t * const dst,
;							const uint8_t * const src,
;							uint32_t stride);
;
;===========================================================================

%macro COPY_8_TO_16 1
  movq mm0, [eax]
  movq mm1, [eax+edx]
  movq mm2, mm0
  movq mm3, mm1
  punpcklbw mm0, mm7
  movq [ecx+%1*32], mm0
  punpcklbw mm1, mm7
  movq [ecx+%1*32+16], mm1
  punpckhbw mm2, mm7
  punpckhbw mm3, mm7
  lea eax,[eax+2*edx]
  movq [ecx+%1*32+8], mm2
  movq [ecx+%1*32+24], mm3
%endmacro

align 16
transfer_8to16copy_mmx:

  mov ecx, [esp+ 4] ; Dst
  mov eax, [esp+ 8] ; Src
  mov edx, [esp+12] ; Stride
  pxor mm7,mm7

  COPY_8_TO_16 0
  COPY_8_TO_16 1
  COPY_8_TO_16 2
  COPY_8_TO_16 3
  ret

;===========================================================================
;
; void transfer_16to8copy_mmx(uint8_t * const dst,
;							const int16_t * const src,
;							uint32_t stride);
;
;===========================================================================

%macro COPY_16_TO_8 1
  movq mm0, [eax+%1*32]
  movq mm1, [eax+%1*32+8]
  packuswb mm0, mm1
  movq [ecx], mm0
  movq mm2, [eax+%1*32+16]
  movq mm3, [eax+%1*32+24]
  packuswb mm2, mm3
  movq [ecx+edx], mm2
%endmacro

align 16
transfer_16to8copy_mmx:

  mov ecx, [esp+ 4] ; Dst
  mov eax, [esp+ 8] ; Src
  mov edx, [esp+12] ; Stride

  COPY_16_TO_8 0
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8 1
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8 2
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8 3
  ret

;===========================================================================
;
; void transfer_8to16sub_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;===========================================================================
;/**************************************************************************
; *
; *	History:
; *
; * 27.12.2001	renamed from 'compensate' to 'transfer_8to16sub'
; * 02.12.2001  loop unrolled, code runs 10% faster now (Isibaar)
; * 30.11.2001  16 pixels are processed per iteration (Isibaar)
; * 30.11.2001	.text missing
; *	06.11.2001	inital version; (c)2001 peter ross <pross@xvid.org>
; *
; *************************************************************************/

%macro COPY_8_TO_16_SUB 1
  movq mm0, [eax]      ; cur
  movq mm2, [eax+edx]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [ebx]      ; ref
	punpckhbw mm1, mm7
	punpckhbw mm3, mm7
  movq mm5, [ebx+edx]  ; ref

  movq mm6, mm4
  movq [eax], mm4
  movq [eax+edx], mm5
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea eax,[eax+2*edx]
  psubsw mm3, mm6
  lea ebx,[ebx+2*edx]

  movq [ecx+%1*32+ 0], mm0 ; dst
	movq [ecx+%1*32+ 8], mm1
	movq [ecx+%1*32+16], mm2
	movq [ecx+%1*32+24], mm3
%endmacro

align 16
transfer_8to16sub_mmx:
  mov ecx, [esp  + 4] ; Dst
  mov eax, [esp  + 8] ; Cur
  push ebx
  mov ebx, [esp+4+12] ; Ref
  mov edx, [esp+4+16] ; Stride
  pxor mm7, mm7

  COPY_8_TO_16_SUB 0
  COPY_8_TO_16_SUB 1
  COPY_8_TO_16_SUB 2
  COPY_8_TO_16_SUB 3

  pop ebx
  ret

;===========================================================================
;
; void transfer_8to16sub2_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;===========================================================================

%macro COPY_8_TO_16_SUB2_MMX 1
  movq mm0, [eax]      ; cur
  movq mm2, [eax+edx]

    ; mm4 <- (ref1+ref2+1) / 2
  movq mm4, [ebx]      ; ref1
  movq mm1, [esi]      ; ref2
  movq mm6, mm4
  movq mm3, mm1
  punpcklbw mm4, mm7
  punpcklbw mm1, mm7
  punpckhbw mm6, mm7
  punpckhbw mm3, mm7
  paddusw mm4, mm1
  paddusw mm6, mm3
  psrlw mm4,1
  psrlw mm6,1
  packuswb mm4, mm6

    ; mm5 <- (ref1+ref2+1) / 2
  movq mm5, [ebx+edx]  ; ref1
  movq mm1, [esi+edx]  ; ref2
  movq mm6, mm5
  movq mm3, mm1
  punpcklbw mm5, mm7
  punpcklbw mm1, mm7
  punpckhbw mm6, mm7
  punpckhbw mm3, mm7  
  paddusw mm5, mm1
  paddusw mm6, mm3
  lea esi,[esi+2*edx]
  psrlw mm5,1
  psrlw mm6,1
  packuswb mm5, mm6


  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
	punpckhbw mm1, mm7
	punpckhbw mm3, mm7

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea eax,[eax+2*edx]
  psubsw mm3, mm6
  lea ebx,[ebx+2*edx]

  movq [ecx+%1*32+ 0], mm0 ; dst
	movq [ecx+%1*32+ 8], mm1
	movq [ecx+%1*32+16], mm2
	movq [ecx+%1*32+24], mm3
%endmacro

align 16
transfer_8to16sub2_mmx:
  mov ecx, [esp  + 4] ; Dst
  mov eax, [esp  + 8] ; Cur
  push ebx
  mov ebx, [esp+4+12] ; Ref1
  push esi
  mov esi, [esp+8+16] ; Ref2
  mov edx, [esp+8+20] ; Stride
  pxor mm7, mm7

  COPY_8_TO_16_SUB2_MMX 0
  COPY_8_TO_16_SUB2_MMX 1
  COPY_8_TO_16_SUB2_MMX 2
  COPY_8_TO_16_SUB2_MMX 3

  pop esi
  pop ebx
  ret

;===========================================================================
;
; void transfer_8to16sub2_xmm(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;===========================================================================

%macro COPY_8_TO_16_SUB2_SSE 1
  movq mm0, [eax]      ; cur
  movq mm2, [eax+edx]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [ebx]      ; ref1
  pavgb mm4, [esi]     ; ref2
	punpckhbw mm1, mm7
	punpckhbw mm3, mm7
  movq mm5, [ebx+edx]  ; ref
  pavgb mm5, [esi+edx] ; ref2

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  lea esi,[esi+2*edx]
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea eax,[eax+2*edx]
  psubsw mm3, mm6
  lea ebx,[ebx+2*edx]

  movq [ecx+%1*32+ 0], mm0 ; dst
	movq [ecx+%1*32+ 8], mm1
	movq [ecx+%1*32+16], mm2
	movq [ecx+%1*32+24], mm3
%endmacro

align 16
transfer_8to16sub2_xmm:
  mov ecx, [esp  + 4] ; Dst
  mov eax, [esp  + 8] ; Cur
  push ebx
  mov ebx, [esp+4+12] ; Ref1
  push esi
  mov esi, [esp+8+16] ; Ref2
  mov edx, [esp+8+20] ; Stride
  pxor mm7, mm7

  COPY_8_TO_16_SUB2_SSE 0
  COPY_8_TO_16_SUB2_SSE 1
  COPY_8_TO_16_SUB2_SSE 2
  COPY_8_TO_16_SUB2_SSE 3

  pop esi
  pop ebx
  ret

;===========================================================================
;
; void transfer_16to8add_mmx(uint8_t * const dst,
;						const int16_t * const src,
;						uint32_t stride);
;
;===========================================================================

%macro COPY_16_TO_8_ADD 1
  movq mm0, [ecx]
  movq mm2, [ecx+edx]
  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  paddsw mm0, [eax+%1*32+ 0]
  paddsw mm1, [eax+%1*32+ 8]
  paddsw mm2, [eax+%1*32+16]
  paddsw mm3, [eax+%1*32+24]
  packuswb mm0, mm1
  movq [ecx], mm0
  packuswb mm2, mm3
  movq [ecx+edx], mm2
%endmacro


align 16
transfer_16to8add_mmx:
  mov ecx, [esp+ 4] ; Dst
  mov eax, [esp+ 8] ; Src
  mov edx, [esp+12] ; Stride
  pxor mm7, mm7

  COPY_16_TO_8_ADD 0
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8_ADD 1
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8_ADD 2
  lea ecx,[ecx+2*edx]
  COPY_16_TO_8_ADD 3
  ret

;===========================================================================
;
; void transfer8x8_copy_mmx(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;===========================================================================

%macro COPY_8_TO_8 0
  movq mm0, [eax]
  movq mm1, [eax+edx]
  movq [ecx], mm0
  lea eax,[eax+2*edx]
  movq [ecx+edx], mm1
%endmacro

align 16
transfer8x8_copy_mmx:
  mov ecx, [esp+ 4] ; Dst
  mov eax, [esp+ 8] ; Src
  mov edx, [esp+12] ; Stride

  COPY_8_TO_8
  lea ecx,[ecx+2*edx]
  COPY_8_TO_8
  lea ecx,[ecx+2*edx]
  COPY_8_TO_8
  lea ecx,[ecx+2*edx]
  COPY_8_TO_8
  ret
