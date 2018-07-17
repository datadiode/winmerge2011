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
#include "StdAfx.h"
#include "ccrystaltextbuffer.h"
#include "coretools.h"
#include "VersionData.h"
#include <pcre.h>
#include "wcwidth.h"
#include "modeline-parser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::vector;

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
#ifdef _DEBUG
	m_bInit = false;
#endif
	m_bReadOnly = false;
	m_bModified = false;
	m_nCRLFMode = CRLF_STYLE_DOS;
	m_nUndoPosition = 0;
	m_bUndoGroup = m_bUndoBeginGroup = FALSE;
	m_bInsertTabs = true;
	m_nTabSize = 4;
	m_bSeparateCombinedChars = false;
	m_nParseCookieCount = 0;
	m_nModeLineOverrides = 0;
	//BEGIN SW
	m_ptLastChange.x = m_ptLastChange.y = -1;
	//END SW
	m_dwCurrentRevisionNumber = 0;
	m_dwRevisionNumberOnSave = 0;

	SetTextType(SRC_PLAIN);
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
	std::for_each(m_aLines.begin(), m_aLines.end(), std::mem_fun_ref(&LineInfo::Clear));
	m_aLines.clear();
#ifdef _DEBUG
	m_bInit = false;
#endif
	//BEGIN SW
	m_ptLastChange.x = m_ptLastChange.y = -1;
	//END SW
}

BOOL CCrystalTextBuffer::InitNew(CRLFSTYLE nCrlfStyle)
{
	ASSERT(!m_bInit);
	ASSERT(m_aLines.size() == 0);
	ASSERT(nCrlfStyle >= 0 && nCrlfStyle <= 2);
	InsertLine(NULL, 0);
#ifdef _DEBUG
	m_bInit = true;
#endif
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
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?
	return m_bReadOnly;
}

void CCrystalTextBuffer::SetReadOnly(bool bReadOnly)
{
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?
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
		SetModified();

	return bChanged;
}

// get pointer to any trailing eol characters (pointer to empty string if none)
LPCTSTR CCrystalTextBuffer::GetLineEol(int nLine) const
{
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?
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
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?
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
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?
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
	std::list<CCrystalTextView *>::iterator pos = std::find(m_lpViews.begin(), m_lpViews.end(), pView);
	ASSERT(pos != m_lpViews.end());
	if (pos != m_lpViews.end())
		m_lpViews.erase(pos);
}

int CCrystalTextBuffer::GetParseCookieCount() const
{
	ASSERT(m_nParseCookieCount >= 0 && m_nParseCookieCount <= GetLineCount());
	return m_nParseCookieCount;
}

void CCrystalTextBuffer::SetParseCookieCount(int nParseCookieCount)
{
	ASSERT(nParseCookieCount >= 0 && nParseCookieCount <= GetLineCount());
	m_nParseCookieCount = nParseCookieCount;
}

void CCrystalTextBuffer::UpdateViews(CCrystalTextView *pSource, CUpdateContext *pContext, DWORD dwUpdateFlags, int nLineIndex)
{
	std::list<CCrystalTextView *>::iterator pos = m_lpViews.begin();
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
void CCrystalTextBuffer::InternalDeleteText(CCrystalTextView *pSource,
	int nStartLine, int nStartChar, int nEndLine, int nEndChar)
{
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?

	ASSERT(nStartLine >= 0 && nStartLine < GetLineCount());
	ASSERT(nStartChar >= 0 && nStartChar <= m_aLines[nStartLine].Length());
	ASSERT(nEndLine >= 0 && nEndLine < GetLineCount());
	ASSERT(nEndChar >= 0 && nEndChar <= m_aLines[nEndLine].Length());
	ASSERT(nStartLine < nEndLine || nStartLine == nEndLine && nStartChar <= nEndChar);
	// some edit functions (delete...) should do nothing when there is no selection.
	// assert to be sure to catch these 'do nothing' cases.
	ASSERT(nStartLine != nEndLine || nStartChar != nEndChar);
	ASSERT(!m_bReadOnly);

	if (m_nParseCookieCount > nStartLine)
		m_nParseCookieCount = nStartLine + 1;

	CDeleteContext context;
	context.m_ptStart.y = nStartLine;
	context.m_ptStart.x = nStartChar;
	context.m_ptEnd.y = nEndLine;
	context.m_ptEnd.x = nEndChar;
	if (nStartLine == nEndLine)
	{
		// delete part of one line
		m_aLines[nStartLine].Delete(nStartChar, nEndChar);

		if (pSource)
			UpdateViews(pSource, &context, UPDATE_SINGLELINE | UPDATE_HORZRANGE, nStartLine);
	}
	else
	{
		// delete multiple lines
		const int nRestCount = m_aLines[nEndLine].FullLength() - nEndChar;
		String sTail(m_aLines[nEndLine].GetLine(nEndChar), nRestCount);

		vector<LineInfo>::iterator iterBegin = m_aLines.begin() + nStartLine + 1;
		vector<LineInfo>::iterator iterEnd = m_aLines.begin() + nEndLine + 1;
		std::for_each(iterBegin, iterEnd, std::mem_fun_ref(&LineInfo::Clear));
		m_aLines.erase(iterBegin, iterEnd);

		//  nEndLine is no more valid
		m_aLines[nStartLine].DeleteEnd(nStartChar);
		if (nRestCount > 0)
		{
			AppendLine(nStartLine, sTail.c_str(), sTail.length());
		}
		if (pSource)
			UpdateViews(pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE, nStartLine);
	}

	SetModified();
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
	ASSERT(m_bInit); // Missed to call InitNew() or LoadFromFile() first?

	ASSERT(nLine >= 0 && nLine < GetLineCount());
	ASSERT(nPos >= 0 && nPos <= m_aLines[nLine].Length());
	ASSERT(!m_bReadOnly);

	if (m_nParseCookieCount > nLine)
		m_nParseCookieCount = nLine + 1;

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
			AppendLine(nLine, pszText, nTextPos);
		}
		else
		{
			InsertLine(pszText, nTextPos, nCurrentLine);
			nInsertedLines++;
		}

		if (nTextPos == cchText)
		{
			// we just finished our insert
			// now we have to reattach the tail
			if (haseol)
			{
				nEndLine = nCurrentLine + 1;
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
					nInsertedLines++;
				}
				else
					AppendLine(nEndLine, sTail.c_str(), nRestCount);
			}
			if (nEndLine == GetLineCount())
			{
				// We left cursor after last screen line
				// which is an illegal cursor position
				// so manufacture a new trailing line
				InsertLine(NULL, 0);
				nInsertedLines++;
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

	if (pSource)
	{
		if (nInsertedLines > 0)
			UpdateViews(pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE, nLine);
		else
			UpdateViews(pSource, &context, UPDATE_SINGLELINE | UPDATE_HORZRANGE, nLine);
	}

	SetModified();

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
	stl_size_t n = GetUndoRecordCount();
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
			pSource->OnEditOperation(undo.m_nAction, undo.GetText(), undo.GetTextLength());
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

static const TCHAR *MemSearch(const TCHAR *p, size_t pLen, const TCHAR *q, size_t qLen)
{
	if (qLen > pLen)
		return NULL;
	const TCHAR *pEnd = p + pLen - qLen;
	const TCHAR *qEnd = q + qLen;
	do
	{
		const TCHAR *u = p;
		const TCHAR *v = q;
		do
		{
			if (v == qEnd)
				return p;
		} while (*u++ == *v++);
	} while (++p <= pEnd);
	return NULL;
}

int CCrystalTextBuffer::FindStringHelper(
	LPCTSTR pchFindWhere, UINT cchFindWhere,
	LPCTSTR pchFindWhat, DWORD dwFlags,
	Captures &ovector)
{
	if (dwFlags & FIND_REGEXP)
	{
		const char *errormsg = NULL;
		int erroroffset = 0;
		const OString regexString = HString::Uni(pchFindWhat)->Oct(CP_UTF8);
		pcre *regexp = pcre_compile(regexString.A,
			dwFlags & FIND_MATCH_CASE ?
			PCRE_UTF8 | PCRE_BSR_ANYCRLF :
			PCRE_UTF8 | PCRE_BSR_ANYCRLF | PCRE_CASELESS,
			&errormsg,
			&erroroffset, NULL);
		pcre_extra *pe = NULL;
		if (regexp)
		{
			errormsg = NULL;
			pe = pcre_study(regexp, 0, &errormsg);
		}

		const OString compString = HString::Uni(pchFindWhere, cchFindWhere)->Oct(CP_UTF8);
		int result = pcre_exec(
			regexp, pe, compString.A, compString.ByteLen(), 0, 0, ovector, _countof(ovector));

		if (result >= 0)
		{
			// Convert UTF-8 offsets to WCHAR offsets
			int i = 2 * std::max(result, 1);
			do
			{
				--i;
				ovector[i] = MultiByteToWideChar(CP_UTF8, 0, compString.A, ovector[i], 0, 0);
			} while(i != 0);
		}

		pcre_free(regexp);
		pcre_free(pe);
		return result;
	}
	else
	{
		ASSERT(pchFindWhere != NULL);
		ASSERT(pchFindWhat != NULL);
		const int cchFindWhat = static_cast<int>(_tcslen(pchFindWhat));
		ovector[0] = 0;
		ovector[1] = cchFindWhat;
		while (LPCTSTR pchPos = MemSearch(pchFindWhere, cchFindWhere, pchFindWhat, cchFindWhat))
		{
			int nLen = static_cast<int>(pchPos - pchFindWhere);
			ovector[0] += nLen;
			ovector[1] += nLen;
			if ((dwFlags & FIND_WHOLE_WORD) == 0)
				return 0;
			if (!(nLen > 0 && xisalnum(pchPos[-1]) || xisalnum(pchPos[cchFindWhat])))
				return 0;
			++ovector[0];
			++ovector[1];
			pchFindWhere = pchPos + 1;
		}
		return -1;
	}
	ASSERT(FALSE); // Unreachable
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

DWORD CCrystalTextBuffer::GetFlags() const
{
	return m_dwFlags;
}

void CCrystalTextBuffer::SetFlags(DWORD dwFlags)
{
	m_dwFlags = dwFlags;
}

int CCrystalTextBuffer::GetTabSize() const
{
	ASSERT(m_nTabSize >= 0 && m_nTabSize <= 64);
	return m_nTabSize;
}

bool CCrystalTextBuffer::GetSeparateCombinedChars() const
{
	return m_bSeparateCombinedChars;
}

void CCrystalTextBuffer::SetTabSize(int nTabSize, bool bSeparateCombinedChars)
{
	ASSERT(nTabSize >= 0 && nTabSize <= 64);
	if (!(m_nModeLineOverrides & MODELINE_SET_TAB_WIDTH))
		m_nTabSize = nTabSize;
	m_bSeparateCombinedChars = bSeparateCombinedChars;
	m_nMaxLineLength = -1;
	int const nLineCount = GetLineCount();
	for (int nLineIndex = 0; nLineIndex < nLineCount; ++nLineIndex)
		m_aLines[nLineIndex].m_nActualLineLength = -1;
}

bool CCrystalTextBuffer::GetInsertTabs() const
{
	return m_bInsertTabs;
}

void CCrystalTextBuffer::SetInsertTabs(bool bInsertTabs)
{
	if (!(m_nModeLineOverrides & MODELINE_SET_INSERT_SPACES))
		m_bInsertTabs = bInsertTabs;
}

void CCrystalTextBuffer::ParseModeLine()
{
	m_nModeLineOverrides |= modeline_parser_apply_modeline(this);
}

void CCrystalTextBuffer::ParseEditorConfig(LPCTSTR path)
{
	// TODO
}

/**
 * @brief Return width of specified character
 */
int CCrystalTextBuffer::GetCharWidthFromChar(LPCTSTR pch) const
{
	UINT ch = *pch;

	if (ch >= _T('\x00') && ch <= _T('\x1F'))
		return 3;

	if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
		return 0;

	if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
	{
		/* If the 16 bits following the high surrogate are in the source buffer... */
		UINT ch2 = *++pch;
		/* If it's a low surrogate, convert to UTF32. */
		if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
		{
			ch = ((ch - UNI_SUR_HIGH_START) << 10) + (ch2 - UNI_SUR_LOW_START) + 0x10000;
		}
		else
		{
			ch = '?';
		}
	}
	/*if (ch >= UNI_SUR_MIN && ch <= UNI_SUR_MAX)
		return 5;*/
	// This assumes a fixed width font
	// But the UNICODE case handles double-wide glyphs (primarily Chinese characters)
	int wcwidth = mk_wcwidth(ch);
	if (wcwidth < (m_bSeparateCombinedChars ? 1 : 0))
		wcwidth = 1; // applies to 8-bit control characters in range 0x7F..0x9F
	return wcwidth;
}

/**
 * @brief Get the line length, for cursor movement
 *
 * @note There are at least 4 line lengths:
 * - number of characters (memory, no EOL)
 * - number of characters (memory, with EOL)
 * - number of characters for cursor position (tabs are expanded, no EOL)
 * - number of displayed characters (tabs are expanded, with EOL)
 * Corresponding functions:
 * - GetLineLength
 * - GetFullLineLength
 * - GetLineActualLength
 * - ExpandChars (returns the line to be displayed as a String)
 */
int CCrystalTextBuffer::GetLineActualLength(int nLineIndex)
{
	LineInfo &li = m_aLines[nLineIndex];

	int nActualLength = li.m_nActualLineLength;
	if (nActualLength == -1)
	{
		// Actual line length is not determined yet, let's calculate a little
		nActualLength = 0;
		int const nLength = GetLineLength(nLineIndex);
		LPCTSTR const pszChars = GetLineChars(nLineIndex);
		int const nTabSize = GetTabSize();
		for (int i = 0; i < nLength; i++)
		{
			TCHAR const c = pszChars[i];
			ASSERT(c != _T('\r') && c != _T('\n'));
			if (c == _T('\t'))
				nActualLength += (nTabSize - nActualLength % nTabSize);
			else
				nActualLength += GetCharWidthFromChar(pszChars + i);
		}
		li.m_nActualLineLength = nActualLength;
	}
	return nActualLength;
}

int CCrystalTextBuffer::GetMaxLineLength(bool bRecalc)
{
	if (m_nMaxLineLength == -1 || bRecalc)
	{
		m_nMaxLineLength = 0;
		int const nLineCount = GetLineCount();
		for (int i = 0; i < nLineCount; ++i)
		{
			int const nActualLength = GetLineActualLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		}
	}
	return m_nMaxLineLength;
}

// Tabsize is commented out since we have only GUI setting for it now.
// Not removed because we may later want to have per-filetype settings again.
// See ccrystaltextview.h for table declaration.
CCrystalTextBuffer::TextDefinition CCrystalTextBuffer::m_StaticSourceDefs[] =
{
	SRC_PLAIN, _T("Plain"), _T("txt;doc;diz"), &ParseLinePlain, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T(""),
	SRC_ASP, _T("ASP"), _T("asp;aspx"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_BASIC), /*2,*/ _T(""), _T(""), _T("'"),
	SRC_BASIC, _T("Basic"), _T("bas;vb;frm;dsm;cls;ctl;pag;dsr"), &ParseLineBasic, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("\'"),
	SRC_VBSCRIPT, _T("VBScript"), _T("vbs"), &ParseLineBasic, SRCOPT_AUTOINDENT|SRCOPT_COOKIE(COOKIE_PARSER_VBSCRIPT), /*4,*/ _T(""), _T(""), _T("\'"),
	SRC_BATCH, _T("Batch"), _T("bat;btm;cmd"), &ParseLineBatch, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("rem "),
	SRC_C, _T("C"), _T("c;cc;cpp;cxx;h;hpp;hxx;hm;inl;rh;tlh;tli;xs"), &ParseLineC, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSHARP, _T("C#"), _T("cs"), &ParseLineCSharp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSHTML, _T("CSHTML"), _T("cshtml"), &ParseLineRazor, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_CSS, _T("CSS"), _T("css"), &ParseLineCss, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T(""),
	SRC_DCL, _T("DCL"), _T("dcl;dcc"), &ParseLineDcl, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_FORTRAN, _T("Fortran"), _T("f;f90;f9p;fpp;for;f77"), &ParseLineFortran, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT, /*8,*/ _T(""), _T(""), _T("!"),
	SRC_GO, _T("Go"), _T("go"), &ParseLineGo, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_HTML, _T("HTML"), _T("html;htm;shtml;ihtml;ssi;stm;stml"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_INI, _T("INI"), _T("ini;reg;vbp;isl"), &ParseLineIni, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_EOLNUNIX, /*2,*/ _T(""), _T(""), _T(";"),
	SRC_INNOSETUP, _T("InnoSetup"), _T("iss"), &ParseLineInnoSetup, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("{"), _T("}"), _T(";"),
	SRC_INSTALLSHIELD, _T("InstallShield"), _T("rul"), &ParseLineIS, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JAVA, _T("Java"), _T("java;jav"), &ParseLineJava, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_JAVA), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JSCRIPT, _T("JavaScript"), _T("js;json"), &ParseLineJava, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_JSCRIPT), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_JSP, _T("JSP"), _T("jsp"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_JAVA), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_LISP, _T("AutoLISP"), _T("lsp;dsl"), &ParseLineLisp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_LUA, _T("LUA"), _T("lua"), &ParseLineLua, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_MWSL, _T("MWSL"), _T("mwsl"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_MWSL), /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_NSIS, _T("NSIS"), _T("nsi;nsh"), &ParseLineNsis, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T(";"),
	SRC_PASCAL, _T("Pascal"), _T("pas"), &ParseLinePascal, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("{"), _T("}"), _T(""),
	SRC_PERL, _T("Perl"), _T("pl;pm;plx"), &ParseLinePerl, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_PHP, _T("PHP"), _T("php;php3;php4;php5;phtml"), &ParseLineAsp, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_COOKIE(COOKIE_PARSER_PHP), /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_PO, _T("PO"), _T("po;pot"), &ParseLinePo, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_POWERSHELL, _T("PowerShell"), _T("ps1"), &ParseLinePowerShell, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T(""), _T(""), _T("#"),
	SRC_PYTHON, _T("Python"), _T("py"), &ParseLinePython, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_REXX, _T("REXX"), _T("rex;rexx"), &ParseLineRexx, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_RSRC, _T("Resources"), _T("rc;dlg;r16;r32;rc2"), &ParseLineRsrc, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_RUBY, _T("Ruby"), _T("rb;rbw;rake;gemspec"), &ParseLineRuby, SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_RUST, _T("Rust"), _T("rs"), &ParseLineRust, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_SGML, _T("Sgml"), _T("sgm;sgml"), &ParseLineSgml, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("<!--"), _T("-->"), _T(""),
	SRC_SH, _T("Shell"), _T("sh;conf"), &ParseLineSh, SRCOPT_INSERTTABS|SRCOPT_AUTOINDENT|SRCOPT_EOLNUNIX, /*4,*/ _T(""), _T(""), _T("#"),
	SRC_SIOD, _T("SIOD"), _T("scm"), &ParseLineSiod, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T(";|"), _T("|;"), _T(";"),
	SRC_SQL, _T("SQL"), _T("sql"), &ParseLineSql, SRCOPT_AUTOINDENT, /*4,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_TCL, _T("TCL"), _T("tcl"), &ParseLineTcl, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI|SRCOPT_EOLNUNIX, /*2,*/ _T(""), _T(""), _T("#"),
	SRC_TEX, _T("TEX"), _T("tex;sty;clo;ltx;fd;dtx"), &ParseLineTex, SRCOPT_AUTOINDENT, /*4,*/ _T(""), _T(""), _T("%"),
	SRC_VERILOG, _T("Verilog"), _T("v;vh"), &ParseLineVerilog, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("/*"), _T("*/"), _T("//"),
	SRC_VHDL, _T("VHDL"), _T("vhd;vhdl;vho"), &ParseLineVhdl, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T(""), _T(""), _T("--"),
	SRC_XML, _T("XML"), _T("xml;dtd"), &ParseLineXml, SRCOPT_AUTOINDENT|SRCOPT_BRACEANSI, /*2,*/ _T("<!--"), _T("-->"), _T("")
}, *CCrystalTextBuffer::m_SourceDefs = m_StaticSourceDefs;

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextView construction/destruction

BOOL CCrystalTextBuffer::ScanParserAssociations(LPTSTR p)
{
	p = EatPrefix(p, _T("version="));
	if (p == NULL)
		return FALSE;
	if (_tcsicmp(p, _T("ignore")) != 0)
	{
		ULARGE_INTEGER version = { 0xFFFFFFFF, 0xFFFFFFFF };
		do
		{
			version.QuadPart <<= 16;
			version.QuadPart |= _tcstoul(p, &p, 10);
		} while (*p && *p++ == _T('.'));
		while (HIWORD(version.HighPart) == 0xFFFF)
			version.QuadPart <<= 16;
		if (VS_FIXEDFILEINFO const *const pVffInfo =
			reinterpret_cast<const VS_FIXEDFILEINFO *>(CVersionData::Load()->Data()))
		{
			if (pVffInfo->dwFileVersionLS != version.LowPart ||
				pVffInfo->dwFileVersionMS != version.HighPart)
			{
				return FALSE;
			}
		}
	}
	p += _tcslen(p) + 1;
	struct tagTextDefinition *defs = new tagTextDefinition[_countof(m_StaticSourceDefs)];
	memcpy(defs, m_StaticSourceDefs, sizeof m_StaticSourceDefs);
	m_SourceDefs = defs;
	while (LPTSTR q = _tcschr(p, _T('=')))
	{
		const size_t r = _tcslen(q);
		*q++ = _T('\0');
		size_t i;
		for (i = 0 ; i < _countof(m_StaticSourceDefs) ; ++i)
		{
			tagTextDefinition &def = defs[i];
			if (_tcsicmp(def.name, p) == 0)
			{
				// Look for additional tags at end of entry
				while (LPTSTR t = _tcsrchr(q, _T(':')))
				{
					*t++ = '\0';
					if (DWORD dwStartCookie = ScriptCookie(t, 0))
					{
						def.flags &= ~SRCOPT_COOKIE(COOKIE_PARSER);
						def.flags |= dwStartCookie << 4;
					}
					else if (PathMatchSpec(t, _T("HTML4")))
						def.flags |= SRCOPT_HTML4_LEXIS;
					else if (PathMatchSpec(t, _T("HTML5")))
						def.flags |= SRCOPT_HTML5_LEXIS;
				}
				def.exts = _tcsdup(q);
				break;
			}
		}
		ASSERT(i < _countof(m_StaticSourceDefs));
		p = q + r;
	}
	return TRUE;
}

void CCrystalTextBuffer::DumpParserAssociations(LPTSTR p)
{
	if (LPCWSTR version = CVersionData::Load()->
		Find(L"StringFileInfo")->First()->Find(L"FileVersion")->Data())
	{
		p += wsprintf(p, _T("version=%ls"), version) + 1;
	}
	TextDefinition *def = m_SourceDefs;
	do
	{
		p += wsprintf(p, _T("%-16s= %s"), def->name, def->exts) + 1;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	*p = _T('\0');
}

void CCrystalTextBuffer::FreeParserAssociations()
{
	if (m_SourceDefs != m_StaticSourceDefs)
	{
		size_t i;
		for (i = 0 ; i < _countof(m_StaticSourceDefs) ; ++i)
		{
			TextDefinition &def = m_SourceDefs[i];
			if (def.exts != m_StaticSourceDefs[i].exts)
				free(const_cast<LPTSTR>(def.exts));
		}
		delete[] m_SourceDefs;
		m_SourceDefs = m_StaticSourceDefs;
	}
}

CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::GetTextType(int index)
{
	if (index >= 0 && index < _countof(m_StaticSourceDefs))
		return m_SourceDefs + index;
	return NULL;
}

CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::GetTextType(LPCTSTR pszExt)
{
	TextDefinition *def = m_SourceDefs;
	do if (PathMatchSpec(pszExt, def->exts))
	{
		return def;
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::SetTextType(LPCTSTR pszExt)
{
	m_CurSourceDef = m_SourceDefs;
	TextDefinition *def = GetTextType(pszExt);
	return SetTextType(def);
}

CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::SetTextType(TextType enuType)
{
	m_CurSourceDef = m_SourceDefs;
	TextDefinition *def = m_SourceDefs;
	do if (def->type == enuType)
	{
		return SetTextType(def);
	} while (++def < m_SourceDefs + _countof(m_StaticSourceDefs));
	return NULL;
}

CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::SetTextType(TextDefinition *def)
{
	if (def != NULL && m_CurSourceDef != def)
	{
		m_CurSourceDef = def;
		InitParseCookie();
		SetFlags(def->flags);
	}
	return def;
}

// Analyze the first line of file to detect its type
// Mainly it works for xml files
CCrystalTextBuffer::TextDefinition *CCrystalTextBuffer::SetTextTypeByContent(LPCTSTR pszContent)
{
	Captures captures;
	if (FindStringHelper(pszContent, static_cast<UINT>(_tcslen(pszContent)),
		_T("^\\s*\\<\\?xml\\s+.+?\\?\\>\\s*$"), FIND_REGEXP, captures) >= 0)
	{
		return SetTextType(SRC_XML);
	}
	return NULL;
}

void CCrystalTextBuffer::InitParseCookie()
{
	m_nMaxLineLength = -1;
	if (m_CurSourceDef)
	{
		SetParseCookieCount(1);
		m_aLines[0].m_cookie = TextBlock::Cookie(m_CurSourceDef->flags & COOKIE_PARSER_GLOBAL);
	}
	else
	{
		SetParseCookieCount(0);
	}
}

TextBlock::Cookie CCrystalTextBuffer::GetParseCookie(int nLineIndex)
{
	int const nLineCount = GetLineCount();
	int L = GetParseCookieCount() - 1;
	LineInfo const &li = m_aLines[nLineIndex < L ? nLineIndex : L];
	TextBlock::Cookie cookie = li.m_cookie;
	TextBlock::Array rBlocks(NULL);
	ASSERT(!cookie.Empty());
	while (L < nLineIndex)
	{
		ParseLine(cookie, L, rBlocks);
		ASSERT(!cookie.Empty());
		if (++L < nLineCount)
			m_aLines[L].m_cookie = cookie;
	}
	SetParseCookieCount(L < nLineCount ? L + 1 : nLineCount);
	return cookie;
}

void CCrystalTextBuffer::ParseLine(TextBlock::Cookie &cookie, int nLineIndex, TextBlock::Array &pBuf)
{
	if (LPCTSTR const pszChars = GetLineChars(nLineIndex))
	{
		int const nLength = GetLineLength(nLineIndex);
		(this->*m_CurSourceDef->ParseLineX)(cookie, pszChars, nLength, -1, pBuf);
	}
}

DWORD CCrystalTextBuffer::ScriptCookie(LPCTSTR lang, DWORD defval)
{
	if (PathMatchSpec(lang, _T("VB")))
		return COOKIE_PARSER_BASIC;
	if (PathMatchSpec(lang, _T("C#")))
		return COOKIE_PARSER_CSHARP;
	if (PathMatchSpec(lang, _T("JAVA")))
		return COOKIE_PARSER_JAVA;
	if (PathMatchSpec(lang, _T("PERL")))
		return COOKIE_PARSER_PERL;
	if (PathMatchSpec(lang, _T("PHP")))
		return COOKIE_PARSER_PHP;
	if (PathMatchSpec(lang, _T("JS*;JAVAS*")))
		return COOKIE_PARSER_JSCRIPT;
	if (PathMatchSpec(lang, _T("VBS*")))
		return COOKIE_PARSER_VBSCRIPT;
	if (PathMatchSpec(lang, _T("CSS")))
		return COOKIE_PARSER_CSS;
	if (PathMatchSpec(lang, _T("MWSL")))
		return COOKIE_PARSER_MWSL;
	if (PathMatchSpec(lang, _T("x-jquery-tmpl")))
		return 0;
	return defval;
}

TextBlock::ParseProc CCrystalTextBuffer::ScriptParseProc(DWORD dwCookie)
{
	switch (dwCookie & COOKIE_PARSER)
	{
	case COOKIE_PARSER_BASIC:
	case COOKIE_PARSER_VBSCRIPT:
		return &ParseLineBasic;
	case COOKIE_PARSER_CSHARP:
		return &ParseLineCSharp;
	case COOKIE_PARSER_JAVA:
	case COOKIE_PARSER_JSCRIPT:
	case COOKIE_PARSER_MWSL:
		return &ParseLineJava;
	case COOKIE_PARSER_PERL:
		return &ParseLinePerl;
	case COOKIE_PARSER_PHP:
		return &ParseLinePhp;
	case COOKIE_PARSER_CSS:
		return &ParseLineCss;
	}
	return &ParseLineUnknown;
}

void CCrystalTextBuffer::ParseLinePlain(TextBlock::Cookie &dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
}

void CCrystalTextBuffer::ParseLineUnknown(TextBlock::Cookie &dwCookie, LPCTSTR const pszChars, int const nLength, int I, TextBlock::Array &pBuf)
{
	pBuf.DefineBlock(I + 1, COLORINDEX_FUNCNAME);
}
