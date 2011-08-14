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

#ifndef HOOKD3DH
#define HOOKD3DH

#include <string>
#include <atlbase.h>

#include "common.h"
#include "tools.h"
#include "apihook.h"
#include "event.h"
#include "mutex.h"

//-----------------------------------------------------------------------------

#define FVF_TLVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct _TLVERTEX
{
	float x, y, z;                //位置情報
	float rhw;                    //頂点変換値
	DWORD color;                  //頂点カラー
	float tu, tv;                 //テクスチャ座標
}TLVERTX, *LPTLVERTEX;

//-----------------------------------------------------------------------------

void SetSquareVertex(LPTLVERTEX vertex, RECT *rect, FRECT *uvrect, DWORD color);

template<typename T, typename U>
struct _sDevInfoD3D
{
private:
	int getMyRefCount()
	{
		int i, count = 0;

		for(i=0; i<MAX_TEXNUM; i++)
		{
			if(tex[i] != NULL)
				count++;
		}

		if(num != NULL)
			count++;

		return count;
	}

public:
	int refCount, myRefCount;
	CComPtr<U> tex[MAX_TEXNUM];
	CComPtr<U> num;
	T *dev;
	sSettings settings;

	void checkSettings(sSettings &_settings)
	{
		int createdTexture[MAX_TEXNUM];

		memset(createdTexture, 0, sizeof(createdTexture));

		if(!num)
		{
			HRESULT hr;
#if(DIRECT3D_VERSION >= 0x0900)
			hr = dev->CreateTexture(NUMTEX_SIZE, NUMTEX_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &num, NULL);
#else
			hr = dev->CreateTexture(NUMTEX_SIZE, NUMTEX_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &num);
#endif
			if(SUCCEEDED(hr))
			{
				D3DLOCKED_RECT lock;

				hr = num->LockRect(0, &lock, NULL, D3DLOCK_NOSYSLOCK);
				if(SUCCEEDED(hr))
				{
					HRSRC src = NULL;
					HGLOBAL hg = NULL;
					char *p = NULL;

					src = FindResource(g_hInstance, "NUMBER_TEXTURE", "DDS_IMAGE");
					if(src)
						hg = LoadResource(g_hInstance, src);
					if(hg)
						p = (char *)LockResource(hg);

					if(p)
					{
						p += 128;
						memcpy(lock.pBits, p, NUMTEX_SIZE*NUMTEX_SIZE*4);	// 32bit
					}
					else
					{
						dprintf("Resource error. (checkSettings() Num)");
					}

					if(hg)
						FreeResource(hg);

					num->UnlockRect(0);
				}
				else
				{
					dprintf("LockRect() failed. (checkSettings() Num)");
				}
			}
			else
			{
				dprintf("CreateTexture() failed. (checkSettings() Num)");
			}
		}

		if(g->smPixels.getData() != NULL)
		{
			int i;
			HRESULT hr;

			myRefCount = 0;

			for(i=0; i<MAX_TEXNUM; i++)
			{
				if(_settings.texInfo[i].texSize > 0)
				{
					if(tex[i] == NULL || _settings.texInfo[i].texSize != settings.texInfo[i].texSize)
					{
						tex[i].Release();
#if(DIRECT3D_VERSION >= 0x0900)
						hr = dev->CreateTexture(_settings.texInfo[i].texSize, _settings.texInfo[i].texSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex[i], NULL);		// 32bit
#else
						hr = dev->CreateTexture(_settings.texInfo[i].texSize, _settings.texInfo[i].texSize, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex[i]);
#endif
						if(FAILED(hr))
						{
							dprintf("CreateTexture() failed. [%d] (recreateTexture)", i);
							continue;
						}

						createdTexture[i] = true;
					}
				}
				else
				{
					tex[i].Release();
				}
			}

			myRefCount = getMyRefCount();


			for(i=0; i<MAX_TEXNUM; i++)
			{
				if(tex[i] != NULL && (_settings.texInfo[i].revision != settings.texInfo[i].revision || createdTexture[i]))
				{
					D3DLOCKED_RECT lock;

					hr = tex[i]->LockRect(0, &lock, NULL, D3DLOCK_NOSYSLOCK);
					if(FAILED(hr))
					{
						dprintf("LockRect() failed. (checkSettings)");
						continue;
					}

					memcpy(
						lock.pBits,
						g->smPixels.getData() + _settings.texInfo[i].offset,
						_settings.texInfo[i].texSize * _settings.texInfo[i].texSize * 4
						);	// 32bit

					tex[i]->UnlockRect(0);
				}
			}

			memcpy(&settings, &_settings, sizeof(sSettings));
		}
		else
		{
//			dprintf("Shared memory error. (Pixels) (checkSettings)");
		}
	}

	void removeAllResource()
	{dprintf("removeAllResource()");
		int i;

		myRefCount = 0;

		for(i=0; i<MAX_TEXNUM; i++)
		{
			CATCHANY_BEGIN;
				tex[i].Release();
			CATCHANY_END("removeResource() release image texture.");	// Steamコミュニティー有効時に発生
		}

		CATCHANY_BEGIN;
			num.Release();
		CATCHANY_END("removeResource() release number texture.");	// Steamコミュニティー有効時に発生
	}

	_sDevInfoD3D(T *_dev)
	{
		dev = _dev;
		refCount = -1;
		myRefCount = 0;

		memset(&settings, 0, sizeof(sSettings));
	}

	~_sDevInfoD3D()
	{
		removeAllResource();
	}
};

//-----------------------------------------------------------------------------

#endif
