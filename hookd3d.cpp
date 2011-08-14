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

#include "hook.h"
#include "hookd3d.h"

//-----------------------------------------------------------------------------

void SetSquareVertex(LPTLVERTEX vertex, RECT *rect, FRECT *uvrect, DWORD color)
{
	int i;
	FRECT uvunit = {0.0f, 0.0f, 1.0f, 1.0f};

	if(uvrect == NULL)
		uvrect = &uvunit;

	vertex[0].tu = uvrect->left;
	vertex[0].tv = uvrect->top;
	vertex[1].tu = uvrect->right;
	vertex[1].tv = uvrect->top;
	vertex[2].tu = uvrect->left;
	vertex[2].tv = uvrect->bottom;
	vertex[3].tu = uvrect->right;
	vertex[3].tv = uvrect->bottom;

	vertex[0].x = rect->left;
	vertex[0].y = rect->top;
	vertex[1].x = rect->right;
	vertex[1].y = rect->top;
	vertex[2].x = rect->left;
	vertex[2].y = rect->bottom;
	vertex[3].x = rect->right;
	vertex[3].y = rect->bottom;

	for(i=0; i<4; i++)
	{
		vertex[i].z = 0.0f;
		vertex[i].rhw = 1.0f;
		vertex[i].color = color;
	}
}

//-----------------------------------------------------------------------------
