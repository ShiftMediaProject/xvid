;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	sse2 cbp calc
; *
; *	This program is an implementation of a part of one or more MPEG-4
; *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
; *	to use this software module in hardware or software products are
; *	advised that its use may infringe existing patents or copyrights, and
; *	any such use would be at such party's own risk.  The original
; *	developer of this software module and his/her company, and subsequent
; *	editors and their companies, will have no liability for use of this
; *	software or modifications or derivatives thereof.
; *
; *	This program is free software; you can redistribute it and/or modify
; *	it under the terms of the GNU General Public License as published by
; *	the Free Software Foundation; either version 2 of the License, or
; *	(at your option) any later version.
; *
; *	This program is distributed in the hope that it will be useful,
; *	but WITHOUT ANY WARRANTY; without even the implied warranty of
; *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *	GNU General Public License for more details.
; *
; *	You should have received a copy of the GNU General Public License
; *	along with this program; if not, write to the Free Software
; *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 14.06.2002  cleanup -Skal-
; * 24.04.2002  had to use sse2's movdqu instead of movdqa (???)
; * 17.04.2002  initial version (c) 2002 Daniel Smith
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

cglobal calc_cbp_sse2

;===========================================================================
;
; uint32_t calc_cbp_sse2(const int16_t coeff[6][64]);
;
; not enabled - slower than mmx?
;
;===========================================================================

%macro LOOP_SSE2 1
    movdqa	xmm0, [edx+(%1)*128]
	pand	xmm0, xmm7
	movdqa	xmm1, [edx+(%1)*128+16]

	por		xmm0, [edx+(%1)*128+32]
	por		xmm1, [edx+(%1)*128+48]
	por		xmm0, [edx+(%1)*128+64]
	por		xmm1, [edx+(%1)*128+80]
	por		xmm0, [edx+(%1)*128+96]
	por		xmm1, [edx+(%1)*128+112]

	por		xmm0, xmm1     ; xmm0 = xmm1 = 128 bits worth of info
	psadbw	xmm0, xmm6     ; contains 2 dwords with sums
	movhlps	xmm1, xmm0     ; move high dword from xmm0 to low xmm1
	por		xmm0, xmm1     ; combine
	movd	ecx, xmm0      ; if ecx set, values were found
	test    ecx, ecx
%endmacro

align 16

calc_cbp_sse2:
    mov     edx, [esp+4]        ; coeff[]
    xor		eax, eax		    ; cbp = 0

	movdqu	xmm7, [ignore_dc]	; mask to ignore dc value
	pxor	xmm6, xmm6          ; zero

  LOOP_SSE2 0
	test ecx, ecx	
	jz		.blk2
    or eax, (1<<5)
.blk2
  LOOP_SSE2 1
	test ecx, ecx	
	jz		.blk3
  or eax, (1<<4)
.blk3
  LOOP_SSE2 2
	test ecx, ecx	
	jz		.blk4
  or eax, (1<<3)
.blk4
  LOOP_SSE2 3
	test ecx, ecx	
	jz		.blk5
  or eax, (1<<2)
.blk5
  LOOP_SSE2 4
	test ecx, ecx	
	jz		.blk6
  or eax, (1<<1)
.blk6
  LOOP_SSE2 5
	test ecx, ecx	
	jz		.finished
  or eax, (1<<0)
.finished
				
  ret
