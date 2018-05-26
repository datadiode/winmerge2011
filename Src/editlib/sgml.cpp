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
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;
using HtmlKeywords::IsDtdTagName;
using HtmlKeywords::IsDtdAttrName;
using HtmlKeywords::IsEntityName;

static BOOL IsSgmlTagName(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSgmlTagNameList[] =
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
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszSgmlTagNameList);
}

static BOOL IsSgmlAttrName(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSgmlAttrNameList[] =
	{
		_T("COMPACT"),
		_T("ID"),
		_T("NAME"),
		_T("SECTION"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszSgmlAttrNameList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_DTD              0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_CHAR             0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_EXT_COMMENT      0x0010

DWORD CCrystalTextView::ParseLineSgml(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie;

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	do
	{
		int const nPrevI = I++;
		if (bRedefineBlock && pBuf.m_bRecording)
		{
			int const nPos = bDecIndex ? nPrevI : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & COOKIE_EXT_COMMENT)
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
			//  String constant "...."
			if (dwCookie & COOKIE_STRING)
			{
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Extended comment <!--....-->
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				switch (dwCookie & COOKIE_DTD)
				{
				case 0:
					if (I > 1 && (nIdentBegin == -1 || I - nIdentBegin >= 6) && pszChars[I] == '>' && pszChars[nPrevI] == '-' && pszChars[nPrevI - 1] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_DTD:
					if (I > 0 && (nIdentBegin == -1 || I - nIdentBegin >= 4) && pszChars[I] == '-' && pszChars[nPrevI] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				}
				continue;
			}

			if (pszChars[I] == '[' || pszChars[I] == ']')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD);
				continue;
			}

			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				// Double-quoted text
				if (pszChars[I] == '"')
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_STRING;
					continue;
				}

				// Single-quoted text
				if (pszChars[I] == '\'' && (I == 0 || !xisxdigit(pszChars[nPrevI])))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					continue;
				}
			}

			switch (dwCookie & COOKIE_DTD)
			{
			case 0:
				if (I < nLength - 3 && pszChars[I] == '<' && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					nIdentBegin = I;
					I += 3;
					dwCookie |= COOKIE_EXT_COMMENT;
					dwCookie &= ~COOKIE_PREPROCESSOR;
					continue;
				}
				break;
			case COOKIE_DTD:
				if (I < nLength - 1 && pszChars[I] == '-' && pszChars[I + 1] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					nIdentBegin = I;
					I += 1;
					dwCookie |= COOKIE_EXT_COMMENT;
					continue;
				}
				break;
			}

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' || pszChars[I] == '-' || pszChars[I] == '!' || pszChars[I] == '#')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
		if (nIdentBegin >= 0)
		{
			LPCTSTR const pchIdent = pszChars + nIdentBegin;
			int const cchIdent = I - nIdentBegin;
			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				if (pchIdent > pszChars && (pchIdent[-1] == '<' || pchIdent[-1] == '/'))
				{
					if (*pchIdent == '!')
					{
						dwCookie |= COOKIE_DTD;
					}
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if ((dwCookie & COOKIE_DTD ? IsDtdTagName : IsSgmlTagName)(pchIdent, cchIdent))
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
					}
				}
				else if (!pBuf.m_bRecording)
				{
					// No need to extract keywords, so skip rest of loop
				}
				else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
				}
				else if ((dwCookie & COOKIE_DTD ? IsDtdAttrName : IsSgmlAttrName)(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
				}
			}
			else if (!pBuf.m_bRecording)
			{
				// No need to extract keywords, so skip rest of loop
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else if (pchIdent > pszChars && pchIdent[-1] == '&')
			{
				if (IsEntityName(pszChars + nIdentBegin, I - nIdentBegin))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
			nIdentBegin = -1;
		}

		bRedefineBlock = TRUE;
		bDecIndex = TRUE;

		switch (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_PARSER))
		{
		case 0:
			// Preprocessor start: < or bracket
			if (pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				DEFINE_BLOCK(I + 1, COLORINDEX_PREPROCESSOR);
				dwCookie |= COOKIE_PREPROCESSOR;
			}
			break;
		case COOKIE_PREPROCESSOR:
			// Preprocessor end: > or bracket
			if (pszChars[I] == '>')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD);
			}
			break;
		}
	} while (I < nLength);

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
		else if (_stscanf(text, _T(" static BOOL IsSgmlTagName %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsSgmlTagName;
		else if (_stscanf(text, _T(" static BOOL IsSgmlAttrName %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsSgmlAttrName;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
