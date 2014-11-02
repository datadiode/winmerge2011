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
 * @file  SaveClosingDlg.cpp
 *
 * @brief Implementation file for SaveClosingDlg dialog
 */
#include "StdAfx.h"
#include "Merge.h"
#include "paths.h"
#include "LanguageSelect.h"
#include "SaveClosingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SaveClosingDlg dialog

/**
 * @brief Constructor.
 * @param [in] pParent Dialog's parent window.
 */
SaveClosingDlg::SaveClosingDlg()
 : OResizableDialog(IDD_SAVECLOSING)
 , m_leftSave(SAVECLOSING_SAVE)
 , m_rightSave(SAVECLOSING_SAVE)
 , m_bAskForLeft(FALSE)
 , m_bAskForRight(FALSE)
 , m_bDisableCancel(FALSE)
{
	// configure how individual controls adjust when dialog resizes
	static const LONG FloatScript[] =
	{
		IDC_SAVECLOSING_LEFTFRAME,		BY<1000>::X2R,
		IDC_SAVECLOSING_LEFTFILE,		BY<1000>::X2R,
		IDC_SAVECLOSING_SAVELEFT,		BY<1000>::X2R,
		IDC_SAVECLOSING_DISCARDLEFT,	BY<1000>::X2R,
		IDC_SAVECLOSING_RIGHTFRAME,		BY<1000>::X2R,
		IDC_SAVECLOSING_RIGHTFILE,		BY<1000>::X2R,
		IDC_SAVECLOSING_SAVERIGHT,		BY<1000>::X2R,
		IDC_SAVECLOSING_DISCARDRIGHT,	BY<1000>::X2R,
		IDOK,							BY<1000>::X2L | BY<1000>::X2R,
		IDCANCEL,						BY<1000>::X2L | BY<1000>::X2R,
		0
	};
	CFloatState::FloatScript = FloatScript;
}

template<ODialog::DDX_Operation op>
bool SaveClosingDlg::UpdateData()
{
	DDX_Check<op>(IDC_SAVECLOSING_SAVELEFT, m_leftSave, 0);
	DDX_Check<op>(IDC_SAVECLOSING_DISCARDLEFT, m_leftSave, 1);
	DDX_Check<op>(IDC_SAVECLOSING_SAVERIGHT, m_rightSave, 0);
	DDX_Check<op>(IDC_SAVECLOSING_DISCARDRIGHT, m_rightSave, 1);
	return true;
}

LRESULT SaveClosingDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			OnOK();
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		case MAKEWPARAM(IDC_SAVECLOSING_DISCARDALL, BN_CLICKED):
			OnDiscardAll();
			break;
		}
		break;
	}
	return OResizableDialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// SaveClosingDlg message handlers

/**
 * @brief Initialize dialog.
 * @return Always TRUE.
 */
BOOL SaveClosingDlg::OnInitDialog()
{
	OResizableDialog::OnInitDialog();
	LanguageSelect.TranslateDialog(m_hWnd);

	paths_UndoMagic(m_sLeftFile);
	paths_UndoMagic(m_sRightFile);

	UpdateData<Set>();

	SetDlgItemText(IDC_SAVECLOSING_LEFTFILE, m_sLeftFile.c_str());
	SetDlgItemText(IDC_SAVECLOSING_RIGHTFILE, m_sRightFile.c_str());

	int idFocus = IDC_SAVECLOSING_SAVELEFT;
	if (!m_bAskForLeft)
	{
		// Left items disabled move focus to right side items
		idFocus = IDC_SAVECLOSING_SAVERIGHT;
		GetDlgItem(IDC_SAVECLOSING_LEFTFRAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_LEFTFILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_SAVELEFT)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_DISCARDLEFT)->EnableWindow(FALSE);
	}
	if (!m_bAskForRight)
	{
		GetDlgItem(IDC_SAVECLOSING_RIGHTFRAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_RIGHTFILE)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_SAVERIGHT)->EnableWindow(FALSE);
		GetDlgItem(IDC_SAVECLOSING_DISCARDRIGHT)->EnableWindow(FALSE);
	}
	if (m_bDisableCancel)
	{
		GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	}
	GetDlgItem(idFocus)->SetFocus();
	return FALSE;
}

void SaveClosingDlg::OnOK()
{
	UpdateData<Get>();
	EndDialog(IDOK);
}

/** 
 * @brief Called when 'Discard All' button is selected.
 */
void SaveClosingDlg::OnDiscardAll()
{
	m_leftSave = SAVECLOSING_DISCARD;
	m_rightSave = SAVECLOSING_DISCARD;
	EndDialog(IDOK);
}
