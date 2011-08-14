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

#ifndef HOOKH
#define HOOKH

#include <windows.h>

#include "common.h"
#include "mutex.h"
#include "sharedmem.h"

struct sLocal
{
	CMutex mutexSettings;
	CMutex mutexMulti;
	CSharedMemory smSettings;
	CSharedMemory smPixels;

	DWORD offsets[MAX_TEXNUM];
	DWORD texSizes[MAX_TEXNUM];
	int maxTextureSize;
	int maxIndex;

	sSettings *settings;

	sLocal() :
		mutexSettings(MUTEX_SETTINGS),
		mutexMulti(MUTEX_MULTI)
	{
		int i;

		maxTextureSize = 0;
		maxIndex = 0;

		smSettings.create(SHARED_SETTINGS, sizeof(sSettings));
		settings = (sSettings *)smSettings.getData();

		for(i=0; i<MAX_TEXNUM; i++)
		{
			offsets[i] = 0;
			texSizes[i] = 0;
		}
	}
};

extern HINSTANCE g_hInstance;
extern BOOL dllActive;

typedef HHOOK (WINAPI *typeSetWindowsHookExW)(int, HOOKPROC, HINSTANCE, DWORD);
extern typeSetWindowsHookExW extSetWindowsHookExW;

#endif
