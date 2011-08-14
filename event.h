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

#ifndef EVENTH
#define EVENTH

class CEvent
{
private:
	HANDLE hEvent;

public:
	CEvent(bool manual_reset, bool initial_state, const char *szName)
	{
		hEvent = CreateEvent(NULL, manual_reset, initial_state, szName);
	}

	CEvent(const char *szName = NULL)
	{
		CEvent(true, false, szName);
	}

	~CEvent()
	{
		if(hEvent)
			CloseHandle(hEvent);
	}

	void set()
	{
		if(hEvent)
			SetEvent(hEvent);
	}

	void reset()
	{
		if(hEvent)
			ResetEvent(hEvent);
	}

	void wait(DWORD millisec = INFINITE)
	{
		if(hEvent)
			WaitForSingleObject(hEvent, millisec);
	}

	bool isSet()
	{
		if(hEvent)
			return (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0);

		return false;
	}

	void pulse()
	{
		if(hEvent)
			PulseEvent(hEvent);
	}
};

#endif