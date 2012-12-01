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

LPCWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix);
LPCWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix);

String GetModulePath(HMODULE hModule = NULL);

size_t linelen(const char *string);

DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir);

void DecorateCmdLine(String &sDecoratedCmdLine, String &sExecutable);
#endif
