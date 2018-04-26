///////////////////////////////////////////////////////////////////////////
//  File:    asp.cpp
//  Version: 1.1.0.4
//  Updated: 19-Jul-1998
//
//  Copyright:  Ferdinand Prantl, portions by Stcherbatchenko Andrei
//  E-mail:     prantl@ff.cuni.cz
//
//  ASP syntax highlighing definition
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

using HtmlKeywords::IsHtmlKeyword;
using HtmlKeywords::IsUser1Keyword;
using HtmlKeywords::IsUser2Keyword;

static BOOL IsAspKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszAspKeywordList[] =
	{
		_T("Abs"),
		_T("AppActivate"),
		_T("As"),
		_T("Asc"),
		_T("Atn"),
		_T("Base"),
		_T("Beep"),
		_T("Call"),
		_T("Case"),
		_T("CDbl"),
		_T("ChDir"),
		_T("ChDrive"),
		_T("CheckBox"),
		_T("Chr"),
		_T("CInt"),
		_T("CLng"),
		_T("Close"),
		_T("Const"),
		_T("Cos"),
		_T("CreateObject"),
		_T("CreateVerifyItem"),
		_T("CSng"),
		_T("CStr"),
		_T("CurDir"),
		_T("CVar"),
		_T("Date"),
		_T("Declare"),
		_T("Dialog"),
		_T("Dim"),
		_T("Dir"),
		_T("DlgEnable"),
		_T("DlgText"),
		_T("DlgVisible"),
		_T("Do"),
		_T("Double"),
		_T("Else"),
		_T("End"),
		_T("EOF"),
		_T("Erase"),
		_T("Error"),
		_T("Exit"),
		_T("Exp"),
		_T("FileCopy"),
		_T("FileLen"),
		_T("Fix"),
		_T("For"),
		_T("Format"),
		_T("Function"),
		_T("GetAttrName"),
		_T("GetAttrType"),
		_T("GetAttrValBool"),
		_T("GetAttrValEnumInt"),
		_T("GetAttrValEnumString"),
		_T("GetAttrValFloat"),
		_T("GetAttrValInt"),
		_T("GetAttrValString"),
		_T("GetClassId"),
		_T("GetGeoType"),
		_T("GetObject"),
		_T("Global"),
		_T("GoSub"),
		_T("GoTo"),
		_T("Hex"),
		_T("Hour"),
		_T("If"),
		_T("Input"),
		_T("Input#"),
		_T("InputBox"),
		_T("InStr"),
		_T("Int"),
		_T("IsDate"),
		_T("IsEmpty"),
		_T("IsNull"),
		_T("IsNumeric"),
		_T("Kill"),
		_T("LBound"),
		_T("LCase"),
		_T("LCase$"),
		_T("Left"),
		_T("Left$"),
		_T("Len"),
		_T("Let"),
		_T("Line"),
		_T("Log"),
		_T("Long"),
		_T("Loop"),
		_T("LTrim"),
		_T("Mid"),
		_T("Minute"),
		_T("MkDir"),
		_T("Month"),
		_T("MsgBox"),
		_T("Name"),
		_T("Next"),
		_T("Now"),
		_T("Oct"),
		_T("On"),
		_T("Open"),
		_T("Option"),
		_T("Print"),
		_T("Rem"),
		_T("Return"),
		_T("Right"),
		_T("RmDir"),
		_T("Rnd"),
		_T("RTrim"),
		_T("Second"),
		_T("Seek"),
		_T("Select"),
		_T("SendKeys"),
		_T("Set"),
		_T("SetAttrValBool"),
		_T("SetAttrValEnumInt"),
		_T("SetAttrValEnumString"),
		_T("SetAttrValFloat"),
		_T("SetAttrValInt"),
		_T("SetAttrValString"),
		_T("Shell"),
		_T("Sin"),
		_T("SMDoMenu"),
		_T("Space"),
		_T("Sqr"),
		_T("Static"),
		_T("Step"),
		_T("Stop"),
		_T("Str"),
		_T("StrComp"),
		_T("String"),
		_T("StringFunction"),
		_T("Sub"),
		_T("Tan"),
		_T("Text"),
		_T("TextBox"),
		_T("Then"),
		_T("Time"),
		_T("TimeSerial"),
		_T("TimeValue"),
		_T("To"),
		_T("Trim"),
		_T("Type"),
		_T("UBound"),
		_T("UCase"),
		_T("Val"),
		_T("VarType"),
		_T("VerifyCardinalities"),
		_T("Wend"),
		_T("While"),
		_T("With"),
		_T("Write"),
		_T("Year"),
	};
	return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszAspKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010
#define COOKIE_USER1            0x0020
#define COOKIE_EXT_USER1        0x0040

DWORD CCrystalTextView::ParseLineAsp(DWORD dwCookie, int nLineIndex, TextBlock::Array &pBuf)
{
	int const nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
		return dwCookie & (COOKIE_EXT_COMMENT|COOKIE_EXT_USER1);

	LPCTSTR const pszChars = GetLineChars(nLineIndex);
	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	int nIdentBegin = -1;
	int I = -1;
	int nPrevI;
	goto start;
	do
	{
		//  Preprocessor start: < or bracket
		if (!(dwCookie & COOKIE_EXT_USER1) && pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
		{
			DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
			DEFINE_BLOCK(I + 1, COLORINDEX_PREPROCESSOR);
			dwCookie |= COOKIE_PREPROCESSOR;
			nIdentBegin = -1;
			goto start;
		}

		//  User1 end: ?>
		if (dwCookie & COOKIE_EXT_USER1)
		{
			if (I > 0 && pszChars[I] == '>' && (pszChars[nPrevI] == '?' || pszChars[nPrevI] == '%'))
			{
				dwCookie &= ~COOKIE_EXT_USER1;
				nIdentBegin = -1;
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				goto start;
			}
		}

		//  Preprocessor end: > or bracket
		if (dwCookie & COOKIE_PREPROCESSOR)
		{
			if (pszChars[I] == '>')
			{
				dwCookie &= ~COOKIE_PREPROCESSOR;
				nIdentBegin = -1;
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				goto start;
			}
		}

		//  Preprocessor start: &
		if (!(dwCookie & COOKIE_EXT_USER1) && pszChars[I] == '&')
		{
			dwCookie |= COOKIE_USER1;
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
			goto start;
		}

		//  Preprocessor end: ;
		if (dwCookie & COOKIE_USER1)
		{
			if (pszChars[I] == ';')
			{
				dwCookie &= ~COOKIE_USER1;
				bRedefineBlock = TRUE;
				bDecIndex = TRUE;
				nIdentBegin = -1;
				goto start;
			}
		}

	start: // First iteration starts here!
		nPrevI = I++;
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
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
			}
			else if (dwCookie & COOKIE_EXT_USER1)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
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
				// Quotation marks in COOKIE_EXT_USER1 context escape themselves
				if (pszChars[I] == '"' && ((dwCookie & COOKIE_EXT_USER1) || I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			//  Char constant '..'
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			//  Extended comment <!--....-->
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				if (!(dwCookie & COOKIE_EXT_USER1))
				{
					if (I > 1 && pszChars[I] == '>' && pszChars[nPrevI] == '-' && pszChars[nPrevI - 1] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
					}
				}
				goto start;
			}

			if ((dwCookie & COOKIE_EXT_USER1) && pszChars[I] == '\'')
			{
				DEFINE_BLOCK(I, COLORINDEX_COMMENT);
				dwCookie |= COOKIE_COMMENT;
				break;
			}

			//  Extended comment <?....?>
			if (dwCookie & COOKIE_EXT_USER1)
			{
				if (I > 0 && pszChars[I] == '>' && (pszChars[nPrevI] == '?' || pszChars[nPrevI] == '%'))
				{
					dwCookie &= ~COOKIE_EXT_USER1;
					bRedefineBlock = TRUE;
					goto start;
				}
			}

			//  Normal text
			if ((dwCookie & (COOKIE_PREPROCESSOR | COOKIE_EXT_USER1)) && pszChars[I] == '"')
			{
				DEFINE_BLOCK(I, COLORINDEX_STRING);
				dwCookie |= COOKIE_STRING;
				goto start;
			}

			if ((dwCookie & (COOKIE_PREPROCESSOR | COOKIE_EXT_USER1)) && pszChars[I] == '\'')
			{
				if (I == 0 || !xisxdigit(pszChars[nPrevI]))
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_CHAR;
					goto start;
				}
			}

			if (!(dwCookie & COOKIE_EXT_USER1))
			{
				if (!(dwCookie & COOKIE_EXT_USER1) && I < nLength - 3 && pszChars[I] == '<' && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					I += 3;
					dwCookie |= COOKIE_EXT_COMMENT;
					dwCookie &= ~COOKIE_PREPROCESSOR;
					goto start;
				}
			}

			//  User1 start: <?
			if (pszChars[I] == '<' && I < nLength - 1 && (pszChars[I + 1] == '?' || pszChars[I + 1] == '%'))
			{
				DEFINE_BLOCK(I, COLORINDEX_NORMALTEXT);
				dwCookie |= COOKIE_EXT_USER1;
				nIdentBegin = -1;
				goto start;
			}

			if (pBuf == NULL)
				goto start; // No need to extract keywords, so skip rest of loop

			if (xisalnum(pszChars[I]) || pszChars[I] == '.')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				goto start;
			}
		}
		if (nIdentBegin >= 0)
		{
			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				if (IsHtmlKeyword(pszChars + nIdentBegin, I - nIdentBegin) && (pszChars[nIdentBegin - 1] == _T ('<') || pszChars[nIdentBegin - 1] == _T ('/')))
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
			}
			else if (dwCookie & COOKIE_EXT_USER1)
			{
				if (IsAspKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
			}
			else if (dwCookie & COOKIE_USER1)
			{
				if (IsUser2Keyword(pszChars + nIdentBegin, I - nIdentBegin))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_STRING | COOKIE_PREPROCESSOR | COOKIE_EXT_USER1);
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
		else if (_stscanf(text, _T(" static BOOL IsAspKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsAspKeyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			VerifyKeyword<_tcsnicmp>(pfnIsKeyword = NULL, NULL, 0);
	}
	fclose(file);
	assert(count == 1);
	return count;
}
