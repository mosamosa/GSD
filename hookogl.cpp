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

#pragma warning (disable: 4786)

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "delayimp.lib")
#pragma comment(linker, "/delayload:opengl32.dll")
#pragma comment(linker, "/delay:unload")

#include <windows.h>
#include <time.h>
#include <gl/gl.h>
#include <string>
#include <math.h>
#include <process.h>
#include <delayimp.h>

#include "common.h"
#include "tools.h"
#include "mutex.h"
#include "apihook.h"

#include "hook.h"
#include "hookogl.h"

//-----------------------------------------------------------------------------

typedef struct _sDevInfoOGL
{
	GLuint tex[MAX_TEXNUM];
	GLuint num;
	sSettings settings;

	void checkSettings(sSettings &_settings)
	{
		if(g->smPixels.getData() != NULL)
		{
			int i, j;

			for(i=0; i<MAX_TEXNUM; i++)
			{
				if(_settings.texInfo[i].texSize == 0)
				{
					if(tex[i] != 0)
					{
						glDeleteTextures(1, &tex[i]);
						tex[i] = 0;
					}
				}
			}

			DWORD *work = NULL, *p, col;
			int len = 0, work_len = 0;

			for(i=0; i<MAX_TEXNUM; i++)
			{
				if(_settings.texInfo[i].revision != settings.texInfo[i].revision)
				{
					if(tex[i] == 0)
						glGenTextures(1, &tex[i]);

					glBindTexture(GL_TEXTURE_2D , tex[i]);

					len = _settings.texInfo[i].texSize * _settings.texInfo[i].texSize;	// 32bit
					if(!work || len > work_len)
					{
						if(work)
							delete [] work;

						work = new DWORD[len];
						work_len = len;
					}

					p = (DWORD *)(g->smPixels.getData() + _settings.texInfo[i].offset);
					for(j=0; j<len; j++)	// 32bit
					{
						col = p[j];
						if(col)
							col = (col&0xff00ff00) | ((col&0x00ff0000)>>16) | ((col&0x000000ff)<<16);
						work[j] = col;
					}						

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _settings.texInfo[i].texSize, _settings.texInfo[i].texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, work);

					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				}
			}

			if(num == 0)
			{
				glGenTextures(1, &num);
				glBindTexture(GL_TEXTURE_2D , num);

				len = NUMTEX_SIZE * NUMTEX_SIZE;	// 32bit
				if(!work || len > work_len)
				{
					if(work)
						delete [] work;

					work = new DWORD[len];
					work_len = len;
				}

				HRSRC src = NULL;
				HGLOBAL hg = NULL;
				char *p2 = NULL;

				src = FindResource(g_hInstance, "NUMBER_TEXTURE", "DDS_IMAGE");
				if(src)
					hg = LoadResource(g_hInstance, src);
				if(hg)
					p2 = (char *)LockResource(hg);

				if(p2)
				{
					p = (DWORD *)(p2 + 128);	// dds
					for(j=0; j<len; j++)		// 32bit
					{
						col = p[j];
						if(col)
							col = (col&0xff00ff00) | ((col&0x00ff0000)>>16) | ((col&0x000000ff)<<16);
						work[j] = col;
					}						

					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NUMTEX_SIZE, NUMTEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, work);

					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				}
				else
				{
					dprintf("Resource error. (checkSettings() Num)");
				}

				if(hg)
					FreeResource(hg);
			}

			if(work)
				delete [] work;

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

		for(i=0; i<MAX_TEXNUM; i++)
		{
			if(tex[i] != 0)
			{
				glDeleteTextures(1, &tex[i]);
				tex[i] = 0;
			}
		}

		if(num != 0)
			glDeleteTextures(1, &num);
		num = 0;
	}

	_sDevInfoOGL()
	{
		num = 0;
		memset(tex, 0, sizeof(tex));
		memset(&settings, 0, sizeof(sSettings));
	}

	~_sDevInfoOGL()
	{
//		removeAllResource();
	}
}sDevInfoOGL;

typedef union
{
	struct
	{
		BYTE b, g, r, a;
	};
	DWORD col;
}BGRA;

//-----------------------------------------------------------------------------

typedef void (WINAPI *type_wglSwapBuffers)(HDC hDC);

static CAPIHook<type_wglSwapBuffers> hook_wglSwapBuffers;

static CCriticalSection csExtFuncOGL;
static sDevInfoOGL *glinfo = NULL;

//-----------------------------------------------------------------------------

void WINAPI ext_wglSwapBuffers(HDC hdc)
{
	CMutexLock o(&csExtFuncOGL);

//	dprintf("wglSwapBuffers()");

	RECT rect;
	HWND hwnd;
	int width, height;

	hwnd = WindowFromDC(hdc);
	GetClientRect(hwnd, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;

	if(g->hwnd != hwnd)
	{
		g->hwnd = hwnd;
		glinfo->removeAllResource();
	}

	//-----------------------------------------------------------------------------
	
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glViewport(0, 0, width, height);

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, -height, 0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glRasterPos2i(0, 0);
	glPixelZoom(1.0, -1.0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_TEXTURE_2D);

	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDisable(GL_DITHER);
	glDisable(GL_CULL_FACE);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_NORMALIZE);
	glDisable(GL_AUTO_NORMAL);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_COLOR_LOGIC_OP);
	glDisable(GL_INDEX_LOGIC_OP);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_Q);	// QRïsóvÅH
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(0x809e); // GL_SAMPLE_ALPHA_TO_COVERAGE_ARB
	glDisable(0x8620); // GL_VERTEX_PROGRAM_ARB
	glDisable(0x8804); // GL_FRAGMENT_PROGRAM_ARB

	glPixelStorei(GL_UNPACK_SWAP_BYTES, false);	// glDrawPixels
	glPixelStorei(GL_UNPACK_LSB_FIRST, false);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	glPixelTransferi(GL_MAP_COLOR, false);
	glPixelTransferi(GL_MAP_STENCIL, false);
	glPixelTransferi(GL_INDEX_SHIFT, 0);
	glPixelTransferi(GL_INDEX_OFFSET, 0);
	glPixelTransferf(GL_RED_SCALE, 1.0);
	glPixelTransferf(GL_GREEN_SCALE, 1.0);
	glPixelTransferf(GL_BLUE_SCALE, 1.0);
	glPixelTransferf(GL_ALPHA_SCALE, 1.0);
	glPixelTransferf(GL_DEPTH_SCALE, 1.0);
	glPixelTransferf(GL_RED_BIAS, 0.0);
	glPixelTransferf(GL_GREEN_BIAS, 0.0);
	glPixelTransferf(GL_BLUE_BIAS, 0.0);
	glPixelTransferf(GL_ALPHA_BIAS, 0.0);
	glPixelTransferf(GL_DEPTH_BIAS, 0.0);

	//-----------------------------------------------------------------------------

	int i;

	if(glinfo->num != 0 && g->settings.showFPS)
	{
		int len, n;
		double fps;
		char sztemp[16];
		FRECT frect;

		fps = g->getFPS();

		len = sprintf(sztemp, "%3d", (int)fps);

		glBindTexture(GL_TEXTURE_2D , glinfo->num);

		int x, y, space;

		g->getFPSPos(x, y, width, height);

		space = NUMTEX_SPACER * (MAX_DISPLAYDIGIT-1);
		for(i=MAX_DISPLAYDIGIT-1; i>=0; i--)
		{
			SetRectSize(&rect, i*NUMTEX_DRAWWIDTH + x + space, y, NUMTEX_REALWIDTH, NUMTEX_REALHEIGHT);

			n = (int)sztemp[i] - (int)'0';
			if(n < 0 || n > 9)
				n = 10;

			frect = g->rectNum[n];

			glColor4f(1.0, 1.0, 1.0, 1.0);
			glBegin(GL_POLYGON);
				glTexCoord2f(frect.left , frect.top);    glVertex2f(rect.left , -rect.top);
				glTexCoord2f(frect.left , frect.bottom); glVertex2f(rect.left , -rect.bottom);
				glTexCoord2f(frect.right, frect.bottom); glVertex2f(rect.right, -rect.bottom);
				glTexCoord2f(frect.right, frect.top);    glVertex2f(rect.right, -rect.top);
			glEnd();
			
			space -= NUMTEX_SPACER;
		}
	}

	for(i=0; i<MAX_TEXNUM; i++)
	{
		if(glinfo->tex[i] == 0 || !glinfo->settings.texInfo[i].active)
			continue;

		glBindTexture(GL_TEXTURE_2D , glinfo->tex[i]);

		int x, y, size;

		x = glinfo->settings.texInfo[i].x;
		y = glinfo->settings.texInfo[i].y;
		size = glinfo->settings.texInfo[i].texSize;

		BGRA col;

		col.col = glinfo->settings.texInfo[i].color;

		glColor4f(col.r/255.0f, col.g/255.0f, col.b/255.0f, col.a/255.0f);
		glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(x, -y);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(x, -(y+size));
			glTexCoord2f(1.0f, 1.0f); glVertex2f(x+size, -(y+size));
			glTexCoord2f(1.0f, 0.0f); glVertex2f(x+size, -y);
		glEnd();
	}

	glFlush();

	//-----------------------------------------------------------------------------

	if(CMicroTimer::orgTime != g->settings.orgTime)
		CMicroTimer::orgTime = g->settings.orgTime;

	if(g->mtTimer.active())
		g->mtTimer.wait();

	g->processFPS();

	hook_wglSwapBuffers.entFunc(hdc);

	//-----------------------------------------------------------------------------

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
		glinfo->checkSettings(settings);
	}

	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();

	glPopAttrib();
	glPopMatrix();
}

//-----------------------------------------------------------------------------

void CheckHookOGL()
{
	hook_wglSwapBuffers.check();
}

//-----------------------------------------------------------------------------

bool SetHookOGL()
{
	bool succeed = false;

	glinfo = new sDevInfoOGL;

	do
	{
		if(!hook_wglSwapBuffers.hook((DWORD)GetProcAddress(GetModuleHandle("opengl32.dll"), "wglSwapBuffers"), ext_wglSwapBuffers))
		{
			dprintf("Hook failed. (wglSwapBuffers)");
			break;
		}

		succeed = true;

	}while(false);

	return succeed;
}

//-----------------------------------------------------------------------------

void ResetHookOGL()
{
	CMutexLock o(&csExtFuncOGL);

	hook_wglSwapBuffers.clear();

	if(glinfo)
		delete glinfo;
	glinfo = NULL;
}

//-----------------------------------------------------------------------------

bool GetOffsetOGL(int *maxTextureSize)
{
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, maxTextureSize);

	return true;
}

//-----------------------------------------------------------------------------








