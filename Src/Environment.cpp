/** 
 * @file  Environment.cpp
 *
 * @brief Environment related routines.
 */
#include "StdAfx.h"
#include "paths.h"
#include "Environment.h"
#include "Merge.h"
#include "OptionsMgr.h"
#include "Constants.h"

/** @brief Temp folder name prefix for WinMerge temp folders. */
static const TCHAR TempFolderPrefix[] = _T("WinMerge_TEMP_%u");

/**
 * @brief Temp path.
 * Static string used by paths_GetTempPath() for storing temp path.
 */
static String strTempPath;

/** 
 * @brief Get folder for temporary files.
 * This function returns system temp folder.
 * @param [out] pnerr Error code if erorr happened.
 * @return Temp path, or empty string if error happened.
 * @note Temp path is cached after first call.
 * @todo Should we return NULL for error case?
 */
LPCTSTR env_GetTempPath()
{
	static LPCTSTR pszTempPath = NULL;
	if (pszTempPath == NULL)
	{
		if (!COptionsMgr::Get(OPT_USE_SYSTEM_TEMP_PATH))
		{
			strTempPath = COptionsMgr::Get(OPT_CUSTOM_TEMP_PATH);
		}
		else if (DWORD n = GetTempPath(0, 0))
		{
			strTempPath.resize(n - 1);
			GetTempPath(n, &strTempPath.front());
		}
		if (!strTempPath.empty())
		{
			strTempPath = paths_GetLongPath(strTempPath.c_str());
			string_format strTempName(TempFolderPrefix, GetCurrentProcessId());
			strTempPath = paths_ConcatPath(strTempPath, strTempName);
			if (HANDLE hMutex = CreateMutex(NULL, FALSE, strTempName.c_str()))
			{
				// Wait for mutex owner to finish its temp folder cleanups.
				// Keep mutex open to protect temp folder against deletion.
				WaitForSingleObject(hMutex, INFINITE);
			}
			paths_CreateIfNeeded(strTempPath.c_str());
		}
		// Existing code that involves temporary paths cannot handle the magic prefix so far.
		paths_UndoMagic(strTempPath);
		pszTempPath = strTempPath.c_str();
	}
	return pszTempPath;
}

/**
 * @brief Get filename for temporary file.
 * @param [in] lpPathName Temporary file folder.
 * @param [in] lpPrefixString Prefix to use for filename.
 * @return Full path for temporary file or empty string if error happened.
 */
String env_GetTempFileName(LPCTSTR lpPathName, LPCTSTR lpPrefixString)
{
	TCHAR path[MAX_PATH];
	return lpPathName[0] != _T('\0') &&
		GetTempFileName(lpPathName, lpPrefixString, 0, path) ? path : String();
}

/**
 * @brief Get Windows directory.
 * @return Windows directory.
 */
String env_GetWindowsDirectory()
{
	TCHAR path[MAX_PATH];
	path[0] = _T('\0');
	GetWindowsDirectory(path, MAX_PATH);
	return path;
}

/**
 * @brief Return User's My Documents Folder.
 * This function returns full path to User's My Documents -folder.
 * @return Full path to My Documents -folder.
 */
String env_GetMyDocuments()
{
	TCHAR path[MAX_PATH];
	path[0] = _T('\0');
	SHGetSpecialFolderPath(NULL, path, CSIDL_MYDOCUMENTS, FALSE);
	return path;
}

/**
 * @brief Expand environment variables.
 */
String env_ExpandVariables(LPCTSTR text)
{
	DWORD n = ExpandEnvironmentStrings(text, NULL, 0);
	String s;
	if (n > 1)
	{
		s.resize(n - 1);
		ExpandEnvironmentStrings(text, &s.front(), n);
	}
	return s;
}

LPCTSTR env_ResolveMoniker(String &moniker)
{
	C_ASSERT(String::npos == -1);
	// Turn any apostrophes inside the query string into quotation marks.
	if (String::size_type i = moniker.find(_T('?')) + 1)
	{
		std::replace(moniker.begin() + i, moniker.end(), _T('\''), _T('"'));
	}
	moniker = env_ExpandVariables(moniker.c_str());
	String::size_type i = moniker.find(_T(':'), 2) + 1;
	String::size_type j = i;
	while (j != 0)
	{
		if (moniker.c_str()[j] == _T('\\') &&
			moniker.c_str()[j + 1] != _T('\\'))
		{
			WCHAR path[MAX_PATH];
			GetModuleFileName(NULL, path, _countof(path));
			PathRemoveFileSpec(path);
			moniker.insert(j, path);
		}
		j = moniker.find(_T('"'), j) + 1;
	}
	return moniker.c_str() + i;
}
