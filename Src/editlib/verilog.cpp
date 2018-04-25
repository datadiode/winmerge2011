///////////////////////////////////////////////////////////////////////////
//  File:       verilog.cpp
//  Version:    1.0
//  Created:    08-Nov-2008
//
//  Copyright:  Stcherbatchenko Andrei, portions by Tim Gerundt
//  E-mail:     windfall@gmx.de
//
//  Verilog syntax highlighing definition
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

// Verilog keywords
static BOOL IsVerilogKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszVerilogKeywordList[] =
	{
		_T("always"),
		_T("and"),
		_T("assign"),
		_T("automatic"),
		_T("begin"),
		_T("buf"),
		_T("bufif0"),
		_T("bufif1"),
		_T("case"),
		_T("casex"),
		_T("casez"),
		_T("cell"),
		_T("cmos"),
		_T("config"),
		_T("deassign"),
		_T("default"),
		_T("defparam"),
		_T("design"),
		_T("disable"),
		_T("edge"),
		_T("else"),
		_T("end"),
		_T("endcase"),
		_T("endconfig"),
		_T("endfunction"),
		_T("endgenerate"),
		_T("endmodule"),
		_T("endprimitive"),
		_T("endspecify"),
		_T("endtable"),
		_T("endtask"),
		_T("event"),
		_T("for"),
		_T("force"),
		_T("forever"),
		_T("fork"),
		_T("function"),
		_T("generate"),
		_T("genvar"),
		_T("highz0"),
		_T("highz1"),
		_T("if"),
		_T("ifnone"),
		_T("incdir"),
		_T("include"),
		_T("initial"),
		_T("inout"),
		_T("input"),
		_T("instance"),
		_T("integer"),
		_T("join"),
		_T("large"),
		_T("liblist"),
		_T("library"),
		_T("localparam"),
		_T("macromodule"),
		_T("medium"),
		_T("module"),
		_T("nand"),
		_T("negedge"),
		_T("nmos"),
		_T("nor"),
		_T("noshowcancelled"),
		_T("not"),
		_T("notif0"),
		_T("notif1"),
		_T("or"),
		_T("output"),
		_T("parameter"),
		_T("pmos"),
		_T("posedge"),
		_T("primitive"),
		_T("pull0"),
		_T("pull1"),
		_T("pulldown"),
		_T("pullup"),
		_T("pulsestyle_ondetect"),
		_T("pulsestyle_onevent"),
		_T("rcmos"),
		_T("real"),
		_T("realtime"),
		_T("reg"),
		_T("release"),
		_T("repeat"),
		_T("rnmos"),
		_T("rpmos"),
		_T("rtran"),
		_T("rtranif0"),
		_T("rtranif1"),
		_T("scalared"),
		_T("showcancelled"),
		_T("signed"),
		_T("small"),
		_T("specify"),
		_T("specparam"),
		_T("strong0"),
		_T("strong1"),
		_T("supply0"),
		_T("supply1"),
		_T("table"),
		_T("task"),
		_T("time"),
		_T("tran"),
		_T("tranif0"),
		_T("tranif1"),
		_T("tri"),
		_T("tri0"),
		_T("tri1"),
		_T("triand"),
		_T("trior"),
		_T("trireg"),
		_T("unsigned"),
		_T("use"),
		_T("vectored"),
		_T("wait"),
		_T("wand"),
		_T("weak0"),
		_T("weak1"),
		_T("while"),
		_T("wire"),
		_T("wor"),
		_T("xnor"),
		_T("xor"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszVerilogKeywordList);
}

// Verilog functions
static BOOL IsVerilogFunction(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszVerilogFunctionList[] =
	{
		_T("$async$and$array"),
		_T("$async$and$plane"),
		_T("$async$nand$array"),
		_T("$async$nand$plane"),
		_T("$async$nor$array"),
		_T("$async$nor$plane"),
		_T("$async$or$array"),
		_T("$async$or$plane"),
		_T("$bitstoreal"),
		_T("$countdrivers"),
		_T("$display"),
		_T("$displayb"),
		_T("$displayh"),
		_T("$displayo"),
		_T("$dist_chi_square"),
		_T("$dist_erlang"),
		_T("$dist_exponential"),
		_T("$dist_normal"),
		_T("$dist_poisson"),
		_T("$dist_t"),
		_T("$dist_uniform"),
		_T("$dumpall"),
		_T("$dumpfile"),
		_T("$dumpflush"),
		_T("$dumplimit"),
		_T("$dumpoff"),
		_T("$dumpon"),
		_T("$dumpportsall"),
		_T("$dumpportsflush"),
		_T("$dumpportslimit"),
		_T("$dumpportsoff"),
		_T("$dumpportson"),
		_T("$dumpvars"),
		_T("$fclose"),
		_T("$fdisplay"),
		_T("$fdisplayb"),
		_T("$fdisplayf"),
		_T("$fdisplayh"),
		_T("$ferror"),
		_T("$fflush"),
		_T("$fgetc"),
		_T("$fgets"),
		_T("$finish"),
		_T("$fmonitor"),
		_T("$fmonitorb"),
		_T("$fmonitorf"),
		_T("$fmonitorh"),
		_T("$fopen"),
		_T("$fread"),
		_T("$fscanf"),
		_T("$fseek"),
		_T("$fsscanf"),
		_T("$fstrobe"),
		_T("$fstrobebb"),
		_T("$fstrobef"),
		_T("$fstrobeh"),
		_T("$ftel"),
		_T("$fullskew"),
		_T("$fwrite"),
		_T("$fwriteb"),
		_T("$fwritef"),
		_T("$fwriteh"),
		_T("$getpattern"),
		_T("$history"),
		_T("$hold"),
		_T("$incsave"),
		_T("$input"),
		_T("$itor"),
		_T("$key"),
		_T("$list"),
		_T("$log"),
		_T("$monitor"),
		_T("$monitorb"),
		_T("$monitorh"),
		_T("$monitoro"),
		_T("$monitoroff"),
		_T("$monitoron"),
		_T("$nochange"),
		_T("$nokey"),
		_T("$nolog"),
		_T("$period"),
		_T("$printtimescale"),
		_T("$q_add"),
		_T("$q_exam"),
		_T("$q_full"),
		_T("$q_initialize"),
		_T("$q_remove"),
		_T("$random"),
		_T("$readmemb"),
		_T("$readmemh"),
		_T("$realtime"),
		_T("$realtobits"),
		_T("$recovery"),
		_T("$recrem"),
		_T("$removal"),
		_T("$reset"),
		_T("$reset_count"),
		_T("$reset_value"),
		_T("$restart"),
		_T("$rewind"),
		_T("$rtoi"),
		_T("$save"),
		_T("$scale"),
		_T("$scope"),
		_T("$sdf_annotate"),
		_T("$setup"),
		_T("$setuphold"),
		_T("$sformat"),
		_T("$showscopes"),
		_T("$showvariables"),
		_T("$showvars"),
		_T("$signed"),
		_T("$skew"),
		_T("$sreadmemb"),
		_T("$sreadmemh"),
		_T("$stime"),
		_T("$stop"),
		_T("$strobe"),
		_T("$strobeb"),
		_T("$strobeh"),
		_T("$strobeo"),
		_T("$swrite"),
		_T("$swriteb"),
		_T("$swriteh"),
		_T("$swriteo"),
		_T("$sync$and$array"),
		_T("$sync$and$plane"),
		_T("$sync$nand$array"),
		_T("$sync$nand$plane"),
		_T("$sync$nor$array"),
		_T("$sync$nor$plane"),
		_T("$sync$or$array"),
		_T("$sync$or$plane"),
		_T("$test$plusargs"),
		_T("$time"),
		_T("$timeformat"),
		_T("$timeskew"),
		_T("$ungetc"),
		_T("$unsigned"),
		_T("$value$plusargs"),
		_T("$width"),
		_T("$write"),
		_T("$writeb"),
		_T("$writeh"),
		_T("$writeo"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszVerilogFunctionList);
}

static BOOL IsVerilogNumber(LPCTSTR pszChars, int nLength)
{
  return _istdigit(pszChars[0]);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineVerilog(DWORD dwCookie, int nLineIndex, TextBlock::Array &pBuf)
{
	int const nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
		return dwCookie & COOKIE_EXT_COMMENT;

	LPCTSTR const pszChars = GetLineChars(nLineIndex);
	BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
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
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
			}
			else if (dwCookie & (COOKIE_CHAR | COOKIE_STRING))
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '$' || (pszChars[nPos] == '\'' && nPos > 0 && (xisalpha(pszChars[nPos + 1]))))
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

			//  String constant "..."
			if (dwCookie & COOKIE_STRING)
			{
				if (pszChars[I] == '"' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				continue;
			}

			//  Extended comment /*...*/
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

			// Line comment //...
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
			if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/' && bWasComment != End)
			{
				DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_EXT_COMMENT;
				bWasComment = Start;
				continue;
			}

			bWasComment = False;

			//  Preprocessor directive `...
			if (dwCookie & COOKIE_PREPROCESSOR)
				continue;

			if (bFirstChar)
			{
				if (pszChars[I] == '`')
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

			if (xisalnum(pszChars[I]) || pszChars[I] == '$' || pszChars[I] == '\'')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				continue;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (IsVerilogKeyword(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
			}
			else if (IsVerilogFunction(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
			}
			else if (IsVerilogNumber(pszChars + nIdentBegin, I - nIdentBegin))
			{
				DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
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
		else if (_stscanf(text, _T(" static BOOL IsVerilogKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsVerilogKeyword;
		else if (_stscanf(text, _T(" static BOOL IsVerilogFunction %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsVerilogFunction;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsncmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 2);
	return count;
}
