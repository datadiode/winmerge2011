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
#include "StdAfx.h"

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

// Try get the wine version.
static const char *wine_get_version()
{
	if (HMODULE h = GetModuleHandle(_T("ntdll.dll")))
	{
		typedef const char *(*wine_get_version)();
		if (FARPROC f = GetProcAddress(h, "wine_get_version"))
			return reinterpret_cast<wine_get_version>(f)();
	}
	return NULL;
}

// Provide the wine version in a global variable.
const char *const wine_version = wine_get_version();
