/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

CAboutDlg::CAboutDlg() : ODialog(IDD_ABOUTBOX)
{
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
	case WM_SETCURSOR:
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(wParam)))
		{
		case IDC_WWW:
			return MessageReflect_WebLinkButton<WM_SETCURSOR>(wParam, lParam);
		}
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

	// Load application icon
	HICON icon = LanguageSelect.LoadIcon(IDR_MAINFRAME);
	SendDlgItemMessage(IDC_ABOUTBOX_ICON, STM_SETICON, reinterpret_cast<WPARAM>(icon));

	CVersionInfo version;

	TCHAR fmt[128];
	if (GetDlgItemText(IDC_VERSION, fmt, _countof(fmt)))
	{
		String s = version.GetProductVersion();
#ifdef _WIN64
		s += _T(" [64 bit]");
#endif
#ifdef _DEBUG
		s += _T(" [Debug]");
#endif
		s = string_format(fmt, s.c_str());
		SetDlgItemText(IDC_VERSION, s.c_str());
	}
	if (GetDlgItemText(IDC_PRIVATEBUILD, fmt, _countof(fmt)))
	{
		String s = version.GetPrivateBuild();
		if (!s.empty())
			s = string_format(fmt, s.c_str());
		SetDlgItemText(IDC_PRIVATEBUILD, s.c_str());
	}
	String copyright = version.GetLegalCopyright();
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
