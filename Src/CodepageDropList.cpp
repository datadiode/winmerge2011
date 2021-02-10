// CodepageDropList.cpp
// Copyright (c) datadiode
// SPDX-License-Identifier: WTFPL

#include "stdafx.h"
#include "resource.h"
#include "SuperComboBox.h"
#include "LanguageSelect.h"
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

LRESULT CodepageDropList::LbWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnSuper = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_NCCALCSIZE:
		::CallWindowProc(pfnSuper, hwnd, message, wParam, lParam);
		if (HMENU const hMenu = ::GetMenu(hwnd))
		{
			if (int n = ::GetMenuItemCount(hMenu))
			{
				RECT rcMenu;
				::GetMenuItemRect(hwnd, hMenu, 0, &rcMenu);
				reinterpret_cast<RECT *>(lParam)->top -= rcMenu.top;
				::GetMenuItemRect(hwnd, hMenu, n - 1, &rcMenu);
				reinterpret_cast<RECT *>(lParam)->top += rcMenu.bottom;
			}
		}
		return 0;
	case WM_NCLBUTTONDOWN:
		if (HMENU const hMenu = ::GetMenu(hwnd))
		{
			POINT pt;
			POINTSTOPOINT(pt, lParam);
			RECT rc;
			::GetWindowRect(hwnd, &rc);
			int n = GetMenuItemCount(hMenu);
			for (int i = 0; i < n; ++i)
			{
				RECT rcMenu;
				::GetMenuItemRect(hwnd, hMenu, i, &rcMenu);
				rcMenu.bottom += rc.top - rcMenu.top;
				rcMenu.top = rc.top;
				rcMenu.left = rc.left;
				rcMenu.right = rc.right;
				rc.top = rcMenu.bottom;
				if (::PtInRect(&rcMenu, pt))
				{
					UINT state = ::GetMenuState(hMenu, i, MF_BYPOSITION);
					state ^= MF_CHECKED;
					::CheckMenuItem(hMenu, i, MF_BYPOSITION | state);
					::MapWindowPoints(NULL, hwnd, (LPPOINT)&rcMenu, 2);
					::RedrawWindow(hwnd, &rcMenu, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
				}
			}
		}
		break;
	case WM_NCPAINT:
		::CallWindowProc(pfnSuper, hwnd, message, wParam, lParam);
		if (HDC const hDC = ::GetWindowDC(hwnd))
		{
			RECT rc;
			::GetWindowRect(hwnd, &rc);
			rc.right -= rc.left;
			rc.left = 0;
			rc.bottom -= rc.top;
			rc.top = 0;
			if (HMENU const hMenu = GetMenu(hwnd))
			{
				::InflateRect(&rc, -1, -1);
				HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(hwnd, WM_GETFONT, 0, 0));
				::SelectObject(hDC, hFont);
				::SetTextColor(hDC, GetSysColor(COLOR_MENUTEXT));
				::SetBkColor(hDC, GetSysColor(COLOR_MENU));
				int n = ::GetMenuItemCount(hMenu);
				for (int i = 0; i < n; ++i)
				{
					RECT rcMenu;
					::GetMenuItemRect(hwnd, hMenu, i, &rcMenu);
					UINT const id = GetMenuItemID(hMenu, i);
					UINT const state = GetMenuState(hMenu, i, MF_BYPOSITION);
					rcMenu.bottom += rc.top - rcMenu.top;
					rcMenu.top = rc.top;
					rcMenu.left = rc.left;
					rcMenu.right = rc.right;
					rc.top = rcMenu.bottom;
					::ExtTextOut(hDC, rcMenu.left, rcMenu.top, ETO_OPAQUE, &rcMenu, NULL, 0, NULL);
					rcMenu.left = rc.left + 1;
					rcMenu.right = rcMenu.left + rcMenu.bottom - rcMenu.top;
					::InflateRect(&rcMenu, -2, -2);
					::DrawFrameControl(hDC, &rcMenu, DFC_BUTTON, state & MF_CHECKED ? DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_CHECKED : DFCS_BUTTONCHECK | DFCS_FLAT);
					rcMenu.left = rcMenu.right + 3;
					rcMenu.right = rc.right;
					try
					{
						::DrawText(hDC, LanguageSelect.LoadString(id).c_str(), -1, &rcMenu, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
					}
					catch (OException *e)
					{
						::DrawText(hDC, e->msg, -1, &rcMenu, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
					}
				}
			}
			ReleaseDC(NULL, hDC);
		}
		return 0;
	case WM_WINDOWPOSCHANGED:
		RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
		break;
	case WM_DESTROY:
		if (HMENU const hMenu = ::GetMenu(hwnd))
			::DestroyMenu(hMenu);
		break;
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
	COMBOBOXINFO info;
	info.cbSize = sizeof info;
	if (GetComboBoxInfo(hCb, &info))
	{
		HMENU hMenu = ::CreateMenu();
		::SetWindowLongPtr(info.hwndList, GWLP_ID, reinterpret_cast<LONG_PTR>(hMenu));
		if (hMenu)
		{
			::AppendMenu(hMenu, MF_OWNERDRAW | MF_CHECKED, IDS_CODEPAGE_RELOAD_FILE, NULL);
			LONG_PTR pfnSuper = ::SetWindowLongPtr(info.hwndList, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(LbWndProc));
			::SetWindowLongPtr(info.hwndList, GWLP_USERDATA, pfnSuper);
			::SetWindowPos(info.hwndList, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}
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
