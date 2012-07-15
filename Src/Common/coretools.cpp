/**
 * @file  coretools.cpp
 *
 * @brief Common routines
 *
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "coretools.h"
#include "paths.h"

size_t linelen(const char *string)
{
	size_t stringlen = 0;
	while (char c = string[stringlen])
	{
		if (c == '\r' || c == '\n')
			break;
		++stringlen;
	}
	return stringlen;
}

/**
 * @brief Eat prefix and return pointer to remaining text
 */
LPCWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix)
{
	int len = lstrlenW(prefix);
	return StrIsIntlEqualW(FALSE, text, prefix, len) ? text + len : NULL;
}

/**
 * @brief Return true if *pszChar is a slash (either direction) or a colon
 *
 * begin points to start of string, in case multibyte trail test is needed
 */
bool IsSlashOrColon(LPCTSTR pszChar, LPCTSTR begin)
{
	return (*pszChar == '/' || *pszChar == ':' || *pszChar == '\\');
}

/**
 * @brief Extract path name components from given full path.
 * @param [in] pathLeft Original path.
 * @param [out] pPath Folder name component of full path, excluding
   trailing slash.
 * @param [out] pFile File name part, excluding extension.
 * @param [out] pExt Filename extension part, excluding leading dot.
 */
void SplitFilename(LPCTSTR pathLeft, String* pPath, String* pFile, String* pExt)
{
	LPCTSTR pszChar = pathLeft + _tcslen(pathLeft);
	LPCTSTR pend = pszChar;
	LPCTSTR extptr = 0;
	bool ext = false;

	while (pathLeft < --pszChar)
	{
		if (*pszChar == '.')
		{
			if (!ext)
			{
				if (pExt)
				{
					(*pExt) = pszChar + 1;
				}
				ext = true; // extension is only after last period
				extptr = pszChar;
			}
		}
		else if (IsSlashOrColon(pszChar, pathLeft))
		{
			// Ok, found last slash, so we collect any info desired
			// and we're done

			if (pPath)
			{
				// Grab directory (omit trailing slash)
				size_t len = pszChar - pathLeft;
				if (*pszChar == ':')
					++len; // Keep trailing colon ( eg, C:filename.txt)
				*pPath = pathLeft;
				pPath->erase(len); // Cut rest of path
			}

			if (pFile)
			{
				// Grab file
				*pFile = pszChar + 1;
			}

			goto endSplit;
		}
	}

	// Never found a delimiter
	if (pFile)
	{
		*pFile = pathLeft;
	}

endSplit:
	// if both filename & extension requested, remove extension from filename

	if (pFile && pExt && extptr)
	{
		size_t extlen = pend - extptr;
		pFile->erase(pFile->length() - extlen);
	}
}

// Split Rational ClearCase view name (file_name@@file_version).
void SplitViewName(LPCTSTR s, String * path, String * name, String * ext)
{
	String sViewName(s);
	size_t nOffset = sViewName.find(_T("@@"));
	if (nOffset != String::npos)
	{
		sViewName.erase(nOffset);
		SplitFilename(sViewName.c_str(), path, name, ext);
	}
}

#ifdef _DEBUG
// Test code for SplitFilename above
void TestSplitFilename()
{
	LPCTSTR tests[] = {
		_T("\\\\hi\\"), _T("\\\\hi"), 0, 0
		, _T("\\\\hi\\a.a"), _T("\\\\hi"), _T("a"), _T("a")
		, _T("a.hi"), 0, _T("a"), _T("hi")
		, _T("a.b.hi"), 0, _T("a.b"), _T("hi")
		, _T("c:"), _T("c:"), 0, 0
		, _T("c:\\"), _T("c:"), 0, 0
		, _T("c:\\d:"), _T("c:\\d:"), 0, 0
	};
	for (int i=0; i < _countof(tests); i += 4)
	{
		LPCTSTR dir = tests[i];
		String path, name, ext;
		SplitFilename(dir, &path, &name, &ext);
		LPCTSTR szpath = tests[i+1] ? tests[i+1] : _T("");
		LPCTSTR szname = tests[i+2] ? tests[i+2] : _T("");
		LPCTSTR szext = tests[i+3] ? tests[i+3] : _T("");
		assert(path == szpath);
		assert(name == szname);
		assert(ext == szext);
	}
}
#endif

HANDLE RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, BOOL bMinimized /*= TRUE*/, BOOL bNewConsole /*= FALSE*/)
{
    STARTUPINFO si = {0};
	PROCESS_INFORMATION procInfo = {0};

    si.cb = sizeof(STARTUPINFO);
    si.lpDesktop = _T("");
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (bMinimized) ? SW_MINIMIZE : SW_HIDE;

	TCHAR args[4096];
	_sntprintf(args, _countof(args), _T("\"%s\" %s"), szExeFile, szArgs);
    if (CreateProcess(szExeFile, args, NULL, NULL,
		FALSE, NORMAL_PRIORITY_CLASS|(bNewConsole? CREATE_NEW_CONSOLE:0),
                         NULL, _T(".\\"), &si, &procInfo))
	{
		CloseHandle(procInfo.hThread);
		return procInfo.hProcess;
	}

	return INVALID_HANDLE_VALUE;
}

/**
 * @brief Return module's path component (without filename).
 * @param [in] hModule Module's handle.
 * @return Module's path.
 */
String GetModulePath(HMODULE hModule /* = NULL*/)
{
	TCHAR temp[MAX_PATH] = {0};
	GetModuleFileName(hModule, temp, MAX_PATH);
	return paths_GetParentPath(temp);
}

/**
 * @brief Return path component from full path.
 * @param [in] fullpath Full path to split.
 * @return Path without filename.
 */
String GetPathOnly(LPCTSTR fullpath)
{
	if (!fullpath || !fullpath[0]) return _T("");
	String spath;
	SplitFilename(fullpath, &spath, 0, 0);
	return spath;
}

/**
 * @brief Decorates commandline for giving to CreateProcess() or
 * ShellExecute().
 *
 * Adds quotation marks around executable path if needed, but not
 * around commandline switches. For example (C:\p ath\ex.exe -p -o)
 * becomes ("C:\p ath\ex.exe" -p -o)
 * @param [in] sCmdLine commandline to decorate
 * @param [out] sDecoratedCmdLine decorated commandline
 * @param [out] sExecutable Executable for validating file extension etc
 */
void GetDecoratedCmdLine(String sCmdLine, String &sDecoratedCmdLine,
	String &sExecutable)
{
	BOOL pathEndFound = FALSE;
	BOOL addQuote = FALSE;
	size_t prevPos = 0;

	sDecoratedCmdLine.clear();
	sExecutable.clear();

	// Remove whitespaces from begin and and
	String::size_type clpos = sCmdLine.find_first_not_of(_T(" \n\t"));
	if (clpos != 0)
		sCmdLine.erase(0, clpos);
	clpos = sCmdLine.find_last_not_of(_T(" \n\t"));
	if (clpos != sCmdLine.length() - 1)
		sCmdLine.erase(clpos, clpos - sCmdLine.length());

	String::size_type pos = sCmdLine.find(_T(" "));
	if (pos != String::npos)
	{
		// First space was before switch, we don't need "s
		// (executable path didn't contain spaces)
		if (sCmdLine[pos + 1] == '/' || sCmdLine[pos + 1] == '-')
		{
			pathEndFound = TRUE;
		}
		else
		{
			addQuote = TRUE;
			sDecoratedCmdLine = _T("\"");
		}

		// Loop until executable path end (first switch) is found
		while (pathEndFound == FALSE)
		{
			prevPos = pos;
			pos = sCmdLine.find(_T(" "), prevPos + 1);

			if (pos != String::npos)
			{
				if (sCmdLine[pos + 1] == '/' || sCmdLine[pos + 1] == '-')
				{
					pathEndFound = TRUE;
				}
			}
			else
			{
				pathEndFound = TRUE;
			}
		}

		if (addQuote)
		{
			if (pos != String::npos)
			{
				sExecutable = sCmdLine.substr(0, pos);
				sDecoratedCmdLine += sExecutable;
				sDecoratedCmdLine += _T("\"");
				sDecoratedCmdLine += sCmdLine.substr(pos, sCmdLine.length() - pos);
			}
			else
			{
				sExecutable = sCmdLine;
				sDecoratedCmdLine += sCmdLine;
				sDecoratedCmdLine += _T("\"");
			}
		}
		else
		{
			sDecoratedCmdLine = sCmdLine;
			sExecutable = sCmdLine;
		}
	}
	else
	{
		sDecoratedCmdLine = sCmdLine;
		sExecutable = sCmdLine;
	}
}
