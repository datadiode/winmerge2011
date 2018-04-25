///////////////////////////////////////////////////////////////////////////
//  File:    perl.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  PERL syntax highlighing definition
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

static BOOL IsPerlKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszPerlKeywordList[] =
	{
		_T("abs"),
		_T("accept"),
		_T("alarm"),
		_T("and"),
		_T("atan2"),
		_T("bind"),
		_T("binmode"),
		_T("bless"),
		_T("bootstrap"),
		_T("caller"),
		_T("chdir"),
		_T("chmod"),
		_T("chomp"),
		_T("chop"),
		_T("chown"),
		_T("chr"),
		_T("chroot"),
		_T("close"),
		_T("closedir"),
		_T("connect"),
		_T("continue"),
		_T("cos"),
		_T("crypt"),
		_T("dbmclose"),
		_T("dbmopen"),
		_T("defined"),
		_T("delete"),
		_T("die"),
		_T("do"),
		_T("dump"),
		_T("each"),
		_T("else"),
		_T("elsif"),
		_T("endgrent"),
		_T("endhostent"),
		_T("endnetent"),
		_T("endprotoent"),
		_T("endpwent"),
		_T("endservent"),
		_T("eof"),
		_T("eq"),
		_T("eval"),
		_T("exec"),
		_T("exists"),
		_T("exit"),
		_T("exp"),
		_T("fcntl"),
		_T("fileno"),
		_T("flock"),
		_T("for"),
		_T("foreach"),
		_T("fork"),
		_T("formline"),
		_T("getc"),
		_T("getgrent"),
		_T("getgrgid"),
		_T("getgrnam"),
		_T("getgrname"),
		_T("gethostbyaddr"),
		_T("gethostbyname"),
		_T("gethostent"),
		_T("getlogin"),
		_T("getnetbyaddr"),
		_T("getnetbyname"),
		_T("getnetent"),
		_T("getpeername"),
		_T("getpgrp"),
		_T("getppid"),
		_T("getpriority"),
		_T("getprotobyname"),
		_T("getprotobynumber"),
		_T("getprotoent"),
		_T("getpwent"),
		_T("getpwnam"),
		_T("getpwuid"),
		_T("getservbyname"),
		_T("getservbyport"),
		_T("getservent"),
		_T("getsockname"),
		_T("getsockopt"),
		_T("glob"),
		_T("gmtime"),
		_T("goto"),
		_T("grep"),
		_T("hex"),
		_T("if"),
		_T("import"),
		_T("index"),
		_T("int"),
		_T("ioctl"),
		_T("join"),
		_T("keys"),
		_T("kill"),
		_T("last"),
		_T("lc"),
		_T("lcfirst"),
		_T("length"),
		_T("link"),
		_T("listen"),
		_T("local"),
		_T("localtime"),
		_T("log"),
		_T("lstat"),
		_T("m"),
		_T("map"),
		_T("mkdir"),
		_T("msgctl"),
		_T("msgget"),
		_T("msgrcv"),
		_T("msgsnd"),
		_T("my"),
		_T("ne"),
		_T("next"),
		_T("no"),
		_T("not"),
		_T("oct"),
		_T("open"),
		_T("opendir"),
		_T("or"),
		_T("ord"),
		_T("pack"),
		_T("package"),
		_T("pipe"),
		_T("pop"),
		_T("pos"),
		_T("print"),
		_T("printf"),
		_T("push"),
		_T("q"),
		_T("qq"),
		_T("quotemeta"),
		_T("qw"),
		_T("qx"),
		_T("rand"),
		_T("read"),
		_T("readdir"),
		_T("readlink"),
		_T("recv"),
		_T("redo"),
		_T("ref"),
		_T("rename"),
		_T("require"),
		_T("reset"),
		_T("return"),
		_T("reverse"),
		_T("rewinddir"),
		_T("rindex"),
		_T("rmdir"),
		_T("s"),
		_T("scalar"),
		_T("seek"),
		_T("seekdir"),
		_T("select"),
		_T("semctl"),
		_T("semget"),
		_T("semop"),
		_T("send"),
		_T("setgrent"),
		_T("sethostent"),
		_T("setnetent"),
		_T("setpgrp"),
		_T("setpriority"),
		_T("setprotoent"),
		_T("setpwent"),
		_T("setservent"),
		_T("setsockopt"),
		_T("sgmget"),
		_T("shift"),
		_T("shmctl"),
		_T("shmread"),
		_T("shmwrite"),
		_T("shutdown"),
		_T("sin"),
		_T("sleep"),
		_T("socket"),
		_T("socketpair"),
		_T("sort"),
		_T("splice"),
		_T("split"),
		_T("sprintf"),
		_T("sqrt"),
		_T("srand"),
		_T("stat"),
		_T("study"),
		_T("sub"),
		_T("substr"),
		_T("symlink"),
		_T("syscall"),
		_T("sysread"),
		_T("system"),
		_T("syswrite"),
		_T("tan"),
		_T("tell"),
		_T("telldir"),
		_T("tie"),
		_T("time"),
		_T("times"),
		_T("tr"),
		_T("truncate"),
		_T("uc"),
		_T("ucfirst"),
		_T("umask"),
		_T("undef"),
		_T("unless"),
		_T("unlink"),
		_T("unpack"),
		_T("unshift"),
		_T("untie"),
		_T("until"),
		_T("use"),
		_T("utime"),
		_T("values"),
		_T("vec"),
		_T("wait"),
		_T("waitpid"),
		_T("wantarray"),
		_T("warn"),
		_T("while"),
		_T("write"),
		_T("x"),
		_T("y"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszPerlKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLinePerl(DWORD dwCookie, int nLineIndex, TextBlock::Array &pBuf)
{
	int const nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

	LPCTSTR const pszChars = GetLineChars(nLineIndex);
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	int I = -1;
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
				if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
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
				// if (I + 1 < nLength && pszChars[I + 1] == '\'' || I + 2 < nLength && pszChars[I + 1] != '\\' && pszChars[I + 2] == '\'' || I + 3 < nLength && pszChars[I + 1] == '\\' && pszChars[I + 3] == '\'')
				if (!I || !xisalnum(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					continue;
				}
			}

			if (pBuf == NULL)
				continue;               //  We don't need to extract keywords,
			//  for faster parsing skip the rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (IsPerlKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
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
		else if (_stscanf(text, _T(" static BOOL IsPerlKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsPerlKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
