/*
 * Author: mosa Åüe5bW6vDOJ. <pcyp4g@gmail.com>
 *
 * This code is hereby placed in the public domain.
 */

#ifndef MMTIMERH
#define MMTIMERH

#include <windows.h>
#include "event.h"

class CMicroTimer
{
private:
	enum
	{
		TERMINATED,
		RUNNING,
	};
	
    int state;
    bool terminate;
    double interval;
    CEvent eventTimer, eventThread;
	
	static void timerThread(void *arg);
	
public:
	CMicroTimer();	
	~CMicroTimer();
	
	bool start(double sec);
	void stop();

	bool active()
	{
		return (state == RUNNING);
	}
	
	int wait()
	{
		eventTimer.wait((DWORD)(interval * 1000.0) + 100);
		return 0;
	}

	static __int64 orgTime;

	static double error;	// SleepÇÃåÎç∑
	static void calcError();
};

#endif
