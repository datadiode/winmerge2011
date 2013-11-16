// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define _WIN32_WINNT 0x0500

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define __INLINE_ISEQUAL_GUID // Avoid a reference to memcmp

#define _countof(array) (sizeof array / sizeof array[0])
#define __CRT_STRINGIZE(value) #value
#define _CRT_STRINGIZE(value) __CRT_STRINGIZE(value)

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlguid.h>

#include <tchar.h>
#include <MyCom.h>

#ifdef _DEBUG
#	include <ntdll.h>
#	define TRACE DbgPrint
#	define assert(e) \
	if (!(e)) \
	{ \
		char buf[1024]; \
		wsprintfA(buf, "%s@%s[%d]", #e, __FILE__, __LINE__); \
		switch (MessageBoxA(NULL, buf, "Assertion failed", MB_ABORTRETRYIGNORE)) \
		{ case IDABORT: ExitProcess(0); case IDRETRY: __debugbreak(); } \
	} typedef int eat_semicolon_ahead
#else
#	define TRACE __noop
#	define assert __noop
#endif

#ifndef DEBUG_NEW
#	define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#endif

#ifdef _DEBUG
#	define ASSERT(condition) assert(condition)
#	define VERIFY(condition) ASSERT(condition)
#else
#	define ASSERT(condition) __noop()
#	define VERIFY(condition) (condition) ? __noop() : __noop()
#endif
