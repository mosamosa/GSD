/*
 * GSD
 *
 * Copyright (C) 2008 mosa ◆e5bW6vDOJ. <pcyp4g@gmail.com>
 * 
 * This is free software with ABSOLUTELY NO WARRANTY.
 * 
 * You can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License.
 *
 */

#include <windows.h>
#include <stdio.h>

#include "common.h"
#include "tools.h"
#include "version.h"
#include "gsd.h"

#include "hook.h"
#include "hookd3d9.h"
#include "hookd3d8.h"
#include "hookogl.h"

//-----------------------------------------------------------------------------

#pragma data_seg(".sharedata")
static HHOOK hHook = NULL;
BOOL dllActive = FALSE;
#pragma data_seg()

//-----------------------------------------------------------------------------

HINSTANCE g_hInstance = NULL;

static bool hooked9 = false;
static bool hooked8 = false;
static bool hookedOGL = false;
static DWORD dwCheckTimer = 0;

static struct sLocal *lo = NULL;

//-----------------------------------------------------------------------------

typeSetWindowsHookExW extSetWindowsHookExW = NULL;
void *addrSetWindowsHookExW = NULL;

__declspec(naked) HHOOK WINAPI extSetWindowsHookExWXP(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId)
{
	__asm
	{
		push	ebp  
		mov		ebp, esp
		jmp		addrSetWindowsHookExW		
	}
}

//-----------------------------------------------------------------------------

bool InitSub();

VOID CALLBACK CheckTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if(!hooked9)
	{
		if(GetModuleHandle("d3d9.dll") != NULL)
		{
			dprintf("Direct3D9 hook...");

			InitSub();

			SetHookD3D9();
			hooked9 = true;
		}
	}

	if(!hooked8)
	{
		if(GetModuleHandle("d3d8.dll") != NULL)
		{
			dprintf("Direct3D8 hook...");

			InitSub();

			SetHookD3D8();
			hooked8 = true;
		}
	}

	if(!hookedOGL)
	{
		if(GetModuleHandle("opengl32.dll") != NULL)
		{
			dprintf("OpenGL hook...");

			InitSub();

			SetHookOGL();
			hookedOGL = true;
		}
	}

	if(hooked9)
		CheckHookD3D9();
	if(hooked8)
		CheckHookD3D8();
	if(hookedOGL)
		CheckHookOGL();
}

//-----------------------------------------------------------------------------

bool Init()
{	// プロセスに読み込まれたとき呼び出される初期化 (1回)
	bool succeed = false;

	CheckTimerProc(NULL, 0, 0, 0);

	do
	{
		dwCheckTimer = SetTimer(NULL, 0, 1000, CheckTimerProc);
		if(dwCheckTimer == 0)
		{
			dprintf("SetTimer() failed.");
			break;
		}

		succeed = true;

	}while(false);

	return succeed;
}

//-----------------------------------------------------------------------------

bool InitSub()
{	// Hookされるときに呼出される初期化 (複数回)
	if(g == NULL)
	{
		g = new sGlobal;

		CMicroTimer::calcError();
	}

	return true;
}

//-----------------------------------------------------------------------------

void UninitSub()
{	// Hookが解除されたときに呼出される (1回)
	if(g != NULL)
		delete g;
	g = NULL;
}

//-----------------------------------------------------------------------------

void Uninit()
{	// プロセスからアンロードされるとき呼び出される (1回)
	KillTimer(NULL, dwCheckTimer);

	if(hooked9 || hooked8|| hookedOGL)
	{
		if(hooked9)
			ResetHookD3D9();
		if(hooked8)
			ResetHookD3D8();
		if(hookedOGL)
			ResetHookOGL();

		UninitSub();

		hooked9 = false;
		hooked8 = false;
		hookedOGL = false;
	}
}

//-----------------------------------------------------------------------------

static bool initiated = false;

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	OSVERSIONINFO ver;
	void *baseAddr;

	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hInstance;
		DisableThreadLibraryCalls(hInstance);

		LoadLibrary("user32.dll");

		baseAddr = GetProcAddress(GetModuleHandle("user32.dll"), "SetWindowsHookExW");

		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&ver);

		if(ver.dwMajorVersion == 5)
		{
			if(ver.dwMinorVersion > 0)
			{
				addrSetWindowsHookExW = (char *)baseAddr + 5;
				extSetWindowsHookExW = extSetWindowsHookExWXP;
			}
		}

		if(ver.dwMajorVersion == 6)
		{
			addrSetWindowsHookExW = (char *)baseAddr + 5;
			extSetWindowsHookExW = extSetWindowsHookExWXP;
		}
		
		if(extSetWindowsHookExW == NULL)
			extSetWindowsHookExW = (typeSetWindowsHookExW)baseAddr;

		{
			bool pass = true;
			char sztemp[MAX_PATH*2], windir[MAX_PATH*2];

			GetWindowsDirectory(windir, MAX_PATH*2);
			GetModuleFileName(GetModuleHandle(NULL), sztemp, MAX_PATH*2);

			if(dllActive == FALSE)
				pass = false;

			if(strstr(sztemp, windir))
				pass = false;

			if(strstr(sztemp, "rrtest.exe"))
				pass = false;

			if(pass)
			{
				dprintf("Inject %s (%d)", sztemp, GetCurrentProcessId());

				Init();
				initiated = true;
			}
		}
		break;
	case DLL_PROCESS_DETACH:

		if(initiated)
			Uninit();

		break;
	}

    return TRUE;
}

//-----------------------------------------------------------------------------

LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

//-----------------------------------------------------------------------------

BOOL WINAPI GSD_Initialize()
{
	int n1 = 128, n2 = 128, n3 = 128;

	if(lo)
		return FALSE;

	lo = new sLocal;

	if(lo->mutexMulti.isAlreadyExists())
	{
		delete lo;
		lo = NULL;

		return FALSE;
	}

	GetOffsetD3D9(&n1);
	GetOffsetD3D8(&n2);
	GetOffsetOGL(&n3);

	lo->maxTextureSize = min(n1, min(n2, n3));

	hHook = extSetWindowsHookExW(WH_CBT, HookProc, g_hInstance, 0);
	if(!hHook)
	{
		dprintf("SetWindowsHook() failed.");

		delete lo;
		lo = NULL;

		return FALSE;
	}

	dllActive = TRUE;

	return TRUE;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_Finalize()
{
	dllActive = FALSE;

	if(lo)
	{
		delete lo;
		lo = NULL;
	}

	if(hHook)
	{
		UnhookWindowsHookEx(hHook);
		hHook = NULL;
	}
}

//-----------------------------------------------------------------------------

int WINAPI GSD_GetMaxTextureNum()
{
	return MAX_TEXNUM;
}

//-----------------------------------------------------------------------------

int WINAPI GSD_GetMaxTextureSize()
{
	if(!lo)
		return 0;

	return lo->maxTextureSize;
}

//-----------------------------------------------------------------------------

DWORD WINAPI GSD_GetLastUpdate()
{
	if(!lo || !lo->settings)
		return 0;

	return lo->settings->lastUpdate;
}

//-----------------------------------------------------------------------------

DWORD WINAPI GSD_GetVersion()
{
	sVersionInfo info;

	if(ReadVersionModule(g_hInstance, &info))
	{
		DWORD out;

		out = ((info.word.wVer1&0xff)<<24) | ((info.word.wVer2&0xff)<<16) | ((info.word.wVer3&0xff)<<8) | (info.word.wVer4&0xff);

		return out;
	}
	else
	{
		return 0;
	}
}

//-----------------------------------------------------------------------------

DWORD WINAPI GSD_GetApiVersion()
{
	return GSD_APIVER;
}

//-----------------------------------------------------------------------------

BOOL WINAPI GSD_CheckApiVersion(DWORD ver)
{
	WORD hi, lo, hi2, lo2;

	hi = HIWORD(GSD_APIVER);
	lo = LOWORD(GSD_APIVER);

	hi2 = HIWORD(ver);
	lo2 = LOWORD(ver);

	if(hi != hi2)
		return FALSE;

	if(lo < lo2)
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------- 
BOOL WINAPI GSD_InitTexture(const DWORD *texSize, int num)
{
	int i;
	DWORD total;
	char sztemp[256];

	if(!lo || !lo->settings)
		return FALSE;

	if(!texSize || num < 1)
		return FALSE;

	lo->offsets[0] = 0;
	lo->texSizes[0] = texSize[0];

	for(i=1; i<num; i++)
	{
		lo->offsets[i] = lo->offsets[i-1] + texSize[i-1]*texSize[i-1]*4;	// 32bit
		lo->texSizes[i] = texSize[i];
	}

	for(; i<MAX_TEXNUM; i++)
	{
		lo->offsets[i] = 0;
		lo->texSizes[i] = 0;

		lo->settings->texInfo[i].texSize = 0;
	}

	total = 0;
	for(i=0; i<num; i++)
		total += texSize[i]*texSize[i]*4;	// 32bit

	lo->settings->pixelsRevision++;

	sprintf(sztemp, "%s%d", SHARED_PIXELS, lo->settings->pixelsRevision);

	if(lo->smPixels.create(sztemp, total))
	{
		lo->settings->pixelsDataSize = total;
		lo->maxIndex = num;
	}
	else
	{
		lo->settings->pixelsDataSize = 0;
		lo->maxIndex = 0;
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL WINAPI GSD_SetTexture(int index, const struct GSD_TextureInfo *info)
{
	if(!lo || !lo->settings)
		return FALSE;

	if(!info || index < 0 || index >= MAX_TEXNUM)
		return FALSE;

	if(index >= lo->maxIndex && info->data)
		return FALSE;

	if(!lo->smPixels.getData() && info->data)
		return FALSE;

	if(lo->texSizes[index] != info->texSize)
		return FALSE;

	lo->settings->texInfo[index].active = info->active;
	lo->settings->texInfo[index].align = info->align;
	lo->settings->texInfo[index].color = info->color;
	lo->settings->texInfo[index].x = info->x;
	lo->settings->texInfo[index].y = info->y;
	lo->settings->texInfo[index].texSize = info->texSize;

	if(info->data)
	{
		BYTE *p;

		lo->settings->texInfo[index].offset = lo->offsets[index];
		lo->settings->texInfo[index].revision++;

		p = (BYTE*)lo->smPixels.getData() + lo->offsets[index];
		if(info->data != p)
			memcpy(p, info->data, lo->texSizes[index]*lo->texSizes[index]*4);	// 32bit
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

BOOL WINAPI GSD_GetTexture(int index, struct GSD_TextureInfo *info)
{
	if(!lo || !lo->settings)
		return FALSE;

	if(!info || index < 0 || index >= MAX_TEXNUM)
		return FALSE;

	info->active = lo->settings->texInfo[index].active;
	info->align = lo->settings->texInfo[index].align;
	info->color = lo->settings->texInfo[index].color;
	info->x = lo->settings->texInfo[index].x;
	info->y = lo->settings->texInfo[index].y;
	info->texSize = lo->texSizes[index];

	if(index < lo->maxIndex && lo->smPixels.getData())
	{
		info->data = (BYTE*)lo->smPixels.getData() + lo->offsets[index];
	}
	else
	{
		info->data = NULL;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_DelTexture(int index)
{
	if(!lo || !lo->settings)
		return;

	if(index < 0 || index >= MAX_TEXNUM)
		return;

	lo->settings->texInfo[index].texSize = 0;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_SetTimerOrg(__int64 org)
{
	if(!lo || !lo->settings)
		return;

	lo->settings->orgTime = org;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_SetFpsLimit(double fps)
{
	if(!lo || !lo->settings)
		return;

	if(fps < 0.0)
		return;

	lo->settings->fpsLimit = fps;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_SetFpsAlign(DWORD align)
{
	if(!lo || !lo->settings)
		return;

	lo->settings->fpsAlign = align;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_ShowFps(BOOL show)
{
	if(!lo || !lo->settings)
		return;

	lo->settings->showFPS = show;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_UpdateOnActive(BOOL uoa)
{
	if(!lo || !lo->settings)
		return;

	lo->settings->updateOnActive = uoa;
}

//-----------------------------------------------------------------------------

void WINAPI GSD_DataLock()
{
	if(!lo)
		return;

	lo->mutexSettings.lock();
}

//-----------------------------------------------------------------------------

void WINAPI GSD_DataUnlock()
{
	if(!lo)
		return;

	lo->mutexSettings.unlock();
}

//-----------------------------------------------------------------------------
