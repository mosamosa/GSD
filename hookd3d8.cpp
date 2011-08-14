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

#pragma warning (disable: 4786)

#include <windows.h>
#include <time.h>
#include <d3d8.h>
#include <string>
#include <math.h>
#include <map>
#include <process.h>
#include <atlbase.h>

#include "common.h"
#include "tools.h"
#include "mutex.h"
#include "apihook.h"

#include "hook.h"
#include "hookd3d.h"
#include "hookd3d8.h"

//-----------------------------------------------------------------------------

typedef struct _sDevInfoD3D<IDirect3DDevice8, IDirect3DTexture8> sDevInfoD3D8;

typedef IDirect3D8* (WINAPI *typeDirect3DCreate8)(UINT);
typedef HRESULT (WINAPI *typePresent8)(IDirect3DDevice8 *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
typedef HRESULT (WINAPI *typeReset8)(IDirect3DDevice8 *, D3DPRESENT_PARAMETERS *);
typedef ULONG (WINAPI *typeRelease8)(IDirect3DDevice8 *);

static CAPIHook<typePresent8> hookPresent8;
static CAPIHook<typeReset8> hookReset8;
static CAPIHook<typeRelease8> hookAddRef8;
static CAPIHook<typeRelease8> hookRelease8;

static std::map<IDirect3DDevice8*, sDevInfoD3D8*> mapDev8;

static CCriticalSection csExtFuncD3D8;

//-----------------------------------------------------------------------------

void CommonPrePresent8(IDirect3DDevice8 *pD3dd, sDevInfoD3D8 *info, int *width, int *height)
{
//	dprintf("commonPresent8()");

	HRESULT hr;
	bool stateChanged = false, beganScene = false;
	DWORD state = 0;
	DWORD oldPixelShader = 0;
	DWORD oldVertexShader = 0;

	if(g->hwnd == NULL)
	{
		D3DDEVICE_CREATION_PARAMETERS parm;

		hr = pD3dd->GetCreationParameters(&parm);
		if(SUCCEEDED(hr))
		{
			g->hwnd = parm.hFocusWindow;
		}
	}

	do
	{
		int i;
		D3DSURFACE_DESC desc;

		{
			CComPtr<IDirect3DSurface8> surfaceBack;

			hr = pD3dd->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &surfaceBack);
			if(FAILED(hr))
			{
				dprintf("GetBackBuffer() failed. (commonPresent8)");
				break;
			}

			hr = surfaceBack->GetDesc(&desc);
			if(FAILED(hr))
			{
				dprintf("GetDesc() failed. (commonPresent8)");
				break;
			}

			*width = desc.Width;
			*height=  desc.Height;
		}

		hr = pD3dd->CreateStateBlock(D3DSBT_ALL, &state);
		if(FAILED(hr))
		{
			dprintf("CreateStateBlock() failed. (commonPresent8)");
			state = 0;
			break;
		}

		try{
			hr = pD3dd->CaptureStateBlock(state);
			if(FAILED(hr))
			{
				dprintf("CaptureStateBlock() failed. (commonPresent8)");
				break;
			}
		}
		catch(...)
		{	// 原因不明
//			dprintf("CaptureStateBlock() access violation. (commonPresent8)");
		}

		stateChanged = true;

		//-------------------------------------------------------------------------

		TLVERTX vertex[4];

		hr = pD3dd->BeginScene();
		if(FAILED(hr))
		{
			dprintf("BeginScene() failed. (commonPresent8)");
			break;
		}

		beganScene = true;

		hr = pD3dd->GetPixelShader(&oldPixelShader);
		if(FAILED(hr))
		{
			oldPixelShader = 0;
		}

		hr = pD3dd->SetPixelShader(NULL);
		if(FAILED(hr))
		{
			dprintf("SetPixelShader() failed. (commonPresent8)");
			break;
		}

		hr = pD3dd->GetVertexShader(&oldVertexShader);
		if(FAILED(hr))
		{
			oldVertexShader = 0;
		}

		hr = pD3dd->SetVertexShader(FVF_TLVERTEX);
		if(FAILED(hr))
		{
			dprintf("SetVertexShader() failed. (commonPresent8)");
			break;
		}

		D3DVIEWPORT8 vp = {0, 0, desc.Width, desc.Height, 0.0f, 1.0f};

		pD3dd->SetViewport(&vp);

		pD3dd->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);	// 不要？

		pD3dd->SetRenderState(D3DRS_LIGHTING, FALSE);
		pD3dd->SetRenderState(D3DRS_FOGENABLE, FALSE);			// 真っ黒対策
		pD3dd->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		pD3dd->SetRenderState(D3DRS_STENCILENABLE, FALSE);		// EsR Demo
		pD3dd->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_DITHERENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_CLIPPING, FALSE);
		pD3dd->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		pD3dd->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
		pD3dd->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		pD3dd->SetRenderState(D3DRS_EDGEANTIALIAS, FALSE);
		pD3dd->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
        pD3dd->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
        pD3dd->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);

		pD3dd->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);	// 透過
		pD3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pD3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		pD3dd->SetRenderState(D3DRS_COLORVERTEX, TRUE);
		pD3dd->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

		pD3dd->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
		pD3dd->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);	// ぼやけ対策
		pD3dd->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
		pD3dd->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		pD3dd->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

		pD3dd->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pD3dd->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pD3dd->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		pD3dd->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		pD3dd->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT);
		pD3dd->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		pD3dd->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        pD3dd->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        pD3dd->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        pD3dd->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        pD3dd->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		//-----------------------------------------------------------------------------

		RECT rect;

		if(info->num && g->settings.showFPS)
		{
			int len, n;
			double fps;
			char sztemp[16];

			fps = g->getFPS();

			len = sprintf(sztemp, "%3d", (int)fps);

			hr = pD3dd->SetTexture(0, info->num);

			int x, y, space;

			g->getFPSPos(x, y, *width, *height);

			space = NUMTEX_SPACER * (MAX_DISPLAYDIGIT-1);
			for(i=MAX_DISPLAYDIGIT-1; i>=0; i--)
			{
				SetRectSize(&rect, i*NUMTEX_DRAWWIDTH + x + space, y, NUMTEX_REALWIDTH, NUMTEX_REALHEIGHT);

				n = (int)sztemp[i] - (int)'0';
				if(n < 0 || n > 9)
					n = 10;

				SetSquareVertex(vertex, &rect, &g->rectNum[n], 0xffffffff);

				hr = pD3dd->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(TLVERTX));

				space -= NUMTEX_SPACER;
			}
		}

		for(i=0; i<MAX_TEXNUM; i++)
		{
			if(info->tex[i] != NULL && info->settings.texInfo[i].active)
			{
				SetRectSize(&rect, (LONG)info->settings.texInfo[i].x, (LONG)info->settings.texInfo[i].y, (LONG)info->settings.texInfo[i].texSize, (LONG)info->settings.texInfo[i].texSize);
				SetSquareVertex(vertex, &rect, NULL, info->settings.texInfo[i].color);

				hr = pD3dd->SetTexture(0, info->tex[i]);
				if(FAILED(hr))
				{
					dprintf("SetTexture() failed. [%d] (commonPresent8)", i);
					break;
				}

				hr = pD3dd->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(TLVERTX));
				if(FAILED(hr))
				{
					dprintf("DrawPrimitiveUp() failed. [%d] (commonPresent8)", i);
					break;
				}
			}
		}

		if(FAILED(hr))
			break;

		//-----------------------------------------------------------------------------

	}while(false);

	if(oldPixelShader)
		pD3dd->SetPixelShader(oldPixelShader);
	if(oldVertexShader)
		pD3dd->SetVertexShader(oldVertexShader);

	if(beganScene)
		pD3dd->EndScene();

	if(stateChanged)
		pD3dd->ApplyStateBlock(state);

	if(state)
		pD3dd->DeleteStateBlock(state);

	//-----------------------------------------------------------------------------

	if(CMicroTimer::orgTime != g->settings.orgTime)
		CMicroTimer::orgTime = g->settings.orgTime;

	if(g->mtTimer.active())
		g->mtTimer.wait();

	g->processFPS();
}

//-----------------------------------------------------------------------------

void CommonPostPresent8(sDevInfoD3D8 *info, int width, int height)
{
	CMutexLock m(&g->mutexSettings);
	sSettings settings;
	
	memcpy(&settings, g->smSettings.getData(), g->smSettings.getLength());

	bool update = true;

	if(settings.updateOnActive)
	{
		DWORD pid = 0;
	
		GetWindowThreadProcessId(GetForegroundWindow(), &pid);
		update = GetCurrentProcessId() == pid;
	}

	if(update)
	{
		settings.lastUpdate = (DWORD)time(NULL);
		memcpy(g->smSettings.getData(), &settings, g->smSettings.getLength());
	}

	FixSettings(settings, width, height);

	g->checkSettings(settings);
	info->checkSettings(settings);
}

//-----------------------------------------------------------------------------

HRESULT WINAPI extPresent8(IDirect3DDevice8 *pD3dd, CONST RECT * pSourceRect, CONST RECT * pDestRect, HWND hDestWindowOverride, CONST RGNDATA * pDirtyRegion)
{
	CMutexLock m(&csExtFuncD3D8);
	HRESULT hr;
	sDevInfoD3D8 *info;
	int width, height;

//	dprintf("extPresent8()");

	info = FindPtrMap(mapDev8, pD3dd);
	if(info == NULL)
	{
		pD3dd->AddRef();
		pD3dd->Release();

		info = FindPtrMap(mapDev8, pD3dd);
	}

	CommonPrePresent8(pD3dd, info, &width, &height);

	hr = hookPresent8.entFunc(pD3dd, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	
	CommonPostPresent8(info, width, height);

	return hr;
}

//-----------------------------------------------------------------------------

HRESULT WINAPI extReset8(IDirect3DDevice8 *pD3dd, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	CMutexLock m(&csExtFuncD3D8);
	sDevInfoD3D8 *info;

//	dprintf("extReset8()");

	info = FindPtrMap(mapDev8, pD3dd);
	if(info)
	{
		info->removeAllResource();
	}

	return hookReset8.entFunc(pD3dd, pPresentationParameters);
}

//-----------------------------------------------------------------------------

ULONG WINAPI extAddRef8(IDirect3DDevice8 *pD3dd)
{
//	dprintf("extAddRef8()");

	ULONG ret;
	sDevInfoD3D8 *info;

	ret = hookAddRef8.entFunc(pD3dd);

	info = FindPtrMap(mapDev8, pD3dd);
	if(!info)
	{
		info = new sDevInfoD3D8(pD3dd);
		mapDev8[pD3dd] = info;
	}

	info->refCount = ret;

	return ret;
}

//-----------------------------------------------------------------------------

ULONG WINAPI extRelease8(IDirect3DDevice8 *pD3dd)
{
//	dprintf("extRelease8()");

	ULONG ret;
	sDevInfoD3D8 *info;
	
	info = FindPtrMap(mapDev8, pD3dd);
	if(info)
	{
		if(info->myRefCount > 0 && info->refCount == info->myRefCount+1)
		{
			{
				CMutexLock m(&csExtFuncD3D8);

				info->removeAllResource();
			}

			ret = hookRelease8.entFunc(pD3dd);

			info->refCount = ret;
		}
		else if(info->refCount == 1)
		{
			{
				CMutexLock m(&csExtFuncD3D8);

				info->removeAllResource();
			}

			g->hwnd = NULL;

			ret = hookRelease8.entFunc(pD3dd);
		}
		else
		{
			ret = hookRelease8.entFunc(pD3dd);

			info->refCount = ret;
		}
	}
	else
	{
		ret = hookRelease8.entFunc(pD3dd);
	}

	return ret;
}

//-----------------------------------------------------------------------------

void CheckHookD3D8()
{
	hookPresent8.check();
	hookReset8.check();
	hookRelease8.check();
	hookAddRef8.check();
}

//-----------------------------------------------------------------------------

bool SetHookD3D8()
{
	CMutexLock m(&csExtFuncD3D8);
	void *baseAddress;
	bool succeed = false;

	do
	{
		baseAddress = GetModuleHandle("d3d8.dll");
		if(baseAddress == NULL)
		{
			dprintf("GetModuleHandle() failed. (d3d8.dll)");
			break;
		}

		if(!hookPresent8.hook((DWORD)baseAddress + (DWORD)offsetPresent8, extPresent8))
		{
			dprintf("Hook failed. (Present8)");
			break;
		}

		if(!hookReset8.hook((DWORD)baseAddress + (DWORD)offsetReset8, extReset8))
		{
			dprintf("Hook failed. (Reset8)");
			break;
		}

		if(!hookAddRef8.hook((DWORD)baseAddress + (DWORD)offsetAddRef8, extAddRef8))
		{
			dprintf("Hook failed. (AddRef8)");
			break;
		}

		if(!hookRelease8.hook((DWORD)baseAddress + (DWORD)offsetRelease8, extRelease8))
		{
			dprintf("Hook failed. (Release8)");
			break;
		}

		succeed = true;

	}while(false);

	return succeed;
}

//-----------------------------------------------------------------------------

void ResetHookD3D8()
{
	CMutexLock m(&csExtFuncD3D8);

//	dprintf("ResetHookD3D8()");

	hookPresent8.clear();
	hookReset8.clear();
	hookRelease8.clear();
	hookAddRef8.clear();

	std::map<IDirect3DDevice8*, sDevInfoD3D8*>::iterator it;

	for(it=mapDev8.begin(); it!=mapDev8.end(); it++)
	{
		sDevInfoD3D8 *info;

		info = it->second;
		delete info;
	}

	mapDev8.clear();
}

//-----------------------------------------------------------------------------

bool GetOffsetD3D8(int *maxTextureSize)
{
	HINSTANCE baseAddress = NULL;
	bool succeed = false;

	baseAddress = LoadLibrary("d3d8.dll");
	if(baseAddress == NULL)
	{
		dprintf("LoadLibrary() failed. (d3d8.dll)");
		return false;
	}

	do
	{
		HRESULT hr;
		HWND hwnd = NULL;
		D3DDISPLAYMODE mode;
		CComPtr<IDirect3D8> d3d;
		CComPtr<IDirect3DDevice8> dev;
		D3DPRESENT_PARAMETERS parm;
		typeDirect3DCreate8 funcDirect3DCreate8 = NULL;

		hwnd = GetDesktopWindow();

		memset(&parm, 0, sizeof(parm));
		parm.BackBufferCount = 0;
		parm.BackBufferFormat = D3DFMT_X8R8G8B8;
		parm.BackBufferWidth = 32;
		parm.BackBufferHeight = 32;
		parm.EnableAutoDepthStencil = FALSE;
		parm.hDeviceWindow = hwnd;
		parm.MultiSampleType = D3DMULTISAMPLE_NONE;
		parm.SwapEffect = D3DSWAPEFFECT_DISCARD;
		parm.Windowed = TRUE;
		parm.Flags = 0;

		funcDirect3DCreate8 = (typeDirect3DCreate8)GetProcAddress(baseAddress, "Direct3DCreate8");
		if(funcDirect3DCreate8 == NULL)
		{
			dprintf("GetProcAddress() failed. (Direct3DCreate8)");
			break;
		}

		d3d = funcDirect3DCreate8(D3D_SDK_VERSION);
		if(d3d == NULL)
		{
			dprintf("Direct3DCreate8() failed.");
			break;
		}

		hr = d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
		if(FAILED(hr))
		{
			dprintf("GetAdapterDisplayMode() failed. (Direct3D8)");
			break;
		}

		parm.BackBufferFormat = mode.Format;

		hr = d3d->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			hwnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&parm,
			&dev
			);
		if(FAILED(hr))
		{
			dprintf("CreateDevice() failed. (Direct3D8)");
			break;
		}

		D3DCAPS8 caps;

		hr = dev->GetDeviceCaps(&caps);
		if(SUCCEEDED(hr))
		{
			*maxTextureSize = min(caps.MaxTextureWidth, caps.MaxTextureHeight);
		}

		DWORD *p;
		
		p = (DWORD *)dev.p;
		offsetPresent8 = (void *)(*((DWORD*)*p+15) - (DWORD)baseAddress);
		offsetReset8 = (void *)(*((DWORD*)*p+14) - (DWORD)baseAddress);
		offsetAddRef8 = (void *)(*((DWORD*)*p+1) - (DWORD)baseAddress);
		offsetRelease8 = (void *)(*((DWORD*)*p+2) - (DWORD)baseAddress);

		succeed = true;

	}while(false);

	FreeLibrary(baseAddress);

	return succeed;
}

//-----------------------------------------------------------------------------
