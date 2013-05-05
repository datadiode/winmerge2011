/** 
 * @file  PropBackups.cpp
 *
 * @brief Implementation of PropBackups propertysheet
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropBackups.h"
#include "FileOrFolderSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Constructor taking OptionsManager parameter.
 */
PropBackups::PropBackups()
: OptionsPanel(IDD_PROPPAGE_BACKUPS, sizeof *this)
{
}

template<ODialog::DDX_Operation op>
bool PropBackups::UpdateData()
{
	DDX_Check<op>(IDC_BACKUP_FOLDERCMP, m_bCreateForFolderCmp);
	DDX_Check<op>(IDC_BACKUP_FILECMP, m_bCreateForFileCmp);
	DDX_Text<op>(IDC_BACKUP_FOLDER, m_sGlobalFolder);
	DDX_Check<op>(IDC_BACKUP_APPEND_BAK, m_bAppendBak);
	DDX_Check<op>(IDC_BACKUP_APPEND_TIME, m_bAppendTime);
	DDX_Check<op>(IDC_BACKUP_ORIGFOLD, m_nBackupFolder, 0);
	DDX_Check<op>(IDC_BACKUP_GLOBALFOLD, m_nBackupFolder, 1);
	return true;
}

LRESULT PropBackups::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_BACKUP_BROWSE, BN_CLICKED):
			OnBnClickedBackupBrowse();
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropBackups::ReadOptions()
{
	m_bCreateForFolderCmp = COptionsMgr::Get(OPT_BACKUP_FOLDERCMP);
	m_bCreateForFileCmp = COptionsMgr::Get(OPT_BACKUP_FILECMP);
	m_nBackupFolder = COptionsMgr::Get(OPT_BACKUP_LOCATION);
	m_sGlobalFolder = COptionsMgr::Get(OPT_BACKUP_GLOBALFOLDER);
	m_bAppendBak = COptionsMgr::Get(OPT_BACKUP_ADD_BAK);
	m_bAppendTime = COptionsMgr::Get(OPT_BACKUP_ADD_TIME);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropBackups::WriteOptions()
{
	string_trim_ws(m_sGlobalFolder);
	if (m_sGlobalFolder.size() > 3 &&
		m_sGlobalFolder[m_sGlobalFolder.size() - 1] != '\\')
	{
		m_sGlobalFolder += _T("\\");
	}
	COptionsMgr::SaveOption(OPT_BACKUP_FOLDERCMP, m_bCreateForFolderCmp != FALSE);
	COptionsMgr::SaveOption(OPT_BACKUP_FILECMP, m_bCreateForFileCmp != FALSE);
	COptionsMgr::SaveOption(OPT_BACKUP_LOCATION, m_nBackupFolder);
	COptionsMgr::SaveOption(OPT_BACKUP_GLOBALFOLDER, m_sGlobalFolder);
	COptionsMgr::SaveOption(OPT_BACKUP_ADD_BAK, m_bAppendBak != FALSE);
	COptionsMgr::SaveOption(OPT_BACKUP_ADD_TIME, m_bAppendTime != FALSE);
}

/** 
 * @brief Called before propertysheet is drawn.
 */
void PropBackups::UpdateScreen()
{
	UpdateData<Set>();
}

/** 
 * @brief Called when user selects Browse-button.
 */
void PropBackups::OnBnClickedBackupBrowse()
{
	if (SelectFolder(m_hWnd, m_sGlobalFolder, 0))
	{
		SetDlgItemText(IDC_BACKUP_FOLDER, m_sGlobalFolder.c_str());
	}
}
