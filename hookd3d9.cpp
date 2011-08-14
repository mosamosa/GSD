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
#include <d3d9.h>
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
#include "hookd3d9.h"

//-----------------------------------------------------------------------------

typedef struct _sDevInfoD3D<IDirect3DDevice9, IDirect3DTexture9> sDevInfoD3D9;

typedef IDirect3D9* (WINAPI *typeDirect3DCreate9)(UINT);
typedef HRESULT (WINAPI *typePresent9)(IDirect3DDevice9 *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *);
typedef HRESULT (WINAPI *typePresentSC9)(IDirect3DSwapChain9 *, CONST RECT *, CONST RECT *, HWND, CONST RGNDATA *, DWORD);
typedef HRESULT (WINAPI *typeReset9)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *);
typedef ULONG (WINAPI *typeRelease9)(IDirect3DDevice9 *);

static CAPIHook<typePresent9> hookPresent9;
static CAPIHook<typePresentSC9> hookPresentSC9;
static CAPIHook<typeReset9> hookReset9;
static CAPIHook<typeRelease9> hookRelease9;
static CAPIHook<typeRelease9> hookAddRef9;

static std::map<IDirect3DDevice9*, sDevInfoD3D9*> mapDev9;
static bool inPresent9 = false;

static CCriticalSection csExtFuncD3D9;

//-----------------------------------------------------------------------------

void CommonPrePresent9(IDirect3DDevice9 *pD3dd, sDevInfoD3D9 *info, int *width, int *height)
{
//	dprintf("commonPresent9()");

	HRESULT hr;
	CComPtr<IDirect3DStateBlock9> state;
	bool stateChanged = false, beganScene = false;

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
			CComPtr<IDirect3DSurface9> surfaceBack;

			hr = pD3dd->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surfaceBack);
			if(FAILED(hr))
			{
				dprintf("GetBackBuffer() failed. (commonPresent9)");
				break;
			}

			hr = surfaceBack->GetDesc(&desc);
			if(FAILED(hr))
			{
				dprintf("GetDesc() failed. (commonPresent9)");
				break;
			}

			*width = desc.Width;
			*height=  desc.Height;
		}

		hr = pD3dd->CreateStateBlock(D3DSBT_ALL, &state);
		if(FAILED(hr))
		{
			dprintf("CreateStateBlock() failed. (commonPresent9)");
			break;
		}

		hr = state->Capture();
		if(FAILED(hr))
		{
			dprintf("Capture() failed. (commonPresent9)");
			break;
		}

		stateChanged = true;

		//-------------------------------------------------------------------------

		TLVERTX vertex[4];

		hr = pD3dd->BeginScene();
		if(FAILED(hr))
		{
			dprintf("BeginScene() failed. (commonPresent9)");
			break;
		}

		beganScene = true;

		pD3dd->SetPixelShader(NULL);
		pD3dd->SetVertexShader(NULL);

		hr = pD3dd->SetFVF(FVF_TLVERTEX);
		if(FAILED(hr))
		{
			dprintf("SetFVF() failed. (commonPresent9)");
			break;
		}

		D3DVIEWPORT9 vp = {0, 0, desc.Width, desc.Height, 0.0f, 1.0f};

		pD3dd->SetViewport(&vp);

		pD3dd->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);				// 不要？

		pD3dd->SetRenderState(D3DRS_LIGHTING, FALSE);
		pD3dd->SetRenderState(D3DRS_FOGENABLE, FALSE);						// 真っ黒対策 (TombRaider Legend)
		pD3dd->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		pD3dd->SetRenderState(D3DRS_STENCILENABLE, FALSE);					// EsR Demo
		pD3dd->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_DITHERENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_CLIPPING, FALSE);
		pD3dd->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		pD3dd->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
		pD3dd->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
		pD3dd->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);				// ワイヤフレーム対策
		pD3dd->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);			// テクスチャ端半透明対策
        pD3dd->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
        pD3dd->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);

		pD3dd->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);				// 透過
		pD3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pD3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		pD3dd->SetRenderState(D3DRS_COLORVERTEX, TRUE);
		pD3dd->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);

		pD3dd->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		pD3dd->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);		// ぼやけ対策
		pD3dd->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		pD3dd->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		pD3dd->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

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
					dprintf("SetTexture() failed. [%d] (commonPresent9)", i);
					break;
				}

				hr = pD3dd->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(TLVERTX));
				if(FAILED(hr))
				{
					dprintf("DrawPrimitiveUp() failed. [%d] (commonPresent9)", i);
					break;
				}
			}
		}

		if(FAILED(hr))
			break;

		//-----------------------------------------------------------------------------

	}while(false);

	if(beganScene)
	{
		pD3dd->EndScene();
	}

	if(stateChanged)
	{
		hr = state->Apply();
		if(FAILED(hr))
		{
			dprintf("Apply() failed.");
		}
	}

	//-----------------------------------------------------------------------------

	if(CMicroTimer::orgTime != g->settings.orgTime)
		CMicroTimer::orgTime = g->settings.orgTime;

	if(g->mtTimer.active())
		g->mtTimer.wait();

	g->processFPS();
}

//-----------------------------------------------------------------------------

void CommonPostPresent9(sDevInfoD3D9 *info, int width, int height)
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

HRESULT WINAPI extPresent9(IDirect3DDevice9 *pD3dd, CONST RECT * pSourceRect, CONST RECT * pDestRect, HWND hDestWindowOverride, CONST RGNDATA * pDirtyRegion)
{
	CMutexLock m(&csExtFuncD3D9);
	HRESULT hr;
	sDevInfoD3D9 *info;
	int width, height;

//	dprintf("extPresent9()");

	inPresent9 = true;

	info = FindPtrMap(mapDev9, pD3dd);
	if(info == NULL)
	{
		pD3dd->AddRef();
		pD3dd->Release();

		info = FindPtrMap(mapDev9, pD3dd);
	}

	CommonPrePresent9(pD3dd, info, &width, &height);

	hr = hookPresent9.entFunc(pD3dd, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	CommonPostPresent9(info, width, height);

	inPresent9 = false;

	return hr;
}

//-----------------------------------------------------------------------------

HRESULT WINAPI extPresentSC9(IDirect3DSwapChain9 *pD3dsc, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	CMutexLock m(&csExtFuncD3D9);

//	dprintf("extPresentSC9()");

	if(!inPresent9)
	{
		do
		{
			HRESULT hr;
			sDevInfoD3D9 *info;
			int width, height;

			{
				CComPtr<IDirect3DDevice9> pD3dd;

				hr = pD3dsc->GetDevice(&pD3dd);
				if(FAILED(hr))
				{
					dprintf("GetDevice() failed. (PresentSC9)");
					break;
				}

				info = FindPtrMap(mapDev9, pD3dd.p);
				if(info == NULL)
				{
					pD3dd.p->AddRef();
					pD3dd.p->Release();

					info = FindPtrMap(mapDev9, pD3dd.p);
				}

				CommonPrePresent9(pD3dd, info, &width, &height);
			}

			hr = hookPresentSC9.entFunc(pD3dsc, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

			CommonPostPresent9(info, width, height);

			return hr;
	
		}while(false);
	}

	return hookPresentSC9.entFunc(pD3dsc, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

//-----------------------------------------------------------------------------

HRESULT WINAPI extReset9(IDirect3DDevice9 *pD3dd, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	CMutexLock m(&csExtFuncD3D9);
	sDevInfoD3D9 *info;

//	dprintf("extReset9()");

	info = FindPtrMap(mapDev9, pD3dd);
	if(info)
	{
		info->removeAllResource();
	}

	return hookReset9.entFunc(pD3dd, pPresentationParameters);
}

//-----------------------------------------------------------------------------

ULONG WINAPI extAddRef9(IDirect3DDevice9 *pD3dd)
{
//	dprintf("extAddRef9()");

	ULONG ret;
	sDevInfoD3D9 *info;

	ret = hookAddRef9.entFunc(pD3dd);
	
	info = FindPtrMap(mapDev9, pD3dd);
	if(!info)
	{
		info = new sDevInfoD3D9(pD3dd);
		mapDev9[pD3dd] = info;
	}

	info->refCount = ret;

	return ret;
}

//-----------------------------------------------------------------------------

ULONG WINAPI extRelease9(IDirect3DDevice9 *pD3dd)
{
//	dprintf("extRelease9()");

	ULONG ret;
	sDevInfoD3D9 *info;
	
	info = FindPtrMap(mapDev9, pD3dd);
	if(info)
	{
		if(info->myRefCount > 0 && info->refCount == info->myRefCount+1)
		{
			{
				CMutexLock m(&csExtFuncD3D9);

				info->removeAllResource();
			}

			ret = hookRelease9.entFunc(pD3dd);

			info->refCount = ret;
		}
		else if(info->refCount == 1)
		{
			{
				CMutexLock m(&csExtFuncD3D9);

				info->removeAllResource();
			}

			g->hwnd = NULL;

			ret = hookRelease9.entFunc(pD3dd);
		}
		else
		{
			ret = hookRelease9.entFunc(pD3dd);

			info->refCount = ret;
		}
	}
	else
	{
		ret = hookRelease9.entFunc(pD3dd);
	}

	return ret;
}

//-----------------------------------------------------------------------------

void CheckHookD3D9()
{
	hookPresent9.check();
	hookPresentSC9.check();
	hookReset9.check();
	hookAddRef9.check();
	hookRelease9.check();
}

//-----------------------------------------------------------------------------

bool SetHookD3D9()
{
	void *baseAddress;
	bool succeed = false;

	do
	{
		baseAddress = GetModuleHandle("d3d9.dll");
		if(baseAddress == NULL)
		{
			dprintf("GetModuleHandle() failed. (d3d9.dll)");
			break;
		}

		if(!hookPresent9.hook((DWORD)baseAddress + (DWORD)offsetPresent9, extPresent9))
		{
			dprintf("Hook failed. (Present9)");
			break;
		}

		if(!hookPresentSC9.hook((DWORD)baseAddress + (DWORD)offsetPresentSC9, extPresentSC9))
		{
			dprintf("Hook failed. (PresentSC9)");
			break;
		}

		if(!hookReset9.hook((DWORD)baseAddress + (DWORD)offsetReset9, extReset9))
		{
			dprintf("Hook failed. (Reset9)");
			break;
		}

		if(!hookAddRef9.hook((DWORD)baseAddress + (DWORD)offsetAddRef9, extAddRef9))
		{
			dprintf("Hook failed. (AddRef9)");
			break;
		}

		if(!hookRelease9.hook((DWORD)baseAddress + (DWORD)offsetRelease9, extRelease9))
		{
			dprintf("Hook failed. (Release9)");
			break;
		}

		succeed = true;

	}while(false);

	return succeed;
}

//-----------------------------------------------------------------------------

void ResetHookD3D9()
{
	CMutexLock m(&csExtFuncD3D9);

//	dprintf("ResetHookD3D9()");

	hookPresent9.clear();
	hookPresentSC9.clear();
	hookReset9.clear();
	hookAddRef9.clear();
	hookRelease9.clear();

	std::map<IDirect3DDevice9*, sDevInfoD3D9*>::iterator it;

	for(it=mapDev9.begin(); it!=mapDev9.end(); it++)
	{
		sDevInfoD3D9 *info;

		info = it->second;
		delete info;
	}

	mapDev9.clear();
}

//-----------------------------------------------------------------------------

bool GetOffsetD3D9(int *maxTextureSize)
{
	HINSTANCE baseAddress = NULL;
	bool succeed = false;

	baseAddress = LoadLibrary("d3d9.dll");
	if(baseAddress == NULL)
	{
		dprintf("LoadLibrary() failed. (d3d9.dll)");
		return false;
	}

	do
	{
		HRESULT hr;
		HWND hwnd;
		D3DDISPLAYMODE mode;
		D3DPRESENT_PARAMETERS parm;
		CComPtr<IDirect3D9> d3d;
		CComPtr<IDirect3DDevice9> dev;
		CComPtr<IDirect3DSwapChain9> swap;
		typeDirect3DCreate9 funcDirect3DCreate9 = NULL;

		hwnd = GetDesktopWindow();

		memset(&parm, 0, sizeof(parm));
		parm.BackBufferCount = 0;
		parm.BackBufferFormat = D3DFMT_X8R8G8B8;
		parm.BackBufferWidth = 32;
		parm.BackBufferHeight = 32;
		parm.EnableAutoDepthStencil = FALSE;
		parm.hDeviceWindow = hwnd;
		parm.MultiSampleType = D3DMULTISAMPLE_NONE;
		parm.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		parm.SwapEffect = D3DSWAPEFFECT_DISCARD;
		parm.Windowed = TRUE;
		parm.Flags = 0;

		funcDirect3DCreate9 = (typeDirect3DCreate9)GetProcAddress(baseAddress, "Direct3DCreate9");
		if(funcDirect3DCreate9 == NULL)
		{
			dprintf("GetProcAddress() failed. (Direct3DCreate9)");
			break;
		}

		d3d = funcDirect3DCreate9(D3D_SDK_VERSION);
		if(d3d == NULL)
		{
			dprintf("Direct3DCreate9() failed.");
			break;
		}

		hr = d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
		if(FAILED(hr))
		{
			dprintf("GetAdapterDisplayMode() failed. (Direct3D9)");
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
			dprintf("CreateDevice() failed. (GetD3D9Offset)");
			break;
		}

		D3DCAPS9 caps;

		hr = dev->GetDeviceCaps(&caps);
		if(SUCCEEDED(hr))
		{
			*maxTextureSize = min(caps.MaxTextureWidth, caps.MaxTextureHeight);
		}

		hr = dev->GetSwapChain(0, &swap);
		if(FAILED(hr))
		{
			dprintf("GetSwapChain() failed. (GetD3D9Offset)");
			break;
		}

		DWORD *p;

		p = (DWORD*)dev.p;
		offsetReset9 = (void *)(*((DWORD*)*p+16) - (DWORD)baseAddress);
		offsetPresent9 = (void *)(*((DWORD*)*p+17) - (DWORD)baseAddress);
		offsetAddRef9 = (void *)(*((DWORD*)*p+1) - (DWORD)baseAddress);
		offsetRelease9 = (void *)(*((DWORD*)*p+2) - (DWORD)baseAddress);

		p = (DWORD*)swap.p;
		offsetPresentSC9 = (void *)(*((DWORD*)*p+3) - (DWORD)baseAddress);

		succeed = true;

	}while(false);

	FreeLibrary(baseAddress);

	return succeed;
}

//-----------------------------------------------------------------------------
