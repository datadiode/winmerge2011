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

/* 7zCommon.cpp: Implement 7z related classes and functions
 * Copyright (c) 2003 Jochen Tucht
 *
 * Remarks:	Different versions of 7-Zip are interfaced through specific
 *			versions of Merge7z (Merge7z311.dll, Merge7z312.dll, etc.)
 *			WinMerge can either use an installed copy of the 7-Zip software,
 *			or fallback to a local set of 7-Zip DLLs, which are to be included
 *			in the WinMerge binary distribution.
 *
 *			Fallback policies are as follows:
 *
 *			1. Detect 7-Zip version installed (XXX).
 *			2. If there is a Merge7zXXX.dll, be happy to use it
 *			3. Detect 7-Zip version from WinMerge distribution (YYY).
 *			4. If there is a Merge7zYYY.dll, be happy to use it.
 *			5. Sorry, no way.
 *
 *			These rules can be customized by setting a registry variable
 *			*Merge7z/Enable* of type DWORD to one of the following values:
 *
 *			0 - Entirely disable 7-Zip integration.
 *			1 - Use installed 7-Zip if present. Otherwise, use local 7-Zip.
 *			2 - Always use local 7-Zip.
 *

Please mind 2. a) of the GNU General Public License, and log your changes below.

DATE:		BY:					DESCRIPTION:
==========	==================	================================================
2003-12-09	Jochen Tucht		Created
2003-12-16	Jochen Tucht		Properly generate .tar.gz and .tar.bz2
2003-12-16	Jochen Tucht		Obtain long path to temporary folder
2004-01-20	Jochen Tucht		Complain only once if Merge7z*.dll is missing
2004-01-25	Jochen Tucht		Fix bad default for OPENFILENAME::nFilterIndex
2004-03-15	Jochen Tucht		Fix Visual Studio 2003 build issue
2004-04-13	Jochen Tucht		Avoid StrNCat to get away with shlwapi 4.70
2004-08-25	Jochen Tucht		More explicit error message
2004-10-17	Jochen Tucht		Leave decision whether to recurse into folders
								to enumerator (Mask.Recurse)
2004-11-03	Jochen Tucht		FIX [1048997] as proposed by Kimmo 2004-11-02
2005-01-15	Jochen Tucht		Read 7-Zip version from 7zip_pad.xml
								Set Merge7z UI language if DllBuild_Merge7z >= 9
2005-01-22	Jochen Tucht		Better explain what's present/missing/outdated
2005-02-05	Jochen Tucht		Fall back to IDD_MERGE7ZMISMATCH template from
								.exe if .lang file isn't up to date.
2005-02-26	Jochen Tucht		Add download link to error message
2005-02-26	Jochen Tucht		Use WinAPI to obtain ISO language/region codes
2005-02-27	Jochen Tucht		FIX [1152375]
2005-04-24	Kimmo Varis			Don't use DiffContext exported from DirView
2005-06-08	Kimmo Varis			Use DIFFITEM, not reference to it (hopefully only
								temporarily, to sort out new directory compare)
2005-06-22	Jochen Tucht		Change recommended version of 7-Zip to 4.20
								Remove noise from Nagbox
2005-07-03	Jochen Tucht		DIFFITEM has changed due to RFE [ 1205516 ]
2005-07-04	Jochen Tucht		New global ArchiveGuessFormat() checks for
								formats to be handled by external command line
								tools. These take precedence over Merge7z
								internal handlers.
2005-07-05	Jochen Tucht		Move to Merge7z::Format::GetDefaultName() to
								build intermediate filenames for multi-step
								compression.
2005-07-15	Jochen Tucht		Remove external command line tool integration
								for now. Rethink about it after 2.4 branch.
2005-08-20	Jochen Tucht		Option to guess archive format by signature
								Map extensions through ExternalArchiveFormat.ini
2005-08-23	Jochen Tucht		Option to entirely disable 7-Zip integration
2007-01-04	Kimmo Varis			Convert using COptionsMgr for options.
2007-06-16	Jochen Neubeck		FIX [1723263] "Zip --> Both" operation...
2007-12-22	Jochen Neubeck		Fix Merge7z UI lang for new translation system
								Change recommended version of 7-Zip to 4.57
2010-05-16	Jochen Neubeck		Read 7-Zip version from 7z.dll (which has long
								ago replaced the various format and codec DLLs)
								Change recommended version of 7-Zip to 4.65
*/

// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "OptionsDef.h"
#include "Merge.h"
#include "SettingStore.h"
#include "OptionsMgr.h"
#include "LanguageSelect.h"
#include "resource.h"
#include "DirFrame.h"
#include "MainFrm.h"
#include "7zCommon.h"
//#include "ExternalArchiveFormat.h"
#include "common/version.h"
#include <paths.h>
#include "Environment.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Wrap Merge7z::GuessFormat() to allow for some customizing:
 * - Check if 7-Zip integration is enabled.
 * - Check for filename extension mappings.
 */
Merge7z::Format *ArchiveGuessFormat(LPCTSTR path)
{
	if (COptionsMgr::Get(OPT_ARCHIVE_ENABLE) == 0)
		return NULL;
	if (PathIsDirectory(path))
		return NULL;
	// Map extensions through ExternalArchiveFormat.ini
	static const TCHAR null[] = _T("");
	static const TCHAR section[] = _T("extensions");
	LPCTSTR entry = PathFindExtension(path);
	TCHAR value[20];
	static LPCTSTR filename = NULL;
	if (filename == NULL)
	{
		TCHAR cPath[INTERNET_MAX_PATH_LENGTH];
		DWORD cchPath = SearchPath(NULL, _T("ExternalArchiveFormat.ini"), NULL,
			INTERNET_MAX_PATH_LENGTH, cPath, NULL);
		filename = cchPath && cchPath < INTERNET_MAX_PATH_LENGTH ? StrDup(cPath) : null;
	}
	if (*filename &&
		GetPrivateProfileString(section, entry, null, value, 20, filename) &&
		*value == '.')
	{
		// Remove end-of-line comments (in string returned from GetPrivateProfileString)
		// that is, remove semicolon & whatever follows it
		if (LPTSTR p = StrChr(value, ';'))
		{
			*p = '\0';
			StrTrim(value, _T(" \t"));
		}
		path = value;
	}

	// PATCH [ 1229867 ] RFE [ 1205516 ], RFE [ 887948 ], and other issues
	// command line integration portion is not yet applied
	// so following code not yet valid, so temporarily commented out
	// Look for command line tool first
	/*Merge7z::Format *pFormat;
	if (CExternalArchiveFormat::GuessFormat(path, pFormat))
	{
		return pFormat;
	}*/
	// Default to Merge7z*.dll
	return Merge7z->GuessFormat(path);
}

/**
 * @brief Self-initializing raw C character buffer class.
 */
template<class TYPE, size_t SIZE> struct CRawString
{
	enum { Size = SIZE };
	TYPE Data[SIZE];
	CRawString()
	{
		Data[0] = 0;
	}
};

/**
 * @brief Recommended version of 7-Zip.
 */
const DWORD C7ZipMismatchException::m_dwVer7zRecommended = DWORD MAKELONG(20,9);

/**
 * @brief Registry key for C7ZipMismatchException's ReportError() popup.
 */
const TCHAR C7ZipMismatchException::m_strRegistryKey[] = _T("7ZipMismatch");

/**
 * @brief Download URL for C7ZipMismatchException's ReportError() popup.
 */
const TCHAR C7ZipMismatchException::m_strDownloadURL[] = _T("https://sourceforge.net/project/showfiles.php?group_id=13216&package_id=143957");

/**
 * @brief Retrieve build number of given DLL.
 */
static DWORD NTAPI GetDllBuild(LPCTSTR cPath)
{
	HMODULE hModule = LoadLibrary(cPath);
	DLLVERSIONINFO dvi;
	dvi.cbSize = sizeof dvi;
	dvi.dwBuildNumber = ~0UL;
	if (hModule)
	{
		dvi.dwBuildNumber = 0UL;
		DLLGETVERSIONPROC DllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hModule, "DllGetVersion");
		if (DllGetVersion)
		{
			DllGetVersion(&dvi);
		}
		FreeLibrary(hModule);
	}
	return dvi.dwBuildNumber;
}

/**
 * @brief Format Merge7z version number and plugin name, and retrieve DllBuild.
 */
DWORD C7ZipMismatchException::FormatVersion(LPTSTR pcVersion, LPTSTR pcPluginName, DWORD dwVersion)
{
	*pcVersion = '\0';
	*pcPluginName = '\0';
	if (dwVersion)
	{
		wsprintf
		(
			pcVersion, _T("%u.%02u"),
			UINT HIWORD(dwVersion),
			UINT LOWORD(dwVersion)
		);
		wsprintf
		(
			pcPluginName,
			sizeof(TCHAR) == 1 ? _T("Merge7z%u%02u.dll") : _T("Merge7z%u%02uU.dll"),
			UINT HIWORD(dwVersion),
			UINT LOWORD(dwVersion)
		);
	}
	return GetDllBuild(pcPluginName);
}

/**
 * @brief Populate ListBox with names/revisions of DLLs matching given pattern.
 */
static void NTAPI DlgDirListDLLs(HWND hWnd, LPTSTR cPattern, int nIDListBox)
{
	HDC hDC = GetDC(hWnd);
	HFONT hFont = (HFONT)SendDlgItemMessage(hWnd, nIDListBox, WM_GETFONT, 0, 0);
	int cxView = (int)SendDlgItemMessage(hWnd, nIDListBox, LB_GETHORIZONTALEXTENT, 0, 0) - 8;
	HGDIOBJ hObject = SelectObject(hDC, hFont);
	WIN32_FIND_DATA ff;
	HANDLE h = FindFirstFile(cPattern, &ff);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			PathRemoveFileSpec(cPattern);
			PathAppend(cPattern, ff.cFileName);
			wsprintf(ff.cFileName, _T(" (dllbuild %04u)"), GetDllBuild(cPattern));
			lstrcat(cPattern, ff.cFileName);
			int cxText = (int)(WORD)GetTabbedTextExtent(hDC, cPattern, lstrlen(cPattern), 0, 0);
			if (cxView < cxText)
				cxView = cxText;
			SendDlgItemMessage(hWnd, nIDListBox, LB_ADDSTRING, 0, (LPARAM)cPattern);
		} while (FindNextFile(h, &ff));
		FindClose(h);
	}
	SelectObject(hDC, hObject);
	ReleaseDC(hWnd, hDC);
	SendDlgItemMessage(hWnd, nIDListBox, LB_SETHORIZONTALEXTENT, cxView + 8, 0);
}

/**
 * @brief DLGPROC for C7ZipMismatchException's ReportError() popup.
 */
INT_PTR CALLBACK C7ZipMismatchException::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			LanguageSelect.TranslateDialog(hWnd);
			if (GetDlgItem(hWnd, 9001) == NULL)
			{
				// Dialog template isn't up to date. Give it a second chance.
				EndDialog(hWnd, -1);
				return FALSE;
			}
			C7ZipMismatchException *pThis = (C7ZipMismatchException *)lParam;
			CRawString<TCHAR,2600> cText;
			CRawString<TCHAR,80> cPresent, cMissing, cOutdated, cNone, cPlugin;
			if (!pThis->m_cause.empty())
			{
				SetDlgItemText(hWnd, 107, pThis->m_cause.c_str());
			}
			else
			{
				GetDlgItemText(hWnd, 107, cText.Data, cText.Size);
				switch (COptionsMgr::Get(OPT_ARCHIVE_ENABLE))
				{
				case 0:
					lstrcat(cText.Data, LanguageSelect.LoadString(IDS_MERGE7Z_ENABLE_0).c_str());
					break;
				case 2:
					lstrcat(cText.Data, LanguageSelect.LoadString(IDS_MERGE7Z_ENABLE_2).c_str());
					break;
				}
				SetDlgItemText(hWnd, 107, cText.Data);
			}
			GetDlgItemText(hWnd, 112, cPresent.Data, cPresent.Size);
			GetDlgItemText(hWnd, 122, cMissing.Data, cMissing.Size);
			GetDlgItemText(hWnd, 132, cOutdated.Data, cOutdated.Size);
			GetDlgItemText(hWnd, 120, cNone.Data, cNone.Size);
			GetDlgItemText(hWnd, 102, cPlugin.Data, cPlugin.Size);
			wsprintf(cText.Data, cPlugin.Data, DllBuild_Merge7z);
			SetDlgItemText(hWnd, 102, cText.Data);
			SetDlgItemText
			(
				hWnd, 109,
				(
					pThis->m_dwVer7zRecommended == pThis->m_dwVer7zInstalled
				||	pThis->m_dwVer7zRecommended == pThis->m_dwVer7zLocal
				) ? cPresent.Data : cMissing.Data
			);
			DWORD dwDllBuild = FormatVersion(cText.Data, cPlugin.Data, pThis->m_dwVer7zRecommended);
			SetDlgItemText(hWnd, 110, *cText.Data ? cText.Data : cNone.Data);
			SetDlgItemText(hWnd, 111, cPlugin.Data);
			SetDlgItemText(hWnd, 112, *cPlugin.Data == '\0' ? cPlugin.Data :
				dwDllBuild == ~0 ? cMissing.Data : dwDllBuild < DllBuild_Merge7z ? cOutdated.Data : cPresent.Data);
			dwDllBuild = FormatVersion(cText.Data, cPlugin.Data, pThis->m_dwVer7zInstalled);
			SetDlgItemText(hWnd, 120, *cText.Data ? cText.Data : cNone.Data);
			SetDlgItemText(hWnd, 121, cPlugin.Data);
			SetDlgItemText(hWnd, 122, *cPlugin.Data == '\0' ? cPlugin.Data :
				dwDllBuild == ~0 ? cMissing.Data : dwDllBuild < DllBuild_Merge7z ? cOutdated.Data : cPresent.Data);
			dwDllBuild = FormatVersion(cText.Data, cPlugin.Data, pThis->m_dwVer7zLocal);
			SetDlgItemText(hWnd, 130, *cText.Data ? cText.Data : cNone.Data);
			SetDlgItemText(hWnd, 131, cPlugin.Data);
			SetDlgItemText(hWnd, 132, *cPlugin.Data == '\0' ? cPlugin.Data :
				dwDllBuild == ~0 ? cMissing.Data : dwDllBuild < DllBuild_Merge7z ? cOutdated.Data : cPresent.Data);
			GetModuleFileName(0, cText.Data, MAX_PATH);
			PathRemoveFileSpec(cText.Data);
			PathAppend(cText.Data, _T("Merge7z*.dll"));
			DlgDirListDLLs(hWnd, cText.Data, 105);
			if (DWORD cchPath = GetEnvironmentVariable(_T("path"), 0, 0))
			{
				static const TCHAR cSep[] = _T(";");
				LPTSTR pchPath = new TCHAR[cchPath];
				GetEnvironmentVariable(_T("PATH"), pchPath, cchPath);
				LPTSTR pchItem = pchPath;
				while (int cchItem = StrCSpn(pchItem += StrSpn(pchItem, cSep), cSep))
				{
					if (cchItem < MAX_PATH)
					{
						CopyMemory(cText.Data, pchItem, cchItem*sizeof*pchItem);
						cText.Data[cchItem] = 0;
						PathAppend(cText.Data, _T("Merge7z*.dll"));
						DlgDirListDLLs(hWnd, cText.Data, 105);
					}
					pchItem += cchItem;
				}
				delete[] pchPath;
			}
			if (SendDlgItemMessage(hWnd, 105, LB_GETCOUNT, 0, 0) == 0)
			{
				SendDlgItemMessage(hWnd, 105, LB_ADDSTRING, 0, (LPARAM) cNone.Data);
			}
			HICON hIcon = LoadIcon(0, IDI_EXCLAMATION);
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
			if (pThis->m_bShowAllways)
			{
				ShowWindow(GetDlgItem(hWnd, 106), SW_HIDE);
			}
		} return TRUE;
		case WM_DRAWITEM:
		{
			switch (wParam)
			{
			case 108:
				OWindow(hWnd).MessageReflect_WebLinkButton<WM_DRAWITEM>(wParam, lParam);
				break;
			}
		} return TRUE;
		case WM_SETCURSOR:
		{
			switch (GetDlgCtrlID((HWND)wParam))
			{
			case 108:
				OWindow(hWnd).MessageReflect_WebLinkButton<WM_SETCURSOR>(wParam, lParam);
				SetWindowLongPtr(hWnd, DWLP_MSGRESULT, 1);
				return TRUE;
			}
		} return FALSE;
		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
				case IDCANCEL:
				{
					int nDontShowAgain = SendDlgItemMessage(hWnd, 106, BM_GETCHECK, 0, 0);
					EndDialog(hWnd, MAKEWORD(IDOK, nDontShowAgain));
				} break;
				case 108:
				{
					HINSTANCE h = ShellExecute(hWnd, _T("open"), m_strDownloadURL, 0, 0, SW_SHOWNORMAL);
					if ((UINT)h > 32)
						OWindow(hWnd).MessageReflect_WebLinkButton<WM_COMMAND>(wParam, lParam);
					else
						MessageBeep(0);
				} break;
			}
		} return TRUE;
	}
	return FALSE;
}

/**
 * @brief Tell user what went wrong and how she can help.
 */
int C7ZipMismatchException::ReportError(HWND hWnd, UINT nType, UINT nMessageID)
{
	UINT_PTR response = -1;
	m_bShowAllways = nMessageID;
	if (!m_bShowAllways)
	{
		// Suppress error message in case 7-Zip is not installed.
		response =
		(
			m_dwVer7zInstalled || m_dwVer7zLocal
		?	SettingStore.GetProfileInt(REGISTRY_SECTION_MESSAGEBOX, m_strRegistryKey, -1)
		:	IDOK
		);
	}
	if (response == -1)
	{
		HWND hwndOwner = ::GetLastActivePopup(hWnd);
		HINSTANCE hinst = LanguageSelect.FindResourceHandle(MAKEINTRESOURCE(IDD_MERGE7ZMISMATCH), RT_DIALOG);
		response = DialogBoxParam(hinst, MAKEINTRESOURCE(IDD_MERGE7ZMISMATCH), hwndOwner, DlgProc, (LPARAM)this);
		ASSERT(response != -1);
		if (HIBYTE(response) == 1)
		{
			SettingStore.WriteProfileInt(REGISTRY_SECTION_MESSAGEBOX, m_strRegistryKey, response = int LOBYTE(response));
		}
	}
	return response;
}

/**
 * @brief Check whether archive support is available.
 */
int NTAPI HasZipSupport()
{
	static int HasZipSupport = -1;
	if (HasZipSupport == -1)
	{
		try
		{
			Merge7z.operator->();
			HasZipSupport = 1;
		}
		catch (C7ZipMismatchException *e)
		{
			delete e;
			HasZipSupport = 0;
		}
	}
	return HasZipSupport;
}

/**
 * @brief Tell user why archive support is not available.
 */
void NTAPI Recall7ZipMismatchError(HWND hWnd)
{
	try
	{
		Merge7z.operator->();
	}
	catch (C7ZipMismatchException *e)
	{
		e->ReportError(hWnd, MB_ICONEXCLAMATION, TRUE);
		delete e;
	}
}

/**
 * @brief Construct path to a temporary folder for pOwner, and clear the folder.
 */
String NTAPI GetClearTempPath(LPVOID pOwner, LPCTSTR pchExt)
{
	string_format strPath
	(
		pOwner ? _T("%s\\%08lX.7z%s") : _T("%s"),
		env_GetTempPath(), pOwner, pchExt
	);
	// SHFileOperation expects a ZZ terminated list of paths!
	String::size_type len = strPath.size();
	strPath.resize(len + 1);
	SHFILEOPSTRUCT fileop =
	{
		0, FO_DELETE, &strPath.front(), 0,
		FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI,
		0, 0, 0
	};
	SHFileOperation(&fileop);
	strPath.resize(len);
	return strPath;
}

/**
 * @brief Delete head of temp path context list, and return its parent context.
 */
CTempPathContext *CTempPathContext::DeleteHead()
{
	CTempPathContext *pParent = m_pParent;
	GetClearTempPath(this, _T("*"));
	delete this;
	return pParent;
}

/**
 * @brief Return installed or local version of 7-Zip.
 */
DWORD NTAPI VersionOf7z(BOOL bLocal)
{
	TCHAR path[MAX_PATH];
	if (bLocal)
	{
		GetModuleFileName(0, path, sizeof path/sizeof*path);
		PathRemoveFileSpec(path);
	}
	else
	{
		static const TCHAR szSubKey[] = _T("Software\\7-Zip");
		static const TCHAR szValue[] = _T("Path");
		DWORD type = 0;
		DWORD size = sizeof path;
		SHGetValue(HKEY_LOCAL_MACHINE, szSubKey, szValue, &type, path, &size);
	}
	PathAppend(path, _T("7z.dll"));
	DWORD versionMS = 0;
	DWORD versionLS = 0;
	CVersionInfo(path).GetFixedFileVersion(versionMS, versionLS);
	return versionMS;
}

/**
 * @brief Access dll functions through proxy.
 */
interface Merge7z *Merge7z::Proxy::operator->()
{
	// As long as the Merge7z*.DLL has not yet been loaded, Merge7z
	// [0] points to the name of the DLL (with placeholders for 7-
	// Zip major and minor version numbers). Once the DLL has been
	// loaded successfully, Merge7z[0] is set to NULL, causing the
	// if to fail on subsequent calls.

	if (const char *format = Merge7z[0])
	{
		// Merge7z has not yet been loaded

		char name[MAX_PATH];
		DWORD flags = ~0;
		String cause;
		switch (COptionsMgr::Get(OPT_ARCHIVE_ENABLE))
		{
		case 1: //Use installed 7-Zip if present. Otherwise, use local 7-Zip.
			if (DWORD ver = VersionOf7z(FALSE))
			{
				flags = Initialize::Default;
				try
				{
					wsprintfA(name, format, UINT HIWORD(ver), UINT LOWORD(ver));
					Merge7z[0] = name;
					stub.Load();
					break;
				}
				catch (OException *e)
				{
					Merge7z[0] = format;
					cause = e->msg;
					delete e;
				}
			}
		case 2: //Always use local 7-Zip.
			if (DWORD ver = VersionOf7z(TRUE))
			{
				flags = Initialize::Default | Initialize::Local7z;
				try
				{
					wsprintfA(name, format, UINT HIWORD(ver), UINT LOWORD(ver));
					Merge7z[0] = name;
					stub.Load();
					break;
				}
				catch (OException *e)
				{
					Merge7z[0] = format;
					cause = e->msg;
					delete e;
				}
			}
		default:
			throw new C7ZipMismatchException
			(
				VersionOf7z(FALSE),
				VersionOf7z(TRUE),
				cause.c_str()
			);
		}
		LANGID wLangID = (LANGID)GetThreadLocale();
		flags |= wLangID << 16;
		if (COptionsMgr::Get(OPT_ARCHIVE_PROBETYPE))
		{
			flags |= Initialize::GuessFormatBySignature | Initialize::GuessFormatByExtension;
		}
		((interface Merge7z *)Merge7z[1])->Initialize(flags);
	}
	return ((interface Merge7z *)Merge7z[1]);
}

/**
 * @brief Proxy for Merge7z
 */
Merge7z::Proxy Merge7z =
{
	{ 0, 0, DllBuild_Merge7z },
	"Merge7z%u%02u"DECORATE_U".dll",
	"Merge7z"
};

/**
 * @brief Tell Merge7z we are going to enumerate just 1 item.
 */
UINT SingleItemEnumerator::Open()
{
	return 1;
}

/**
 * @brief Pass information about the item to Merge7z.
 */
Merge7z::Envelope *SingleItemEnumerator::Enum(Item &item)
{
	item.Mask.Item = item.Mask.Name|item.Mask.FullPath|item.Mask.Recurse;
	item.Name = Name;
	item.FullPath = FullPath;
	return 0;
}

/**
 * @brief SingleFileEnumerator constructor.
 */
SingleItemEnumerator::SingleItemEnumerator(LPCTSTR path, LPCTSTR FullPath, LPCTSTR Name)
: FullPath(FullPath)
, Name(Name)
{
}

/**
 * @brief Construct a CDirView::DirItemEnumerator.
 *
 * Argument *nFlags* controls operation as follows:
 * LVNI_ALL:		Enumerate all items.
 * LVNI_SELECTED:	Enumerate selected items only.
 * Original:		Set folder prefix for first iteration to "original"
 * Altered:			Set folder prefix for second iteration to "altered"
 * BalanceFolders:	Ensure that all nonempty folders on either side have a
 *					corresponding folder on the other side, even if it is
 *					empty (DirScan doesn't recurse into folders which
 *					appear only on one side).
 * DiffsOnly:		Enumerate diffs only.
 */
CDirView::DirItemEnumerator::DirItemEnumerator(CDirView *pView, int nFlags)
: m_pView(pView)
, m_nFlags(nFlags)
{
	if (m_nFlags & Original)
	{
		m_rgFolderPrefix.push_back(_T("original"));
	}
	if (m_nFlags & Altered)
	{
		m_rgFolderPrefix.push_back(_T("altered"));
	}
	if (m_nFlags & BalanceFolders)
	{
		// Collect implied folders
		for (UINT i = Open() ; i-- ; )
		{
			const DIFFITEM &di = Next();
			if ((m_nFlags & DiffsOnly) && !m_pView->IsItemNavigableDiff(di))
			{
				continue;
			}
			if (m_bRight) 
			{
				// Enumerating items on right side
				if (!di.diffcode.isSideLeftOnly())
				{
					// Item is present on right side, i.e. folder is implied
					m_rgImpliedFoldersRight[di.right.path] = PVOID(1);
				}
			}
			else
			{
				// Enumerating items on left side
				if (!di.diffcode.isSideRightOnly())
				{
					// Item is present on left side, i.e. folder is implied
					m_rgImpliedFoldersLeft[di.left.path] = PVOID(1);
				}
			}
		}
	}
}

/**
 * @brief Initialize enumerator, return number of items to be enumerated.
 */
UINT CDirView::DirItemEnumerator::Open()
{
	m_nIndex = -1;
	m_curFolderPrefix = m_rgFolderPrefix.begin();
	m_bRight = (m_nFlags & Right) != 0;
	size_t nrgFolderPrefix = m_rgFolderPrefix.size();
	if (nrgFolderPrefix)
	{
		m_strFolderPrefix = *m_curFolderPrefix++;
	}
	else
	{
		nrgFolderPrefix = 1;
	}
	return
	(
		m_nFlags & LVNI_SELECTED
	?	m_pView->GetSelectedCount()
	:	m_pView->GetItemCount()
	) * nrgFolderPrefix;
}

/**
 * @brief Return next item.
 */
const DIFFITEM &CDirView::DirItemEnumerator::Next()
{
	enum {nMask = LVNI_FOCUSED|LVNI_SELECTED|LVNI_CUT|LVNI_DROPHILITED};
	while ((m_nIndex = m_pView->GetNextItem(m_nIndex, m_nFlags & nMask)) == -1)
	{
		m_strFolderPrefix = *m_curFolderPrefix++;
		m_bRight = TRUE;
	}
	return m_pView->GetDiffItem(m_nIndex);
}

/**
 * @brief Pass information about an item to Merge7z.
 *
 * Information is passed through struct Merge7z::DirItemEnumerator::Item.
 * The *mask* member denotes which of the other members contain valid data.
 * If *mask* is zero upon return, which will be the case if Enum() decides to
 * leave the struct untouched, Merge7z will ignore the item.
 * If Enum() allocates temporary storage for string members, it must also
 * allocate an Envelope, providing a Free() method to free the temporary
 * storage, along with the Envelope itself. The Envelope pointer is passed to
 * Merge7z as the return value of the function. It is not meant to be a success
 * indicator, so if no temporary storage is required, it is perfectly alright
 * to return NULL.
 */
Merge7z::Envelope *CDirView::DirItemEnumerator::Enum(Item &item)
{
	CDirFrame *pDoc = m_pView->m_pFrame;
	const DIFFITEM &di = Next();

	if ((m_nFlags & DiffsOnly) && !m_pView->IsItemNavigableDiff(di))
	{
		return 0;
	}

	bool isSideLeft = di.diffcode.isSideLeftOnly();
	bool isSideRight = di.diffcode.isSideRightOnly();

	Envelope *envelope = new Envelope;

	const String &sFilename = m_bRight ? di.right.filename : di.left.filename;
	const String &sSubdir = m_bRight ? di.right.path : di.left.path;
	envelope->Name = sFilename;
	if (sSubdir.length())
	{
		envelope->Name.insert(0, _T("\\"));
		envelope->Name.insert(0, sSubdir);
	}
	envelope->FullPath = sFilename;
	envelope->FullPath.insert(0, _T("\\"));
	envelope->FullPath.insert(0, m_bRight ?
		di.GetRightFilepath(pDoc->GetRightBasePath()) :
		di.GetLeftFilepath(pDoc->GetLeftBasePath()));

	UINT32 Recurse = item.Mask.Recurse;

	if (m_nFlags & BalanceFolders)
	{
		if (m_bRight)
		{
			// Enumerating items on right side
			if (isSideLeft)
			{
				// Item is missing on right side
				PVOID &implied = m_rgImpliedFoldersRight[di.left.path.c_str()];
				if (!implied)
				{
					// Folder is not implied by some other file, and has
					// not been enumerated so far, so enumerate it now!
					envelope->Name = di.left.path;
					envelope->FullPath = di.GetLeftFilepath(pDoc->GetLeftBasePath());
					implied = PVOID(2); // Don't enumerate same folder twice!
					isSideLeft = false;
					Recurse = 0;
				}
			}
		}
		else
		{
			// Enumerating items on left side
			if (isSideRight)
			{
				// Item is missing on left side
				PVOID &implied = m_rgImpliedFoldersLeft[di.right.path.c_str()];
				if (!implied)
				{
					// Folder is not implied by some other file, and has
					// not been enumerated so far, so enumerate it now!
					envelope->Name = di.right.path;
					envelope->FullPath = di.GetRightFilepath(pDoc->GetRightBasePath());
					implied = PVOID(2); // Don't enumerate same folder twice!
					isSideRight = false;
					Recurse = 0;
				}
			}
		}
	}

	if (m_bRight ? isSideLeft : isSideRight)
	{
		return envelope;
	}

	if (m_strFolderPrefix.length())
	{
		if (envelope->Name.length())
			envelope->Name.insert(0, _T("\\"));
		envelope->Name.insert(0, m_strFolderPrefix);
	}

	item.Mask.Item = item.Mask.Name|item.Mask.FullPath|item.Mask.CheckIfPresent|Recurse;
	item.Name = envelope->Name.c_str();
	item.FullPath = envelope->FullPath.c_str();
	return envelope;
}

/**
 * @brief Apply appropriate handlers from left to right.
 */
bool CDirView::DirItemEnumerator::MultiStepCompressArchive(LPCTSTR path)
{
	DeleteFile(path);
	if (Merge7z::Format *piHandler = ArchiveGuessFormat(path))
	{
		HWND hwndOwner = GetMainFrame()->GetLastActivePopup()->m_hWnd;
		String pathIntermediate;
		SysFreeString(Assign(pathIntermediate, piHandler->GetDefaultName(hwndOwner, path)));
		String pathPrepend = path;
		pathPrepend.resize(pathPrepend.rfind('\\') + 1);
		pathIntermediate.insert(0, pathPrepend);
		bool bDone = MultiStepCompressArchive(pathIntermediate.c_str());
		if (bDone)
		{
			piHandler->CompressArchive(hwndOwner, path,
				&SingleItemEnumerator(path, pathIntermediate.c_str()));
			DeleteFile(pathIntermediate.c_str());
		}
		else
		{
			piHandler->CompressArchive(hwndOwner, path, this);
		}
		return true;
	}
	return false;
}

/**
 * @brief Generate archive from DirView items.
 */
void CDirView::DirItemEnumerator::CompressArchive(LPCTSTR path)
{
	if (path == NULL)
	{
		// No path given, so prompt for path!
		static const TCHAR _T_Merge7z[] = _T("Merge7z");
		static const TCHAR _T_FilterIndex[] = _T("FilterIndex");
		/*String strFilter; // = CExternalArchiveFormat::GetOpenFileFilterString();
		strFilter.insert(0, _T_Filter);
		strFilter += _T("|");*/
		OPENFILENAME ofn;
		ZeroMemory(&ofn, OPENFILENAME_SIZE_VERSION_400);
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
		ofn.hwndOwner = GetMainFrame()->GetLastActivePopup()->m_hWnd;
		ofn.lpstrFilter = CDirView::SuggestArchiveExtensions();
		ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOREADONLYRETURN;
		static TCHAR buffer[MAX_PATH];
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.nFilterIndex = SettingStore.GetProfileInt(_T_Merge7z, _T_FilterIndex, 1);
		// Use extension from current filter as default extension:
		if (int i = ofn.nFilterIndex)
		{
			ofn.lpstrDefExt = ofn.lpstrFilter;
			while (*ofn.lpstrDefExt && --i)
			{
				ofn.lpstrDefExt += lstrlen(ofn.lpstrDefExt) + 1;
				ofn.lpstrDefExt += lstrlen(ofn.lpstrDefExt) + 1;
			}
			if (*ofn.lpstrDefExt)
			{
				ofn.lpstrDefExt += lstrlen(ofn.lpstrDefExt) + 3;
			}
		}
		if (GetSaveFileName(&ofn))
		{
			path = ofn.lpstrFile;
			SettingStore.WriteProfileInt(_T_Merge7z, _T_FilterIndex, ofn.nFilterIndex);
		}
	}
	if (path && !MultiStepCompressArchive(path))
	{
		LanguageSelect.MsgBox(IDS_UNKNOWN_ARCHIVE_FORMAT, MB_ICONEXCLAMATION);
	}
}
