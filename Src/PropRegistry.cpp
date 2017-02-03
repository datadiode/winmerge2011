/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  PropRegistry.cpp
 *
 * @brief PropRegistry implementation file
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "Constants.h"
#include "Environment.h"
#include "PropRegistry.h"
#include "Common/RegKey.h"
#include "Common/coretools.h"
#include "Common/SuperComboBox.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropRegistry::PropRegistry()
: OptionsPanel(IDD_PROPPAGE_SYSTEM, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropRegistry::UpdateData()
{
	DDX_Text<op>(IDC_EXT_EDITOR_PATH, m_strEditorPath);
	DDX_Check<op>(IDC_USE_SHELL_FILE_OPERATIONS, m_bUseShellFileOperations);
	DDX_Check<op>(IDC_USE_RECYCLE_BIN, m_bUseRecycleBin);
	DDX_Check<op>(IDC_USE_SHELL_FILE_BROWSE_DIALOGS, m_bUseShellFileBrowseDialogs);
	DDX_Text<op>(IDC_SUPPLEMENT_FOLDER, m_supplementFolder);
	DDX_Check<op>(IDC_TMPFOLDER_CUSTOM, m_tempFolderType, 0);
	DDX_Check<op>(IDC_TMPFOLDER_SYSTEM, m_tempFolderType, 1);
	DDX_Text<op>(IDC_TMPFOLDER_NAME, m_tempFolder);
	return true;
}

BOOL PropRegistry::OnInitDialog()
{
	OptionsPanel::OnInitDialog();
	if (HComboBox *pCb = static_cast<HComboBox *>(GetDlgItem(IDC_SUPPLEMENT_FOLDER)))
	{
		TCHAR path[MAX_PATH];
		if (SHGetSpecialFolderPath(NULL, path, CSIDL_APPDATA, FALSE))
		{
			PathAppend(path, WinMergeDocumentsFolder);
			pCb->AddString(path);
		}
		if (SHGetSpecialFolderPath(NULL, path, CSIDL_COMMON_APPDATA, FALSE))
		{
			PathAppend(path, WinMergeDocumentsFolder);
			pCb->AddString(path);
		}
		if (SHGetSpecialFolderPath(NULL, path, CSIDL_MYDOCUMENTS, FALSE))
		{
			PathAppend(path, WinMergeDocumentsFolder);
			pCb->AddString(path);
		}
		if (SHGetSpecialFolderPath(NULL, path, CSIDL_COMMON_DOCUMENTS, FALSE))
		{
			PathAppend(path, WinMergeDocumentsFolder);
			pCb->AddString(path);
		}
		if (GetEnvironmentVariable(_T("PortableRoot"), path, _countof(path)))
		{
			wsprintf(path, _T("%%PortableRoot%%ProgramData\\%s"), WinMergeDocumentsFolder);
			pCb->AddString(path);
		}
	}
	return TRUE;
}

LRESULT PropRegistry::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_USE_SHELL_FILE_OPERATIONS, BN_CLICKED):
			UpdateScreen();
			break;
		case MAKEWPARAM(IDC_EXT_EDITOR_BROWSE, BN_CLICKED):
			OnBrowseEditor();
			break;
		case MAKEWPARAM(IDC_SUPPLEMENT_FOLDER_BROWSE, BN_CLICKED):
			OnBrowseSupplementFolder();
			break;
		case MAKEWPARAM(IDC_TMPFOLDER_BROWSE, BN_CLICKED):
			OnBrowseTmpFolder();
			break;
		case MAKEWPARAM(IDC_SUPPLEMENT_FOLDER, CBN_DROPDOWN):
			reinterpret_cast<HSuperComboBox *>(lParam)->AdjustDroppedWidth();
			break;
		case IDC_LABEL_SUPPLEMENT_FOLDER:
			if ((UINT)ShellExecute(NULL, _T("open"),
				env_ExpandVariables(m_supplementFolder.c_str()).c_str(),
				NULL, NULL, SW_SHOWNORMAL) > 32)
			{
				MessageReflect_WebLinkButton<WM_COMMAND>(wParam, lParam);
			}
			else
			{
				MessageBeep(0); // unable to open folder
			}
			break;
		}
		break;
	case WM_DRAWITEM:
		switch (wParam)
		{
		case IDC_LABEL_SUPPLEMENT_FOLDER:
			return MessageReflect_WebLinkButton<WM_DRAWITEM>(wParam, lParam);
		}
		break;
	case WM_SETCURSOR:
		switch (::GetDlgCtrlID(reinterpret_cast<HWND>(wParam)))
		{
		case IDC_LABEL_SUPPLEMENT_FOLDER:
			return MessageReflect_WebLinkButton<WM_SETCURSOR>(wParam, lParam);
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropRegistry::ReadOptions()
{
	m_strEditorPath = COptionsMgr::Get(OPT_EXT_EDITOR_CMD);
	m_bUseShellFileOperations = COptionsMgr::Get(OPT_USE_SHELL_FILE_OPERATIONS);
	m_bUseRecycleBin = COptionsMgr::Get(OPT_USE_RECYCLE_BIN);
	m_bUseShellFileBrowseDialogs = COptionsMgr::Get(OPT_USE_SHELL_FILE_BROOWSE_DIALOGS);
	m_supplementFolder = COptionsMgr::Get(OPT_SUPPLEMENT_FOLDER);
	m_tempFolderType = COptionsMgr::Get(OPT_USE_SYSTEM_TEMP_PATH);
	m_tempFolder = COptionsMgr::Get(OPT_CUSTOM_TEMP_PATH);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropRegistry::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_USE_SHELL_FILE_OPERATIONS, m_bUseShellFileOperations != FALSE);
	COptionsMgr::SaveOption(OPT_USE_RECYCLE_BIN, m_bUseRecycleBin != FALSE);
	COptionsMgr::SaveOption(OPT_USE_SHELL_FILE_BROOWSE_DIALOGS, m_bUseShellFileBrowseDialogs != FALSE);
	string_trim_ws(m_strEditorPath);
	if (m_strEditorPath.empty())
		m_strEditorPath = COptionsMgr::GetDefault(OPT_EXT_EDITOR_CMD);
	COptionsMgr::SaveOption(OPT_EXT_EDITOR_CMD, m_strEditorPath);
	string_trim_ws(m_supplementFolder);
	COptionsMgr::SaveOption(OPT_SUPPLEMENT_FOLDER, m_supplementFolder);
	COptionsMgr::SaveOption(OPT_USE_SYSTEM_TEMP_PATH, m_tempFolderType != 0);
	string_trim_ws(m_tempFolder);
	COptionsMgr::SaveOption(OPT_CUSTOM_TEMP_PATH, m_tempFolder);
}

void PropRegistry::UpdateScreen()
{
	GetDlgItem(IDC_USE_RECYCLE_BIN)->EnableWindow(m_bUseShellFileOperations);
	UpdateData<Set>();
}

/// Open file browse dialog to locate editor
void PropRegistry::OnBrowseEditor()
{
	String sCmd = m_strEditorPath;
	String sExecutable;
	DecorateCmdLine(sCmd, sExecutable);
	if (SelectFile(m_hWnd, sExecutable, IDS_OPEN_TITLE, IDS_PROGRAMFILES, TRUE))
	{
		sExecutable.swap(m_strEditorPath);
		UpdateData<Set>();
	}
}

/// Open Folder selection dialog for user to select filter folder.
void PropRegistry::OnBrowseSupplementFolder()
{
	if (SelectFolder(m_hWnd, m_supplementFolder, 0))
		UpdateData<Set>();
}

/// Select temporary files folder.
void PropRegistry::OnBrowseTmpFolder()
{
	if (SelectFolder(m_hWnd, m_tempFolder, 0))
		UpdateData<Set>();
}
