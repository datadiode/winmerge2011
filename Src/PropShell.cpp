/** 
 * @file  PropShell.h
 *
 * @brief Implementation file for Shell Options dialog.
 *
 */
#include "StdAfx.h"
#include "OptionsPanel.h"
#include "resource.h"
#include "PropShell.h"
#include "SettingStore.h"
#include "Common/RegKey.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Flags for enabling and mode of extension
#define CONTEXT_F_ENABLED 0x01
#define CONTEXT_F_ADVANCED 0x02
#define CONTEXT_F_SUBFOLDERS 0x04
#define CONTEXT_F_FLATTENED 0x08

/// Registry path to WinMerge
static const TCHAR f_RegDir[] = _T("Software\\Thingamahoochie\\WinMerge");

// registry values
static const TCHAR f_RegValueEnabled[] = _T("ContextMenuEnabled");

static BOOL OwnsShellExtension()
{
	TCHAR exe[MAX_PATH];
	GetModuleFileName(NULL, exe, _countof(exe));
	TCHAR dll[MAX_PATH];
	static const TCHAR subkey[] = _T("CLSID\\{4E716236-AA30-4C65-B225-D68BBA81E9C2}\\InprocServer32");
	return SHRegGetPath(HKEY_CLASSES_ROOT, subkey, NULL, dll, 0) == ERROR_SUCCESS
		&& exe + PathCommonPrefix(exe, dll, NULL) + 1 == PathFindFileName(exe);
}

PropShell::PropShell() 
: OptionsPanel(IDD_PROPPAGE_SHELL, sizeof *this)
, m_bOwnsShellExtension(OwnsShellExtension())
{
}

void PropShell::UpdateScreen()
{
	GetDlgItem(IDC_EXPLORER_CONTEXT)->EnableWindow(m_bOwnsShellExtension);
	BOOL bEnable = m_bOwnsShellExtension && m_bContextAdded;
	GetDlgItem(IDC_EXPLORER_ADVANCED)->EnableWindow(bEnable);
	GetDlgItem(IDC_EXPLORER_SUBFOLDERS)->EnableWindow(bEnable);
	if (!m_bContextAdded)
	{
		m_bContextAdvanced = BST_UNCHECKED;
		m_bContextSubfolders = BST_UNCHECKED;
		m_bContextFlattened = BST_UNCHECKED;
	}
	if (m_bContextSubfolders != BST_CHECKED)
	{
		m_bContextFlattened = BST_UNCHECKED;
		bEnable = FALSE;
	}
	GetDlgItem(IDC_EXPLORER_IGNORE_FOLDER_STRUCTURE)->EnableWindow(bEnable);
	UpdateData<Set>();
}

template<ODialog::DDX_Operation op>
bool PropShell::UpdateData()
{
	DDX_Check<op>(IDC_ENABLE_DIR_SHELL_CONTEXT_MENU, m_bEnableDirShellContextMenu);
	DDX_Check<op>(IDC_ENABLE_MERGEEDIT_SHELL_CONTEXT_MENU, m_bEnableMegeEditShellContextMenu);
	DDX_Check<op>(IDC_EXPLORER_CONTEXT, m_bContextAdded);
	DDX_Check<op>(IDC_EXPLORER_ADVANCED, m_bContextAdvanced);
	DDX_Check<op>(IDC_EXPLORER_SUBFOLDERS, m_bContextSubfolders);
	DDX_Check<op>(IDC_EXPLORER_IGNORE_FOLDER_STRUCTURE, m_bContextFlattened);
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
		case MAKEWPARAM(IDC_EXPLORER_SUBFOLDERS, BN_CLICKED):
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
	if (m_bOwnsShellExtension)
		ReadContextRegValues();
	m_bEnableDirShellContextMenu = COptionsMgr::Get(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU);
	m_bEnableMegeEditShellContextMenu = COptionsMgr::Get(OPT_MERGEEDITVIEW_ENABLE_SHELL_CONTEXT_MENU);
}

/** 
 * @brief Writes options values from UI to storage.
 */
void PropShell::WriteOptions()
{
	COptionsMgr::SaveOption(OPT_DIRVIEW_ENABLE_SHELL_CONTEXT_MENU, m_bEnableDirShellContextMenu != BST_UNCHECKED);
	COptionsMgr::SaveOption(OPT_MERGEEDITVIEW_ENABLE_SHELL_CONTEXT_MENU, m_bEnableMegeEditShellContextMenu != BST_UNCHECKED);
	if (m_bOwnsShellExtension)
		SaveContextRegValues();
}

/// Get registry values for ShellExtension
void PropShell::ReadContextRegValues()
{
	CRegKeyEx reg;
	if (reg.Open(HKEY_CURRENT_USER, f_RegDir) == ERROR_SUCCESS)
	{
		// Read bitmask for shell extension settings
		DWORD dwContextEnabled = reg.ReadDword(f_RegValueEnabled, 0);

		if (dwContextEnabled & CONTEXT_F_ENABLED)
			m_bContextAdded = BST_CHECKED;

		if (dwContextEnabled & CONTEXT_F_ADVANCED)
			m_bContextAdvanced = BST_CHECKED;

		switch (dwContextEnabled & (CONTEXT_F_SUBFOLDERS | CONTEXT_F_FLATTENED))
		{
		case CONTEXT_F_FLATTENED:
			m_bContextSubfolders = BST_INDETERMINATE;
			break;
		case CONTEXT_F_SUBFOLDERS | CONTEXT_F_FLATTENED:
			m_bContextFlattened = BST_CHECKED;
		case CONTEXT_F_SUBFOLDERS:
			m_bContextSubfolders = BST_CHECKED;
			break;
		}
	}
}

/// Saves Context Menu settings
void PropShell::SaveContextRegValues()
{
	CRegKeyEx reg;
	if (reg.Open(HKEY_CURRENT_USER, f_RegDir) == ERROR_SUCCESS)
	{
		// Determine bitmask for shell extension
		DWORD dwContextEnabled = reg.ReadDword(f_RegValueEnabled, 0);
		dwContextEnabled &= ~(CONTEXT_F_ENABLED | CONTEXT_F_ADVANCED | CONTEXT_F_SUBFOLDERS | CONTEXT_F_FLATTENED);

		if (m_bContextAdded)
			dwContextEnabled |= CONTEXT_F_ENABLED;

		if (m_bContextAdvanced)
			dwContextEnabled |= CONTEXT_F_ADVANCED;

		if (m_bContextSubfolders == BST_CHECKED)
			dwContextEnabled |= CONTEXT_F_SUBFOLDERS;
		if (m_bContextFlattened == BST_CHECKED || m_bContextSubfolders == BST_INDETERMINATE)
			dwContextEnabled |= CONTEXT_F_FLATTENED;
			
		reg.WriteDword(f_RegValueEnabled, dwContextEnabled);
	}
}
