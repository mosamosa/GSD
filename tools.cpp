/*
 * GSD
 *
 * Copyright (C) 2008 mosa Åüe5bW6vDOJ. <pcyp4g@gmail.com>
 * 
 * This is free software with ABSOLUTELY NO WARRANTY.
 * 
 * You can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License.
 *
 */

#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "tools.h"

#define BUFLEN 512

//-----------------------------------------------------------------------------

int dprintf(const char *format, ...)
{
	va_list ap;
	char buffer[BUFLEN];
	int iret;

	va_start(ap, format);
	iret = vsprintf(buffer, format, ap);
	va_end(ap);

    OutputDebugString(buffer);

	return iret;
}

//-----------------------------------------------------------------------------

double pc2time(__int64 t)
{
	static __int64 freq = 1;

	if(freq == 1)
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

	if(freq > 0)
		return ((double)t / (double)freq);
	else
		return 0.0;
}

//-----------------------------------------------------------------------------

double round(double n)
{
	int i;
	bool plus = n > 0.0;

	n = fabs(n);
	i = (int)n;
	n -= (double)i;

	if(n >= 0.5)
		return plus ? (double)(i+1) : -(double)(i+1);
	else
		return plus ? (double)i : -(double)i;
}

//-----------------------------------------------------------------------------
