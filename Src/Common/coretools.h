/**
 * @file  coretools.h
 *
 * @brief Declaration file for Coretools.cpp
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef CORETOOLS_H
#define CORETOOLS_H

/******** function protos ********/

LPSTR NTAPI EatPrefix(LPCSTR text, LPCSTR prefix);
LPCWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix);
LPCWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix);

String GetModulePath(HMODULE hModule = NULL);

int linelen(const char *string);
size_t unslash(unsigned codepage, char *string);

inline void unslash(unsigned codepage, stl::string &s)
{
	s.resize(static_cast<stl::string::size_type>(unslash(codepage, &s.front())));
}

DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir);

void DecorateCmdLine(String &sDecoratedCmdLine, String &sExecutable);
#endif
