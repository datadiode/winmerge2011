/**
 * @file  coretools.cpp
 *
 * @brief Common routines
 *
 */
#include "StdAfx.h"
#include "coretools.h"
#include "paths.h"

int linelen(const char *string)
{
	int stringlen = 0;
	while (char c = string[stringlen])
	{
		if (c == '\r' || c == '\n')
			break;
		++stringlen;
	}
	return stringlen;
}

/**
 * @brief Convert C style \\nnn, \\r, \\n, \\t etc into their indicated characters.
 * @param [in] codepage Codepage to use in conversion.
 * @param [in,out] s String to convert.
 */
size_t unslash(unsigned codepage, char *string)
{
	char *p = string;
	char *q = p;
	char c;
	do
	{
		char *r = q + 1;
		switch (c = *q)
		{
		case '\\':
			switch (c = *r++)
			{
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'x':
				*p = (char)strtol(r, &q, 16);
				break;
			default:
				*p = (char)strtol(r - 1, &q, 8);
				break;
			}
			if (q >= r)
				break;
			// fall through
		default:
			*p = c;
			if ((*p & 0x80) && IsDBCSLeadByteEx(codepage, *p))
				*++p = *r++;
			q = r;
		}
		++p;
	} while (c != '\0');
	//s.resize(static_cast<stl::string::size_type>(p - 1 - &s.front()));
	return p - 1 - string;
}

/**
 * @brief Remove prefix from the text.
 * @param [in] text Text to process.
 * @param [in] prefix Prefix to remove.
 * @return Text without the prefix.
 */
LPSTR NTAPI EatPrefix(LPCSTR text, LPCSTR prefix)
{
	size_t len = strlen(prefix);
	if (len)
		if (_memicmp(text, prefix, len) == 0)
			return const_cast<LPSTR>(text + len);
	return 0;
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
 * @brief Eat prefix and whitespace and return pointer to remaining text
 */
LPCWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix)
{
	text = EatPrefix(text, prefix);
	return text ? text + StrSpn(text, _T(" \t\r\n")) : NULL;
}

DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir)
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof si);
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_MINIMIZE;
	TCHAR args[4096];
	_sntprintf(args, _countof(args), _T("\"%s\" %s"), szExeFile, szArgs);
	DWORD code = STILL_ACTIVE;
	if (CreateProcess(szExeFile, args, NULL, NULL, FALSE,
			CREATE_NEW_CONSOLE, NULL, szDir, &si, &pi))
	{
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &code);
		CloseHandle(pi.hProcess);
	}
	return code;
}

/**
 * @brief Return module's path component (without filename).
 * @param [in] hModule Module's handle.
 * @return Module's path.
 */
String GetModulePath(HMODULE hModule /* = NULL*/)
{
	TCHAR temp[MAX_PATH];
	temp[0] = _T('\0');
	GetModuleFileName(hModule, temp, MAX_PATH);
	return paths_GetParentPath(temp);
}

/**
 * @brief Decorates commandline for giving to CreateProcess() or
 * ShellExecute().
 *
 * Adds quotation marks around executable path if needed, but not
 * around commandline switches. For example (C:\p ath\ex.exe -p -o)
 * becomes ("C:\p ath\ex.exe" -p -o)
 * @param [bi] sCmdLine commandline to decorate
 * @param [out] sExecutable Executable for validating file extension etc
 */
void DecorateCmdLine(String &sCmdLine, String &sExecutable)
{
	// Remove whitespaces from begin and and
	// (Earlier code was cutting off last non-ws character)
	string_trim_ws(sCmdLine);
	String::size_type pos = 0;
	do
	{
		pos = sCmdLine.find_first_of(_T("\"/-"), pos + 1);
	} while (pos != String::npos && sCmdLine[pos - 1] != _T(' '));
	if (pos != String::npos)
	{
		sExecutable = sCmdLine.substr(0U, pos);
		string_trim_ws(sExecutable);
		sCmdLine.insert(sExecutable.length(), 1U, _T('"'));
		sCmdLine.insert(0U, 1U, _T('"'));
	}
	else
	{
		sExecutable = sCmdLine;
	}
}
