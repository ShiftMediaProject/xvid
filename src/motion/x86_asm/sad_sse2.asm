;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  sse2 sum of absolute difference
; *
; *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
; *  Copyright(C) 2002 Dmitry Rozhdestvensky
; *
; *  This program is an implementation of a part of one or more MPEG-4
; *  Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
; *  to use this software module in hardware or software products are
; *  advised that its use may infringe existing patents or copyrights, and
; *  any such use would be at such party's own risk.  The original
; *  developer of this software module and his/her company, and subsequent
; *  editors and their companies, will have no liability for use of this
; *  software or modifications or derivatives thereof.
; *
; *  This program is free software; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
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
; ****************************************************************************/

bits 32

%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%define sad_debug 0 ;1=unaligned 2=ref unaligned 3=aligned 0=autodetect
%define dev_debug 2 ;1=unaligned 2=aligned 0=autodetect
%define test_stride_alignment 0 ;test stride for alignment while autodetect
%define early_return 0 ;use early return in sad

section .data

align 64
buffer  times 4*8 dd 0   ;8 128-bit words
zero    times 4   dd 0

section .text

cglobal  sad16_sse2
cglobal  dev16_sse2

;===========================================================================
;               General macros for SSE2 code
;===========================================================================

%macro load_stride 1
                mov     ecx,%1
                add     ecx,ecx
                mov     edx,ecx
                add     ecx,%1          ;stride*3
                add     edx,edx         ;stride*4
%endmacro

%macro sad8lines 1

                psadbw  xmm0,[%1]
                psadbw  xmm1,[%1+ebx]
                psadbw  xmm2,[%1+ebx*2]
                psadbw  xmm3,[%1+ecx]

                add     %1,edx

                psadbw  xmm4,[%1]
                psadbw  xmm5,[%1+ebx]
                psadbw  xmm6,[%1+ebx*2]
                psadbw  xmm7,[%1+ecx]

                add     %1,edx
%endmacro

%macro after_sad 1 ; Summarizes 0th and 4th words of all xmm registers

                paddusw xmm0,xmm1
                paddusw xmm2,xmm3
                paddusw xmm4,xmm5
                paddusw xmm6,xmm7

                paddusw xmm0,xmm2
                paddusw xmm4,xmm6

                paddusw xmm4,xmm0
                pshufd  xmm5,xmm4,11111110b
                paddusw xmm5,xmm4

                pextrw  %1,xmm5,0       ;less latency then movd
%endmacro

%macro restore 1  ;restores used registers

%if %1=1
                pop ebp
%endif
                pop edi
                pop esi
                pop ebx
%endmacro

;===========================================================================
;
; uint32_t sad16_sse2 (const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
;
;===========================================================================

align 16
sad16_sse2
                push    ebx
                push    esi
                push    edi

                mov     ebx,[esp + 3*4 + 12]    ;stride

%if sad_debug<>0
                mov     edi,[esp + 3*4 + 4]
                mov     esi,[esp + 3*4 + 8]
%endif

%if sad_debug=1
                jmp     sad16_sse2_ul
%endif
%if sad_debug=2
                jmp     sad16_sse2_semial
%endif        
%if sad_debug=3
                jmp     sad16_sse2_al
%endif

%if test_stride_alignment<>0
                test    ebx,15
                jnz     sad16_sse2_ul
%endif
                mov     edi,[esp + 3*4 + 4]     ;cur (most likely aligned)

                test    edi,15
                cmovz   esi,[esp + 3*4 + 8]     ;load esi if edi is aligned
                cmovnz  esi,edi                 ;move to esi and load edi
                cmovnz  edi,[esp + 3*4 + 8]     ;if not
                jnz     esi_unaligned

                test    esi,15                     
                jnz     near sad16_sse2_semial           
                jmp     sad16_sse2_al

esi_unaligned:  test    edi,15
                jnz     near sad16_sse2_ul
                jmp     sad16_sse2_semial

;===========================================================================
;       Branch requires 16-byte alignment of esi and edi and stride
;===========================================================================

%macro sad16x8_al 1

                movdqa  xmm0,[esi]
                movdqa  xmm1,[esi+ebx]
                movdqa  xmm2,[esi+ebx*2]
                movdqa  xmm3,[esi+ecx]

                add     esi,edx

                movdqa  xmm4,[esi]
                movdqa  xmm5,[esi+ebx]
                movdqa  xmm6,[esi+ebx*2]
                movdqa  xmm7,[esi+ecx]

                add     esi,edx

                sad8lines edi

                after_sad %1

%endmacro

align 16
sad16_sse2_al

                load_stride ebx

                sad16x8_al eax

%if early_return=1
                cmp     eax,[esp + 3*4 + 16]    ;best_sad
                jg      continue_al
%endif

                sad16x8_al ebx

                add     eax,ebx

continue_al:    restore 0

                ret

;===========================================================================
;       Branch requires 16-byte alignment of the edi and stride only
;===========================================================================

%macro sad16x8_semial 1

                movdqu  xmm0,[esi]
                movdqu  xmm1,[esi+ebx]
                movdqu  xmm2,[esi+ebx*2]
                movdqu  xmm3,[esi+ecx]

                add     esi,edx

                movdqu  xmm4,[esi]
                movdqu  xmm5,[esi+ebx]
                movdqu  xmm6,[esi+ebx*2]
                movdqu  xmm7,[esi+ecx]

                add     esi,edx

                sad8lines edi

                after_sad %1

%endmacro

align 16
sad16_sse2_semial

                load_stride ebx

                sad16x8_semial eax

%if early_return=1
                cmp     eax,[esp + 3*4 + 16]    ;best_sad
                jg      cont_semial
%endif

                sad16x8_semial ebx

                add     eax,ebx

cont_semial:    restore 0

                ret


;===========================================================================
;               Branch does not require alignment, even stride
;===========================================================================

%macro sad16x4_ul 1

                movdqu  xmm0,[esi]
                movdqu  xmm1,[esi+ebx]
                movdqu  xmm2,[esi+ebx*2]
                movdqu  xmm3,[esi+ecx]

                add     esi,edx

                movdqu  xmm4,[edi]
                movdqu  xmm5,[edi+ebx]
                movdqu  xmm6,[edi+ebx*2]
                movdqu  xmm7,[edi+ecx]

                add     edi,edx

                psadbw  xmm4,xmm0
                psadbw  xmm5,xmm1
                psadbw  xmm6,xmm2
                psadbw  xmm7,xmm3

                paddusw xmm4,xmm5
                paddusw xmm6,xmm7

                paddusw xmm4,xmm6
                pshufd  xmm7,xmm4,11111110b
                paddusw xmm7,xmm4

                pextrw  %1,xmm7,0
%endmacro
                

align 16
sad16_sse2_ul

                load_stride ebx

                push ebp

                sad16x4_ul eax

%if early_return=1
                cmp     eax,[esp + 4*4 + 16]    ;best_sad
                jg      continue_ul
%endif

                sad16x4_ul ebp
                add     eax,ebp

%if early_return=1
                cmp     eax,[esp + 4*4 + 16]    ;best_sad
                jg      continue_ul
%endif

                sad16x4_ul ebp
                add     eax,ebp

%if early_return=1
                cmp     eax,[esp + 4*4 + 16]    ;best_sad
                jg      continue_ul
%endif

                sad16x4_ul ebp
                add     eax,ebp

continue_ul:    restore 1

                ret

;===========================================================================
;
; uint32_t dev16_sse2(const uint8_t * const cur,
;					const uint32_t stride);
;
; experimental!
;
;===========================================================================

align 16
dev16_sse2

                push    ebx
		push 	esi
		push 	edi
                push    ebp

                mov     esi, [esp + 4*4 + 4]      ; cur
                mov     ebx, [esp + 4*4 + 8]      ; stride
                mov     edi, buffer

%if dev_debug=1
                jmp     dev16_sse2_ul
%endif

%if dev_debug=2
                jmp     dev16_sse2_al
%endif

                test    esi,15
                jnz     near dev16_sse2_ul

%if test_stride_alignment=1
                test    ebx,15
                jnz     dev16_sse2_ul
%endif

                mov     edi,esi
                jmp     dev16_sse2_al

;===========================================================================
;               Branch requires alignment of both the cur and stride
;===========================================================================

%macro make_mean 0
                add     eax,ebp         ;mean 16-bit
                mov     al,ah           ;eax= {0 0 mean/256 mean/256}
                mov     ebp,eax
                shl     ebp,16
                or      eax,ebp
%endmacro

%macro sad_mean16x8_al 3        ;destination,0=zero,1=mean from eax,source

%if %2=0
                pxor    xmm0,xmm0
%else
                movd    xmm0,eax
                pshufd  xmm0,xmm0,0
%endif
                movdqa  xmm1,xmm0
                movdqa  xmm2,xmm0
                movdqa  xmm3,xmm0
                movdqa  xmm4,xmm0
                movdqa  xmm5,xmm0
                movdqa  xmm6,xmm0
                movdqa  xmm7,xmm0

                sad8lines %3

                after_sad %1

%endmacro

align 16
dev16_sse2_al

                load_stride ebx

                sad_mean16x8_al eax,0,esi
                sad_mean16x8_al ebp,0,esi

                make_mean

                sad_mean16x8_al ebp,1,edi
                sad_mean16x8_al eax,1,edi

                add eax,ebp

                restore 1

                ret

;===========================================================================
;               Branch does not require alignment
;===========================================================================

%macro sad_mean16x8_ul 2

                pxor    xmm7,xmm7

                movdqu  xmm0,[%1]
                movdqu  xmm1,[%1+ebx]
                movdqu  xmm2,[%1+ebx*2]
                movdqu  xmm3,[%1+ecx]

                add     %1,edx

                movdqa  [buffer+16*0],xmm0
                movdqa  [buffer+16*1],xmm1
                movdqa  [buffer+16*2],xmm2
                movdqa  [buffer+16*3],xmm3
                
                movdqu  xmm4,[%1]
                movdqu  xmm5,[%1+ebx]
                movdqu  xmm6,[%1+ebx*2]
                movdqa  [buffer+16*4],xmm4
                movdqa  [buffer+16*5],xmm5
                movdqa  [buffer+16*6],xmm6

                psadbw  xmm0,xmm7
                psadbw  xmm1,xmm7
                psadbw  xmm2,xmm7
                psadbw  xmm3,xmm7
                psadbw  xmm4,xmm7
                psadbw  xmm5,xmm7
                psadbw  xmm6,xmm7

                movdqu  xmm7,[%1+ecx]
                movdqa  [buffer+16*7],xmm7
                psadbw  xmm7,[zero]

                add     %1,edx

                after_sad %2
%endmacro

align 16
dev16_sse2_ul

                load_stride ebx

                sad_mean16x8_ul esi,eax
                sad_mean16x8_ul esi,ebp

                make_mean

                sad_mean16x8_al ebp,1,edi
                sad_mean16x8_al eax,1,edi

                add     eax,ebp

                restore 1

                ret
