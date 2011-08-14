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

#ifndef VERSIONH
#define VERSIONH

#include <windows.h>

typedef struct{
	union{
		unsigned __int64 qwVer;

		struct{
			WORD wVer4;
			WORD wVer3;
			WORD wVer2;
			WORD wVer1;
		}word;
	};
}sVersionInfo;

bool ReadVersionFile(LPCTSTR szFileName, sVersionInfo *info);
bool ReadVersionModule(HMODULE hModule, sVersionInfo *info);

#endif