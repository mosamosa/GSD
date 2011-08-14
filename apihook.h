/*
 * Author: mosa Åüe5bW6vDOJ. <pcyp4g@gmail.com>
 *
 * This code is hereby placed in the public domain.
 */

#ifndef APIHOOKH
#define APIHOOKH

#include <windows.h>

//-----------------------------------------------------------------------------

#define JUMPCODE_LEN 5
#define ENTRYCODE_LEN (6+5)

extern const unsigned char startCode[5];
extern const unsigned char startCodeSingle5[3];
extern const unsigned char startCodeJT[6];
extern const unsigned char startCode2000[6];

//-----------------------------------------------------------------------------

bool make_entrycode(void *addr, void *fn, int len);
bool make_jmpredir(void *addr, void *fn);
bool set_jump(void *addr, void *fn, unsigned char *code);
bool reset_jump(void *addr, unsigned char *code);

//-----------------------------------------------------------------------------

template<typename T>
class CAPIHook
{
private:
	T orgFunc, extFunc;
	unsigned char code[JUMPCODE_LEN];
	unsigned char jumpcode[JUMPCODE_LEN];
	unsigned char entrycode[ENTRYCODE_LEN];

public:
	T entFunc;

	CAPIHook()
	{
		orgFunc = NULL;
		entFunc = NULL;
	}

	~CAPIHook()
	{
		clear();
	}

	bool hook(DWORD addr, T _extFunc)
	{
		int i;
		DWORD old;
		bool single5 = false;
		
		{
			unsigned char *p = (unsigned char *)addr;
			dprintf("Code start. (CAPIHook) %08x | %02x %02x %02x %02x %02x %02x %02x %02x",
				p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		}

		clear();

		extFunc = _extFunc;

		VirtualProtect(entrycode, ENTRYCODE_LEN, PAGE_EXECUTE_READWRITE, &old);

		orgFunc = (T)addr;

		for(i=0; i<sizeof(startCodeSingle5); i++)
		{
			if(memcmp(orgFunc, startCodeSingle5+i, 1) == 0)
			{
				single5 = true;

				break;
			}
		}

		if(single5 ||
		   memcmp(orgFunc, startCode, sizeof(startCode)) == 0)
		{
			unsigned char *tmp = (unsigned char *)orgFunc;

			if(single5 && tmp[0] == 0xe9)
			{
				if(!make_jmpredir(entrycode, orgFunc))
				{
					dprintf("Can't make entry code. (jmpredir)");

					orgFunc = NULL;
					return false;
				}
			}
			else
			{
				if(!make_entrycode(entrycode, orgFunc, 5))
				{
					dprintf("Can't make entry code. 5");

					orgFunc = NULL;
					return false;
				}
			}
		}
		else if(memcmp(orgFunc, startCodeJT, sizeof(startCodeJT)) == 0 ||
		        memcmp(orgFunc, startCode2000, sizeof(startCode2000)) == 0)
		{
			if(!make_entrycode(entrycode, orgFunc, 6))
			{
				dprintf("Can't make entry code. 6");

				orgFunc = NULL;
				return false;
			}
		}
		else
		{
			unsigned char *p = (unsigned char *)addr;
			dprintf("Invalid code start. (CAPIHook) %08x | %02x %02x %02x %02x %02x %02x %02x %02x",
				p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

			orgFunc = NULL;
			return false;
		}

		entFunc = (T)(void*)entrycode;
		if(!set_jump(orgFunc, extFunc, code))
		{
			dprintf("set_jump() failed. (CAPIHook::hook())");

			orgFunc = NULL;
			return false;
		}
		memcpy(jumpcode, orgFunc, JUMPCODE_LEN);

		return true;
	}

	void check()
	{
		if(orgFunc == NULL)
			return;

		if(memcmp(orgFunc, jumpcode, JUMPCODE_LEN) != 0)
		{
			unsigned char temp[JUMPCODE_LEN], *p;

			p = (unsigned char *)orgFunc;
			dprintf("Original code changed. %08x | %02x %02x %02x %02x %02x",
				p, p[0], p[1], p[2], p[3], p[4]);

			if(!set_jump(orgFunc, extFunc, temp))
			{
				orgFunc = NULL;
				dprintf("set_jump() failed. (CAPIHook::check())");
				return;
			}
		}
	}

	void clear()
	{
		if(orgFunc)
			reset_jump(orgFunc, code);

		orgFunc = NULL;
		entFunc = NULL;
		extFunc = NULL;
	}
};

//-----------------------------------------------------------------------------

#endif
