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

#ifndef COMMONH
#define COMMONH

#include "mmtimer.h"
#include "mutex.h"
#include "event.h"
#include "sharedmem.h"

#define DllExport extern "C" __declspec(dllexport)
#define SAFE_RELEASE(p) { if(p){ p->Release(); p = NULL; } }

#define CATCHANY_BEGIN try{
#define CATCHANY_END(STR) }catch(...){ dprintf("Catch exception: " STR); }

#define MUTEX_SETTINGS  "gsd_mutex"
#define MUTEX_MULTI     "gsd_mutex_multi"
#define SHARED_SETTINGS "gsd_settings"
#define SHARED_PIXELS   "gsd_pixels"
#define MAX_TEXNUM 32

#define NUMTEX_SIZE 64
#define NUMTEX_REALWIDTH 16
#define NUMTEX_DRAWWIDTH 14
#define NUMTEX_REALHEIGHT 21
#define NUMTEX_DRAWHEIGHT 21
#define NUMTEX_SPACER 2
#define NUMTEX_MARGIN 2
#define MAX_DISPLAYDIGIT 3		// FPS表示桁数

#define MAX_FPSNUM 10			// 何フレーム分記録するか
#define FPS_REFRESHTIME 0.125	// フレームレート描画間隔

//-----------------------------------------------------------------------------

struct sTexInfo
{
	BOOL active;
	DWORD texSize;	// 0ならテクスチャ削除
	DWORD offset;
	DWORD revision;	// 1～
	int x, y;
	DWORD color;
	DWORD align;
	DWORD reserve1, reserve2, reserve3, reserve4;
};

struct sSettings
{
	DWORD pixelsDataSize;
	DWORD pixelsRevision;	// 1～
	struct sTexInfo texInfo[MAX_TEXNUM];
	DWORD lastUpdate;		// [out]
	double fpsLimit;
	__int64 orgTime;
	BOOL showFPS;
	DWORD fpsAlign;
	BOOL updateOnActive;
	DWORD reserve1, reserve2, reserve3, reserve4;
};

//-----------------------------------------------------------------------------

typedef struct _FRECT
{
	float left, top, right, bottom;
}FRECT;

//-----------------------------------------------------------------------------

struct sGlobal
{
	CMicroTimer mtTimer;
	CMutex mutexSettings;
	CSharedMemory smSettings;
	CSharedMemory smPixels;

	__int64 pcList[MAX_FPSNUM];
	int pcListPos;
	__int64 beforeTime;
	double beforeFPS;
	FRECT rectNum[12];

	sSettings settings;
	HWND hwnd;

	sGlobal();

	void checkSettings(sSettings &_settings);
	void processFPS();
	double getFPS();
	void getFPSPos(int &x, int &y, int width, int height);
};

//-----------------------------------------------------------------------------

extern sGlobal *g;

//-----------------------------------------------------------------------------

extern void *offsetPresent9;
extern void *offsetPresentSC9;
extern void *offsetReset9;
extern void *offsetAddRef9;
extern void *offsetRelease9;
extern void *offsetPresent8;
extern void *offsetReset8;
extern void *offsetRelease8;
extern void *offsetAddRef8;

//-----------------------------------------------------------------------------

void FixSettings(sSettings &settings, int width, int height);

#endif
