/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
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
// Look at http://www.codeproject.com/shell/ for excellent guide
// to Windows Shell programming by Michael Dunn.
//
// This extension needs two registry values to be defined:
//  HKEY_CURRENT_USER\Software\Thingamahoochie\WinMerge\ContextMenuEnabled
//   defines if context menu is shown (extension enabled) and if
//   we show simple or advanced menu
//
//  HKEY_CURRENT_USER\Software\Thingamahoochie\WinMerge\FirstSelection
//   is used to store path for first selection in advanced mode
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  WinMergeShell.cpp
 *
 * @brief Implementation of the ShellExtension class
 */
#include "StdAfx.h"
#include "resource.h"
#include "ShellExtension.h"
#include "WinMergeShell.h"
#include "RegKey.h"
#include <atliface_h.h>

/**
 * @brief Flags for enabling and other settings of context menu.
 */
enum ExtensionFlags
{
	EXT_ENABLED = 0x01, /**< ShellExtension enabled/disabled. */
	EXT_ADVANCED = 0x02, /**< Advanced menuitems enabled/disabled. */
	EXT_SUBFOLDERS = 0x04, /**< Subfolders included by default? */
	EXT_FLATTENED = 0x08, /**< Ignore folder structure? */
};

/// Max. filecount to select
static const UINT MaxFileCount = 2;
/// Registry path to WinMerge
#define REGDIR _T("Software\\Thingamahoochie\\WinMerge")
static const TCHAR f_RegDir[] = REGDIR;
static const TCHAR f_RegLocaleDir[] = REGDIR _T("\\Locale");

/**
 * @name Registry valuenames.
 */
/*@{*/
/** Shell context menuitem enabled/disabled */
static const TCHAR f_RegValueEnabled[] = _T("ContextMenuEnabled");
/** 'Saved' path in advanced mode */
static const TCHAR f_FirstSelection[] = _T("FirstSelection");
/** LanguageId */
static const TCHAR f_LanguageId[] = _T("LanguageId");
/*@}*/

/**
 * @brief The states in which the menu can be.
 * These states define what items are added to the menu and how those
 * items work.
 */
enum
{
	MENU_SIMPLE,		/**< Simple menu, only "Compare item" is shown. */
	MENU_ONESEL_NOPREV,	/**< One item selected, no previous selections. */
	MENU_ONESEL_PREV,	/**< One item selected, previous selection exists. */
	MENU_TWOSEL,		/**< Two items are selected. */
};

void CWinMergeShell::SetWinMergeLocale()
{
	CRegKeyEx reg;
	if (reg.Open(HKEY_CURRENT_USER, f_RegLocaleDir) == ERROR_SUCCESS)
	{
		m_lcid = reg.ReadDword(f_LanguageId, 0x409);
	}
}

/**
 * @brief Load a string from resource.
 * @param [in] Resource string ID.
 * @return String loaded from resource.
 */
CMyComBSTR CWinMergeShell::GetResourceString(UINT id)
{
	UINT block = id / 16 + 1;
	UINT index = id % 16;
	if (HRSRC hrsrc = FindResourceEx(m_hInstance, RT_STRING,
		MAKEINTRESOURCE(block), LOWORD(m_lcid)))
	{
		if (LPTSTR pch = (LPTSTR)LoadResource(m_hInstance, hrsrc))
		{
			for (;;)
			{
				UINT len = *pch++;
				if (index == 0)
					return CMyComBSTR(len, pch);
				--index;
				pch += len;
			}
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CWinMergeShell

void *CWinMergeShell::operator new(size_t)
{
	static char buf[sizeof CWinMergeShell];
	return buf;
}

/// Default constructor, loads icon bitmap
CWinMergeShell::CWinMergeShell(HINSTANCE hInstance)
: m_cRef(0)
, m_hInstance(hInstance)
, m_dwMenuState(0)
, m_lcid(0)
{
	// compress or stretch icon bitmap according to menu item height
	m_MergeBmp = (HBITMAP)LoadImage(m_hInstance, MAKEINTRESOURCE(IDB_WINMERGE), IMAGE_BITMAP,
		GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK), LR_DEFAULTCOLOR);
}

CWinMergeShell::~CWinMergeShell()
{
	DeleteObject(m_MergeBmp);
}

/// Reads selected paths
HRESULT CWinMergeShell::Initialize(LPCITEMIDLIST pidlFolder,
		LPDATAOBJECT pDataObj, HKEY hProgID)
{
	SetWinMergeLocale();
	HRESULT hr = E_INVALIDARG;
	// Files/folders selected normally from the explorer
	if (pDataObj)
	{
		FORMATETC fmt = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		STGMEDIUM stg = {TYMED_HGLOBAL};
		HDROP hDropInfo;

		// Look for CF_HDROP data in the data object.
		if (FAILED(pDataObj->GetData(&fmt, &stg)))
			// Nope! Return an "invalid argument" error back to Explorer.
			return E_INVALIDARG;

		// Get a pointer to the actual data.
		hDropInfo = (HDROP) GlobalLock(stg.hGlobal);

		// Make sure it worked.
		if (NULL == hDropInfo)
			return E_INVALIDARG;

		// Sanity check & make sure there is at least one filename.
		UINT fileCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
		m_nSelectedItems = fileCount;

		if (fileCount == 0)
		{
			GlobalUnlock(stg.hGlobal);
			ReleaseStgMedium(&stg);
			return E_INVALIDARG;
		}

		hr = S_OK;

		// Get all file names.
		for (UINT i = 0 ; i < MaxFileCount; i++)
		{
			if (i >= fileCount)
			{
				m_strPaths[i].Empty();
			}
			else if (UINT len = DragQueryFile(hDropInfo, i, NULL, 0))
			{
				if (!SysReAllocStringLen(&m_strPaths[i].m_str, NULL, len))
					return E_OUTOFMEMORY;
				DragQueryFile(hDropInfo, i, m_strPaths[i].m_str, len + 1);
			}
		}
		GlobalUnlock(stg.hGlobal);
		ReleaseStgMedium(&stg);
	}

	// No item selected - selection is the folder background
	if (pidlFolder)
	{
		TCHAR szPath[MAX_PATH];
		if (SHGetPathFromIDList(pidlFolder, szPath))
		{
			m_strPaths[0] = szPath;
			m_nSelectedItems = 1;
			hr = S_OK;
		}
		else
			hr = E_INVALIDARG;
	}
	return hr;
}

/// Adds context menu item
HRESULT CWinMergeShell::QueryContextMenu(HMENU hmenu, UINT uMenuIndex,
		UINT uidFirstCmd, UINT uidLastCmd, UINT uFlags)
{
	int nItemsAdded = 0;
	SetWinMergeLocale();

	// If the flags include CMF_DEFAULTONLY then we shouldn't do anything.
	if (uFlags & CMF_DEFAULTONLY)
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

	// Check if user wants to use context menu
	CRegKeyEx reg;
	if (reg.Open(HKEY_CURRENT_USER, f_RegDir) != ERROR_SUCCESS)
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

	m_dwContextMenuEnabled = reg.ReadDword(f_RegValueEnabled, 0);
	m_strPreviousPath = reg.ReadString(f_FirstSelection, _T(""));

	if (m_dwContextMenuEnabled & EXT_ENABLED) // Context menu enabled
	{
		// Check if advanced menuitems enabled
		if ((m_dwContextMenuEnabled & EXT_ADVANCED) == 0)
		{
			m_dwMenuState = MENU_SIMPLE;
			nItemsAdded = DrawSimpleMenu(hmenu, uMenuIndex, uidFirstCmd);
		}
		else
		{
			if (m_nSelectedItems == 1 && m_strPreviousPath.Length() == 0)
				m_dwMenuState = MENU_ONESEL_NOPREV;
			else if (m_nSelectedItems == 1 && m_strPreviousPath.Length() != 0)
				m_dwMenuState = MENU_ONESEL_PREV;
			else if (m_nSelectedItems == 2)
				m_dwMenuState = MENU_TWOSEL;

			nItemsAdded = DrawAdvancedMenu(hmenu, uMenuIndex, uidFirstCmd);
		}

		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, nItemsAdded);
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
}

/// Gets string shown explorer's status bar when menuitem selected
HRESULT CWinMergeShell::GetCommandString(UINT_PTR idCmd, UINT uFlags,
		UINT* pwReserved, LPSTR pszName, UINT  cchMax)
{
	SetWinMergeLocale();

	// Check idCmd, it must be 0 in simple mode and 0 or 1 in advanced mode.
	if ((m_dwMenuState & EXT_ADVANCED) == 0)
	{
		if (idCmd > 0)
			return E_INVALIDARG;
	}
	else
	{
		if (idCmd > 1)
			return E_INVALIDARG;
	}

	// If Explorer is asking for a help string, copy our string into the
	// supplied buffer.
	if (uFlags & GCS_HELPTEXT)
	{
		CMyComBSTR strHelp = GetHelpText(idCmd);

		if (uFlags & GCS_UNICODE)
			// We need to cast pszName to a Unicode string, and then use the
			// Unicode string copy API.
			lstrcpynW((LPWSTR) pszName, strHelp, cchMax);
		else
			// Use the ANSI string copy API to return the help string.
			wnsprintfA(pszName, cchMax, "%ls", strHelp.m_str);

		return S_OK;
	}
	return E_INVALIDARG;
}

/// Runs WinMerge with given paths
HRESULT CWinMergeShell::InvokeCommand(LPCMINVOKECOMMANDINFO pCmdInfo)
{
	CRegKeyEx reg;
	BOOL bCompare = FALSE;
	SetWinMergeLocale();

	// If lpVerb really points to a string, ignore this function call and bail out.
	if (HIWORD(pCmdInfo->lpVerb) != 0)
		return E_INVALIDARG;

	// Guess path to WinMerge executable
	TCHAR strWinMergePath[MAX_PATH];
	GetModuleFileName(m_hInstance, strWinMergePath, _countof(strWinMergePath));
	PathRemoveFileSpec(strWinMergePath);
	PathAppend(strWinMergePath, _T("WinMergeU.exe"));

	// Check that file we are trying to execute exists
	if (!PathFileExists(strWinMergePath))
		return S_FALSE;

	PathQuoteSpaces(strWinMergePath);

	if (LOWORD(pCmdInfo->lpVerb) == 0)
	{
		switch (m_dwMenuState)
		{
		case MENU_SIMPLE:
			bCompare = TRUE;
			break;

		case MENU_ONESEL_NOPREV:
			m_strPreviousPath = m_strPaths[0];
			if (reg.Open(HKEY_CURRENT_USER, f_RegDir) == ERROR_SUCCESS)
				reg.WriteString(f_FirstSelection, m_strPreviousPath);
			break;

		case MENU_ONESEL_PREV:
			m_strPaths[1] = m_strPaths[0];
			m_strPaths[0] = m_strPreviousPath;
			bCompare = TRUE;

			// Forget previous selection
			if (reg.Open(HKEY_CURRENT_USER, f_RegDir) == ERROR_SUCCESS)
				reg.WriteString(f_FirstSelection, _T(""));
			break;

		case MENU_TWOSEL:
			// "Compare" - compare paths
			bCompare = TRUE;
			m_strPreviousPath.Empty();
			break;
		}
	}
	else if (LOWORD(pCmdInfo->lpVerb) == 1)
	{
		switch (m_dwMenuState)
		{
		case MENU_ONESEL_PREV:
			m_strPreviousPath = m_strPaths[0];
			if (reg.Open(HKEY_CURRENT_USER, f_RegDir) == ERROR_SUCCESS)
				reg.WriteString(f_FirstSelection, m_strPreviousPath);
			bCompare = FALSE;
			break;
		default:
			// "Compare..." - user wants to compare this single item and open WinMerge
			m_strPaths[1].Empty();
			bCompare = TRUE;
			break;
		}
	}
	else
		return E_INVALIDARG;

	if (bCompare == FALSE)
		return S_FALSE;

	DWORD dwAlter = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? EXT_SUBFOLDERS : 0;
	CMyComBSTR strCommandLine = FormatCmdLine(
		strWinMergePath, m_strPaths[0], m_strPaths[1], dwAlter);

	// Finally start a new WinMerge process
	STARTUPINFO si;
	SecureZeroMemory(&si, sizeof si);
	si.cb = sizeof si;
	PROCESS_INFORMATION pi;

	BOOL retVal = CreateProcess(NULL, strCommandLine,
			NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL,
			&si, &pi);

	if (!retVal)
		return S_FALSE;

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return S_OK;
}

/// Create menu for simple mode
int CWinMergeShell::DrawSimpleMenu(HMENU hmenu, UINT uMenuIndex,
		UINT uidFirstCmd)
{
	CMyComBSTR strMenu = GetResourceString(IDS_CONTEXT_MENU);
	InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd, strMenu);

	// Add bitmap
	if (m_MergeBmp != NULL)
		SetMenuItemBitmaps(hmenu, uMenuIndex, MF_BYPOSITION, m_MergeBmp, NULL);

	// Show menu item as grayed if more than two items selected
	if (m_nSelectedItems > MaxFileCount)
		EnableMenuItem(hmenu, uMenuIndex, MF_BYPOSITION | MF_GRAYED);

	return 1;
}

/// Create menu for advanced mode
int CWinMergeShell::DrawAdvancedMenu(HMENU hmenu, UINT uMenuIndex,
		UINT uidFirstCmd)
{
	CMyComBSTR strCompare = GetResourceString(IDS_COMPARE);
	CMyComBSTR strCompareEllipsis = GetResourceString(IDS_COMPARE_ELLIPSIS);
	CMyComBSTR strCompareTo = GetResourceString(IDS_COMPARE_TO);
	CMyComBSTR strReselect = GetResourceString(IDS_RESELECT_FIRST);
	int nItemsAdded = 0;

	switch (m_dwMenuState)
	{
		// No items selected earlier
		// Select item as first item to compare
	case MENU_ONESEL_NOPREV:
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strCompareTo);
		uMenuIndex++;
		uidFirstCmd++;
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strCompareEllipsis);
		nItemsAdded = 2;
		break;

		// One item selected earlier:
		// Allow re-selecting first item or selecting second item
	case MENU_ONESEL_PREV:
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strCompare);
		uMenuIndex++;
		uidFirstCmd++;
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strReselect);
		nItemsAdded = 2;
		break;

		// Two items selected
		// Select both items for compare
	case MENU_TWOSEL:
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strCompare);
		nItemsAdded = 1;
		break;

	default:
		InsertMenu(hmenu, uMenuIndex, MF_BYPOSITION, uidFirstCmd,
				strCompare);
		nItemsAdded = 1;
		break;
	}

	// Add bitmap
	if (m_MergeBmp != NULL)
	{
		if (nItemsAdded == 2)
			SetMenuItemBitmaps(hmenu, uMenuIndex - 1, MF_BYPOSITION, m_MergeBmp, NULL);
		SetMenuItemBitmaps(hmenu, uMenuIndex, MF_BYPOSITION, m_MergeBmp, NULL);
	}

	// Show menu item as grayed if more than two items selected
	if (m_nSelectedItems > MaxFileCount)
	{
		if (nItemsAdded == 2)
			EnableMenuItem(hmenu, uMenuIndex - 1, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(hmenu, uMenuIndex, MF_BYPOSITION | MF_GRAYED);
	}

	return nItemsAdded;
}

/// Determine help text shown in explorer's statusbar
CMyComBSTR CWinMergeShell::GetHelpText(UINT_PTR idCmd)
{
	CMyComBSTR strHelp;
	// More than two items selected, advice user
	if (m_nSelectedItems > MaxFileCount)
	{
		strHelp = GetResourceString(IDS_CONTEXT_HELP_MANYITEMS);
	}
	else if (idCmd == 0)
	{
		switch (m_dwMenuState)
		{
		case MENU_SIMPLE:
			strHelp = GetResourceString(IDS_CONTEXT_HELP);;
			break;

		case MENU_ONESEL_NOPREV:
			strHelp = GetResourceString(IDS_HELP_SAVETHIS);
			break;

		case MENU_ONESEL_PREV:
			strHelp = GetResourceString(IDS_HELP_COMPARESAVED);
			if (CMyComBSTR tail = strHelp.SplitAt(L"%1"))
			{
				strHelp += m_strPreviousPath.m_str;
				strHelp += tail;
			}
			break;

		case MENU_TWOSEL:
			strHelp = GetResourceString(IDS_CONTEXT_HELP);
			break;
		}
	}
	else if (idCmd == 1)
	{
		switch (m_dwMenuState)
		{
		case MENU_ONESEL_PREV:
			strHelp = GetResourceString(IDS_HELP_SAVETHIS);
			break;
		default:
			strHelp = GetResourceString(IDS_CONTEXT_HELP);
			break;
		}
	}
	// Return using move constructor
	return CMyComBSTR(&strHelp);
}

/// Format commandline used to start WinMerge
CMyComBSTR CWinMergeShell::FormatCmdLine(LPCTSTR winmergePath,
		LPCTSTR path1, LPCTSTR path2, DWORD flags)
{
	CMyComBSTR strCommandline = winmergePath;

	// Merge in flags from registry
	flags ^= m_dwContextMenuEnabled;
	// Consider EXT_FLATTENED only when working with flags as found in registry
	if (flags != m_dwContextMenuEnabled)
		flags &= ~EXT_FLATTENED;
		
	switch (flags & (EXT_SUBFOLDERS | EXT_FLATTENED))
	{
	case EXT_SUBFOLDERS:
		strCommandline += _T(" /r"); // Recurse
		break;
	case EXT_SUBFOLDERS | EXT_FLATTENED:
		strCommandline += _T(" /r2"); // Recurse but ignore folder structure
		break;
	case EXT_FLATTENED: // Follow options as previously adjusted in Open dialog
		strCommandline += _T(" /rp");
		break;
	}

	strCommandline += _T(" \"");
	strCommandline += path1;
	strCommandline += _T("\"");

	if (m_strPaths[1].Length() != 0)
	{
		strCommandline += _T(" \"");
		strCommandline += path2;
		strCommandline += _T("\"");
	}
	// Return using move constructor
	return CMyComBSTR(&strCommandline);
}

HRESULT CWinMergeShell::RegisterClassObject(BOOL bRegister)
{
	SetWinMergeLocale();
	HRESULT hr;
	CMyComPtr<IRegistrar> spRegistrar;
	if (FAILED(hr = spRegistrar.CoCreateInstance(CLSID_Registrar, IID_IRegistrar)))
		return hr;
	TCHAR module[MAX_PATH];
	GetModuleFileName(m_hInstance, module, _countof(module));
	if (FAILED(hr = spRegistrar->AddReplacement(L"MODULE", module)))
		return hr;
	if (bRegister)
		hr = spRegistrar->ResourceRegister(module, IDR_WINMERGESHELL, _T("REGISTRY"));
	else
		hr = spRegistrar->ResourceUnregister(module, IDR_WINMERGESHELL, _T("REGISTRY"));
	return hr;
}

HRESULT CWinMergeShell::QueryInterface(REFIID iid, void **ppv)
{
	static const QITAB rgqit[] = 
	{   
		QITABENT(CWinMergeShell, IClassFactory),
		QITABENT(CWinMergeShell, IShellExtInit),
		QITABENT(CWinMergeShell, IContextMenu),
		{ 0 }
	};
	return QISearch(this, rgqit, iid, ppv);
}

ULONG CWinMergeShell::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CWinMergeShell::Release()
{
	return InterlockedDecrement(&m_cRef);
}

HRESULT CWinMergeShell::CreateInstance(IUnknown *pUnk, const IID &iid, void **ppv)
{
	return QueryInterface(iid, ppv);
}

HRESULT CWinMergeShell::LockServer(BOOL)
{
	return E_NOTIMPL;
}
