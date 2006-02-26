#include "debug.h"
#include <stdarg.h>
#include <windows.h>

#include <stdio.h>	/* vsprintf */
#define DPRINTF_BUF_SZ  1024

void OutputDebugStringf(char *fmt, ...)
{
	va_list args;
	char buf[DPRINTF_BUF_SZ];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	OutputDebugString(buf);
}
