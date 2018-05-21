///////////////////////////////////////////////////////////////////////////
// razor.cpp
// Modified from prior art code. Original header comment reproduced below.
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

using HtmlKeywords::IsHtmlTagName;
using HtmlKeywords::IsHtmlAttrName;
using HtmlKeywords::IsDtdTagName;
using HtmlKeywords::IsDtdAttrName;
using HtmlKeywords::IsEntityName;

// Keywords which do nothing about parsing logic
static BOOL IsTrivialKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszTrivialKeywordList[] =
	{
		_T("section"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszTrivialKeywordList);
}

// Keywords which pass rest of line to subordinate parser
static BOOL IsSpecialKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSpecialKeywordList[] =
	{
		_T("model"),
		_T("using"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszSpecialKeywordList);
}

// Keywords which enter snorkel mode when found at top nesting level
static BOOL IsSnorkelKeyword(LPCTSTR pszChars, int nLength)
{
	static LPCTSTR const s_apszSnorkelKeywordList[] =
	{
		_T("do"),
		_T("for"),
		_T("foreach"),
		_T("helper"),
		_T("if"),
		_T("lock"),
		_T("switch"),
		_T("try"),
		_T("using"),
		_T("while"),
	};
	return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszSnorkelKeywordList);
}

#define DEFINE_BLOCK pBuf.DefineBlock

#define COOKIE_DTD              0x00010000UL
#define COOKIE_PREPROCESSOR     0x00020000UL
#define COOKIE_STRING           0x000C0000UL
#define COOKIE_STRING_SINGLE    0x00040000UL
#define COOKIE_STRING_DOUBLE    0x00080000UL
#define COOKIE_STRING_REGEXP    0x000C0000UL
#define COOKIE_EXT_COMMENT      0x00100000UL
#define COOKIE_REJECT_REGEXP    0x00200000UL
#define COOKIE_ASP              0x00400000UL
#define COOKIE_SCRIPT           0x00800000UL
#define COOKIE_RAZOR_NESTING    0x0000FF00UL

#define ODDNESS_AT              0x00000001UL

#define SNORKEL(X) (0 - (X) & (X))
#define GO_DOWN(C, X) ((C) & ~(X) | (C) - (X) & (X))
#define COME_UP(C, X) ((C) & ~(X) | (C) + (X) & (X))

static DWORD ParseProcCookie(DWORD dwCookie)
{
	if ((dwCookie & COOKIE_RAZOR_NESTING) >= SNORKEL(COOKIE_RAZOR_NESTING))
		return COOKIE_PARSER_CSHARP;
	return dwCookie & COOKIE_PARSER;
}

DWORD CCrystalTextView::ParseLineRazor(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	int nScriptBegin = ((dwCookie & COOKIE_PARSER) ||
		(dwCookie & COOKIE_RAZOR_NESTING) && !(dwCookie & COOKIE_SCRIPT)) &&
		!(dwCookie & (COOKIE_ASP | COOKIE_STRING)) ? 0 : -1;

	if (nLength == 0)
	{
		if (nScriptBegin >= 0)
		{
			DWORD const dwParser = ParseProcCookie(dwCookie);
			dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
		}
		return dwCookie;
	}

	int const nHtmlTagNameMask =
		(m_dwFlags & (SRCOPT_HTML4_LEXIS | SRCOPT_HTML5_LEXIS)) == SRCOPT_HTML4_LEXIS ? COMMON_LEXIS | HTML4_LEXIS :
		(m_dwFlags & (SRCOPT_HTML4_LEXIS | SRCOPT_HTML5_LEXIS)) == SRCOPT_HTML5_LEXIS ? COMMON_LEXIS | HTML5_LEXIS :
		COMMON_LEXIS | HTML4_LEXIS | HTML5_LEXIS;

	BOOL bRedefineBlock = TRUE;
	BOOL bDecIndex = FALSE;
	enum { False, Start, End } bWasComment = False;
	DWORD dwOddness = 0;
	DWORD dwScriptTagCookie = dwCookie & COOKIE_SCRIPT ? dwCookie & COOKIE_PARSER : 0;
	int nIdentBegin = -1;
	int nInlineNesting = 0;
	pBuf.m_bRecording = nScriptBegin != -1 ? NULL : pBuf;
	do
	{
		int const nDecIndex = I++;
		if (bRedefineBlock && pBuf.m_bRecording)
		{
			int const nPos = bDecIndex ? nDecIndex : I;
			bRedefineBlock = FALSE;
			bDecIndex = FALSE;
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_COMMENT);
			}
			else if (dwCookie & COOKIE_STRING)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_STRING);
			}
			else if (dwCookie & COOKIE_PREPROCESSOR)
			{
				DEFINE_BLOCK(nPos, COLORINDEX_PREPROCESSOR);
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
		if (nInlineNesting && !(dwCookie & (COOKIE_STRING | COOKIE_EXT_COMMENT)))
		{
			if (nInlineNesting == 1 && !xisalnum(pszChars[I]))
			{
				LPCTSTR const pchIdent = pszChars + nScriptBegin;
				int const cchIdent = I - nScriptBegin;
				if (IsTrivialKeyword(pchIdent, cchIdent))
				{
					pBuf.m_bRecording = pBuf;
					DEFINE_BLOCK(nScriptBegin, COLORINDEX_USER3);
					nScriptBegin = -1;
					nInlineNesting = 0;
					dwCookie = dwCookie & ~COOKIE_PARSER | dwScriptTagCookie & COOKIE_PARSER;
					continue;
				}
				if (IsSpecialKeyword(pchIdent, cchIdent))
				{
					// Beware of @using ... vs. @using (...)
					int nPos = I;
					while (xisspace(pszChars[nPos]))
						++nPos;
					if (pszChars[nPos] != '(')
					{
						pBuf.m_bRecording = pBuf;
						DEFINE_BLOCK(nScriptBegin, COLORINDEX_USER3);
						I = nLength;
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nPos - 1, pBuf);
						dwCookie = dwCookie & ~COOKIE_PARSER | dwScriptTagCookie & COOKIE_PARSER;
						continue;
					}
				}
				if (IsSnorkelKeyword(pchIdent, cchIdent))
				{
					// Leave keyword coloring to subordinate parser
					pBuf.m_bRecording = NULL;
					nInlineNesting = 0;
					if (!(dwCookie & COOKIE_RAZOR_NESTING))
						dwCookie |= SNORKEL(COOKIE_RAZOR_NESTING);
					dwCookie = dwCookie & ~COOKIE_PARSER | dwScriptTagCookie & COOKIE_PARSER;
					continue;
				}
				// Don't prematurely break @await'ed implicit expressions
				if (xisequal<_tcsncmp>(pchIdent, cchIdent, _T("await")))
				{
					while (xisspace(pszChars[I]))
						++I;
				}
			}
		}
		// Can be bigger than length if there is binary data
		// See bug #1474782 Crash when comparing SQL with with binary data
		if (I < nLength)
		{
			if (nInlineNesting && !(dwCookie & (COOKIE_STRING | COOKIE_EXT_COMMENT)))
			{
				switch (TCHAR ch = pszChars[I])
				{
				case '(':
				case '[':
					++nInlineNesting;
					continue;
				case ')':
				case ']':
					if (--nInlineNesting == 1)
						++I; // Leave coloring of final closing bracket to subordinate parser
					// fall through
				default:
					if (nInlineNesting > 1 || xisalnum(ch) || ch == '.')
						continue;
					pBuf.m_bRecording = pBuf;
					if (nScriptBegin >= 0)
					{
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
						nScriptBegin = -1;
					}
					dwCookie = dwCookie & ~(COOKIE_PARSER | COOKIE_STRING | COOKIE_REJECT_REGEXP) | dwScriptTagCookie & (COOKIE_PARSER | COOKIE_STRING);
					dwScriptTagCookie &= ~COOKIE_STRING;
					if (dwCookie & COOKIE_STRING)
					{
						DEFINE_BLOCK(I, COLORINDEX_STRING);
					}
					else if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
					{
						dwCookie &= ~COOKIE_REJECT_REGEXP;
						if ((dwCookie & COOKIE_PARSER) || (dwCookie & COOKIE_RAZOR_NESTING) && !(dwCookie & COOKIE_SCRIPT))
						{
							pBuf.m_bRecording = NULL;
							nScriptBegin = I;
						}
					}
					nInlineNesting = 0;
					break;
				}
			}

			if (pszChars[I] != '@')
			{
				dwOddness &= ~ODDNESS_AT;
			}
			else if (!(dwCookie & COOKIE_EXT_COMMENT) && !(I > 0 && xisalnum(pszChars[I - 1]) && xisalnum(pszChars[I + 1])))
			{
				pBuf.m_bRecording = pBuf;
				if (nScriptBegin >= 0)
				{
					DWORD const dwParser = ParseProcCookie(dwCookie);
					dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
					nScriptBegin = -1;
				}
				if ((dwCookie & COOKIE_RAZOR_NESTING) == SNORKEL(COOKIE_RAZOR_NESTING))
					dwCookie &= ~COOKIE_RAZOR_NESTING;
				if (!(dwOddness & ODDNESS_AT) && (pszChars[I + 1] == '(' || xisalnum(pszChars[I + 1])))
				{
					DEFINE_BLOCK(I, COLORINDEX_USER3);
					dwScriptTagCookie = dwCookie & (COOKIE_PARSER | COOKIE_STRING);
					dwCookie &= ~(COOKIE_PARSER | COOKIE_STRING);
					dwCookie |= COOKIE_PARSER_CSHARP;
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 1;
					nInlineNesting = 1;
					continue;
				}
				dwOddness ^= ODDNESS_AT;
			}

			switch (dwCookie & COOKIE_STRING)
			{
			case COOKIE_STRING_SINGLE:
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_STRING_SINGLE;
					bRedefineBlock = TRUE;
					if (dwCookie & COOKIE_ASP)
						continue;
					switch (ParseProcCookie(dwCookie))
					{
					case 0:
					case COOKIE_PARSER_BASIC:
					case COOKIE_PARSER_VBSCRIPT:
						break;
					default:
						int nPrevI = I;
						while (nPrevI && pszChars[--nPrevI] == '\\')
						{
							dwCookie ^= COOKIE_STRING_SINGLE;
							bRedefineBlock ^= TRUE;
						}
						break;
					}
					if (!(dwCookie & COOKIE_STRING) && (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER)))
					{
						if (dwScriptTagCookie & COOKIE_PARSER)
						{
							dwCookie &= ~(COOKIE_PARSER | 0xFF);
							dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
						}
						if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
						{
							pBuf.m_bRecording = NULL;
							nScriptBegin = I + 1;
						}
					}
				}
				continue;
			case COOKIE_STRING_DOUBLE:
				if (pszChars[I] == '"')
				{
					dwCookie &= ~COOKIE_STRING_DOUBLE;
					bRedefineBlock = TRUE;
					if (dwCookie & COOKIE_ASP)
						continue;
					switch (ParseProcCookie(dwCookie))
					{
					case 0:
					case COOKIE_PARSER_BASIC:
					case COOKIE_PARSER_VBSCRIPT:
						break;
					default:
						int nPrevI = I;
						while (nPrevI && pszChars[--nPrevI] == '\\')
						{
							dwCookie ^= COOKIE_STRING_DOUBLE;
							bRedefineBlock ^= TRUE;
						}
						break;
					}
					if (!(dwCookie & COOKIE_STRING) && (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER)))
					{
						if (dwScriptTagCookie & COOKIE_PARSER)
						{
							dwCookie &= ~(COOKIE_PARSER | 0xFF);
							dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
						}
						if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
						{
							pBuf.m_bRecording = NULL;
							nScriptBegin = I + 1;
						}
					}
				}
				continue;
			case COOKIE_STRING_REGEXP:
				switch (ParseProcCookie(dwCookie))
				{
				case 0:
				case COOKIE_PARSER_CSS:
				case COOKIE_PARSER_BASIC:
				case COOKIE_PARSER_VBSCRIPT:
					break;
				case COOKIE_PARSER_JSCRIPT:
					if (pszChars[I] == '/')
					{
						dwCookie &= ~COOKIE_STRING_REGEXP;
						bRedefineBlock = TRUE;
						int nPrevI = I;
						while (nPrevI && pszChars[--nPrevI] == '\\')
						{
							dwCookie ^= COOKIE_STRING_REGEXP;
							bRedefineBlock ^= TRUE;
						}
						if (!(dwCookie & COOKIE_STRING_REGEXP))
						{
							int nNextI = I;
							while (++nNextI < nLength && xisalnum(pszChars[nNextI]))
								I = nNextI;
							bWasComment = End;
						}
					}
					break;
				default:
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
					break;
				}
				if (!(dwCookie & COOKIE_STRING) && (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER)))
				{
					if (dwScriptTagCookie & COOKIE_PARSER)
					{
						dwCookie &= ~(COOKIE_PARSER | 0xFF);
						dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
					}
					if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
					{
						pBuf.m_bRecording = NULL;
						nScriptBegin = I + 1;
					}
				}
				continue;
			}

			// Extended comment <!--....-->
			if (dwCookie & COOKIE_EXT_COMMENT)
			{
				switch (dwCookie & (COOKIE_DTD | COOKIE_ASP | COOKIE_SCRIPT) | ParseProcCookie(dwCookie))
				{
				case 0:
				case COOKIE_SCRIPT | COOKIE_PARSER_BASIC:
				case COOKIE_SCRIPT | COOKIE_PARSER_CSHARP:
					if (I > 1 && (nIdentBegin == -1 || I - nIdentBegin >= 6) && pszChars[I] == '>' && pszChars[I - 1] == '-' && pszChars[I - 2] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_DTD:
					if (I > 0 && (nIdentBegin == -1 || I - nIdentBegin >= 4) && pszChars[I] == '-' && pszChars[I - 1] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_ASP:
				case COOKIE_SCRIPT | COOKIE_PARSER_VBSCRIPT:
					break;
				default:
					if (I > 0 && pszChars[I] == '/' && pszChars[I - 1] == '*' && bWasComment != Start)
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
				continue;
			}

			switch (ParseProcCookie(dwCookie))
			{
			case 0:
				break;
			case COOKIE_PARSER_BASIC:
			case COOKIE_PARSER_VBSCRIPT:
				if (pszChars[I] == '\'')
				{
					I = nLength;
					continue; // breaks the loop
				}
				break;
			default:
				if (I > 0 && pszChars[I] == '/' && pszChars[I - 1] == '/' && bWasComment != End)
				{
					I = nLength;
					continue; // breaks the loop
				}
				break;
			}

			if (dwCookie & COOKIE_RAZOR_NESTING)
			{
				switch (pszChars[I])
				{
				case '{':
					dwCookie = GO_DOWN(dwCookie, COOKIE_RAZOR_NESTING);
					pBuf.m_bRecording = pBuf;
					if (nScriptBegin >= 0)
					{
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
						nScriptBegin = -1;
					}
					DEFINE_BLOCK(I, COLORINDEX_USER3);
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 1;
					dwCookie &= ~COOKIE_REJECT_REGEXP;
					nIdentBegin = -1;
					continue;
				case '}':
					pBuf.m_bRecording = pBuf;
					if (nScriptBegin >= 0)
					{
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I - 1, nScriptBegin - 1, pBuf);
						nScriptBegin = -1;
					}
					dwCookie = COME_UP(dwCookie, COOKIE_RAZOR_NESTING);
					dwCookie &= ~COOKIE_REJECT_REGEXP;
					if ((dwCookie & COOKIE_RAZOR_NESTING) == 0)
					{
						DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
						bRedefineBlock = TRUE;
						bDecIndex = FALSE;
						continue;
					}
					// Start a new script fragment
					DEFINE_BLOCK(I, COLORINDEX_USER3);
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 1;
					continue;
				case '<':
					if ((dwCookie & COOKIE_RAZOR_NESTING) == SNORKEL(COOKIE_RAZOR_NESTING) && !(dwCookie & COOKIE_REJECT_REGEXP))
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							DWORD const dwParser = ParseProcCookie(dwCookie);
							dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						dwCookie &= ~COOKIE_RAZOR_NESTING;
					}
					break;
				}
			}

			if (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_RAZOR_NESTING | COOKIE_PARSER))
			{
				if (I > 0 && pszChars[I] == '/' && pszChars[I - 1] == '<')
				{
					pBuf.m_bRecording = pBuf;
					if (nScriptBegin >= 0)
					{
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I - 1, nScriptBegin - 1, pBuf);
						nScriptBegin = -1;
					}
					DEFINE_BLOCK(I - 1, COLORINDEX_OPERATOR);
					DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
					dwCookie &= ~(COOKIE_PARSER | COOKIE_SCRIPT);
					dwCookie |= COOKIE_PREPROCESSOR;
					continue;
				}

				if (pszChars[I] == '[' || pszChars[I] == ']')
				{
					dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD);
				}

				if (!((dwCookie & COOKIE_RAZOR_NESTING) && (dwCookie & COOKIE_SCRIPT)) || (dwCookie & COOKIE_ASP))
				{
					// Double-quoted text
					if (pszChars[I] == '"')
					{
						if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
						{
							pBuf.m_bRecording = pBuf;
							if (nScriptBegin >= 0)
							{
								DWORD const dwParser = ParseProcCookie(dwCookie);
								dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
								nScriptBegin = -1;
							}
						}
						DEFINE_BLOCK(I, COLORINDEX_STRING);
						dwCookie |= (dwCookie & COOKIE_RAZOR_NESTING) && I > 0 && pszChars[I - 1] == '@' ? COOKIE_STRING_REGEXP : COOKIE_STRING_DOUBLE;
						continue;
					}

					// Single-quoted text
					if (pszChars[I] == '\'' && (I == 0 || !xisxdigit(pszChars[I - 1])))
					{
						if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
						{
							pBuf.m_bRecording = pBuf;
							if (nScriptBegin >= 0)
							{
								DWORD const dwParser = ParseProcCookie(dwCookie);
								dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
								nScriptBegin = -1;
							}
						}
						DEFINE_BLOCK(I, COLORINDEX_STRING);
						dwCookie |= COOKIE_STRING_SINGLE;
						continue;
					}
				}
			}

			switch (dwCookie & (COOKIE_DTD | COOKIE_ASP | COOKIE_SCRIPT) | ParseProcCookie(dwCookie))
			{
			case 0:
			case COOKIE_SCRIPT | COOKIE_PARSER_BASIC:
			case COOKIE_SCRIPT | COOKIE_PARSER_CSHARP:
				if (I < nLength - 3 && pszChars[I] == '<' && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					nIdentBegin = I;
					I += 3;
					dwCookie |= COOKIE_EXT_COMMENT;
					dwCookie &= ~COOKIE_PREPROCESSOR;
					continue;
				}
				break;
			case COOKIE_DTD:
				if (I < nLength - 1 && pszChars[I] == '-' && pszChars[I + 1] == '-')
				{
					DEFINE_BLOCK(I, COLORINDEX_COMMENT);
					nIdentBegin = I;
					I += 1;
					dwCookie |= COOKIE_EXT_COMMENT;
					continue;
				}
				break;
			case COOKIE_SCRIPT | COOKIE_PARSER_VBSCRIPT:
				break;
			case COOKIE_SCRIPT | COOKIE_PARSER_JSCRIPT:
				if (pszChars[I] == '/' && I < nLength - 1 && pszChars[I + 1] != '/' && pszChars[I + 1] != '*')
				{
					if (!(dwCookie & COOKIE_REJECT_REGEXP))
					{
						if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
						{
							pBuf.m_bRecording = pBuf;
							if (nScriptBegin >= 0)
							{
								DWORD const dwParser = ParseProcCookie(dwCookie);
								dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
								nScriptBegin = -1;
							}
						}
						DEFINE_BLOCK(I, dwCookie & COOKIE_PREPROCESSOR ? COLORINDEX_PREPROCESSOR : COLORINDEX_STRING);
						dwCookie |= COOKIE_STRING_REGEXP | COOKIE_REJECT_REGEXP;
						continue;
					}
				}
				// fall through
			default:
				if (I > 0 && pszChars[I] == '*' && pszChars[I - 1] == '/')
				{
					DEFINE_BLOCK(I - 1, COLORINDEX_COMMENT);
					dwCookie |= COOKIE_EXT_COMMENT;
					bWasComment = Start;
					continue;
				}
				bWasComment = False;
				break;
			}

			if (xisalnum(pszChars[I]) || pszChars[I] == '.' || pszChars[I] == '-' || pszChars[I] == '!' || pszChars[I] == '#')
			{
				if (nIdentBegin == -1)
					nIdentBegin = I;
				dwCookie |= COOKIE_REJECT_REGEXP;
				continue;
			}

			if (pszChars[I] == '@')
			{
				if ((dwOddness & ODDNESS_AT) && (pszChars[I + 1] == '{' || xisalnum(pszChars[I + 1])))
				{
					if ((dwCookie & COOKIE_RAZOR_NESTING) == SNORKEL(COOKIE_RAZOR_NESTING))
						dwCookie &= ~COOKIE_RAZOR_NESTING;
					pBuf.m_bRecording = pBuf;
					if (nScriptBegin >= 0)
					{
						DWORD const dwParser = ParseProcCookie(dwCookie);
						dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
						nScriptBegin = -1;
					}
					DEFINE_BLOCK(I, COLORINDEX_USER3);
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 1;
					if ((dwCookie & COOKIE_RAZOR_NESTING) > SNORKEL(COOKIE_RAZOR_NESTING))
						dwCookie &= ~COOKIE_SCRIPT;
					else
						dwCookie |= SNORKEL(COOKIE_RAZOR_NESTING);
				}
			}
		}
		if (nIdentBegin >= 0 && nScriptBegin == -1)
		{
			LPCTSTR const pchIdent = pszChars + nIdentBegin;
			int const cchIdent = I - nIdentBegin;
			if (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_RAZOR_NESTING))
			{
				if (pchIdent > pszChars && (pchIdent[-1] == '<' || pchIdent[-1] == '/'))
				{
					if (pchIdent[-1] == '<')
					{
						if (*pchIdent == '!')
						{
							dwCookie |= COOKIE_DTD;
						}
						else if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("script")))
						{
							dwCookie |= COOKIE_ASP | COOKIE_SCRIPT;
							dwScriptTagCookie = COOKIE_PARSER_JSCRIPT;
						}
						else if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("style")))
						{
							dwCookie |= COOKIE_ASP | COOKIE_SCRIPT;
							dwScriptTagCookie = COOKIE_PARSER_CSS;
						}
						else if ((dwCookie & COOKIE_RAZOR_NESTING) > SNORKEL(COOKIE_RAZOR_NESTING))
						{
							dwCookie |= COOKIE_ASP | COOKIE_SCRIPT;
							dwScriptTagCookie = 0;
						}
						else
						{
							dwCookie &= ~COOKIE_RAZOR_NESTING;
						}
					}
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if (dwCookie & COOKIE_DTD ?
						IsDtdTagName(pchIdent, cchIdent) :
						IsHtmlTagName(pchIdent, cchIdent) & nHtmlTagNameMask)
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_KEYWORD);
					}
					else if ((dwCookie & COOKIE_RAZOR_NESTING) > SNORKEL(COOKIE_RAZOR_NESTING))
					{
						if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("text")))
						{
							DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER3);
						}
					}
				}
				else
				{
					if (dwCookie & COOKIE_ASP)
					{
						TCHAR lang[32];
						if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("type")) ?
							_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z]/%31[a-zA-Z#\\-]"), lang, lang, lang) == 3 :
							xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("language")) &&
							_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z#\\-]"), lang, lang) == 2)
						{
							dwScriptTagCookie = ScriptCookie(lang);
							if (dwScriptTagCookie == 0)
								dwCookie &= ~COOKIE_ASP; // Render element content as HTML
						}
					}
					if (!pBuf.m_bRecording)
					{
						// No need to extract keywords, so skip rest of loop
					}
					else if (IsNumeric(pchIdent, cchIdent))
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_NUMBER);
					}
					else if ((dwCookie & COOKIE_DTD ? IsDtdAttrName : IsHtmlAttrName)(pchIdent, cchIdent))
					{
						DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER1);
					}
				}
			}
			else if (!pBuf.m_bRecording)
			{
				// No need to extract keywords, so skip rest of loop
			}
			else if (pchIdent > pszChars && pchIdent[-1] == '&')
			{
				if (IsEntityName(pchIdent, cchIdent))
				{
					DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER2);
				}
			}
		}
		nIdentBegin = -1;

		bRedefineBlock = TRUE;
		bDecIndex = TRUE;

		switch (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_PARSER))
		{
		case 0:
			// Preprocessor start: < or bracket
			// COOKIE_REJECT_REGEXP helps distinguish C# generics from tags.
			if (pszChars[I] == '<' && !(
				(dwCookie & COOKIE_RAZOR_NESTING) && !(dwCookie & COOKIE_SCRIPT) ?
				!xisalnum(pszChars[I + 1]) || dwCookie & COOKIE_REJECT_REGEXP :
				pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
			{
				pBuf.m_bRecording = pBuf;
				if (nScriptBegin >= 0)
				{
					DWORD const dwParser = ParseProcCookie(dwCookie);
					dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
					nScriptBegin = -1;
				}
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				bDecIndex = FALSE;
				dwCookie |= COOKIE_PREPROCESSOR;
			}
			break;
		case COOKIE_PREPROCESSOR:
			// Preprocessor end: > or bracket
			if (pszChars[I] == '>')
			{
				DEFINE_BLOCK(I, COLORINDEX_OPERATOR);
				if (I > 0 && pszChars[I - 1] == '/')
				{
					dwCookie &= ~(COOKIE_SCRIPT | COOKIE_PARSER);
				}
				else if (dwCookie & COOKIE_ASP)
				{
					dwCookie |= dwScriptTagCookie;
					pBuf.m_bRecording = pBuf;
				}
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD | COOKIE_ASP);
				if ((dwCookie & COOKIE_PARSER) ||
					(dwCookie & COOKIE_RAZOR_NESTING) && !(dwCookie & COOKIE_SCRIPT))
				{
					pBuf.m_bRecording = NULL;
					nScriptBegin = I + 1;
				}
			}
			break;
		}
		if ((dwCookie & (COOKIE_STRING | COOKIE_EXT_COMMENT)) == 0 && !xisspace(pszChars[I]))
		{
			// Preserve COOKIE_REJECT_REGEXP across inline comments
			switch (pszChars[I])
			{
			case '/': case '*':
				switch (pszChars[I + 1])
				{
				case '/': case '*':
					continue;
				}
			}
			dwCookie &= ~COOKIE_REJECT_REGEXP;
			switch (TCHAR peer = pszChars[I])
			{
			case '+':
			case '-':
				if (I == 0 || pszChars[I - 1] != peer)
					break;
				// fall through
			case ')':
			case ']':
				dwCookie |= COOKIE_REJECT_REGEXP;
				break;
			}
		}
	} while (I < nLength);

	if (dwCookie & (COOKIE_RAZOR_NESTING | COOKIE_PARSER))
	{
		pBuf.m_bRecording = pBuf;
		if (nScriptBegin >= 0)
		{
			DWORD const dwParser = ParseProcCookie(dwCookie);
			dwCookie = dwCookie & 0xFFFFFF00 | (this->*ScriptParseProc(dwParser))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
		}
	}

	return dwCookie;
}
