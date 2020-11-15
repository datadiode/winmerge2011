/////////////////////////////////////////////////////////////////////////////
//    SPDX-License-Identifier: GPL-3.0-or-later
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  AboutDlg.cpp
 *
 * @brief Implementation of the About-dialog.
 *
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "AboutDlg.h"
#include "common/version.h"
#include "common/coretools.h"
#include "paths.h"
#include "coretools.h"

// https://www.gnu.org/graphics/gnu-ascii.html
// Copyright (c) 2001 Free Software Foundation, Inc.
// Converted with https://tomeko.net/online_tools/cpp_text_escape.php
static const char gnu_ascii[] =
"  ,           ,\n"
" /             \\\n"
"((__-^^-,-^^-__))\n"
" `-_---' `---_-'\n"
"  `--|o` 'o|--'\n"
"     \\  `  /\n"
"      ): :(\n"
"      :o_o:\n"
"       \"-\"";

static void MakeWebLinkButton(HWND hWnd, int nID)
{
	TCHAR szText[1024];
	HWND const hwndStatic = ::GetDlgItem(hWnd, nID);
	::GetWindowText(hwndStatic, szText, _countof(szText));
	HDC const hDC = ::GetDC(NULL);
	HFONT const hFont = (HFONT)::SendMessage(hwndStatic, WM_GETFONT, 0, 0);
	::SelectObject(hDC, hFont);
	if (LPTSTR const szLower = StrChr(szText, _T('[')))
	{
		StrTrim(szLower, _T("["));
		if (LPTSTR const szUpper = StrChr(szLower, _T(']')))
		{
			*szUpper = _T('\0');
			int y = 0;
			LPCTSTR szLine = szText;
			LPCTSTR q = szLower;
			RECT rgrc[2];
			::GetClientRect(hwndStatic, &rgrc[1]);
			::DrawText(hDC, szText, static_cast<int>(q - szText), &rgrc[1], DT_CALCRECT | DT_WORDBREAK);
			while (LPCTSTR const p = StrRChr(szText, q, _T(' ')))
			{
				q = p;
				::GetClientRect(hwndStatic, &rgrc[0]);
				::DrawText(hDC, szText, static_cast<int>(q - szText), &rgrc[0], DT_CALCRECT | DT_WORDBREAK);
				if (rgrc[1].bottom > rgrc[0].bottom)
				{
					y += rgrc[1].bottom - rgrc[0].bottom;
					rgrc[1].bottom = rgrc[0].bottom;
					if (szLine <= p)
						szLine = p + 1;
				}
			}
			::GetClientRect(hwndStatic, &rgrc[0]);
			::DrawText(hDC, szLine, static_cast<int>(szLower - szLine), &rgrc[0], DT_CALCRECT | DT_WORDBREAK);
			::GetClientRect(hwndStatic, &rgrc[1]);
			::DrawText(hDC, szLine, static_cast<int>(szUpper - szLine), &rgrc[1], DT_CALCRECT | DT_WORDBREAK);
			if (rgrc[1].bottom > rgrc[0].bottom)
			{
				y += rgrc[1].bottom - rgrc[0].bottom;
				szLower[-1] = '\n';
				rgrc[0].right = rgrc[0].top = 0;
				::DrawText(hDC, szLower, static_cast<int>(szUpper - szLower), &rgrc[1], DT_CALCRECT);
			}
			rgrc[1].right -= rgrc[0].right;
			rgrc[1].bottom -= rgrc[1].top;
			::MapWindowPoints(hwndStatic, hWnd, (LPPOINT)&rgrc, 2);
			HWND const hwndButton = ::CreateWindow(WC_BUTTON, szLower,
				WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY,
				rgrc[0].right, rgrc[0].top + y, rgrc[1].right, rgrc[1].bottom + 1,
				hWnd, (HMENU)nID, 0, 0);
			::SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, 0);
			::SetWindowPos(hwndButton, hwndStatic, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			*szUpper = _T(']');
			StrTrim(szUpper, _T("]"));
		}
		::SetWindowText(hwndStatic, szText);
	}
	::ReleaseDC(NULL, hDC);
}

CAboutDlg::CAboutDlg()
	: ODialog(IDD_ABOUTBOX)
	, m_font_gnu_ascii(NULL)
{
	if (HRSRC hRsrc = FindResource(NULL, MAKEINTRESOURCE(IDR_SPLASH), _T("IMAGE")))
	{
		if (HGLOBAL hGlob = LoadResource(NULL, hRsrc))
		{
			if (BYTE *pbData = static_cast<BYTE *>(LockResource(hGlob)))
			{
				DWORD cbData = SizeofResource(NULL, hRsrc);
				if (IStream *pstm = SHCreateMemStream(pbData, cbData))
				{
					OleLoadPicture(pstm, cbData, FALSE, IID_IPicture, (void**)&m_spIPicture);
					pstm->Release();
				}
			}
		}
	}
}

LRESULT CAboutDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(wParam);
			break;
		case IDC_OPEN_CONTRIBUTORS:
			OpenFileOrUrl(ContributorsPath, NULL);
			break;
		case IDC_WWW:
			if ((UINT)ShellExecute(NULL, _T("open"), WinMergeURL, NULL, NULL, SW_SHOWNORMAL) > 32)
				MessageReflect_WebLinkButton<WM_COMMAND>(wParam, lParam);
			else
				MessageBeep(0); // unable to execute file!
			break;
		case IDC_COMPANY:
			if (OpenFileOrUrl(LicenseFile, LicenceUrl))
				MessageReflect_WebLinkButton<WM_COMMAND>(wParam, lParam);
			else
				MessageBeep(0); // unable to execute file!
			break;
		}
		break;
	case WM_DRAWITEM:
		switch (wParam)
		{
		case IDC_WWW:
		case IDC_COMPANY:
			return MessageReflect_WebLinkButton<WM_DRAWITEM>(wParam, lParam);
		}
		break;
	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWindow *>(lParam)->GetDlgCtrlID() == IDC_GNU_ASCII)
			reinterpret_cast<HSurface *>(wParam)->SetTextColor(RGB(128, 128, 128));
		reinterpret_cast<HSurface *>(wParam)->SetBkMode(TRANSPARENT);
		return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
	case WM_CTLCOLORBTN:
		if (reinterpret_cast<HWindow *>(lParam)->GetDlgCtrlID() == IDC_COMPANY)
			return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
		break;
	case WM_ERASEBKGND:
		if (m_spIPicture)
		{
			HSurface *const pdc = reinterpret_cast<HSurface *>(wParam);
			RECT rc;
			GetClientRect(&rc);
			SIZE size;
			m_spIPicture->get_Width(&size.cx);
			m_spIPicture->get_Height(&size.cy);
			rc.top = MulDiv(rc.right, size.cy, size.cx);
			pdc->SetBkColor(GetSysColor(COLOR_BTNFACE));
			pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
			rc.bottom = rc.top;
			rc.top = 0;
			m_spIPicture->Render(pdc->m_hDC, 0, 0, rc.right, rc.bottom, 0, size.cy, size.cx, -size.cy, NULL);
			return TRUE;
		}
		break;
	case WM_SETCURSOR:
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(wParam)))
		{
		case IDC_WWW:
		case IDC_COMPANY:
			return MessageReflect_WebLinkButton<WM_SETCURSOR>(wParam, lParam);
		}
		break;
	case WM_NCDESTROY:
		VERIFY(m_font_gnu_ascii->DeleteObject());
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/**
 * @brief Read version info from resource to dialog.
 */
BOOL CAboutDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	if (HWindow *const ctrl = GetDlgItem(IDC_GNU_ASCII))
	{
		HSurface *pdc = ctrl->GetDC();
		int logpixelsy = pdc->GetDeviceCaps(LOGPIXELSY);
		int fontHeight = -MulDiv(14, logpixelsy, 72);
		m_font_gnu_ascii = HFont::Create(fontHeight, 0, 0, 0, FW_BOLD, FALSE,
			FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH, _T("Courier New"));
		ctrl->SetFont(m_font_gnu_ascii);
		SetDlgItemTextA(IDC_GNU_ASCII, gnu_ascii);
		ctrl->ReleaseDC(pdc);
	}

	CVersionInfo version;

	TCHAR fmt[128];
	if (GetDlgItemText(IDC_VERSION, fmt, _countof(fmt)))
	{
		String s = version.GetProductVersion();
		int n = s.length();
#ifdef _WIN64
		s += _T("\n64-bit");
#else
		s += _T("\n32-bit");
#endif
#ifdef _DEBUG
		s += _T(" Debug");
#endif
		String private_build = version.GetPrivateBuild();
		if (!private_build.empty())
			s += _T(" + ") + private_build;
		s = string_format(fmt, s.c_str());
		SetDlgItemText(IDC_VERSION, s.c_str());
	}
	String copyright = LanguageSelect.LoadString(IDS_SPLASH_GPLTEXT);
	copyright += _T("\n");
	copyright += version.GetLegalCopyright();
	String::size_type const pos = copyright.rfind(_T('\xA9'));
	if (pos != String::npos)
		copyright.insert(pos, 3, _T(' ')); // approximate an em space
	SetDlgItemText(IDC_COMPANY, copyright.c_str());
	MakeWebLinkButton(m_hWnd, IDC_COMPANY);
	return TRUE;
}

/**
 * @brief Open file, if it exists, else open url
 */
bool CAboutDlg::OpenFileOrUrl(LPCTSTR szFile, LPCTSTR szUrl)
{
	HINSTANCE ret = NULL;
	String docPath = GetModulePath() + szFile;
	if (paths_DoesPathExist(docPath.c_str()) == IS_EXISTING_FILE)
	{
		docPath.insert(docPath.begin(), _T('"'));
		docPath.push_back(_T('"'));
		ret = ShellExecute(m_hWnd, _T("open"), _T("notepad.exe"), docPath.c_str(), NULL, SW_SHOWNORMAL);
	}
	else if (szUrl)
	{
		ret = ShellExecute(NULL, _T("open"), szUrl, NULL, NULL, SW_SHOWNORMAL);
	}
	else
	{
		LanguageSelect.MsgBox(IDS_ERROR_FILE_NOT_FOUND, docPath.c_str(), MB_ICONSTOP);
	}
	return reinterpret_cast<UINT_PTR>(ret) > HINSTANCE_ERROR;
}
