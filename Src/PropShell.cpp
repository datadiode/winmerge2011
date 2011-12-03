/** 
 * @file  PropShell.h
 *
 * @brief Implementation file for Shell Options dialog.
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropShell.h"
#include "RegKey.h"
#include "OptionsDef.h"
#include "OptionsMgr.h"

String GetSysError(int nerr);
void LogErrorString(LPCTSTR sz);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Flags for enabling and mode of extension
#define CONTEXT_F_ENABLED 0x01
#define CONTEXT_F_ADVANCED 0x02
#define CONTEXT_F_SUBFOLDERS 0x04

 // registry dir to WinMerge
static const TCHAR f_RegDir[] = _T("Software\\Thingamahoochie\\WinMerge");

// registry values
static const TCHAR f_RegValueEnabled[] = _T("ContextMenuEnabled");


PropShell::PropShell() 
: OptionsPanel(IDD_PROPPAGE_SHELL)
, m_bEnableShellContextMenu(FALSE)
, m_bContextAdded(FALSE)
, m_bContextAdvanced(FALSE)
, m_bContextSubfolders(FALSE)
{
}

void PropShell::UpdateScreen()
{
	GetDlgItem(IDC_EXPLORER_ADVANCED)->EnableWindow(m_bContextAdded);
	GetDlgItem(IDC_EXPLORER_SUBFOLDERS)->EnableWindow(m_bContextAdded);
	if (!m_bContextAdded)
	{
		m_bContextAdvanced = FALSE;
		m_bContextSubfolders = FALSE;
	}
	UpdateData<Set>();
}

template<ODialog::DDX_Operation op>
bool PropShell::UpdateData()
{
	DDX_Check<op>(IDC_ENABLE_SHELL_CONTEXT_MENU, m_bEnableShellContextMenu);
	DDX_Check<op>(IDC_EXPLORER_CONTEXT, m_bContextAdded);
	DDX_Check<op>(IDC_EXPLORER_ADVANCED, m_bContextAdvanced);
	DDX_Check<op>(IDC_EXPLORER_SUBFOLDERS, m_bContextSubfolders);
	return true;
}

LRESULT PropShell::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (IsUserInputCommand(wParam))
			UpdateData<Get>();
		switch (wParam)
		{
		case MAKEWPARAM(IDC_EXPLORER_CONTEXT, BN_CLICKED):
			UpdateScreen();
			break;
		}
		break;
	}
	return OptionsPanel::WindowProc(message, wParam, lParam);
}

/** 
 * @brief Reads options values from storage to UI.
 */
void PropShell::ReadOptions()
{
	GetContextRegValues();
	m_bEnableShellContextMenu = COptionsMgr::Get(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropShell::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU, m_bEnableShellContextMenu != BST_UNCHECKED);
	SaveMergePath(); // saves context menu settings as well
}

/// Get registry values for ShellExtension
void PropShell::GetContextRegValues()
{
	CRegKeyEx reg;
	LONG retVal = reg.Open(HKEY_CURRENT_USER, f_RegDir);
	if (retVal != ERROR_SUCCESS)
	{
		string_format msg(
			_T("Failed to open registry key HKCU/%s:\n\t%d : %s"),
			f_RegDir, retVal, GetSysError(retVal).c_str());
		LogErrorString(msg.c_str());
		return;
	}

	// Read bitmask for shell extension settings
	DWORD dwContextEnabled = reg.ReadDword(f_RegValueEnabled, 0);

	if (dwContextEnabled & CONTEXT_F_ENABLED)
		m_bContextAdded = TRUE;

	if (dwContextEnabled & CONTEXT_F_ADVANCED)
		m_bContextAdvanced = TRUE;

	if (dwContextEnabled & CONTEXT_F_SUBFOLDERS)
		m_bContextSubfolders = TRUE;
}

/// Saves given path to registry for ShellExtension, and Context Menu settings
void PropShell::SaveMergePath()
{
	CRegKeyEx reg;
	LONG retVal = reg.Open(HKEY_CURRENT_USER, f_RegDir);
	if (retVal != ERROR_SUCCESS)
	{
		string_format msg(
			_T("Failed to open registry key HKCU/%s:\n\t%d : %s"),
			f_RegDir, retVal, GetSysError(retVal).c_str());
		LogErrorString(msg.c_str());
		return;
	}

	// Determine bitmask for shell extension
	DWORD dwContextEnabled = reg.ReadDword(f_RegValueEnabled, 0);
	if (m_bContextAdded)
		dwContextEnabled |= CONTEXT_F_ENABLED;
	else
		dwContextEnabled &= ~CONTEXT_F_ENABLED;

	if (m_bContextAdvanced)
		dwContextEnabled |= CONTEXT_F_ADVANCED;
	else
		dwContextEnabled &= ~CONTEXT_F_ADVANCED;

	if (m_bContextSubfolders)
		dwContextEnabled |= CONTEXT_F_SUBFOLDERS;
	else
		dwContextEnabled &= ~CONTEXT_F_SUBFOLDERS;

	retVal = reg.WriteDword(f_RegValueEnabled, dwContextEnabled);
	if (retVal != ERROR_SUCCESS)
	{
		string_format msg(
			_T("Failed to set registry value %s to %d:\n\t%d : %s"),
			f_RegValueEnabled, dwContextEnabled, retVal, GetSysError(retVal).c_str());
		LogErrorString(msg.c_str());
	}
}
