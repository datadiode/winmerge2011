// WildcardDropList.cpp
// Copyright (c) datadiode
// SPDX-License-Identifier: WTFPL

#include "stdafx.h"
#include "WildcardDropList.h"
#include "LanguageSelect.h"

/**
 * @brief DropList window procedure.
 */
LRESULT WildcardDropList::LbWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC const pfnSuper = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_PAINT:
		PAINTSTRUCT ps;
		if (HDC const hDC = ::BeginPaint(hwnd, &ps))
		{
			RECT rc;
			GetClientRect(hwnd, &rc);
			::SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
			::SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			int const cxCross = ::GetSystemMetrics(SM_CXVSCROLL) - 1;
			RECT rcMenu = { rc.right - cxCross, rc.top - 1, rc.right + 1, rc.top + cxCross };
			::DrawFrameControl(hDC, &rcMenu, DFC_CAPTION, DFCS_CAPTIONCLOSE | DFCS_FLAT);
			rcMenu.top = rcMenu.bottom;
			rcMenu.bottom = rc.bottom;
			::ExtTextOut(hDC, rcMenu.left, rcMenu.top, ETO_OPAQUE, &rcMenu, NULL, 0, NULL);
			if (HMENU const hMenu = ::GetMenu(hwnd))
			{
				rc.right -= cxCross;
				HFONT const hFont = reinterpret_cast<HFONT>(::SendMessage(hwnd, WM_GETFONT, 0, 0));
				::SelectObject(hDC, hFont);
				int const n = ::GetMenuItemCount(hMenu);
				for (int i = 0; i < n; ++i)
				{
					RECT rcMenu;
					::GetMenuItemRect(hwnd, hMenu, i, &rcMenu);
					UINT const id = ::GetMenuItemID(hMenu, i);
					UINT const state = ::GetMenuState(hMenu, i, MF_BYPOSITION);
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
			::EndPaint(hwnd, &ps);
		}
		return 0;
	case WM_LBUTTONDOWN:
		if (HWND const hTc = GetDlgItem(hwnd, 100))
		{
			TCHITTESTINFO info;
			POINTSTOPOINT(info.pt, lParam);
			RECT rc;
			::GetClientRect(hwnd, &rc);
			int const cxCross = ::GetSystemMetrics(SM_CXVSCROLL) - 1;
			RECT rcMenu = { rc.right - cxCross, rc.top - 1, rc.right + 1, rc.top + cxCross };
			if (::PtInRect(&rcMenu, info.pt))
			{
				if (HWND const hCb = reinterpret_cast<HWND>(::GetWindowLongPtr(hTc, GWLP_USERDATA)))
				{
					::EnableWindow(hTc, FALSE);
					::PostMessage(hCb, CB_SHOWDROPDOWN, 0, 0);
					break;
				}
			}
			if (HMENU const hMenu = ::GetMenu(hwnd))
			{
				rc.right -= cxCross;
				int const n = ::GetMenuItemCount(hMenu);
				for (int i = 0; i < n; ++i)
				{
					RECT rcMenu;
					::GetMenuItemRect(hwnd, hMenu, i, &rcMenu);
					rcMenu.bottom += rc.top - rcMenu.top;
					rcMenu.top = rc.top;
					rcMenu.left = rc.left;
					rcMenu.right = rc.right;
					rc.top = rcMenu.bottom;
					if (::PtInRect(&rcMenu, info.pt))
					{
						UINT state = ::GetMenuState(hMenu, i, MF_BYPOSITION);
						state ^= MF_CHECKED;
						::CheckMenuItem(hMenu, i, MF_BYPOSITION | state);
						rcMenu.right = rcMenu.left + rcMenu.bottom - rcMenu.top;
						::RedrawWindow(hwnd, &rcMenu, NULL, RDW_INVALIDATE);
						::EnableWindow(hTc, TRUE);
					}
				}
			}
			::MapWindowPoints(hwnd, hTc, &info.pt, 1);
			int i = TabCtrl_HitTest(hTc, &info);
			if (i != -1)
			{
				TCITEM item;
				item.mask = TCIF_STATE;
				item.dwStateMask = TCIS_HIGHLIGHTED;
				TabCtrl_GetItem(hTc, i, &item);
				item.dwState ^= TCIS_HIGHLIGHTED;
				TabCtrl_SetItem(hTc, i, &item);
				::EnableWindow(hTc, TRUE);
			}
		}
		break;
	case WM_RBUTTONDOWN:
		if (HWND const hTc = ::GetDlgItem(hwnd, 100))
		{
			if (HWND const hCb = reinterpret_cast<HWND>(::GetWindowLongPtr(hTc, GWLP_USERDATA)))
			{
				::SendMessage(hCb, CB_SHOWDROPDOWN, 0, 0);
			}
		}
		break;
	case WM_HOTKEY:
		if (wParam == IDCANCEL)
		{
			if (HWND const hTc = ::GetDlgItem(hwnd, 100))
			{
				if (HWND const hCb = reinterpret_cast<HWND>(::GetWindowLongPtr(hTc, GWLP_USERDATA)))
				{
					::EnableWindow(hTc, FALSE);
					::SendMessage(hCb, CB_SHOWDROPDOWN, 0, 0);
				}
			}
		}
		break;
	case WM_DESTROY:
		if (HMENU const hMenu = ::GetMenu(hwnd))
			::DestroyMenu(hMenu);
		break;
	case WM_WINDOWPOSCHANGED:
		// Redraw window including frame to remove possible animation leftovers
		::RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME);
		break;
	}
	return ::CallWindowProc(pfnSuper, hwnd, message, wParam, lParam);
}

/**
 * @brief Handles the CBN_DROPDOWN notification.
 * @param [in] hCb Handle to ComboBox control.
 * @param [in] columns Number of columns to fit in one line.
 * @param [in] fixedPatterns Semicolon delimited list of wildcard patterns.
 * @param [in] allowUserAddedPatterns Whether to allow user-added patterns
 */
void WildcardDropList::OnDropDown(HWND hCb,
	int columns, LPCTSTR fixedPatterns, bool allowUserAddedPatterns, HMENU hMenu)
{
	COMBOBOXINFO info;
	info.cbSize = sizeof info;
	if (!::GetComboBoxInfo(hCb, &info))
		return;
	RECT rc;
	::GetClientRect(info.hwndList, &rc);
	LONG_PTR pfnSuper = ::SetWindowLongPtr(info.hwndList, GWLP_WNDPROC, (LONG_PTR)LbWndProc);
	::SetWindowLongPtr(info.hwndList, GWLP_USERDATA, pfnSuper);
	::SetWindowLongPtr(info.hwndList, GWLP_ID, (LONG_PTR)hMenu);
	if (hMenu)
	{
		if (int n = ::GetMenuItemCount(hMenu))
		{
			RECT rcMenu;
			::GetMenuItemRect(info.hwndList, hMenu, 0, &rcMenu);
			rc.top -= rcMenu.top;
			::GetMenuItemRect(info.hwndList, hMenu, n - 1, &rcMenu);
			rc.top += rcMenu.bottom;
		}
	}
	int const cxCross = ::GetSystemMetrics(SM_CXVSCROLL) - 1;
	HWND const hTc = ::CreateWindow(WC_TABCONTROL, NULL,
		WS_CHILD | WS_VISIBLE | WS_DISABLED | TCS_BUTTONS |
		TCS_FIXEDWIDTH | TCS_FORCELABELLEFT | TCS_MULTILINE,
		0, rc.top, rc.right - cxCross, 10000,
		info.hwndList, reinterpret_cast<HMENU>(100), NULL, NULL);
	::SetWindowLongPtr(hTc, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hCb));
	::SendMessage(hTc, WM_SETFONT, ::SendMessage(hCb, WM_GETFONT, 0, 0), 0);
	if (HDC const hDC = ::GetDC(hTc))
	{
		SIZE size;
		::GetTextExtentPoint(hDC, _T("0"), 1, &size);
		TabCtrl_SetItemSize(hTc, (rc.right - cxCross) / columns - 3, size.cy + 4);
		::ReleaseDC(hTc, hDC);
	}
	int const len = ::GetWindowTextLength(hCb) + 1;
	TCHAR *const patterns = static_cast<TCHAR *>(_alloca(len * sizeof(TCHAR)));
	::GetWindowText(hCb, patterns, len);
	int i = 0;
	LPCTSTR pch = fixedPatterns;
	while (size_t const cch = _tcscspn(pch += _tcsspn(pch, _T("; ")), _T("; ")))
	{
		TCHAR text[20];
		*std::copy<>(pch, pch + std::min<>(cch, _countof(text) - 1), text) = _T('\0');
		TCITEM item;
		item.dwStateMask = TCIS_HIGHLIGHTED;
		item.dwState = (patterns[0] && PathMatchSpec(text, patterns)) ? TCIS_HIGHLIGHTED : 0;
		item.pszText = text;
		item.mask = TCIF_TEXT;
		TabCtrl_InsertItem(hTc, i, &item);
		item.mask = TCIF_STATE;
		TabCtrl_SetItem(hTc, i, &item);
		++i;
		pch += cch;
	}
	if (allowUserAddedPatterns)
	{
		pch = patterns;
		while (size_t const cch = _tcscspn(pch += _tcsspn(pch, _T("; ")), _T("; ")))
		{
			TCHAR text[20];
			*std::copy<>(pch, pch + std::min<>(cch, _countof(text) - 1), text) = _T('\0');
			if (!fixedPatterns[0] || !PathMatchSpec(text, fixedPatterns))
			{
				TCITEM item;
				item.dwStateMask = TCIS_HIGHLIGHTED;
				item.dwState = TCIS_HIGHLIGHTED;
				item.pszText = text;
				item.mask = TCIF_TEXT;
				TabCtrl_InsertItem(hTc, i, &item);
				item.mask = TCIF_STATE;
				TabCtrl_SetItem(hTc, i, &item);
				++i;
			}
			pch += cch;
		}
	}
	TabCtrl_SetCurSel(hTc, -1);
	TabCtrl_AdjustRect(hTc, FALSE, &rc);
	rc.right = static_cast<int>(::SendMessage(hCb, CB_GETDROPPEDWIDTH, 0, 0));
	::SetWindowPos(info.hwndList, NULL, 0, 0, rc.right, rc.top - 2, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	::RegisterHotKey(info.hwndList, IDCANCEL, 0, VK_ESCAPE);
	::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}

/**
 * @brief Handles the CBN_CLOSEUP notification.
 * @param [in] hCb Handle to ComboBox control.
 */
HWND WildcardDropList::OnCloseUp(HWND hCb)
{
	COMBOBOXINFO info;
	info.cbSize = sizeof info;
	if (!::GetComboBoxInfo(hCb, &info))
		return NULL;
	::UnregisterHotKey(info.hwndList, IDCANCEL);
	HWND ret = NULL;
	if (HWND const hTc = ::GetDlgItem(info.hwndList, 100))
	{
		if (::IsWindowEnabled(hTc))
		{
			TCHAR text[20];
			int const n = TabCtrl_GetItemCount(hTc);
			TCHAR *const patterns = static_cast<TCHAR *>(
				_alloca(n * _countof(text) * sizeof(TCHAR)));
			TCHAR *pch = patterns;
			for (int i = 0; i < n; ++i)
			{
				TCITEM item;
				item.pszText = text;
				item.cchTextMax = _countof(text);
				item.mask = TCIF_TEXT | TCIF_STATE;
				item.dwStateMask = TCIS_HIGHLIGHTED;
				TabCtrl_GetItem(hTc, i, &item);
				if (item.dwState & TCIS_HIGHLIGHTED)
				{
					if (pch > patterns)
						*pch++ = _T(';');
					while (TCHAR ch = *item.pszText++)
						*pch++ = ch;
				}
			}
			*pch = _T('\0');
			::SetWindowText(hCb, patterns);
			::SendMessage(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
			ret = info.hwndList;
		}
		::DestroyWindow(hTc);
	}
	LONG_PTR pfnSuper = ::SetWindowLongPtr(info.hwndList, GWLP_USERDATA, 0);
	::SetWindowLongPtr(info.hwndList, GWLP_WNDPROC, pfnSuper);
	return ret;
}

LRESULT WildcardDropList::LvWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnSuper = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
			TCHAR text[4096];
			LONG_PTR data;
		case CBN_CLOSEUP:
			if (HWND const hwndList = OnCloseUp(reinterpret_cast<HWND>(lParam)))
			{
				::GetWindowText(reinterpret_cast<HWND>(lParam), text, _countof(text));
				data = ::GetWindowLongPtr(reinterpret_cast<HWND>(lParam), GWLP_USERDATA);
				ListView_SetItemText(hwnd, SHORT LOWORD(data), SHORT HIWORD(data), text);
				if (HMENU const hMenu = GetMenu(hwndList))
				{
					LVITEM item;
					item.mask = LVIF_PARAM;
					item.iItem = SHORT LOWORD(data);
					item.iSubItem = 0;
					if (ListView_GetItem(hwnd, &item))
					{
						int const n = GetMenuItemCount(hMenu);
						for (int i = 0; i < n; ++i)
						{
							MENUITEMINFO mii;
							mii.cbSize = sizeof mii;
							mii.fMask = MIIM_STATE | MIIM_DATA;
							if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
							{
								if (mii.fState & MF_CHECKED)
									item.lParam |= mii.dwItemData;
								else
									item.lParam &= ~mii.dwItemData;
							}
						}
						ListView_SetItem(hwnd, &item);
					}
				}
			}
			::DestroyWindow(reinterpret_cast<HWND>(lParam));
			::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pfnSuper);
			::SetFocus(hwnd);
			break;
		case CBN_SELENDOK:
			::GetWindowText(reinterpret_cast<HWND>(lParam), text, _countof(text));
			data = ::GetWindowLongPtr(reinterpret_cast<HWND>(lParam), GWLP_USERDATA);
			ListView_SetItemText(hwnd, SHORT LOWORD(data), SHORT HIWORD(data), text);
			break;
		}
		return 0;
	}
	return ::CallWindowProc(pfnSuper, hwnd, message, wParam, lParam);
}

void WildcardDropList::OnItemActivate(HWND hLv, int iItem, int iSubItem,
	int columns, LPCTSTR fixedPatterns, bool allowUserAddedPatterns, HMENU hMenu)
{
	RECT rc;
	ListView_EnsureVisible(hLv, iItem, FALSE);
	ListView_GetSubItemRect(hLv, iItem, iSubItem, LVIR_BOUNDS, &rc);
	TCHAR text[4096];
	ListView_GetItemText(hLv, iItem, iSubItem, text, _countof(text));
	HWND hCb = ::CreateWindow(WC_COMBOBOX, NULL, WS_CHILD | WS_VISIBLE |
		WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL | CBS_NOINTEGRALHEIGHT,
		rc.left, rc.top - 1, rc.right - rc.left, 0,
		hLv, reinterpret_cast<HMENU>(1), NULL, NULL);
	::SetWindowLongPtr(hCb, GWLP_USERDATA, MAKELPARAM(iItem, iSubItem));
	::SetWindowText(hCb, text);
	::SendMessage(hCb, WM_SETFONT, ::SendMessage(hLv, WM_GETFONT, 0, 0), 0);
	::SetFocus(hCb);
	LONG_PTR pfnSuper = ::SetWindowLongPtr(hLv, GWLP_WNDPROC, (LONG_PTR)LvWndProc);
	::SetWindowLongPtr(hLv, GWLP_USERDATA, pfnSuper);
	if (hMenu)
	{
		LVITEM item;
		item.mask = LVIF_PARAM;
		item.iItem = iItem;
		item.iSubItem = 0;
		if (ListView_GetItem(hLv, &item))
		{
			MENUITEMINFO mii;
			mii.cbSize = sizeof mii;
			mii.fType = MFT_OWNERDRAW;
			int const n = ::GetMenuItemCount(hMenu);
			for (int i = 0; i < n; ++i)
			{
				mii.fMask = MIIM_STATE | MIIM_DATA;
				if (::GetMenuItemInfo(hMenu, i, TRUE, &mii))
				{
					mii.fMask = MIIM_STATE | MIIM_TYPE;
					if (item.lParam & mii.dwItemData)
						mii.fState |= MF_CHECKED;
					::SetMenuItemInfo(hMenu, i, TRUE, &mii);
					mii.fType |= MFT_MENUBARBREAK;
				}
			}
		}
	}
	OnDropDown(hCb, columns, fixedPatterns, allowUserAddedPatterns, hMenu);
	::SendMessage(hCb, CB_SHOWDROPDOWN, TRUE, 0);
}
