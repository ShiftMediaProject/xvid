;/******************************************************************************
; *
; *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>
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
; * $Id: cpuid.asm,v 1.3 2002-11-17 00:51:11 edgomez Exp $
; ******************************************************************************/

bits 32

%define CPUID_TSC				0x00000010
%define CPUID_MMX				0x00800000
%define CPUID_SSE				0x02000000
%define CPUID_SSE2				0x04000000

%define EXT_CPUID_3DNOW			0x80000000
%define EXT_CPUID_AMD_3DNOWEXT	0x40000000
%define EXT_CPUID_AMD_MMXEXT	0x00400000

%define XVID_CPU_MMX			0x00000001
%define XVID_CPU_MMXEXT			0x00000002
%define XVID_CPU_SSE	        0x00000004
%define XVID_CPU_SSE2			0x00000008
%define XVID_CPU_3DNOW          0x00000010
%define XVID_CPU_3DNOWEXT		0x00000020
%define XVID_CPU_TSC            0x00000040


%macro cglobal 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

ALIGN 32

section .data

vendorAMD	db "AuthenticAMD"

%macro  CHECK_FEATURE         3

    mov     ecx, %1
    and     ecx, edx
    neg     ecx
    sbb     ecx, ecx
    and     ecx, %2
    or      %3, ecx

%endmacro

section .text

; int check_cpu_feature(void)

cglobal check_cpu_features
check_cpu_features:

	push ebx
	push esi
	push edi
	push ebp

	xor ebp,ebp

	; CPUID command ?
	pushfd
	pop		eax
	mov		ecx, eax
	xor		eax, 0x200000
	push	eax
	popfd
	pushfd
	pop		eax
	cmp		eax, ecx

	jz		near .cpu_quit		; no CPUID command -> exit


	; get vendor string, used later
    xor     eax, eax
    cpuid
    mov     [esp-12], ebx       ; vendor string
    mov     [esp-12+4], edx
    mov     [esp-12+8], ecx
    test    eax, eax

    jz      near .cpu_quit

    mov     eax, 1
    cpuid

    ; RDTSC command ?
	CHECK_FEATURE CPUID_TSC, XVID_CPU_TSC, ebp

    ; MMX support ?
	CHECK_FEATURE CPUID_MMX, XVID_CPU_MMX, ebp

    ; SSE support ?
	CHECK_FEATURE CPUID_SSE, (XVID_CPU_MMXEXT|XVID_CPU_SSE), ebp

	; SSE2 support?
	CHECK_FEATURE CPUID_SSE2, XVID_CPU_SSE2, ebp

	; extended functions?
    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000000
    jbe     near .cpu_quit

    mov     eax, 0x80000001
    cpuid

	; AMD cpu ?
    lea     esi, [vendorAMD]
    lea     edi, [esp-12]
    mov     ecx, 12
    cld
    repe    cmpsb
    jnz     .cpu_quit

    ; 3DNow! support ?
	CHECK_FEATURE EXT_CPUID_3DNOW, XVID_CPU_3DNOW, ebp

	; 3DNOW extended ?
	CHECK_FEATURE EXT_CPUID_AMD_3DNOWEXT, XVID_CPU_3DNOWEXT, ebp

	; extended MMX ?
	CHECK_FEATURE EXT_CPUID_AMD_MMXEXT, XVID_CPU_MMXEXT, ebp

.cpu_quit:

	mov eax, ebp

	pop ebp
	pop edi
	pop esi
	pop ebx

	ret



; sse/sse2 operating support detection routines
; these will trigger an invalid instruction signal if not supported.

cglobal sse_os_trigger
align 16
sse_os_trigger:
	xorps xmm0, xmm0
	ret


cglobal sse2_os_trigger
align 16
sse2_os_trigger:
	xorpd xmm0, xmm0
	ret

