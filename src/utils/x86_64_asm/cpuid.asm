;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - CPUID check processors capabilities -
; *
; *  Copyright (C) 2001 Michael Militzer <isibaar@xvid.org>
; *                2004 Andre Werthmann <wertmann@aei.mpg.de>
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
; * $Id: cpuid.asm,v 1.3 2008-08-19 09:06:48 Isibaar Exp $
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
; Constants
;=============================================================================

%define CPUID_TSC               0x00000010
%define CPUID_MMX               0x00800000
%define CPUID_SSE               0x02000000
%define CPUID_SSE2              0x04000000
%define CPUID_SSE3              0x00000001

%define EXT_CPUID_3DNOW         0x80000000
%define EXT_CPUID_AMD_3DNOWEXT  0x40000000
%define EXT_CPUID_AMD_MMXEXT    0x00400000

;;; NB: Make sure these defines match the ones defined in xvid.h
%define XVID_CPU_MMX      (1<< 0)
%define XVID_CPU_MMXEXT   (1<< 1)
%define XVID_CPU_SSE      (1<< 2)
%define XVID_CPU_SSE2     (1<< 3)
%define XVID_CPU_SSE3     (1<< 8)
%define XVID_CPU_3DNOW    (1<< 4)
%define XVID_CPU_3DNOWEXT (1<< 5)
%define XVID_CPU_TSC      (1<< 6)

;=============================================================================
; Read only data
;=============================================================================

ALIGN 64
%ifdef FORMAT_COFF
SECTION .rodata
%else
SECTION .rodata align=16
%endif

vendorAMD:
		db "AuthenticAMD"

;=============================================================================
; Macros
;=============================================================================

%macro  CHECK_FEATURE 4
  mov rax, %1
  and rax, %4
  neg rax
  sbb rax, rax
  and rax, %2
  or %3, rax
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text align=16

; int check_cpu_feature(void)
; NB:
; in theory we know x86_64 CPUs support mmx, mmxext, sse, sse2 but
; for security sake, when intel cpus will come with amd64 support
; it will be necessary to check if 3dnow can be used or not...
; so better use cpuid, even if it's mostly ignored for now.

cglobal check_cpu_features
check_cpu_features:

  push rbx
  push rbp

  sub rsp, 12             ; Stack space for vendor name
  
  xor rbp, rbp

	; get vendor string, used later
  xor rax, rax
  cpuid
  mov [rsp], ebx       ; vendor string
  mov [rsp+4], edx
  mov [rsp+8], ecx
  test rax, rax

  jz near .cpu_quit

 ; NB: we don't test for cpuid support like in ia32, we know
 ;     it is supported.
  mov rax, 1
  cpuid

 ; RDTSC command ?
  CHECK_FEATURE CPUID_TSC, XVID_CPU_TSC, rbp, rdx

  ; MMX support ?
  CHECK_FEATURE CPUID_MMX, XVID_CPU_MMX, rbp, rdx

  ; SSE support ?
  CHECK_FEATURE CPUID_SSE, (XVID_CPU_MMXEXT|XVID_CPU_SSE), rbp, rdx

  ; SSE2 support?
  CHECK_FEATURE CPUID_SSE2, XVID_CPU_SSE2, rbp, rdx

  ; SSE3 support?
  CHECK_FEATURE CPUID_SSE3, XVID_CPU_SSE3, rbp, rcx

  ; extended functions?
  mov rax, 0x80000000
  cpuid
  cmp rax, 0x80000000
  jbe near .cpu_quit

  mov rax, 0x80000001
  cpuid

 ; AMD cpu ?
  lea rsi, [vendorAMD wrt rip]
  lea rdi, [rsp]
  mov rcx, 12
  cld
  repe cmpsb
  jnz .cpu_quit

  ; 3DNow! support ?
  CHECK_FEATURE EXT_CPUID_3DNOW, XVID_CPU_3DNOW, rbp, rdx

  ; 3DNOW extended ?
  CHECK_FEATURE EXT_CPUID_AMD_3DNOWEXT, XVID_CPU_3DNOWEXT, rbp, rdx

  ; extended MMX ?
  CHECK_FEATURE EXT_CPUID_AMD_MMXEXT, XVID_CPU_MMXEXT, rbp, rdx

.cpu_quit:

  mov rax, rbp

  add rsp, 12

  pop rbp
  pop rbx

  ret
.endfunc

; sse/sse2 operating support detection routines
; these will trigger an invalid instruction signal if not supported.
ALIGN 16
cglobal sse_os_trigger
sse_os_trigger:
  xorps xmm0, xmm0
  ret
.endfunc


ALIGN 16
cglobal sse2_os_trigger
sse2_os_trigger:
  xorpd xmm0, xmm0
  ret
.endfunc


; enter/exit mmx state
ALIGN 16
cglobal emms_mmx
emms_mmx:
  emms
  ret
.endfunc

; faster enter/exit mmx state
ALIGN 16
cglobal emms_3dn
emms_3dn:
  femms
  ret
.endfunc



%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif


%ifidn __OUTPUT_FORMAT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

