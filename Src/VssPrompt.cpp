/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  VssPrompt.cpp
 *
 * @brief Code for CVssPrompt class
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "VssPrompt.h"
#include "RegKey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVssPrompt dialog


/**
 * @brief Default constructor.
 * @param [in] pParent Pointer to parent component.
 */
CVssPrompt::CVssPrompt()
	: ODialog(IDD_VSS)
{
}

template<ODialog::DDX_Operation op>
bool CVssPrompt::UpdateData()
{
	DDX_CBStringExact<op>(IDC_PROJECT_COMBO, m_strProject);
	DDX_Text<op>(IDC_USER, m_strUser);
	DDX_Text<op>(IDC_PASSWORD, m_strPassword);
	DDX_Text<op>(IDC_MESSAGE, m_strMessage);
	DDX_CBStringExact<op>(IDC_DATABASE_LIST, m_strSelectedDatabase);
	DDX_Check<op>(IDC_MULTI_CHECKOUT, m_bMultiCheckouts);
	return true;
}

LRESULT CVssPrompt::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case IDC_SAVE_AS:
			EndDialog(wParam);
			break;
		case MAKEWPARAM(IDC_PROJECT_COMBO, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			RegisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE), MOD_SHIFT, VK_DELETE);
			break;
		case MAKEWPARAM(IDC_PROJECT_COMBO, CBN_CLOSEUP):
			UnregisterHotKey(m_hWnd, MAKEWPARAM(LOWORD(wParam), VK_DELETE));
			break;
		}
		break;
	case WM_HOTKEY:
		if (HIWORD(wParam) == VK_DELETE)
		{
			HComboBox *const pCb = static_cast<HComboBox *>(GetDlgItem(LOWORD(wParam)));
			pCb->DeleteString(pCb->GetCurSel());
		}
		break;
	}
	return ODialog::WindowProc(message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CVssPrompt message handlers

/**
 * @brief Initialize the dialog.
 * @return TRUE, unless focus is modified.
 */
BOOL CVssPrompt::OnInitDialog()
{
	LanguageSelect.TranslateDialog(m_hWnd);
	ODialog::OnInitDialog();

	m_pCbProject = static_cast<HSuperComboBox *>(GetDlgItem(IDC_PROJECT_COMBO));
	m_pCbDBCombo = static_cast<HComboBox *>(GetDlgItem(IDC_DATABASE_LIST));

	m_pCbProject->LoadState(_T("Vss"));

	int i = 0;
	CRegKeyEx reg;

	// Open key containing VSS databases
	static const TCHAR key[] = _T("SOFTWARE\\Microsoft\\SourceSafe\\Databases");
	if (reg.QueryRegMachine(key) != ERROR_SUCCESS &&
		reg.QueryRegUser(key) != ERROR_SUCCESS)
	{
		LanguageSelect.MsgBox(IDS_VSS_NODATABASES, MB_ICONERROR);
		EndDialog(IDCANCEL);
		return FALSE;
	}

	for (;;)
	{
		TCHAR cName[MAX_PATH];
		TCHAR cString[MAX_PATH];
		DWORD cssize = MAX_PATH;
		DWORD csize = MAX_PATH;
		LONG retval = RegEnumValue(reg, i, cName, &csize, NULL,
			NULL, reinterpret_cast<BYTE *>(cString), &cssize);
		if (retval != ERROR_SUCCESS && retval != ERROR_MORE_DATA)
			break;
		m_pCbDBCombo->InsertString(i, cString);
		i++;
	}
	UpdateData<Set>();
	return TRUE;
}

/**
 * @brief Close dialog with OK-button.
 */
void CVssPrompt::OnOK()
{
	UpdateData<Get>();
	if (m_strProject.empty())
	{
		LanguageSelect.MsgBox(IDS_NOPROJECT, MB_ICONSTOP);
		m_pCbProject->SetFocus();
		return;
	}

	m_pCbProject->SaveState(_T("Vss"));
	EndDialog(IDOK);
}
