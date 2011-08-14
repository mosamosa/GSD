/*
 * Author: mosa Åüe5bW6vDOJ. <pcyp4g@gmail.com>
 *
 * This code is hereby placed in the public domain.
 */

#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include <math.h>

#include "mmtimer.h"
#include "tools.h"

//-----------------------------------------------------------------------------

static struct sMMTimerGlobal
{
	TIMECAPS tc;

    sMMTimerGlobal()
	{
		timeGetDevCaps(&tc, sizeof(TIMECAPS));
		timeBeginPeriod(tc.wPeriodMin);
	}

	~sMMTimerGlobal()
	{
		timeEndPeriod(tc.wPeriodMin);
    }
}MMTimerGlobal;

//-----------------------------------------------------------------------------
	
void CMicroTimer::timerThread(void *arg)
{
	CMicroTimer *_this = (CMicroTimer *)arg;
	__int64 freq, org, _org, crr, obj;
	double remain;
	
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&_org);
	
    while(!_this->terminate)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&crr);

		if(orgTime)
			org = orgTime;
		else
			org = _org;
		
		obj = (__int64)(_this->interval * (double)freq);
		obj = obj - ((crr - org) % obj);
		remain = (double)obj / (double)freq;
		obj += crr;
		
		remain *= 1000.0;
		remain -= error;	// Sleep()ÇÃåÎç∑
		remain = floor(remain);
		if(remain >= 1.0)
			Sleep((DWORD)remain);

		for(;;)
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&crr);

			if(crr >= obj || _this->terminate)
				break;
		}
		
        _this->eventTimer.set();
	}
	
    _this->state = TERMINATED;
    _this->eventThread.set();
}

//-----------------------------------------------------------------------------

CMicroTimer::CMicroTimer() :
	eventTimer(false, false, NULL),
	eventThread(true, true, NULL)
{
	state = TERMINATED;
	terminate = false;
	interval = 1.0;
}

//-----------------------------------------------------------------------------

CMicroTimer::~CMicroTimer()
{
	stop();
}

//-----------------------------------------------------------------------------

bool CMicroTimer::start(double sec)
{
	stop();
	
	if(sec <= 0.002)
		return false;
	
	if(state != RUNNING)
	{
		interval = sec;
		terminate = false;
		state = RUNNING;
		eventThread.reset();
		
		if((long)_beginthread(timerThread, 0, this) == -1L)
		{
			state = TERMINATED;
			eventThread.set();
			
			return false;
		}
		
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

void CMicroTimer::stop()
{
	if(state == RUNNING)
	{
		terminate = true;
		eventThread.wait((DWORD)(interval * 1000.0) + 100);
		state = TERMINATED;
	}
}

//-----------------------------------------------------------------------------

double CMicroTimer::error = 0.0;
__int64 CMicroTimer::orgTime = 0;

void CMicroTimer::calcError()
{

	HANDLE hThread;
	__int64 start, end;
	double ave = 0.0;
	int i, pri;
	const int interval = 10, loop = 10;

	hThread = GetCurrentThread();
	pri = GetThreadPriority(hThread);
	SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);

	for(i=0; i<loop; i++)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
		Sleep(interval);
		QueryPerformanceCounter((LARGE_INTEGER*)&end);

		ave += pc2time(end-start)*1000.0 - (double)interval;
	}
	ave /= loop;

	SetThreadPriority(hThread, pri);

	error = ave + 0.1;
	if(error < 0.0)
		error = 0.0;
	if(error > 1.5)
		error = 1.5;
}

//-----------------------------------------------------------------------------
