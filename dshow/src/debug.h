#ifndef _DSHOW_DEBUG_
#define _DSHOW_DEBUG_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static __inline void OutputDebugStringf(char *fmt, ...)
{
	va_list args;
	char buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	strcat(buf, "\n");
	OutputDebugString(buf);
	va_end(args);
}

#ifdef _DEBUG
#define DPRINTF OutputDebugStringf
#else
static __inline void 
DPRINTF(char *fmt, ...) { }
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DSHOW_DEBUG */
