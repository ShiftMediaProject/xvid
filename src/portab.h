/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Portable macros, types and inlined assembly -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *               2002 Peter Ross <pross@xvid.org>
 *               2002 Edouard Gomez <ed.gomez@wanadoo.fr>
 *
 *  This file is part of XviD, a free MPEG-4 video encoder/decoder
 *
 *  XviD is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Under section 8 of the GNU General Public License, the copyright
 *  holders of XVID explicitly forbid distribution in the following
 *  countries:
 *
 *    - Japan
 *    - United States of America
 *
 *  Linking XviD statically or dynamically with other modules is making a
 *  combined work based on XviD.  Thus, the terms and conditions of the
 *  GNU General Public License cover the whole combination.
 *
 *  As a special exception, the copyright holders of XviD give you
 *  permission to link XviD with independent modules that communicate with
 *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
 *  license terms of these independent modules, and to copy and distribute
 *  the resulting combined work under terms of your choice, provided that
 *  every copy of the combined work is accompanied by a complete copy of
 *  the source code of XviD (the version of XviD used to produce the
 *  combined work), being distributed under the terms of the GNU General
 *  Public License plus this exception.  An independent module is a module
 *  which is not derived from or based on XviD.
 *
 *  Note that people who make modified versions of XviD are not obligated
 *  to grant this special exception for their modified versions; it is
 *  their choice whether to do so.  The GNU General Public License gives
 *  permission to release a modified version without this exception; this
 *  exception also makes it possible to release a modified version which
 *  carries forward this exception.
 *
 * $Id: portab.h,v 1.36 2002-11-23 18:11:58 chl Exp $
 *
 ****************************************************************************/

#ifndef _PORTAB_H_
#define _PORTAB_H_

/*****************************************************************************
 *  Common things
 ****************************************************************************/

/* Debug level masks */
#define DPRINTF_ERROR		0x00000001
#define DPRINTF_STARTCODE	0x00000002
#define DPRINTF_HEADER		0x00000004
#define DPRINTF_TIMECODE	0x00000008
#define DPRINTF_MB			0x00000010
#define DPRINTF_COEFF		0x00000020
#define DPRINTF_MV			0x00000040
#define DPRINTF_DEBUG		0x80000000

/* debug level for this library */
#define DPRINTF_LEVEL		0

/* Buffer size for non C99 compliant compilers (msvc) */
#define DPRINTF_BUF_SZ  1024

/*****************************************************************************
 *  Types used in XviD sources
 ****************************************************************************/

/*----------------------------------------------------------------------------
 | Standard Unix include file (sorry, we put all unix into "linux" case)
 *---------------------------------------------------------------------------*/

#if defined(LINUX) || defined(BEOS) || defined(FREEBSD)

/* All (u)int(size)_t types are defined here */
#    include <inttypes.h>

/*----------------------------------------------------------------------------
 | msvc (lacks such a header file)
 *---------------------------------------------------------------------------*/

#elif defined(_MSC_VER)
#    define int8_t   char
#    define uint8_t  unsigned char
#    define int16_t  short
#    define uint16_t unsigned short
#    define int32_t  int
#    define uint32_t unsigned int
#    define int64_t  __int64
#    define uint64_t unsigned __int64

/*----------------------------------------------------------------------------
 | Fallback when using gcc
 *---------------------------------------------------------------------------*/

#elif defined(__GNUC__) || defined(__ICC__)

#    define int8_t   char
#    define uint8_t  unsigned char
#    define int16_t  short
#    define uint16_t unsigned short
#    define int32_t  int
#    define uint32_t unsigned int
#    define int64_t  long long
#    define uint64_t unsigned long long

/*----------------------------------------------------------------------------
 | Ok, we don't know how to define these types... error
 *---------------------------------------------------------------------------*/

#else
#    error Do not know how to define (u)int(size)_t types
#endif

/*****************************************************************************
 *  Some things that are only architecture dependant
 ****************************************************************************/

#if defined(ARCH_X86) || defined(ARCH_PPC) || defined(ARCH_MIPS)  || defined(ARCH_SPARC)
#    define CACHE_LINE  16
#    define ptr_t uint32_t
#elif defined(ARCH_IA64)
#    define CACHE_LINE  32
#    define ptr_t uint64_t
#else
#    error Architecture not supported.
#endif

/*****************************************************************************
 *  Things that must be sorted by compiler and then by architecture
 ****************************************************************************/

/*****************************************************************************
 *  MSVC compiler specific macros, functions
 ****************************************************************************/

#if defined(_MSC_VER)

/*----------------------------------------------------------------------------
 | Common msvc stuff
 *---------------------------------------------------------------------------*/

#include <windows.h>
#include <stdio.h>

    /*
     * This function must be declared/defined all the time because MSVC does
     * not support C99 variable arguments macros
     */
    static __inline void DPRINTF(int level, char *fmt, ...)
    {
        if (DPRINTF_LEVEL & level) {
            va_list args;
            char buf[DPRINTF_BUF_SZ];
            va_start(args, fmt);
            vsprintf(buf, fmt, args);
            OutputDebugString(buf);
            fprintf(stderr, "%s\n", buf);
         }
     }

#    if _MSC_VER <= 1200
#        define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
                type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
                type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))
#    else
#        define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
                __declspec(align(alignment)) type name[(sizex)*(sizey)]
#    endif


/*----------------------------------------------------------------------------
 | msvc x86 specific macros/functions
 *---------------------------------------------------------------------------*/
#    if defined(ARCH_X86)
#        define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax
#        define EMMS() __asm {emms}

             static __inline int64_t read_counter(void)
             {
                 int64_t ts;
                 uint32_t ts1, ts2;
                 __asm {
                     rdtsc
                     mov ts1, eax
                     mov ts2, edx
                 }
                 ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
                 return ts;
             }

/*----------------------------------------------------------------------------
 | msvc unknown architecture
 *---------------------------------------------------------------------------*/
#    else
#        error Architecture not supported.
#    endif




/*****************************************************************************
 *  GNU CC compiler stuff
 ****************************************************************************/

#elif defined(__GNUC__) || defined(__ICC__) /* Compiler test */

/*----------------------------------------------------------------------------
 | Common gcc stuff
 *---------------------------------------------------------------------------*/

/*
 * As gcc is (mostly) C99 compliant, we define DPRINTF only if it's realy needed
 * and it's a macro calling fprintf directly
 */
#    ifdef _DEBUG

        /* Needed for all debuf fprintf calls */
#       include <stdio.h>

#       define DPRINTF(level, format, ...) \
            do {\
                if(DPRINTF_LEVEL & level)\
                    fprintf(stderr, format"\n", ##__VA_ARGS__);\
            }while(0);

#    else /* _DEBUG */
#        define DPRINTF(level, format, ...)
#    endif /* _DEBUG */



#    define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
            type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
            type * name = (type *) (((ptr_t) name##_storage+(alignment - 1)) & ~((ptr_t)(alignment)-1))

/*----------------------------------------------------------------------------
 | gcc x86 specific macros/functions
 *---------------------------------------------------------------------------*/
#    if defined(ARCH_X86)
#        define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) );
#        define EMMS() __asm__ ("emms\n\t");

         static __inline int64_t read_counter(void)
         {
             int64_t ts;
             uint32_t ts1, ts2;
             __asm__ __volatile__("rdtsc\n\t":"=a"(ts1), "=d"(ts2));
             ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
             return ts;
         }

/*----------------------------------------------------------------------------
 | gcc PPC and PPC Altivec specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_PPC)
#        define BSWAP(a) __asm__ __volatile__ \
                ( "lwbrx %0,0,%1; eieio" : "=r" (a) : "r" (&(a)), "m" (a));
#        define EMMS()

         static __inline unsigned long get_tbl(void)
         {
             unsigned long tbl;
             asm volatile ("mftb %0":"=r" (tbl));
             return tbl;
         }

         static __inline unsigned long get_tbu(void)
         {
             unsigned long tbl;
             asm volatile ("mftbu %0":"=r" (tbl));
             return tbl;
         }

         static __inline int64_t read_counter(void)
         {
             unsigned long tb, tu;
             do {
                 tu = get_tbu();
                 tb = get_tbl();
             }while (tb != get_tbl());
             return (((int64_t) tu) << 32) | (int64_t) tb;
         }

/*----------------------------------------------------------------------------
 | gcc IA64 specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_IA64)
#        define BSWAP(a)  __asm__ __volatile__ \
                ("mux1 %1 = %0, @rev" ";;" \
                 "shr.u %1 = %1, 32" : "=r" (a) : "r" (a));
#        define EMMS()

         static __inline int64_t read_counter(void) {
             unsigned long result;
             __asm__ __volatile__("mov %0=ar.itc" : "=r"(result) :: "memory");
             return result;
         }

/*----------------------------------------------------------------------------
 | gcc SPARC specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_SPARC)
#        define BSWAP(a) \
                ((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
                       (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))
#        define EMMS()

         static __inline int64_t read_counter(void)
         {
             return 0;
         }

/*----------------------------------------------------------------------------
 | gcc MIPS specific macros/functions
 *---------------------------------------------------------------------------*/
#    elif defined(ARCH_MIPS)
#        define BSWAP(a) \
                ((a) = (((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | \
                       (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))
#        define EMMS()

         static __inline int64_t read_counter(void)
         {
             return 0;
         }

/*----------------------------------------------------------------------------
 | XviD + gcc unsupported Architecture
 *---------------------------------------------------------------------------*/
#    else
#        error Architecture not supported.
#    endif /* Architecture checking */

/*****************************************************************************
 *  Unknown compiler
 ****************************************************************************/
#else /* Compiler test */

#    error Compiler not supported

#endif /* Compiler test */


#endif
