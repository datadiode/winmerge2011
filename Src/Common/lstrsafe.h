// This file contains a few semi-safe substitutes for WinAPI lstr* functions.
// The main issue is that prior to Windows Vista, lstrlenW() can fail on valid
// input, by hiding a possible stack overflow in its own catch-all SEH handler.
// http://www.blackhat.com/presentations/bh-usa-07/Dowd_McDonald_and_Mehta/Whitepaper/bh-usa-07-dowd_mcdonald_and_mehta.pdf
// elaborates on the issue.
// Besides, SEH usage in these functions can degrade performance and hide bugs.

#pragma once

#ifndef StrCpyNA
#error Need to include shlwapi.h before lstrsafe.h
#endif

#undef StrCpyNA

EXTERN_C __inline int StrLenA(LPCSTR p)
{
	return static_cast<int>(p ? strlen(p) : 0);
}

EXTERN_C __inline int StrLenW(LPCWSTR p)
{
	return static_cast<int>(p ? wcslen(p) : 0);
}

EXTERN_C __inline LPSTR StrCpyNA(LPSTR p, LPCSTR q, int capacity)
{
	if (_memccpy(p, q, 0, capacity - 1) == NULL)
		p[capacity - 1] = '\0';
	return p;
}

#define lstrlenA StrLenA
#define lstrlenW StrLenW
#define lstrcpynA StrCpyNA
#define lstrcpynW StrCpyNW
