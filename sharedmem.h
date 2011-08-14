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

#ifndef SHAREDMEMH
#define SHAREDMEMH

//-----------------------------------------------------------------------------

class CSharedMemory
{
private:
	DWORD error;
	HANDLE map;
	char *data;
	int length;

public:
	CSharedMemory();
	~CSharedMemory();

	bool create(const char *name, int len);
	void release();

	char *getData(){ return data; }
	bool isFirst(){ return (error != ERROR_ALREADY_EXISTS); }
	int getLength(){ return length; }
};

//-----------------------------------------------------------------------------

#endif
