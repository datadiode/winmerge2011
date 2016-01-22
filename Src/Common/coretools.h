/**
 * @file  coretools.h
 *
 * @brief Declaration file for Coretools.cpp
 */
#pragma once

/******** function protos ********/

LPSTR NTAPI EatPrefix(LPCSTR text, LPCSTR prefix);
LPCWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix);
LPCWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix);

String GetModulePath(HMODULE hModule = NULL);
DWORD_PTR GetShellImageList();

int linelen(const char *string);
size_t unslash(unsigned codepage, char *string);

inline void unslash(unsigned codepage, std::string &s)
{
	s.resize(static_cast<std::string::size_type>(unslash(codepage, &s.front())));
}

HANDLE NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, HANDLE *phReadPipe, WORD wShowWindow = SW_SHOWMINNOACTIVE);
DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, WORD wShowWindow = SW_SHOWMINNOACTIVE);

void DecorateCmdLine(String &sDecoratedCmdLine, String &sExecutable);
