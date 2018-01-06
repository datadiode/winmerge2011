///////////////////////////////////////////////////////////////////////////
//  File:       rust.cpp
//  Version:    1.0.0.0
//  Created:    23-Jul-2017
//
//  Copyright:  Stcherbatchenko Andrei, portions by Takashi Sawanaka
//  E-mail:     sdottaka@users.sourceforge.net
//
//  Rust syntax highlighing definition
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

// Rust keywords
static BOOL IsRustKeyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszRustKeywordList[] =
  {
    _T ("Self"),
    _T ("abstract"),
    _T ("alignof"),
    _T ("as"),
    _T ("become"),
    _T ("box"),
    _T ("break"),
    _T ("const"),
    _T ("continue"),
    _T ("crate"),
    _T ("do"),
    _T ("else"),
    _T ("enum"),
    _T ("extern"),
    _T ("false"),
    _T ("final"),
    _T ("fn"),
    _T ("for"),
    _T ("if"),
    _T ("impl"),
    _T ("in"),
    _T ("let"),
    _T ("loop"),
    _T ("macro"),
    _T ("match"),
    _T ("mod"),
    _T ("move"),
    _T ("mut"),
    _T ("offsetof"),
    _T ("override"),
    _T ("priv"),
    _T ("proc"),
    _T ("pub"),
    _T ("pure"),
    _T ("ref"),
    _T ("return"),
    _T ("self"),
    _T ("sizeof"),
    _T ("static"),
    _T ("struct"),
    _T ("super"),
    _T ("trait"),
    _T ("true"),
    _T ("type"),
    _T ("typeof"),
    _T ("unsafe"),
    _T ("unsized"),
    _T ("use"),
    _T ("virtual"),
    _T ("where"),
    _T ("while"),
    _T ("yield"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszRustKeywordList);
}

static BOOL IsUser1Keyword(LPCTSTR pszChars, int nLength)
{
  static LPCTSTR const s_apszUser1KeywordList[] =
  {
    _T ("String"),
    _T ("binary32"),
    _T ("binary64"),
    _T ("bool"),
    _T ("char"),
    _T ("f32"),
    _T ("f64"),
    _T ("i16"),
    _T ("i32"),
    _T ("i64"),
    _T ("i8"),
    _T ("isize"),
    _T ("str"),
    _T ("u16"),
    _T ("u32"),
    _T ("u64"),
    _T ("u8"),
    _T ("usize"),
  };
  return xiskeyword<_tcsnicmp>(pszChars, nLength, s_apszUser1KeywordList);
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
#define COOKIE_EXT_COMMENT      0x0004
#define COOKIE_STRING           0x0008
#define COOKIE_RAWSTRING        0x0020
#define COOKIE_GET_EXT_COMMENT_DEPTH(cookie) (((cookie) & 0x0F00) >> 8)
#define COOKIE_SET_EXT_COMMENT_DEPTH(cookie, depth) (cookie) = (((cookie) & 0xF0FF) | ((depth) << 8))
#define COOKIE_GET_RAWSTRING_NUMBER_COUNT(cookie) (((cookie) & 0xF000) >> 12)
#define COOKIE_SET_RAWSTRING_NUMBER_COUNT(cookie, count) (cookie) = (((cookie) & 0x0FFF) | ((count) << 12))

DWORD CCrystalTextView::ParseLineRust(DWORD dwCookie, int nLineIndex, TEXTBLOCK * pBuf, int &nActualItems)
{
  const int nLength = GetLineLength(nLineIndex);
  if (nLength == 0)
    return dwCookie & (COOKIE_EXT_COMMENT | COOKIE_RAWSTRING | COOKIE_STRING | 0xFF00);

  const LPCTSTR pszChars = GetLineChars(nLineIndex);
  LPCTSTR pszRawStringBegin = NULL;
  LPCTSTR pszCommentBegin = NULL;
  LPCTSTR pszCommentEnd = NULL;
  bool bRedefineBlock = true;
  bool bDecIndex = false;
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
          else if (dwCookie & (COOKIE_STRING | COOKIE_RAWSTRING))
            {
              DEFINE_BLOCK(nPos, COLORINDEX_STRING);
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
                  bRedefineBlock = true;
                  bDecIndex = true;
                  goto out;
                }
            }
          bRedefineBlock = false;
          bDecIndex = false;
        }
out:

      // Can be bigger than length if there is binary data
      // See bug #1474782 Crash when comparing SQL with with binary data
      if (I >= nLength || pszChars[I] == 0)
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
              bRedefineBlock = true;
            }
          continue;
        }

      //  Raw string constant r"...." ,  r#"..."# , r##"..."##
      if (dwCookie & COOKIE_RAWSTRING)
        {
          const int nNumberCount = COOKIE_GET_RAWSTRING_NUMBER_COUNT(dwCookie);
          if (I >= nNumberCount && pszChars[I - nNumberCount] == '"')
            {
              if ((pszRawStringBegin < pszChars + I - nNumberCount) && 
                  (std::count(pszChars + I - nNumberCount + 1, pszChars + I + 1, '#') == nNumberCount))
                {
                  dwCookie &= ~COOKIE_RAWSTRING;
                  bRedefineBlock = true;
                  pszRawStringBegin = NULL;
                }
            }
          continue;
        }

      //  Extended comment /*....*/
      if (dwCookie & COOKIE_EXT_COMMENT)
        {
          const int depth = COOKIE_GET_EXT_COMMENT_DEPTH(dwCookie);
          if ((pszCommentEnd < pszChars + I) && (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/'))
            {
              COOKIE_SET_EXT_COMMENT_DEPTH(dwCookie, depth + 1);
              pszCommentBegin = pszChars + I + 1;
            }
          else if ((pszCommentBegin < pszChars + I) && (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '*'))
            {
              if (depth == 0)
                {
                  dwCookie &= ~COOKIE_EXT_COMMENT;
                  bRedefineBlock = true;
                }
              else
                {
                  COOKIE_SET_EXT_COMMENT_DEPTH(dwCookie, depth - 1);
                }
              pszCommentEnd = pszChars + I + 1;
            }
          continue;
        }

      if (I > 0 && pszChars[I] == '/' && pszChars[nPrevI] == '/')
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

      //  Raw string
      if ((pszChars[I] == 'r' && I + 1 < nLength && (pszChars[I + 1] == '"' || pszChars[I + 1] == '#')) ||
          (pszChars[I] == 'b' && I + 2 < nLength && pszChars[I + 1] == 'r' && (pszChars[I + 2] == '"' || pszChars[I + 2] == '#')))
        {
          const int nprefix = (pszChars[I] == 'r' ? 1 : 2);
          const TCHAR *p = pszChars + I + nprefix;
          while (p < pszChars + nLength && *p == '#') ++p;
          if (p != pszChars + nLength && *p == '"')
            {
              DEFINE_BLOCK(I, COLORINDEX_STRING);
              dwCookie |= COOKIE_RAWSTRING;
              COOKIE_SET_RAWSTRING_NUMBER_COUNT(dwCookie, static_cast<int>(p - (pszChars + I + nprefix)));
              pszRawStringBegin = p + 1;
              continue;
            }
        }
      if ((pszCommentEnd < pszChars + I) && (I > 0 && pszChars[I] == '*' && pszChars[nPrevI] == '/'))
        {
          DEFINE_BLOCK(nPrevI, COLORINDEX_COMMENT);
          dwCookie |= COOKIE_EXT_COMMENT;
          pszCommentBegin = pszChars + I + 1;
          continue;
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
              if (IsRustKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
                  bool bFunction = false;

                  for (int j = I; j < nLength; j++)
                    {
                      if (!xisspace(pszChars[j]) && pszChars[j] != '!')
                        {
                          if (pszChars[j] == '(')
                            {
                              bFunction = true;
                            }
                          break;
                        }
                    }
                  if (bFunction)
                    {
                      DEFINE_BLOCK(nIdentBegin, COLORINDEX_FUNCNAME);
                    }
                }
              bRedefineBlock = true;
              bDecIndex = true;
              nIdentBegin = -1;
            }
        }
    }

  if (nIdentBegin >= 0)
    {
      if (IsRustKeyword(pszChars + nIdentBegin, I - nIdentBegin))
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
          bool bFunction = false;

          for (int j = I; j < nLength; j++)
            {
              if (!xisspace(pszChars[j]) && pszChars[j] != '!')
                {
                  if (pszChars[j] == '(')
                    {
                      bFunction = true;
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

  dwCookie &= COOKIE_EXT_COMMENT | COOKIE_RAWSTRING | COOKIE_STRING | 0xFF00;
  return dwCookie;
}
