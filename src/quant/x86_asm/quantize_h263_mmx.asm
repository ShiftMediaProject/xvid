;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MPEG4 Quantization H263 implementation / MMX optimized -
; *
; *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
; *               2002-2003 Pascal Massimino <skal@planet-d.net>
; *               2004      Jean-Marc Bastide <jmtest@voila.fr>
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
; * $Id: quantize_h263_mmx.asm,v 1.3 2004-04-12 15:49:56 edgomez Exp $
; *
; ****************************************************************************/

; enable dequant saturate [-2048,2047], test purposes only.
%define SATURATE

BITS 32

%macro cglobal 1
       %ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

;=============================================================================
; Read only Local data
;=============================================================================

%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata data align=16
%endif

ALIGN 16
plus_one:
	times 8 dw 1

;-----------------------------------------------------------------------------
;
; quant table
;
;-----------------------------------------------------------------------------

ALIGN 16
mmx_quant:
%assign quant 0
%rep 32
	times 4 dw quant
	%assign quant quant+1
%endrep

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

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal quant_h263_intra_mmx
cglobal quant_h263_intra_sse2
cglobal quant_h263_inter_mmx
cglobal quant_h263_inter_sse2
cglobal dequant_h263_intra_mmx
cglobal dequant_h263_intra_xmm
cglobal dequant_h263_intra_sse2
cglobal dequant_h263_inter_mmx
cglobal dequant_h263_inter_xmm
cglobal dequant_h263_inter_sse2

;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_mmx(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint32_t dcscalar,
;                               const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
quant_h263_intra_mmx:

  push esi

  mov esi, [esp + 4 + 8]     ; data
 
  movsx eax, word [esi]      ; data[0]
 
  mov ecx,[esp + 4 + 16]         ; dcscalar
  mov edx,eax
  sar ecx,1
  add eax,ecx
  sub edx,ecx
  cmovl eax,edx             ; +/- dcscalar/2
  mov ecx, [esp + 4 + 12]    ; quant
  cdq 
  idiv dword [esp + 4 + 16]  ; dcscalar
  cmp ecx, 1
  mov edx, [esp + 4 + 4]     ; coeff
  je .low
 
  movq mm7, [mmx_div+ecx * 8 - 8]
  mov ecx,4

.loop
  movq mm0, [esi]           ; data      
  pxor mm4,mm4
  movq mm1, [esi + 8]
  pcmpgtw mm4,mm0           ; (data<0)
  pxor mm5,mm5
  pmulhw mm0,mm7            ; /(2*quant)
  pcmpgtw mm5,mm1
  movq mm2, [esi+16] 
  psubw mm0,mm4             ;  +(data<0)
  pmulhw mm1,mm7
  pxor mm4,mm4
  movq mm3,[esi+24]
  pcmpgtw mm4,mm2
  psubw mm1,mm5 
  pmulhw mm2,mm7 
  pxor mm5,mm5
  pcmpgtw mm5,mm3
  pmulhw mm3,mm7
  psubw mm2,mm4
  psubw mm3,mm5  
  movq [edx], mm0
  lea esi, [esi+32]
  movq [edx + 8], mm1
  movq [edx + 16], mm2
  movq [edx + 24], mm3
   
  dec ecx
  lea edx, [edx+32]
  jne .loop
  jmp .end
   
.low
  movd mm7,ecx
  mov ecx,4
.loop_low  
  movq mm0, [esi]           
  pxor mm4,mm4
  movq mm1, [esi + 8]
  pcmpgtw mm4,mm0
  pxor mm5,mm5
  psubw mm0,mm4
  pcmpgtw mm5,mm1
  psraw mm0,mm7
  psubw mm1,mm5 
  movq mm2,[esi+16]
  pxor mm4,mm4
  psraw mm1,mm7
  pcmpgtw mm4,mm2
  pxor mm5,mm5
  psubw mm2,mm4
  movq mm3,[esi+24]
  pcmpgtw mm5,mm3
  psraw mm2,mm7
  psubw mm3,mm5
  movq [edx], mm0
  psraw mm3,mm7
  movq [edx + 8], mm1
  movq [edx+16],mm2
  lea esi, [esi+32]
  movq [edx+24],mm3
  
  dec ecx
  lea edx, [edx+32]
  jne .loop_low
  
.end
  mov edx, [esp + 4 + 4]     ; coeff
  mov [edx],ax  
  xor eax,eax                ; return 0

  pop esi
  ret
 

;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_sse2(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint32_t dcscalar,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
quant_h263_intra_sse2:

  push esi

  mov esi, [esp + 4 + 8]     ; data
 
  movsx eax, word [esi]      ; data[0]
 
  mov ecx,[esp + 4 + 16]     ; dcscalar
  mov edx,eax
  sar ecx,1
  add eax,ecx
  sub edx,ecx
  cmovl eax,edx              ; +/- dcscalar/2
  mov ecx, [esp + 4 + 12]    ; quant
  cdq 
  idiv dword [esp + 4 + 16]  ; dcscalar
  cmp ecx, 1
  mov edx, [esp + 4 + 4]     ; coeff
  movq xmm7, [mmx_div+ecx * 8 - 8]
  je .low
  
  mov ecx,2
  movlhps xmm7,xmm7

.loop
  movdqa xmm0, [esi]           
  pxor xmm4,xmm4
  movdqa xmm1, [esi + 16]
  pcmpgtw xmm4,xmm0 
  pxor xmm5,xmm5
  pmulhw xmm0,xmm7
  pcmpgtw xmm5,xmm1
  movdqa xmm2, [esi+32] 
  psubw xmm0,xmm4
  pmulhw xmm1,xmm7
  pxor xmm4,xmm4
  movdqa xmm3,[esi+48]
  pcmpgtw xmm4,xmm2
  psubw xmm1,xmm5 
  pmulhw xmm2,xmm7 
  pxor xmm5,xmm5
  pcmpgtw xmm5,xmm3
  pmulhw xmm3,xmm7
  psubw xmm2,xmm4
  psubw xmm3,xmm5  
  movdqa [edx], xmm0
  lea esi, [esi+64]
  movdqa [edx + 16], xmm1
  movdqa [edx + 32], xmm2
  movdqa [edx + 48], xmm3
   
  dec ecx
  lea edx, [edx+64]
  jne .loop
  jmp .end
   
.low
  movd xmm7,ecx
  mov ecx,2
.loop_low  
  movdqa xmm0, [esi]           
  pxor xmm4,xmm4
  movdqa xmm1, [esi + 16]
  pcmpgtw xmm4,xmm0
  pxor xmm5,xmm5
  psubw xmm0,xmm4
  pcmpgtw xmm5,xmm1
  psraw xmm0,xmm7
  psubw xmm1,xmm5 
  movdqa xmm2,[esi+32]
  pxor xmm4,xmm4
  psraw xmm1,xmm7
  pcmpgtw xmm4,xmm2
  pxor xmm5,xmm5
  psubw xmm2,xmm4
  movdqa xmm3,[esi+48]
  pcmpgtw xmm5,xmm3
  psraw xmm2,xmm7
  psubw xmm3,xmm5
  movdqa [edx], xmm0
  psraw xmm3,xmm7
  movdqa [edx+16], xmm1
  movdqa [edx+32],xmm2
  lea esi, [esi+64]
  movdqa [edx+48],xmm3
  
  dec ecx
  lea edx, [edx+64]
  jne .loop_low
  
.end
  mov edx, [esp + 4 + 4]     ; coeff
  mov [edx],ax  
  xor eax,eax                ; return 0

  pop esi
  ret
 
;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_mmx(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
  
ALIGN 16
quant_h263_inter_mmx:

  push ecx
  push esi
  push edi

  mov edi, [esp + 12 + 4]           ; coeff
  mov esi, [esp + 12 + 8]           ; data
  mov eax, [esp + 12 + 12]          ; quant

  xor ecx, ecx

  pxor mm5, mm5                     ; sum
  movq mm6, [mmx_sub + eax * 8 - 8] ; sub

  cmp al, 1
  jz .q1loop

  movq mm7, [mmx_div + eax * 8 - 8] ; divider

ALIGN 8
.loop
  movq mm0, [esi + 8*ecx]           ; mm0 = [1st]
  movq mm3, [esi + 8*ecx + 8]
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
  movq [edi + 8*ecx], mm0
  movq [edi + 8*ecx + 8], mm3

  add ecx, 2
  cmp ecx, 16
  jnz .loop

.done
  pmaddwd mm5, [plus_one]
  movq mm0, mm5
  psrlq mm5, 32
  paddd mm0, mm5

  movd eax, mm0     ; return sum
  pop edi
  pop esi
  pop ecx

  ret

ALIGN 8
.q1loop
  movq mm0, [esi + 8*ecx]           ; mm0 = [1st]
  movq mm3, [esi + 8*ecx+ 8]        ;
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
  movq [edi + 8*ecx], mm0
  movq [edi + 8*ecx + 8], mm3

  add ecx, 2
  cmp ecx, 16
  jnz .q1loop

  jmp .done



;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_sse2(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
quant_h263_inter_sse2:

  push esi
  push edi

  mov edi, [esp + 8 + 4]      ; coeff
  mov esi, [esp + 8 + 8]      ; data
  mov eax, [esp + 8 + 12]     ; quant

  xor ecx, ecx

  pxor xmm5, xmm5                           ; sum

  movq mm0, [mmx_sub + eax*8 - 8]           ; sub
  movq2dq xmm6, mm0                         ; load into low 8 bytes
  movlhps xmm6, xmm6                        ; duplicate into high 8 bytes

  cmp al, 1
  jz near .qes2_q1loop

.qes2_not1
  movq mm0, [mmx_div + eax*8 - 8]           ; divider
  movq2dq xmm7, mm0
  movlhps xmm7, xmm7

ALIGN 16
.qes2_loop
  movdqa xmm0, [esi + ecx*8]                ; xmm0 = [1st]
  movdqa xmm3, [esi + ecx*8 + 16]           ; xmm3 = [2nd]
  pxor xmm1, xmm1
  pxor xmm4, xmm4
  pcmpgtw xmm1, xmm0
  pcmpgtw xmm4, xmm3
  pxor xmm0, xmm1
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  psubusw xmm0, xmm6
  psubusw xmm3, xmm6
  pmulhw xmm0, xmm7
  pmulhw xmm3, xmm7
  paddw xmm5, xmm0
  pxor xmm0, xmm1
  paddw xmm5, xmm3
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  movdqa [edi + ecx*8], xmm0
  movdqa [edi + ecx*8 + 16], xmm3

  add ecx, 4
  cmp ecx, 16
  jnz .qes2_loop

.qes2_done
  movdqu xmm6, [plus_one]
  pmaddwd xmm5, xmm6
  movhlps xmm6, xmm5
  paddd xmm5, xmm6
  movdq2q mm0, xmm5

  movq mm5, mm0
  psrlq mm5, 32
  paddd mm0, mm5

  movd eax, mm0         ; return sum

  pop edi
  pop esi

  ret

ALIGN 16
.qes2_q1loop
  movdqa xmm0, [esi + ecx*8]        ; xmm0 = [1st]
  movdqa xmm3, [esi + ecx*8 + 16]   ; xmm3 = [2nd]
  pxor xmm1, xmm1
  pxor xmm4, xmm4
  pcmpgtw xmm1, xmm0
  pcmpgtw xmm4, xmm3
  pxor xmm0, xmm1
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  psubusw xmm0, xmm6
  psubusw xmm3, xmm6
  psrlw xmm0, 1
  psrlw xmm3, 1
  paddw xmm5, xmm0
  pxor xmm0, xmm1
  paddw xmm5, xmm3
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  movdqa [edi + ecx*8], xmm0
  movdqa [edi + ecx*8 + 16], xmm3

  add ecx, 4
  cmp ecx, 16
  jnz .qes2_q1loop
  jmp .qes2_done


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_mmx(int16_t *data,
;                                 const int16_t const *coeff,
;                                 const uint32_t quant,
;                                 const uint32_t dcscalar,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
dequant_h263_intra_mmx:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff
  pcmpeqw mm0,mm0
  movq mm6, [mmx_quant + ecx*8]     ; quant
  shl ecx,31                        ; quant & 1 ? 0 : - 1
  movq mm7,mm6
  movq mm5,mm0
  movd mm1,ecx
  mov edx, [esp+ 4]                 ; data
  psllw mm0,mm1
  paddw mm7,mm7                     ; 2*quant
  paddw mm6,mm0                     ; quant-1
  psllw mm5,12
  mov ecx,8
  psrlw mm5,1

.loop: 
  movq mm0,[eax] 
  pxor mm2,mm2
  pxor mm4,mm4
  pcmpgtw mm2,mm0
  pcmpeqw mm4,mm0
  pmullw mm0,mm7      ; * 2 * quant  
  movq mm1,[eax+8]
  psubw mm0,mm2 
  pxor mm2,mm6
  pxor mm3,mm3
  pandn mm4,mm2
  pxor mm2,mm2
  pcmpgtw mm3,mm1
  pcmpeqw mm2,mm1
  pmullw mm1,mm7
  paddw mm0,mm4
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddsw mm0, mm5        ; saturate
  paddw mm1,mm2
   
  paddsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  paddsw mm0, mm5
  paddsw mm1, mm5
  
  movq [edx],mm0
  lea eax,[eax+16]
  movq [edx+8],mm1
 
  dec ecx
  lea edx,[edx+16]
  jne .loop
  
   ; deal with DC
  mov eax, [esp+ 8]                ; coeff
  movsx eax,word [eax]
  mov ecx,2047
  imul dword [esp+16]              ; dcscalar
  mov edx, [esp+ 4]                ; data
  cmp eax,ecx
  cmovg eax,ecx
  not ecx
  cmp eax,ecx
  cmovl eax,ecx
 
  mov [edx], ax

  xor eax, eax                    ; return 0
  ret

;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_xmm(int16_t *data,
;                                 const int16_t const *coeff,
;                                 const uint32_t quant,
;                                 const uint32_t dcscalar,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

  
ALIGN 16 
dequant_h263_intra_xmm:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff

  movd mm6,ecx                      ; quant
  pcmpeqw mm0,mm0
  pshufw mm6,mm6,0                  ; all quant
  shl ecx,31
  movq mm5,mm0
  movq mm7,mm6
  movd mm1,ecx
  mov edx, [esp+ 4]                 ; data
  psllw mm0,mm1                     ; quant & 1 ? 0 : - 1
  movq mm4,mm5
  paddw mm7,mm7                     ; quant*2
  paddw mm6,mm0                     ; quant-1
  psrlw mm4,5                       ; mm4=2047
  mov ecx,8
  pxor mm5,mm4                      ; mm5=-2048
  
.loop:
  movq mm0,[eax] 
  pxor mm2,mm2
  pxor mm3,mm3

  pcmpgtw mm2,mm0
  pcmpeqw mm3,mm0     ; if coeff==0...
  pmullw mm0,mm7      ; * 2 * quant
  movq mm1,[eax+8]
  
  psubw mm0,mm2
  pxor mm2,mm6
  pandn mm3,mm2       ; ...then data=0
  pxor mm2,mm2
  paddw mm0,mm3
  pxor mm3,mm3
  pcmpeqw mm2,mm1
  pcmpgtw mm3,mm1
  pmullw mm1,mm7
   
  pminsw mm0,mm4
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddw mm1,mm2
  
  pmaxsw mm0,mm5
  pminsw mm1,mm4
  movq [edx],mm0
  pmaxsw mm1,mm5
  lea eax,[eax+16]
  movq [edx+8],mm1
  
  dec ecx
  lea edx,[edx+16]
  jne .loop
  
   ; deal with DC
  mov eax, [esp+ 8]                 ; coeff
  mov edx, [esp+ 4]                 ; data
  movd mm0, [eax]
  pmullw mm0, [esp+16]              ; dcscalar
  pminsw mm0,mm4
  pmaxsw mm0,mm5
  movd eax, mm0
  mov [edx], ax

  xor eax, eax                      ; return 0
  ret


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_sse2(int16_t *data,
;                                  const int16_t const *coeff,
;                                  const uint32_t quant,
;                                  const uint32_t dcscalar,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16 
dequant_h263_intra_sse2:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff
 
  movd xmm6,ecx                     ; quant
;  shr ecx,1
  shl ecx,31
  pshuflw xmm6,xmm6,0
  pcmpeqw xmm0,xmm0
  movlhps xmm6,xmm6                 ; all quant
  movd xmm1,ecx
  movdqa xmm5,xmm0
  movdqa xmm7,xmm6
  mov edx, [esp+ 4]                 ; data
  paddw xmm7,xmm7                   ; quant *2
  psllw xmm0,xmm1                   ; quant & 1 ? 0 : - 1 
  movdqa xmm4,xmm5
  paddw xmm6,xmm0                   ; quant-1
  psrlw xmm4,5                      ; 2047
  mov ecx,4
  pxor xmm5,xmm4                    ; mm5=-2048
  
.loop:
  movdqa xmm0,[eax] 
  pxor xmm2,xmm2
  pxor xmm3,xmm3

  pcmpgtw xmm2,xmm0
  pcmpeqw xmm3,xmm0
  pmullw xmm0,xmm7      ; * 2 * quant
  movdqa xmm1,[eax+16]
  
  psubw xmm0,xmm2
  pxor xmm2,xmm6
  pandn xmm3,xmm2
  pxor xmm2,xmm2
  paddw xmm0,xmm3
  pxor xmm3,xmm3
  pcmpeqw xmm2,xmm1
  pcmpgtw xmm3,xmm1
  pmullw xmm1,xmm7
   
  pminsw xmm0,xmm4
  psubw xmm1,xmm3
  pxor xmm3,xmm6
  pandn xmm2,xmm3
  paddw xmm1,xmm2
  
  pmaxsw xmm0,xmm5
  pminsw xmm1,xmm4
  movdqa [edx],xmm0
  pmaxsw xmm1,xmm5
  lea eax,[eax+32]
  movdqa [edx+16],xmm1
 
  dec ecx
  lea edx,[edx+32]
  jne .loop
    
   ; deal with DC

  mov eax, [esp+ 8]             ; coeff
  movsx eax,word [eax]
  imul dword [esp+16]            ; dcscalar
  mov edx, [esp+ 4]              ; data
  movd xmm0,eax
  pminsw xmm0,xmm4
  pmaxsw xmm0,xmm5
  movd eax,xmm0
  
  mov [edx], ax

  xor eax, eax              ; return 0
  ret

;-----------------------------------------------------------------------------
;
; uint32t dequant_h263_inter_mmx(int16_t * data,
;                                const int16_t * const coeff,
;                                const uint32_t quant,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
dequant_h263_inter_mmx:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff
  pcmpeqw mm0,mm0
  movq mm6, [mmx_quant + ecx*8]     ; quant
  shl ecx,31                        ; odd/even
  movq mm7,mm6
  movd mm1,ecx
  mov edx, [esp+ 4]                 ; data
  movq mm5,mm0
  psllw mm0,mm1                     ; quant & 1 ? 0 : - 1
  paddw mm7,mm7                     ; quant*2
  paddw mm6,mm0                     ; quant & 1 ? quant : quant - 1
  psllw mm5,12
  mov ecx,8
  psrlw mm5,1                       ; 32767-2047 (32768-2048)

.loop:
  movq mm0,[eax] 
  pxor mm4,mm4
  pxor mm2,mm2
  pcmpeqw mm4,mm0     ; if coeff==0...
  pcmpgtw mm2,mm0
  pmullw mm0,mm7      ; * 2 * quant
  pxor mm3,mm3
  psubw mm0,mm2 
  movq mm1,[eax+8]
  pxor mm2,mm6
  pcmpgtw mm3,mm1
  pandn mm4,mm2      ; ... then data==0
  pmullw mm1,mm7
  pxor mm2,mm2
  pcmpeqw mm2,mm1
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddw mm0,mm4
  paddw mm1,mm2
    
  paddsw mm0, mm5        ; saturate
  paddsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  paddsw mm0, mm5
  paddsw mm1, mm5
  
  movq [edx],mm0
  lea eax,[eax+16]
  movq [edx+8],mm1
 
  dec ecx
  lea edx,[edx+16]
  jne .loop
  
  xor eax, eax              ; return 0
  ret


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_xmm(int16_t * data,
;                                 const int16_t * const coeff,
;                                 const uint32_t quant,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
ALIGN 16 
dequant_h263_inter_xmm:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff
  pcmpeqw mm0,mm0
  movq mm6, [mmx_quant + ecx*8]     ; quant
  shl ecx,31
  movq mm5,mm0
  movd mm1,ecx
  movq mm7,mm6
  psllw mm0,mm1
  mov edx, [esp+ 4]                 ; data
  movq mm4,mm5
  paddw mm7,mm7
  paddw mm6,mm0                     ; quant-1

  psrlw mm4,5
  mov ecx,8
  pxor mm5,mm4                      ; mm5=-2048
   
.loop:
  movq mm0,[eax] 
  pxor mm3,mm3
  pxor mm2,mm2
  pcmpeqw mm3,mm0
  pcmpgtw mm2,mm0
  pmullw mm0,mm7                    ; * 2 * quant
  pandn mm3,mm6
  movq mm1,[eax+8]
  psubw mm0,mm2 
  pxor mm2,mm3
  pxor mm3,mm3
  paddw mm0,mm2
  pxor mm2,mm2
  pcmpgtw mm3,mm1
  pcmpeqw mm2,mm1
  pmullw mm1,mm7
  pandn mm2,mm6
  psubw mm1,mm3
  pxor mm3,mm2
  paddw mm1,mm3
  
  pminsw mm0,mm4
  pminsw mm1,mm4
  pmaxsw mm0,mm5
  pmaxsw mm1,mm5
    
  movq [edx],mm0
  lea eax,[eax+16]
  movq [edx+8],mm1
 
  dec ecx
  lea edx,[edx+16]
  jne .loop

  xor eax, eax              ; return 0
  ret

 
;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_sse2(int16_t * data,
;                                  const int16_t * const coeff,
;                                  const uint32_t quant,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN 16
dequant_h263_inter_sse2:

  mov ecx, [esp+12]                 ; quant
  mov eax, [esp+ 8]                 ; coeff

  movq xmm6, [mmx_quant + ecx*8]    ; quant
  inc ecx
  pcmpeqw xmm5,xmm5
  and ecx,1
  movlhps xmm6,xmm6
  movd xmm0,ecx
  movdqa xmm7,xmm6
  pshuflw xmm0,xmm0,0
  movdqa xmm4,xmm5
  mov edx, [esp+ 4]                 ; data
  movlhps xmm0,xmm0
  paddw xmm7,xmm7
  psubw xmm6,xmm0
  psrlw xmm4,5   ; 2047
  mov ecx,4
  pxor xmm5,xmm4 ; mm5=-2048
  
.loop:
  movdqa xmm0,[eax] 
  pxor xmm3,xmm3
  pxor xmm2,xmm2
  pcmpeqw xmm3,xmm0
  pcmpgtw xmm2,xmm0
  pmullw xmm0,xmm7      ; * 2 * quant
  pandn xmm3,xmm6
  movdqa xmm1,[eax+16]
  psubw xmm0,xmm2 
  pxor xmm2,xmm3
  pxor xmm3,xmm3
  paddw xmm0,xmm2
  pxor xmm2,xmm2
  pcmpgtw xmm3,xmm1
  pcmpeqw xmm2,xmm1
  pmullw xmm1,xmm7
  pandn xmm2,xmm6
  psubw xmm1,xmm3
  pxor xmm3,xmm2
  paddw xmm1,xmm3
  
  pminsw xmm0,xmm4
  pminsw xmm1,xmm4
  pmaxsw xmm0,xmm5
  pmaxsw xmm1,xmm5
    
  movdqa [edx],xmm0
  lea eax,[eax+32]
  movdqa [edx+16],xmm1
 
  dec ecx
  lea edx,[edx+32]
  jne .loop

  xor eax, eax              ; return 0
  ret

