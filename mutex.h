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

#ifndef MUTEXH
#define MUTEXH

#include <windows.h>

//-----------------------------------------------------------------------------

class CMutexBase
{
public:
	virtual void lock() = 0;
	virtual void unlock() = 0;
};

//-----------------------------------------------------------------------------

class CMutexLock
{
private:
	CMutexBase *m_mutex;

public:
	CMutexLock(CMutexBase *mutex)
	{
		m_mutex = mutex;
		m_mutex->lock();
	}

	~CMutexLock()
	{
		m_mutex->unlock();
	}
};

//-----------------------------------------------------------------------------

class CMutexUnlock
{
private:
	CMutexBase *m_mutex;

public:
	CMutexUnlock(CMutexBase *mutex)
	{
		m_mutex = mutex;
	}

	~CMutexUnlock()
	{
		m_mutex->unlock();
	}
};

//-----------------------------------------------------------------------------

class CMutex : public CMutexBase
{
private:
	HANDLE mutex;
	int error;

public:
	CMutex()
	{
		CMutex(NULL);
	}

	CMutex(const char *name)
	{
		mutex = CreateMutex(NULL, false, name);
		error = GetLastError();
	}

	~CMutex()
	{
		if(mutex)
			CloseHandle(mutex);
	}

	void lock()
	{
		if(mutex)
			WaitForSingleObject(mutex, INFINITE);
	}

	void unlock()
	{
		if(mutex)
			ReleaseMutex(mutex);
	}

	bool isAlreadyExists()
	{
		if(error == ERROR_ALREADY_EXISTS)
			return true;
		else
			return false;
	}
};

//-----------------------------------------------------------------------------

class CCriticalSection : public CMutexBase
{
private:
	CRITICAL_SECTION cs;

public:
	CCriticalSection()
	{
		InitializeCriticalSection(&cs);
	}

	~CCriticalSection()
	{
		DeleteCriticalSection(&cs);
	}

	void lock()
	{
		EnterCriticalSection(&cs);
	}

	void unlock()
	{
		LeaveCriticalSection(&cs);
	}

	bool tryLock()
	{
		return (TryEnterCriticalSection(&cs) != 0);
	}
};

//-----------------------------------------------------------------------------

#endif
