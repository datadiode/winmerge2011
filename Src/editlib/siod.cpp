///////////////////////////////////////////////////////////////////////////
//  File:    siod.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  SIOD syntax highlighing definition
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

static BOOL IsSiodKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSiodKeywordList[] =
	{
		_T("abs"),
		_T("alarm"),
		_T("and"),
		_T("append"),
		_T("apply"),
		_T("ass"),
		_T("assoc"),
		_T("assq"),
		_T("assv"),
		_T("base64decode"),
		_T("base64encode"),
		_T("begin"),
		_T("caaar"),
		_T("caadr"),
		_T("caar"),
		_T("cadar"),
		_T("caddr"),
		_T("cadr"),
		_T("car"),
		_T("cdaar"),
		_T("cdadr"),
		_T("cdar"),
		_T("cddar"),
		_T("cdddr"),
		_T("cddr"),
		_T("cdr"),
		_T("cond"),
		_T("cons-array"),
		_T("define"),
		_T("eq?"),
		_T("equal?"),
		_T("eqv?"),
		_T("eval"),
		_T("exec"),
		_T("exit"),
		_T("fclose"),
		_T("fopen"),
		_T("if"),
		_T("lambda"),
		_T("length"),
		_T("let"),
		_T("let*"),
		_T("letrec"),
		_T("list"),
		_T("load"),
		_T("max"),
		_T("member"),
		_T("memq"),
		_T("memv"),
		_T("min"),
		_T("nil"),
		_T("not"),
		_T("null?"),
		_T("number->string"),
		_T("number?"),
		_T("or"),
		_T("pair?"),
		_T("quit"),
		_T("quote"),
		_T("read"),
		_T("reverse"),
		_T("set!"),
		_T("set-car!"),
		_T("set-cdr!"),
		_T("string->number"),
		_T("string-append"),
		_T("string-length"),
		_T("string?"),
		_T("substring"),
		_T("symbol?"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszSiodKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser1KeywordList[] =
	{
		_T("acos"),
		_T("asin"),
		_T("atan"),
		_T("cos"),
		_T("exp"),
		_T("log"),
		_T("sin"),
		_T("sqrt"),
		_T("tan"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszUser1KeywordList);
}

static BOOL IsUser2Keyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszUser2KeywordList[] =
	{
		_T("%%%memref"),
		_T("%%closure"),
		_T("%%closure-code"),
		_T("%%closure-env"),
		_T("%%stack-limit"),
		_T("*after-gc*"),
		_T("*args*"),
		_T("*catch"),
		_T("*env*"),
		_T("*eval-history-ptr*"),
		_T("*pi*"),
		_T("*plists*"),
		_T("*throw"),
		_T("*traced*"),
		_T("access-problem?"),
		_T("allocate-heap"),
		_T("apropos"),
		_T("aref"),
		_T("array->hexstr"),
		_T("aset"),
		_T("ash"),
		_T("atan2"),
		_T("benchmark-eval"),
		_T("benchmark-funcall1"),
		_T("benchmark-funcall2"),
		_T("bit-and"),
		_T("bit-not"),
		_T("bit-or"),
		_T("bit-xor"),
		_T("butlast"),
		_T("bytes-append"),
		_T("chdir"),
		_T("chmod"),
		_T("chown"),
		_T("closedir"),
		_T("copy-list"),
		_T("cpu-usage-limits"),
		_T("crypt"),
		_T("current-resource-usage"),
		_T("datlength"),
		_T("datref"),
		_T("decode-file-mode"),
		_T("delete-file"),
		_T("delq"),
		_T("encode-file-mode"),
		_T("encode-open-flags"),
		_T("endpwent"),
		_T("env-lookup"),
		_T("eof-val"),
		_T("errobj"),
		_T("error"),
		_T("F_GETLK"),
		_T("F_SETLK"),
		_T("F_SETLKW"),
		_T("fast-load"),
		_T("fast-print"),
		_T("fast-read"),
		_T("fast-save"),
		_T("fchmod"),
		_T("fflush"),
		_T("file-times"),
		_T("first"),
		_T("fmod"),
		_T("fnmatch"),
		_T("fork"),
		_T("fread"),
		_T("fseek"),
		_T("fstat"),
		_T("ftell"),
		_T("fwrite"),
		_T("gc"),
		_T("gc-info"),
		_T("gc-status"),
		_T("get"),
		_T("getc"),
		_T("getcwd"),
		_T("getenv"),
		_T("getgid"),
		_T("getgrgid"),
		_T("getpass"),
		_T("getpgrp"),
		_T("getpid"),
		_T("getppid"),
		_T("getpwent"),
		_T("getpwnam"),
		_T("getpwuid"),
		_T("gets"),
		_T("getuid"),
		_T("gmtime"),
		_T("hexstr->bytes"),
		_T("href"),
		_T("hset"),
		_T("html-encode"),
		_T("intern"),
		_T("kill"),
		_T("larg-default"),
		_T("last"),
		_T("last-c-error"),
		_T("lchown"),
		_T("link"),
		_T("lkey-default"),
		_T("load-so"),
		_T("localtime"),
		_T("lref-default"),
		_T("lstat"),
		_T("make-list"),
		_T("mapcar"),
		_T("md5-final"),
		_T("md5-init"),
		_T("md5-update"),
		_T("mkdatref"),
		_T("mkdir"),
		_T("mktime"),
		_T("nconc"),
		_T("nice"),
		_T("nreverse"),
		_T("nth"),
		_T("opendir"),
		_T("os-classification"),
		_T("parse-number"),
		_T("pclose"),
		_T("popen"),
		_T("pow"),
		_T("prin1"),
		_T("print"),
		_T("print-to-string"),
		_T("prog1"),
		_T("putc"),
		_T("putenv"),
		_T("putprop"),
		_T("puts"),
		_T("qsort"),
		_T("rand"),
		_T("random"),
		_T("read-from-string"),
		_T("readdir"),
		_T("readline"),
		_T("readlink"),
		_T("realtime"),
		_T("rename"),
		_T("require"),
		_T("require-so"),
		_T("rest"),
		_T("rld-pathnames"),
		_T("rmdir"),
		_T("runtime"),
		_T("save-forms"),
		_T("sdatref"),
		_T("set-eval-history"),
		_T("set-symbol-value!"),
		_T("setprop"),
		_T("setpwent"),
		_T("setuid"),
		_T("siod-lib"),
		_T("sleep"),
		_T("so-ext"),
		_T("srand"),
		_T("srandom"),
		_T("stat"),
		_T("strbreakup"),
		_T("strcat"),
		_T("strcmp"),
		_T("strcpy"),
		_T("strcspn"),
		_T("strftime"),
		_T("string-dimension"),
		_T("string-downcase"),
		_T("string-lessp"),
		_T("string-search"),
		_T("string-trim"),
		_T("string-trim-left"),
		_T("string-trim-right"),
		_T("string-upcase"),
		_T("strptime"),
		_T("strspn"),
		_T("subset"),
		_T("substring-equal?"),
		_T("swrite"),
		_T("sxhash"),
		_T("symbol-bound?"),
		_T("symbol-value"),
		_T("symbolconc"),
		_T("symlink"),
		_T("system"),
		_T("t"),
		_T("the-environment"),
		_T("trace"),
		_T("trunc"),
		_T("typeof"),
		_T("unbreakupstr"),
		_T("ungetc"),
		_T("unix-ctime"),
		_T("unix-time"),
		_T("unix-time->strtime"),
		_T("unlink"),
		_T("untrace"),
		_T("url-decode"),
		_T("url-encode"),
		_T("utime"),
		_T("verbose"),
		_T("wait"),
		_T("while"),
		_T("writes"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszUser2KeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_CHAR             0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_EXT_COMMENT      0xFF00

DWORD CCrystalTextView::ParseLineSiod(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	if (nLength == 0)
		return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_STRING);

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

			if (I > 0 && pszChars[I] == '|' && pszChars[nPrevI] == '#' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie - COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
				bWasComment = Start;
				continue;
			}

			//  Extended comment #|....|#
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				if (I > 0 && pszChars[I] == '#' && pszChars[nPrevI] == '|' && bWasComment != Start)
				{
					dwCookie = dwCookie & ~COOKIE_EXT_COMMENT | dwCookie + COOKIE_EXT_COMMENT & COOKIE_EXT_COMMENT;
					bRedefineBlock = TRUE;
					bWasComment = End;
				}
				else
				{
					bWasComment = False;
				}
				continue;
			}
			bWasComment = False;

			if (I > 0 && pszChars[nPrevI] == ';')
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
			if (IsSiodKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				if (!_tcsnicmp(_T("defun"), pszChars + nIdentBegin, 5))
				{
					bDefun = TRUE;
				}
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

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_STRING);
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
		else if (_stscanf(text, _T(" static BOOL IsSiodKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsSiodKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (_stscanf(text, _T(" static BOOL IsUser2Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser2Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 3);
	return count;
}
