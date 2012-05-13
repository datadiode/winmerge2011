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
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "ChildFrm.h"
#include "LanguageSelect.h"
#include "LocationView.h"
#include "MergeEditView.h"
#include "MergeDiffDetailView.h"
#include "MainFrm.h"
#include "OptionsMgr.h"
#include "FileTransform.h"
#include "WMGotoDlg.h"
#include "OptionsDef.h"
#include "SyntaxColors.h"
#include "MergeLineFlags.h"
#include "Common/stream_util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using stl::vector;

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

/////////////////////////////////////////////////////////////////////////////
// CMergeEditView

CMergeEditView::CMergeEditView(
	HWindow *pWnd,
	CChildFrame *pDocument,
	int nThisPane,
	size_t ZeroInit
) : CGhostTextView(ZeroInit),
	m_pDocument(pDocument),
	m_nThisPane(nThisPane),
	m_bAutomaticRescan(false),
	m_pStatusBar(NULL)
{
	SubclassWindow(pWnd);
	RegisterDragDrop(m_hWnd, this);
	SetParser(&m_xParser);
}

CMergeEditView::~CMergeEditView()
{
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
				OnEditSwitchOvrmode();
				break;
			}
		}
	}
}

/**
 * @brief Return text buffer for file in view
 */
CDiffTextBuffer *CMergeEditView::LocateTextBuffer()
{
	return m_pDocument->m_ptBuf[m_nThisPane];
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CMergeEditView::UpdateResources()
{
	// Update annotated ghost lines when operating in limited context mode.
	Invalidate();
}

void CMergeEditView::PrimeListWithFile()
{
	// Set the tab size now, just in case the options change...
	// We don't update it at the end of OnOptions,
	// we can update it safely now
	SetTabSize(COptionsMgr::Get(OPT_TAB_SIZE));
}

/**
 * @brief Get diffs inside selection.
 * @param [out] firstDiff First diff inside selection
 * @param [out] lastDiff Last diff inside selection
 * @note -1 is returned in parameters if diffs cannot be determined
 * @todo This shouldn't be called when there is no diffs, so replace
 * first 'if' with ASSERT()?
 */
void CMergeEditView::GetFullySelectedDiffs(int & firstDiff, int & lastDiff)
{
	firstDiff = -1;
	lastDiff = -1;

	const int nDiffs = m_pDocument->m_diffList.GetSignificantDiffs();
	if (nDiffs == 0)
		return;

	int firstLine, lastLine;
	GetFullySelectedLines(firstLine, lastLine);
	if (lastLine < firstLine)
		return;

	firstDiff = m_pDocument->m_diffList.NextSignificantDiffFromLine(firstLine);
	lastDiff = m_pDocument->m_diffList.PrevSignificantDiffFromLine(lastLine);
	if (firstDiff != -1 && lastDiff != -1)
	{
		DIFFRANGE di;
		
		// Check that first selected line is first diff's first line or above it
		VERIFY(m_pDocument->m_diffList.GetDiff(firstDiff, di));
		if ((int)di.dbegin0 < firstLine)
		{
			if (firstDiff < lastDiff)
				++firstDiff;
		}

		// Check that last selected line is last diff's last line or below it
		VERIFY(m_pDocument->m_diffList.GetDiff(lastDiff, di));
		if ((int)di.dend0 > lastLine)
		{
			if (firstDiff < lastDiff)
				--lastDiff;
		}

		// Special case: one-line diff is not selected if cursor is in it
		if (firstLine == lastLine)
		{
			firstDiff = -1;
			lastDiff = -1;
		}
	}
}

int CMergeEditView::GetAdditionalTextBlocks (int nLineIndex, TEXTBLOCK *pBuf)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_DIFF) != LF_DIFF)
		return 0;

	if (!COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT))
		return 0;

	int nLineLength = GetLineLength(nLineIndex);
	vector<wdiff> worddiffs;
	m_pDocument->GetWordDiffArray(nLineIndex, &worddiffs);

	if (nLineLength == 0 || worddiffs.size() == 0 || // Both sides are empty
		IsSide0Empty(worddiffs, nLineLength) || IsSide1Empty(worddiffs, nLineLength))
	{
		return 0;
	}

	bool lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);
	int nWordDiffs = worddiffs.size();

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
 */
void CMergeEditView::GetLineColors(int nLineIndex, COLORREF & crBkgnd,
                                COLORREF & crText, BOOL & bDrawWhitespace)
{
	DWORD ignoreFlags = 0;
	GetLineColors2(nLineIndex, ignoreFlags, crBkgnd, crText, bDrawWhitespace);
}

/**
 * @brief Determine text and background color for line
 * @param [in] nLineIndex Index of line in view (NOT line in file)
 * @param [in] ignoreFlags Flags that caller wishes ignored
 * @param [out] crBkgnd Backround color for line
 * @param [out] crText Text color for line
 *
 * This version allows caller to suppress particular flags
 */
void CMergeEditView::GetLineColors2(int nLineIndex, DWORD ignoreFlags, COLORREF & crBkgnd,
                                COLORREF & crText, BOOL & bDrawWhitespace)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);

	if (dwLineFlags & ignoreFlags)
		dwLineFlags &= (~ignoreFlags);

	// Line inside diff
	if (dwLineFlags & LF_WINMERGE_FLAGS)
	{
		crText = m_cachedColors.clrDiffText;
		bDrawWhitespace = true;
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
			crBkgnd = GetColor (COLORINDEX_BKGND);
			crText = GetColor (COLORINDEX_NORMALTEXT);
			bDrawWhitespace = false;
		}
		else
			// Syntax highlighting, get colors from CrystalEditor
			CCrystalEditViewEx::GetLineColors(nLineIndex, crBkgnd,
				crText, bDrawWhitespace);
	}
}

void CMergeEditView::DrawScreenLine(
	HSurface *pdc, POINT &ptOrigin, const RECT &rcClip,
	TEXTBLOCK *pBuf, int nBlocks, int &nActualItem,
	COLORREF crText, COLORREF crBkgnd, BOOL bDrawWhitespace,
	LPCTSTR pszChars, int nOffset, int nCount, int &nActualOffset, int nLineIndex)
{
	const int nLineHeight = GetLineHeight();
	RECT frect = rcClip;
	frect.top = ptOrigin.y;
	frect.bottom = frect.top + nLineHeight;

	CCrystalTextView::DrawScreenLine(pdc, ptOrigin, rcClip, pBuf, nBlocks,
		nActualItem, crText, crBkgnd, bDrawWhitespace, pszChars, nOffset,
		nCount, nActualOffset, nLineIndex);

	if (nLineIndex != -1)
	{
		// If this is a placeholder for a sequence of excluded lines,
		// indicate its length by drawing a label in italic letters.
		const LineInfo &li = m_pTextBuffer->GetLineInfo(nLineIndex);
		if ((li.m_dwFlags & LF_GHOST) && (li.m_nSkippedLines != 0))
		{
			pdc->SelectObject(GetFont(TRUE));
			pdc->DrawText(
				FormatAmount<IDS_LINE_EXCLUDED, IDS_LINES_EXCLUDED>(li.m_nSkippedLines),
				&frect,
				DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
		}
	}
}

/**
 * @brief Sync other pane position
 */
void CMergeEditView::UpdateSiblingScrollPos(BOOL bHorz)
{
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	// limit the TopLine : must be smaller than GetLineCount for all the panels
	int newTopSubLine = m_nTopSubLine;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		if (pChild == m_pWnd)
			continue;
		CMergeEditView *pSiblingView = static_cast<CMergeEditView *>(FromHandle(pChild));
		if (pSiblingView->GetSubLineCount() <= newTopSubLine)
			newTopSubLine = pSiblingView->GetSubLineCount()-1;
	}
	if (m_nTopSubLine != newTopSubLine)
		ScrollToSubLine(newTopSubLine);
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		if (pChild == m_pWnd) // We don't need to update ourselves
			continue;
		CMergeEditView *pSiblingView = static_cast<CMergeEditView *>(FromHandle(pChild));
		pSiblingView->OnUpdateSibling(this, bHorz);
	}
}

/**
 * @brief Update other panes
 */
void CMergeEditView::OnUpdateSibling(CCrystalTextView * pUpdateSource, BOOL bHorz)
{
	if (pUpdateSource != this)
	{
		ASSERT (pUpdateSource != NULL);
		CMergeEditView *pSrcView = static_cast<CMergeEditView*>(pUpdateSource);
		if (!bHorz)  // changed this so bHorz works right
		{
			ASSERT (pSrcView->m_nTopSubLine >= 0);

			// This ASSERT is wrong: panes have different files and
			// different linecounts
			// ASSERT (pSrcView->m_nTopLine < GetLineCount ());
			if (pSrcView->m_nTopSubLine != m_nTopSubLine)
			{
				ScrollToSubLine (pSrcView->m_nTopSubLine, true, false);
				UpdateCaret ();
				RecalcVertScrollBar(true);
			}
		}
		else
		{
			ASSERT (pSrcView->m_nOffsetChar >= 0);

			// This ASSERT is wrong: panes have different files and
			// different linelengths
			// ASSERT (pSrcView->m_nOffsetChar < GetMaxLineLength ());
			if (pSrcView->m_nOffsetChar != m_nOffsetChar)
			{
				ScrollToChar (pSrcView->m_nOffsetChar, true, false);
				UpdateCaret ();
				RecalcHorzScrollBar(true);
			}
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
void CMergeEditView::SelectDiff(int nDiff, bool bScroll /*=true*/, bool bSelectText /*=true*/)
{
	// Check that nDiff is valid
	if (nDiff < 0)
		TRACE("Diffnumber negative (%d)", nDiff);
	if (nDiff >= m_pDocument->m_diffList.GetSize())
		TRACE("Selected diff > diffcount (%d >= %d)",
			nDiff, m_pDocument->m_diffList.GetSize());

	SelectNone();
	m_pDocument->SetCurrentDiff(nDiff);
	ShowDiff(bScroll, bSelectText);
	m_pDocument->UpdateAllViews(this);

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
	// If no diffs, nothing to select
	if (!m_pDocument->m_diffList.HasSignificantDiffs())
		return;
	// GetCurrentDiff() returns -1 if no diff selected
	int nDiff = m_pDocument->GetCurrentDiff();
	if (nDiff == -1)
	{
		// If cursor is inside diff, select that diff
		POINT pos = GetCursorPos();
		nDiff = m_pDocument->m_diffList.LineToDiff(pos.y);
	}
	if (nDiff != -1 && m_pDocument->m_diffList.IsDiffSignificant(nDiff))
		SelectDiff(nDiff, true, false);
}

/**
 * @brief Copy selected text to clipboard
 */
void CMergeEditView::OnEditCopy()
{
	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	// Nothing selected
	if (ptSelStart == ptSelEnd)
		return;
	String text;
	LocateTextBuffer()->GetTextWithoutEmptys(
		ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, text);
	PutToClipboard(text.c_str(), text.length());
}

/**
 * @brief Cut current selection to clipboard
 */
void CMergeEditView::OnEditCut()
{
	if (!QueryEditable())
		return;

	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	// Nothing selected
	if (ptSelStart == ptSelEnd)
		return;
	String text;
	LocateTextBuffer()->GetTextWithoutEmptys(
		ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, text);
	PutToClipboard(text.c_str(), text.length());

	POINT ptCursorPos = ptSelStart;
	ASSERT_VALIDTEXTPOS(ptCursorPos);
	SetAnchor(ptCursorPos);
	SetSelection(ptCursorPos, ptCursorPos);
	SetCursorPos(ptCursorPos);
	EnsureVisible(ptCursorPos);

	LocateTextBuffer()->DeleteText(this,
		ptSelStart.y, ptSelStart.x, ptSelEnd.y, ptSelEnd.x, CE_ACTION_CUT);

	m_pTextBuffer->SetModified(true);
}

/**
 * @brief Paste text from clipboard
 */
void CMergeEditView::OnEditPaste()
{
	if (!QueryEditable())
		return;

	CCrystalEditViewEx::Paste();
	m_pTextBuffer->SetModified(true);
}

/**
 * @brief Go to first diff
 *
 * Called when user selects "First Difference"
 * @sa CMergeEditView::SelectDiff()
 */
void CMergeEditView::OnFirstdiff()
{
	if (m_pDocument->m_diffList.HasSignificantDiffs())
	{
		int nDiff = m_pDocument->m_diffList.FirstSignificantDiff();
		SelectDiff(nDiff, true, false);
	}
}

/**
 * @brief Go to last diff
 */
void CMergeEditView::OnLastdiff()
{
	if (m_pDocument->m_diffList.HasSignificantDiffs())
	{
		int nDiff = m_pDocument->m_diffList.LastSignificantDiff();
		SelectDiff(nDiff, true, false);
	}
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
	int cnt = m_pDocument->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// Returns -1 if no diff selected
	int curDiff = m_pDocument->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int nextDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select next from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the first line of the diff
			++line;
			if (!IsValidTextPosY(line))
				line = m_nTopLine;
			nextDiff = m_pDocument->m_diffList.NextSignificantDiffFromLine(line);
		}
		else
		{
			// Find out if there is a following significant diff
			if (curDiff < m_pDocument->m_diffList.GetSize() - 1)
			{
				nextDiff = m_pDocument->m_diffList.NextSignificantDiff(curDiff);
			}
		}
		if (nextDiff == -1)
			nextDiff = curDiff;

		// nextDiff is the next one if there is one, else it is the one we're on
		SelectDiff(nextDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(line))
			line = m_nTopLine;
		curDiff = m_pDocument->m_diffList.NextSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
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
	int cnt = m_pDocument->m_ptBuf[0]->GetLineCount();
	if (cnt <= 0)
		return;

	// GetCurrentDiff() returns -1 if no diff selected
	int curDiff = m_pDocument->GetCurrentDiff();
	if (curDiff != -1)
	{
		// We're on a diff
		int prevDiff = curDiff;
		if (!IsDiffVisible(curDiff))
		{
			// Selected difference not visible, select previous from cursor
			int line = GetCursorPos().y;
			// Make sure we aren't in the last line of the diff
			--line;
			if (!IsValidTextPosY(line))
				line = m_nTopLine;
			prevDiff = m_pDocument->m_diffList.PrevSignificantDiffFromLine(line);
		}
		else
		{
			// Find out if there is a preceding significant diff
			if (curDiff > 0)
			{
				prevDiff = m_pDocument->m_diffList.PrevSignificantDiff(curDiff);
			}
		}
		if (prevDiff == -1)
			prevDiff = curDiff;

		// prevDiff is the preceding one if there is one, else it is the one we're on
		SelectDiff(prevDiff, true, false);
	}
	else
	{
		// We don't have a selected difference,
		// but cursor can be inside inactive diff
		int line = GetCursorPos().y;
		if (!IsValidTextPosY(line))
			line = m_nTopLine;
		curDiff = m_pDocument->m_diffList.PrevSignificantDiffFromLine(line);
		if (curDiff >= 0)
			SelectDiff(curDiff, true, false);
	}
}

/**
 * @brief Clear selection
 */
void CMergeEditView::SelectNone()
{
	SetSelection (GetCursorPos(), GetCursorPos());
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
	int nLineCount = LocateTextBuffer()->GetLineCount();
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
	POINT pos = GetCursorPos();
	int diff = m_pDocument->m_diffList.LineToDiff(pos.y);
	if (diff != -1 && m_pDocument->m_diffList.IsDiffSignificant(diff))
		SelectDiff(diff, false, false);
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
		POINT pos = GetCursorPos();
		if (!IsLineInCurrentDiff(pos.y))
		{
			m_pDocument->SetCurrentDiff(-1);
			m_pDocument->UpdateAllViews(NULL);
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
			if (!SetTimer(IDT_RESCAN, RESCAN_TIMEOUT, NULL))
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
void CMergeEditView::ShowDiff(bool bScroll, bool bSelectText)
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
		DIFFRANGE curDiff;
		m_pDocument->m_diffList.GetDiff(nDiff, curDiff);

		ptStart.x = 0;
		ptStart.y = curDiff.dbegin0;
		ptEnd.x = 0;
		ptEnd.y = curDiff.dend0;

		if (bScroll)
		{
			if (!IsDiffVisible(curDiff, CONTEXT_LINES_BELOW))
			{
				// Difference is not visible, scroll it so that max amount of
				// scrolling is done while keeping the diff in screen. So if
				// scrolling is downwards, scroll the diff to as up in screen
				// as possible. This usually brings next diff to the screen
				// and we don't need to scroll into it.
				int nLine = GetSubLineIndex(ptStart.y);
				if (nLine > CONTEXT_LINES_ABOVE)
				{
					nLine -= CONTEXT_LINES_ABOVE;
				}
				pCurrentView->ScrollToSubLine(nLine);
				pOtherView->ScrollToSubLine(nLine);
			}
			pCurrentView->SetCursorPos(ptStart);
			pOtherView->SetCursorPos(ptStart);
			pCurrentView->SetAnchor(ptStart);
			pOtherView->SetAnchor(ptStart);
			pCurrentView->SetSelection(ptStart, ptStart);
			pOtherView->SetSelection(ptStart, ptStart);
		}

		if (bSelectText)
		{
			ptEnd.x = GetLineLength(ptEnd.y);
			SetSelection(ptStart, ptEnd);
			UpdateCaret();
		}
		else
			Invalidate();
	}
}


void CMergeEditView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_RESCAN)
	{
		KillTimer(IDT_RESCAN);
		m_pDocument->RescanIfNeeded(RESCAN_TIMEOUT/1000);
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

	if (!m_bCursorHidden && !bShowHide)
		m_bMergeUndo = false;

	POINT cursorPos = GetCursorPos();
	int nScreenLine = cursorPos.y;
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
		curChar = cursorPos.x + 1;
		chars = GetLineLength(nScreenLine);
		column = CalculateActualOffset(nScreenLine, cursorPos.x, true) + 1;
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
	SetLineInfoStatus(sLine, column, columns,
		curChar, chars, sEol, editMode);

	m_pDocument->m_wndLocationView.UpdateVisiblePos(
		m_nTopSubLine, m_nTopSubLine + GetScreenLines());

	// Update Command UI
	m_pDocument->UpdateClipboardCmdUI();
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
	pSub->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, theApp.m_pMainWnd->m_hWnd);
	pMenu->DestroyMenu();
}

/**
 * @brief Change EOL mode and unify all the lines EOL to this new mode
 */
void CMergeEditView::OnConvertEolTo(UINT nID )
{
	CRLFSTYLE nStyle = CRLF_STYLE_AUTOMATIC;;
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
	SetInsertTabs(COptionsMgr::Get(OPT_TAB_TYPE) == 0);
	SetSelectionMargin(COptionsMgr::Get(OPT_VIEW_FILEMARGIN));

	if (!COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT))
		SetTextType(CCrystalTextView::SRC_PLAIN);

	// SetTextType will revert to language dependent defaults for tab
	SetTabSize(COptionsMgr::Get(OPT_TAB_SIZE));
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
}

/** 
 * @brief Goto given line.
 * @param [in] nLine Destination linenumber
 * @param [in] bRealLine if TRUE linenumber is real line, otherwise
 * it is apparent line (including deleted lines)
 * @param [in] pane Pane index of goto target pane (0 = left, 1 = right).
 */
void CMergeEditView::GotoLine(UINT nLine, bool bRealLine, int pane)
{
	CMergeEditView *pLeftView = m_pDocument->GetLeftView();
	CMergeEditView *pRightView = m_pDocument->GetRightView();
	CMergeEditView *pCurrentView = m_pDocument->GetActiveMergeView();
	bool bLeftIsCurrent = pLeftView == pCurrentView;
	int nRealLine = nLine;
	int nApparentLine = nLine;

	// Compute apparent (shown linenumber) line
	if (bRealLine)
	{
		if (nRealLine > m_pDocument->m_ptBuf[pane]->GetLineCount() - 1)
			nRealLine = m_pDocument->m_ptBuf[pane]->GetLineCount() - 1;
		nApparentLine = m_pDocument->m_ptBuf[pane]->ComputeApparentLine(nRealLine);
	}

	POINT ptPos = { 0, nApparentLine };
	// Scroll line to center of view
	int nScrollLine = GetSubLineIndex(nApparentLine);
	nScrollLine -= GetScreenLines() / 2;
	if (nScrollLine < 0)
		nScrollLine = 0;
	
	pLeftView->ScrollToSubLine(nScrollLine);
	pRightView->ScrollToSubLine(nScrollLine);
	pLeftView->SetCursorPos(ptPos);
	pRightView->SetCursorPos(ptPos);
	pLeftView->SetAnchor(ptPos);
	pRightView->SetAnchor(ptPos);

	// If goto target is another view - activate another view.
	// This is done for user convenience as user probably wants to
	// work with goto target file.
	//if ((bLeftIsCurrent && pane == 1) || (!bLeftIsCurrent && pane == 0))
	//	pSplitterWnd->ActivateNext();
}

/**
 * @brief Copy selected lines adding linenumbers.
 */
void CMergeEditView::OnEditCopyLineNumbers()
{
	POINT ptStart;
	POINT ptEnd;
	String strText;

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
		_sntprintf(strNum, _countof(strNum), _T("%*d: "), nNumWidth, line + 1);
		strText += strNum;
		if (LPCTSTR pszLine = GetLineChars(i))
			strText += pszLine;
	}
	PutToClipboard(strText.c_str(), strText.size());
}

void CMergeEditView::OnSize() 
{
	InvalidateScreenRect();
	Invalidate();

	// recalculate m_nTopSubLine
	m_nTopSubLine = GetSubLineIndex(m_nTopLine);

	UpdateCaret();
	
	RecalcVertScrollBar();
	RecalcHorzScrollBar();

	UpdateSiblingScrollPos(FALSE);

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

void CMergeEditView::RecalcVertScrollBar(BOOL bPositionOnly)
{
	CGhostTextView::RecalcVertScrollBar(bPositionOnly);
	if (!bPositionOnly)
		m_pDocument->m_wndLocationView.ForceRecalculate();
}

/**
 * @brief returns the number of empty lines which are added for synchronizing the line in two panes.
 */
int CMergeEditView::GetEmptySubLines( int nLineIndex )
{
	int	nBreaks[2] = { 0, 0 };
	if (CMergeEditView *pLeftView = m_pDocument->GetLeftView())
	{
		if (nLineIndex >= pLeftView->GetLineCount())
			return 0;
		pLeftView->WrapLineCached( nLineIndex, pLeftView->GetScreenChars(), NULL, nBreaks[0] );
	}
	if (CMergeEditView *pRightView = m_pDocument->GetRightView())
	{
		if (nLineIndex >= pRightView->GetLineCount())
			return 0;
		pRightView->WrapLineCached( nLineIndex, pRightView->GetScreenChars(), NULL, nBreaks[1] );
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
	DIFFRANGE diff;
	m_pDocument->m_diffList.GetDiff(nDiff, diff);
	return IsDiffVisible(diff);
}

/**
 * @brief Determine if difference is visible on screen.
 * @param [in] diff diff to check.
 * @param [in] nLinesBelow Allow "minimizing" the number of visible lines.
 * @return TRUE if difference is visible, FALSE otherwise.
 */
bool CMergeEditView::IsDiffVisible(const DIFFRANGE& diff, int nLinesBelow /*=0*/)
{
	const int nDiffStart = GetSubLineIndex(diff.dbegin0);
	const int nDiffEnd = GetSubLineIndex(diff.dend0);
	// Diff's height is last line - first line + last line's line count
	const int nDiffHeight = nDiffEnd - nDiffStart + GetSubLines(diff.dend0) + 1;

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
}

/**
* @brief Change the editor's syntax highlighting scheme.
* @param [in] nID Selected color scheme sub menu id.
*/
void CMergeEditView::OnChangeScheme(UINT nID)
{
	for (int nPane = 0; nPane < 2; nPane++) 
	{
		if (CMergeEditView *const pView = m_pDocument->GetView(nPane))
		{
			pView->SetTextType(CCrystalTextView::TextType(nID - ID_COLORSCHEME_FIRST));
		}
	}
	m_pDocument->UpdateAllViews(NULL);
}

/**
 * @brief Called when mouse's wheel is scrolled.
 */
BOOL CMergeEditView::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	POINTSTOPOINT(pt, lParam);

	WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
	short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

	if (fwKeys == MK_CONTROL)
	{
		ZoomText(zDelta < 0 ? -1 : 1);
		return TRUE;
	}
	if (fwKeys == MK_SHIFT)
	{
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

		VERIFY(GetScrollInfo(SB_HORZ, &si));
		// new horz pos
		si.nPos -= zDelta / 40;
		if (si.nPos > si.nMax)
			si.nPos = si.nMax;
		if (si.nPos < si.nMin)
			si.nPos = si.nMin;

		SetScrollInfo(SB_HORZ, &si);
		// for update
		SendMessage(WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nPos) , NULL );
		return TRUE;
	}
	return FALSE;
}

/**
 * @brief Change font size (zoom) in views.
 * @param [in] amount Amount of change/zoom, negative number makes
 *  font smaller, positive number bigger and 0 reset the font size.
 */
void CMergeEditView::ZoomText(short amount)
{
	if (HSurface *pDC = GetDC())
	{
		LOGFONT lf;
		GetFont(lf);

		const int nLogPixelsY = pDC->GetDeviceCaps(LOGPIXELSY);

		int nPointSize = -MulDiv(lf.lfHeight, 72, nLogPixelsY);

		if ( amount == 0)
		{
			nPointSize = -MulDiv(theApp.m_pMainWnd->m_lfDiff.lfHeight, 72, nLogPixelsY);
		}

		nPointSize += amount;
		if (nPointSize < 2)
			nPointSize = 2;

		lf.lfHeight = -MulDiv(nPointSize, nLogPixelsY, 72);

		for (int nPane = 0; nPane < MERGE_VIEW_COUNT; nPane++) 
		{
			if (CMergeEditView *const pView = m_pDocument->GetView(nPane))
			{
				pView->SetFont(lf);
			}
		}
		ReleaseDC(pDC);
	}
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
	CDiffTextBuffer *const pTextBuffer = LocateTextBuffer();
	StreamLineReader reader = pstm;
	stl::string line;
	OString text = NULL;
	unsigned pos1, len1, pos2, len2;
	TRACE("#BOF#\n");
	while (reader.readLine(line))
	{
		if (line.find_first_not_of('-') == 3)
		{
			stl::replace(line.begin(), line.end(), '\t', ' ');
			text.Free();
			text.Append(HString::Oct(line.c_str())->Uni(CP_UTF8)->Trim());
		}
		else if (line.find_first_not_of('+') == 3)
		{
			stl::replace(line.begin(), line.end(), '\t', ' ');
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
			stl::vector<CMyComBSTR> vec1(len1);
			stl::vector<CMyComBSTR> vec2(len2);
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
				size_t i;
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
		if (int id = pMenu->TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, m_hWnd))
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
		POINT ptClient = { pt.x, pt.y };
		ScreenToClient(&ptClient);
		ShowDropIndicator(ptClient);
		*pdwEffect = DROPEFFECT_COPY;
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
