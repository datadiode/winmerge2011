///////////////////////////////////////////////////////////////////////////
//  File:    sgml.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  SGML syntax highlighing definition
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
using HtmlKeywords::IsUser2Keyword;

static BOOL IsSgmlKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSgmlKeywordList[] =
	{
		_T("ABSTRACT"),
		_T("ARTICLE"),
		_T("AUTHOR"),
		_T("BF"),
		_T("BOOK"),
		_T("CHAPT"),
		_T("CODE"),
		_T("COPYRIGHT"),
		_T("DATE"),
		_T("DEBIANDOC"),
		_T("DESCRIP"),
		_T("DOCTYPE"),
		_T("EM"),
		_T("EMAIL"),
		_T("ENUM"),
		_T("ENUMLIST"),
		_T("EXAMPLE"),
		_T("FOOTNOTE"),
		_T("FTPPATH"),
		_T("FTPSITE"),
		_T("HEADING"),
		_T("HTMLURL"),
		_T("HTTPPATH"),
		_T("HTTPSITE"),
		_T("IT"),
		_T("ITEM"),
		_T("ITEMIZE"),
		_T("LABEL"),
		_T("LIST"),
		_T("MANREF"),
		_T("NAME"),
		_T("P"),
		_T("PRGN"),
		_T("PUBLIC"),
		_T("QREF"),
		_T("QUOTE"),
		_T("REF"),
		_T("SECT"),
		_T("SECT1"),
		_T("SECT2"),
		_T("SECT3"),
		_T("SECT4"),
		_T("STRONG"),
		_T("SYSTEM"),
		_T("TAG"),
		_T("TAGLIST"),
		_T("TITLE"),
		_T("TITLEPAG"),
		_T("TOC"),
		_T("TSCREEN"),
		_T("TT"),
		_T("URL"),
		_T("VAR"),
		_T("VERB"),
		_T("VERSION"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszSgmlKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("COMPACT"),
		_T("ID"),
		_T("NAME"),
		_T("SECTION"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszUser1KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010
#define COOKIE_USER1            0x0020
#define COOKIE_EXT_USER1        0x0040

DWORD CCrystalTextView::ParseLineSgml(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	goto start;
	do
	{
		//  Preprocessor start: < or bracket
		if (!(dwCookie & COOKIE_EXT_USER1) && pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-') || pszChars[I] == '{')
		{
			DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
			DEFINE_BLOCK(I + 1, COLORINDEX_PREPROCESSOR);
			dwCookie |= COOKIE_PREPROCESSOR;
			nIdentBegin = -1;
			goto start;
		}

		//  Preprocessor end: > or bracket
		if (dwCookie & COOKIE_PREPROCESSOR)
		{
			if (pszChars[I] == '>' || pszChars[I] == '}')
			{
				dwCookie &= ~COOKIE_PREPROCESSOR;
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				nIdentBegin = -1;
				goto start;
			}
		}

		//  Preprocessor start: &
		if (pszChars[I] == '&')
		{
			dwCookie |= COOKIE_USER1;
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
			goto start;
		}

		//  Preprocessor end: ;
		if (dwCookie & COOKIE_USER1)
		{
			if (pszChars[I] == ';')
			{
				dwCookie &= ~COOKIE_USER1;
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				nIdentBegin = -1;
				goto start;
			}
		}

	start: // First iteration starts here!
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
			else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
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
				if (I > 1 && pszChars[I] == '>' && pszChars[nPrevI] == '-' && pszChars[nPrevI - 1] == '-')
				{
					dwCookie &= ~COOKIE_EXT_COMMENT;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			//  Extended comment <?....?>
			if (dwCookie & COOKIE_EXT_USER1)
			{
				if (I > 0 && pszChars[I] == '>' && pszChars[nPrevI] == '?')
				{
					dwCookie &= ~COOKIE_EXT_USER1;
					bRedefineBlock = TRUE;
				}
				goto start;
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

			if (!(dwCookie & COOKIE_EXT_USER1) && I < nLength - 3 && pszChars[I] == '<' && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-')
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				I += 3;
				dwCookie |= COOKIE_EXT_COMMENT;
				dwCookie &= ~COOKIE_PREPROCESSOR;
				goto start;
			}

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
			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				if (IsSgmlKeyword(pszChars + nIdentBegin, I - nIdentBegin) && (pszChars[nIdentBegin - 1] == _T ('<') || pszChars[nIdentBegin - 1] == _T ('/')))
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
			}
			else if (dwCookie & COOKIE_USER1)
			{
				if (IsUser2Keyword(pszChars + nIdentBegin, I - nIdentBegin))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_STRING | COOKIE_PREPROCESSOR | COOKIE_EXT_USER1);
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
		else if (_stscanf(text, _T(" static BOOL IsSgmlKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsSgmlKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
