;/******************************************************************************
; *                                                                            *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
; *                                                                            *
; *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
; *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
; *  software module in hardware or software products are advised that its     *
; *  use may infringe existing patents or copyrights, and any such use         *
; *  would be at such party's own risk.  The original developer of this        *
; *  software module and his/her company, and subsequent editors and their     *
; *  companies, will have no liability for use of this software or             *
; *  modifications or derivatives thereof.                                     *
; *                                                                            *
; *  XviD is free software; you can redistribute it and/or modify it           *
; *  under the terms of the GNU General Public License as published by         *
; *  the Free Software Foundation; either version 2 of the License, or         *
; *  (at your option) any later version.                                       *
; *                                                                            *
; *  XviD is distributed in the hope that it will be useful, but               *
; *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
; *  GNU General Public License for more details.                              *
; *                                                                            *
; *  You should have received a copy of the GNU General Public License         *
; *  along with this program; if not, write to the Free Software               *
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
; *                                                                            * 
; ******************************************************************************/
;
;/******************************************************************************
; *   quant4 bugs have been fixed: (a) overflow bug for matrix elements 	   *
; *   equal to 1 or 2 is fixed by substituting pmulhw with pmulhuw (iSSE) 	   *
; *   and using multiplier 0ffffh instead of 10001h (for matrix element = 1;   *
; *   in that case, 1 is added before multiplying, that additional 1 comes 	   *
; *   from intra_matrix1; (b) rounding error for large coefficients and matrix *
; *   elements is fixed by two-step approach: first approximation (rounded 	   *
; *   down) is found as usual; the result is multiplied by the matrix element  *
; *   and mismatch is used to calculate the correction.						   *
; ******************************************************************************/
;
; _3dne functions are compatible with iSSE, but are optimized specifically for 
; K7 pipelines
;
;------------------------------------------------------------------------------
; 09.12.2002  Athlon optimizations contributed by Jaan Kalda 
;------------------------------------------------------------------------------


; data/text alignment														   
%define ALIGN 8
%define SATURATE

bits 32

%ifdef FORMAT_COFF
SECTION .data data
%else
SECTION .data data align=8
%endif

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%macro cextern 1 
	%ifdef PREFIX
		extern _%1 
		%define %1 _%1
	%else
		extern %1
	%endif
%endmacro
align 8
mmzero dd 0,0

mmx_one times 4	dw	 1

;===========================================================================
;
; divide by 2Q table 
;
;===========================================================================

align ALIGN
mmx_divs            ;i>2
%assign i 1
%rep 31
	times 4 dw  ((1 << 15) / i + 1) 
	%assign i i+1
%endrep

align ALIGN
mmx_div            ;i>2
%assign i 1 
%rep 31
	times 4 dw  ((1 << 16) / i + 1) 
	%assign i i+1
%endrep


;===========================================================================
;
; intra matrix 
;
;===========================================================================

%macro FIXX 1
dw (1 << 16) / (%1) + 1
%endmacro 

cextern intra_matrix_fixl
cextern intra_matrix_fix
cextern intra_matrix1
cextern intra_matrix

;===========================================================================
;
; inter matrix
;
;===========================================================================

cextern inter_matrix1
cextern inter_matrix
cextern inter_matrix_fix
cextern inter_matrix_fixl


%define VM18P 3
%define VM18Q 4
%define nop4 DB 08Dh,074h,026h,0
%define nop3 add esp,byte 0
%define nop2 mov esp,esp
%define nop7 db 08dh,02ch,02dh,0,0,0,0
%define nop6 add ebp,dword 0
;===========================================================================
;
; quantd table 
;
;===========================================================================


quantd
%assign i 1
%rep 31
	times 4 dw  (((VM18P*i) + (VM18Q/2)) / VM18Q)
	%assign i i+1
%endrep

;===========================================================================
;
; multiple by 2Q table
;
;===========================================================================


mmx_mul_quant
%assign i 1
%rep 31
	times 4 dw  i
	%assign i i+1
%endrep

;===========================================================================
;
; saturation limits 
;
;===========================================================================

align 16

mmx_32767_minus_2047				times 4 dw (32767-2047)
mmx_32768_minus_2048				times 4 dw (32768-2048)
mmx_2047 times 4 dw 2047
mmx_minus_2048 times 4 dw (-2048)
zero times 4 dw 0

int_div
dd 0
%assign i 1
%rep 255 
	dd  (1 << 17) / ( i) + 1
	%assign i i+1
%endrep

section .text

;===========================================================================
;
; void quant_intra4_xmm(int16_t * coeff, 
;					const int16_t const * data,
;					const uint32_t quant,
;					const uint32_t dcscalar);
;
;===========================================================================

align ALIGN
cglobal quant4_intra_xmm
quant4_intra_xmm

		mov	eax, [esp  + 8]		; data
		mov	ecx, [esp  + 12]	; quant
		mov	edx, [esp  + 4]		; coeff
		push	esi
		push	edi
		push	ebx
		nop
		mov edi,mmzero
		mov esi,-14		
		pxor mm0,mm0
		pxor mm3,mm3
		cmp	ecx,byte 1
		je	near .q1loop
		cmp	ecx,byte 19
		jg near .lloop
		nop6

		 
align ALIGN
.loop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi + 120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4			; level << 4
		psllw   mm3, 4			;		
		paddw   mm0, [intra_matrix1 + 8*esi+112] 
		paddw   mm3, [intra_matrix1 + 8*esi+120]
		movq    mm5,[intra_matrix_fixl + 8*esi+112] 
		movq    mm7,[intra_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  
		mov esp,esp
		movq   mm2,[intra_matrix + 8*esi+112]
		movq   mm6,[intra_matrix + 8*esi+120]
		pmullw mm2,mm5 
		pmullw mm6,mm7 
		psubw mm0,mm2 
		psubw mm3,mm6
		nop4
		movq mm2,[quantd + ecx * 8 - 8]
		movq mm6,[mmx_divs + ecx * 8 - 8] 
		paddw mm5,mm2
		paddw mm7,mm2
		mov esp,esp
		pmulhuw mm0,[intra_matrix_fix + 8*esi+112] 
		pmulhuw mm3,[intra_matrix_fix + 8*esi+120]
		paddw mm5,mm0
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]		
		pmulhuw	mm5, mm6		; mm0 = (mm0 / 2Q) >> 16
		pmulhuw	mm7, mm6		;  (level + quantd) / quant (0<quant<32)
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		movq	[edx + 8*esi+112], mm5
		movq	[edx + 8*esi +120], mm7
		add esi,byte 2
		jng 	near .loop 
.done	
		; caclulate  data[0] // (int32_t)dcscalar)


	mov 	esi, [esp + 12 + 16]	; dcscalar
  	movsx 	ecx, word [eax]
 	mov		edi,ecx
 	mov 	edx,[esp + 12 + 16]
 	shr 	edx, 1			; ebx = dcscalar /2
 	sar		edi,31 ;cdq is vectorpath
 	xor		edx,edi ; ebx = eax V -eax -1
 	sub		ecx,edi
 	add		ecx,edx
  	mov 	edx,[dword esp + 12 + 4]
	mov	esi,[int_div+4*esi]
	imul	ecx,esi
	sar	ecx,17
;		idiv	cx			; ecx = edi:ecx / dcscalar
		
		mov ebx,[esp]
		mov edi,[esp+4]
		mov esi,[esp+8]
		add esp,byte 12
		mov	[edx], cx		; coeff[0] = ax

		ret				

align ALIGN
.q1loop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi+120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4								; level << 4
		psllw   mm3, 4			
		paddw   mm0, [intra_matrix1 + 8*esi+112] 	;mm0 is to be divided
		paddw   mm3, [intra_matrix1 + 8*esi+120] 	; intra1 contains fix for division by 1
		movq    mm5,[intra_matrix_fixl + 8*esi+112] ;with rounding down
		movq    mm7,[intra_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  							;mm7: first approx of division
		mov esp,esp
		movq   mm2,[intra_matrix + 8*esi+112]
		movq   mm6,[intra_matrix + 8*esi+120]		; divs for q<=16
		pmullw mm2,mm5 								;test value <= original
		pmullw mm6,mm7 
		psubw mm0,mm2 								;mismatch
		psubw mm3,mm6
		nop4
		movq mm2,[quantd + ecx * 8 - 8]
		paddw mm5,mm2 								;first approx with quantd
		paddw mm7,mm2
		mov esp,esp
		pmulhuw mm0,[intra_matrix_fix + 8*esi+112]  ;correction
		pmulhuw mm3,[intra_matrix_fix + 8*esi+120]
		paddw mm5,mm0 								;final result with quantd
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]
		mov esp,esp
		psrlw   mm5, 1			;  (level + quantd) /2  (quant = 1)
		psrlw   mm7, 1		
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		movq	[edx + 8*esi+112], mm5
		movq	[edx + 8*esi +120], mm7
		add esi,byte 2
		jng	near .q1loop
		jmp	near .done

align 8
.lloop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi+120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4			; level << 4
		psllw   mm3, 4			;		
		paddw   mm0, [intra_matrix1 + 8*esi+112] ;mm0 is to be divided intra1 contains fix for division by 1
		paddw   mm3, [intra_matrix1 + 8*esi+120]
		movq    mm5,[intra_matrix_fixl + 8*esi+112] 
		movq    mm7,[intra_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  ;mm7: first approx of division
		mov esp,esp
		movq   mm2,[intra_matrix + 8*esi+112]
		movq   mm6,[intra_matrix + 8*esi+120]
		pmullw mm2,mm5 ;test value <= original
		pmullw mm6,mm7 
		psubw mm0,mm2 ;mismatch
		psubw mm3,mm6
		nop4
		movq mm2,[quantd + ecx * 8 - 8]
		movq mm6,[mmx_div + ecx * 8 - 8] ; divs for q<=16
		paddw mm5,mm2 ;first approx with quantd
		paddw mm7,mm2
		mov esp,esp
		pmulhuw mm0,[intra_matrix_fix + 8*esi+112] ;correction
		pmulhuw mm3,[intra_matrix_fix + 8*esi+120]
		paddw mm5,mm0 ;final result with quantd
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]
		mov esp,esp
		pmulhuw	mm5, mm6		; mm0 = (mm0 / 2Q) >> 16
		pmulhuw	mm7, mm6		;  (level + quantd) / quant (0<quant<32)
		psrlw   mm5, 1			; (level + quantd) / (2*quant)
		psrlw   mm7, 1		
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		movq	[edx + 8*esi+112], mm5
		movq	[edx + 8*esi +120], mm7
		add esi,byte 2
		jng 	near .lloop 
		jmp	near .done

;===========================================================================
;
; uint32_t quant4_inter_xmm(int16_t * coeff,
;					const int16_t const * data,
;					const uint32_t quant);
;
;===========================================================================

align ALIGN
cglobal quant4_inter_xmm
		quant4_inter_xmm

		mov	eax, [esp  + 8]		; data
		mov	ecx, [esp  + 12]	; quant
		mov	edx, [esp  + 4]		; coeff
		push	esi
		push	edi
		push	ebx
		nop
		mov edi,mmzero
		mov esi,-14		
		mov ebx,esp
		sub esp,byte 24
		lea ebx,[esp+8]
		and ebx,byte -8 ;align 8
		pxor mm0,mm0
		pxor mm3,mm3
		movq [byte ebx],mm0
		db 0Fh, 7Fh, 44h, 23h, 8 ;movq [ebx+8],mm0
		cmp	ecx,byte 1
		je	near .q1loop
		cmp	ecx,byte 19
		jg near .lloop
		nop

		 
align ALIGN
.loop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi + 120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4			; level << 4
		psllw   mm3, 4			;		
		paddw   mm0, [inter_matrix1 + 8*esi+112] 
		paddw   mm3, [inter_matrix1 + 8*esi+120]
		movq    mm5,[inter_matrix_fixl + 8*esi+112] 
		movq    mm7,[inter_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  
		mov esp,esp
		movq   mm2,[inter_matrix + 8*esi+112]
		movq   mm6,[inter_matrix + 8*esi+120]
		pmullw mm2,mm5 
		pmullw mm6,mm7 
		psubw mm0,mm2 
		psubw mm3,mm6
		movq mm2,[byte ebx]
		movq mm6,[mmx_divs + ecx * 8 - 8] 
		pmulhuw mm0,[inter_matrix_fix + 8*esi+112] 
		pmulhuw mm3,[inter_matrix_fix + 8*esi+120]
		paddw mm2,[ebx+8]   ;sum
		paddw mm5,mm0
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]		
		pmulhuw	mm5, mm6		; mm0 = (mm0 / 2Q) >> 16
		pmulhuw	mm7, mm6		;  (level ) / quant (0<quant<32)
		add esi,byte 2
		paddw 	mm2,mm5 ;sum += x1
		movq 	[ebx],mm7 ;store x2
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		db 0Fh, 7Fh, 54h, 23h, 08 ;movq 	[ebx+8],mm2 ;store sum
		movq	[edx + 8*esi+112-16], mm5
		movq	[edx + 8*esi +120-16], mm7
		jng 	near .loop 
.done	
		; caclulate  data[0] // (int32_t)dcscalar)

		paddw mm2,[ebx]
		mov ebx,[esp+24]
		mov edi,[esp+4+24]
		mov esi,[esp+8+24]
		add esp,byte 12+24
		pmaddwd mm2, [mmx_one]
		punpckldq mm0,mm2 ;get low dw to mm0:high
		paddd mm0,mm2
		punpckhdq mm0,mm0 ;get result to low
		movd	eax, mm0		
		ret				

align ALIGN
.q1loop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi+120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4								; level << 4
		psllw   mm3, 4			
		paddw   mm0, [inter_matrix1 + 8*esi+112] 	;mm0 is to be divided
		paddw   mm3, [inter_matrix1 + 8*esi+120] 	; inter1 contains fix for division by 1
		movq    mm5,[inter_matrix_fixl + 8*esi+112] ;with rounding down
		movq    mm7,[inter_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  							;mm7: first approx of division
		mov esp,esp
		movq   mm2,[inter_matrix + 8*esi+112]
		movq   mm6,[inter_matrix + 8*esi+120]		; divs for q<=16
		pmullw mm2,mm5 								;test value <= original
		pmullw mm6,mm7 
		psubw mm0,mm2 								;mismatch
		psubw mm3,mm6
		movq mm2,[byte ebx]
		pmulhuw mm0,[inter_matrix_fix + 8*esi+112]  ;correction
		pmulhuw mm3,[inter_matrix_fix + 8*esi+120]
		paddw mm2,[ebx+8]   ;sum		
		paddw mm5,mm0 								;final result
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]
		psrlw   mm5, 1			;  (level ) /2  (quant = 1)
		psrlw   mm7, 1		
		add esi,byte 2
		paddw 	mm2,mm5 ;sum += x1
		movq 	[ebx],mm7 ;store x2		
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		movq 	[ebx+8],mm2 ;store sum		
		movq	[edx + 8*esi+112-16], mm5
		movq	[edx + 8*esi +120-16], mm7
		jng	near .q1loop
		jmp	near .done

align 8
.lloop
		movq	mm1, [eax + 8*esi+112]		; mm0 = [1st]
		psubw 	mm0,mm1 ;-mm1
		movq	mm4, [eax + 8*esi+120]	; 
		psubw	mm3,mm4 ;-mm4
		pmaxsw  mm0,mm1 ;|src|
		pmaxsw  mm3,mm4
		nop2
		psraw   mm1,15 ;sign src
		psraw   mm4,15 		
		psllw   mm0, 4			; level << 4
		psllw   mm3, 4			;		
		paddw   mm0, [inter_matrix1 + 8*esi+112] ;mm0 is to be divided inter1 contains fix for division by 1
		paddw   mm3, [inter_matrix1 + 8*esi+120]
		movq    mm5,[inter_matrix_fixl + 8*esi+112] 
		movq    mm7,[inter_matrix_fixl + 8*esi+120]
		pmulhuw mm5,mm0
		pmulhuw mm7,mm3  ;mm7: first approx of division
		mov esp,esp
		movq   mm2,[inter_matrix + 8*esi+112]
		movq   mm6,[inter_matrix + 8*esi+120]
		pmullw mm2,mm5 ;test value <= original
		pmullw mm6,mm7 
		psubw mm0,mm2 ;mismatch
		psubw mm3,mm6
		movq mm2,[byte ebx]
		movq mm6,[mmx_div + ecx * 8 - 8] ; divs for q<=16
		pmulhuw mm0,[inter_matrix_fix + 8*esi+112] ;correction
		pmulhuw mm3,[inter_matrix_fix + 8*esi+120]
		paddw mm2,[ebx+8]   ;sum		
		paddw mm5,mm0 ;final result
		paddw mm7,mm3
		movq mm0,[edi]
		movq mm3,[edi]
		pmulhuw	mm5, mm6		; mm0 = (mm0 / 2Q) >> 16
		pmulhuw	mm7, mm6		;  (level ) / quant (0<quant<32)
		add esi,byte 2
		psrlw   mm5, 1			; (level ) / (2*quant)
		paddw 	mm2,mm5 ;sum += x1
		psrlw   mm7, 1		
		movq 	[ebx],mm7 ;store x2		
		pxor	mm5, mm1		; mm0 *= sign(mm0)
		pxor	mm7, mm4		;
		psubw	mm5, mm1		; undisplace
		psubw	mm7, mm4		;
		db 0Fh, 7Fh, 54h, 23h, 08 ;movq 	[ebx+8],mm2 ;store sum
		movq	[edx + 8*esi+112-16], mm5
		movq	[edx + 8*esi +120-16], mm7
		jng 	near .lloop 
		jmp	near .done


;===========================================================================
;
; void dequant4_intra_mmx(int16_t *data,
;                    const int16_t const *coeff,
;                    const uint32_t quant,
;                    const uint32_t dcscalar);
;
;===========================================================================

  ;   Note: in order to saturate 'easily', we pre-shift the quantifier
  ; by 4. Then, the high-word of (coeff[]*matrix[i]*quant) are used to
  ; build a saturating mask. It is non-zero only when an overflow occured.
  ; We thus avoid packing/unpacking toward double-word.
  ; Moreover, we perform the mult (matrix[i]*quant) first, instead of, e.g.,
  ; (coeff[i]*matrix[i]). This is less prone to overflow if coeff[] are not
  ; checked. Input ranges are: coeff in [-127,127], inter_matrix in [1..255],a
  ; and quant in [1..31]. 
  ;
  ;********************************************************************
%macro DEQUANT4INTRAMMX 1
  movq mm1, [byte ecx+ 16 * %1]   ; mm0 = c  = coeff[i]
  movq mm4, [ecx+ 16 * %1 +8]; mm3 = c' = coeff[i+1]
  psubw mm0,mm1
  psubw mm3,mm4
  pmaxsw mm0,mm1
  pmaxsw mm3,mm4
  psraw mm1,15
  psraw mm4,15
%if %1  
  movq mm2,[eax+8]   ;preshifted quant
  movq mm7,[eax+8]  
%endif
  pmullw mm2,  [intra_matrix + 16 * %1 ]  ; matrix[i]*quant
  pmullw mm7,  [intra_matrix + 16 * %1 +8]  ; matrix[i+1]*quant
  movq mm5,mm0 
  movq mm6,mm3
  pmulhw mm0, mm2   ; high of coeff*(matrix*quant)
  pmulhw mm3, mm7   ; high of coeff*(matrix*quant)
  pmullw mm2, mm5   ; low  of coeff*(matrix*quant)
  pmullw mm7, mm6   ; low  of coeff*(matrix*quant)
  pcmpgtw mm0, [eax]
  pcmpgtw mm3, [eax]
  paddusw mm2, mm0
  paddusw mm7, mm3
  psrlw mm2, 5
  psrlw mm7, 5 
  pxor mm2, mm1  ; start negating back
  pxor mm7, mm4  ; start negating back
  psubusw mm1, mm0
  psubusw mm4, mm3
  movq mm0,[eax] ;zero
  movq mm3,[eax] ;zero
  psubw mm2, mm1 ; finish negating back  
  psubw mm7, mm4 ; finish negating back   
  movq [byte edx + 16 * %1], mm2   ; data[i]
  movq [edx + 16 * %1  +8], mm7   ; data[i+1]
%endmacro

align 16
cglobal dequant4_intra_3dne
dequant4_intra_3dne:

  mov eax, [esp+12] ; quant
  mov ecx, [esp+8]  ; coeff
  movq mm7, [mmx_mul_quant  + eax*8 - 8]
  psllw mm7, 2   ; << 2. See comment.
  mov edx, [esp+4]  ; data
	push ebx	
  movsx ebx,word [ecx]
  pxor mm0, mm0   
  pxor mm3, mm3   
  push esi
  lea eax,[esp-28]
  sub esp,byte 32
  and eax,byte -8 ;points to qword aligned space on stack
  movq [eax],mm0
  movq [eax+8],mm7
  imul ebx,[esp+16+8+32]    ; dcscalar  
  movq mm2,mm7
  

align 4
	DEQUANT4INTRAMMX 0
	mov esi,-2048
	nop
	cmp ebx,esi
	DEQUANT4INTRAMMX 1
	cmovl ebx,esi
	neg esi
	sub esi,byte 1 ;2047
	DEQUANT4INTRAMMX 2
	cmp ebx,esi
	cmovg ebx,esi
	lea ebp,[byte ebp]
	DEQUANT4INTRAMMX 3
    mov esi,[esp+32]
	  mov [byte edx], bx
	  mov ebx,[esp+32+4]
	DEQUANT4INTRAMMX 4
	DEQUANT4INTRAMMX 5
	DEQUANT4INTRAMMX 6
	DEQUANT4INTRAMMX 7
	  add esp,byte 32+8

 ret

;===========================================================================
;
; void dequant4_inter_3dne(int16_t * data,
;                    const int16_t * const coeff,
;                    const uint32_t quant);
;
;===========================================================================

    ; Note:  We use (2*c + sgn(c) - sgn(-c)) as multiplier
    ; so we handle the 3 cases: c<0, c==0, and c>0 in one shot.
    ; sgn(x) is the result of 'pcmpgtw 0,x':  0 if x>=0, -1 if x<0.
    ; It's mixed with the extraction of the absolute value.

align 16
cglobal dequant4_inter_3dne
dequant4_inter_3dne:

  mov    edx, [esp+ 4]        ; data
  mov    ecx, [esp+ 8]        ; coeff
  mov    eax, [esp+12]        ; quant
  movq mm7, [mmx_mul_quant  + eax*8 - 8]
  mov eax, -14
  paddw mm7, mm7    ; << 1
  pxor mm6, mm6 ; mismatch sum
  push esi
  mov esi,mmzero
  pxor mm1,mm1
  pxor mm3,mm3
  nop
  nop4

align 16
.loop
  movq mm0, [ecx+8*eax + 7*16   ]   ; mm0 = coeff[i]
  pcmpgtw mm1, mm0  ; mm1 = sgn(c)    (preserved)
  movq mm2, [ecx+8*eax + 7*16 +8]   ; mm2 = coeff[i+1]
  pcmpgtw mm3, mm2  ; mm3 = sgn(c')   (preserved)
  paddsw mm0, mm1   ; c += sgn(c)
  paddsw mm2, mm3   ; c += sgn(c')
  paddw mm0, mm0    ; c *= 2
  paddw mm2, mm2    ; c'*= 2

  movq mm4, [esi]
  movq mm5, [esi]
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
  nop
  pmullw mm4,  [inter_matrix + 8*eax + 7*16]
  movq mm5, mm4
  pmulhw mm5, mm0   ; high of c*(matrix*quant)
  pmullw mm0, mm4   ; low  of c*(matrix*quant)

  movq mm4, mm7     ; (matrix*quant)
  pmullw mm4,  [inter_matrix + 8*eax + 7*16 + 8]
  add eax,byte 2

  pcmpgtw mm5, [esi]
  paddusw mm0, mm5
  psrlw mm0, 5
  pxor mm0, mm1     ; start restoring sign
  psubusw mm1, mm5

  movq mm5, mm4
  pmulhw mm5, mm2   ; high of c*(matrix*quant)
  pmullw mm2, mm4   ; low  of c*(matrix*quant)
  psubw mm0, mm1    ; finish restoring sign

  pcmpgtw mm5, [esi]
  paddusw mm2, mm5
  psrlw mm2, 5
  pxor mm2, mm3    ; start restoring sign
  psubusw mm3, mm5
  psubw mm2, mm3   ; finish restoring sign
  movq mm1, [esi]
  movq mm3, [byte esi]
  pxor mm6, mm0     ; mismatch control
  movq [edx + 8*eax + 7*16 -2*8   ], mm0   ; data[i]
  pxor mm6, mm2     ; mismatch control
  movq [edx + 8*eax + 7*16 -2*8 +8], mm2   ; data[i+1]

  jng  .loop
  nop

  ; mismatch control

  pshufw mm0,mm6,01010101b 
  pshufw mm1,mm6,10101010b 
  pshufw mm2,mm6,11111111b 
  pxor mm6, mm0
  pxor mm1, mm2
  pxor mm6, mm1
  movd eax, mm6
  and eax,byte 1
  xor eax,byte 1
  mov esi,[esp]
  add esp,byte 4
  xor word [edx + 2*63], ax

  ret

