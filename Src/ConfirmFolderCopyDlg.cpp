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
 * @file  ConfirmFolderCopyDlg.cpp
 *
 * @brief Implementation file for ConfirmFolderCopyDlg dialog
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "stdafx.h"
#include "Merge.h"
#include "paths.h"
#include "LanguageSelect.h"
#include "ConfirmFolderCopyDlg.h"
#include "SettingStore.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ConfirmFolderCopyDlg dialog

ConfirmFolderCopyDlg::ConfirmFolderCopyDlg()
: OResizableDialog(IDD_CONFIRM_COPY)
, m_bMakeTargetItemWritable(FALSE)
{
	// configure how individual controls adjust when dialog resizes
	static const LONG FloatScript[] =
	{
		IDC_FLDCONFIRM_FROM_PATH,		BY<1000>::X2R,
		IDC_FLDCONFIRM_TO_PATH,			BY<1000>::X2R,
		IDYES,							BY<1000>::X2L | BY<1000>::X2R,
		IDNO,							BY<1000>::X2L | BY<1000>::X2R,
		0
	};
	CFloatState::FloatScript = FloatScript;
}

/**
 * @brief Handler for WM_INITDIALOG; conventional location to initialize
 * controls. At this point dialog and control windows exist.
 */
BOOL ConfirmFolderCopyDlg::OnInitDialog()
{
	OResizableDialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	// Load warning icon
	// TODO: we can have per-action icons?
	HICON icon = LoadIcon(NULL, IDI_EXCLAMATION);
	SendDlgItemMessage(IDC_FLDCONFIRM_ICON, STM_SETICON, (WPARAM)icon);

	if (!m_caption.empty())
		SetWindowText(m_caption.c_str());

	paths_UndoMagic(m_fromPath);
	paths_UndoMagic(m_toPath);

	SetDlgItemText(IDC_FLDCONFIRM_FROM_TEXT, m_fromText.c_str());
	SetDlgItemText(IDC_FLDCONFIRM_TO_TEXT, m_toText.c_str());
	SetDlgItemText(IDC_FLDCONFIRM_FROM_PATH, m_fromPath.c_str());
	SetDlgItemText(IDC_FLDCONFIRM_TO_PATH, m_toPath.c_str());
	SetDlgItemText(IDC_FLDCONFIRM_QUERY, m_question.c_str());
	m_bMakeTargetItemWritable = SettingStore.GetProfileInt(
		_T("Settings"), _T("DirViewMakeTargetItemWritable"), 0);
	CheckDlgButton(IDC_FLDCONFIRM_MAKE_WRITABLE, m_bMakeTargetItemWritable);

	return TRUE;
}

LRESULT ConfirmFolderCopyDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDYES:
			m_bMakeTargetItemWritable = IsDlgButtonChecked(IDC_FLDCONFIRM_MAKE_WRITABLE);
			SettingStore.WriteProfileInt(_T("Settings"),
				_T("DirViewMakeTargetItemWritable"), m_bMakeTargetItemWritable);
		case IDNO:
		case IDCANCEL:
			EndDialog(wParam);
			break;
		}
		break;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}
