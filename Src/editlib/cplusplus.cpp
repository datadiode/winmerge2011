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
#include "SyntaxColors.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using CommonKeywords::IsNumeric;

// C++ keywords (MSVC5.0 + POET5.0)
static BOOL IsCppKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszCppKeywordList[] =
  {
    _T ("__asm"),
    _T ("__based"),
    _T ("__cdecl"),
    _T ("__declspec"),
    _T ("__except"),
    _T ("__export"),
    _T ("__far16"),
    _T ("__fastcall"),
    _T ("__finally"),
    _T ("__inline"),
    _T ("__int16"),
    _T ("__int32"),
    _T ("__int64"),
    _T ("__int8"),
    _T ("__leave"),
    _T ("__multiple_inheritance"),
    _T ("__pascal"),
    _T ("__single_inheritance"),
    _T ("__stdcall"),
    _T ("__syscall"),
    _T ("__try"),
    _T ("__uuidof"),
    _T ("__virtual_inheritance"),
    _T ("_asm"),
    _T ("_cdecl"),
    _T ("_export"),
    _T ("_far16"),
    _T ("_fastcall"),
    _T ("_pascal"),
    _T ("_persistent"),
    _T ("_stdcall"),
    _T ("_syscall"),
    _T ("alignas"),
    _T ("alignof"),
    _T ("auto"),
    _T ("bool"),
    _T ("break"),
    _T ("case"),
    _T ("catch"),
    _T ("char"),
    _T ("char16_t"),
    _T ("char32_t"),
    _T ("class"),
    _T ("const"),
    _T ("const_cast"),
    _T ("constexpr"),
    _T ("continue"),
    _T ("cset"),
    _T ("decltype"),
    _T ("default"),
    _T ("delete"),
    _T ("depend"),
    _T ("dllexport"),
    _T ("dllimport"),
    _T ("do"),
    _T ("double"),
    _T ("dynamic_cast"),
    _T ("else"),
    _T ("enum"),
    _T ("explicit"),
    _T ("extern"),
    _T ("false"),
    _T ("float"),
    _T ("for"),
    _T ("friend"),
    _T ("goto"),
    _T ("if"),
    _T ("indexdef"),
    _T ("inline"),
    _T ("int"),
    _T ("interface"),
    _T ("long"),
    _T ("main"),
    _T ("mutable"),
    _T ("naked"),
    _T ("namespace"),
    _T ("new"),
    _T ("noexcept"),
    _T ("nullptr"),
    _T ("ondemand"),
    _T ("operator"),
    _T ("persistent"),
    _T ("private"),
    _T ("protected"),
    _T ("public"),
    _T ("register"),
    _T ("reinterpret_cast"),
    _T ("return"),
    _T ("short"),
    _T ("signed"),
    _T ("sizeof"),
    _T ("static"),
    _T ("static_assert"),
    _T ("static_cast"),
    _T ("struct"),
    _T ("switch"),
    _T ("template"),
    _T ("this"),
    _T ("thread"),
    _T ("thread_local"),
    _T ("throw"),
    _T ("transient"),
    _T ("transient"),
    _T ("true"),
    _T ("try"),
    _T ("typedef"),
    _T ("typeid"),
    _T ("typename"),
    _T ("union"),
    _T ("unsigned"),
    _T ("useindex"),
    _T ("using"),
    _T ("uuid"),
    _T ("virtual"),
    _T ("void"),
    _T ("volatile"),
    _T ("while"),
    _T ("wmain"),
    _T ("xalloc"),
  };
  return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszCppKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszUser1KeywordList[] =
  {
    _T ("BOOL"),
    _T ("BSTR"),
    _T ("BYTE"),
    _T ("CHAR"),
    _T ("COLORREF"),
    _T ("DWORD"),
    _T ("DWORD32"),
    _T ("FALSE"),
    _T ("HANDLE"),
    _T ("INT"),
    _T ("INT16"),
    _T ("INT32"),
    _T ("INT64"),
    _T ("INT8"),
    _T ("LONG"),
    _T ("LPARAM"),
    _T ("LPBOOL"),
    _T ("LPBYTE"),
    _T ("LPCSTR"),
    _T ("LPCTSTR"),
    _T ("LPCTSTR"),
    _T ("LPCWSTR"),
    _T ("LPDWORD"),
    _T ("LPINT"),
    _T ("LPLONG"),
    _T ("LPRECT"),
    _T ("LPSTR"),
    _T ("LPTSTR"),
    _T ("LPTSTR"),
    _T ("LPVOID"),
    _T ("LPVOID"),
    _T ("LPWORD"),
    _T ("LPWSTR"),
    _T ("LRESULT"),
    _T ("LRESULT"),
    _T ("PBOOL"),
    _T ("PBYTE"),
    _T ("PDWORD"),
    _T ("PDWORD32"),
    _T ("PINT"),
    _T ("PINT16"),
    _T ("PINT32"),
    _T ("PINT64"),
    _T ("PINT8"),
    _T ("POSITION"),
    _T ("PUINT"),
    _T ("PUINT16"),
    _T ("PUINT32"),
    _T ("PUINT64"),
    _T ("PUINT8"),
    _T ("PULONG32"),
    _T ("PWORD"),
    _T ("TCHAR"),
    _T ("TRUE"),
    _T ("UINT"),
    _T ("UINT16"),
    _T ("UINT32"),
    _T ("UINT64"),
    _T ("UINT8"),
    _T ("ULONG32"),
    _T ("VOID"),
    _T ("WCHAR"),
    _T ("WNDPROC"),
    _T ("WORD"),
    _T ("WPARAM"),
  };
  return xiskeyword<_tcsncmp>(pszChars, nLength, s_apszUser1KeywordList);
}

#define DEFINE_BLOCK(pos, colorindex)   \
ASSERT((pos) >= 0 && (pos) <= nLength);\
if (pBuf != NULL)\
  {\
    if (nActualItems == 0 || pBuf[nActualItems - 1].m_nCharPos <= (pos)){\
        pBuf[nActualItems].m_nCharPos = (pos);\
        pBuf[nActualItems].m_nColorIndex = (colorindex);\
        pBuf[nActualItems].m_nBgColorIndex = COLORINDEX_BKGND;\
        nActualItems ++;}\
  }

#define COOKIE_COMMENT          0x0001
#define COOKIE_PREPROCESSOR     0x0002
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_CHAR             0x0010

DWORD CCrystalTextView::ParseLineC(DWORD dwCookie, int nLineIndex, TEXTBLOCK *pBuf, int &nActualItems)
{
  const int nLength = GetLineLength(nLineIndex);
  if (nLength == 0)
    return dwCookie & COOKIE_EXT_COMMENT;

  const LPCTSTR pszChars = GetLineChars(nLineIndex);
  BOOL bFirstChar = (dwCookie & ~COOKIE_EXT_COMMENT) == 0;
  BOOL bRedefineBlock = TRUE;
  BOOL bWasCommentStart = FALSE;
  BOOL bDecIndex = FALSE;
  int nIdentBegin = -1;
  int nPrevI = -1;
  int I;
  for (I = 0; I <= nLength; nPrevI = I++)
    {
      if (bRedefineBlock)
        {
          int nPos = I;
          if (bDecIndex)
            nPos = nPrevI;
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
          else
            {
              if (xisalnum(pszChars[nPos]) || pszChars[nPos] == '.' && nPos > 0 && (!xisalpha(pszChars[nPos - 1]) && !xisalpha(pszChars[nPos + 1])))
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_NORMALTEXT);
                }
              else
                {
                  DEFINE_BLOCK(nPos, COLORINDEX_OPERATOR);
                  bRedefineBlock = TRUE;
                  bDecIndex = TRUE;
                  goto out;
                }
            }
          bRedefineBlock = FALSE;
          bDecIndex = FALSE;
        }
out:

      // Can be bigger than length if there is binary data
      // See bug #1474782 Crash when comparing SQL with with binary data
      if (I >= nLength)
        break;

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

      //  Extended comment /*....*/
      if (dwCookie & COOKIE_EXT_COMMENT)
        {
          // if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '*')
          if ((I > 1 && pszChars[I] == '/' && pszChars[nPrevI] == '*' /*&& pszChars[nPrevI - 1] != '/'*/ && !bWasCommentStart) || (I == 1 && pszChars[I] == '/' && pszChars[nPrevI] == '*'))
            {
              dwCookie &= ~COOKIE_EXT_COMMENT;
              bRedefineBlock = TRUE;
            }
          bWasCommentStart = FALSE;
          continue;
        }

      if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/')
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_COMMENT;
          break;
        }

      //  Preprocessor directive #....
      if (dwCookie & COOKIE_PREPROCESSOR)
        {
          if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/')
            {
              DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
              dwCookie |= COOKIE_EXT_COMMENT;
            }
          continue;
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
      if (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/')
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_EXT_COMMENT;
          bWasCommentStart = TRUE;
          continue;
        }

      bWasCommentStart = FALSE;

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
        continue;               //  We don't need to extract keywords,
      //  for faster parsing skip the rest of loop

      if (xisalnum(pszChars[I]) || pszChars[I] == '.' && I > 0 && (!xisalpha(pszChars[nPrevI]) && !xisalpha(pszChars[I + 1])))
        {
          if (nIdentBegin == -1)
            nIdentBegin = I;
        }
      else
        {
          if (nIdentBegin >= 0)
            {
              if (IsCppKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
                  bool bFunction = FALSE;

                  for (int j = I; j < nLength; j++)
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
                  if (bFunction)
                    {
                      DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
                    }
                }
              bRedefineBlock = TRUE;
              bDecIndex = TRUE;
              nIdentBegin = -1;
            }
        }
    }

  if (nIdentBegin >= 0)
    {
      if (IsCppKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
          bool bFunction = FALSE;

          for (int j = I; j < nLength; j++)
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
          if (bFunction)
            {
              DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
            }
        }
    }

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
			assert(pfnIsKeyword(p, static_cast<int>(q - p)));
		else if (_stscanf(text, _T(" static BOOL IsCppKeyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsCppKeyword;
		else if (_stscanf(text, _T(" static BOOL IsUser1Keyword %c"), &c) == 1 && c == '(')
			pfnIsKeyword = IsUser1Keyword;
		else if (pfnIsKeyword && _stscanf(text, _T(" } %c"), &c) == 1 && (c == ';' ? ++count : 0))
			pfnIsKeyword = NULL;
	}
	fclose(file);
	assert(count == 2);
	return count;
}
