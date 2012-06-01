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
 * @file  WMGotoDlg.cpp
 *
 * @brief Implementation of the WMGotoDlg dialog.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MergeEditView.h"
#include "WMGotoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGotoDlg dialog

/**
 * @brief Constructor.
 */
WMGotoDlg::WMGotoDlg(CMergeEditView *pBuddy)
	: ODialog(IDD_WMGOTO)
	, m_pBuddy(pBuddy)
{
}

/**
 * @brief Initialize dialog.
 */
BOOL WMGotoDlg::OnInitDialog()
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	UpdateData<Set>();
	return TRUE;
}

template<ODialog::DDX_Operation op>
bool WMGotoDlg::UpdateData()
{
	DDX_Text<op>(IDC_WMGOTO_PARAM, m_strParam);
	DDX_Check<op>(IDC_WMGOTO_FILELEFT, m_nFile, 0);
	DDX_Check<op>(IDC_WMGOTO_FILERIGHT, m_nFile, 1);
	DDX_Check<op>(IDC_WMGOTO_TOLINE, m_nGotoWhat, 0);
	DDX_Check<op>(IDC_WMGOTO_TODIFF, m_nGotoWhat, 1);
	return true;
}

LRESULT WMGotoDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			UpdateData<Get>();
			// fall through
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break;
	case WM_SHOWWINDOW:
		if (wParam != FALSE)
			CenterWindow(m_pWnd, m_pBuddy->m_pWnd);
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}
