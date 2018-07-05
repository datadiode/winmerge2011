///////////////////////////////////////////////////////////////////////////
//  File:    rsrc.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  Windows resources syntax highlighing definition
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

static BOOL IsRsrcKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszRsrcKeywordList[] =
	{
		_T("ACCELERATORS"),
		_T("ALT"),
		_T("ASCII"),
		_T("AUTO3STATE"),
		_T("AUTOCHECKBOX"),
		_T("AUTORADIOBUTTON"),
		_T("BEDIT"),
		_T("BEGIN"),
		_T("BITMAP"),
		_T("BLOCK"),
		_T("BUTTON"),
		_T("CAPTION"),
		_T("CHARACTERISTICS"),
		_T("CHECKBOX"),
		_T("CHECKED"),
		_T("CLASS"),
		_T("COMBOBOX"),
		_T("CONTROL"),
		_T("CTEXT"),
		_T("CURSOR"),
		_T("DEFPUSHBUTTON"),
		_T("DIALOG"),
		_T("DIALOGEX"),
		_T("DISCARDABLE"),
		_T("EDIT"),
		_T("EDITTEXT"),
		_T("END"),
		_T("EXSTYLE"),
		_T("FILEFLAGS"),
		_T("FILEFLAGSMASK"),
		_T("FILEOS"),
		_T("FILESUBTYPE"),
		_T("FILETYPE"),
		_T("FILEVERSION"),
		_T("FIXED"),
		_T("FONT"),
		_T("GRAYED"),
		_T("GROUPBOX"),
		_T("HEDIT"),
		_T("HELP"),
		_T("ICON"),
		_T("IEDIT"),
		_T("IMPURE"),
		_T("INACTIVE"),
		_T("LANGUAGE"),
		_T("LISTBOX"),
		_T("LOADONCALL"),
		_T("LTEXT"),
		_T("MENU"),
		_T("MENUBARBREAK"),
		_T("MENUBREAK"),
		_T("MENUEX"),
		_T("MENUITEM"),
		_T("MESSAGETABLE"),
		_T("MOVEABLE"),
		_T("NOINVERT"),
		_T("NONSHARED"),
		_T("POPUP"),
		_T("PRELOAD"),
		_T("PRODUCTVERSION"),
		_T("PURE"),
		_T("PUSHBOX"),
		_T("PUSHBUTTON"),
		_T("RADIOBUTTON"),
		_T("RCDATA"),
		_T("RTEXT"),
		_T("SCROLLBAR"),
		_T("SEPARATOR"),
		_T("SHARED"),
		_T("SHIFT"),
		_T("STATE3"),
		_T("STATIC"),
		_T("STRINGTABLE"),
		_T("STYLE"),
		_T("TEXTINCLUDE"),
		_T("USERBUTTON"),
		_T("VALUE"),
		_T("VERSION"),
		_T("VERSIONINFO"),
		_T("VIRTKEY"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszRsrcKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("VK_BACK"),
		_T("VK_CANCEL"),
		_T("VK_CAPITAL"),
		_T("VK_CLEAR"),
		_T("VK_CONTROL"),
		_T("VK_DELETE"),
		_T("VK_DOWN"),
		_T("VK_END"),
		_T("VK_ESCAPE"),
		_T("VK_F1"),
		_T("VK_F10"),
		_T("VK_F11"),
		_T("VK_F12"),
		_T("VK_F13"),
		_T("VK_F14"),
		_T("VK_F15"),
		_T("VK_F16"),
		_T("VK_F17"),
		_T("VK_F18"),
		_T("VK_F19"),
		_T("VK_F2"),
		_T("VK_F20"),
		_T("VK_F21"),
		_T("VK_F22"),
		_T("VK_F23"),
		_T("VK_F24"),
		_T("VK_F3"),
		_T("VK_F4"),
		_T("VK_F5"),
		_T("VK_F6"),
		_T("VK_F7"),
		_T("VK_F8"),
		_T("VK_F9"),
		_T("VK_HELP"),
		_T("VK_HOME"),
		_T("VK_INSERT"),
		_T("VK_LBUTTON"),
		_T("VK_LEFT"),
		_T("VK_MBUTTON"),
		_T("VK_MENU"),
		_T("VK_NEXT"),
		_T("VK_PAUSE"),
		_T("VK_PRIOR"),
		_T("VK_RBUTTON"),
		_T("VK_RETURN"),
		_T("VK_RIGHT"),
		_T("VK_SELECT"),
		_T("VK_SHIFT"),
		_T("VK_SPACE"),
		_T("VK_TAB"),
		_T("VK_UP"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszUser1KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

void CCrystalTextBuffer::ParseLineRsrc(TextBlock::Cookie &cookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	DWORD &dwCookie = cookie.m_dwCookie;

	if (nLength == 0)
	{
		dwCookie &= COOKIE_EXT_COMMENT;
		return;
	}

	BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
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
				dwCookie |= COOKIE_STRING;
				continue;
			}
			if (pszChars[I] == '\'')
			{
				if (I == 0 || !xisxdigit(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
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
			if (IsRsrcKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword, p, static_cast<int>(q - p));
		else if (_stscanf(text, _T(" static BOOL IsRsrcKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsRsrcKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
