#ifndef _PORTAB_H_
#define _PORTAB_H_


// debug level masks
#define DPRINTF_ERROR		0x00000001
#define DPRINTF_STARTCODE	0x00000002
#define DPRINTF_HEADER		0x00000004
#define DPRINTF_TIMECODE	0x00000008
#define DPRINTF_MB			0x00000010
#define DPRINTF_COEFF		0x00000020
#define DPRINTF_MV			0x00000040
#define DPRINTF_DEBUG		0x80000000

// debug level
#define DPRINTF_LEVEL		0


#define DPRINTF_BUF_SZ  1024


#if defined(WIN32)

#include <windows.h>
#include <stdio.h>

static __inline void
DPRINTF(int level, char *fmt,
		...)
{
	if ((DPRINTF_LEVEL & level))
	{
		va_list args;
		char buf[DPRINTF_BUF_SZ];

		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		OutputDebugString(buf);
		fprintf(stdout, "%s\n", buf);
		fflush(stdout);
	}
}


#define DEBUGCBR(A,B,C) { char tmp[100]; wsprintf(tmp, "CBR: frame: %i, quant: %i, deviation: %i\n", (A), (B), (C)); OutputDebugString(tmp); }

#ifdef _DEBUG
#define DEBUG(S) OutputDebugString((S));
#define DEBUG1(S,I) { char tmp[100]; wsprintf(tmp, "%s %i\n", (S), (I)); OutputDebugString(tmp); }
#define DEBUG2(X,A,B) { char tmp[100]; wsprintf(tmp, "%s %i %i\n", (X), (A), (B)); OutputDebugString(tmp); }
#define DEBUG3(X,A,B,C){ char tmp[1000]; wsprintf(tmp,"%s %i %i %i",(X),(A), (B), (C)); OutputDebugString(tmp); }
#define DEBUG4(X,A,B,C,D){ char tmp[1000]; wsprintf(tmp,"%s %i %i %i %i",(X),(A), (B), (C), (D)); OutputDebugString(tmp); }
#define DEBUG8(X,A,B,C,D,E,F,G,H){ char tmp[1000]; wsprintf(tmp,"%s %i %i %i %i %i %i %i %i",(X),(A),(B),(C),(D),(E),(F),(G),(H)); OutputDebugString(tmp); }
#else
#define DEBUG(S)
#define DEBUG1(S,I)
#define DEBUG2(X,A,B)
#define DEBUG3(X,A,B,C)
#define DEBUG4(X,A,B,C,D)
#define DEBUG8(X,A,B,C,D,E,F,G,H)
#endif


#define int8_t char
#define uint8_t unsigned char
#define int16_t short
#define uint16_t unsigned short
#define int32_t int
#define uint32_t unsigned int
#define int64_t __int64
#define uint64_t unsigned __int64
#define ptr_t uint32_t

#define EMMS() __asm {emms}

#define CACHE_LINE  16

#if _MSC_VER <= 1200
#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
	type * name = (type *) (((int32_t) name##_storage+(alignment - 1)) & ~((int32_t)(alignment)-1))
#else
#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	__declspec(align(alignment)) type name[(sizex)*(sizey)]
#endif

// needed for bitstream.h
#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax

// needed for timer.c
static __inline int64_t
read_counter()
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

#elif defined(LINUX) || defined(DJGPP) || defined(FREEBSD)

#include <stdio.h>
#include <stdarg.h>

static __inline void
DPRINTF(int level, char *fmt,
		...)
{
	if ((DPRINTF_LEVEL & level)) {
		va_list args;
		char buf[DPRINTF_BUF_SZ];

		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		fprintf(stdout, "%s\n", buf);
	}
}

#ifdef _DEBUG

#include <stdio.h>
#define DEBUG_WHERE               stdout
#define DEBUG(S)                  fprintf(DEBUG_WHERE, "%s\n", (S));
#define DEBUG1(S,I)               fprintf(DEBUG_WHERE, "%s %i\n", (S), (I))
#define DEBUG2(S,A,B)             fprintf(DEBUG_WHERE, "%s%i=%i\n", (S), (A), (B))
#define DEBUG3(S,A,B,C)           fprintf(DEBUG_WHERE, "%s %i %x %x\n", (S), (A), (B), (C))
#define DEBUG8(S,A,B,C,D,E,F,G,H)
#define DEBUGCBR(A,B,C)           fprintf(DEBUG_WHERE, "CBR: frame: %i, quant: %i, deviation: %i\n", (A), (B), (C))
#else
#define DEBUG(S)
#define DEBUG1(S,I)
#define DEBUG2(X,A,B)
#define DEBUG3(X,A,B,C)
#define DEBUG8(X,A,B,C,D,E,F,G,H)
#define DEBUGCBR(A,B,C)
#endif

#if defined(LINUX)

#include <stdint.h>

#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	type name##_storage[(sizex)*(sizey)+(alignment)-1]; \
	type * name = (type *) (((ptr_t) name##_storage+(alignment - 1)) & ~((ptr_t)(alignment)-1))

#else

#define DECLARE_ALIGNED_MATRIX(name,sizex,sizey,type,alignment) \
	__attribute__ ((__aligned__(CACHE_LINE))) type name[(sizex)*(sizey)]

#define int8_t   char
#define uint8_t  unsigned char
#define int16_t  short
#define uint16_t unsigned short
#define int32_t  int
#define uint32_t unsigned int
#define int64_t  long long
#define uint64_t unsigned long long

#endif


// needed for bitstream.h
#ifdef ARCH_PPC
#define BSWAP(a) __asm__ __volatile__ ( "lwbrx %0,0,%1; eieio" : "=r" (a) : \
		"r" (&(a)), "m" (a));
#define EMMS()

static __inline unsigned long
get_tbl(void)
{
	unsigned long tbl;
	asm volatile ("mftb %0":"=r" (tbl));

	return tbl;
}
static __inline unsigned long
get_tbu(void)
{
	unsigned long tbl;
	asm volatile ("mftbu %0":"=r" (tbl));

	return tbl;
}
static __inline int64_t
read_counter()
{
	unsigned long tb, tu;

	do {
		tu = get_tbu();
		tb = get_tbl();
	} while (tb != get_tbl());
	return (((int64_t) tu) << 32) | (int64_t) tb;
}

#define ptr_t   uint32_t

#define CACHE_LINE 16

#elif defined(ARCH_IA64)

#define ptr_t   uint64_t

#define CACHE_LINE 32

#define EMMS()

// needed for bitstream.h
#define BSWAP(a) \
	 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))

// rdtsc command most likely not supported,
// so just dummy code here
static __inline int64_t read_counter() {
	return 0;
}

#else
#define BSWAP(a) __asm__ ( "bswapl %0\n" : "=r" (a) : "0" (a) )
#define EMMS() __asm__("emms\n\t")


// needed for timer.c
static __inline int64_t
read_counter()
{
	int64_t ts;
	uint32_t ts1, ts2;

	__asm__ __volatile__("rdtsc\n\t":"=a"(ts1),
						 "=d"(ts2));

	ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);

	return ts;
}

#define ptr_t   uint32_t

#define CACHE_LINE 16

#endif

#else							// OTHER OS


#include <stdio.h>
#include <stdarg.h>

static __inline void
DPRINTF(int level, char *fmt, ...)
{
	if ((DPRINTF_LEVEL & level)) {

		va_list args;
		char buf[DPRINTF_BUF_SZ];

		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		fprintf(stdout, "%s\n", buf);
	}
}


#define DEBUG(S)
#define DEBUG1(S,I)
#define DEBUG2(X,A,B)
#define DEBUG3(X,A,B,C)
#define DEBUG8(X,A,B,C,D,E,F,G,H)
#define DEBUGCBR(A,B,C)

#include <inttypes.h>

#define EMMS()

// needed for bitstream.h
#define BSWAP(a) \
	 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))

// rdtsc command most likely not supported,
// so just dummy code here
static __inline int64_t
read_counter()
{
	return 0;
}

#define ptr_t uint32_t

#define CACHE_LINE  16
#define CACHE_ALIGN

#endif

#endif							// _PORTAB_H_
