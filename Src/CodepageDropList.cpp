// CodepageDropList.cpp
// Copyright (c) datadiode
// SPDX-License-Identifier: WTFPL

#include "stdafx.h"
#include "SuperComboBox.h"
#include "codepage_detect.h"
#include "CodepageDropList.h"

BOOL CodepageDropList::EnumCodePagesProc(LPTSTR lpCodePageString)
{
	if (UINT const codepage = _tcstol(lpCodePageString, &lpCodePageString, 10))
	{
		CPINFOEX info;
		if (::GetCPInfoEx(codepage, 0, &info))
		{
			HWND const hCb = ::GetCapture();
			int lower = 0;
			int upper = static_cast<int>(::SendMessage(hCb, CB_GETCOUNT, 0, 0));
			while (lower < upper)
			{
				int const match = (upper + lower) >> 1;
				UINT const cmp = static_cast<UINT>(::SendMessage(hCb, CB_GETITEMDATA, match, 0));
				if (cmp >= info.CodePage)
					upper = match;
				if (cmp <= info.CodePage)
					lower = match + 1;
			}
			// Cosmetic: Remove excess spaces between numeric identifier and what follows it
			if (_tcstol(info.CodePageName, &lpCodePageString, 10) && *lpCodePageString == _T(' '))
				StrTrim(lpCodePageString + 1, _T(" "));
			int index = static_cast<int>(::SendMessage(hCb, CB_INSERTSTRING, lower, reinterpret_cast<LPARAM>(info.CodePageName)));
			::SendMessage(hCb, CB_SETITEMDATA, index, info.CodePage);
			LONG_PTR data = ::GetWindowLongPtr(hCb, GWLP_USERDATA);
			if (info.CodePage == HIWORD(data))
				::SendMessage(hCb, CB_SETCURSEL, index, 0);
		}
	}
	return TRUE; // continue enumeration
}

LRESULT CodepageDropList::SbWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnSuper = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	switch (message)
	{
	case WM_COMMAND:
		HWND hCb = reinterpret_cast<HWND>(lParam);
		LONG_PTR data = ::GetWindowLongPtr(hCb, GWLP_USERDATA);
		HWND hClient = reinterpret_cast<HWND>(::GetWindowLongPtr(hCb, GWLP_ID));
		::SendMessage(hClient, message, MAKEWPARAM(LOWORD(data), HIWORD(wParam)), lParam);
		switch (HIWORD(wParam))
		{
		case CBN_DROPDOWN:
			reinterpret_cast<HSuperComboBox *>(hCb)->AdjustDroppedWidth();
			break;
		case CBN_CLOSEUP:
			::DestroyWindow(hCb);
			::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pfnSuper));
			::SetFocus(hClient);
			break;
		}
		return 0;
	}
	return ::CallWindowProc(pfnSuper, hwnd, message, wParam, lParam);
}

void CodepageDropList::OnItemActivate(HWND hSb, HWND hClient, int iPart)
{
	RECT rc;
	::SendMessage(hSb, SB_GETRECT, iPart, reinterpret_cast<LPARAM>(&rc));
	HWND hCb = ::CreateWindow(WC_COMBOBOX, NULL, WS_CHILD | WS_VISIBLE |
		WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWN | CBS_NOINTEGRALHEIGHT,
		rc.left - 2, 0, rc.right - rc.left + 2, 100,
		hSb, reinterpret_cast<HMENU>(hClient), NULL, NULL);
	CHAR text[1024];
	::SendMessage(hSb, SB_GETTEXTA, iPart, reinterpret_cast<LPARAM>(text));
	unsigned codepage = LookupEncoding(text)->GetCodePage();
	::SetWindowLongPtr(hCb, GWLP_USERDATA, MAKELONG(iPart, codepage));
	::SendMessage(hCb, WM_SETFONT, ::SendMessage(hSb, WM_GETFONT, 0, 0), 0);
	LONG_PTR pfnSuper = ::SetWindowLongPtr(hSb, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SbWndProc));
	::SetWindowLongPtr(hSb, GWLP_USERDATA, pfnSuper);
	::SetCapture(hCb);
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	::EnumSystemCodePages(EnumCodePagesProc, CP_INSTALLED);
	::SendMessage(hCb, CB_SHOWDROPDOWN, TRUE, 0);
	::SendMessage(hSb, SB_GETTEXTA, iPart, reinterpret_cast<LPARAM>(text));
	::SetWindowTextA(hCb, text);
	::SetFocus(hCb);
}
