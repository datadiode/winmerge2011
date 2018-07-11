///////////////////////////////////////////////////////////////////////////
//  File:       rust.cpp
//  Version:    1.0.0.0
//  Created:    23-Jul-2017
//
//  Copyright:  Stcherbatchenko Andrei, portions by Takashi Sawanaka
//  E-mail:     sdottaka@users.sourceforge.net
//
//  Rust syntax highlighing definition
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextbuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// Rust keywords
static BOOL IsRustKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszRustKeywordList[] =
	{
		_T("Self"),
		_T("abstract"),
		_T("alignof"),
		_T("as"),
		_T("become"),
		_T("box"),
		_T("break"),
		_T("const"),
		_T("continue"),
		_T("crate"),
		_T("do"),
		_T("else"),
		_T("enum"),
		_T("extern"),
		_T("false"),
		_T("final"),
		_T("fn"),
		_T("for"),
		_T("if"),
		_T("impl"),
		_T("in"),
		_T("let"),
		_T("loop"),
		_T("macro"),
		_T("match"),
		_T("mod"),
		_T("move"),
		_T("mut"),
		_T("offsetof"),
		_T("override"),
		_T("priv"),
		_T("proc"),
		_T("pub"),
		_T("pure"),
		_T("ref"),
		_T("return"),
		_T("self"),
		_T("sizeof"),
		_T("static"),
		_T("struct"),
		_T("super"),
		_T("trait"),
		_T("true"),
		_T("type"),
		_T("typeof"),
		_T("unsafe"),
		_T("unsized"),
		_T("use"),
		_T("virtual"),
		_T("where"),
		_T("while"),
		_T("yield"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszRustKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("String"),
		_T("binary32"),
		_T("binary64"),
		_T("bool"),
		_T("char"),
		_T("f32"),
		_T("f64"),
		_T("i16"),
		_T("i32"),
		_T("i64"),
		_T("i8"),
		_T("isize"),
		_T("str"),
		_T("u16"),
		_T("u32"),
		_T("u64"),
		_T("u8"),
		_T("usize"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser1KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_STRING           0x0008
#define COOKIE_RAWSTRING        0x00F0
#define COOKIE_EXT_COMMENT      0x0F00

#define COOKIE_GET_RAWSTRING_NUMBER_COUNT(cookie) (((cookie) & 0xF0) >> 4)
#define COOKIE_SET_RAWSTRING_NUMBER_COUNT(cookie, count) (cookie) = (((cookie) & ~0xF0) | ((count) << 4))

void CCrystalTextBuffer::ParseLineRust(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_RAWSTRING | COOKIE_STRING);
		return;
	}

	LPCTSTR pszRawStringBegin = NULL;
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
	int nIdentBegin = -1;
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
			else if (dwCookie & (COOKIE_STRING | COOKIE_RAWSTRING))
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
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
					int nPrevI = I;
					while (nPrevI && pszChars[--nPrevI] == '\\')
					{
						dwCookie ^= COOKIE_STRING;
						bRedefineBlock ^= TRUE;
					}
				}
				continue;
			}

			//  Raw string constant r"...." ,  r#"..."# , r##"..."##
			if (dwCookie & COOKIE_RAWSTRING)
			{
				const int nNumberCount = COOKIE_GET_RAWSTRING_NUMBER_COUNT(dwCookie);
				if (I >= nNumberCount && pszChars[I - nNumberCount] == '"')
				{
					if ((pszRawStringBegin < pszChars + I - nNumberCount) &&
						(std::count(pszChars + I - nNumberCount + 1, pszChars + I + 1, '#') == nNumberCount))
					{
						dwCookie &= ~COOKIE_RAWSTRING;
						bRedefineBlock = TRUE;
						pszRawStringBegin = NULL;
					}
				}
				continue;
			}

			if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie - COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
				bWasComment = Start;
				continue;
			}

			//  Extended comment /*....*/
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '*' && bWasComment != Start)
				{
					dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie + COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
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
			bWasComment = False;

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				continue;
			}

			//  Raw string
			if ((pszChars[I] == 'r' && I + 1 < nLength && (pszChars[I + 1] == '"' || pszChars[I + 1] == '#')) ||
				(pszChars[I] == 'b' && I + 2 < nLength && pszChars[I + 1] == 'r' && (pszChars[I + 2] == '"' || pszChars[I + 2] == '#')))
			{
				const int nprefix = (pszChars[I] == 'r' ? 1 : 2);
				const TCHAR *p = pszChars + I + nprefix;
				while (p < pszChars + nLength && *p == '#') ++p;
				if (p != pszChars + nLength && *p == '"')
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_RAWSTRING;
					COOKIE_SET_RAWSTRING_NUMBER_COUNT(dwCookie, static_cast<int>(p - (pszChars + I + nprefix)));
					pszRawStringBegin = p + 1;
					continue;
				}
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
			if (IsRustKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsUser1Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else
			{
				for (int j = I; j < nLength; j++)
				{
					if (!xisspace(pszChars[j]) && pszChars[j] != '!')
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

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_RAWSTRING | COOKIE_STRING);
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
		if (pfnIsKeyword && (p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			VerifyKeyword<_tcsncmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsRustKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsRustKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
