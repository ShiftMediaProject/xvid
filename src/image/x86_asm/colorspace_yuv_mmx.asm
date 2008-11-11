;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM YV12->YV12 conversion -
; *
; *  Copyright(C) 2001 Michael Militzer <isibaar@xvid.org>
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
; * $Id: colorspace_yuv_mmx.asm,v 1.8 2008-11-11 20:46:24 Isibaar Exp $
; *
; ***************************************************************************/

BITS 32

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
; Helper macros
;=============================================================================

;------------------------------------------------------------------------------
; PLANE_COPY ( DST, DST_STRIDE, SRC, SRC_STRIDE, WIDTH, HEIGHT, OPT )
; DST		dst buffer
; DST_STRIDE	dst stride
; SRC		src destination buffer
; SRC_STRIDE	src stride
; WIDTH		width
; HEIGHT	height
; OPT		0=plain mmx, 1=xmm
;------------------------------------------------------------------------------

%macro	PLANE_COPY	7
%define DST		%1
%define DST_STRIDE      %2
%define SRC		%3
%define SRC_STRIDE      %4
%define WIDTH		%5
%define HEIGHT		%6
%define OPT		%7

  mov eax, WIDTH
  mov ebp, HEIGHT           ; $ebp$ = height
  mov esi, SRC
  mov edi, DST

  mov ebx, eax
  shr eax, 6                ; $eax$ = width / 64
  and ebx, 63               ; remainder = width % 64
  mov edx, ebx
  shr ebx, 4                ; $ebx$ = remainder / 16
  and edx, 15               ; $edx$ = remainder % 16

%%loop64_start_pc:
  push edi
  push esi
  mov  ecx, eax              ; width64
  test eax, eax
  jz %%loop16_start_pc
  
%%loop64_pc:
%if OPT == 1                ; xmm
  prefetchnta [esi + 64]    ; non temporal prefetch
  prefetchnta [esi + 96]
%endif
  movq mm1, [esi     ]           ; read from src
  movq mm2, [esi +  8]
  movq mm3, [esi + 16]
  movq mm4, [esi + 24]
  movq mm5, [esi + 32]
  movq mm6, [esi + 40]
  movq mm7, [esi + 48]
  movq mm0, [esi + 56]

%if OPT == 0                ; plain mmx
  movq [edi     ], mm1           ; write to y_out
  movq [edi +  8], mm2
  movq [edi + 16], mm3
  movq [edi + 24], mm4
  movq [edi + 32], mm5
  movq [edi + 40], mm6
  movq [edi + 48], mm7
  movq [edi + 56], mm0
%else
  movntq [edi     ], mm1         ; write to y_out
  movntq [edi +  8], mm2
  movntq [edi + 16], mm3
  movntq [edi + 24], mm4
  movntq [edi + 32], mm5
  movntq [edi + 40], mm6
  movntq [edi + 48], mm7
  movntq [edi + 56], mm0
%endif

  add esi, 64
  add edi, 64
  loop %%loop64_pc


%%loop16_start_pc:
  mov  ecx, ebx              ; width16
  test ebx, ebx
  jz %%loop1_start_pc

%%loop16_pc:
  movq mm1, [esi]
  movq mm2, [esi + 8]
%if OPT == 0                ; plain mmx
  movq [edi], mm1
  movq [edi + 8], mm2
%else
  movntq [edi], mm1
  movntq [edi + 8], mm2
%endif

  add esi, 16
  add edi, 16
  loop %%loop16_pc


%%loop1_start_pc:
  mov ecx, edx
  rep movsb

  pop esi
  pop edi
  add esi, SRC_STRIDE
  add edi, DST_STRIDE
  dec ebp
  jg near %%loop64_start_pc
%endmacro

;------------------------------------------------------------------------------
; PLANE_FILL ( DST, DST_STRIDE, WIDTH, HEIGHT, OPT )
; DST		dst buffer
; DST_STRIDE	dst stride
; WIDTH		width
; HEIGHT	height
; OPT		0=plain mmx, 1=xmm
;------------------------------------------------------------------------------

%macro	PLANE_FILL	5
%define DST		%1
%define DST_STRIDE      %2
%define WIDTH		%3
%define HEIGHT		%4
%define OPT		%5

  mov esi, WIDTH
  mov ebp, HEIGHT           ; $ebp$ = height
  mov edi, DST

  mov eax, 0x80808080
  mov ebx, esi
  shr esi, 6                ; $esi$ = width / 64
  and ebx, 63               ; ebx = remainder = width % 64
  movd mm0, eax
  mov edx, ebx
  shr ebx, 4                ; $ebx$ = remainder / 16
  and edx, 15               ; $edx$ = remainder % 16
  punpckldq mm0, mm0

%%loop64_start_pf:
  push edi
  mov  ecx, esi              ; width64
  test esi, esi
  jz %%loop16_start_pf

%%loop64_pf:

%if OPT == 0                ; plain mmx
  movq [edi     ], mm0          ; write to y_out
  movq [edi +  8], mm0
  movq [edi + 16], mm0
  movq [edi + 24], mm0
  movq [edi + 32], mm0
  movq [edi + 40], mm0
  movq [edi + 48], mm0
  movq [edi + 56], mm0
%else
  movntq [edi     ], mm0        ; write to y_out
  movntq [edi +  8], mm0
  movntq [edi + 16], mm0
  movntq [edi + 24], mm0
  movntq [edi + 32], mm0
  movntq [edi + 40], mm0
  movntq [edi + 48], mm0
  movntq [edi + 56], mm0
%endif

  add edi, 64
  loop %%loop64_pf

%%loop16_start_pf:
  mov  ecx, ebx              ; width16
  test ebx, ebx
  jz %%loop1_start_pf

%%loop16_pf:
%if OPT == 0                ; plain mmx
  movq [edi    ], mm0
  movq [edi + 8], mm0
%else
  movntq [edi    ], mm0
  movntq [edi + 8], mm0
%endif

  add edi, 16
  loop %%loop16_pf

%%loop1_start_pf:
  mov ecx, edx
  rep stosb

  pop edi
  add edi, DST_STRIDE
  dec ebp
  jg near %%loop64_start_pf
%endmacro

;------------------------------------------------------------------------------
; MAKE_YV12_TO_YV12( NAME, OPT )
; NAME	function name
; OPT	0=plain mmx, 1=xmm
;
; yv12_to_yv12_mmx(uint8_t * y_dst, uint8_t * u_dst, uint8_t * v_dst,
; 				int y_dst_stride, int uv_dst_stride,
; 				uint8_t * y_src, uint8_t * u_src, uint8_t * v_src,
; 				int y_src_stride, int uv_src_stride,
; 				int width, int height, int vflip)
;------------------------------------------------------------------------------
%macro	MAKE_YV12_TO_YV12	2
%define	NAME		%1
%define	OPT		%2
ALIGN 16
cglobal NAME
NAME:
%define pushsize	16
%define localsize	12

%define vflip		esp + localsize + pushsize + 52
%define height		esp + localsize + pushsize + 48
%define width        	esp + localsize + pushsize + 44
%define uv_src_stride	esp + localsize + pushsize + 40
%define y_src_stride	esp + localsize + pushsize + 36
%define v_src		esp + localsize + pushsize + 32
%define u_src   	esp + localsize + pushsize + 28
%define y_src		esp + localsize + pushsize + 24
%define uv_dst_stride	esp + localsize + pushsize + 20
%define y_dst_stride	esp + localsize + pushsize + 16
%define v_dst		esp + localsize + pushsize + 12
%define u_dst   	esp + localsize + pushsize + 8
%define y_dst		esp + localsize + pushsize + 4
%define _ip		esp + localsize + pushsize + 0

  push ebx	;	esp + localsize + 16
  push esi	;	esp + localsize + 8
  push edi	;	esp + localsize + 4
  push ebp	;	esp + localsize + 0

%define width2			esp + localsize - 4
%define height2			esp + localsize - 8

  sub esp, localsize

  mov eax, [width]
  mov ebx, [height]
  shr eax, 1                    ; calculate widht/2, heigh/2
  shr ebx, 1
  mov [width2], eax
  mov [height2], ebx

  mov eax, [vflip]
  test eax, eax
  jz near .go

        ; flipping support
  mov eax, [height]
  mov esi, [y_src]
  mov ecx, [y_src_stride]
  sub eax, 1
  imul eax, ecx
  add esi, eax                  ; y_src += (height-1) * y_src_stride
  neg ecx
  mov [y_src], esi
  mov [y_src_stride], ecx       ; y_src_stride = -y_src_stride

  mov eax, [height2]
  mov esi, [u_src]
  mov edi, [v_src]
  mov ecx, [uv_src_stride]
  test esi, esi
  jz .go
  test edi, edi
  jz .go
  sub eax, 1                    ; eax = height2 - 1
  imul eax, ecx
  add esi, eax                  ; u_src += (height2-1) * uv_src_stride
  add edi, eax                  ; v_src += (height2-1) * uv_src_stride
  neg ecx
  mov [u_src], esi
  mov [v_src], edi
  mov [uv_src_stride], ecx      ; uv_src_stride = -uv_src_stride

.go:

  PLANE_COPY [y_dst], [y_dst_stride],  [y_src], [y_src_stride],  [width],  [height], OPT

  mov eax, [u_src]
  or  eax, [v_src]
  jz near .UVFill_0x80
  PLANE_COPY [u_dst], [uv_dst_stride], [u_src], [uv_src_stride], [width2], [height2], OPT
  PLANE_COPY [v_dst], [uv_dst_stride], [v_src], [uv_src_stride], [width2], [height2], OPT

.Done_UVPlane:
  add esp, localsize
  pop ebp
  pop edi
  pop esi
  pop ebx
  ret

.UVFill_0x80:
  PLANE_FILL [u_dst], [uv_dst_stride], [width2], [height2], OPT
  PLANE_FILL [v_dst], [uv_dst_stride], [width2], [height2], OPT
  jmp near .Done_UVPlane
ENDFUNC
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

MAKE_YV12_TO_YV12	yv12_to_yv12_mmx, 0

MAKE_YV12_TO_YV12	yv12_to_yv12_xmm, 1

%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

