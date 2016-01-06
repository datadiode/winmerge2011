// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
/**
 * @file  Src/StdAfx.h
 *
 * @brief Project-wide includes and declarations
 */

// On Win XP, with VS2008, do not use default WINVER 0x0600 because of 
// some windows structure used in API (on VISTA they are longer)
#define WINVER 0x0502

#pragma warning(disable: 4812)

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_NON_CONFORMING_SWPRINTFS

#define _countof(array) (sizeof array / sizeof array[0])
#define __CRT_STRINGIZE(value) #value
#define _CRT_STRINGIZE(value) __CRT_STRINGIZE(value)

// Define the warnings that may be disabled in some rare particular cases
#define warning_this_used_in_base_member_initializer_list 4355
#define warning_possible_loss_of_data_due_to_type_conversion 4244

// CRT headers
#include "system32/system32.h"

// SDK headers
#include <olectl.h> // for VT_FONT
#include <oleacc.h>
#include <shlobj.h>
#include <shtypes.h>
#include <shlwapi.h>
#include <wininet.h>

// MSHTML headers
#include <mshtml.h>
//#include <iextag.h>
#include <mshtmhst.h>
//#include <mshtmcid.h>

// STL headers
#include <stl.h>
#include stl(list)
#include stl(map)
#include stl(set)
#include stl(vector)
#include stl(string)
#include stl(algorithm)
#include stl(sort)

// H2O headers
#include <H2O.h>
#include <H2O2.h>

// H2O
using H2O::HWindow;
using H2O::HMenu;
using H2O::HGdiObj;
using H2O::HFont;
using H2O::HBitmap;
using H2O::HPen;
using H2O::HBrush;
using H2O::HImageList;
using H2O::HSurface;
using H2O::HListBox;
using H2O::HComboBox;
using H2O::HEdit;
using H2O::HStatic;
using H2O::HButton;
using H2O::HHeaderCtrl;
using H2O::HListView;
using H2O::HTreeView;
using H2O::HToolTips;
using H2O::HToolBar;
using H2O::HStatusBar;
using H2O::HTabCtrl;
using H2O::HString;
using H2O::UNotify;

// H2O2
using H2O::OException;
using H2O::OWindow;
using H2O::ODialog;
using H2O::OResizableDialog;
using H2O::OPropertySheet;
using H2O::OResizableDialog;
using H2O::OListBox;
using H2O::OComboBox;
using H2O::OEdit;
using H2O::OButton;
using H2O::OHeaderCtrl;
using H2O::OListView;
using H2O::OString;
using H2O::ZeroInit;

// A version of _beginthreadex() with a WINAPI like signature
typedef HANDLE(__cdecl*fnBeginThreadEx)(LPSECURITY_ATTRIBUTES,
	UINT, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
#define BeginThreadEx ((fnBeginThreadEx)_beginthreadex)

// Global variable to hold the wine version.
extern const char *const wine_version;

#ifdef _DEBUG
#	include "common/ntdll.h"
#	define TRACE DbgPrint
#else
#	define TRACE __noop
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

#define A2W(A) OString(HString::Oct(A)->Uni()).W
#define W2A(W) OString(HString::Uni(W)->Oct()).A
#define UTF82W(A) OString(HString::Oct(A)->Uni(CP_UTF8)).W
#define W2UTF8(W) OString(HString::Uni(W)->Oct(CP_UTF8)).A

#define A2T A2W
#define T2A W2A
#define UTF82T UTF82W
#define T2UTF8 W2UTF8

/**
 * @name User-defined Windows-messages
 */
/* @{ */
/// Directory compare thread asks UI (view) update
const UINT MSG_UI_UPDATE = WM_APP + 1;
/* @} */

const UINT MERGE_VIEW_COUNT = 2;

/// Seconds ignored in filetime differences if option enabled
const UINT SmallTimeDiff = 2;

#include "UnicodeString.h"

/** @brief Retrieve error description from Windows; uses FormatMessage */
String GetSysError(int nerr);

/** @brief Send message to log file */
void LogErrorString(LPCTSTR sz);

template<class any>
void LogErrorString(any fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String s;
	s.append_sprintf_va_list(fmt, args);
	LogErrorString(s.c_str());
	va_end(args);
}

#define strdupa(s) strcpy((char *)_alloca(strlen(s) + 1), s)
#define wcsdupa(s) wcscpy((wchar_t *)_alloca((wcslen(s) + 1) * sizeof(wchar_t)), s)

/** @brief include for the custom dialog boxes, with do not ask/display again */
#include "MessageBoxDialog.h"

#include "WaitStatusCursor.h"

#include "locality.h"

inline bool operator == (const POINT &p, const POINT &q)
{
	return (p.x == q.x) && (p.y == q.y);
}

inline bool operator != (const POINT &p, const POINT &q)
{
	return (p.x != q.x) || (p.y != q.y);
}

inline bool operator == (const RECT &p, const RECT &q)
{
	return EqualRect(&p, &q) != FALSE;
}

inline bool operator != (const RECT &p, const RECT &q)
{
	return EqualRect(&p, &q) == FALSE;
}
