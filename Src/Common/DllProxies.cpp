// DllProxies.cpp
// Copyright (c) 2007 Jochen Neubeck
// SPDX-License-Identifier: MIT
// Last change: 2019-01-12 by Jochen Neubeck
#include "stdafx.h"
#include "DllProxies.h"

/**
 * @brief Load a dll and import a number of functions.
 */
LPVOID DllProxy::Load()
{
	if (Names[0])
	{
		if (Names[1] == 0 || Names[1] == Names[0])
			return 0;
		HMODULE handle = LoadLibraryA(Names[0]);
		if (handle == 0)
		{
			Names[1] = 0;
			return 0;
		}
		LPCSTR *p = Names;
		*Names = 0;
		while (LPCSTR name = *++p)
		{
			*p = (LPCSTR)GetProcAddress(handle, name);
			if (*p == 0)
			{
				Names[0] = Names[1] = name;
				Names[2] = (LPCSTR)handle;
				return 0;
			}
		}
		*p = (LPCSTR)handle;
	}
	return this + 1;
}

/**
 * @brief Load a dll and import a number of functions, or throw an exception.
 */
LPVOID DllProxy::EnsureLoad()
{
	if (!Load())
	{
		TCHAR buf[1024];
		FormatMessage(buf);
		OException::Throw(buf);
	}
	return this + 1;
}

/**
 * @brief Format an appropriate error message.
 */
void DllProxy::FormatMessage(LPTSTR buf)
{
	int cch = wsprintf(buf, _T("%hs"), Names[0]);
	DWORD error = ERROR_MOD_NOT_FOUND;
	if (Names[1])
	{
		buf[cch++] = '@';
		cch += ::GetModuleFileName((HMODULE)Names[2], buf + cch, MAX_PATH);
		error = ERROR_PROC_NOT_FOUND;
	}
	buf[cch++] = ':';
	buf[cch++] = '\n';
	::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, buf + cch, MAX_PATH, 0);
}

/**
 * @brief ICONV dll proxy
 */
DllProxy::Instance<struct ICONV> ICONV =
{
	"ICONV.DLL",
	"libiconv_open",
	"libiconv",
	"libiconv_close",
	"libiconvctl",
	"libiconvlist",
	"_libiconv_version",
	(HMODULE)0
};

/**
 * @brief MSHTML dll proxy
 */
DllProxy::Instance<struct MSHTML> MSHTML =
{
	"MSHTML.DLL",
	"CreateHTMLPropertyPage",
	"ShowHTMLDialogEx",
	(HMODULE)0
};

/**
 * @brief KERNEL32 dll proxy for XP+
 */
DllProxy::Instance<struct KERNEL32V51> KERNEL32V51 =
{
	"KERNEL32.DLL",
	"AttachConsole",
	(HMODULE)0
};
