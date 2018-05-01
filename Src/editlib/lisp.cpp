///////////////////////////////////////////////////////////////////////////
//  File:    lisp.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  LISP (particularly AutoLISP) syntax highlighing definition
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

static BOOL IsLispKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszLispKeywordList[] =
	{
		_T("abs"),
		_T("acad_colordlg"),
		_T("acad_helpdlg"),
		_T("acad_strlsort"),
		_T("action_tile"),
		_T("add_list"),
		_T("ads"),
		_T("alert"),
		_T("alloc"),
		_T("and"),
		_T("angle"),
		_T("angtof"),
		_T("angtos"),
		_T("append"),
		_T("apply"),
		_T("arx"),
		_T("arxload"),
		_T("arxunload"),
		_T("ascii"),
		_T("assoc"),
		_T("atan"),
		_T("atof"),
		_T("atoi"),
		_T("atom"),
		_T("atoms-family"),
		_T("autoarxload"),
		_T("autoload"),
		_T("autoxload"),
		_T("boole"),
		_T("boundp"),
		_T("caaaar"),
		_T("caaadr"),
		_T("caaar"),
		_T("caadar"),
		_T("caaddr"),
		_T("caadr"),
		_T("caar"),
		_T("cadaar"),
		_T("cadadr"),
		_T("cadar"),
		_T("caddar"),
		_T("cadddr"),
		_T("caddr"),
		_T("cadr"),
		_T("car"),
		_T("cdaaar"),
		_T("cdaadr"),
		_T("cdaar"),
		_T("cdadar"),
		_T("cdaddr"),
		_T("cdadr"),
		_T("cdar"),
		_T("cddaar"),
		_T("cddadr"),
		_T("cddar"),
		_T("cdddar"),
		_T("cddddr"),
		_T("cdddr"),
		_T("cddr"),
		_T("cdr"),
		_T("chr"),
		_T("client_data_tile"),
		_T("close"),
		_T("command"),
		_T("cond"),
		_T("cons"),
		_T("cos"),
		_T("cvunit"),
		_T("defun"),
		_T("dictnext"),
		_T("dictsearch"),
		_T("dimx_tile"),
		_T("dimy_tile"),
		_T("distance"),
		_T("distof"),
		_T("done_dialog"),
		_T("end_image"),
		_T("end_list"),
		_T("entdel"),
		_T("entget"),
		_T("entlast"),
		_T("entmake"),
		_T("entmod"),
		_T("entnext"),
		_T("entsel"),
		_T("entupd"),
		_T("eq"),
		_T("equal"),
		_T("eval"),
		_T("exit"),
		_T("exp"),
		_T("expand"),
		_T("expt"),
		_T("fill_image"),
		_T("findfile"),
		_T("fix"),
		_T("float"),
		_T("foreach"),
		_T("gc"),
		_T("gcd"),
		_T("get_attr"),
		_T("get_tile"),
		_T("getangle"),
		_T("getcfg"),
		_T("getcorner"),
		_T("getdist"),
		_T("getenv"),
		_T("getfield"),
		_T("getint"),
		_T("getkword"),
		_T("getorient"),
		_T("getpoint"),
		_T("getreal"),
		_T("getstring"),
		_T("getvar"),
		_T("graphscr"),
		_T("grclear"),
		_T("grdraw"),
		_T("grread"),
		_T("grtext"),
		_T("grvecs"),
		_T("handent"),
		_T("help"),
		_T("if"),
		_T("initget"),
		_T("inters"),
		_T("itoa"),
		_T("lambda"),
		_T("last"),
		_T("length"),
		_T("list"),
		_T("listp"),
		_T("load"),
		_T("load_dialog"),
		_T("log"),
		_T("logand"),
		_T("logior"),
		_T("lsh"),
		_T("mapcar"),
		_T("max"),
		_T("mem"),
		_T("member"),
		_T("menucmd"),
		_T("min"),
		_T("minusp"),
		_T("mode_tile"),
		_T("namedobjdict"),
		_T("nentsel"),
		_T("nentselp"),
		_T("new_dialog"),
		_T("not"),
		_T("nth"),
		_T("null"),
		_T("numberp"),
		_T("open"),
		_T("or"),
		_T("osnap"),
		_T("polar"),
		_T("prin1"),
		_T("princ"),
		_T("print"),
		_T("progn"),
		_T("prompt"),
		_T("quit"),
		_T("quote"),
		_T("read"),
		_T("read-char"),
		_T("read-line"),
		_T("redraw"),
		_T("regapp"),
		_T("rem"),
		_T("repeat"),
		_T("reverse"),
		_T("rtos"),
		_T("set"),
		_T("set_tile"),
		_T("setcfg"),
		_T("setfunhelp"),
		_T("setq"),
		_T("setvar"),
		_T("sin"),
		_T("slide_image"),
		_T("snvalid"),
		_T("sqrt"),
		_T("ssadd"),
		_T("ssdel"),
		_T("ssget"),
		_T("sslength"),
		_T("ssmemb"),
		_T("ssname"),
		_T("start_dialog"),
		_T("start_image"),
		_T("start_list"),
		_T("startapp"),
		_T("strcase"),
		_T("strcat"),
		_T("strlen"),
		_T("subst"),
		_T("substr"),
		_T("tablet"),
		_T("tblnext"),
		_T("tblobjname"),
		_T("tblsearch"),
		_T("term_dialog"),
		_T("terpri"),
		_T("textbox"),
		_T("textpage"),
		_T("textscr"),
		_T("trace"),
		_T("trans"),
		_T("type"),
		_T("unload_dialog"),
		_T("untrace"),
		_T("vector_image"),
		_T("ver"),
		_T("vmon"),
		_T("vports"),
		_T("wcmatch"),
		_T("while"),
		_T("write-char"),
		_T("write-line"),
		_T("xdroom"),
		_T("xdsize"),
		_T("xload"),
		_T("xunload"),
		_T("zerop"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszLispKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineLisp(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

	BOOL bRedefineBlock = TRUE;
	BOOL bDefun = FALSE;
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
				if (I > 0 && pszChars[I] == ';' && pszChars[nPrevI] == '|' && bWasComment != Start)
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

			if (I > 0 && pszChars[I] != '|' && pszChars[nPrevI] == ';' && bWasComment != End)
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
			if (I > 0 && pszChars[I] == '|' && pszChars[nPrevI] == ';' && bWasComment != End)
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
			if (IsLispKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				if (!_tcsncmp(_T("defun"), pszChars + nIdentBegin, 5))
				{
					bDefun = TRUE;
				}
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsNumeric(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
			}
			else
			{
				BOOL bFunction = FALSE;
				if (!bDefun)
				{
					for (int j = nIdentBegin; --j >= 0;)
					{
						if (!xisspace(pszChars[j]))
						{
							if (pszChars[j] == '(')
							{
								bFunction = TRUE;
							}
							break;
						}
					}
				}
				if (!bFunction)
				{
					for (int j = I; j >= 0; j--)
					{
						if (!xisspace(pszChars[j]))
						{
							if (pszChars[j] == '(')
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
		else if (_stscanf(text, _T(" static BOOL IsLispKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsLispKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
