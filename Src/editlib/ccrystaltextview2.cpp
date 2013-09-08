////////////////////////////////////////////////////////////////////////////
//  File:       ccrystaltextview2.cpp
//  Version:    1.0.0.0
//  Created:    29-Dec-1998
//
//  Author:     Stcherbatchenko Andrei
//  E-mail:     windfall@gmx.de
//
//  Implementation of the CCrystalTextView class, a part of Crystal Edit -
//  syntax coloring text editor.
//
//  You are free to use or modify this code to the following restrictions:
//  - Acknowledge me somewhere in your about box, simple "Parts of code by.."
//  will be enough. If you can't (or don't want to), contact me personally.
//  - LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//	21-Feb-99
//		Paul Selormey, James R. Twine:
//	+	FEATURE: description for Undo/Redo actions
//	+	FEATURE: multiple MSVC-like bookmarks
//	+	FEATURE: 'Disable backspace at beginning of line' option
//	+	FEATURE: 'Disable drag-n-drop editing' option
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//  19-Jul-99
//      Ferdinand Prantl:
//  +   FEATURE: regular expressions, go to line and things ...
//  +   FEATURE: plenty of syntax highlighting definitions
//  +   FEATURE: corrected bug in syntax highlighting C comments
//  +   FEATURE: extended registry support for saving settings
//  +   FEATURE: some other things I've forgotten ...
//
//  ... it's being edited very rapidly so sorry for non-commented
//        and maybe "ugly" code ...
////////////////////////////////////////////////////////////////////////////
/** 
 * @file  ccrystaltextview2.cpp
 *
 * @brief More functions for CCrystalTextView class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "editcmd.h"
#include "ccrystaltextview.h"
#include "ccrystaltextbuffer.h"
#include "string_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CRYSTAL_TIMER_DRAGSEL   1001

/////////////////////////////////////////////////////////////////////////////
// CCrystalTextView

void CCrystalTextView::AdjustCursorAfterMoveLeft()
{
	LPCTSTR pszChars = GetLineChars(m_ptCursorPos.y);
	while (m_ptCursorPos.x > 0 && GetCharWidthFromChar(pszChars + m_ptCursorPos.x) == 0)
		--m_ptCursorPos.x;
}

void CCrystalTextView::AdjustCursorAfterMoveRight()
{
	int nLength = GetLineLength(m_ptCursorPos.y);
	LPCTSTR pszChars = GetLineChars(m_ptCursorPos.y);
	while (m_ptCursorPos.x < nLength && GetCharWidthFromChar(pszChars + m_ptCursorPos.x) == 0)
		++m_ptCursorPos.x;
}

void CCrystalTextView::MoveLeft(BOOL bSelect)
{
	PrepareSelBounds();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
	{
		m_ptCursorPos = m_ptDrawSelStart;
	}
	else
	{
		if (m_ptCursorPos.x == 0)
		{
			if (m_ptCursorPos.y > 0)
			{
				--m_ptCursorPos.y;
				m_ptCursorPos.x = GetLineLength(m_ptCursorPos.y);
			}
		}
		else
		{
			--m_ptCursorPos.x;
		}
	}
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveRight(BOOL bSelect)
{
	PrepareSelBounds();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
	{
		m_ptCursorPos = m_ptDrawSelEnd;
	}
	else
	{
		if (m_ptCursorPos.x == GetLineLength(m_ptCursorPos.y))
		{
			if (m_ptCursorPos.y < GetLineCount() - 1)
			{
				m_ptCursorPos.y++;
				m_ptCursorPos.x = 0;
			}
		}
		else
		{
			m_ptCursorPos.x++;
		}
	}
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveRight();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveWordLeft(BOOL bSelect)
{
	PrepareSelBounds();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
	{
		MoveLeft(bSelect);
		return;
	}

	if (m_ptCursorPos.x == 0)
	{
		if (m_ptCursorPos.y == 0)
			return;
		--m_ptCursorPos.y;
		m_ptCursorPos.x = GetLineLength(m_ptCursorPos.y);
	}

	LPCTSTR pszChars = GetLineChars(m_ptCursorPos.y);
	int nPos = m_ptCursorPos.x;
	int nPrevPos;
	while (nPos > 0 && xisspace(pszChars[nPrevPos = nPos - 1]))
		nPos = nPrevPos;

	if (nPos > 0)
	{
		int nPrevPos = nPos - 1;
		nPos = nPrevPos;
		if (xisalnum(pszChars[nPos]))
		{
			while (nPos > 0 && xisalnum(pszChars[--nPrevPos]))
				nPos = nPrevPos;
		}
		else
		{
			while (nPos > 0 && !xisalnum(pszChars[--nPrevPos]) && !xisspace(pszChars[nPrevPos]))
				nPos = nPrevPos;
		}
	}

	m_ptCursorPos.x = nPos;
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveWordRight(BOOL bSelect)
{
	PrepareSelBounds();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
	{
		MoveRight(bSelect);
		return;
	}

	if (m_ptCursorPos.x == GetLineLength (m_ptCursorPos.y))
	{
		if (m_ptCursorPos.y == GetLineCount() - 1)
			return;
		++m_ptCursorPos.y;
		m_ptCursorPos.x = 0;
	}

	int nLength = GetLineLength(m_ptCursorPos.y);
	if (m_ptCursorPos.x == nLength)
	{
		MoveRight(bSelect);
		return;
	}

	LPCTSTR pszChars = GetLineChars(m_ptCursorPos.y);
	int nPos = m_ptCursorPos.x;
	if (xisalnum(pszChars[nPos]))
	{
		while (nPos < nLength && xisalnum(pszChars[nPos]))
			++nPos;
	}
	else
	{
		while (nPos < nLength && !xisalnum(pszChars[nPos]) && !xisspace(pszChars[nPos]))
			++nPos;
	}

	while (nPos < nLength && xisspace(pszChars[nPos]))
		++nPos;

	m_ptCursorPos.x = nPos;
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveRight();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveUp(BOOL bSelect)
{
	PrepareSelBounds();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
		m_ptCursorPos = m_ptDrawSelStart;

	//BEGIN SW
	POINT subLinePos;
	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, subLinePos);

	int nSubLine = GetSubLineIndex(m_ptCursorPos.y) + subLinePos.y;

	if (nSubLine > 0)
	/*ORIGINAL
	if (m_ptCursorPos.y > 0)
	*///END SW
	{
		if (m_nIdealCharPos == -1)
			m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
		//BEGIN SW
		do {
			nSubLine--;
		} while (IsEmptySubLineIndex(nSubLine));
		SubLineCursorPosToTextPos(m_nIdealCharPos, nSubLine, m_ptCursorPos);
		/*ORIGINAL
		m_ptCursorPos.y --;
		m_ptCursorPos.x = ApproxActualOffset(m_ptCursorPos.y, m_nIdealCharPos);
		*///END SW
		const int nLineLength = GetLineLength(m_ptCursorPos.y);
		if (m_ptCursorPos.x > nLineLength)
			m_ptCursorPos.x = nLineLength;
	}
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveDown(BOOL bSelect)
{
	PrepareSelBounds ();
	if (m_ptDrawSelStart != m_ptDrawSelEnd && !bSelect)
		m_ptCursorPos = m_ptDrawSelEnd;

	//BEGIN SW
	POINT subLinePos;
	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, subLinePos);

	int nSubLine = GetSubLineIndex(m_ptCursorPos.y) + subLinePos.y;

	if (nSubLine < GetSubLineCount() - 1)
	/*ORIGINAL
	if (m_ptCursorPos.y < GetLineCount() - 1)
	*/
	{
		if (m_nIdealCharPos == -1)
			m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
		//BEGIN SW
		do {
			nSubLine++;
		} while (IsEmptySubLineIndex(nSubLine));
		SubLineCursorPosToTextPos(m_nIdealCharPos, nSubLine, m_ptCursorPos);
		/*ORIGINAL
		m_ptCursorPos.y ++;
		m_ptCursorPos.x = ApproxActualOffset(m_ptCursorPos.y, m_nIdealCharPos);
		*///END SW
		const int nLineLength = GetLineLength(m_ptCursorPos.y);
		if (m_ptCursorPos.x > nLineLength)
			m_ptCursorPos.x = nLineLength;
	}
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveHome(BOOL bSelect)
{
	int nLength = GetLineLength(m_ptCursorPos.y);
	LPCTSTR pszChars = GetLineChars(m_ptCursorPos.y);
	//BEGIN SW
	POINT pos;
	CharPosToPoint( m_ptCursorPos.y, m_ptCursorPos.x, pos );
	int nHomePos = SubLineHomeToCharPos( m_ptCursorPos.y, pos.y );
	int nOriginalHomePos = nHomePos;
	/*ORIGINAL
	int nHomePos = 0;
	*///END SW
	while (nHomePos < nLength && xisspace (pszChars[nHomePos]))
		nHomePos++;
	if (nHomePos == nLength || m_ptCursorPos.x == nHomePos)
		//BEGIN SW
		m_ptCursorPos.x = nOriginalHomePos;
		/*ORIGINAL
		m_ptCursorPos.x = 0;
		*///END SW
	else
		m_ptCursorPos.x = nHomePos;
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveEnd(BOOL bSelect)
{
	//BEGIN SW
	POINT pos;
	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, pos);
	m_ptCursorPos.x = SubLineEndToCharPos(m_ptCursorPos.y, pos.y);
	/*ORIGINAL
	m_ptCursorPos.x = GetLineLength(m_ptCursorPos.y);
	*///END SW
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MovePgUp(BOOL bSelect)
{
	const int nScreenLines = GetScreenLines();
	const int nPage = nScreenLines > 1 ? nScreenLines - 1 : 1;
	// scrolling windows
	int nNewTopSubLine = m_nTopSubLine - nPage;
	if (nNewTopSubLine < 0)
		nNewTopSubLine = 0;
	if (m_nTopSubLine != nNewTopSubLine)
	{
		ScrollToSubLine(nNewTopSubLine);
		UpdateSiblingScrollPos();
	}

	// setting cursor
	POINT subLinePos;
	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, subLinePos);

	int nSubLine = GetSubLineIndex(m_ptCursorPos.y) + subLinePos.y - nPage;

	if (nSubLine < nNewTopSubLine || nSubLine >= nNewTopSubLine + nScreenLines)
		nSubLine = nNewTopSubLine;

	if (nSubLine < 0)
		nSubLine = 0;

	SubLineCursorPosToTextPos(m_nIdealCharPos, nSubLine, m_ptCursorPos);

	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();    //todo: no vertical scroll
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MovePgDn(BOOL bSelect)
{
	//BEGIN SW
	// scrolling windows
	const int nScreenLines = GetScreenLines();
	const int nPage = nScreenLines > 1 ? nScreenLines - 1 : 1;
	const int nSubLineCount = GetSubLineCount();
	int nNewTopSubLine = m_nTopSubLine + nPage;

	if (nNewTopSubLine > nSubLineCount - nScreenLines)
		nNewTopSubLine = nSubLineCount - nScreenLines;
	if (nNewTopSubLine < m_nTopSubLine)
		nNewTopSubLine = m_nTopSubLine;

	if (m_nTopSubLine != nNewTopSubLine)
	{
		ScrollToSubLine(nNewTopSubLine);
		UpdateSiblingScrollPos();
	}

	// setting cursor
	POINT subLinePos;
	CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, subLinePos);

	int nSubLine = GetSubLineIndex(m_ptCursorPos.y) + subLinePos.y + nPage;

	if (nSubLine < nNewTopSubLine || nSubLine >= nNewTopSubLine + nScreenLines)
		nSubLine = nNewTopSubLine + nScreenLines - 1;

	if (nSubLine >= nSubLineCount)
		nSubLine = nSubLineCount - 1;

	SubLineCursorPosToTextPos(m_nIdealCharPos, nSubLine, m_ptCursorPos);

	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	AdjustCursorAfterMoveLeft();
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();    //todo: no vertical scroll
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveCtrlHome(BOOL bSelect)
{
	m_ptCursorPos.x = 0;
	m_ptCursorPos.y = 0;
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::MoveCtrlEnd(BOOL bSelect)
{
	m_ptCursorPos.y = GetLineCount() - 1;
	m_ptCursorPos.x = GetLineLength(m_ptCursorPos.y);
	m_nIdealCharPos = CalculateActualOffset (m_ptCursorPos.y, m_ptCursorPos.x);
	if (!bSelect)
		m_ptAnchor = m_ptCursorPos;
	EnsureCursorVisible();
	SetSelection(m_ptAnchor, m_ptCursorPos);
	UpdateCaret();
}

void CCrystalTextView::ScrollUp()
{
	if (m_nTopLine > 0)
	{
		ScrollToLine(m_nTopLine - 1);
		UpdateSiblingScrollPos();
	}
}

void CCrystalTextView::ScrollDown()
{
	if (m_nTopLine < GetLineCount() - 1)
	{
		ScrollToLine(m_nTopLine + 1);
		UpdateSiblingScrollPos();
	}
}

void CCrystalTextView::ScrollLeft()
{
	if (m_nOffsetChar > 0)
	{
		ScrollToChar(m_nOffsetChar - 1);
		UpdateSiblingScrollPos();
	}
}

void CCrystalTextView::ScrollRight()
{
	if (m_nOffsetChar < GetMaxLineLength() - 1)
	{
		ScrollToChar(m_nOffsetChar + 1);
		UpdateSiblingScrollPos();
	}
}

POINT CCrystalTextView::WordToRight(POINT pt)
{
	ASSERT_VALIDTEXTPOS(pt);
	int nLength = GetLineLength(pt.y);
	LPCTSTR pszChars = GetLineChars(pt.y);
	while (pt.x < nLength && xisalnum(pszChars[pt.x]))
	{
		++pt.x;
	}
	ASSERT_VALIDTEXTPOS(pt);
	return pt;
}

POINT CCrystalTextView::WordToLeft(POINT pt)
{
	ASSERT_VALIDTEXTPOS(pt);
	LPCTSTR pszChars = GetLineChars(pt.y);
	int nPrevX = pt.x;
	while (pt.x > 0 && xisalnum(pszChars[--nPrevX]))
	{
		pt.x = nPrevX;
	}
	ASSERT_VALIDTEXTPOS(pt);
	return pt;
}

void CCrystalTextView::SelectAll()
{
	int nLineCount = GetLineCount();
	m_ptCursorPos.y = nLineCount - 1;
	m_ptCursorPos.x = GetLineLength(m_ptCursorPos.y);
	POINT ptStart = { 0, 0 };
	SetSelection(ptStart, m_ptCursorPos);
	UpdateCaret();
}

/** 
 * @brief Called when left mousebutton pressed down in editor.
 * This function handles left mousebutton down in editor.
 * @param [in] nFlags Flags indicating if virtual keys are pressed.
 * @param [in] point Point where mousebutton is pressed.
 */
void CCrystalTextView::OnLButtonDown(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	SetFocus();

	bool bShift = GetKeyState(VK_SHIFT) < 0;
	bool bControl = GetKeyState(VK_CONTROL) < 0;

	if (point.x < GetMarginWidth())
	{
		if (bControl)
		{
			SelectAll();
		}
		else
		{
			AdjustTextPoint(point);
			m_ptCursorPos = ClientToText(point);
			// Find char pos that is the beginning of the subline clicked on
			POINT pos;
			CharPosToPoint(m_ptCursorPos.y, m_ptCursorPos.x, pos);
			m_ptCursorPos.x = SubLineHomeToCharPos(m_ptCursorPos.y, pos.y);
			if (!bShift)
				m_ptAnchor = m_ptCursorPos;
			SetCapture();
			m_nDragSelTimer = SetTimer(CRYSTAL_TIMER_DRAGSEL, 100);
			ASSERT(m_nDragSelTimer != 0);
			m_bWordSelection = false;
			m_bLineSelection = true;
			m_bDragSelection = true;
			OnMouseMove(lParam);
		}
	}
	else
    {
		POINT ptText = ClientToText(point);
		PrepareSelBounds();
		//  [JRT]:  Support For Disabling Drag and Drop...
		if (IsInsideSelBlock(ptText) &&    // If Inside Selection Area
			!m_bDisableDragAndDrop)    // And D&D Not Disabled
		{
			m_bPreparingToDrag = true;
		}
		else
		{
			AdjustTextPoint(point);
			m_ptCursorPos = ClientToText(point);
			if (!bShift)
				m_ptAnchor = m_ptCursorPos;
			SetCapture();
			m_nDragSelTimer = SetTimer(CRYSTAL_TIMER_DRAGSEL, 100);
			ASSERT(m_nDragSelTimer != 0);
			m_bWordSelection = bControl;
			m_bLineSelection = false;
			m_bDragSelection = true;
			OnMouseMove(lParam);
		}
	}

	ASSERT_VALIDTEXTPOS(m_ptCursorPos);
	// we must set the ideal character position here!
	m_nIdealCharPos = CalculateActualOffset(m_ptCursorPos.y, m_ptCursorPos.x);
}

void CCrystalTextView::OnMouseMove(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	if (m_bDragSelection)
	{
		bool bOnMargin = point.x < GetMarginWidth();

		AdjustTextPoint(point);
		POINT ptNewCursorPos = ClientToText(point);

		POINT ptStart, ptEnd;
		if (m_bLineSelection)
		{
			if (bOnMargin)
			{
				POINT pos;
				ptEnd = m_ptAnchor;
				CharPosToPoint(ptEnd.y, ptEnd.x, pos);
				if (ptNewCursorPos.y < m_ptAnchor.y ||
					ptNewCursorPos.y == m_ptAnchor.y && ptNewCursorPos.x < m_ptAnchor.x)
				{
					const int nSublineIndex = GetSubLineIndex(ptEnd.y) + pos.y + 1;
					if (nSublineIndex >= GetSubLineCount())
						ptEnd.x = SubLineEndToCharPos(ptEnd.y, pos.y);
					else
					{
						int	nLine, nSubLine;
						GetLineBySubLine(nSublineIndex, nLine, nSubLine);
						ptEnd.y = nLine;
						ptEnd.x = SubLineHomeToCharPos(nLine, nSubLine);
					}
					CharPosToPoint(ptNewCursorPos.y, ptNewCursorPos.x, pos);
					ptNewCursorPos.x = SubLineHomeToCharPos(ptNewCursorPos.y, pos.y);
				}
				else
				{
					ptEnd.x = SubLineHomeToCharPos(ptEnd.y, pos.y);
					m_ptCursorPos = ptNewCursorPos;
					CharPosToPoint(ptNewCursorPos.y, ptNewCursorPos.x, pos);
					const int nSublineIndex = GetSubLineIndex(ptNewCursorPos.y) + pos.y + 1;
					if (nSublineIndex >= GetSubLineCount())
						ptNewCursorPos.x = SubLineEndToCharPos(ptNewCursorPos.y, pos.y);
					else
					{
						int	nLine, nSubLine;
						GetLineBySubLine(nSublineIndex, nLine, nSubLine);
						ptNewCursorPos.y = nLine;
						ptNewCursorPos.x = SubLineHomeToCharPos(nLine, nSubLine);
					}
				}
				m_ptCursorPos = ptNewCursorPos;
				SetSelection(ptNewCursorPos, ptEnd);
				UpdateCaret();
				return;
			}
			//  Moving to normal selection mode
			::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
			m_bLineSelection = m_bWordSelection = false;
		}

		if (m_bWordSelection)
		{
			if (ptNewCursorPos.y < m_ptAnchor.y ||
				ptNewCursorPos.y == m_ptAnchor.y && ptNewCursorPos.x < m_ptAnchor.x)
			{
				ptStart = WordToLeft(ptNewCursorPos);
				ptEnd = WordToRight(m_ptAnchor);
			}
			else
			{
				ptStart = WordToLeft(m_ptAnchor);
				ptEnd = WordToRight(ptNewCursorPos);
			}
		}
		else
		{
			ptStart = m_ptAnchor;
			ptEnd = ptNewCursorPos;
		}

		m_ptCursorPos = ptEnd;
		SetSelection(ptStart, ptEnd);
		UpdateCaret();
	}

	if (m_bPreparingToDrag)
	{
		m_bPreparingToDrag = false;
		if (HGLOBAL hData = PrepareDragData())
		{
			if (QueryEditable())
				m_pTextBuffer->BeginUndoGroup();

			m_bDraggingText = true;
			DWORD de = GetDropEffect();
			HRESULT hr = DoDragDrop(this, this, de, &de);
			if (SUCCEEDED(hr) && de != DROPEFFECT_NONE)
				OnDropSource(de);
			m_bDraggingText = false;

			if (QueryEditable())
				m_pTextBuffer->FlushUndoGroup(this);
		}
	}

	ASSERT_VALIDTEXTPOS(m_ptCursorPos);
}

void CCrystalTextView::OnLButtonUp(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);

	if (m_bDragSelection)
	{
		EnsureCursorVisible();
		ReleaseCapture();
		KillTimer(m_nDragSelTimer);
		m_bDragSelection = false;
	}

	if (m_bPreparingToDrag)
	{
		m_bPreparingToDrag = false;
		AdjustTextPoint(point);
		m_ptCursorPos = ClientToText(point);
		EnsureCursorVisible();
		SetSelection(m_ptCursorPos, m_ptCursorPos);
		UpdateCaret();
	}

	ASSERT_VALIDTEXTPOS(m_ptCursorPos);
}

void CCrystalTextView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == CRYSTAL_TIMER_DRAGSEL)
	{
		ASSERT(m_bDragSelection);
		POINT pt;
		::GetCursorPos(&pt);
		ScreenToClient(&pt);
		RECT rcClient;
		GetClientRect(&rcClient);

		bool bChanged = false;

		//  Scroll vertically, if necessary
		int nNewTopLine = m_nTopLine;
		int nLineCount = GetLineCount();
		if (pt.y < rcClient.top)
		{
			nNewTopLine--;
			if (pt.y < rcClient.top - GetLineHeight())
				nNewTopLine -= 2;
		}
		else if (pt.y >= rcClient.bottom)
		{
			nNewTopLine++;
			if (pt.y >= rcClient.bottom + GetLineHeight())
				nNewTopLine += 2;
		}

		if (nNewTopLine < 0)
			nNewTopLine = 0;
		if (nNewTopLine >= nLineCount)
			nNewTopLine = nLineCount - 1;

		if (m_nTopLine != nNewTopLine)
		{
			ScrollToLine(nNewTopLine);
			UpdateSiblingScrollPos();
			bChanged = true;
		}

		//  Scroll horizontally, if necessary
		int nNewOffsetChar = m_nOffsetChar;
		int nMaxLineLength = GetMaxLineLength();
		if (pt.x < rcClient.left)
			nNewOffsetChar--;
		else if (pt.x >= rcClient.right)
			nNewOffsetChar++;

		if (nNewOffsetChar >= nMaxLineLength)
			nNewOffsetChar = nMaxLineLength - 1;
		if (nNewOffsetChar < 0)
			nNewOffsetChar = 0;

		if (m_nOffsetChar != nNewOffsetChar)
		{
			ScrollToChar(nNewOffsetChar);
			UpdateSiblingScrollPos();
			bChanged = true;
		}

		//  Fix changes
		if (bChanged)
		{
			AdjustTextPoint(pt);
			POINT ptNewCursorPos = ClientToText(pt);
			if (ptNewCursorPos != m_ptCursorPos)
			{
				m_ptCursorPos = ptNewCursorPos;
				SetSelection(m_ptAnchor, m_ptCursorPos);
				UpdateCaret();
			}
		}
	}
}

/** 
 * @brief Called when mouse is double-clicked in editor.
 * This function handles mouse double-click in editor. There are many things
 * we can do, depending on where mouse is double-clicked etc:
 * - in selection area toggles bookmark
 * - in editor selects word below cursor
 * @param [in] nFlags Flags indicating if virtual keys are pressed.
 * @param [in] point Point where mouse is double-clicked.
 */
void CCrystalTextView::OnLButtonDblClk(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);

	if (point.x < GetMarginWidth())
	{
		AdjustTextPoint(point);
		POINT ptCursorPos = ClientToText(point);
		ToggleBookmark(ptCursorPos.y);
		return;
	}

	if (!m_bDragSelection)
	{
		AdjustTextPoint(point);
		m_ptCursorPos = ClientToText(point);
		m_ptAnchor = m_ptCursorPos;
		SetCapture();
		m_nDragSelTimer = SetTimer(CRYSTAL_TIMER_DRAGSEL, 100);
		ASSERT(m_nDragSelTimer != 0);
		m_bWordSelection = true;
		m_bLineSelection = false;
		m_bDragSelection = true;
		OnMouseMove(lParam);
	}
}

void CCrystalTextView::OnRButtonDown(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	AdjustTextPoint(point);
	point = ClientToText(point);
	// If there is selection, dont' clear it
	if (!IsSelection())
	{
		m_ptAnchor = m_ptCursorPos = point;
		EnsureCursorVisible();
		SetSelection(m_ptCursorPos, m_ptCursorPos);
		UpdateCaret();
	}
}

bool CCrystalTextView::IsSelection()
{
	return m_ptSelStart != m_ptSelEnd;
}

void CCrystalTextView::OnEditCopy()
{
	WaitStatusCursor wc;
	if (HGLOBAL hData = PrepareDragData())
	{
		PutToClipboard(hData);
	}
}

bool CCrystalTextView::QueryEditable()
{
	return false;
}

void CCrystalTextView::PutToClipboard(HGLOBAL hData)
{
	if (OpenClipboard())
	{
		EmptyClipboard();
		if (SetClipboardData(CF_UNICODETEXT, hData))
		{
			SetClipboardData(RegisterClipboardFormat(_T("WinMergeClipboard")), NULL);
		}
		CloseClipboard();
	}
	else
	{
		::GlobalFree(hData);
	}
}
