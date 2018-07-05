///////////////////////////////////////////////////////////////////////////
//  File:    python.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  Python syntax highlighing definition
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

// Python 2.6 keywords
static BOOL IsPythonKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszPythonKeywordList[] =
	{
		_T("and"),
		_T("as"),
		_T("assert"),
		_T("break"),
		_T("class"),
		_T("continue"),
		_T("def"),
		_T("del"),
		_T("elif"),
		_T("else"),
		_T("except"),
		_T("exec"),
		_T("finally"),
		_T("for"),
		_T("from"),
		_T("global"),
		_T("if"),
		_T("import"),
		_T("in"),
		_T("is"),
		_T("lambda"),
		_T("not"),
		_T("or"),
		_T("pass"),
		_T("print"),
		_T("raise"),
		_T("return"),
		_T("try"),
		_T("while"),
		_T("whith"),
		_T("yield"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszPythonKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("AttributeError"),
		_T("EOFError"),
		_T("Ellipsis"),
		_T("False"),
		_T("IOError"),
		_T("ImportError"),
		_T("IndexError"),
		_T("KeyError"),
		_T("KeyboardInterrupt"),
		_T("MemoryError"),
		_T("NameError"),
		_T("None"),
		_T("NotImplemented"),
		_T("OverflowError"),
		_T("RuntimeError"),
		_T("SyntaxError"),
		_T("SystemError"),
		_T("SystemExit"),
		_T("True"),
		_T("TypeError"),
		_T("ValueError"),
		_T("ZeroDivisionError"),
		_T("__debug__"),
		_T("argv"),
		_T("builtin_module_names"),
		_T("exc_traceback"),
		_T("exc_type"),
		_T("exc_value"),
		_T("exit"),
		_T("exitfunc"),
		_T("last_traceback"),
		_T("last_type"),
		_T("last_value"),
		_T("modules"),
		_T("path"),
		_T("ps1"),
		_T("ps2"),
		_T("setprofile"),
		_T("settrace"),
		_T("stderr"),
		_T("stdin"),
		_T("stdout"),
		_T("tracebacklimit"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser1KeywordList);
}

static BOOL IsUser2Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser2KeywordList[] =
	{
		_T("__abs__"),
		_T("__add__"),
		_T("__and__"),
		_T("__bases__"),
		_T("__class__"),
		_T("__cmp__"),
		_T("__coerce__"),
		_T("__del__"),
		_T("__dict__"),
		_T("__div__"),
		_T("__divmod__"),
		_T("__float__"),
		_T("__getitem__"),
		_T("__hash__"),
		_T("__hex__"),
		_T("__init__"),
		_T("__int__"),
		_T("__invert__"),
		_T("__len__"),
		_T("__long__"),
		_T("__lshift__"),
		_T("__members__"),
		_T("__methods__"),
		_T("__mod__"),
		_T("__mul__"),
		_T("__neg__"),
		_T("__nonzero__"),
		_T("__oct__"),
		_T("__or__"),
		_T("__pos__"),
		_T("__pow__"),
		_T("__repr__"),
		_T("__rshift__"),
		_T("__str__"),
		_T("__sub__"),
		_T("__xor__"),
		_T("abs"),
		_T("coerce"),
		_T("divmod"),
		_T("float"),
		_T("hex"),
		_T("id"),
		_T("int"),
		_T("len"),
		_T("long"),
		_T("nonzero"),
		_T("oct"),
		_T("pow"),
		_T("range"),
		_T("round"),
		_T("xrange"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser2KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

void CCrystalTextBuffer::ParseLinePython(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
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
			if (IsPythonKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
		else if (_stscanf(text, _T(" static BOOL IsPythonKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPythonKeyword;
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
