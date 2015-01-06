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
 * @file  CCPrompt.cpp
 *
 * @brief Implementation file for ClearCase dialog
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "CCPrompt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCCPrompt::CCCPrompt()
	: ODialog(IDD_CLEARCASE)
	, m_bMultiCheckouts(FALSE)
	, m_bCheckin(FALSE)
{
}

template<ODialog::DDX_Operation op>
bool CCCPrompt::UpdateData()
{
	DDX_Text<op>(IDC_COMMENTS, m_comments);
	DDX_Check<op>(IDC_MULTI_CHECKOUT, m_bMultiCheckouts);
	DDX_Check<op>(IDC_CHECKIN, m_bCheckin);
	return true;
}

LRESULT CCCPrompt::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
		case IDC_SAVE_AS:
			UpdateData<Get>();
			// fall through
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/**
 * @brief Handler for WM_INITDIALOG; conventional location to initialize
 * controls. At this point dialog and control windows exist.
 */
BOOL CCCPrompt::OnInitDialog() 
{
	ODialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);
	UpdateData<Set>();
	return TRUE;
}
