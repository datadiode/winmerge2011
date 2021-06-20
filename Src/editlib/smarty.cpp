///////////////////////////////////////////////////////////////////////////
//  File:    smarty.cpp
//  Version: 1.0.0.0
//  Updated: 11-Jun-2021
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  Smarty syntax highlighing definition
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

// See https://www.smarty.net/docs/en/

static BOOL IsSmartyKeyword(LPCTSTR pszChars, int nLength)
{
	// Chapter7
	// Built-in Functions
	static LPCTSTR const s_apszBuiltInFunctionList[] =
	{
		_T("append"),
		_T("as"),            // This is used with "foreach".
		_T("assign"),
		_T("block"),
		_T("break"),
		_T("call"),
		_T("capture"),
		_T("config_load"),
		_T("continue"),
		_T("debug"),
		_T("else"),
		_T("elseif"),
		_T("extends"),
		_T("for"),
		_T("foreach"),
		_T("foreachelse"),
		_T("forelse"),
		_T("function"),
		_T("if"),
		_T("include"),
		_T("include_php"),
		_T("insert"),
		_T("ldelim"),
		_T("literal"),
		_T("nocache"),
		_T("php"),
		_T("rdelim"),
		_T("section"),
		_T("sectionelse"),
		_T("setfilter"),
		_T("step"),      // This is used with "for".
		_T("strip"),
		_T("to"),        // This is used with "for".
		_T("while"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszBuiltInFunctionList);
}

static BOOL IsOperatorKeyword(LPCTSTR pszChars, int nLength)
{
	// Operators
	static LPCTSTR const s_apszOperatorList[] =
	{
		_T("by"),
		_T("div"),
		_T("eq"),
		_T("even"),
		_T("ge"),
		_T("gt"),
		_T("gte"),
		_T("is"),
		_T("le"),
		_T("lt"),
		_T("lte"),
		_T("mod"),
		_T("ne"),
		_T("neq"),
		_T("not"),
		_T("odd"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszOperatorList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	// Chapter8
	// Custom Functions
	static LPCTSTR const s_apszCustomFunctionList[] =
	{
		_T("counter"),
		_T("cycle"),
		_T("eval"),
		_T("fetch"),
		_T("html_checkboxes"),
		_T("html_image"),
		_T("html_options"),
		_T("html_radios"),
		_T("html_select_date"),
		_T("html_select_time"),
		_T("html_table"),
		_T("mailto"),
		_T("math"),
		_T("textformat"),
	};

	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszCustomFunctionList);
}

static BOOL IsUser2Keyword(LPCTSTR pszChars, int nLength)
{
	// Chapter5
	// Variable Modifiers
	static LPCTSTR const s_apszVariableModifierList[] =
	{
		_T("capitalize"),
		_T("cat"),
		_T("count_characters"),
		_T("count_paragraphs"),
		_T("count_sentences"),
		_T("count_words"),
		_T("date_format"),
		_T("default"),
		_T("escape"),
		_T("from_charset"),
		_T("indent"),
		_T("lower"),
		_T("nl2br"),
		_T("regex_replace"),
		_T("replace"),
		_T("spacify"),
		_T("string_format"),
		//  _T ("strip"),             // This is also defined as a build-in function, so comment it out.
		_T("strip_tags"),
		_T("to_charset"),
		_T("truncate"),
		_T("unescape"),
		_T("upper"),
		_T("wordwrap"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszVariableModifierList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_CHAR             0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_USER1            0x0020
#define COOKIE_USER2            0x0040
#define COOKIE_TRANSPARENT      0xFFFFFF00

void TextDefinition::ParseLineSmarty(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf) const
{
	DWORD &dwCookie = cookie.m_dwCookie;

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	do
	{
		int const nPrevI = I++;
		if (bRedefineBlock)
		{
			int const nPos = bDecIndex ? nPrevI : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (dwCookie & COOKIE_USER1)         // Config???
			{
				DEFINE_BLOCK(nPos, COLORINDEX_USER1);
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
				if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || *::CharPrev(pszChars, pszChars + nPrevI) == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = true;
				}
				continue;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || *::CharPrev(pszChars, pszChars + nPrevI) == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = true;
				}
				continue;
			}

			//  Variables loaded from config files #....#
			if (dwCookie & COOKIE_USER1)
			{
				if (pszChars[I] == '#' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || *::CharPrev(pszChars, pszChars + nPrevI) == '\\')))
				{
					dwCookie &= ~COOKIE_USER1;
					bRedefineBlock = true;
				}
				continue;
			}

			//  Normal text
			if (pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				continue;
			}

			if (pszChars[I] == '\'')
			{
				if (!I || !xisalnum(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					continue;
				}
			}

			if (pszChars[I] == '#')
			{
				DEFINE_BLOCK(I, COLORINDEX_USER1);
				dwCookie |= COOKIE_USER1;
				continue;
			}

			//  Preprocessor start: $
			if (pszChars[I] == '$')
			{
				dwCookie |= COOKIE_USER2;
				nIdentBegin = -1;
				continue;
			}

			//  Preprocessor end: ...
			if (dwCookie & COOKIE_USER2)
			{
				if (!xisalnum(pszChars[I]))
				{
					dwCookie &= ~COOKIE_USER2;
					nIdentBegin = -1;
					continue;
				}
			}

			if (pBuf == NULL)
				continue; // No need to extract keywords, so skip rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (dwCookie & COOKIE_USER2)
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
			}
			if (IsSmartyKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsUser1Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
			}
			else if (IsUser2Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
			}
			else if (IsOperatorKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_OPERATOR);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else
			{
				bool bFunction = false;

				for (int j = I; j < nLength; j++)
				{
					if (!xisspace(pszChars[j]))
					{
						if (pszChars[j] == '(')
						{
							bFunction = true;
						}
						break;
					}
				}
				if (bFunction)
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
				}
			}
			nIdentBegin = -1;
		}
		bRedefineBlock = TRUE;
		bDecIndex = TRUE;
	} while (I < nLength);

	dwCookie &= COOKIE_TRANSPARENT;
}
