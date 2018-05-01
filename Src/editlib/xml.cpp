///////////////////////////////////////////////////////////////////////////
//  File:    xml.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  XML syntax highlighing definition
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
using HtmlKeywords::IsDtdTagName;
using HtmlKeywords::IsDtdAttrName;
using HtmlKeywords::IsEntityName;

static BOOL IsXmlTagName(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszXmlTagNameList[] =
	{
		_T("xml"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszXmlTagNameList);
}

static BOOL IsXmlAttrName(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszXmlAttrNameList[] =
	{
		_T("encoding"),
		_T("standalone"),
		_T("version"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszXmlAttrNameList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_DTD              0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_CHAR             0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_EXT_COMMENT      0x0010
#define COOKIE_ENTITY           0x0020
#define COOKIE_XML              0x0040

DWORD CCrystalTextView::ParseLineXml(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_XML | COOKIE_DTD);

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	goto start;
	do
	{
		//  Preprocessor start: < or bracket
		if (pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
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
			if (pszChars[I] == '>')
			{
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_XML | COOKIE_DTD);
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				nIdentBegin = -1;
				goto start;
			}
		}

		//  Preprocessor start: &
		if (pszChars[I] == '&')
		{
			dwCookie |= COOKIE_ENTITY;
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
			goto start;
		}

		//  Preprocessor end: ;
		if (dwCookie & COOKIE_ENTITY)
		{
			if (pszChars[I] == ';')
			{
				dwCookie &= ~COOKIE_ENTITY;
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
				goto start;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				goto start;
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
				goto start;
			}

			if (pszChars[I] == '[' || pszChars[I] == ']')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD);
				goto start;
			}

			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				// Double-quoted text
				if (pszChars[I] == '"')
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_STRING;
					goto start;
				}

				// Single-quoted text
				if (pszChars[I] == '\'')
				{
					if (I == 0 || !xisxdigit(pszChars[nPrevI]))
					{
						DEFINE_BLOCK(I, COLORINDEX_STRING);
						dwCookie |= COOKIE_CHAR;
						goto start;
					}
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
					goto start;
				}
				break;
			case COOKIE_DTD:
				if (I < nLength - 1 && pszChars[I] == '-' && pszChars[I + 1] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					nIdentBegin = I;
					I += 1;
					dwCookie |= COOKIE_EXT_COMMENT;
					goto start;
				}
				break;
			}

			// Xml start: <?
			if (pszChars[I] == '<' && I < nLength - 1 && pszChars[I + 1] == '?')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				DEFINE_BLOCK(I + 1, COLORINDEX_PREPROCESSOR);
				bRedefineBlock = FALSE;
				dwCookie |= COOKIE_XML | COOKIE_PREPROCESSOR;
				nIdentBegin = -1;
				goto start;
			}

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' || pszChars[I] == '-' || pszChars[I] == '!' || pszChars[I] == '#')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				goto start;
			}
		}
		if (nIdentBegin >= 0)
		{
			LPCTSTR const pchIdent = pszChars + nIdentBegin;
			int const cchIdent = I - nIdentBegin;
			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				if (dwCookie & COOKIE_XML)
				{
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if (pchIdent > pszChars && ((pchIdent[-1] == '?' ? IsXmlTagName : IsXmlAttrName)(pchIdent, cchIdent)))
					{
						DEFINE_BLOCK(nIdentBegin, pchIdent[-1] == '?' ? COLORINDEX_KEYWORD :  COLORINDEX_USER1);
					}
				}
				else if (pchIdent > pszChars && (pchIdent[-1] == '<' && pchIdent[0] == '!'))
				{
					dwCookie |= COOKIE_DTD;
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if (IsDtdTagName(pchIdent, cchIdent))
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
					}
				}
				else if (!pBuf.m_bRecording)
				{
					// No need to extract keywords, so skip rest of loop
				}
				else if (IsNumeric(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
				}
				else if ((dwCookie & COOKIE_DTD) && IsDtdAttrName(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
				}
			}
			else if (IsNumeric(pchIdent, cchIdent))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else if (dwCookie & COOKIE_ENTITY)
			{
				if (IsEntityName(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_STRING | COOKIE_PREPROCESSOR | COOKIE_XML | COOKIE_DTD);
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
		else if (_stscanf(text, _T(" static BOOL IsXmlTagName %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsXmlTagName;
		else if (_stscanf(text, _T(" static BOOL IsXmlAttrName %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsXmlAttrName;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
