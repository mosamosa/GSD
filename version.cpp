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
#include <winver.h>
#include "version.h"

#pragma comment(lib, "version.lib")

bool ReadVersionFile(LPCTSTR szFileName, sVersionInfo *info)
{
	bool bRet = false;

	DWORD cbBlock = GetFileVersionInfoSize(szFileName, 0);

	if(cbBlock){
		BYTE *pbBlock = new BYTE [cbBlock];

		if(GetFileVersionInfo(szFileName, 0, cbBlock, (LPVOID)pbBlock)){
			LPVOID pvBuf = NULL;
			UINT uLen = 0;

			if(VerQueryValue((const LPVOID)pbBlock, "\\", &pvBuf, &uLen)){
				VS_FIXEDFILEINFO *pvfi = (VS_FIXEDFILEINFO *)pvBuf;

				info->word.wVer1 = HIWORD(pvfi->dwFileVersionMS);
				info->word.wVer2 = LOWORD(pvfi->dwFileVersionMS);
				info->word.wVer3 = HIWORD(pvfi->dwFileVersionLS);
				info->word.wVer4 = LOWORD(pvfi->dwFileVersionLS);

				bRet = true;
			}
		}

		delete [] pbBlock;
	}

	return bRet;
}

bool ReadVersionModule(HMODULE hModule, sVersionInfo *info)
{
	TCHAR szPath[MAX_PATH];

	if(GetModuleFileName(hModule, szPath, MAX_PATH))
		return ReadVersionFile(szPath, info);

	return false;
}
