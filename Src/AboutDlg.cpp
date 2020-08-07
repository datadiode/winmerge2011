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
#include "Constants.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "AboutDlg.h"
#include "common/version.h"
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
			OnBnClickedOpenContributors();
			break;
		case IDC_WWW:
			if ((UINT)ShellExecute(NULL, _T("open"), WinMergeURL, NULL, NULL, SW_SHOWNORMAL) > 32)
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
			return MessageReflect_WebLinkButton<WM_DRAWITEM>(wParam, lParam);
		}
		break;
	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWindow *>(lParam)->GetDlgCtrlID() == IDC_GNU_ASCII)
			reinterpret_cast<HSurface *>(wParam)->SetTextColor(RGB(128, 128, 128));
		reinterpret_cast<HSurface *>(wParam)->SetBkMode(TRANSPARENT);
		return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
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
	SetDlgItemText(IDC_COMPANY, copyright.c_str());
	return TRUE;
}

/**
 * @brief Show contributors list.
 * Opens Contributors.txt into notepad.
 */
void CAboutDlg::OnBnClickedOpenContributors()
{
	String defPath = GetModulePath();
	// Don't add quotation marks yet, CFile doesn't like them
	String docPath = defPath + ContributorsPath;
	if (paths_DoesPathExist(docPath.c_str()) == IS_EXISTING_FILE)
	{
		// Now, add quotation marks so ShellExecute() doesn't fail if path
		// includes spaces
		docPath.insert(docPath.begin(), _T('"'));
		docPath.push_back(_T('"'));
		HINSTANCE ret = ShellExecute(m_hWnd, NULL, _T("notepad"), docPath.c_str(), defPath.c_str(), SW_SHOWNORMAL);

		// values < 32 are errors (ref to MSDN)
		if ((int)ret < 32)
		{
			// Try to open with associated application (.txt)
			ret = ShellExecute(m_hWnd, _T("open"), docPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			if ((int)ret < 32)
				LanguageSelect.MsgBox(IDS_ERROR_EXECUTE_FILE, _T("Notepad.exe"), MB_ICONSTOP);
		}
	}
	else
	{
		LanguageSelect.MsgBox(IDS_ERROR_FILE_NOT_FOUND, docPath.c_str(), MB_ICONSTOP);
	}
}
