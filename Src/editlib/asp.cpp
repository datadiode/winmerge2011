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

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_COMMENT          0x00010000UL
#define COOKIE_PREPROCESSOR     0x00020000UL
#define COOKIE_EXT_COMMENT      0x00040000UL
#define COOKIE_STRING           0x00080000UL
#define COOKIE_CHAR             0x00100000UL
#define COOKIE_USER1            0x00200000UL
#define COOKIE_SCRIPT           0x00400000UL

DWORD CCrystalTextView::ParseLineAsp(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	int nScriptBegin = dwCookie & COOKIE_PARSER ? 0 : -1;

	if (nLength == 0)
	{
		if (nScriptBegin >= 0)
		{
			dwCookie = dwCookie & 0xFFFF0000 | ScriptParseProc(dwCookie)(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
		}
		return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_PARSER | COOKIE_PARSER_GLOBAL | COOKIE_SCRIPT | 0xFFFF);
	}

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
	DWORD dwScriptTagCookie = (dwCookie & (COOKIE_PARSER << 4)) >> 4;
	int nIdentBegin = -1;
	pBuf.m_bRecording = dwCookie & COOKIE_PARSER ? NULL : pBuf;
	int nPrevI;
	goto start;
	do
	{
		// Preprocessor start: < or bracket
		if (!(dwCookie & COOKIE_PARSER) && pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
		{
			DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
			DEFINE_BLOCK(I + 1, COLORINDEX_PREPROCESSOR);
			dwCookie |= COOKIE_PREPROCESSOR;
			nIdentBegin = -1;
			goto start;
		}

		// Preprocessor end: > or bracket
		if (dwCookie & COOKIE_PREPROCESSOR)
		{
			if (pszChars[I] == '>')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				bRedefineBlock = TRUE;
				if (dwCookie & COOKIE_SCRIPT)
				{
					pBuf.m_bRecording = NULL;
					dwCookie = dwCookie & ~COOKIE_PARSER | dwScriptTagCookie;
					nScriptBegin = I + 1;
				}
				else
				{
					bDecIndex = TRUE;
				}
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_SCRIPT);
				nIdentBegin = -1;
				goto start;
			}
		}

		//  Preprocessor start: &
		if (!(dwCookie & COOKIE_PARSER) && pszChars[I] == '&')
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
		if (bRedefineBlock && pBuf.m_bRecording)
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
			else if (dwCookie & COOKIE_PARSER)
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

			// Double-quoted text
			if (dwCookie & COOKIE_STRING)
			{
				// Quotation marks in COOKIE_PARSER_BASIC context escape themselves
				if (pszChars[I] == '"' && ((dwCookie & (COOKIE_PARSER | COOKIE_SCRIPT)) == COOKIE_PARSER_BASIC || I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_STRING;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			// Single-quoted text
			if (dwCookie & COOKIE_CHAR)
			{
				if (pszChars[I] == '\'' && (I == 0 || I == 1 && pszChars[nPrevI] != '\\' || I >= 2 && (pszChars[nPrevI] != '\\' || pszChars[nPrevI] == '\\' && pszChars[nPrevI - 1] == '\\')))
				{
					dwCookie &= ~COOKIE_CHAR;
					bRedefineBlock = TRUE;
				}
				goto start;
			}

			// Extended comment <!--....-->
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				switch (dwCookie & COOKIE_PARSER)
				{
				case 0:
					if (I > 1 && pszChars[I] == '>' && pszChars[nPrevI] == '-' && pszChars[nPrevI - 1] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
					}
					break;
				case COOKIE_PARSER_BASIC:
					break;
				default:
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
					break;
				}
				goto start;
			}

			switch (dwCookie & COOKIE_PARSER)
			{
			case 0:
				break;
			case COOKIE_PARSER_BASIC:
				if (pszChars[I] == '\'')
				{
					I = nLength;
					continue; // breaks the loop
				}
				break;
			default:
				if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/' && bWasComment != End)
				{
					I = nLength;
					continue; // breaks the loop
				}
				break;
			}

			if (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_PARSER))
			{
				// Extended comment <?....?>
				if ((dwCookie & COOKIE_PARSER) && I > 0)
				{
					if (pszChars[I] == '>' && (pszChars[nPrevI] == '?' || pszChars[nPrevI] == '%'))
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | ScriptParseProc(dwCookie)(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						DEFINE_BLOCK(nPrevI, COLORINDEX_NORMALTEXT);
						dwCookie &= ~(COOKIE_PARSER | COOKIE_SCRIPT);
						goto start;
					}
					if (pszChars[nPrevI] == '<' && xisequal<_tcsnicmp>(pszChars + I, _T("/script>")))
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | ScriptParseProc(dwCookie)(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						DEFINE_BLOCK(nPrevI, COLORINDEX_OPERATOR);
						DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
						dwCookie &= ~COOKIE_PARSER;
						dwCookie |= COOKIE_PREPROCESSOR;
						goto start;
					}
				}

				// Double-quoted text
				if (pszChars[I] == '"')
				{
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_STRING;
					goto start;
				}

				// Single-quoted text
				if (pszChars[I] == '\'')
				{
					if (I == 0 || !xisxdigit(pszChars[nPrevI]))
					{
						DEFINE_BLOCK(I, COLORINDEX_STRING);
						dwCookie |= COOKIE_CHAR;
						goto start;
					}
				}
			}

			switch (dwCookie & COOKIE_PARSER)
			{
			case 0:
				if (I < nLength - 3 && pszChars[I] == '<' && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					I += 3;
					dwCookie |= COOKIE_EXT_COMMENT;
					dwCookie &= ~COOKIE_PREPROCESSOR;
					goto start;
				}
				break;
			case COOKIE_PARSER_BASIC:
				break;
			default:
				if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/')
				{
					DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
					dwCookie |= COOKIE_EXT_COMMENT;
					bWasComment = Start;
					goto start;
				}
				bWasComment = False;
				break;
			}

			// Script start: <? or <%
			if (pszChars[I] == '<' && I < nLength - 1 && (pszChars[I + 1] == '?' || pszChars[I + 1] == '%'))
			{
				DEFINE_BLOCK(I, COLORINDEX_NORMALTEXT);
				dwCookie &= ~COOKIE_PARSER;
				if (I < nLength - 2 && pszChars[I + 2] == '@')
				{
					dwCookie |= COOKIE_SCRIPT;
				}
				else
				{
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 2;
				}
				dwCookie |= (dwCookie & (COOKIE_PARSER << 4)) >> 4;
				nIdentBegin = -1;
				goto start;
			}

			if (xisalnum(pszChars[I]) || pszChars[I] == '.')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				goto start;
			}
		}
		if (nIdentBegin >= 0)
		{
			LPCTSTR const pchIdent = pszChars + nIdentBegin;
			int const cchIdent = I - nIdentBegin;
			if (dwCookie & COOKIE_PREPROCESSOR)
			{
				if (pchIdent[-1] == '<' || pchIdent[-1] == '/')
				{
					if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("script")))
					{
						if (pchIdent[-1] == '<')
							dwCookie |= COOKIE_SCRIPT;
						dwScriptTagCookie = (dwCookie & (COOKIE_PARSER << 4)) >> 4;
					}
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if (IsHtmlKeyword(pchIdent, cchIdent))
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
					}
				}
				else if (!pBuf.m_bRecording)
				{
					// No need to extract keywords, so skip rest of loop
				}
				else if (IsNumeric(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
				}
				else if (IsUser1Keyword(pchIdent, cchIdent))
				{
					if (dwCookie & COOKIE_SCRIPT)
					{
						TCHAR lang[32];
						if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("type")) ?
							_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z]/%31[a-zA-Z#]"), lang, lang, lang) == 3 :
							xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("language")) &&
							_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z#]"), lang, lang) == 2)
						{
							dwScriptTagCookie = ScriptCookie(lang);
						}
					}
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
				}
			}
			else if (dwCookie & COOKIE_PARSER)
			{
				if (dwCookie & COOKIE_SCRIPT)
				{
					TCHAR lang[32];
					if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("language")) &&
						_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z#]"), lang, lang) == 2)
					{
						dwCookie = dwCookie & ~(COOKIE_PARSER << 4) | (ScriptCookie(lang) << 4);
					}
				}
			}
			else if (!pBuf.m_bRecording)
			{
				// No need to extract keywords, so skip rest of loop
			}
			else if (dwCookie & COOKIE_USER1)
			{
				if (IsUser2Keyword(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
			bRedefineBlock = TRUE;
			bDecIndex = TRUE;
			nIdentBegin = -1;
		}
	} while (I < nLength);

	if (dwCookie & COOKIE_PARSER)
	{
		pBuf.m_bRecording = pBuf;
		if (nScriptBegin >= 0)
		{
			dwCookie = dwCookie & 0xFFFF0000 | ScriptParseProc(dwCookie)(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
		}
	}

	dwCookie &= (COOKIE_EXT_COMMENT | COOKIE_STRING | COOKIE_PREPROCESSOR | COOKIE_PARSER | COOKIE_PARSER_GLOBAL | COOKIE_SCRIPT | 0xFFFF);
	return dwCookie;
}
