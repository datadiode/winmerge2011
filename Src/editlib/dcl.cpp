///////////////////////////////////////////////////////////////////////////
//  File:    dcl.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  AutoCAD DCL syntax highlighing definition
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

static BOOL IsDclKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszDclKeywordList[] =
	{
		_T("boxed_column"),
		_T("boxed_radio_column"),
		_T("boxed_radio_row"),
		_T("boxed_row"),
		_T("button"),
		_T("column"),
		_T("concatenation"),
		_T("dialog"),
		_T("edit_box"),
		_T("image"),
		_T("list_box"),
		_T("paragraph"),
		_T("popup_list"),
		_T("radio_button"),
		_T("row"),
		_T("slider"),
		_T("spacer"),
		_T("text"),
		_T("text_part"),
		_T("toggle"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszDclKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("cancel_button"),
		_T("default_button"),
		_T("errtile"),
		_T("help_button"),
		_T("image_button"),
		_T("info_button"),
		_T("ok_button"),
		_T("ok_cancel"),
		_T("ok_cancel_err"),
		_T("ok_cancel_help"),
		_T("ok_cancel_help_errtile"),
		_T("ok_cancel_help_info"),
		_T("ok_only"),
		_T("radio_column"),
		_T("radio_row"),
		_T("retirement_button"),
		_T("spacer_0"),
		_T("spacer_1"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser1KeywordList);
}

static BOOL IsUser2Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser2KeywordList[] =
	{
		_T("action"),
		_T("alignment"),
		_T("allow_accept"),
		_T("aspect_ratio"),
		_T("big_increment"),
		_T("children_alignment"),
		_T("children_fixed_height"),
		_T("children_fixed_width"),
		_T("edit_limit"),
		_T("edit_width"),
		_T("fixed_height"),
		_T("fixed_width"),
		_T("height"),
		_T("initial_focus"),
		_T("is_bold"),
		_T("is_cancel"),
		_T("is_default"),
		_T("is_enabled"),
		_T("is_tab_stop"),
		_T("key"),
		_T("label"),
		_T("layout"),
		_T("list"),
		_T("max_value"),
		_T("min_value"),
		_T("mnemonic"),
		_T("multiple_select"),
		_T("small_increment"),
		_T("tabs"),
		_T("value"),
		_T("width"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser2KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineDcl(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

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

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
					int nPrevI = I;
					while (nPrevI && pszChars[--nPrevI] == '\\')
					{
						dwCookie ^= COOKIE_CHAR;
						bRedefineBlock ^= TRUE;
					}
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
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				continue;
			}
			if (pszChars[I] == '\'')
			{
				if (I == 0 || !xisxdigit(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
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
			if (IsDclKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsUser1Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
			}
			else if (IsUser2Keyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else
			{
				bool bFunction = FALSE;

				for (int j = I; j < nLength; j++)
				{
					if (!xisspace(pszChars[j]))
					{
						if (pszChars[j] == '{' || pszChars[j] == ':')
						{
							bFunction = TRUE;
						}
						break;
					}
				}
				if (!bFunction)
				{
					for (int j = nIdentBegin; --j >= 0;)
					{
						if (!xisspace(pszChars[j]))
						{
							if (pszChars[j] == ':')
							{
								bFunction = TRUE;
							}
							break;
						}
					}
				}
				if (bFunction)
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

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
		if (pfnIsKeyword && (p = _tcschr(text, '"')) != NULL && (q = _tcschr(++p, '"')) != NULL)
			VerifyKeyword<_tcsncmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsDclKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsDclKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (_stscanf(text, _T(" static BOOL IsUser2Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser2Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 3);
	return count;
}
