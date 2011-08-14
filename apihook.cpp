/*
 * Author: mosa Åüe5bW6vDOJ. <pcyp4g@gmail.com>
 *
 * This code is hereby placed in the public domain.
 */

#include "apihook.h"
#include "tools.h"

//-----------------------------------------------------------------------------

const unsigned char startCode[5] = {0x8b, 0xff, 0x55, 0x8b, 0xec};
const unsigned char startCodeSingle5[3] = {0x68, 0xb8, 0xe9};
const unsigned char startCodeJT[6] = {0x64, 0xa1, 0x18, 0x00, 0x00, 0x00};
const unsigned char startCode2000[6] = {0x55, 0x8b, 0xec, 0x83, 0xec, 0x10};

//-----------------------------------------------------------------------------

bool make_entrycode(void *addr, void *fn, int len)	// coded by nya
{
	DWORD old1, old2;
	unsigned char *entrycode = (unsigned char *)addr;
	unsigned char *code = (unsigned char *)fn;

	if(!VirtualProtect(addr, ENTRYCODE_LEN, PAGE_READWRITE, &old1))
		return false;

	memset(entrycode, 0x90, ENTRYCODE_LEN);
	memcpy(entrycode, code, len);
	entrycode[ENTRYCODE_LEN-5] = 0xe9;	// longjump
	*(DWORD*)(entrycode+ENTRYCODE_LEN-4) = (DWORD)fn - ((DWORD)addr + ENTRYCODE_LEN) + len;

	VirtualProtect(addr, ENTRYCODE_LEN, old1, &old2);

	return true;
}

//-----------------------------------------------------------------------------

bool make_jmpredir(void *addr, void *fn)
{
	const int len = 5;
	DWORD old1, old2;
	unsigned char *entrycode = (unsigned char *)addr;
	unsigned char *code = (unsigned char *)fn;

	if(!VirtualProtect(addr, ENTRYCODE_LEN, PAGE_READWRITE, &old1))
		return false;

	memset(entrycode, 0x90, ENTRYCODE_LEN);
	entrycode[0] = 0xe9;	// longjump
	*(DWORD*)(entrycode+1) = *(DWORD*)(code+1) - ((DWORD)addr - (DWORD)fn);

	VirtualProtect(addr, ENTRYCODE_LEN, old1, &old2);

	return true;
}

//-----------------------------------------------------------------------------

bool set_jump(void *addr, void *fn, unsigned char *code)
{
	DWORD old1, old2, temp;
	unsigned char *target = (unsigned char *)addr;

	if(!VirtualProtect(addr, JUMPCODE_LEN, PAGE_READWRITE, &old1))
		return false;

	memcpy(code, addr, JUMPCODE_LEN);

	temp = (DWORD)fn - (DWORD)addr - JUMPCODE_LEN;
	target[0] = 0xe9;	// longjmp
	memcpy(target+1, &temp, 4);	

	VirtualProtect(addr, JUMPCODE_LEN, old1, &old2);

	return true;
}

//-----------------------------------------------------------------------------

bool reset_jump(void *addr, unsigned char *code)
{
	DWORD old1, old2;

	if(!VirtualProtect(addr, JUMPCODE_LEN, PAGE_READWRITE, &old1))
		return false;

	memcpy(addr, code, JUMPCODE_LEN);

	VirtualProtect(addr, JUMPCODE_LEN, old1, &old2);

	return true;
}

//-----------------------------------------------------------------------------
