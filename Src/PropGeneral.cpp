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
 * @file  PropGeneral.h
 *
 * @brief Implementation file for PropGeneral propertyheet
 *
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "SettingStore.h"
#include "LanguageSelect.h"
#include "PropGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Constructor initialising members.
 */
PropGeneral::PropGeneral()
: OptionsPanel(IDD_PROPPAGE_GENERAL, sizeof *this)
{
}

PropGeneral::~PropGeneral()
{
}

BOOL PropGeneral::OnInitDialog()
{
	if (HComboBox *combo = static_cast<HComboBox *>(GetDlgItem(IDC_AUTO_COMPLETE_SOURCE)))
	{
		combo->AddString(LanguageSelect.LoadString(IDS_AUTOCOMPLETE_DISABLED).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_AUTOCOMPLETE_FILE_SYS).c_str());
		combo->AddString(LanguageSelect.LoadString(IDS_AUTOCOMPLETE_MRU).c_str());
	}
	return OptionsPanel::OnInitDialog();
}

template<ODialog::DDX_Operation op>
bool PropGeneral::UpdateData()
{
	DDX_Check<op>(IDC_SCROLL_CHECK, m_bScroll);
	DDX_Check<op>(IDC_DISABLE_SPLASH, m_bDisableSplash);
	DDX_Check<op>(IDC_SINGLE_INSTANCE, m_bSingleInstance);
	DDX_Check<op>(IDC_VERIFY_OPEN_PATHS, m_bVerifyPaths);
	DDX_Check<op>(IDC_ESC_CLOSES_WINDOW, m_bCloseWindowWithEsc);
	DDX_Check<op>(IDC_ASK_MULTIWINDOW_CLOSE, m_bAskMultiWindowClose);
	DDX_Check<op>(IDC_MULTIDOC_FILECMP, m_bMultipleFileCmp);
	DDX_Check<op>(IDC_MULTIDOC_DIRCMP, m_bMultipleDirCmp);
	DDX_CBIndex<op>(IDC_AUTO_COMPLETE_SOURCE, m_nAutoCompleteSource);
	DDX_Check<op>(IDC_PRESERVE_FILETIME, m_bPreserveFiletime);
	DDX_Check<op>(IDC_STARTUP_FOLDER_SELECT, m_bShowSelectFolderOnStartup);
	return true;
}

LRESULT PropGeneral::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_RESET_ALL_MESSAGE_BOXES, BN_CLICKED):
			OnResetAllMessageBoxes();
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropGeneral::ReadOptions()
{
	m_bScroll = COptionsMgr::Get(OPT_SCROLL_TO_FIRST);
	m_bDisableSplash = COptionsMgr::Get(OPT_DISABLE_SPLASH);
	m_bSingleInstance = COptionsMgr::Get(OPT_SINGLE_INSTANCE);
	m_bVerifyPaths = COptionsMgr::Get(OPT_VERIFY_OPEN_PATHS);
	m_bCloseWindowWithEsc = COptionsMgr::Get(OPT_CLOSE_WITH_ESC);
	m_bAskMultiWindowClose = COptionsMgr::Get(OPT_ASK_MULTIWINDOW_CLOSE);
	m_bMultipleFileCmp = COptionsMgr::Get(OPT_MULTIDOC_MERGEDOCS);
	m_bMultipleDirCmp = COptionsMgr::Get(OPT_MULTIDOC_DIRDOCS);
	m_nAutoCompleteSource = COptionsMgr::Get(OPT_AUTO_COMPLETE_SOURCE);
	m_bPreserveFiletime = COptionsMgr::Get(OPT_PRESERVE_FILETIMES);
	m_bShowSelectFolderOnStartup = COptionsMgr::Get(OPT_SHOW_SELECT_FILES_AT_STARTUP);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropGeneral::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_SCROLL_TO_FIRST, m_bScroll == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_DISABLE_SPLASH, m_bDisableSplash == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_SINGLE_INSTANCE, m_bSingleInstance == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_VERIFY_OPEN_PATHS, m_bVerifyPaths == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_CLOSE_WITH_ESC, m_bCloseWindowWithEsc == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_ASK_MULTIWINDOW_CLOSE, m_bAskMultiWindowClose == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_MULTIDOC_MERGEDOCS, m_bMultipleFileCmp == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_MULTIDOC_DIRDOCS, m_bMultipleDirCmp == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_AUTO_COMPLETE_SOURCE, m_nAutoCompleteSource);
	COptionsMgr::SaveOption(OPT_PRESERVE_FILETIMES, m_bPreserveFiletime == BST_CHECKED);
	COptionsMgr::SaveOption(OPT_SHOW_SELECT_FILES_AT_STARTUP, m_bShowSelectFolderOnStartup == BST_CHECKED);
}

void PropGeneral::UpdateScreen()
{
	UpdateData<Set>();
}

/** 
 * @brief Called when user wants to see all messageboxes again.
 */
void PropGeneral::OnResetAllMessageBoxes()
{
	// Delete the message box results stored in the registry.
	HKEY hAppKey = SettingStore.GetAppRegistryKey();
	SettingStore.SHDeleteKey(hAppKey, REGISTRY_SECTION_MESSAGEBOX);
	SettingStore.RegCloseKey(hAppKey);
	LanguageSelect.MsgBox(IDS_MESSAGE_BOX_ARE_RESET, MB_ICONINFORMATION);
}
