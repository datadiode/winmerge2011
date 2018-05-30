///////////////////////////////////////////////////////////////////////////
// lua.cpp - Lua syntax highlighting definition
// Modified from prior art code. Original header comment reproduced below.
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
#include "ccrystaltextview.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// Lua keywords
static BOOL IsLuaKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszLuaKeywordList[] =
	{
		_T("and"),
		_T("break"),
		_T("do"),
		_T("else"),
		_T("elseif"),
		_T("end"),
		_T("false"),
		_T("for"),
		_T("function"),
		_T("goto"),
		_T("if"),
		_T("in"),
		_T("local"),
		_T("nil"),
		_T("not"),
		_T("or"),
		_T("repeat"),
		_T("return"),
		_T("then"),
		_T("true"),
		_T("until"),
		_T("while"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszLuaKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_COMMENT_BALANCED 0x0002
#define COOKIE_STRING           0x000C
#define COOKIE_STRING_SINGLE    0x0004
#define COOKIE_STRING_DOUBLE    0x0008
#define COOKIE_STRING_BALANCED  0x000C
#define COOKIE_BALANCE          0xFF00

#define ENCODE_BALANCE(number) ((number) << 8)
#define DECODE_BALANCE(cookie) (((cookie) & COOKIE_BALANCE) >> 8)

void CCrystalTextView::ParseLineLua(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= (COOKIE_COMMENT_BALANCED | COOKIE_STRING_BALANCED | COOKIE_BALANCE);
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
			if (dwCookie & (COOKIE_COMMENT | COOKIE_COMMENT_BALANCED))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & (COOKIE_STRING))
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

			switch (dwCookie & COOKIE_STRING)
			{
			case COOKIE_STRING_SINGLE:
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_STRING_SINGLE;
					bRedefineBlock = TRUE;
					int nPrevI = I;
					while (nPrevI && pszChars[--nPrevI] == '\\')
					{
						dwCookie ^= COOKIE_STRING_SINGLE;
						bRedefineBlock ^= TRUE;
					}
				}
				continue;
			case COOKIE_STRING_DOUBLE:
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING_DOUBLE;
					bRedefineBlock = TRUE;
					int nPrevI = I;
					while (nPrevI && pszChars[--nPrevI] == '\\')
					{
						dwCookie ^= COOKIE_STRING_DOUBLE;
						bRedefineBlock ^= TRUE;
					}
				}
				continue;
			case COOKIE_STRING_BALANCED:
				// Tagged string literal [[....]], [=[....]=], [==[....]==]
				if (pszChars[I] == ']')
				{
					const int nNumberCount = DECODE_BALANCE(dwCookie);
					if (I > nNumberCount && pszChars[I - nNumberCount - 1] == ']')
					{
						if (std::count(pszChars + I - nNumberCount, pszChars + I, '=') == nNumberCount)
						{
							dwCookie &= ~(COOKIE_STRING_BALANCED | COOKIE_BALANCE);
							bRedefineBlock = TRUE;
						}
					}
				}
				continue;
			}

			// Extended comment --[[....]], --[=[....]=], --[==[....]==]
			if (dwCookie & COOKIE_COMMENT_BALANCED)
			{
				if (pszChars[I] == ']')
				{
					const int nNumberCount = DECODE_BALANCE(dwCookie);
					if (I > nNumberCount && pszChars[I - nNumberCount - 1] == ']')
					{
						if (std::count(pszChars + I - nNumberCount, pszChars + I, '=') == nNumberCount)
						{
							dwCookie &= ~(COOKIE_COMMENT_BALANCED | COOKIE_BALANCE);
							bRedefineBlock = TRUE;
						}
					}
				}
				continue;
			}

			if (I > 0 && pszChars[I] == '-' && pszChars[nPrevI] == '-')
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				if (pszChars[I + 1] == '[' && (pszChars[I + 2] == '[' || pszChars[I + 2] == '='))
				{
					const TCHAR *p = pszChars + I + 2;
					while (p < pszChars + nLength && *p == '=') ++p;
					if (p != pszChars + nLength && *p == '[')
					{
						DEFINE_BLOCK(I, COLORINDEX_STRING);
						dwCookie |= COOKIE_COMMENT_BALANCED;
						dwCookie |= ENCODE_BALANCE(static_cast<int>(p - (pszChars + I + 2)));
						continue;
					}
				}
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING_DOUBLE;
				continue;
			}
			if (pszChars[I] == '\'' && (I == 0 || !xisxdigit(pszChars[nPrevI])))
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING_SINGLE;
				continue;
			}
			if (pszChars[I] == '[' && (pszChars[I + 1] == '[' || pszChars[I + 1] == '='))
			{
				const TCHAR *p = pszChars + I + 1;
				while (p < pszChars + nLength && *p == '=') ++p;
				if (p != pszChars + nLength && *p == '[')
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_STRING_BALANCED;
					dwCookie |= ENCODE_BALANCE(static_cast<int>(p - (pszChars + I + 1)));
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
			if (IsLuaKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
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

	dwCookie &= COOKIE_COMMENT_BALANCED | COOKIE_STRING_BALANCED | COOKIE_BALANCE;
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
		else if (_stscanf(text, _T(" static BOOL IsLuaKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsLuaKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
