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

DWORD CCrystalTextView::ParseLineAsp(DWORD dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	int nScriptBegin = (dwCookie & COOKIE_PARSER) && !(dwCookie & (COOKIE_ASP | COOKIE_STRING)) ? 0 : -1;

	if (nLength == 0)
	{
		if (nScriptBegin >= 0)
		{
			dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
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
	DWORD dwScriptTagCookie = dwCookie & COOKIE_SCRIPT ? dwCookie & COOKIE_PARSER : 0;
	int nIdentBegin = -1;
	pBuf.m_bRecording = nScriptBegin != -1 ? NULL : pBuf;
	do
	{
		int const nPrevI = I++;
		if (bRedefineBlock && pBuf.m_bRecording)
		{
			int const nPos = bDecIndex ? nPrevI : I;
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
			// Script start: <? or <%
			if (pszChars[I] == '<' && (pszChars[I + 1] == '?' || pszChars[I + 1] == '%'))
			{
				pBuf.m_bRecording = pBuf;
				DEFINE_BLOCK(I, COLORINDEX_USER3);
				bRedefineBlock = TRUE;
				if (pszChars[I + 2] == '@') // directive expression
				{
					I += 3;
					dwCookie |= COOKIE_ASP | COOKIE_PARSER;
				}
				else if (pszChars[I + 2] == '-' && pszChars[I + 3] == '-') // server-side comment
				{
					dwCookie |= COOKIE_ASP | COOKIE_EXT_COMMENT;
				}
				else
				{
					I += 2;
					pBuf.m_bRecording = NULL;
					nScriptBegin = I;
					switch (dwCookie & COOKIE_PARSER_GLOBAL)
					{
					case SRCOPT_COOKIE(COOKIE_PARSER_PHP):
						if (LPCTSTR pScriptBegin = xisequal<_tcsnicmp>(pszChars + nScriptBegin, _T("php")))
							nScriptBegin = static_cast<int>(pScriptBegin - pszChars);
						break;
					}
					if (xisalnum(pszChars[nScriptBegin]))
					{
						dwCookie |= COOKIE_PARSER; // unknown coding language
					}
					else
					{
						dwCookie &= ~COOKIE_PARSER;
						dwCookie |= (dwCookie & COOKIE_PARSER_GLOBAL) >> 4;
						switch (pszChars[nScriptBegin])
						{
						case '$': // expression builder
							dwCookie |= COOKIE_PARSER; // unknown coding language
							// fall through
						case '=': // displaying expression
						case ':': // displaying expression using HTML encoding
						case '#': // data-binding expression
							++nScriptBegin;
							break;
						}
					}
					if (dwCookie & COOKIE_SCRIPT)
						dwScriptTagCookie |= dwCookie & COOKIE_STRING;
					else
						dwScriptTagCookie = dwCookie & (COOKIE_STRING | COOKIE_PARSER);
					dwCookie &= ~COOKIE_STRING;
				}
				nIdentBegin = -1;
				continue;
			}

			switch (dwCookie & COOKIE_PARSER_GLOBAL)
			{
			case SRCOPT_COOKIE(COOKIE_PARSER_MWSL):
				if (pszChars[I] == ':')
				{
					// Script start: :=
					if (pszChars[I + 1] == '=')
					{
						pBuf.m_bRecording = pBuf;
						DEFINE_BLOCK(I, COLORINDEX_USER3);
						pBuf.m_bRecording = NULL;
						nScriptBegin = I + 2;
						dwCookie &= ~COOKIE_PARSER;
						dwCookie |= COOKIE_PARSER_MWSL;
						if (dwCookie & COOKIE_SCRIPT)
							dwScriptTagCookie |= dwCookie & COOKIE_STRING;
						else
							dwScriptTagCookie = dwCookie & (COOKIE_STRING | COOKIE_PARSER);
						dwCookie &= ~COOKIE_STRING;
						nIdentBegin = -1;
						continue;
					}
					// Script end: :
					if ((dwCookie & COOKIE_PARSER) == COOKIE_PARSER_MWSL &&
						(!(dwCookie & COOKIE_SCRIPT) || (dwScriptTagCookie & COOKIE_PARSER) != COOKIE_PARSER_MWSL))
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						DEFINE_BLOCK(I, COLORINDEX_USER3);
						dwCookie &= ~COOKIE_PARSER;
						dwCookie |= dwScriptTagCookie & COOKIE_STRING;
						if (!(dwCookie & COOKIE_ASP))
							dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
						bRedefineBlock = TRUE;
						bDecIndex = FALSE;
						continue;
					}
				}
				break;
			}

			switch (dwCookie & COOKIE_STRING)
			{
			case COOKIE_STRING_SINGLE:
				if (pszChars[I] == '\'')
				{
					dwCookie &= ~COOKIE_STRING_SINGLE;
					bRedefineBlock = TRUE;
					if ((dwCookie & COOKIE_ASP) && !(dwScriptTagCookie & COOKIE_STRING))
						continue;
					switch (dwCookie)
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
					if (!(dwCookie & COOKIE_STRING) && (dwCookie & COOKIE_PARSER_GLOBAL))
					{
						if (DWORD dwParser = dwCookie & COOKIE_ASP ?
							(dwCookie & COOKIE_PARSER_GLOBAL >> 4) : dwScriptTagCookie & COOKIE_PARSER)
						{
							dwCookie &= ~(COOKIE_PARSER | 0xFFFF);
							dwCookie |= dwParser;
						}
						if (dwCookie & COOKIE_PARSER)
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
					if ((dwCookie & COOKIE_ASP) && !(dwScriptTagCookie & COOKIE_STRING))
						continue;
					switch (dwCookie)
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
					if (!(dwCookie & COOKIE_STRING) && (dwCookie & COOKIE_PARSER_GLOBAL))
					{
						if (DWORD dwParser = dwCookie & COOKIE_ASP ?
							(dwCookie & COOKIE_PARSER_GLOBAL >> 4) : dwScriptTagCookie & COOKIE_PARSER)
						{
							dwCookie &= ~(COOKIE_PARSER | 0xFFFF);
							dwCookie |= dwParser;
						}
						if (dwCookie & COOKIE_PARSER)
						{
							pBuf.m_bRecording = NULL;
							nScriptBegin = I + 1;
						}
					}
				}
				continue;
			case COOKIE_STRING_REGEXP:
				switch (dwCookie & COOKIE_PARSER)
				{
				case COOKIE_PARSER_CSHARP:
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
				default:
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
				}
				if (!(dwCookie & COOKIE_STRING) && (dwCookie & COOKIE_PARSER_GLOBAL))
				{
					if (dwScriptTagCookie & COOKIE_PARSER)
					{
						dwCookie &= ~(COOKIE_PARSER | 0xFFFF);
						dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
					}
					if (dwCookie & COOKIE_PARSER)
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
				switch (dwCookie & (COOKIE_PARSER | COOKIE_DTD | COOKIE_ASP))
				{
				case 0:
					if (I > 1 && (nIdentBegin == -1 || I - nIdentBegin >= 6) && pszChars[I] == '>' && pszChars[nPrevI] == '-' && pszChars[nPrevI - 1] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_DTD:
					if (I > 0 && (nIdentBegin == -1 || I - nIdentBegin >= 4) && pszChars[I] == '-' && pszChars[nPrevI] == '-')
					{
						dwCookie &= ~COOKIE_EXT_COMMENT;
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_ASP:
					if (I > 2 && (nIdentBegin == -1 || I - nIdentBegin >= 5) && pszChars[I] == '>' &&
						pszChars[nPrevI] == '%' && pszChars[nPrevI - 1] == '-' && pszChars[nPrevI - 2] == '-')
					{
						dwCookie &= ~(COOKIE_EXT_COMMENT | COOKIE_ASP);
						bRedefineBlock = TRUE;
						nIdentBegin = -1;
					}
					break;
				case COOKIE_PARSER_BASIC:
				case COOKIE_PARSER_VBSCRIPT:
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
				continue;
			}

			switch (dwCookie & COOKIE_PARSER)
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
							dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						DEFINE_BLOCK(nPrevI, COLORINDEX_USER3);
						if (!(dwCookie & COOKIE_SCRIPT))
						{
							dwCookie &= ~COOKIE_ASP;
							dwScriptTagCookie &= COOKIE_STRING;
						}
						dwCookie &= ~COOKIE_PARSER;
						dwCookie |= dwScriptTagCookie & COOKIE_STRING;
						if (!(dwCookie & COOKIE_ASP))
							dwCookie |= dwScriptTagCookie & COOKIE_PARSER;
						bRedefineBlock = TRUE;
						bDecIndex = FALSE;
						continue;
					}
					if (pszChars[I] == '/' && pszChars[nPrevI] == '<')
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
						DEFINE_BLOCK(nPrevI, COLORINDEX_OPERATOR);
						DEFINE_BLOCK(I, COLORINDEX_PREPROCESSOR);
						dwCookie &= ~(COOKIE_PARSER | COOKIE_SCRIPT);
						dwCookie |= COOKIE_PREPROCESSOR;
						continue;
					}
				}

				if (pszChars[I] == '[' || pszChars[I] == ']')
				{
					dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD);
				}

				// Double-quoted text
				if (pszChars[I] == '"')
				{
					if (dwCookie & COOKIE_PARSER_GLOBAL)
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
					}
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= (dwCookie & COOKIE_PARSER) == COOKIE_PARSER_CSHARP && I > 0 && pszChars[nPrevI] == '@' ? COOKIE_STRING_REGEXP : COOKIE_STRING_DOUBLE;
					continue;
				}

				// Single-quoted text
				if (pszChars[I] == '\'' && (I == 0 || !xisxdigit(pszChars[nPrevI])))
				{
					if (dwCookie & COOKIE_PARSER_GLOBAL)
					{
						pBuf.m_bRecording = pBuf;
						if (nScriptBegin >= 0)
						{
							dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
							nScriptBegin = -1;
						}
					}
					DEFINE_BLOCK(I, COLORINDEX_STRING);
					dwCookie |= COOKIE_STRING_SINGLE;
					continue;
				}
			}

			switch (dwCookie & (COOKIE_PARSER | COOKIE_DTD))
			{
			case 0:
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
			case COOKIE_PARSER_BASIC:
			case COOKIE_PARSER_VBSCRIPT:
				break;
			case COOKIE_PARSER_JAVA:
			case COOKIE_PARSER_JSCRIPT:
				if (pszChars[I] == '/' && I < nLength - 1 && pszChars[I + 1] != '/' && pszChars[I + 1] != '*')
				{
					if (!(dwCookie & COOKIE_REJECT_REGEXP))
					{
						if (dwCookie & COOKIE_PARSER_GLOBAL)
						{
							pBuf.m_bRecording = pBuf;
							if (nScriptBegin >= 0)
							{
								dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
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
				if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/')
				{
					DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
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
		}
		if (nIdentBegin >= 0)
		{
			LPCTSTR const pchIdent = pszChars + nIdentBegin;
			int const cchIdent = I - nIdentBegin;
			if (dwCookie & COOKIE_PREPROCESSOR)
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
							dwScriptTagCookie = 0;
						}
						else if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("style")))
						{
							dwCookie |= COOKIE_ASP | COOKIE_SCRIPT;
							dwScriptTagCookie = COOKIE_PARSER_CSS;
						}
						else switch (dwCookie & COOKIE_PARSER_GLOBAL)
						{
						case SRCOPT_COOKIE(COOKIE_PARSER_MWSL):
							if (xisequal<_tcsncmp>(pchIdent, cchIdent, _T("MWSL")))
							{
								dwCookie |= COOKIE_ASP | COOKIE_SCRIPT;
								dwScriptTagCookie = COOKIE_PARSER_MWSL;
							}
							break;
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
					else switch (dwCookie & COOKIE_PARSER_GLOBAL)
					{
					case SRCOPT_COOKIE(COOKIE_PARSER_MWSL):
						if (xisequal<_tcsncmp>(pchIdent, cchIdent, _T("MWSL")))
						{
							DEFINE_BLOCK(nIdentBegin, COLORINDEX_USER3);
						}
						break;
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
						else if (dwScriptTagCookie == 0 &&
							xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("runat")) &&
							_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z#\\-]"), lang, lang) == 2 &&
							_tcsicmp(lang, _T("server")) == 0)
						{
							dwScriptTagCookie = (dwCookie & COOKIE_PARSER_GLOBAL) >> 4;
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
			else if (dwCookie & COOKIE_ASP)
			{
				TCHAR lang[32];
				if (xisequal<_tcsnicmp>(pchIdent, cchIdent, _T("language")) &&
					_stscanf(pchIdent + cchIdent, _T("%31[^a-zA-Z]%31[a-zA-Z#]"), lang, lang) == 2)
				{
					dwCookie = dwCookie & ~COOKIE_PARSER_GLOBAL | (ScriptCookie(lang) << 4);
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
			nIdentBegin = -1;
		}

		bRedefineBlock = TRUE;
		bDecIndex = TRUE;

		switch (dwCookie & (COOKIE_PREPROCESSOR | COOKIE_PARSER))
		{
		case 0:
			// Preprocessor start: < or bracket
			if (pszChars[I] == '<' && !(I < nLength - 3 && pszChars[I + 1] == '!' && pszChars[I + 2] == '-' && pszChars[I + 3] == '-'))
			{
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
				if (pszChars[nPrevI] == '/')
				{
					dwCookie &= ~(COOKIE_SCRIPT | COOKIE_PARSER);
				}
				else if (dwCookie & COOKIE_ASP)
				{
					dwCookie &= ~COOKIE_PARSER;
					if ((dwScriptTagCookie & COOKIE_PARSER) == 0)
						dwScriptTagCookie = COOKIE_PARSER_JSCRIPT;
					dwCookie |= dwScriptTagCookie;
				}
				dwCookie &= ~(COOKIE_PREPROCESSOR | COOKIE_DTD | COOKIE_ASP);
				if (dwCookie & COOKIE_PARSER)
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
				if (I == 0 || pszChars[nPrevI] != peer)
					break;
				// fall through
			case ')':
			case ']':
				dwCookie |= COOKIE_REJECT_REGEXP;
				break;
			}
		}
	} while (I < nLength);

	if (dwCookie & COOKIE_PARSER)
	{
		pBuf.m_bRecording = pBuf;
		if (nScriptBegin >= 0)
		{
			dwCookie = dwCookie & 0xFFFF0000 | (this->*ScriptParseProc(dwCookie))(dwCookie, pszChars, I, nScriptBegin - 1, pBuf);
		}
	}

	return dwCookie;
}
