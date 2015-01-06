/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  GhostTextBuffer.cpp
 *
 * @brief Implementation of GhostTextBuffer class.
 */
#include "StdAfx.h"
#include "GhostTextBuffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
#define _ADVANCED_BUGCHECK  1
#endif

using std::vector;

/**
 * @brief Constructor.
 */
CGhostTextBuffer::CGhostTextBuffer()
{
}

/**
 * @brief Initialize a new buffer.
 * @param [in] nCrlfStyle EOL style for the buffer.
 * @return TRUE if the initialization succeeded.
 */
BOOL CGhostTextBuffer::InitNew(CRLFSTYLE nCrlfStyle)
{
	return CCrystalTextBuffer::InitNew(nCrlfStyle);
}

void CGhostTextBuffer::FreeAll()
{
	m_aUndoBuf.clear();
	CCrystalTextBuffer::FreeAll();
}

/**
 * @brief Insert a ghost line.
 * @param [in] pSource View into which to insert the line.
 * @param [in] nLine Line (apparent/screen) where to insert the ghost line.
 * @return TRUE if the insertion succeeded, FALSE otherwise.
 */
BOOL CGhostTextBuffer::InternalInsertGhostLine(CCrystalTextView *pSource, int nLine)
{
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	//  You must call InitNew() or LoadFromFile() first!

	ASSERT(nLine >= 0 && nLine <= GetLineCount());
	if (m_bReadOnly)
		return FALSE;

	CInsertContext context;
	context.m_ptStart.x = 0;
	context.m_ptStart.y = nLine;
	context.m_ptEnd.x = 0;
	context.m_ptEnd.y = nLine + 1;

	CCrystalTextBuffer::InsertLine (_T(""), 0, nLine);
	if (pSource != NULL)
		UpdateViews (pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE, nLine);

	if (!m_bModified)
		SetModified(true);

	OnNotifyLineHasBeenEdited(nLine);
	return TRUE;
}

/** InternalDeleteGhostLine accepts only apparent line numbers */
/**
 * @brief Delete a ghost line.
 * @param [in] pSource View into which to insert the line.
 * @param [in] nLine Line index where to insert the ghost line.
 * @return TRUE if the deletion succeeded, FALSE otherwise.
 * @note @p nLine must be an apparent line number (ghost lines added).
 */
BOOL CGhostTextBuffer::InternalDeleteGhostLine(CCrystalTextView *pSource, int nLine, int nCount)
{
	ASSERT(m_bInit);             //  Text buffer not yet initialized.
	//  You must call InitNew() or LoadFromFile() first!
	ASSERT(nLine >= 0 && nLine <= GetLineCount());

	if (m_bReadOnly)
		return FALSE;
	if (nCount == 0)
		return TRUE;

	CDeleteContext context;
	context.m_ptStart.y = nLine;
	context.m_ptStart.x = 0;
	context.m_ptEnd.y = nLine + nCount;
	context.m_ptEnd.x = 0;

	for (int i = nLine ; i < nLine + nCount; i++)
	{
		ASSERT(GetLineFlags(i) & LF_GHOST);
		m_aLines[i].Clear();
	}

	vector<LineInfo>::iterator iterBegin = m_aLines.begin() + nLine;
	vector<LineInfo>::iterator iterEnd = iterBegin + nCount;
	m_aLines.erase(iterBegin, iterEnd);

	if (pSource != NULL)
	{
		// The last parameter is optimization - don't recompute lines preceeding
		// the removed line.
		if (nLine == GetLineCount())
		{
			UpdateViews (pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE,
					GetLineCount() - 1);
		}
		else
		{
			UpdateViews (pSource, &context, UPDATE_HORZRANGE | UPDATE_VERTRANGE,
					nLine);
		}
	}

	if (!m_bModified)
		SetModified(true);

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// undo/redo functions

bool CGhostTextBuffer::Undo(CCrystalTextView *pSource, POINT &ptCursorPos)
{
	ASSERT(CanUndo());
	ASSERT((m_aUndoBuf[0].m_dwFlags & UNDO_BEGINGROUP) != 0);
	DWORD dwFlags;
	do
	{
		GhostUndoRecord &ur = m_aUndoBuf[--m_nUndoPosition];
		dwFlags = ur.m_dwFlags;
		// Undo records are stored in file line numbers
		// and must be converted to apparent (screen) line numbers for use
		POINT apparent_ptStartPos = ur.m_ptStartPos;
		apparent_ptStartPos.y = ComputeApparentLine(ur.m_ptStartPos.y, ur.m_ptStartPos_nGhost);
		POINT apparent_ptEndPos = ur.m_ptEndPos;
		apparent_ptEndPos.y = ComputeApparentLine(ur.m_ptEndPos.y, ur.m_ptEndPos_nGhost);

		if (ur.m_ptStartPos_nGhost > 0)
			// if we need a ghost line at position apparent_ptStartPos.y
			if (apparent_ptStartPos.y >= GetLineCount() || (GetLineFlags(apparent_ptStartPos.y) & LF_GHOST) == 0)
			{
				// if we don't find it, we insert it 
				InsertGhostLine(pSource, apparent_ptStartPos.y);
				// and recompute apparent_ptEndPos
				apparent_ptEndPos.y = ComputeApparentLine(ur.m_ptEndPos.y, ur.m_ptEndPos_nGhost);
			}

		// EndPos defined only for UNDO_INSERT (when we delete)
		if (dwFlags & UNDO_INSERT && ur.m_ptEndPos_nGhost > 0)
			// if we need a ghost line at position apparent_ptStartPos.y
			if (apparent_ptEndPos.y >= GetLineCount() || (GetLineFlags(apparent_ptEndPos.y) & LF_GHOST) == 0)
			{
				// if we don't find it, we insert it
				InsertGhostLine (pSource, apparent_ptEndPos.y);
			}

		if (dwFlags & UNDO_INSERT)
		{
			// WINMERGE -- Check that text in undo buffer matches text in
			// file buffer. If not, then rescan() has moved lines and undo
			// is skipped.

			// we need to put the cursor before the deleted section
			String text;
			ur.m_redo_ptEndPos.x = apparent_ptEndPos.x;
			ur.m_redo_ptEndPos.y = ComputeRealLineAndGhostAdjustment (apparent_ptEndPos.y, ur.m_redo_ptEndPos_nGhost);

			// flags are going to be deleted so we store them now
			const DWORD bLastLineGhost = GetLineFlags(apparent_ptEndPos.y) & LF_GHOST;

			const int size = GetLineCount();
			if ((apparent_ptStartPos.y < size) &&
				(apparent_ptStartPos.x <= m_aLines[apparent_ptStartPos.y].Length()) &&
				(apparent_ptEndPos.y < size) &&
				(apparent_ptEndPos.x <= m_aLines[apparent_ptEndPos.y].Length()))
			{
				GetText(apparent_ptStartPos.y, apparent_ptStartPos.x,
					apparent_ptEndPos.y, apparent_ptEndPos.x, text, _T("\n"));
				if (ur.VerifyText(text))
				{
					InternalDeleteText(pSource, 
						apparent_ptStartPos.y, apparent_ptStartPos.x,
						apparent_ptEndPos.y, apparent_ptEndPos.x);
					m_dwCurrentRevisionNumber++;
					ptCursorPos = apparent_ptStartPos;
				}
				else
				{
					//..Try to ensure that we are undoing correctly...
					//  Just compare the text as it was before Undo operation
#ifdef _ADVANCED_BUGCHECK
					ASSERT(0);
#endif
					break;
				}
			}
			else
			{
				break;
			}

			OnNotifyLineHasBeenEdited(apparent_ptStartPos.y);

			LineInfo &li = m_aLines[apparent_ptStartPos.y];
			// default : the remaining line inherits the status of the last line of the deleted block
			if (bLastLineGhost)
			{
				li.m_dwFlags |= LF_GHOST;
				li.m_nSkippedLines = 0;
			}
			else
			{
				li.m_dwFlags &= ~LF_GHOST;
			}
			// the number of real lines must be the same before the action and after undo
			int nNumberDeletedRealLines = ur.m_ptEndPos.y - ur.m_ptStartPos.y;
			if (nNumberDeletedRealLines == ur.m_nRealLinesCreated)
				;
			else if (nNumberDeletedRealLines == ur.m_nRealLinesCreated - 1)
			{
				// we inserted in a ghost line (which then became real), we must send it back to its world
				li.m_dwFlags |= LF_GHOST;
				li.m_nSkippedLines = 0;
			}
			else
				ASSERT(0);

			// it is not easy to know when Recompute so we do it always
			RecomputeRealityMapping();

			RecomputeEOL(pSource, apparent_ptStartPos.y, apparent_ptStartPos.y);
		}
		else
		{
			ptCursorPos = InternalInsertText(pSource, 
				apparent_ptStartPos.y, apparent_ptStartPos.x,
				ur.GetText(), ur.GetTextLength());
			m_dwCurrentRevisionNumber++;

			// for the flags, the logic is nearly the same as in insertText
			int bFirstLineGhost = ((GetLineFlags(apparent_ptStartPos.y) & LF_GHOST) != 0);
			// when inserting an EOL terminated text into a ghost line,
			// there is a dicrepancy between nInsertedLines and nEndLine-nRealLine
			const bool bDiscrepancyInInsertedLines = bFirstLineGhost && ptCursorPos.x == 0;

			int i;
			for (i = apparent_ptStartPos.y ; i < ptCursorPos.y ; i++)
				OnNotifyLineHasBeenEdited(i);
			if (!bDiscrepancyInInsertedLines)
				OnNotifyLineHasBeenEdited(i);

			// We know the number of real lines in the deleted block (including partial lines for extremities)
			// there may be more lines (difficult to explain) then they must be ghost
			for (i = apparent_ptStartPos.y ; i < apparent_ptStartPos.y + ur.m_nRealLinesInDeletedBlock ; i++)
				m_aLines[i].m_dwFlags &= ~LF_GHOST;
			for ( ; i <= ptCursorPos.y ; i++)
			{
				LineInfo &li = m_aLines[i];
				li.m_dwFlags |= LF_GHOST;
				li.m_nSkippedLines = 0;
			}

			// it is not easy to know when Recompute so we do it always
			RecomputeRealityMapping();

			RecomputeEOL(pSource, apparent_ptStartPos.y, ptCursorPos.y);
		}

		// store infos for redo
		ur.m_redo_ptStartPos.x = apparent_ptStartPos.x;
		ur.m_redo_ptStartPos.y = ComputeRealLineAndGhostAdjustment(apparent_ptStartPos.y, ur.m_redo_ptStartPos_nGhost);
		if (dwFlags & UNDO_INSERT)
		{
			ur.m_redo_ptEndPos.x = -1;
			ur.m_redo_ptEndPos.y = 0;
		}
		else
		{
			ur.m_redo_ptEndPos.x = m_ptLastChange.x;
			ur.m_redo_ptEndPos.y = ComputeRealLineAndGhostAdjustment(m_ptLastChange.y, ur.m_redo_ptEndPos_nGhost);
		}

		// restore line revision numbers
		stl_size_t naSavedRevisonNumbersSize = ur.m_paSavedRevisonNumbers.size();
		for (stl_size_t i = 0; i < naSavedRevisonNumbersSize; i++)
			m_aLines[apparent_ptStartPos.y + i].m_dwRevisionNumber = ur.m_paSavedRevisonNumbers[i];

	} while ((dwFlags & UNDO_BEGINGROUP) == 0);
	if (m_bModified && m_nSyncPosition == m_nUndoPosition)
		SetModified(false);
	if (!m_bModified && m_nSyncPosition != m_nUndoPosition)
		SetModified(true);
	if ((dwFlags & UNDO_BEGINGROUP) != 0)
		return true;
	// If the Undo failed, clear the entire Undo/Redo stack
	// Not only can we not Redo the failed Undo, but the Undo
	// may have partially completed (if in a group)
	m_nUndoPosition = 0;
	m_aUndoBuf.clear();
	return false;
}

bool CGhostTextBuffer::Redo(CCrystalTextView *pSource, POINT &ptCursorPos)
{
	ASSERT(CanRedo());
	ASSERT((m_aUndoBuf[0].m_dwFlags & UNDO_BEGINGROUP) != 0);
	ASSERT((m_aUndoBuf[m_nUndoPosition].m_dwFlags & UNDO_BEGINGROUP) != 0);

	while(1)
	{
		const GhostUndoRecord &ur = m_aUndoBuf[m_nUndoPosition];
		POINT apparent_ptStartPos = ur.m_redo_ptStartPos;
		apparent_ptStartPos.y = ComputeApparentLine (ur.m_redo_ptStartPos.y, ur.m_redo_ptStartPos_nGhost);
		POINT apparent_ptEndPos = ur.m_redo_ptEndPos;
		apparent_ptEndPos.y = ComputeApparentLine (ur.m_redo_ptEndPos.y, ur.m_redo_ptEndPos_nGhost);

		if (ur.m_redo_ptStartPos_nGhost > 0) 
			// we need a ghost line at position apparent_ptStartPos.y
			if (apparent_ptStartPos.y >= GetLineCount() || (GetLineFlags(apparent_ptStartPos.y) & LF_GHOST) == 0)
			{
				// if we don't find it, we insert it 
				InsertGhostLine (pSource, apparent_ptStartPos.y);
				// and recompute apparent_ptEndPos
				apparent_ptEndPos.y = ComputeApparentLine (ur.m_redo_ptEndPos.y, ur.m_redo_ptEndPos_nGhost);
			} 

		// EndPos defined only for UNDO_DELETE (when we delete)
		if ((ur.m_dwFlags & UNDO_INSERT) == 0 && ur.m_redo_ptEndPos_nGhost > 0)
			// we need a ghost line at position apparent_ptStartPos.y
			if (apparent_ptEndPos.y >= GetLineCount() || (GetLineFlags(apparent_ptEndPos.y) & LF_GHOST) == 0)
			{
				// if we don't find it, we insert it
				InsertGhostLine (pSource, apparent_ptEndPos.y);
			}

		// now we can use normal (CGhostTextBuffer::) insertTxt or deleteText
		if (ur.m_dwFlags & UNDO_INSERT)
		{
			ptCursorPos = InsertText(pSource, apparent_ptStartPos.y, apparent_ptStartPos.x,
				ur.GetText(), ur.GetTextLength(), 0, FALSE);
		}
		else
		{
#ifdef _ADVANCED_BUGCHECK
			String text;
			GetText(apparent_ptStartPos.y, apparent_ptStartPos.x,
				apparent_ptEndPos.y, apparent_ptEndPos.x, text, _T("\n"));
			ASSERT(ur.VerifyText(text));
#endif
			DeleteText(pSource, apparent_ptStartPos.y, apparent_ptStartPos.x, 
				apparent_ptEndPos.y, apparent_ptEndPos.x, 0, FALSE);
			ptCursorPos = apparent_ptStartPos;
		}
		m_nUndoPosition++;
		if (m_nUndoPosition == m_aUndoBuf.size ())
			break;
		if ((m_aUndoBuf[m_nUndoPosition].m_dwFlags & UNDO_BEGINGROUP) != 0)
			break;
	}

	if (m_bModified && m_nSyncPosition == m_nUndoPosition)
		SetModified(false);
	if (!m_bModified && m_nSyncPosition != m_nUndoPosition)
		SetModified(true);
	return true;
}


/** The POINT received parameters are apparent (on screen) line numbers */
UndoRecord &CGhostTextBuffer::AddUndoRecord(BOOL bInsert, const POINT &ptStartPos,
		const POINT &ptEndPos, LPCTSTR pszText, int cchText,
		int nRealLinesChanged, int nActionType)
{
	//  Forgot to call BeginUndoGroup()?
	ASSERT(m_bUndoGroup);
	ASSERT(m_aUndoBuf.size () == 0 || (m_aUndoBuf[0].m_dwFlags & UNDO_BEGINGROUP) != 0);

	//  Strip unnecessary undo records (edit after undo wipes all potential redo records)
	size_t nBufSize = m_aUndoBuf.size();
	if (m_nUndoPosition < nBufSize)
	{
		m_aUndoBuf.resize(m_nUndoPosition);
	}

	//  Add new record
	GhostUndoRecord ur;
	ur.m_dwFlags = bInsert ? UNDO_INSERT : 0;
	ur.m_nAction = nActionType;
	if (m_bUndoBeginGroup)
	{
		ur.m_dwFlags |= UNDO_BEGINGROUP;
		m_bUndoBeginGroup = FALSE;
	}
	ur.m_ptStartPos = ptStartPos;
	ur.m_ptEndPos = ptEndPos;
	ur.m_ptStartPos.y = ComputeRealLineAndGhostAdjustment(ptStartPos.y, ur.m_ptStartPos_nGhost);
	ur.m_ptEndPos.y = ComputeRealLineAndGhostAdjustment(ptEndPos.y, ur.m_ptEndPos_nGhost);
	if (bInsert)
		ur.m_nRealLinesCreated = nRealLinesChanged;
	else
		ur.m_nRealLinesInDeletedBlock = nRealLinesChanged;
	ur.SetText(pszText, cchText);

	// Optimize memory allocation
	if (m_aUndoBuf.capacity() == m_aUndoBuf.size())
	{
		if (m_aUndoBuf.size() == 0)
			m_aUndoBuf.reserve(16);
		else if (m_aUndoBuf.size() <= 1024)
			m_aUndoBuf.reserve(m_aUndoBuf.size() * 2);
		else
			m_aUndoBuf.reserve(m_aUndoBuf.size() + 1024);
	}
	m_aUndoBuf.push_back (ur);
	m_nUndoPosition = (int) m_aUndoBuf.size ();
	return m_aUndoBuf.back();
}




////////////////////////////////////////////////////////////////////////////
// edition functions

/**
 * @brief Insert text to the buffer.
 * @param [in] pSource View into which to insert the text.
 * @param [in] nLine Line number (apparent/screen) where the insertion starts.
 * @param [in] nPos Character position where the insertion starts.
 * @param [in] pszText The text to insert.
 * @param [out] nEndLine Line number of last added line in the buffer.
 * @param [out] nEndChar Character position of the end of the added text
 *   in the buffer.
 * @param [in] nAction Edit action.
 * @param [in] bHistory Save insertion for undo/redo?
 * @return TRUE if the insertion succeeded, FALSE otherwise.
 * @note Line numbers are apparent (screen) line numbers, not real
 * line numbers in the file.
 * @note @p nEndLine and @p nEndChar are valid as long as you do not call
 *   FlushUndoGroup. If you need to call FlushUndoGroup, just store them in a
 *   variable which is preserved with real line number during Rescan
 *   (m_ptCursorPos, m_ptLastChange for example).
 */
POINT CGhostTextBuffer::InsertText(CCrystalTextView *pSource,
	int nLine, int nPos, LPCTSTR pszText, int cchText, int nAction, BOOL bHistory)
{
	BOOL bGroupFlag = FALSE;
	if (bHistory)
	{
		if (!m_bUndoGroup)
		{
			BeginUndoGroup ();
			bGroupFlag = TRUE;
		} 
	}

	// save line revision numbers for undo
	vector<DWORD> paSavedRevisonNumbers;
	paSavedRevisonNumbers.push_back(m_aLines[nLine].m_dwRevisionNumber);

	POINT ptCursorPos = InternalInsertText(pSource, nLine, nPos, pszText, cchText);

	m_dwCurrentRevisionNumber++;

	// set WinMerge flags
	const DWORD bFirstLineGhost = GetLineFlags(nLine) & LF_GHOST;

	// when inserting an EOL terminated text into a ghost line,
	// there is a dicrepancy between nInsertedLines and nEndLine-nRealLine
	const bool bDiscrepancyInInsertedLines = bFirstLineGhost && ptCursorPos.x == 0;

	// compute the number of real lines created (for undo)
	int nRealLinesCreated = ptCursorPos.y - nLine;
	if (bFirstLineGhost && ptCursorPos.x > 0)
		// we create one more real line
		++nRealLinesCreated;

	int i;
	for (i = nLine ; i < ptCursorPos.y ; i++)
	{
		// update line revision numbers of modified lines
		m_aLines[i].m_dwRevisionNumber = m_dwCurrentRevisionNumber;
		OnNotifyLineHasBeenEdited(i);
	}
	if (!bDiscrepancyInInsertedLines)
	{
		m_aLines[i].m_dwRevisionNumber = m_dwCurrentRevisionNumber;
		OnNotifyLineHasBeenEdited(i);
	}

	// when inserting into a ghost line block, we want to replace ghost lines
	// with our text, so delete some ghost lines below the inserted text
	if (bFirstLineGhost)
	{
		// where is the first line after the inserted text ?
		int nInsertedTextLinesCount = ptCursorPos.y - nLine + (bDiscrepancyInInsertedLines ? 0 : 1);
		int nLineAfterInsertedBlock = nLine + nInsertedTextLinesCount;
		// delete at most nInsertedTextLinesCount - 1 ghost lines
		// as the first ghost line has been reused
		int nMaxGhostLineToDelete = min(nInsertedTextLinesCount - 1, GetLineCount()-nLineAfterInsertedBlock);
		for (i = 0 ; i < nMaxGhostLineToDelete ; i++)
			if ((GetLineFlags(nLineAfterInsertedBlock+i) & LF_GHOST) == 0)
				break;
		InternalDeleteGhostLine(pSource, nLineAfterInsertedBlock, i);
	}

	for (i = nLine ; i < ptCursorPos.y ; i++)
		m_aLines[i].m_dwFlags &= ~LF_GHOST;
	if (!bDiscrepancyInInsertedLines)
		// if there is no discrepancy, the final cursor line is real
		// as either some text was inserted in it, or it inherits the real status from the first line
		m_aLines[i].m_dwFlags &= ~LF_GHOST;
	else
		// if there is a discrepancy, the final cursor line was not changed during insertion so we do nothing
		;

	// now we can recompute
	if ((ptCursorPos.y > nLine) || bFirstLineGhost)
	{
		// TODO: Be smarter, and don't recompute if it is easy to see what changed
		RecomputeRealityMapping();
	}

	RecomputeEOL(pSource, nLine, ptCursorPos.y);

	if (bHistory)
	{
		POINT ptStartPos = { nPos, nLine };
		UndoRecord &ur = AddUndoRecord(TRUE, ptStartPos, ptCursorPos,
			pszText, cchText, nRealLinesCreated, nAction);
		ur.m_paSavedRevisonNumbers.swap(paSavedRevisonNumbers);

		if (bGroupFlag)
			FlushUndoGroup(pSource);

		// nEndLine may have changed during Rescan
		// (TODO: Shouldn't that happen outside the if?)
		ptCursorPos.y = m_ptLastChange.y;
	}
	return ptCursorPos;
}

/**
 * @brief Remove text from the buffer.
 * @param [in] pSource View from which to remove the text.
 * @param [in] nLine Line number (apparent/screen) where the deletion starts.
 * @param [in] nPos Character position where the deletion starts.
 * @param [in] nEndLine Line number (apparent/screen) where the deletion ends.
 * @param [out] nEndChar Character position where the deletion ends.
 * @param [in] nAction Edit action.
 * @param [in] bHistory Save insertion for undo/redo?
 * @return TRUE if the deletion succeeded, FALSE otherwise.
 */
void CGhostTextBuffer::DeleteText(CCrystalTextView * pSource, int nStartLine,
		int nStartChar, int nEndLine, int nEndChar, int nAction, BOOL bHistory)
{
	String sTextToDelete;
	GetText(nStartLine, nStartChar, nEndLine, nEndChar, sTextToDelete);
	// If there is nothing to delete, bail out.
	if (sTextToDelete.empty())
		return;

	// If we want to add undo record, but haven't created undo group yet,
	// create new group for this action. It gets flushed at end of the
	// function.
	BOOL bGroupFlag = FALSE;
	if (bHistory)
	{
		if (!m_bUndoGroup)
		{
			BeginUndoGroup();
			bGroupFlag = TRUE;
		} 
	}

	// save line revision numbers for undo
	vector<DWORD> paSavedRevisonNumbers;
	paSavedRevisonNumbers.resize(nEndLine - nStartLine + 1);
	int i, j;
	for (i = 0, j = 0; i < nEndLine - nStartLine + 1; i++)
	{
		DWORD dwLineFlag = GetLineFlags(nStartLine + i);
		if (!(dwLineFlag & LF_GHOST))
			paSavedRevisonNumbers[j++] = m_aLines[nStartLine + i].m_dwRevisionNumber;
	}
	paSavedRevisonNumbers.resize(j);

	// flags are going to be deleted so we store them now
	const DWORD bLastLineGhost = GetLineFlags(nEndLine) & LF_GHOST;
	const DWORD bFirstLineGhost = GetLineFlags(nStartLine) & LF_GHOST;
	// count the number of real lines in the deleted block (for first/last line,
	// include partial real lines)
	int nRealLinesInDeletedBlock = ComputeRealLine(nEndLine) - ComputeRealLine(nStartLine);
	if (!bLastLineGhost)
		++nRealLinesInDeletedBlock;

	InternalDeleteText(pSource, nStartLine, nStartChar, nEndLine, nEndChar);

	m_dwCurrentRevisionNumber++;

	OnNotifyLineHasBeenEdited(nStartLine);

	// update line revision numbers of modified lines
	m_dwCurrentRevisionNumber++;

	LineInfo &li = m_aLines[nStartLine];

	if (nStartChar != 0 || nEndChar != 0)
		li.m_dwRevisionNumber = m_dwCurrentRevisionNumber;

	// the first line inherits the status of the last one 
	// but exception... if the last line is a ghost, we preserve the status of the first line
	// (then if we use backspace in a ghost line, we don't delete the previous line)
	if (bLastLineGhost && bFirstLineGhost)
	{
		li.m_dwFlags |= LF_GHOST;
		li.m_nSkippedLines = 0;
	}
	else
	{
		li.m_dwFlags &= ~LF_GHOST;
	}

	// now we can recompute
	if (nStartLine != nEndLine)
	{
		// TODO: Be smarter, and don't recompute if it is easy to see what changed
		RecomputeRealityMapping();
	}

	RecomputeEOL(pSource, nStartLine, nStartLine);

	if (bHistory)
	{
		POINT ptStartPos = { nStartChar, nStartLine };
		POINT ptEndPos = { 0, -1 };
		UndoRecord &ur = AddUndoRecord(FALSE, ptStartPos, ptEndPos,
			sTextToDelete.c_str(), sTextToDelete.length(),
			nRealLinesInDeletedBlock, nAction);
		ur.m_paSavedRevisonNumbers.swap(paSavedRevisonNumbers);

		if (bGroupFlag)
			FlushUndoGroup(pSource);
	}
}

/**
 * @brief Insert a ghost line to the buffer (and view).
 * @param [in] pSource The view to which to add the ghost line.
 * @param [in] Line index (apparent/screen) where to add the ghost line.
 * @return TRUE if the addition succeeded, FALSE otherwise.
 */
BOOL CGhostTextBuffer::InsertGhostLine(CCrystalTextView * pSource, int nLine)
{
	if (!InternalInsertGhostLine(pSource, nLine))
		return FALSE;
	// Set WinMerge flags  
	m_aLines[nLine].m_dwFlags |= LF_GHOST;
	RecomputeRealityMapping();
	// Don't need to recompute EOL as real lines are unchanged.
	// Never AddUndoRecord as Rescan clears the ghost lines.
	return TRUE;
}

/**
 * @brief Remove all the ghost lines from the buffer.
 */
void CGhostTextBuffer::RemoveAllGhostLines(DWORD dwMask)
{
	const size_t n = m_aLines.size();
	stl_size_t i = 0;
	stl_size_t j = 0;
	// Free the buffer of ghost lines, and compact non-ghost lines
	// (we copy the buffer address, so the buffer don't move and we don't free it)
	while (i < n)
	{
		if (m_aLines[i].m_dwFlags & dwMask)
		{
			m_aLines[i].Clear();
		}
		else
		{
			m_aLines[j] = m_aLines[i];
			++j;
		}
		++i;
	}
	// Discard unused entries in one shot
	m_aLines.resize(j);
	RecomputeRealityMapping();
}

////////////////////////////////////////////////////////////////////////////
// apparent <-> real line conversion

/**
 * @brief Get last apparent (screen) line index.
 * @return Last apparent line, or -1 if no lines in the buffer.
 */
int CGhostTextBuffer::ApparentLastRealLine() const
{
	const int size = m_RealityBlocks.size();
	if (size == 0)
		return -1;
	const RealityBlock &block = m_RealityBlocks.back();
	return block.nStartApparent + block.nCount - 1;
}

/**
 * @brief Get a real line for the apparent (screen) line.
 * This function returns the real line for the given apparent (screen) line.
 * For ghost lines we return next real line. For trailing ghost line we return
 * last real line + 1). Ie, lines 0->0, 1->2, 2->4, for argument of 3,
 * return 2.
 * @param [in] nApparentLine Apparent line for which to get the real line.
 * @return The real line for the apparent line.
 */
int CGhostTextBuffer::ComputeRealLine(int nApparentLine) const
{
	const int size = m_RealityBlocks.size();
	if (size == 0)
		return 0;

	// after last apparent line ?
	ASSERT(nApparentLine < GetLineCount());

	// after last block ?
	const RealityBlock &maxblock = m_RealityBlocks.back();
	if (nApparentLine >= maxblock.nStartApparent + maxblock.nCount)
		return maxblock.nStartReal + maxblock.nCount;

	// binary search to find correct (or nearest block)
	int blo = 0;
	int bhi = size - 1;
	while (blo <= bhi)
	{
		int i = (blo + bhi) / 2;
		const RealityBlock &block = m_RealityBlocks[i];
		if (nApparentLine < block.nStartApparent)
			bhi = i - 1;
		else if (nApparentLine >= block.nStartApparent + block.nCount)
			blo = i + 1;
		else // found it inside this block
			return (nApparentLine - block.nStartApparent) + block.nStartReal;
	}
	// it is a ghost line just before block blo
	return m_RealityBlocks[blo].nStartReal;
}

/**
 * @brief Get an apparent (screen) line for the real line.
 * @param [in] nRealLine Real line for which to get the apparent line.
 * @return The apparent line for the real line. If real line is out of bounds
 *   return last valid apparent line + 1.
 */
int CGhostTextBuffer::ComputeApparentLine(int nRealLine) const
{
	const int size = m_RealityBlocks.size();
	if (size == 0)
		return 0;

	// after last block ?
	const RealityBlock & maxblock = m_RealityBlocks.back();
	if (nRealLine >= maxblock.nStartReal + maxblock.nCount)
		return GetLineCount();

	// binary search to find correct (or nearest block)
	int blo = 0;
	int bhi = size - 1;
	while (blo <= bhi)
	{
		int i = (blo + bhi) / 2;
		const RealityBlock & block = m_RealityBlocks[i];
		if (nRealLine < block.nStartReal)
			bhi = i - 1;
		else if (nRealLine >= block.nStartReal + block.nCount)
			blo = i + 1;
		else
			return (nRealLine - block.nStartReal) + block.nStartApparent;
	}
	// Should have found it; all real lines should be in a block
	ASSERT(0);
	return -1;
}

/**
 * @brief Get a real line for apparent (screen) line.
 * This function returns the real line for the given apparent (screen) line.
 * For ghost lines we return next real line. For trailing ghost line we return
 * last real line + 1). Ie, lines 0->0, 1->2, 2->4, for argument of 3,
 * return 2. And decToReal would be 1.
 * @param [in] nApparentLine Apparent line for which to get the real line.
 * @param [out] decToReal Difference of the apparent and real line.
 * @return The real line for the apparent line.
 */
int CGhostTextBuffer::ComputeRealLineAndGhostAdjustment(int nApparentLine,
		int& decToReal) const
{
	const int size = m_RealityBlocks.size();
	if (size == 0) 
	{
		decToReal = 0;
		return 0;
	}

	// after last apparent line ?
	ASSERT(nApparentLine < GetLineCount());

	// after last block ?
	const RealityBlock & maxblock = m_RealityBlocks.back();
	if (nApparentLine >= maxblock.nStartApparent + maxblock.nCount)
	{
		decToReal = GetLineCount() - nApparentLine;
		return maxblock.nStartReal + maxblock.nCount;
	}

	// binary search to find correct (or nearest block)
	int blo = 0;
	int bhi = size - 1;
	while (blo <= bhi)
	{
		int i = (blo + bhi) / 2;
		const RealityBlock & block = m_RealityBlocks[i];
		if (nApparentLine < block.nStartApparent)
			bhi = i - 1;
		else if (nApparentLine >= block.nStartApparent + block.nCount)
			blo = i + 1;
		else // found it inside this block
		{
			decToReal = 0;
			return (nApparentLine - block.nStartApparent) + block.nStartReal;
		}
	}
	// it is a ghost line just before block blo
	decToReal = m_RealityBlocks[blo].nStartApparent - nApparentLine;
	return m_RealityBlocks[blo].nStartReal;
}

/**
 * @brief Get an apparent (screen) line for the real line.
 * @param [in] nRealLine Real line for which to get the apparent line.
 * @param [out] decToReal Difference of the apparent and real line.
 * @return The apparent line for the real line. If real line is out of bounds
 *   return last valid apparent line + 1.
 */
int CGhostTextBuffer::ComputeApparentLine(int nRealLine, int decToReal) const
{
	int nPreviousBlock;
	int nApparent;

	const int size = (int) m_RealityBlocks.size();
	int blo = 0;
	int bhi = size - 1;
	int i;
	if (size == 0)
		return 0;

	// after last block ?
	const RealityBlock & maxblock = m_RealityBlocks.back();
	if (nRealLine >= maxblock.nStartReal + maxblock.nCount)
	{
		nPreviousBlock = size - 1;
		nApparent = GetLineCount();
		goto limitWithPreviousBlock;
	}

	// binary search to find correct (or nearest block)
	while (blo <= bhi)
	{
		i = (blo + bhi) / 2;
		const RealityBlock & block = m_RealityBlocks[i];
		if (nRealLine < block.nStartReal)
			bhi = i - 1;
		else if (nRealLine >= block.nStartReal + block.nCount)
			blo = i + 1;
		else
		{
			if (nRealLine > block.nStartReal)
				// limited by the previous line in this block
				return (nRealLine - block.nStartReal) + block.nStartApparent;
			nPreviousBlock = i - 1;
			nApparent = (nRealLine - block.nStartReal) + block.nStartApparent;
			goto limitWithPreviousBlock;
		}
	}
	// Should have found it; all real lines should be in a block
	ASSERT(0);
	return -1;

limitWithPreviousBlock:
	// we must keep above the value lastApparentInPreviousBlock
	int lastApparentInPreviousBlock;
	if (nPreviousBlock == -1)
		lastApparentInPreviousBlock = -1;
	else
	{
		const RealityBlock & previousBlock = m_RealityBlocks[nPreviousBlock];
		lastApparentInPreviousBlock = previousBlock.nStartApparent + previousBlock.nCount - 1;
	}

	while (decToReal --) 
	{
		nApparent --;
		if (nApparent == lastApparentInPreviousBlock)
			return nApparent + 1;
	}
	return nApparent;
}

/** Do what we need to do just after we've been reloaded */
void CGhostTextBuffer::FinishLoading()
{
	ASSERT(m_bInit);
	RecomputeRealityMapping();
}

/** Recompute the reality mapping (this is fairly naive) */
void CGhostTextBuffer::RecomputeRealityMapping()
{
	m_RealityBlocks.clear();
	stl_size_t reality = 0; // last encountered real line
	stl_size_t i = 0; // current line
	const stl_size_t n = m_aLines.size();

	// This used to be a state machine with 2 states, but has been converted to
	// human-readable code to help get an idea how to account for skipped lines.

	// state 1, i-1 not real line
	while (i < n)
	{
		if (m_aLines[i].m_dwFlags & LF_GHOST)
		{
			reality += m_aLines[i++].m_nSkippedLines;
		}
		else
		{
			// this is the first line of a reality block
			RealityBlock block;
			block.nStartApparent = i;
			block.nStartReal = reality;
			// fall through to other state
			// state 2, i - 1 is real line
			do
			{
				++reality;
				++i;
			} while ((i < n) && (m_aLines[i].m_dwFlags & LF_GHOST) == 0);
			// i-1 is the last line of a reality block
			ASSERT(reality > 0);
			block.nCount = i - block.nStartApparent;
			ASSERT(block.nCount > 0);
			ASSERT(reality - block.nStartReal == block.nCount);
			// Optimize memory allocation
			if (m_RealityBlocks.capacity() == m_RealityBlocks.size())
			{
				m_RealityBlocks.reserve(m_RealityBlocks.size() ? m_RealityBlocks.size() * 2 : 16);
			}
			m_RealityBlocks.push_back(block);
		}
	}
}

/** we recompute EOL from the real line before nStartLine to nEndLine */
void CGhostTextBuffer::RecomputeEOL(CCrystalTextView *pSource, int nStartLine, int nEndLine)
{
	int nApparentLastRealLine = ApparentLastRealLine();
	if (nApparentLastRealLine <= nEndLine)
	{
		// EOL may have to change on the real line before nStartLine
		int nRealBeforeStart = nStartLine;
		while (--nRealBeforeStart >= 0)
			if ((GetLineFlags(nRealBeforeStart) & LF_GHOST) == 0)
				break;
		if (nRealBeforeStart >= 0)
			nStartLine = nRealBeforeStart;
	}
	for (int i = nEndLine ; i >= nStartLine ; i--)
	{
		LineInfo &li = m_aLines[i];
		if ((li.m_dwFlags & LF_GHOST) != 0 || i == nApparentLastRealLine)
		{
			if (li.HasEol()) 
			{
				// if a ghost line has an EOL, remove it
				li.RemoveEol();
				if (pSource != NULL)
					UpdateViews(pSource, NULL, UPDATE_HORZRANGE | UPDATE_SINGLELINE, i);
			}
		}
		else
		{
			if (!li.HasEol()) 
			{
				// if a real line (not the last) has no EOL, add one
				AppendLine(i, GetDefaultEol(), (int) _tcslen(GetDefaultEol()));
				if (pSource != NULL)
					UpdateViews(pSource, NULL, UPDATE_HORZRANGE | UPDATE_SINGLELINE, i);
			}
		}
	}
}

#ifdef _DEBUG
/** 
Check all lines, and ASSERT if reality blocks differ from flags. 
This means that this only has effect in DEBUG build
*/
void CGhostTextBuffer::checkFlagsFromReality() const
{
	const int size = m_RealityBlocks.size();
	int i = 0;
	for (int b = 0 ; b < size ; b ++)
	{
		const RealityBlock & block = m_RealityBlocks[b];
		for ( ; i < block.nStartApparent ; i++)
			ASSERT((GetLineFlags(i) & LF_GHOST) != 0);
		for ( ; i < block.nStartApparent+block.nCount ; i++)
			ASSERT((GetLineFlags(i) & LF_GHOST) == 0);
	}

	for ( ; i < GetLineCount() ; i++)
		ASSERT((GetLineFlags(i) & LF_GHOST) != 0);
}
#endif
