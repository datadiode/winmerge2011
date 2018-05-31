///////////////////////////////////////////////////////////////////////////
//  File:       cplusplus.cpp
//  Version:    1.2.0.5
//  Created:    29-Dec-1998
//
//  Copyright:  Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalEditView class, a part of the Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  16-Aug-99
//      Ferdinand Prantl:
//  +   FEATURE: corrected bug in syntax highlighting C comments
//  +   FEATURE: extended levels 1- 4 of keywords in some languages
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ccrystaltextview.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// C# keywords
static BOOL IsCSharpKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszCSharpKeywordList[] =
	{
		_T("abstract"),
		_T("as"),
		_T("async"),
		_T("await"),
		_T("base"),
		_T("bool"),
		_T("break"),
		_T("byte"),
		_T("case"),
		_T("catch"),
		_T("char"),
		_T("checked"),
		_T("class"),
		_T("const"),
		_T("continue"),
		_T("decimal"),
		_T("default"),
		_T("delegate"),
		_T("do"),
		_T("double"),
		_T("else"),
		_T("enum"),
		_T("event"),
		_T("exdouble"),
		_T("exfloat"),
		_T("explicit"),
		_T("extern"),
		_T("false"),
		_T("finally"),
		_T("fixed"),
		_T("float"),
		_T("for"),
		_T("foreach"),
		_T("get"),
		_T("goto"),
		_T("if"),
		_T("implicit"),
		_T("in"),
		_T("int"),
		_T("interface"),
		_T("internal"),
		_T("is"),
		_T("lock"),
		_T("long"),
		_T("namespace"),
		_T("new"),
		_T("null"),
		_T("object"),
		_T("operator"),
		_T("out"),
		_T("override"),
		_T("params"),
		_T("private"),
		_T("protected"),
		_T("public"),
		_T("readonly"),
		_T("ref"),
		_T("return"),
		_T("sbyte"),
		_T("sealed"),
		_T("set"),
		_T("short"),
		_T("sizeof"),
		_T("stackalloc"),
		_T("static"),
		_T("string"),
		_T("struct"),
		_T("switch"),
		_T("this"),
		_T("throw"),
		_T("true"),
		_T("try"),
		_T("typeof"),
		_T("uint"),
		_T("ulong"),
		_T("unchecked"),
		_T("unsafe"),
		_T("ushort"),
		_T("using"),
		_T("var"),
		_T("virtual"),
		_T("void"),
		_T("volatile"),
		_T("while"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszCSharpKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_STRING           0x000C
#define COOKIE_STRING_SINGLE    0x0004
#define COOKIE_STRING_DOUBLE    0x0008
#define COOKIE_STRING_REGEXP    0x000C // actually used here for verbatim string literals
#define COOKIE_EXT_COMMENT      0x0010
#define COOKIE_TRANSPARENT      0xFFFFFF80

void CCrystalTextView::ParseLineCSharp(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= (COOKIE_TRANSPARENT | COOKIE_EXT_COMMENT | COOKIE_STRING);
		return;
	}

	BOOL bFirstChar = (dwCookie & ~(COOKIE_TRANSPARENT | COOKIE_EXT_COMMENT)) == 0;
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
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
			}
			else if (dwCookie & COOKIE_STRING)
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
			case COOKIE_STRING_REGEXP:
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING_REGEXP;
					bRedefineBlock = TRUE;
					int nNextI = I;
					while (++nNextI < nLength && pszChars[nNextI] == '"')
					{
						I = nNextI;
						dwCookie ^= COOKIE_STRING_REGEXP;
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
				DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
				dwCookie |= I > 0 && pszChars[nPrevI] == '@' ? COOKIE_STRING_REGEXP : COOKIE_STRING_DOUBLE;
				continue;
			}
			if (pszChars[I] == '\'' && (I == 0 || !xisxdigit(pszChars[nPrevI])))
			{
				DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING_SINGLE;
				continue;
			}
			if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_EXT_COMMENT;
				bWasComment = Start;
				continue;
			}

			bWasComment = False;

			//  Preprocessor directive #....
			if (dwCookie & COOKIE_PREPROCESSOR)
				continue;

			if (bFirstChar)
			{
				if (pszChars[I] == '#')
				{
					DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
					dwCookie |= COOKIE_PREPROCESSOR;
					continue;
				}
				if (!xisspace(pszChars[I]))
					bFirstChar = FALSE;
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
			if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else if (IsCSharpKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
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
		dwCookie &= (COOKIE_TRANSPARENT | COOKIE_EXT_COMMENT | COOKIE_STRING);
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
		else if (_stscanf(text, _T(" static BOOL IsCSharpKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsCSharpKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
