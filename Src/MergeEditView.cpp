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
 * @file  MergeEditView.cpp
 *
 * @brief Implementation of the CMergeEditView class
 */
#include "StdAfx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "SyntaxColors.h"
#include "LanguageSelect.h"
#include "MergeEditView.h"
#include "MergeDiffDetailView.h"
#include "Environment.h"
#include "ConsoleWindow.h"
#include "Common/stream_util.h"
#include "ShellContextMenu.h"
#include "paths.h"
#include "PidlContainer.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::vector;

/** @brief Timer ID for delayed rescan. */
const UINT IDT_RESCAN = 2;
/** @brief Timer timeout for delayed rescan. */
const UINT RESCAN_TIMEOUT = 1000;

/**
 * @brief Statusbar pane indexes
 */
enum
{
	PANE_INFO,
	PANE_RO,
	PANE_ENCODING,
	PANE_EOL,
};

/** @brief RO status panel width */
static UINT RO_PANEL_WIDTH = 40;
/** @brief Encoding status panel width */
static UINT ENCODING_PANEL_WIDTH = 80;
/** @brief EOL type status panel width */
static UINT EOL_PANEL_WIDTH = 60;

// The resource ID constants/limits for the Shell context menu
static const UINT MergeViewCmdFirst	= 0x9000;
static const UINT MergeViewCmdLast	= 0xAFFF;



/////////////////////////////////////////////////////////////////////////////
// CMergeEditView

CMergeEditView::CMergeEditView(HWindow *pWnd, CChildFrame *pDocument, int nThisPane)
	: CGhostTextView(pDocument, nThisPane, sizeof *this)
	, m_pShellContextMenu(new CShellContextMenu(MergeViewCmdFirst, MergeViewCmdLast))
{
	ASSERT(m_hShellContextMenu == NULL);
	SubclassWindow(pWnd);
	RegisterDragDrop(m_hWnd, this);
	SetParser(&m_xParser);
}

CMergeEditView::~CMergeEditView()
{
	delete m_pShellContextMenu;
}

LRESULT CMergeEditView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		RevokeDragDrop(m_hWnd);
		break;
	case WM_CHAR:
		OnChar(wParam);
		break;
	case WM_SETFOCUS:
		m_pDocument->UpdateHeaderActivity(m_nThisPane, true);
		break;
	case WM_KILLFOCUS:
		m_pDocument->UpdateHeaderActivity(m_nThisPane, false);
		break;
	case WM_LBUTTONDBLCLK:
		OnLButtonDblClk();
		break;
	case WM_LBUTTONUP:
		OnLButtonUp();
		break;
	case WM_TIMER:
		OnTimer(wParam);
		break;
	case WM_SIZE:
		OnSize();
		break;
	case WM_MOUSEWHEEL:
		if (OnMouseWheel(wParam, lParam))
			return 0;
		break;
	case WM_CONTEXTMENU:
		OnContextMenu(lParam);
		break;
	case WM_NOTIFY:
		OnNotity(lParam);
		break;
	case WM_HSCROLL:
		// In limited context mode, invalidate panes to ensure proper rendering
		// of placeholders for sequences of excluded lines.
		if (m_pDocument->m_idContextLines != ID_VIEW_CONTEXT_UNLIMITED)
		{
			LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
			HWindow *pParent = GetParent();
			HWindow *pChild = NULL;
			while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
			{
				pChild->Invalidate();
			}
		}
		break;
	}
	return CCrystalEditViewEx::WindowProc(uMsg, wParam, lParam);
}

void CMergeEditView::OnNotity(LPARAM lParam)
{
	union
	{
		LPARAM lp;
		NMHDR *hdr;
		NMMOUSE *mouse;
	} const nm = { lParam };
	if (nm.hdr->hwndFrom == m_pStatusBar->m_hWnd)
	{
		if (nm.hdr->code == NM_CLICK)
		{
			switch (nm.mouse->dwItemSpec)
			{
			case PANE_RO:
				m_bOvrMode = !m_bOvrMode;
				UpdateCaret(true);
				UpdateLineInfoStatus();
				break;
			}
		}
	}
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CMergeEditView::UpdateResources()
{
	// Update static texts in status bars.
	OnUpdateCaret();
	// Update annotated ghost lines when operating in limited context mode.
	Invalidate();
}

int CMergeEditView::GetAdditionalTextBlocks(int nLineIndex, TEXTBLOCK *&rpBuf)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_DIFF) != LF_DIFF)
		return 0;

	if (!COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT))
		return 0;

	int nLineLength = GetLineLength(nLineIndex);
	vector<wdiff> worddiffs;
	m_pDocument->GetWordDiffArray(nLineIndex, worddiffs);

	if (nLineLength == 0 || worddiffs.size() == 0 || // Both sides are empty
		IsSide0Empty(worddiffs, nLineLength) || IsSide1Empty(worddiffs, nLineLength))
	{
		return 0;
	}

	bool lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);
	int nWordDiffs = worddiffs.size();

	TEXTBLOCK *pBuf = new TEXTBLOCK[nWordDiffs * 2 + 1];
	rpBuf = pBuf;
	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NONE;
	pBuf[0].m_nBgColorIndex = COLORINDEX_NONE;
	for (int i = 0; i < nWordDiffs; i++)
	{
		pBuf[1 + i * 2].m_nCharPos = worddiffs[i].start[m_nThisPane];
		pBuf[2 + i * 2].m_nCharPos = worddiffs[i].end[m_nThisPane] + 1;
		if (lineInCurrentDiff)
		{
			pBuf[1 + i * 2].m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT1 | COLORINDEX_APPLYFORCE;
			if (worddiffs[i].start[0] == worddiffs[i].end[0] + 1 ||
				worddiffs[i].start[1] == worddiffs[i].end[1] + 1)
				pBuf[1 + i * 2].m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND4 | COLORINDEX_APPLYFORCE;
			else
				pBuf[1 + i * 2].m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND1 | COLORINDEX_APPLYFORCE;
		}
		else
		{
			pBuf[1 + i * 2].m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT2 | COLORINDEX_APPLYFORCE;
			if (worddiffs[i].start[0] == worddiffs[i].end[0] + 1 ||
				worddiffs[i].start[1] == worddiffs[i].end[1] + 1)
			{
				// Case on one side char/words are inserted or deleted 
				pBuf[1 + i * 2].m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND3 | COLORINDEX_APPLYFORCE;
			}
			else
			{
				pBuf[1 + i * 2].m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND2 | COLORINDEX_APPLYFORCE;
			}
		}
		pBuf[2 + i * 2].m_nColorIndex = COLORINDEX_NONE;
		pBuf[2 + i * 2].m_nBgColorIndex = COLORINDEX_NONE;
	}

	return nWordDiffs * 2 + 1;
}

COLORREF CMergeEditView::GetColor(int nColorIndex)
{
	switch (nColorIndex & ~COLORINDEX_APPLYFORCE)
	{
	case COLORINDEX_HIGHLIGHTBKGND1:
		return m_cachedColors.clrSelWordDiff;
	case COLORINDEX_HIGHLIGHTTEXT1:
		return m_cachedColors.clrSelWordDiffText;
	case COLORINDEX_HIGHLIGHTBKGND2:
		return m_cachedColors.clrWordDiff;
	case COLORINDEX_HIGHLIGHTTEXT2:
		return m_cachedColors.clrWordDiffText;
	case COLORINDEX_HIGHLIGHTBKGND3:
		return m_cachedColors.clrWordDiffDeleted;
	case COLORINDEX_HIGHLIGHTBKGND4:
		return m_cachedColors.clrSelWordDiffDeleted;

	default:
		return CCrystalTextView::GetColor(nColorIndex);
	}
}

/**
 * @brief Determine text and background color for line
 * @param [in] nLineIndex Index of line in view (NOT line in file)
 * @param [out] crBkgnd Backround color for line
 * @param [out] crText Text color for line
 *
 * This version allows caller to suppress particular flags
 */
void CMergeEditView::GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	// Line inside diff
	if (dwLineFlags & LF_WINMERGE_FLAGS)
	{
		crText = m_cachedColors.clrDiffText;
		bool lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);

		if (dwLineFlags & LF_DIFF)
		{
			if (lineInCurrentDiff)
			{
				if (dwLineFlags & LF_MOVED)
				{
					if (dwLineFlags & LF_GHOST)
						crBkgnd = m_cachedColors.clrSelMovedDeleted;
					else
						crBkgnd = m_cachedColors.clrSelMoved;
					crText = m_cachedColors.clrSelMovedText;
				}
				else
				{
					crBkgnd = m_cachedColors.clrSelDiff;
					crText = m_cachedColors.clrSelDiffText;
				}
			
			}
			else
			{
				if (dwLineFlags & LF_MOVED)
				{
					if (dwLineFlags & LF_GHOST)
						crBkgnd = m_cachedColors.clrMovedDeleted;
					else
						crBkgnd = m_cachedColors.clrMoved;
					crText = m_cachedColors.clrMovedText;
				}
				else
				{
					crBkgnd = m_cachedColors.clrDiff;
					crText = m_cachedColors.clrDiffText;
				}
			}
		}
		else if (dwLineFlags & LF_TRIVIAL)
		{
			// trivial diff can not be selected
			if (dwLineFlags & LF_GHOST)
				// ghost lines in trivial diff has their own color
				crBkgnd = m_cachedColors.clrTrivialDeleted;
			else
				crBkgnd = m_cachedColors.clrTrivial;
			crText = m_cachedColors.clrTrivialText;
		}
		else if (dwLineFlags & LF_GHOST)
		{
			if (lineInCurrentDiff)
				crBkgnd = m_cachedColors.clrSelDiffDeleted;
			else
				crBkgnd = m_cachedColors.clrDiffDeleted;
		}
	}
	else
	{
		// Line not inside diff,
		if (!COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT))
		{
			// If no syntax hilighting, get windows default colors
			crBkgnd = GetColor(COLORINDEX_BKGND);
			crText = GetColor(COLORINDEX_NORMALTEXT);
		}
		else
		{
			crBkgnd = CLR_NONE;
			crText = CLR_NONE;
		}
	}
}

/**
 * @brief Selects diff by number and syncs other file
 * @param [in] nDiff Diff to select, must be >= 0
 * @param [in] bScroll Scroll diff to view
 * @param [in] bSelectText Select diff text
 * @sa CMergeEditView::ShowDiff()
 * @sa CChildFrame::SetCurrentDiff()
 * @todo Parameter bSelectText is never used?
 */
void CMergeEditView::SelectDiff(int nDiff, bool bScroll)
{
	// Check that nDiff is valid
	if (nDiff < 0)
		TRACE("Diffnumber negative (%d)", nDiff);
	if (nDiff >= m_pDocument->m_diffList.GetSize())
		TRACE("Selected diff > diffcount (%d >= %d)",
			nDiff, m_pDocument->m_diffList.GetSize());

	SelectNone();
	m_pDocument->SetCurrentDiff(nDiff);
	ShowDiff(bScroll);
	m_pDocument->UpdateAllViews();

	// notify either side, as it will notify the other one
	m_pDocument->GetLeftDetailView()->OnDisplayDiff(nDiff);
	m_pDocument->GetRightDetailView()->OnDisplayDiff(nDiff);
}

/**
 * @brief Called when user selects "Current Difference".
 * Goes to active diff. If no active diff, selects diff under cursor
 * @sa CMergeEditView::SelectDiff()
 * @sa CChildFrame::GetCurrentDiff()
 * @sa CChildFrame::LineToDiff()
 */
void CMergeEditView::OnCurdiff()
{
	// GetCurrentDiff() returns -1 if no diff selected
	int nDiff = m_pDocument->GetCurrentDiff();
	if (nDiff == -1)
	{
		// If cursor is inside diff, select that diff
		nDiff = m_pDocument->m_diffList.LineToDiff(m_ptCursorPos.y);
	}
	if (nDiff != -1 && m_pDocument->m_diffList.IsDiffSignificant(nDiff))
		SelectDiff(nDiff);
}

/**
 * @brief Go to first diff
 *
 * Called when user selects "First Difference"
 * @sa CMergeEditView::SelectDiff()
 */
void CMergeEditView::OnFirstdiff()
{
	int nDiff = m_pDocument->m_diffList.FirstSignificantDiff();
	if (nDiff != -1)
		SelectDiff(nDiff);
}

/**
 * @brief Go to last diff
 */
void CMergeEditView::OnLastdiff()
{
	int nDiff = m_pDocument->m_diffList.LastSignificantDiff();
	if (nDiff != -1)
		SelectDiff(nDiff);
}

/**
 * @brief Go to next diff and select it.
 *
 * Finds and selects next difference. There are several cases:
 * - if there is selected difference, and that difference is visible
 * on screen, next found difference is selected.
 * - if there is selected difference but it is not visible, next
 * difference from cursor position is selected. This is what user
 * expects to happen and is natural thing to do. Also reduces
 * needless scrolling.
 * - if there is no selected difference, next difference from cursor
 * position is selected.
 */
void CMergeEditView::OnNextdiff()
{
	// Returns -1 if no diff selected
	int curDiff = m_pDocument->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int nextDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select next from cursor
			nextDiff = m_pDocument->m_diffList.NextSignificantDiffFromLine(m_ptCursorPos.y);
		}
		else
		{
			// Find out if there is a following significant diff
			nextDiff = m_pDocument->m_diffList.NextSignificantDiff(curDiff);
		}
		if (nextDiff == -1)
			nextDiff = curDiff;

		// nextDiff is the next one if there is one, else it is the one we're on
		SelectDiff(nextDiff);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = m_ptCursorPos.y;
		if (!IsValidTextPosY(line))
			line = m_nTopLine;
		curDiff = m_pDocument->m_diffList.NextSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff);
	}
}

/**
 * @brief Go to previous diff and select it.
 *
 * Finds and selects previous difference. There are several cases:
 * - if there is selected difference, and that difference is visible
 * on screen, previous found difference is selected.
 * - if there is selected difference but it is not visible, previous
 * difference from cursor position is selected. This is what user
 * expects to happen and is natural thing to do. Also reduces
 * needless scrolling.
 * - if there is no selected difference, previous difference from cursor
 * position is selected.
 */
void CMergeEditView::OnPrevdiff()
{
	// GetCurrentDiff() returns -1 if no diff selected
	int curDiff = m_pDocument->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int prevDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select previous from cursor
			prevDiff = m_pDocument->m_diffList.PrevSignificantDiffFromLine(m_ptCursorPos.y);
		}
		else
		{
			// Find out if there is a preceding significant diff
			prevDiff = m_pDocument->m_diffList.PrevSignificantDiff(curDiff);
		}
		if (prevDiff == -1)
			prevDiff = curDiff;
		// prevDiff is the preceding one if there is one, else it is the one we're on
		SelectDiff(prevDiff);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = m_ptCursorPos.y;
		if (!IsValidTextPosY(line))
			line = m_nTopLine;
		curDiff = m_pDocument->m_diffList.PrevSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff);
	}
}

/**
 * @brief Clear selection
 */
void CMergeEditView::SelectNone()
{
	SetSelection(m_ptCursorPos, m_ptCursorPos);
	UpdateCaret();
}

/**
 * @brief Check if line is inside currently selected diff
 * @param [in] nLine 0-based linenumber in view
 * @sa CChildFrame::GetCurrentDiff()
 * @sa CChildFrame::LineInDiff()
 */
bool CMergeEditView::IsLineInCurrentDiff(int nLine)
{
	// Check validity of nLine
#ifdef _DEBUG
	if (nLine < 0)
		_RPTF1(_CRT_ERROR, "Linenumber is negative (%d)!", nLine);
	int nLineCount = m_pTextBuffer->GetLineCount();
	if (nLine >= nLineCount)
		_RPTF2(_CRT_ERROR, "Linenumber > linecount (%d>%d)!", nLine, nLineCount);
#endif

	int curDiff = m_pDocument->GetCurrentDiff();
	if (curDiff == -1)
		return false;
	return m_pDocument->m_diffList.LineInDiff(nLine, curDiff);
}

/**
 * @brief Called when mouse left-button double-clicked
 *
 * Double-clicking mouse inside diff selects that diff
 */
void CMergeEditView::OnLButtonDblClk()
{
	int diff = m_pDocument->m_diffList.LineToDiff(m_ptCursorPos.y);
	if (diff != -1 && m_pDocument->m_diffList.IsDiffSignificant(diff))
		SelectDiff(diff, false);
}

/**
 * @brief Called when mouse left button is released.
 *
 * If button is released outside diffs, current diff
 * is deselected.
 */
void CMergeEditView::OnLButtonUp()
{
	// If we have a selected diff, deselect it
	int nCurrentDiff = m_pDocument->GetCurrentDiff();
	if (nCurrentDiff != -1)
	{
		if (!IsLineInCurrentDiff(m_ptCursorPos.y))
		{
			m_pDocument->SetCurrentDiff(-1);
			m_pDocument->UpdateAllViews();
		}
	}
}

/**
 * @brief Finds longest line (needed for scrolling etc).
 * @sa CCrystalTextView::GetMaxLineLength()
 */
void CMergeEditView::UpdateLineLengths()
{
	GetMaxLineLength();
}

/**
 * @brief This function is called before other edit events.
 * @param [in] nAction Edit operation to do
 * @param [in] pszText Text to insert, delete etc
 * @sa CCrystalEditView::OnEditOperation()
 * @todo More edit-events for rescan delaying?
 */
void CMergeEditView::OnEditOperation(int nAction, LPCTSTR pszText)
{
	if (!QueryEditable())
	{
		// We must not arrive here, and assert helps detect troubles
		ASSERT(FALSE);
		return;
	}

	m_pDocument->SetEditedAfterRescan(m_nThisPane);

	// perform original function
	CCrystalEditViewEx::OnEditOperation(nAction, pszText);

	// augment with additional operations

	// Change header to inform about changed doc
	m_pDocument->UpdateHeaderPath(m_nThisPane);
	// Update Edit Command UI
	m_pDocument->UpdateEditCmdUI();

	// If automatic rescan enabled, rescan after edit events
	if (m_bAutomaticRescan)
	{
		// keep document up to date
		// (Re)start timer to rescan only when user edits text
		// If timer starting fails, rescan immediately
		switch (nAction)
		{
		case CE_ACTION_TYPING:
		case CE_ACTION_REPLACE:
		case CE_ACTION_BACKSPACE:
		case CE_ACTION_INDENT:
		case CE_ACTION_PASTE:
		case CE_ACTION_DELSEL:
		case CE_ACTION_DELETE:
		case CE_ACTION_CUT:
			if (!SetTimer(IDT_RESCAN, RESCAN_TIMEOUT))
			{
			default:
				m_pDocument->FlushAndRescan();
			}
			break;
		}
	}
	else
	{
		if (m_bWordWrap)
		{
			// Update other pane for sync line.
			if (CCrystalEditView *const pView = m_pDocument->GetView(1 - m_nThisPane))
			{
				pView->Invalidate();
			}
		}
	}
}

/**
 * @brief Scrolls to current diff and/or selects diff text
 * @param [in] bScroll If TRUE scroll diff to view
 * @param [in] bSelectText If TRUE select diff text
 * @note If bScroll and bSelectText are FALSE, this does nothing!
 * @todo This shouldn't be called when no diff is selected, so
 * somebody could try to ASSERT(nDiff > -1)...
 */
void CMergeEditView::ShowDiff(bool bScroll)
{
	const int nDiff = m_pDocument->GetCurrentDiff();

	// Try to trap some errors
	if (nDiff >= m_pDocument->m_diffList.GetSize())
		TRACE("Selected diff > diffcount (%d > %d)!",
			nDiff, m_pDocument->m_diffList.GetSize());

	CMergeEditView *const pCurrentView = m_pDocument->GetView(m_nThisPane);
	CMergeEditView *const pOtherView = m_pDocument->GetView(1 - m_nThisPane);

	if (nDiff >= 0 && nDiff < m_pDocument->m_diffList.GetSize())
	{
		POINT ptStart, ptEnd;
		const DIFFRANGE *curDiff = m_pDocument->m_diffList.DiffRangeAt(nDiff);

		ptStart.x = 0;
		ptStart.y = curDiff->dbegin0;
		ptEnd.x = 0;
		ptEnd.y = curDiff->dend0;

		if (bScroll && !IsDiffVisible(curDiff, CONTEXT_LINES_BELOW))
		{
			// Difference is not visible, scroll it so that max amount of
			// scrolling is done while keeping the diff in screen. So if
			// scrolling is downwards, scroll the diff to as up in screen
			// as possible. This usually brings next diff to the screen
			// and we don't need to scroll into it.
			int nLine = GetSubLineIndex(ptStart.y) - CONTEXT_LINES_ABOVE;
			if (nLine < 0)
				nLine = 0;
			pCurrentView->ScrollToSubLine(nLine);
			pOtherView->ScrollToSubLine(nLine);
		}

		pCurrentView->SetCursorPos(ptStart);
		pOtherView->SetCursorPos(ptStart);
		pCurrentView->SetAnchor(ptStart);
		pOtherView->SetAnchor(ptStart);
		pCurrentView->SetSelection(ptStart, ptStart);
		pOtherView->SetSelection(ptStart, ptStart);

		Invalidate();
	}
}


void CMergeEditView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_RESCAN)
	{
		KillTimer(IDT_RESCAN);
		m_pDocument->RescanIfNeeded(RESCAN_TIMEOUT / 1000);
	}
}

/**
 * @brief Enable/Disable automatic rescanning
 */
bool CMergeEditView::EnableRescan(bool bEnable)
{
	bool bOldValue = m_bAutomaticRescan;
	m_bAutomaticRescan = bEnable;
	return bOldValue;
}

/**
 * @brief Update statusbar info, Override from CCrystalTextView
 * @note we tab-expand column, but we don't tab-expand char count,
 * since we want to show how many chars there are and tab is just one
 * character although it expands to several spaces.
 */
void CMergeEditView::OnUpdateCaret(bool bShowHide)
{
	if (!IsTextBufferInitialized())
		return;

	if (bShowHide)
		return;

	if (!m_bCursorHidden)
		m_bMergeUndo = false;

	UpdateLineInfoStatus();
}

void CMergeEditView::UpdateLineInfoStatus()
{
	int nScreenLine = m_ptCursorPos.y;
	const int nRealLine = ComputeRealLine(nScreenLine);
	TCHAR sLine[40];
	int chars = -1;
	LPCTSTR sEol = _T("hidden");
	int column = -1;
	int columns = -1;
	int curChar = -1;
	DWORD dwLineFlags = m_pTextBuffer->GetLineFlags(nScreenLine);
	// Is this a ghost line ?
	if (dwLineFlags & LF_GHOST)
	{
		// Ghost lines display eg "Line 12-13"
		wsprintf(sLine, _T("%d-%d"), nRealLine, nRealLine + 1);
	}
	else
	{
		// Regular lines display eg "Line 13 Characters: 25 EOL: CRLF"
		wsprintf(sLine, _T("%d"), nRealLine + 1);
		curChar = m_ptCursorPos.x + 1;
		chars = GetLineLength(nScreenLine);
		column = CalculateActualOffset(nScreenLine, m_ptCursorPos.x, true) + 1;
		columns = CalculateActualOffset(nScreenLine, chars, true) + 1;
		chars++;
		if (COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) ||
			m_pDocument->IsMixedEOL(m_nThisPane))
		{
			sEol = GetTextBufferEol(nScreenLine);
		}
	}
	EDITMODE editMode = !QueryEditable() ? EDIT_MODE_READONLY :
		GetOverwriteMode() ? EDIT_MODE_OVERWRITE : EDIT_MODE_INSERT;
	SetLineInfoStatus(sLine, column, columns, curChar, chars, sEol, editMode);

	// Update Command UI
	m_pDocument->UpdateGeneralCmdUI();
}

/**
 * @brief Offer a context menu built with scriptlet/ActiveX functions
 */
void CMergeEditView::OnContextMenu(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	// Create the menu and populate it with the available functions
	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_MERGEVIEW);
	HMenu *const pSub = pMenu->GetSubMenu(0);
	SetFocus();
	// Remove copying item copying from active side
	if (m_nThisPane == 0) // left?
		pSub->RemoveMenu(ID_R2L, MF_BYCOMMAND);
	else
		pSub->RemoveMenu(ID_L2R, MF_BYCOMMAND);
	// Context menu opened using keyboard has no coordinates
	if (point.x == -1 && point.y == -1)
	{
		point.x = point.y = 5;
		ClientToScreen(&point);
	}

	// If view is editable, add available editor scripts
	HMenu *pScriptMenu = NULL;
	if (QueryEditable())
	{
		pSub->AppendMenu(MF_STRING, ID_SCRIPT_LAST, _T("PluginMonikers"));
		pScriptMenu = theApp.m_pMainWnd->SetScriptMenu(pSub, "EditorScripts.Menu");
		if (pScriptMenu->GetMenuItemCount())
			pSub->InsertMenu(ID_SCRIPT_FIRST, MF_SEPARATOR);
	}

	if (COptionsMgr::Get(OPT_MERGEEDITVIEW_ENABLE_SHELL_CONTEXT_MENU))
	{
		pSub->AppendMenu(MF_SEPARATOR, 0, NULL);
		String s = LanguageSelect.LoadString(IDS_SHELL_CONTEXT_MENU);
		m_hShellContextMenu = ::CreatePopupMenu();
		pSub->AppendMenu(MF_POPUP, (UINT_PTR)m_hShellContextMenu, s.c_str());
	}

	int nCmd = pSub->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		point.x, point.y, theApp.m_pMainWnd->m_pWnd);
	if (nCmd >= ID_SCRIPT_FIRST && nCmd <= ID_SCRIPT_LAST) try
	{
		TCHAR method[INFOTIPSIZE];
		pScriptMenu->GetMenuString(nCmd, method, INFOTIPSIZE);
		String pluginMoniker;
		if (LPTSTR p = _tcschr(method, _T('@')))
		{
			*p++ = _T('\0');
			pluginMoniker = p;
		}
		env_ResolveMoniker(pluginMoniker);
		CMyComPtr<IDispatch> spDispatch;
		OException::Check(CoGetObject(pluginMoniker.c_str(), NULL,
			IID_IDispatch, reinterpret_cast<void **>(&spDispatch)));
		CMyDispId DispId;

		if (SUCCEEDED(DispId.Init(spDispatch, L"ShowConsole")))
		{
			CMyVariant var;
			OException::Check(DispId.Call(spDispatch,
				CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
			OException::Check(var.ChangeType(VT_UI2));
			ShowConsoleWindow(V_UI2(&var));
		}

		OException::Check(DispId.Init(spDispatch, method));
		String text;
		if (IsSelection())
		{
			POINT ptStart, ptEnd;
			GetSelection(ptStart, ptEnd);
			GetText(ptStart, ptEnd, text);
		}
		CMyVariant var;
		OException::Check(DispId.Call(spDispatch,
			CMyDispParams<1>().Unnamed[text.c_str()], DISPATCH_METHOD, &var));
		if (V_VT(&var) != VT_EMPTY)
		{
			OException::Check(var.ChangeType(VT_BSTR));
			BSTR bstr = V_BSTR(&var);
			ReplaceSelection(bstr, SysStringLen(bstr), 0);
		}
	}
	catch (OException *e)
	{
		e->ReportError(m_hWnd, MB_ICONSTOP);
		delete e;
	}
	else if (!m_pShellContextMenu->InvokeCommand(nCmd, m_pDocument->m_hWnd))
	{
		m_pDocument->PostMessage(WM_COMMAND, nCmd);
	}
	m_hShellContextMenu = NULL;
	if (pScriptMenu)
	{
		theApp.m_pMainWnd->SetScriptMenu(pSub, NULL);
		pScriptMenu->DestroyMenu();
	}
	pMenu->DestroyMenu();
}

/**
 * @brief Change EOL mode and unify all the lines EOL to this new mode
 */
void CMergeEditView::OnConvertEolTo(UINT nID)
{
	CRLFSTYLE nStyle = CRLF_STYLE_AUTOMATIC;
	switch (nID)
	{
	case ID_EOL_TO_DOS:
		nStyle = CRLF_STYLE_DOS;
		break;
	case ID_EOL_TO_UNIX:
		nStyle = CRLF_STYLE_UNIX;
		break;
	case ID_EOL_TO_MAC:
		nStyle = CRLF_STYLE_MAC;
		break;
	default:
		// Catch errors
		_RPTF0(_CRT_ERROR, "Unhandled EOL type conversion!");
		break;
	}
	m_pTextBuffer->SetCRLFMode(nStyle);

	// we don't need a derived applyEOLMode for ghost lines as they have no EOL char
	if (m_pTextBuffer->applyEOLMode())
	{
		m_pDocument->UpdateHeaderPath(m_nThisPane);
		m_pDocument->FlushAndRescan(true);
	}
}

/**
 * @brief Reload options.
 */
void CMergeEditView::RefreshOptions()
{ 
	// Enable/disable automatic rescan (rescanning after edit)
	m_bAutomaticRescan = COptionsMgr::Get(OPT_AUTOMATIC_RESCAN);
	// Set tab type (tabs/spaces)
	m_pTextBuffer->SetInsertTabs(COptionsMgr::Get(OPT_TAB_TYPE) == 0);
	SetSelectionMargin(COptionsMgr::Get(OPT_VIEW_FILEMARGIN));

	if (!COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT))
		SetTextType(CCrystalTextView::SRC_PLAIN);

	// SetTextType will revert to language dependent defaults for tab
	SetTabSize(COptionsMgr::Get(OPT_TAB_SIZE),
		COptionsMgr::Get(OPT_SEPARATE_COMBINING_CHARS));
	SetViewTabs(COptionsMgr::Get(OPT_VIEW_WHITESPACE));

	SetWordWrapping(COptionsMgr::Get(OPT_WORDWRAP));
	SetViewLineNumbers(COptionsMgr::Get(OPT_VIEW_LINENUMBERS));

	const bool mixedEOLs = COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) ||
		m_pDocument->IsMixedEOL(m_nThisPane);
	SetViewEols(COptionsMgr::Get(OPT_VIEW_WHITESPACE), mixedEOLs);

	m_cachedColors.clrDiff = COptionsMgr::Get(OPT_CLR_DIFF);
	m_cachedColors.clrSelDiff = COptionsMgr::Get(OPT_CLR_SELECTED_DIFF);
	m_cachedColors.clrDiffDeleted = COptionsMgr::Get(OPT_CLR_DIFF_DELETED);
	m_cachedColors.clrSelDiffDeleted = COptionsMgr::Get(OPT_CLR_SELECTED_DIFF_DELETED);
	m_cachedColors.clrDiffText = COptionsMgr::Get(OPT_CLR_DIFF_TEXT);
	m_cachedColors.clrSelDiffText = COptionsMgr::Get(OPT_CLR_SELECTED_DIFF_TEXT);
	m_cachedColors.clrTrivial = COptionsMgr::Get(OPT_CLR_TRIVIAL_DIFF);
	m_cachedColors.clrTrivialDeleted = COptionsMgr::Get(OPT_CLR_TRIVIAL_DIFF_DELETED);
	m_cachedColors.clrTrivialText = COptionsMgr::Get(OPT_CLR_TRIVIAL_DIFF_TEXT);
	m_cachedColors.clrMoved = COptionsMgr::Get(OPT_CLR_MOVEDBLOCK);
	m_cachedColors.clrMovedDeleted = COptionsMgr::Get(OPT_CLR_MOVEDBLOCK_DELETED);
	m_cachedColors.clrMovedText = COptionsMgr::Get(OPT_CLR_MOVEDBLOCK_TEXT);
	m_cachedColors.clrSelMoved = COptionsMgr::Get(OPT_CLR_SELECTED_MOVEDBLOCK);
	m_cachedColors.clrSelMovedDeleted = COptionsMgr::Get(OPT_CLR_SELECTED_MOVEDBLOCK_DELETED);
	m_cachedColors.clrSelMovedText = COptionsMgr::Get(OPT_CLR_SELECTED_MOVEDBLOCK_TEXT);
	m_cachedColors.clrWordDiff = COptionsMgr::Get(OPT_CLR_WORDDIFF);
	m_cachedColors.clrWordDiffDeleted = COptionsMgr::Get(OPT_CLR_WORDDIFF_DELETED);
	m_cachedColors.clrSelWordDiff = COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF);
	m_cachedColors.clrSelWordDiffDeleted = COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF_DELETED);
	m_cachedColors.clrWordDiffText = COptionsMgr::Get(OPT_CLR_WORDDIFF_TEXT);
	m_cachedColors.clrSelWordDiffText = COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF_TEXT);

	OnSize();
	CCrystalTextView::OnSize();
	Invalidate();
}

/**
 * @brief Goto given line.
 * @param [in] nLine Apparent destination line number (including deleted lines)
 */
void CMergeEditView::GotoLine(int nLine)
{
	CMergeEditView *const pLeftView = m_pDocument->GetLeftView();
	CMergeEditView *const pRightView = m_pDocument->GetRightView();

	// Scroll line to center of view
	int nScrollLine = GetSubLineIndex(nLine) - GetScreenLines() / 2;
	if (nScrollLine < 0)
		nScrollLine = 0;

	pLeftView->ScrollToSubLine(nScrollLine);
	pRightView->ScrollToSubLine(nScrollLine);
	POINT ptPos;
	ptPos.x = 0;
	ptPos.y = min(nLine, pLeftView->GetSubLineCount() - 1);
	pLeftView->SetCursorPos(ptPos);
	pLeftView->SetAnchor(ptPos);
	ptPos.y = min(nLine, pRightView->GetSubLineCount() - 1);
	pRightView->SetCursorPos(ptPos);
	pRightView->SetAnchor(ptPos);
}

/**
 * @brief Copy selected lines adding linenumbers.
 */
void CMergeEditView::OnEditCopyLineNumbers()
{
	if (!OpenClipboard())
		return;
	UniStdioFile file;
	file.SetUnicoding(UCS2LE);
	if (HGLOBAL hMem = file.CreateStreamOnHGlobal())
	{
		POINT ptStart;
		POINT ptEnd;
		GetSelection(ptStart, ptEnd);
		// Get last selected line (having widest linenumber)
		UINT line = m_pDocument->m_ptBuf[1]->ComputeRealLine(ptEnd.y);
		TCHAR strNum[20];
		int nNumWidth = _sntprintf(strNum, _countof(strNum), _T("%d"), line + 1);
		for (int i = ptStart.y; i <= ptEnd.y; i++)
		{
			if (GetLineFlags(i) & LF_GHOST)
				continue;
			// We need to convert to real linenumbers
			line = m_pDocument->m_ptBuf[m_nThisPane]->ComputeRealLine(i);
			int cchNum = _sntprintf(strNum, _countof(strNum), _T("%*d: "), nNumWidth, line + 1);
			file.WriteString(strNum, cchNum);
			if (LPCTSTR pszLine = GetLineChars(i))
			{
				int cchLine = GetFullLineLength(i);
				file.WriteString(pszLine, cchLine);
			}
		}
		file.WriteString(_T(""), 1);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
	}
	CloseClipboard();
}

void CMergeEditView::OnSize() 
{
	if (HStatusBar *const pBar = m_pStatusBar)
	{
		RECT rect;
		GetWindowRect(&rect);
		rect.right -= rect.left;
		if (pBar->GetStyle() & SBS_SIZEGRIP)
			rect.right -= ::GetSystemMetrics(SM_CXVSCROLL);
		int parts[4];
		parts[3] = rect.right;
		rect.right -= EOL_PANEL_WIDTH;
		parts[2] = rect.right;
		rect.right -= ENCODING_PANEL_WIDTH;
		parts[1] = rect.right;
		rect.right -= RO_PANEL_WIDTH;
		parts[0] = rect.right;
		pBar->SetParts(4, parts);
	}
}

int CMergeEditView::RecalcVertScrollBar(bool bPositionOnly)
{
	int nPos = CGhostTextView::RecalcVertScrollBar(bPositionOnly);
	if (bPositionOnly)
		m_pDocument->m_wndLocationView.UpdateVisiblePos(nPos, nPos + GetScreenLines());
	else
		m_pDocument->m_wndLocationView.ForceRecalculate();
	return nPos;
}

/**
 * @brief returns the number of empty lines which are added for synchronizing the line in two panes.
 */
int CMergeEditView::GetEmptySubLines(int nLineIndex)
{
	int	nBreaks[2] = { 0, 0 };
	if (CMergeEditView *pLeftView = m_pDocument->GetLeftView())
	{
		if (nLineIndex >= pLeftView->GetLineCount())
			return 0;
		pLeftView->WrapLineCached(nLineIndex, NULL, nBreaks[0]);
	}
	if (CMergeEditView *pRightView = m_pDocument->GetRightView())
	{
		if (nLineIndex >= pRightView->GetLineCount())
			return 0;
		pRightView->WrapLineCached(nLineIndex, NULL, nBreaks[1]);
	}

	if (nBreaks[m_nThisPane] < nBreaks[1 - m_nThisPane])
		return nBreaks[1 - m_nThisPane] - nBreaks[m_nThisPane];
	else
		return 0;
}

/**
 * @brief Invalidate sub line index cache from the specified index to the end of file.
 * @param [in] nLineIndex Index of the first line to invalidate 
 */
void CMergeEditView::InvalidateSubLineIndexCache(int nLineIndex)
{
    // We have to invalidate sub line index cache on both panes.
	for (int nPane = 0; nPane < 2; nPane++) 
	{
		if (CMergeEditView *pView = m_pDocument->GetView(nPane))
			pView->CCrystalTextView::InvalidateSubLineIndexCache(nLineIndex);
	}
}

/**
* @brief Determine if difference is visible on screen.
* @param [in] nDiff Number of diff to check.
* @return TRUE if difference is visible.
*/
bool CMergeEditView::IsDiffVisible(int nDiff)
{
	const DIFFRANGE *diff = m_pDocument->m_diffList.DiffRangeAt(nDiff);
	return diff && IsDiffVisible(diff);
}

/**
 * @brief Determine if difference is visible on screen.
 * @param [in] diff diff to check.
 * @param [in] nLinesBelow Allow "minimizing" the number of visible lines.
 * @return TRUE if difference is visible, FALSE otherwise.
 */
bool CMergeEditView::IsDiffVisible(const DIFFRANGE *diff, int nLinesBelow /*=0*/)
{
	const int nDiffStart = GetSubLineIndex(diff->dbegin0);
	const int nDiffEnd = GetSubLineIndex(diff->dend0);
	// Diff's height is last line - first line + last line's line count
	const int nDiffHeight = nDiffEnd - nDiffStart + GetSubLines(diff->dend0) + 1;

	// If diff first line outside current view - context OR
	// if diff last line outside current view - context OR
	// if diff is bigger than screen
	if ((nDiffStart < m_nTopSubLine) ||
		(nDiffEnd >= m_nTopSubLine + GetScreenLines() - nLinesBelow) ||
		(nDiffHeight >= GetScreenLines()))
	{
		return false;
	}
	else
	{
		return true;
	}
}

/**
 * @brief Called after document is loaded.
 * This function is called from CChildFrame::OpenDocs() after documents are
 * loaded. So this is good place to set View's options etc.
 */
void CMergeEditView::DocumentsLoaded()
{
	RefreshOptions();
	SetFont(theApp.m_pMainWnd->m_lfDiff);
	UpdateLineInfoStatus();
}

/// Visible representation of eol
static String EolString(const String & sEol)
{
	if (sEol == _T("\r\n"))
	{
		return LanguageSelect.LoadString(IDS_EOL_CRLF);
	}
	if (sEol == _T("\n"))
	{
		return LanguageSelect.LoadString(IDS_EOL_LF);
	}
	if (sEol == _T("\r"))
	{
		return LanguageSelect.LoadString(IDS_EOL_CR);
	}
	if (sEol.empty())
	{
		return LanguageSelect.LoadString(IDS_EOL_NONE);
	}
	if (sEol == _T("hidden"))
		return _T("");
	return _T("?");
}

/// Receive status line info from crystal window and display
void CMergeEditView::SetLineInfoStatus(LPCTSTR szLine, int nColumn,
	int nColumns, int nChar, int nChars, LPCTSTR szEol, EDITMODE editMode)
{
	if (m_pStatusBar)
	{
		String sEolDisplay = EolString(szEol);
		UINT id = nChars == -1 ? IDS_EMPTY_LINE_STATUS_INFO :
			sEolDisplay.empty() ? IDS_LINE_STATUS_INFO :
			IDS_LINE_STATUS_INFO_EOL;
		m_pStatusBar->SetPartText(0, LanguageSelect.Format(id, szLine, nColumn, nColumns,
			nChar, nChars, sEolDisplay.c_str()), 0);
		String sText = editMode == EDIT_MODE_READONLY ? _T("READ") :
			editMode == EDIT_MODE_INSERT ? _T("INS") : _T("OVR");
		m_pStatusBar->SetPartText(PANE_RO, sText.c_str(), 0);
	}
}

void CMergeEditView::SetEncodingStatus(LPCTSTR text)
{
	m_pStatusBar->SetPartText(PANE_ENCODING, text, 0);
}

void CMergeEditView::SetCRLFModeStatus(CRLFSTYLE crlfMode)
{
	UINT id = 0;
	switch (crlfMode)
	{
	case CRLF_STYLE_DOS:
		id = IDS_EOL_DOS;
		break;
	case CRLF_STYLE_UNIX:
		id = IDS_EOL_UNIX;
		break;
	case CRLF_STYLE_MAC:
		id = IDS_EOL_MAC;
		break;
	case CRLF_STYLE_MIXED:
		id = IDS_EOL_MIXED;
		break;
	}
	String sText;
	if (id)
		sText = LanguageSelect.LoadString(id);
	m_pStatusBar->SetPartText(PANE_EOL, sText.c_str(), 0);
}

HMenu *CMergeEditView::ApplyPatch(IStream *pstm, int id)
{
	IStream_Reset(pstm);
	HMenu *pMenu = id ? NULL : HMenu::CreatePopupMenu();
	CDiffTextBuffer *const pTextBuffer = GetTextBuffer();
	StreamLineReader reader = pstm;
	std::string line;
	OString text = NULL;
	unsigned pos1, len1, pos2, len2;
	TRACE("#BOF#\n");
	while (reader.readLine(line))
	{
		if (line.find_first_not_of('-') == 3)
		{
			std::replace(line.begin(), line.end(), '\t', ' ');
			text.Free();
			text.Append(HString::Oct(line.c_str())->Uni(CP_UTF8)->Trim());
		}
		else if (line.find_first_not_of('+') == 3)
		{
			std::replace(line.begin(), line.end(), '\t', ' ');
			text.Append(L"\t");
			text.Append(HString::Oct(line.c_str())->Uni(CP_UTF8)->Trim());
			if (pMenu)
				pMenu->AppendMenu(MF_STRING, ++id, text.W);
			else
				--id;
		}
		else if (sscanf(line.c_str(), "@@ -%u,%u +%u,%u @@", &pos1, &len1, &pos2, &len2) == 4)
		{
			TRACE("%s", line.c_str());
			// Continue with zero-based line numbers
			--pos1;
			--pos2;
			// Verification starts at pos1 or at pos2, depending on whether we
			// operate in verify-only mode or in apply mode.
			if (pMenu == NULL && id == 0)
				pos1 = pos2;
			std::vector<CMyComBSTR> vec1(len1);
			std::vector<CMyComBSTR> vec2(len2);
			len1 = len2 = 0;
			while (len1 < vec1.size() || len2 < vec2.size())
			{
				if (!reader.readLine(line))
					break;
				const char *pc = line.c_str();
				int c = *pc++;
				switch (c) case '\x20': case '-':
				{
					if (len1 < vec1.size())
						vec1[len1].m_str = HString::Oct(pc)->Uni(CP_UTF8)->B;
					++len1;
				}
				switch (c) case '\x20': case '+':
				{
					if (len2 < vec2.size())
						vec2[len2].m_str = HString::Oct(pc)->Uni(CP_UTF8)->B;
					++len2;
				}
			}
			if (len1 == vec1.size() && len2 == vec2.size())
			{
				TRACE("#VALID ");
				int nLineCount = pTextBuffer->GetLineCount();
				unsigned i;
				for (i = 0 ; i < len1 ; ++i)
				{
					int nApparentLine = pTextBuffer->ComputeApparentLine(pos1 + i);
					if (nApparentLine < 0 || nApparentLine >= nLineCount)
					{
						break;
					}
					LPCTSTR pchText = pTextBuffer->GetLineChars(nApparentLine);
					int cchText = pTextBuffer->GetLineLength(nApparentLine);
					BSTR bstrText = vec1[i];
					if (StrCmpNW(pchText, bstrText, cchText) ||
						bstrText[cchText] && !LineInfo::IsEol(bstrText[cchText]))
					{
						break;
					}
				}
				if (i == len1)
				{
					TRACE("EVEN FOR THIS FILE#\n");
					// If in apply mode, replace text accordingly.
					if (pMenu == NULL && id == 0)
					{
						pTextBuffer->BeginUndoGroup();
						int nStartLine = pTextBuffer->ComputeApparentLine(pos2);
						int nEndLine = pTextBuffer->ComputeApparentLine(pos2 + len1);
						pTextBuffer->DeleteText(this, nStartLine, 0, nEndLine, 0);
						for (i = 0 ; i < len2 ; ++i)
						{
							BSTR bstrText = vec2[i];
							int nApparentLine = pTextBuffer->ComputeApparentLine(pos2 + i);
							pTextBuffer->InsertText(this, nApparentLine, 0,
								bstrText, SysStringLen(bstrText));
						}
						pTextBuffer->FlushUndoGroup(this);
					}
				}
				else
				{
					TRACE("BUT NOT FOR THIS FILE#\n");
					if (pMenu)
						pMenu->EnableMenuItem(id, MF_GRAYED);
				}
			}
			else
			{
				TRACE("#INVALID#\n");
			}
		}
	}
	TRACE("#EOF#\n");
	return pMenu;
}

void CMergeEditView::ApplyPatch(IStream *pstm, const POINTL &pt)
{
	if (HMenu *pMenu = ApplyPatch(pstm, 0))
	{
		if (int id = pMenu->TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_pWnd))
		{
			ApplyPatch(pstm, id);
		}
		pMenu->DestroyMenu();
	}
}

HRESULT CMergeEditView::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	HRESULT hr;
	if ((hr = CMyFormatEtc(CF_HDROP).QueryGetData(pDataObj)) == S_OK ||
		(hr = CMyFormatEtc(CFSTR_FILEDESCRIPTORA).QueryGetData(pDataObj)) == S_OK)
	{
		DragOver(grfKeyState, pt, pdwEffect);
	}
	else
	{
		hr = CGhostTextView::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
	}
	return hr;
}

HRESULT CMergeEditView::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	HideDropIndicator();
	CMyStgMedium stgmedium;
	HRESULT hr;
	if ((hr = CMyFormatEtc(CF_HDROP).GetData(pDataObj, &stgmedium)) == S_OK)
	{
		HDROP dropInfo = reinterpret_cast<HDROP>(stgmedium.hGlobal);
		// Get the number of pathnames that have been dropped
		UINT fileCount = DragQueryFile(dropInfo, 0xFFFFFFFF, NULL, 0);
		if (fileCount == 1)
		{
			String fileName;
			// Get the number of bytes required by the file's full pathname
			if (UINT len = DragQueryFile(dropInfo, 0, NULL, 0))
			{
				fileName.resize(len);
				DragQueryFile(dropInfo, 0, &fileName.front(), len + 1);
			}
			if (PathMatchSpec(fileName.c_str(), _T("*.patch")))
			{
				CMyComPtr<IStream> pstm;
				if (SUCCEEDED(hr = SHCreateStreamOnFile(fileName.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, &pstm)))
				{
					ApplyPatch(pstm, pt);
				}
			}
		}
		*pdwEffect = DROPEFFECT_COPY;
	}
	else if ((hr = CMyFormatEtc(CFSTR_FILEDESCRIPTORA).GetData(pDataObj, &stgmedium)) == S_OK)
	{
		FILEGROUPDESCRIPTORA *data = reinterpret_cast<FILEGROUPDESCRIPTORA *>(GlobalLock(stgmedium.hGlobal));
		FILEDESCRIPTORA *pfd = data->fgd;
		if (data->cItems == 1)
		{
			if (PathMatchSpecA(pfd->cFileName, "*.patch"))
			{
				CMyFormatEtc formatetc(CFSTR_FILECONTENTS, 0, TYMED_ISTREAM);
				CMyStgMedium stgmedium;
				if ((hr = formatetc.GetData(pDataObj, &stgmedium)) == S_OK)
				{
					ApplyPatch(stgmedium.pstm, pt);
				}
			}
		}
		*pdwEffect = DROPEFFECT_COPY;
	}
	else
	{
		hr = CGhostTextView::Drop(pDataObj, grfKeyState, pt, pdwEffect);
	}
	return hr;
}

bool CMergeEditView::IsShellMenuCmdID(UINT id)
{
	return (id >= MergeViewCmdFirst) && (id <= MergeViewCmdLast);
}

void CMergeEditView::HandleMenuSelect(WPARAM wParam, LPARAM lParam)
{
	if ((HIWORD(wParam) & MF_POPUP) && (lParam != 0))
	{
		HMENU hMenu = reinterpret_cast<HMENU>(lParam);
		MENUITEMINFO mii;
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_SUBMENU;
		::GetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
		HMENU hSubMenu = mii.hSubMenu;
		if (hSubMenu == m_hShellContextMenu)
		{
			mii.hSubMenu = ListShellContextMenu();
			m_hShellContextMenu = NULL;
		}
		if (hSubMenu != mii.hSubMenu)
		{
			::SetMenuItemInfo(hMenu, LOWORD(wParam), TRUE, &mii);
			::DestroyMenu(hSubMenu);
		}
	}
}

/**
 * @brief Handle messages related to correct menu working.
 *
 * We need to requery shell context menu each time we switch from context menu
 * for one side to context menu for other side. Here we check whether we need to
 * requery and call ShellContextMenuHandleMenuMessage.
 */
LRESULT CMergeEditView::HandleMenuMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
	m_pShellContextMenu->HandleMenuMessage(message, wParam, lParam, res);
	return res;
}

/**
 * @brief Gets Explorer's context menu for a group of selected files.
 *
 * @param [in] Side whether to get context menu for the files from the left or
 *   right side.
 * @retval The HMENU.
 */
HMENU CMergeEditView::ListShellContextMenu()
{
	String file = m_pDocument->m_strPath[this->m_nThisPane];
	paths_UndoMagic(file);
	LPCTSTR fileName = PathFindFileName(file.c_str());
	
	CMyComPtr<IShellFolder> pDesktop;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);
	if (FAILED(hr))
		return NULL;

	String parentDir = paths_GetParentPath(file.c_str());
	CMyComPtr<IShellFolder> pCurrFolder;
	CPidlContainer pidls;

	LPITEMIDLIST dirPidl;
	paths_UndoMagic(parentDir);
	hr = pDesktop->ParseDisplayName(NULL, NULL,
		const_cast<LPOLESTR>(parentDir.c_str()), NULL, &dirPidl, NULL);
	if (FAILED(hr))
		return NULL;
	hr = pDesktop->BindToObject(dirPidl, NULL,
		IID_IShellFolder, reinterpret_cast<void**>(&pCurrFolder));
	pidls.Free(dirPidl);
	if (FAILED(hr))
		return NULL;
	LPITEMIDLIST pidl;
	hr = pCurrFolder->ParseDisplayName(NULL, NULL,
		const_cast<LPOLESTR>(fileName), NULL, &pidl, NULL);
	if (FAILED(hr))
		return NULL;
	pidls.Add(pidl);

	if (pCurrFolder == NULL)
		return NULL;

	CMyComPtr<IContextMenu> pContextMenu;
	hr = pCurrFolder->GetUIObjectOf(NULL,
		pidls.Size(), pidls.GetList(), IID_IContextMenu, 0,
		reinterpret_cast<void**>(&pContextMenu));
	if (FAILED(hr))
		return NULL;

	return m_pShellContextMenu->QueryShellContextMenu(pContextMenu);
}
