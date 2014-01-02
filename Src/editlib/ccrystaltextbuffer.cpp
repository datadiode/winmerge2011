////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextbuffer.cpp
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalTextBuffer class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  17-Feb-99
//  +   FIX: unnecessary 'HANDLE' in CCrystalTextBuffer::SaveToFile
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  21-Feb-99
//      Paul Selormey, James R. Twine:
//  +   FEATURE: description for Undo/Redo actions
//  +   FEATURE: multiple MSVC-like bookmarks
//  +   FEATURE: 'Disable backspace at beginning of line' option
//  +   FEATURE: 'Disable drag-n-drop editing' option
//
//  +   FEATURE: changed layout of SUndoRecord. Now takes less memory
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: some other things I've forgotten ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	??-Aug-99
//		Sven Wiegand (search for "//BEGIN SW" to find my changes):
//	+ FEATURE: Remembering the text-position of the latest change.
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	24-Oct-99
//		Sven Wiegand
//	+ FIX: Setting m_ptLastChange to the beginning of the selection in
//		     InternalDeleteText(), so that position is valid in any case.
//		     Editor won't crash any more i.e. by selecting whole buffer and
//		     deleting it and then executing ID_EDIT_GOTO_LAST_CHANGE-command.
////////////////////////////////////////////////////////////////////////////
/** 
 * @file ccrystaltextbuffer.cpp
 *
 * @brief Code for CCrystalTextBuffer class
 */
// line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "editcmd.h"
#include "LineInfo.h"
#include "UndoRecord.h"
#include "ccrystaltextbuffer.h"
#include "ccrystaltextview.h"
#include "coretools.h"
#include <mbctype.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using stl::vector;

#ifdef _DEBUG
#define _ADVANCED_BUGCHECK  1
#endif

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer::CUpdateContext

void CCrystalTextBuffer::CInsertContext::RecalcPoint(POINT & ptPoint)
{
  ASSERT(m_ptEnd.y > m_ptStart.y ||
         m_ptEnd.y == m_ptStart.y && m_ptEnd.x >= m_ptStart.x);
  if (ptPoint.y < m_ptStart.y)
    return;
  if (ptPoint.y > m_ptStart.y)
    {
      ptPoint.y += (m_ptEnd.y - m_ptStart.y);
      return;
    }
  if (ptPoint.x <= m_ptStart.x)
    return;
  ptPoint.y += (m_ptEnd.y - m_ptStart.y);
  ptPoint.x = m_ptEnd.x + (ptPoint.x - m_ptStart.x);
}

void CCrystalTextBuffer::CDeleteContext::RecalcPoint(POINT & ptPoint)
{
  ASSERT(m_ptEnd.y > m_ptStart.y ||
         m_ptEnd.y == m_ptStart.y && m_ptEnd.x >= m_ptStart.x);
  if (ptPoint.y < m_ptStart.y)
    return;
  if (ptPoint.y > m_ptEnd.y)
    {
      ptPoint.y -= (m_ptEnd.y - m_ptStart.y);
      return;
    }
  if (ptPoint.y == m_ptEnd.y && ptPoint.x >= m_ptEnd.x)
    {
      ptPoint.y = m_ptStart.y;
      ptPoint.x = m_ptStart.x + (ptPoint.x - m_ptEnd.x);
      return;
    }
  if (ptPoint.y == m_ptStart.y)
    {
      if (ptPoint.x > m_ptStart.x)
        ptPoint.x = m_ptStart.x;
      return;
    }
  ptPoint = m_ptStart;
}


/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer

CCrystalTextBuffer::CCrystalTextBuffer()
{
	m_bInit = false;
	m_bReadOnly = false;
	m_bModified = false;
	m_nCRLFMode = CRLF_STYLE_DOS;
	m_nUndoPosition = 0;
	m_bUndoGroup = m_bUndoBeginGroup = FALSE;
	m_bInsertTabs = true;
	m_nTabSize = 4;
	//BEGIN SW
	m_ptLastChange.x = m_ptLastChange.y = -1;
	//END SW
	m_dwCurrentRevisionNumber = 0;
	m_dwRevisionNumberOnSave = 0;
}

CCrystalTextBuffer::~CCrystalTextBuffer()
{
	ASSERT(!m_bInit); // You must call FreeAll() before deleting the object
}

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextBuffer message handlers

/** 
 * @brief Insert the same line once or several times
 *
 * @param nPosition : not defined (or -1) = add lines at the end of array
 */
void CCrystalTextBuffer::InsertLine(LPCTSTR pszLine, int nLength, int nPosition)
{
	ASSERT(nLength != -1);

	LineInfo line;
	line.Create(pszLine, nLength);

	// nPosition not defined ? Insert at end of array
	if (nPosition == -1)
		nPosition = GetLineCount();

	// insert all lines in one pass
	m_aLines.insert(m_aLines.begin() + nPosition, 1, line);

#ifdef _DEBUG
	// Warning : this function is also used during rescan
	// and this trace will appear even after the initial load
	size_t nLines = m_aLines.size();
	if (nLines % 5000 == 0)
		TRACE("%u lines loaded!\n", nLines);
#endif
}

// Add characters to end of specified line
// Specified line must not have any EOL characters
void CCrystalTextBuffer::AppendLine(int nLineIndex, LPCTSTR pszChars, int nLength)
{
	ASSERT(nLength != -1);
	if (nLength > 0)
	{
		LineInfo &li = m_aLines[nLineIndex];
		li.Append(pszChars, nLength);
	}
}

/**
 * @brief Copy line range [line1;line2] to range starting at newline1
 *
 * NB: Lines are assigned, not inserted
 *
 * Example#1:
 *   MoveLine(5,7,100)
 *    line1=5
 *    line2=7
 *    newline1=100
 *    ldiff=95
 *     l=10  lines[105] = lines[10]
 *     l=9   lines[104] = lines[9]
 *     l=8   lines[103] = lines[8]
 *
 * Example#2:
 *   MoveLine(40,42,10)
 *    line1=40
 *    line2=42
 *    newline1=10
 *    ldiff=-30
 *     l=40  lines[10] = lines[40]
 *     l=41  lines[11] = lines[41]
 *     l=42  lines[12] = lines[42]
 */
void CCrystalTextBuffer::MoveLine(int line1, int line2, int newline1)
{
	int ldiff = newline1 - line1;
	if (ldiff > 0) {
		for (int l = line2; l >= line1; l--)
			m_aLines[l+ldiff] = m_aLines[l];
	}
	else if (ldiff < 0) {
		for (int l = line1; l <= line2; l++)
			m_aLines[l+ldiff] = m_aLines[l];
	}
}

void CCrystalTextBuffer::FreeAll()
{
	// Free text
	vector<LineInfo>::iterator iter = m_aLines.begin();
	vector<LineInfo>::iterator end = m_aLines.end();
	while (iter != end)
	{
		iter->Clear();
		++iter;
	}
	//m_aUndoBuf.clear();
	m_aLines.clear();
	m_bInit = false;
	//BEGIN SW
	m_ptLastChange.x = m_ptLastChange.y = -1;
	//END SW
}

BOOL CCrystalTextBuffer::InitNew(CRLFSTYLE nCrlfStyle)
{
	ASSERT(!m_bInit);
	ASSERT(m_aLines.size() == 0);
	ASSERT(nCrlfStyle >= 0 && nCrlfStyle <= 2);
	InsertLine(_T(""), 0);
	m_bInit = true;
	m_bReadOnly = false;
	m_nCRLFMode = nCrlfStyle;
	m_bModified = false;
	m_bInsertTabs = true;
	m_nTabSize = 4;
	m_nSyncPosition = m_nUndoPosition = 0;
	m_bUndoGroup = m_bUndoBeginGroup = FALSE;
	ASSERT(GetUndoRecordCount() == 0);
	m_dwCurrentRevisionNumber = 0;
	m_dwRevisionNumberOnSave = 0;
	UpdateViews(NULL, NULL, UPDATE_RESET);
	//BEGIN SW
	m_ptLastChange.x = m_ptLastChange.y = -1;
	//END SW
	return TRUE;
}

bool CCrystalTextBuffer::GetReadOnly() const
{
	ASSERT(m_bInit);        //  Text buffer not yet initialized.
	//  You must call InitNew() or LoadFromFile() first!
	return m_bReadOnly;
}

void CCrystalTextBuffer::SetReadOnly(bool bReadOnly)
{
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	//  You must call InitNew() or LoadFromFile() first!
	m_bReadOnly = bReadOnly;
}

CRLFSTYLE CCrystalTextBuffer::GetCRLFMode() const
{
	return m_nCRLFMode;
}

// Default EOL to use if editor has to manufacture one
// (this occurs with ghost lines)
void CCrystalTextBuffer::SetCRLFMode(CRLFSTYLE nCRLFMode)
{
	if (nCRLFMode == CRLF_STYLE_AUTOMATIC)
		nCRLFMode = CRLF_STYLE_DOS;
	m_nCRLFMode = nCRLFMode;

	ASSERT(m_nCRLFMode == CRLF_STYLE_DOS || m_nCRLFMode == CRLF_STYLE_UNIX ||
		m_nCRLFMode == CRLF_STYLE_MAC || m_nCRLFMode == CRLF_STYLE_MIXED);
}

bool CCrystalTextBuffer::applyEOLMode()
{
	LPCTSTR lpEOLtoApply = GetDefaultEol();
	bool bChanged = false;
	const int size = m_aLines.size();
	for (int i = 0 ; i < size; i++)
	{
		// the last real line has no EOL
		if (m_aLines[i].HasEol())
			bChanged |= ChangeLineEol(i, lpEOLtoApply);
	}

	if (bChanged)
		SetModified(true);

	return bChanged;
}

// get pointer to any trailing eol characters (pointer to empty string if none)
LPCTSTR CCrystalTextBuffer::GetLineEol(int nLine) const
{
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	if (LPCTSTR eol = m_aLines[nLine].GetEol())
		return eol;
	return _T("");
}

bool CCrystalTextBuffer::ChangeLineEol(int nLine, LPCTSTR lpEOL) 
{
	return m_aLines[nLine].ChangeEol(lpEOL);
}

const LineInfo &CCrystalTextBuffer::GetLineInfo(int nLine) const
{
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	//  You must call InitNew() or LoadFromFile() first!
	return m_aLines[nLine];
}

int CCrystalTextBuffer::FindLineWithFlag(DWORD dwFlag, int nLine) const
{
	const int nLineCount = m_aLines.size();
	while (++nLine < nLineCount)
	{
		if ((m_aLines[nLine].m_dwFlags & dwFlag) != 0)
			return nLine;
	}
	return -1;
}

void CCrystalTextBuffer::SetLineFlags(int nLine, DWORD dwFlags)
{
	//  You must call InitNew() or LoadFromFile() first!
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	ASSERT(nLine != -1);
	if (m_aLines[nLine].m_dwFlags != dwFlags)
	{
		m_aLines[nLine].m_dwFlags = dwFlags;
		UpdateViews(NULL, NULL, UPDATE_SINGLELINE | UPDATE_FLAGSONLY, nLine);
	}
}

/**
 * @brief Get text of specified line range
 */
void CCrystalTextBuffer::GetText(int nStartLine, int nStartChar,
	int nEndLine, int nEndChar, String &text, LPCTSTR pszCRLF) const
{
	ASSERT(nStartLine >= 0 && nStartLine < GetLineCount());
	ASSERT(nStartChar >= 0 && nStartChar <= GetLineLength(nStartLine));
	ASSERT(nEndLine >= 0 && nEndLine < GetLineCount());
	ASSERT(nEndChar >= 0 && nEndChar <= GetFullLineLength(nEndLine));
	ASSERT(nStartLine < nEndLine || nStartLine == nEndLine && nStartChar <= nEndChar);
	// some edit functions (copy...) should do nothing when there is no selection.
	// assert to be sure to catch these 'do nothing' cases.
	//ASSERT(nStartLine != nEndLine || nStartChar != nEndChar);

	// estimate size (upper bound)
	int nBufSize = 0;
	int i;
	for (i = nStartLine; i <= nEndLine; ++i)
		nBufSize += m_aLines[i].Length() + 2; // in case we insert EOLs
	text.resize(nBufSize);
	LPTSTR pszBuf = &text.front();

	if (pszCRLF != NULL)
	{
		// we must copy this EOL type only
		const size_t nCRLFLength = _tcslen(pszCRLF);
		for (i = nStartLine; i <= nEndLine; ++i)
		{
			// exclude ghost lines
			const LineInfo &li = m_aLines[i];
			if (li.FullLength() == 0)
				continue;

			// copy the line, excluding the EOL
			int chars = (i == nEndLine ? nEndChar : li.Length()) - nStartChar;
			LPCTSTR szLine = li.GetLine(nStartChar);
			CopyMemory(pszBuf, szLine, chars * sizeof(TCHAR));
			pszBuf += chars;
			nStartChar = 0;

			// copy the EOL of the requested type
			if (i < nEndLine && li.GetEol())
			{
				CopyMemory(pszBuf, pszCRLF, nCRLFLength * sizeof(TCHAR));
				pszBuf += nCRLFLength;
			}
		}
	}
	else
	{
		for (i = nStartLine; i <= nEndLine; ++i)
		{
			// exclude ghost lines
			const LineInfo &li = m_aLines[i];
			if (li.FullLength() == 0)
				continue;

			// copy the line including the EOL
			int chars = (i == nEndLine ? nEndChar : li.FullLength()) - nStartChar;
			LPCTSTR szLine = li.GetLine(nStartChar);
			CopyMemory(pszBuf, szLine, chars * sizeof(TCHAR));
			pszBuf += chars;
			nStartChar = 0;
		}
	}
	text.resize(static_cast<String::size_type>(pszBuf - text.c_str()));
}

void CCrystalTextBuffer::AddView(CCrystalTextView * pView)
{
	m_lpViews.push_back(pView);
}

void CCrystalTextBuffer::RemoveView(CCrystalTextView *pView)
{
	stl::list<CCrystalTextView *>::iterator pos = stl::find(m_lpViews.begin(), m_lpViews.end(), pView);
	assert(pos != m_lpViews.end());
	if (pos != m_lpViews.end())
		m_lpViews.erase(pos);
}

CCrystalTextView::TextDefinition *CCrystalTextBuffer::RetypeViews(LPCTSTR lpszFileName)
{
	LPCTSTR ext = PathFindExtension(lpszFileName);
	if (*ext == _T('.'))
		++ext;
	CCrystalTextView::TextDefinition *def = CCrystalTextView::GetTextType(ext);
	stl::list<CCrystalTextView *>::iterator pos = m_lpViews.begin();
	while (pos != m_lpViews.end())
	{
		CCrystalTextView *const pView = *pos++;
		pView->SetTextType(def);
	}
	return def;
}

void CCrystalTextBuffer::UpdateViews(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwUpdateFlags, int nLineIndex)
{
	stl::list<CCrystalTextView *>::iterator pos = m_lpViews.begin();
	while (pos != m_lpViews.end())
	{
		CCrystalTextView *const pView = *pos++;
		pView->UpdateView(pSource, pContext, dwUpdateFlags, nLineIndex);
	}
}

/**
 * @brief Delete text from the buffer.
 * @param [in] pSource A view from which the text is deleted.
 * @param [in] nStartLine Starting line for the deletion.
 * @param [in] nStartChar Starting char position for the deletion.
 * @param [in] nEndLine Ending line for the deletion.
 * @param [in] nEndChar Ending char position for the deletion.
 * @return TRUE if the insertion succeeded, FALSE otherwise.
 * @note Line numbers are apparent (screen) line numbers, not real
 * line numbers in the file.
 */
void CCrystalTextBuffer::InternalDeleteText(
	CCrystalTextView *pSource,
	int nStartLine, int nStartChar,
	int nEndLine, int nEndChar)
{
  ASSERT(m_bInit);             //  Text buffer not yet initialized.
  //  You must call InitNew() or LoadFromFile() first!

  ASSERT(nStartLine >= 0 && nStartLine < GetLineCount());
  ASSERT(nStartChar >= 0 && nStartChar <= m_aLines[nStartLine].Length());
  ASSERT(nEndLine >= 0 && nEndLine < GetLineCount());
  ASSERT(nEndChar >= 0 && nEndChar <= m_aLines[nEndLine].Length());
  ASSERT(nStartLine < nEndLine || nStartLine == nEndLine && nStartChar <= nEndChar);
  // some edit functions (delete...) should do nothing when there is no selection.
  // assert to be sure to catch these 'do nothing' cases.
  ASSERT(nStartLine != nEndLine || nStartChar != nEndChar);
  ASSERT(!m_bReadOnly);

  CDeleteContext context;
  context.m_ptStart.y = nStartLine;
  context.m_ptStart.x = nStartChar;
  context.m_ptEnd.y = nEndLine;
  context.m_ptEnd.x = nEndChar;
  if (nStartLine == nEndLine)
    {
      // delete part of one line
      m_aLines[nStartLine].Delete(nStartChar, nEndChar);

      if (pSource!=NULL)
        UpdateViews (pSource, &context, UPDATE_SINGLELINE | UPDATE_HORZRANGE, nStartLine);
    }
  else
    {
      // delete multiple lines
      const int nRestCount = m_aLines[nEndLine].FullLength() - nEndChar;
      String sTail(m_aLines[nEndLine].GetLine(nEndChar), nRestCount);

      const int nDelCount = nEndLine - nStartLine;
      for (int L = nStartLine + 1; L <= nEndLine; L++)
        m_aLines[L].Clear();
      vector<LineInfo>::iterator iterBegin = m_aLines.begin() + nStartLine + 1;
      vector<LineInfo>::iterator iterEnd = iterBegin + nDelCount;
      m_aLines.erase(iterBegin, iterEnd);

      //  nEndLine is no more valid
      m_aLines[nStartLine].DeleteEnd(nStartChar);
      if (nRestCount > 0)
        {
          AppendLine (nStartLine, sTail.c_str(), sTail.length());
        }

      if (pSource!=NULL)
        UpdateViews (pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE, nStartLine);
    }

  if (!m_bModified)
    SetModified(true);
  //BEGIN SW
  // remember current cursor position as last editing position
  m_ptLastChange = context.m_ptStart;
  //END SW

}

// Remove the last [bytes] characters from specified line, and return them
// (EOL characters are included)
String CCrystalTextBuffer::StripTail(int i, int bytes)
{
	LineInfo & li = m_aLines[i];
	// Must at least take off the EOL characters
	ASSERT(bytes >= li.FullLength() - li.Length());

	const int offset = li.FullLength() - bytes;
	// Must not take off more than exist
	ASSERT(offset >= 0);

	String ret(li.GetLine(offset), bytes);
	li.DeleteEnd(offset);
	return ret;
}


/**
 * @brief Insert text to the buffer.
 * @param [in] pSource A view to which the text is added.
 * @param [in] nLine Line to add the text.
 * @param [in] nPos Position in the line to insert the text.
 * @param [in] pszText The text to insert.
 * @param [in] cchText The length of the text.
 * @param [out] nEndLine Line number of last added line in the buffer.
 * @param [out] nEndChar Character position of the end of the added text
 *   in the buffer.
 * @param [in] nAction Edit action.
 * @param [in] bHistory Save insertion for undo/redo?
 * @return TRUE if the insertion succeeded, FALSE otherwise.
 * @note Line numbers are apparent (screen) line numbers, not real
 * line numbers in the file.
 */
POINT CCrystalTextBuffer::InternalInsertText(CCrystalTextView *pSource,
	int nLine, int nPos, LPCTSTR pszText, int cchText)
{
  ASSERT(m_bInit);             //  Text buffer not yet initialized.
  //  You must call InitNew() or LoadFromFile() first!

  ASSERT(nLine >= 0 && nLine < GetLineCount());
  ASSERT(nPos >= 0 && nPos <= m_aLines[nLine].Length());
  ASSERT(!m_bReadOnly);

  CInsertContext context;
  context.m_ptStart.x = nPos;
  context.m_ptStart.y = nLine;
  int nEndLine = 0;
  int nEndChar = 0;

  int nRestCount = GetFullLineLength(nLine) - nPos;
  String sTail;
  if (nRestCount > 0)
    {
      // remove end of line (we'll put it back on afterwards)
      sTail = StripTail(nLine, nRestCount);
    }


  int nInsertedLines = 0;
  int nCurrentLine = nLine;
  int nTextPos;
  for (;;)
    {
      int haseol = 0;
      nTextPos = 0;
      // advance to end of line
      while (nTextPos < cchText && !LineInfo::IsEol(pszText[nTextPos]))
        nTextPos++;
      // advance after EOL of line
      if (nTextPos < cchText)
        {
          haseol = 1;
          LPCTSTR eol = &pszText[nTextPos];
          nTextPos++;
          if (nTextPos < cchText && LineInfo::IsDosEol(eol))
            nTextPos++;
        }

      // The first line of the new text is appended to the start line
      // All succeeding lines are inserted
      if (nCurrentLine == nLine)
        {
          AppendLine (nLine, pszText, nTextPos);
        }
      else
        {
          InsertLine (pszText, nTextPos, nCurrentLine);
          nInsertedLines ++;
        }


      if (nTextPos == cchText)
        {
          // we just finished our insert
          // now we have to reattach the tail
          if (haseol)
            {
              nEndLine = nCurrentLine+1;
              nEndChar = 0;
            }
          else
            {
              nEndLine = nCurrentLine;
              nEndChar = GetLineLength(nEndLine);
            }
          if (!sTail.empty())
            {
              if (haseol)
              {
                InsertLine(sTail.c_str(), sTail.length(), nEndLine);
                nInsertedLines ++;
              }
              else
                AppendLine(nEndLine, sTail.c_str(), nRestCount);
            }
          if (nEndLine == GetLineCount())
            {
              // We left cursor after last screen line
              // which is an illegal cursor position
              // so manufacture a new trailing line
              InsertLine(_T(""), 0);
              nInsertedLines ++;
            }
          break;
        }

      ++nCurrentLine;
      pszText += nTextPos;
      cchText -= nTextPos;
    }

  // Compute the context : all positions after context.m_ptBegin are
  // shifted accordingly to (context.m_ptEnd - context.m_ptBegin)
  // The begin point is the insertion point.
  // The end point is more tedious : if we insert in a ghost line, we reuse it, 
  // so we insert fewer lines than the number of lines in the text buffer
  if (nEndLine - nLine != nInsertedLines)
    {
      context.m_ptEnd.y = nLine + nInsertedLines;
      context.m_ptEnd.x = GetFullLineLength(context.m_ptEnd.y);
    }
  else
    {
      context.m_ptEnd.x = nEndChar;
      context.m_ptEnd.y = nEndLine;
    }


  if (pSource!=NULL)
    {
      if (nInsertedLines > 0)
        UpdateViews (pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE, nLine);
      else
        UpdateViews (pSource, &context, UPDATE_SINGLELINE | UPDATE_HORZRANGE, nLine);
    }

  if (!m_bModified)
    SetModified(true);

  // remember current cursor position as last editing position
  m_ptLastChange.x = nEndChar;
  m_ptLastChange.y = nEndLine;
  return m_ptLastChange;
}

bool CCrystalTextBuffer::CanUndo() const
{
	ASSERT(m_nUndoPosition >= 0 && m_nUndoPosition <= GetUndoRecordCount());
	return m_nUndoPosition > 0;
}

bool CCrystalTextBuffer::CanRedo() const
{
	ASSERT(m_nUndoPosition >= 0 && m_nUndoPosition <= GetUndoRecordCount());
	return m_nUndoPosition < GetUndoRecordCount();
}

stl_size_t CCrystalTextBuffer::GetUndoActionCode(int & nAction, stl_size_t pos) const
{
	ASSERT(CanUndo());          //  Please call CanUndo() first
	ASSERT((GetUndoRecord(0).m_dwFlags & UNDO_BEGINGROUP) != 0);
	if (pos == 0)
	{
		//  Start from beginning
		pos = m_nUndoPosition;
	}
	else
	{
		ASSERT(pos > 0 && pos < m_nUndoPosition);
		ASSERT((GetUndoRecord(pos).m_dwFlags & UNDO_BEGINGROUP) != 0);
	}
	//  Advance to next undo group
	do
	{
		--pos;
	} while ((GetUndoRecord(pos).m_dwFlags & UNDO_BEGINGROUP) == 0);
	//  Get description
	nAction = GetUndoRecord(pos).m_nAction;
	//  Now, if we stop at zero position, this will be the last action,
	//  since we return (POSITION) nPosition
	return pos;
}

stl_size_t CCrystalTextBuffer::GetRedoActionCode(int & nAction, stl_size_t pos) const
{
	ASSERT(CanRedo());          //  Please call CanRedo() before!
	ASSERT((GetUndoRecord(0).m_dwFlags & UNDO_BEGINGROUP) != 0);
	ASSERT((GetUndoRecord(m_nUndoPosition).m_dwFlags & UNDO_BEGINGROUP) != 0);
	if (pos == 0)
	{
		//  Start from beginning
		pos = m_nUndoPosition;
	}
	else
	{
		ASSERT(pos > m_nUndoPosition);
		ASSERT((GetUndoRecord(pos).m_dwFlags & UNDO_BEGINGROUP) != 0);
	}
	//  Get description
	nAction = GetUndoRecord(pos).m_nAction;
	//  Advance to next undo group
	size_t n = GetUndoRecordCount();
	if (n > 1)
	{
		do
		{
			++pos;
		} while (pos != n && (GetUndoRecord(pos).m_dwFlags & UNDO_BEGINGROUP) == 0);
	}
	return pos < n ? pos : 0;
}

/**
 * @brief Get EOL style string.
 * @param [in] nCRLFMode.
 * @return string of CRLF style.
 */
LPCTSTR CCrystalTextBuffer::GetStringEol(CRLFSTYLE nCRLFMode)
{
	switch(nCRLFMode)
	{
	case CRLF_STYLE_UNIX:
		return _T("\n");
	case CRLF_STYLE_MAC:
		return _T("\r");
	default:
		return _T("\r\n");
	}
}

LPCTSTR CCrystalTextBuffer::GetDefaultEol() const
{
	return GetStringEol(m_nCRLFMode);
}

void CCrystalTextBuffer::SetModified(bool bModified)
{
	m_bModified = bModified;
}

void CCrystalTextBuffer::BeginUndoGroup(BOOL bMergeWithPrevious)
{
	ASSERT(!m_bUndoGroup);
	m_bUndoGroup = TRUE;
	m_bUndoBeginGroup = m_nUndoPosition == 0 || !bMergeWithPrevious;
}

void CCrystalTextBuffer::FlushUndoGroup(CCrystalTextView *pSource)
{
	ASSERT(m_bUndoGroup);
	if (pSource != NULL)
	{
		ASSERT(m_nUndoPosition == GetUndoRecordCount());
		if (m_nUndoPosition > 0)
		{
			const UndoRecord &undo = GetUndoRecord(m_nUndoPosition - 1);
			pSource->OnEditOperation(undo.m_nAction, undo.GetText());
		}
	}
	m_bUndoGroup = FALSE;
}

int CCrystalTextBuffer::FindNextBookmarkLine(int nCurrentLine, int nDirection) const
{
	const int nSize = GetLineCount();
	const int nStartingPoint = nCurrentLine;
	for (;;)
	{
		nCurrentLine += nDirection;
		if (nCurrentLine < 0 || nCurrentLine >= nSize)
			nCurrentLine -= nDirection * nSize;
		if (nCurrentLine == nStartingPoint ||
			(m_aLines[nCurrentLine].m_dwFlags & LF_BOOKMARKS) != 0)
		{
			break;
		}
	}
	return nCurrentLine;
}

//BEGIN SW
POINT CCrystalTextBuffer::GetLastChangePos() const
{
	return m_ptLastChange;
}
//END SW
void CCrystalTextBuffer::RestoreLastChangePos(POINT pt)
{
	m_ptLastChange = pt;
}

int CCrystalTextBuffer::GetTabSize() const
{
	ASSERT( m_nTabSize >= 0 && m_nTabSize <= 64 );
	return m_nTabSize;
}

void CCrystalTextBuffer::SetTabSize(int nTabSize)
{
	ASSERT( nTabSize >= 0 && nTabSize <= 64 );
	m_nTabSize = nTabSize;
}
