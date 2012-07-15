/** 
 * @file  paths.cpp
 *
 * @brief Path handling routines
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "paths.h"
#include "Common/coretools.h"

static const TCHAR paths_magic_prefix[] = _T("\\\\?\\");
static const TCHAR paths_magic_uncfix[] = _T("?\\UNC\\");

/** 
 * @brief Checks if string ends with slash.
 * This function checks if given string ends with slash. In many places,
 * especially in GUI, we assume folder paths end with slash.
 * @param [in] s String to check.
 * @return true if last char in string is slash.
 */
bool paths_EndsWithSlash(LPCTSTR s)
{
	s = PathFindFileName(s);
	return !PathIsFileSpec(s);
}

/** 
 * @brief Checks if path exists and if it points to folder or file.
 * This function first checks if path exists. If path exists
 * then function checks if path points to folder or file.
 * @param [in] szPath Path to check.
 * @return One of:
 * - DOES_NOT_EXIST : path does not exists
 * - IS_EXISTING_DIR : path points to existing folder
 * - IS_EXISTING_FILE : path points to existing file
 */
PATH_EXISTENCE paths_DoesPathExist(LPCTSTR szPath)
{
	if (!szPath || !szPath[0])
		return DOES_NOT_EXIST;
	DWORD attr = GetFileAttributes(szPath);
	if (attr == INVALID_FILE_ATTRIBUTES)
		return DOES_NOT_EXIST;
	else if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return IS_EXISTING_DIR;
	else
		return IS_EXISTING_FILE;
}

/** 
 * @brief Eat the magic pefix.
 * This is intended to hide the magic pefix from users. Change to return NULL
 * if you want the magic pefix to show up in the UI for debugging purposes.
 */
/*LPCTSTR paths_EatMagicPrefix(LPCTSTR path)
{
	return EatPrefix(path, paths_magic_prefix);
}*/

static const String &paths_DoMagic(String &path)
{
	// As for now no magic prefix on UNC paths!
	if (PathIsUNC(path.c_str()))
		path.insert(2, paths_magic_uncfix);
	else
		path.insert(0, paths_magic_prefix);
	return path;
}

LPCTSTR paths_UndoMagic(LPTSTR path)
{
	size_t i = 0;
	if (LPCTSTR p = EatPrefix(path, paths_magic_prefix))
	{
		if (LPCTSTR q = EatPrefix(p, paths_magic_uncfix + 2))
		{
			i = q - 2 - path;
			path[i] = _T('\\');
		}
		else
		{
			i = p - path;
		}
	}
	return path + i;
}

/**
 * Convert path to canonical long path.
 * This function converts given path to canonical long form. For example
 * foldenames with ~ short names are expanded.
 * If path does not exist, make canonical
 * the part that does exist, and leave the rest as is. Result, if a directory,
 * usually does not have a trailing backslash.
 * @param [in] sPath Path to convert.
 * @return Converted path.
 */
String paths_GetLongPath(LPCTSTR szPath)
{
	if (*szPath == _T('\0') || EatPrefix(szPath, paths_magic_prefix))
		return szPath;

	//                                         GetFullPathName  GetLongPathName
	// Convert to fully qualified form              Yes               No
	//    (Including .)
	// Convert /, //, \/, ... to \                  Yes               No
	// Handle ., .., ..\..\..                       Yes               No
	// Convert 8.3 names to long names              No                Yes
	// Fail when file/directory does not exist      No                Yes
	//
	// Fully qualify/normalize name using GetFullPathName.

	String sFull;
	if (DWORD cch = GetFullPathName(szPath, 0, NULL, NULL))
	{
		sFull.resize(cch - 1);
		GetFullPathName(szPath, cch, &sFull.front(), NULL);
	}
	else
	{
		sFull = szPath;
	}
	TCHAR *fullPath = &sFull.front();

	// We are done if this is not a short name.
	if (_tcschr(fullPath, _T('~')) == NULL)
		return paths_DoMagic(sFull);

	// We have to do it the hard way because GetLongPathName is not
	// available on Win9x and some WinNT 4

	// The file/directory does not exist, use as much long name as we can
	// and leave the invalid stuff at the end.
	String sLong;
	TCHAR *ptr = PathSkipRoot(fullPath);

	// Skip to \ position     d:\abcd or \\host\share\abcd
	// indicated by ^           ^                    ^

	if (!ptr)
		return paths_DoMagic(sFull);

	ptr[-1] = _T('\0');
	sLong += fullPath;

	// now walk down each directory and do short to long name conversion
	while (ptr)
	{
		TCHAR *end = _tcschr(ptr, _T('\\'));
		// zero-terminate current component
		// (if we're at end, its already zero-terminated)
		if (end)
			*end = 0;

		String sTemp(sLong);
		sTemp += _T('\\');
		sTemp += ptr;

		// advance to next component (or set ptr==0 to flag end)
		ptr = (end ? end+1 : 0);

		// (Couldn't get info for just the directory from CFindFile)
		WIN32_FIND_DATA ffd;
		HANDLE h = FindFirstFile(sTemp.c_str(), &ffd);
		if (h == INVALID_HANDLE_VALUE)
		{
			sLong = sTemp;
			if (ptr)
			{
				sLong += _T('\\');
				sLong += ptr;
			}
			return paths_DoMagic(sLong);
		}
		sLong += _T('\\');
		sLong += ffd.cFileName;
		FindClose(h);
	}
	return paths_DoMagic(sLong);
}

/**
 * @brief Check if the path exist and create the folder if needed.
 * This function checks if path exists. If path does not yet exist
 * function creates needed folder structure.
 * @param [in] sPath Path to check/create.
 * @return true if path exists or if we successfully created it.
 */
bool paths_CreateIfNeeded(LPCTSTR szPath)
{
	if (*szPath == '\0')
		return false;

	if (PathFileExists(szPath))
		return true;

	String sFull = szPath;

	String sLong;
	TCHAR *ptr = PathSkipRoot(&sFull.front());

	// Skip to \ position     d:\abcd or \\host\share\abcd
	// indicated by ^           ^                    ^

	if (!ptr)
		return false;

	while (ptr)
	{
		TCHAR *end = _tcschr(ptr, '\\');
		// zero-terminate current component
		// (if we're at end, its already zero-terminated)
		if (end)
			*end = 0;

		// advance to next component (or set ptr==0 to flag end)
		ptr = (end ? end+1 : 0);

		if (!PathFileExists(sFull.c_str()))
		{
			// try to create directory, and then double-check its existence
			if (!CreateDirectory(sFull.c_str(), 0) ||
				!PathFileExists(sFull.c_str()))
			{
				return false;
			}
		}
		// if not finished, restore directory string we're working in
		if (ptr)
			*end = '\\';
	}
	return true;
}

/** 
 * @brief Check if paths are both folders or files.
 * This function checks if paths are "compatible" as in many places we need
 * to have two folders or two files.
 * @param [in] pszLeft Left path.
 * @param [in] pszRight Right path.
 * @return One of:
 *  - IS_EXISTING_DIR : both are directories & exist
 *  - IS_EXISTING_FILE : both are files & exist
 *  - DOES_NOT_EXIST : in all other cases
*/
PATH_EXISTENCE GetPairComparability(LPCTSTR pszLeft, LPCTSTR pszRight)
{
	// fail if not both specified
	if (!pszLeft || !pszLeft[0] || !pszRight || !pszRight[0])
		return DOES_NOT_EXIST;
	PATH_EXISTENCE p1 = paths_DoesPathExist(pszLeft);
	// short circuit testing right if left doesn't exist
	if (p1 == DOES_NOT_EXIST)
		return DOES_NOT_EXIST;
	PATH_EXISTENCE p2 = paths_DoesPathExist(pszRight);
	if (p1 != p2)
		return DOES_NOT_EXIST;
	return p1;
}

/**
 * @brief Check if the given path points to shotcut.
 * Windows considers paths having a filename with extension ".lnk" as
 * shortcuts. This function checks if the given path is shortcut.
 * We usually want to expand shortcuts with ExpandShortcut().
 * @param [in] inPath Path to check;
 * @return TRUE if the path points to shortcut, FALSE otherwise.
 */
BOOL paths_IsShortcut(LPCTSTR inPath)
{
	return PathMatchSpec(inPath, _T("*.lnk"));
}

//////////////////////////////////////////////////////////////////
//	use IShellLink to expand the shortcut
//	returns the expanded file, or "" on error
//
//	original code was part of CShortcut 
//	1996 by Rob Warner
//	rhwarner@southeast.net
//	http://users.southeast.net/~rhwarner

/** 
 * @brief Expand given shortcut to full path.
 * @param [in] inFile Shortcut to expand.
 * @return Full path or empty string if error happened.
 */
String ExpandShortcut(LPCTSTR inFile)
{
	// No path, nothing to return
	if (inFile[0] == _T('\0'))
		return String();
	TCHAR outFile[MAX_PATH];
	IShellLink *psl = NULL;
	// Create instance for shell link
	HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		IID_IShellLink, (LPVOID*) &psl);
	if (SUCCEEDED(hr))
	{
		// Get a pointer to the persist file interface
		IPersistFile *ppf = NULL;
		hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*) &ppf);
		if (SUCCEEDED(hr))
		{
			// Load shortcut
			hr = ppf->Load(inFile, STGM_READ);
			if (SUCCEEDED(hr))
			{
				// Find the path from that
				hr = psl->GetPath(outFile, MAX_PATH, NULL, SLGP_UNCPRIORITY);
			}
			ppf->Release();
		}
		psl->Release();
	}
	return SUCCEEDED(hr) ? outFile : String();
}

/** 
 * @brief Append subpath to path.
 * This function appends subpath to given path. Function ensures there
 * is only one backslash between path parts.
 * @param [in] path "Base" path where other part is appended.
 * @param [in] subpath Path part to append to base part.
 * @return Formatted path. If one of arguments is empty then returns
 * non-empty argument. If both argumets are empty empty string is returned.
 */
String paths_ConcatPath(const String & path, const String & subpath)
{
	if (path.empty())
		return subpath;
	if (subpath.empty())
		return path;
	if (paths_EndsWithSlash(path.c_str()))
		return path + &subpath.c_str()[subpath.c_str()[0] == _T('\\')];
	else
		return path + &_T("\\")[subpath.c_str()[0] == _T('\\')] + subpath;
}

/** 
 * @brief Get parent path.
 * This function returns parent path for given path. For example for
 * path "c:\folder\subfolder" we return "c:\folder".
 * @param [in] path Path to get parent path for.
 * @return Parent path.
 */
String paths_GetParentPath(LPCTSTR path)
{
	size_t cb = (_tcslen(path) + 1) * sizeof(TCHAR);
	TCHAR *parentPath = reinterpret_cast<TCHAR *>(memcpy(_alloca(cb), path, cb));
	PathRemoveBackslash(parentPath);
	PathRemoveFileSpec(parentPath);
	return parentPath;
}

/**
 * @brief Checks if folder exists and creates it if needed.
 * This function checks if folder exists and creates it if not.
 * @param [in] sPath
 * @return Path if it exists or were created successfully. If
 * path points to file or folder failed to create returns empty
 * string.
 */
bool paths_EnsurePathExist(LPCTSTR path)
{
	return paths_CreateIfNeeded(path) && paths_DoesPathExist(path) == IS_EXISTING_DIR;
}

/**
 * @brief paths_PathIsExe
 */
bool paths_PathIsExe(LPCTSTR path)
{
	LPCTSTR ext = PathFindExtension(path);
	if (int len = lstrlen(ext))
	{
		ext = StrStrI(_T(".exe.com.pif.cmd.bat.scf.scr"), ext);
		return ext && ext[len] <= '.';
	}
	return false;
}
