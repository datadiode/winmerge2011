/** 
 * @file  locality.h
 *
 * @brief Implementation of helper functions involving locale
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace locality;

/**
 * @brief Insert commas (or periods) into string, as appropriate for locale & preferences
 *
 * NB: We are not converting digits from ASCII via LOCALE_SNATIVEDIGITS
 *   So we always use ASCII digits, instead of, eg, the Chinese digits
 */
void NumToLocaleStr::getLocaleStr(LPCTSTR str)
{
	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, str, NULL, out, _countof(out)))
	{
		LPTSTR q = out;
		while (TCHAR c = *str++)
			if (LPTSTR p = StrChr(q, c))
				q = p + 1;
		*q = _T('\0');
	}
	else
	{
		lstrcpyn(out, str, _countof(out));
	}
}

/**
 * @brief Convert FILETIME to string to show in the GUI.
 */
TimeString::TimeString(const SYSTEMTIME &sysTime)
{
	if (int len = ::GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, out, 60))
	{
		out[len - 1] = _T(' ');
		::GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, out + len, 60);
	}
	else
	{
		out[0] = _T('\0');
	}
}
