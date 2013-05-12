/** 
 * @file  Environment.cpp
 *
 * @brief Environment related routines.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "paths.h"
#include "Environment.h"
#include "Merge.h"
#include "OptionsMgr.h"

/**
 * @brief Temp path.
 * Static string used by paths_GetTempPath() for storing temp path.
 */
static String strTempPath;

/** @brief Per-instance part of the temp path. */
static const string_format strTempPathInstance(_T("WM_%lu"), GetCurrentProcessId());

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
			strTempPath = paths_ConcatPath(strTempPath, strTempPathInstance);
			strTempPath = paths_GetLongPath(strTempPath.c_str());
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
	String::size_type i = moniker.find(_T(':'), 2) + 1;
	if (moniker.c_str()[i] != _T('\\'))
	{
		moniker = env_ExpandVariables(moniker.c_str());
	}
	else if (moniker.c_str()[i + 1] != _T('\\'))
	{
		WCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, _countof(path));
		PathRemoveFileSpec(path);
		moniker.insert(i, path);
	}
	return moniker.c_str() + i;
}
