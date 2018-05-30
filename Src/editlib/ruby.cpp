///////////////////////////////////////////////////////////////////////////
//  File:    ruby.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  RUBY syntax highlighing definition
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

// Ruby reserved words.
static BOOL IsRubyKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszRubyKeywordList[] =
	{
		_T("BEGIN"),
		_T("END"),
		_T("alias"),
		_T("alias_method"),
		_T("and"),
		_T("begin"),
		_T("break"),
		_T("case"),
		_T("catch"),
		_T("class"),
		_T("def"),
		_T("do"),
		_T("else"),
		_T("elsif"),
		_T("end"),
		_T("ensure"),
		_T("false"),
		_T("for"),
		_T("if"),
		_T("in"),
		_T("lambda"),
		_T("load"),
		_T("module"),
		_T("next"),
		_T("nil"),
		_T("not"),
		_T("or"),
		_T("proc"),
		_T("raise"),
		_T("redo"),
		_T("remove_method"),
		_T("require"),
		_T("require_gem"),
		_T("rescue"),
		_T("retry"),
		_T("return"),
		_T("self"),
		_T("super"),
		_T("then"),
		_T("throw"),
		_T("true"),
		_T("undef"),
		_T("undef_method"),
		_T("unless"),
		_T("until"),
		_T("when"),
		_T("while"),
		_T("yield"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszRubyKeywordList);
}

// Ruby constants (preprocessor color).
static BOOL IsRubyConstant(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszRubyConstantsList[] =
	{
		_T("$DEBUG"),
		_T("$FILENAME"),
		_T("$LOAD_PATH"),
		_T("$SAFE"),
		_T("$VERBOSE"),
		_T("$deferr"),
		_T("$defout"),
		_T("$stderr"),
		_T("$stdin"),
		_T("$stdout"),
		_T("ARGF"),
		_T("ARGV"),
		_T("DATA"),
		_T("ENV"),
		_T("FALSE"),
		_T("NIL"),
		_T("RUBY_PLATFORM"),
		_T("RUBY_RELEASE_DATE"),
		_T("RUBY_VERSION"),
		_T("SCRIPT_LINES__"),
		_T("STDERR"),
		_T("STDIN"),
		_T("STDOUT"),
		_T("TOPLEVEL_BINDING"),
		_T("TRUE"),
		_T("__FILE__"),
		_T("__LINE__"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszRubyConstantsList);
}

inline BOOL xisspace(LPCTSTR pch)
{
	return pch && xisspace(*pch);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010
#define COOKIE_VARIABLE			0X0020

void CCrystalTextView::ParseLineRuby(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= COOKIE_EXT_COMMENT;
		return;
	}

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
			if (dwCookie & (COOKIE_COMMENT | COOKIE_EXT_COMMENT))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (dwCookie & COOKIE_VARIABLE)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_USER1);
			}
			else
			{
				if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
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

			//  Extended comment =begin....=end
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				if (I == 1 && pszChars[nPrevI] == '=' && xisspace(xisequal<_tcsncmp>(pszChars + I, _T("end"))))
				{
					dwCookie &= ~COOKIE_EXT_COMMENT;
				}
				continue;
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

			if (I == 1 && pszChars[nPrevI] == '=' && xisspace(xisequal<_tcsncmp>(pszChars + I, _T("begin"))))
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_EXT_COMMENT;
				continue;
			}

			// Variable begins
			if (pszChars[I] == '$' || pszChars[I] == '@')
			{
				DEFINE_BLOCK(I, COLORINDEX_USER1);
				dwCookie |= COOKIE_VARIABLE;
				continue;
			}

			// Variable ends
			if (dwCookie & COOKIE_VARIABLE)
			{
				if (!xisalnum(pszChars[I]))
				{
					dwCookie &= ~COOKIE_VARIABLE;
					bRedefineBlock = TRUE;
					bDecIndex = TRUE;
				}
				continue;
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
			if (IsRubyKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsRubyConstant (pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_PREPROCESSOR);
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

	if (pszChars[nLength - 1] != '\\')
		dwCookie &= COOKIE_EXT_COMMENT;
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
		else if (_stscanf(text, _T(" static BOOL IsRubyKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsRubyKeyword;
		else if (_stscanf(text, _T(" static BOOL IsRubyConstant %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsRubyConstant;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
