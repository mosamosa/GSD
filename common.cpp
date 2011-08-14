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

#include <math.h>

#include "common.h"
#include "tools.h"

//-----------------------------------------------------------------------------

#pragma data_seg(".sharedata")
void *offsetPresent9 = NULL;
void *offsetPresentSC9 = NULL;
void *offsetReset9 = NULL;
void *offsetAddRef9 = NULL;
void *offsetRelease9 = NULL;
void *offsetPresent8 = NULL;
void *offsetReset8 = NULL;
void *offsetRelease8 = NULL;
void *offsetAddRef8 = NULL;
#pragma data_seg()

//-----------------------------------------------------------------------------

sGlobal *g = NULL;

//-----------------------------------------------------------------------------

void FixSettings(sSettings &settings, int width, int height)
{
	int i;

	for(i=0; i<MAX_TEXNUM; i++)
	{
		switch(settings.texInfo[i].align & 0x03)
		{
		case DT_CENTER:
			settings.texInfo[i].x = (width - settings.texInfo[i].texSize) / 2 + settings.texInfo[i].x;
			break;
		case DT_RIGHT:
			settings.texInfo[i].x = width - settings.texInfo[i].x - settings.texInfo[i].texSize;
			break;
		}

		switch(settings.texInfo[i].align & 0x0c)
		{
		case DT_VCENTER:
			settings.texInfo[i].y = (height - settings.texInfo[i].texSize) / 2 + settings.texInfo[i].y;
			break;
		case DT_BOTTOM:
			settings.texInfo[i].y = height - settings.texInfo[i].y - settings.texInfo[i].texSize;
			break;
		}
	}
}

//-----------------------------------------------------------------------------

sGlobal::sGlobal() :
	mutexSettings(MUTEX_SETTINGS)
{
	int i, j, k;

	memset(&settings, 0, sizeof(sSettings));
	hwnd = NULL;

	smSettings.create(SHARED_SETTINGS, sizeof(sSettings));

	beforeFPS = 0.0;
	beforeTime = 0;

	pcListPos = 0;
	for(i=0; i<MAX_FPSNUM; i++)
		pcList[i] = 0;

	for(j=0, k=0; j<3; j++)
	{
		for(i=0; i<4; i++, k++)
		{
			SetRectSize(
				&rectNum[k],
				(float)NUMTEX_REALWIDTH / (float)NUMTEX_SIZE * (float)i,
				(float)NUMTEX_REALHEIGHT / (float)NUMTEX_SIZE * (float)j,
				(float)NUMTEX_REALWIDTH / (float)NUMTEX_SIZE,
				(float)NUMTEX_REALHEIGHT / (float)NUMTEX_SIZE
				);
		}
	}
}

//-----------------------------------------------------------------------------

void sGlobal::getFPSPos(int &x, int &y, int width, int height)
{
	switch(settings.fpsAlign & 0x03)
	{
	case DT_RIGHT:
		x = width - (NUMTEX_DRAWWIDTH*MAX_DISPLAYDIGIT+NUMTEX_SPACER*(MAX_DISPLAYDIGIT-1)) - NUMTEX_MARGIN;
		break;
	default:
		x = NUMTEX_MARGIN;
	}

	switch(settings.fpsAlign & 0x0c)
	{
	case DT_BOTTOM:
		y = height - NUMTEX_DRAWHEIGHT - NUMTEX_MARGIN;
		break;
	default:
		y = NUMTEX_MARGIN;
	}
}

//-----------------------------------------------------------------------------

void sGlobal::checkSettings(sSettings &_settings)
{
	if(_settings.pixelsDataSize != settings.pixelsDataSize ||
	   _settings.pixelsRevision != settings.pixelsRevision)
	{
		char sztemp[256];

		sprintf(sztemp, "%s%d", SHARED_PIXELS, _settings.pixelsRevision);
		smPixels.create(sztemp, _settings.pixelsDataSize);
	}

	if(_settings.fpsLimit != settings.fpsLimit)
	{
		if(_settings.fpsLimit <= 0.0)
			mtTimer.stop();
		else
			mtTimer.start(1.0 / _settings.fpsLimit);
	}

	memcpy(&settings, &_settings, sizeof(sSettings));
}

//-----------------------------------------------------------------------------

void sGlobal::processFPS()
{
	__int64 t;

	QueryPerformanceCounter((LARGE_INTEGER*)&t);

	pcList[pcListPos] = t;

	pcListPos++;
	if(pcListPos >= MAX_FPSNUM)
		pcListPos = 0;
}

//-----------------------------------------------------------------------------

double sGlobal::getFPS()
{
	int i;
	double ave = 0.0;
	double maxFPS;

	maxFPS = pow(10, MAX_DISPLAYDIGIT) - 1;

	i = pcListPos - 1;
	if(i < 0)
		i = MAX_FPSNUM - 1;

	if(pc2time(pcList[i] - beforeTime) >= FPS_REFRESHTIME)
	{
		beforeTime = pcList[i];

		ave = pc2time(pcList[i] - pcList[pcListPos]) / (double)(MAX_FPSNUM-1);
		if(ave > 0.0)
			ave = 1.0 / ave;
		else
			ave = maxFPS;

		if(ave > maxFPS)
			ave = maxFPS;

		ave += 0.3;

		beforeFPS = ave;
	}
	else
	{
		ave = beforeFPS;
	}

	return ave;
}

//-----------------------------------------------------------------------------
