///////////////////////////////////////////////////////////////////////////
//  File:    php.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  PHP syntax highlighing definition
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextview.h"
#include "SyntaxColors.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

static BOOL IsPhpKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszPhpKeywordList[] =
	{
		_T("array"),
		_T("as"),
		_T("break"),
		_T("case"),
		_T("cfunction"),
		_T("class"),
		_T("const"),
		_T("continue"),
		_T("declare"),
		_T("default"),
		_T("die"),
		_T("do"),
		_T("echo"),
		_T("else"),
		_T("elseif"),
		_T("empty"),
		_T("enddeclare"),
		_T("endfor"),
		_T("endforeach"),
		_T("endif"),
		_T("endswitch"),
		_T("endwhile"),
		_T("eval"),
		_T("exception"),
		_T("exit"),
		_T("extends"),
		_T("for"),
		_T("foreach"),
		_T("function"),
		_T("global"),
		_T("if"),
		_T("include"),
		_T("include_once"),
		_T("isset"),
		_T("list"),
		_T("new"),
		_T("old_function"),
		_T("php_user_filter"),
		_T("print"),
		_T("require"),
		_T("require_once"),
		_T("return"),
		_T("static"),
		_T("switch"),
		_T("unset"),
		_T("use"),
		_T("var"),
		_T("while"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPhpKeywordList);
}

static BOOL IsPhp1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszPhp1KeywordList[] =
	{
		_T("AND"),
		_T("OR"),
		_T("XOR"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPhp1KeywordList);
}

static BOOL IsPhp2Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszPhp2KeywordList[] =
	{
		_T("__CLASS__"),
		_T("__FILE__"),
		_T("__FUNCTION__"),
		_T("__LINE__"),
		_T("__METHOD__"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPhp2KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_CHAR             0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_EXT_COMMENT      0x0010
#define COOKIE_USER2            0x0020

DWORD CCrystalTextView::ParseLinePhp(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_CHAR | COOKIE_STRING);

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
	int nIdentBegin = -1;
	int nPrevI;
	goto start;
	do
	{
		//  Preprocessor start: $
		if (pszChars[I] == '$')
		{
			dwCookie |= COOKIE_USER2;
			nIdentBegin = -1;
			goto start;
		}

		//  Preprocessor end: ...
		if (dwCookie & COOKIE_USER2)
		{
			if (!xisalnum(pszChars[I]))
			{
				dwCookie &= ~COOKIE_USER2;
				nIdentBegin = -1;
				goto start;
			}
		}

	start: // First iteration starts here!
		nPrevI = I++;
		if (bRedefineBlock)
		{
			int const nPos = bDecIndex ? nPrevI : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.')
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
				goto start;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			//  Extended comment <!--....-->
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
				goto start;
			}

			if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			if (pszChars[I] == '#')
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				goto start;
			}

			if (pszChars[I] == '\'')
			{
				if (I == 0 || !xisxdigit(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					goto start;
				}
			}

			if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/')
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_EXT_COMMENT;
				bWasComment = Start;
				goto start;
			}
			bWasComment = False;

			if (pBuf == NULL)
				continue; // No need to extract keywords, so skip rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				goto start;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (dwCookie & COOKIE_USER2)
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
			}
			if (IsPhpKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsPhp1Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_OPERATOR);
			}
			else if (IsPhp2Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
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

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_CHAR | COOKIE_STRING);
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
		if (pfnIsKeyword && (p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsPhpKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPhpKeyword;
		else if (_stscanf(text, _T(" static BOOL IsPhp1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPhp1Keyword;
		else if (_stscanf(text, _T(" static BOOL IsPhp2Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPhp2Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 3);
	return count;
}
