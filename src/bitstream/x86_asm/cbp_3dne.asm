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


; these 3dne functions are compatible with iSSE, but are optimized specifically for 
; K7 pipelines
;
;------------------------------------------------------------------------------
; 09.12.2002  Athlon optimizations contributed by Jaan Kalda 
;------------------------------------------------------------------------------

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

section .text

cglobal calc_cbp_3dne

;===========================================================================
;
; uint32_t calc_cbp_3dne(const int16_t coeff[6][64]);
;
;===========================================================================
%macro calc_cbp 1
  pshufw	mm0, [eax], 229 ; =11100101
  movq		mm1, [eax+8]
  por     	mm0, [eax+64]
  por     	mm1, [eax+72]
  movq		mm2, [eax+16]
  movq		mm3, [eax+24]
  por     	mm2, [eax+80]
  por     	mm3, [eax+88]
  movq		mm4, [eax+32]
  movq		mm5, [eax+40]
  por     	mm4, [eax+96]
  por     	mm5, [eax+104]
  movq		mm6, [eax+48]
  movq		mm7, [eax+56]  
  por     	mm6, [eax+112]
  por     	mm7, [eax+120]
  por     	mm1, mm0
%if %1 
  sub		eax,byte -128 ;ecx ;+= 128; needed 3 bytes for alignment
%else
  xor	eax,eax
  xor	edx,edx
%endif  
  por     	mm3, mm2
  por     	mm5, mm4
  por     	mm7, mm6
  por     	mm3, mm1
  por     	mm7, mm5
  por     	mm7, mm3
  packsswb	mm7,mm7
  movd		[esp+%1*4],mm7
%endmacro



align 16 ;AMD K7, in cache: ca 80 clk
calc_cbp_3dne:
   mov   eax, [esp+ 4]	; coeff
  lea	esp,[esp-24]
  calc_cbp 5 ;bit 5
  calc_cbp 4 ;b4 
  calc_cbp 3 ;b3
  calc_cbp 2 ;b2
  calc_cbp 1 ;b1
  calc_cbp 0 ;b0
  cmp	eax,[esp+5*4]
  adc	eax,eax
  cmp	edx,[esp+4*4]
  adc	eax,eax
  cmp	edx,[esp+3*4]
  adc	eax,eax
  cmp	edx,[esp+2*4]
  adc	eax,eax
  cmp	edx,[esp+1*4]
  adc	eax,eax
  cmp	edx,[esp+0*4]
  adc	eax,eax
  add	esp,byte 24
  ret