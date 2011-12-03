/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
// stdafx.cpp : source file that includes just the standard includes
//	Merge.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information
//
// RCS ID line follows -- this is updated by CVS
// $Id$

#include "StdAfx.h"

#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Support EASTL as per <EASTL/allocator.h> line 194.
void* operator new[](size_t size, const char* pName, int flags,
    unsigned debugFlags, const char* file, int line)
{
#ifdef _DEBUG
    return ::operator new[](size, _NORMAL_BLOCK, file, line);
#else
    return ::operator new[](size);
#endif
}

// Support EASTL as per <EASTL/allocator.h> line 195.
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
    const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    // this allocator doesn't support alignment
#ifdef _DEBUG
    return ::operator new[](size, _NORMAL_BLOCK, file, line);
#else
    return ::operator new[](size);
#endif
}

// Support EASTL as per <EASTL/string.h> line 197.
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
	// The _vscprintf() avoids reallocations by pretending C99 conformance.
	return n ? _vsnprintf(pDestination, n, pFormat, arguments) : _vscprintf(pFormat, arguments);
}

// Support EASTL as per <EASTL/string.h> line 198.
int Vsnprintf16(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
{
	// The _vscwprintf() avoids reallocations by pretending C99 conformance.
	return n ? _vsnwprintf(pDestination, n, pFormat, arguments) : _vscwprintf(pFormat, arguments);
}
