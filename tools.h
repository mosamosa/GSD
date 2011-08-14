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

#ifndef TOOLSH
#define TOOLSH

#include <map>

//-----------------------------------------------------------------------------

int dprintf(const char *format, ...);
double pc2time(__int64 t);
double round(double n);

//-----------------------------------------------------------------------------

template<typename U, typename T>
T FindPtrMap(std::map<U, T> &_map, U key)
{
	std::map<U, T>::iterator p;

	p = _map.find(key);

	if(p != _map.end())
		return p->second;
	else
		return NULL;
}

//-----------------------------------------------------------------------------

template<typename T, typename U>
void SetRectSize(T *rect, U x, U y, U width, U height)
{
	rect->left = x;
	rect->right = x + width;
	rect->top = y;
	rect->bottom = y + height;
}

#endif
