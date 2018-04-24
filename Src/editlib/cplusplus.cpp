///////////////////////////////////////////////////////////////////////////
//  File:       cplusplus.cpp
//  Version:    1.2.0.5
//  Created:    29-Dec-1998
//
//  Copyright:  Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalEditView class, a part of the Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  16-Aug-99
//      Ferdinand Prantl:
//  +   FEATURE: corrected bug in syntax highlighting C comments
//  +   FEATURE: extended levels 1- 4 of keywords in some languages
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextview.h"
#include "SyntaxColors.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// C++ keywords (MSVC5.0 + POET5.0)
static BOOL IsCppKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszCppKeywordList[] =
	{
		_T("__asm"),
		_T("__based"),
		_T("__cdecl"),
		_T("__declspec"),
		_T("__except"),
		_T("__export"),
		_T("__far16"),
		_T("__fastcall"),
		_T("__finally"),
		_T("__inline"),
		_T("__int16"),
		_T("__int32"),
		_T("__int64"),
		_T("__int8"),
		_T("__leave"),
		_T("__multiple_inheritance"),
		_T("__pascal"),
		_T("__single_inheritance"),
		_T("__stdcall"),
		_T("__syscall"),
		_T("__try"),
		_T("__uuidof"),
		_T("__virtual_inheritance"),
		_T("_asm"),
		_T("_cdecl"),
		_T("_export"),
		_T("_far16"),
		_T("_fastcall"),
		_T("_pascal"),
		_T("_persistent"),
		_T("_stdcall"),
		_T("_syscall"),
		_T("alignas"),
		_T("alignof"),
		_T("auto"),
		_T("bool"),
		_T("break"),
		_T("case"),
		_T("catch"),
		_T("char"),
		_T("char16_t"),
		_T("char32_t"),
		_T("class"),
		_T("const"),
		_T("const_cast"),
		_T("constexpr"),
		_T("continue"),
		_T("cset"),
		_T("decltype"),
		_T("default"),
		_T("delete"),
		_T("depend"),
		_T("dllexport"),
		_T("dllimport"),
		_T("do"),
		_T("double"),
		_T("dynamic_cast"),
		_T("else"),
		_T("enum"),
		_T("explicit"),
		_T("extern"),
		_T("false"),
		_T("float"),
		_T("for"),
		_T("friend"),
		_T("goto"),
		_T("if"),
		_T("indexdef"),
		_T("inline"),
		_T("int"),
		_T("interface"),
		_T("long"),
		_T("main"),
		_T("mutable"),
		_T("naked"),
		_T("namespace"),
		_T("new"),
		_T("noexcept"),
		_T("nullptr"),
		_T("ondemand"),
		_T("operator"),
		_T("persistent"),
		_T("private"),
		_T("protected"),
		_T("public"),
		_T("register"),
		_T("reinterpret_cast"),
		_T("return"),
		_T("short"),
		_T("signed"),
		_T("sizeof"),
		_T("static"),
		_T("static_assert"),
		_T("static_cast"),
		_T("struct"),
		_T("switch"),
		_T("template"),
		_T("this"),
		_T("thread"),
		_T("thread_local"),
		_T("throw"),
		_T("transient"),
		_T("true"),
		_T("try"),
		_T("typedef"),
		_T("typeid"),
		_T("typename"),
		_T("union"),
		_T("unsigned"),
		_T("useindex"),
		_T("using"),
		_T("uuid"),
		_T("virtual"),
		_T("void"),
		_T("volatile"),
		_T("while"),
		_T("wmain"),
		_T("xalloc"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszCppKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("BOOL"),
		_T("BSTR"),
		_T("BYTE"),
		_T("CHAR"),
		_T("COLORREF"),
		_T("DWORD"),
		_T("DWORD32"),
		_T("FALSE"),
		_T("HANDLE"),
		_T("INT"),
		_T("INT16"),
		_T("INT32"),
		_T("INT64"),
		_T("INT8"),
		_T("LONG"),
		_T("LPARAM"),
		_T("LPBOOL"),
		_T("LPBYTE"),
		_T("LPCBYTE"),
		_T("LPCSTR"),
		_T("LPCTSTR"),
		_T("LPCVOID"),
		_T("LPCWSTR"),
		_T("LPDWORD"),
		_T("LPINT"),
		_T("LPLONG"),
		_T("LPRECT"),
		_T("LPSTR"),
		_T("LPTSTR"),
		_T("LPVOID"),
		_T("LPWORD"),
		_T("LPWSTR"),
		_T("LRESULT"),
		_T("PBOOL"),
		_T("PBYTE"),
		_T("PDWORD"),
		_T("PDWORD32"),
		_T("PINT"),
		_T("PINT16"),
		_T("PINT32"),
		_T("PINT64"),
		_T("PINT8"),
		_T("POSITION"),
		_T("PUINT"),
		_T("PUINT16"),
		_T("PUINT32"),
		_T("PUINT64"),
		_T("PUINT8"),
		_T("PULONG32"),
		_T("PWORD"),
		_T("TCHAR"),
		_T("TRUE"),
		_T("UINT"),
		_T("UINT16"),
		_T("UINT32"),
		_T("UINT64"),
		_T("UINT8"),
		_T("ULONG32"),
		_T("VOID"),
		_T("WCHAR"),
		_T("WNDPROC"),
		_T("WORD"),
		_T("WPARAM"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser1KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineC(DWORD dwCookie, int nLineIndex, TextBlock::Array &pBuf)
{
	int const nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

	LPCTSTR const pszChars = GetLineChars(nLineIndex);
	BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
	int nIdentBegin = -1;
	int I = -1;
	do
	{
		int const nPrevI = I++;
		if (bRedefineBlock)
		{
			int const nPos = bDecIndex ? nPrevI : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
			}
			else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
			}
			else
			{
				DEFINE_BLOCK(nPos, COLORINDEX_OPERATOR);
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
			}
		}
		// Can be bigger than length if there is binary data
		// See bug #1474782 Crash when comparing SQL with with binary data
		if (I < nLength)
	    {
			if (dwCookie & COOKIE_COMMENT)
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  String constant "...."
			if (dwCookie & COOKIE_STRING)
			{
				if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Extended comment /*....*/
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '*' && bWasComment != Start)
				{
					dwCookie &= ~COOKIE_EXT_COMMENT;
					bRedefineBlock = TRUE;
					bWasComment = End;
				}
				else
				{
					bWasComment = False;
				}
				continue;
			}

			if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				continue;
			}
			if (pszChars[I] == '\'')
			{
				if (I == 0 || !xisxdigit(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					continue;
				}
			}
			if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_EXT_COMMENT;
				bWasComment = Start;
				continue;
			}

			bWasComment = False;

			//  Preprocessor directive #....
			if (dwCookie & COOKIE_PREPROCESSOR)
				continue;

			if (bFirstChar)
			{
				if (pszChars[I] == '#')
				{
					DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
					dwCookie |= COOKIE_PREPROCESSOR;
					continue;
				}
				if (!xisspace(pszChars[I]))
					bFirstChar = FALSE;
			}

			if (pBuf == NULL)
				continue; // No need to extract keywords, so skip rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
        if (nIdentBegin >= 0)
		{
			// If preceded by an apostrophe, assume that the identifier is a
			// continuation of a numeric literal using C++14 digit grouping.
			// Misdetections will occur on identifiers which immediately follow
			// character literals, but this seems more like an academic issue.
			if (nIdentBegin > 0 && pszChars[nIdentBegin - 1] == '\'' ||
				IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else if (IsCppKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsUser1Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
			}
			else
			{
				for (int j = I; j < nLength; j++)
				{
					if (!xisspace(pszChars[j]))
					{
						if (pszChars[j] == '(')
						{
							DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
						}
						break;
					}
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	if (pszChars[nLength - 1] != '\\')
		dwCookie &= COOKIE_EXT_COMMENT;
	return dwCookie;
}

TESTCASE
{
	int count = 0;
	BOOL (*pfnIsKeyword)(LPCTSTR, int) = NULL;
	FILE *file = fopen(__FILE__, "r");
	assert(file);
	TCHAR text[1024];
	while (_fgetts(text, _countof(text), file))
	{
		TCHAR c, *p, *q;
		if ((p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			VerifyKeyword<_tcsncmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsCppKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsCppKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
