;/**************************************************************************
; *
; *	XVID MPEG-4 VIDEO CODEC
; *	mmx cbp calc
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
; * 24.04.2002	had to use sse2's movdqu instead of movdqa (???)
; * 17.04.2002  sse2 stuff
; * 22.03.2002      0.01          ; Min Chen <chenm001@163.com>
; *                               ; use 386 cpu's 'BTS' to replace 'cbp |= 1 << (edx-1)'
; *	24.11.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
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


;===========================================================================
;
; uint32_t calc_cbp_mmx(const int16_t coeff[6][64]);
;
;===========================================================================

align 16
cglobal calc_cbp_mmx
calc_cbp_mmx
				push	ebx
				push	ecx
				push	edx
				push	esi

                mov     esi, [esp + 16 + 4]              ; coeff
				movq	mm7, [ignore_dc]

				xor		eax, eax		; cbp = 0
				mov		edx, 6
.loop
                movq	mm0, [esi]
				pand	mm0, mm7
                movq	mm1, [esi+8]

                por     mm0, [esi+16]
                por     mm1, [esi+24]

                por     mm0, [esi+32]
                por     mm1, [esi+40]

                por     mm0, [esi+48]
                por     mm1, [esi+56]

                por     mm0, [esi+64]
                por     mm1, [esi+72]

                por     mm0, [esi+80]
                por     mm1, [esi+88]

                por     mm0, [esi+96]
                por     mm1, [esi+104]

                por     mm0, [esi+112]
                por     mm1, [esi+120]

                por     mm0, mm1
                movq    mm1, mm0
                psrlq   mm1, 32
                por     mm0, mm1
				movd    ebx, mm0

				add		esi, 128

				or		ebx, ebx
				jz		.iterate

				; cbp |= 1 << (edx-1)
				
                                ; Change by Chenm001 <chenm001@163.com>
                                ;mov             ecx, edx
                                ;dec             ecx
                                ;mov             ebx, 1
                                ;shl             ebx, cl
                                ;or              eax, ebx
                                lea             ebx,[edx-1]
                                bts             eax,ebx

.iterate		dec	edx
				jnz		.loop

				pop	esi
				pop	edx
				pop	ecx
				pop	ebx
				
				ret



;===========================================================================
;
; uint32_t calc_cbp_sse2(const int16_t coeff[6][64]);
;
; not enabled - slower than mmx?
;
;===========================================================================

align 16
cglobal calc_cbp_sse2
calc_cbp_sse2
				push	esi

				mov     esi, [esp + 4 + 4]		; coeff
				movdqu	xmm7, [ignore_dc]		; mask to ignore dc value

				xor		eax, eax				; cbp = 0
				pxor	xmm6, xmm6				; zeroes to help psadbw

.first			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1				; xmm0 = xmm1 = 128 bits worth of info
				psadbw	xmm0, xmm6				; contains 2 dwords with sums
				movhlps	xmm1, xmm0				; move high dword from xmm0 to low xmm1
				por		xmm0, xmm1				; combine
				movd	ecx, xmm0				; if ecx set, values were found

				add		esi, 128

				or		ecx, ecx
				jz		.second

				bts		eax, 5

.second			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1
				psadbw	xmm0, xmm6
				movhlps	xmm1, xmm0
				por		xmm0, xmm1
				movd	ecx, xmm0

				add		esi, 128

				or		ecx, ecx
				jz		.third

				bts		eax, 4

.third			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1
				psadbw	xmm0, xmm6
				movhlps	xmm1, xmm0
				por		xmm0, xmm1
				movd	ecx, xmm0

				add		esi, 128

				or		ecx, ecx
				jz		.fourth

				bts		eax, 3

.fourth			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1
				psadbw	xmm0, xmm6
				movhlps	xmm1, xmm0
				por		xmm0, xmm1
				movd	ecx, xmm0

				add		esi, 128

				or		ecx, ecx
				jz		.fifth

				bts		eax, 2

.fifth			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1
				psadbw	xmm0, xmm6
				movhlps	xmm1, xmm0
				por		xmm0, xmm1
				movd	ecx, xmm0

				add		esi, 128

				or		ecx, ecx
				jz		.sixth

				bts		eax, 1

.sixth			movdqa	xmm0, [esi]
				pand	xmm0, xmm7
				movdqa	xmm1, [esi+16]

				por		xmm0, [esi+32]
				por		xmm1, [esi+48]
				por		xmm0, [esi+64]
				por		xmm1, [esi+80]
				por		xmm0, [esi+96]
				por		xmm1, [esi+112]

				por		xmm0, xmm1
				psadbw	xmm0, xmm6
				movhlps	xmm1, xmm0
				por		xmm0, xmm1
				movd	ecx, xmm0

				or		ecx, ecx
				jz		.end

				bts		eax, 0

.end			pop	esi

				ret
