/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  GhostTextView.cpp
 *
 * @brief Implementation of GhostTextView class.
 */
#include "stdafx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "LanguageSelect.h"
#include "MergeEditView.h"
#include "MergeDiffDetailView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/**
 * @brief Constructor, initializes members.
 */
CGhostTextView::CGhostTextView(HWindow *pWnd, CChildFrame *pDocument, int nThisPane, size_t ZeroInit)
: CCrystalEditView(pWnd, ZeroInit)
, m_pDocument(pDocument)
, m_nThisPane(nThisPane)
{
}

void CGhostTextView::popPosition(SCursorPushed const &src, POINT &pt) const
{
	pt.x = src.x;
	pt.y = static_cast<CGhostTextBuffer *>
		(m_pTextBuffer)->ComputeApparentLine(src.y, src.nToFirstReal);
	// if the cursor was in a trailing ghost line, and this disappeared,
	// got at the end of the last line
	int const yLimit = GetLineCount() - 1;
	if (pt.y > yLimit)
	{
		pt.y = yLimit;
		pt.x = GetLineLength(pt.y);
	}
}

void CGhostTextView::pushPosition(SCursorPushed &dst, POINT &pt) const
{
	int const yLimit = GetLineCount() - 1;
	if (pt.y > yLimit)
		pt.y = yLimit;
	if (pt.y >= 0)
	{
		int const xLimit = GetLineLength(pt.y);
		if (pt.x > xLimit)
			pt.x = xLimit;
	}
	dst.x = pt.x;
	dst.y = static_cast<CGhostTextBuffer *>
		(m_pTextBuffer)->ComputeRealLineAndGhostAdjustment(pt.y, dst.nToFirstReal);
}

void CGhostTextView::PopCursors()
{
	POINT ptCursorLast = m_ptCursorLast;
	popPosition(m_ptCursorPosPushed, ptCursorLast);

	ASSERT_VALIDTEXTPOS(ptCursorLast);
	SetCursorPos(ptCursorLast);

	popPosition(m_ptSelStartPushed, m_ptSelStart);
	ASSERT_VALIDTEXTPOS(m_ptSelStart);
	popPosition(m_ptSelEndPushed, m_ptSelEnd);
	ASSERT_VALIDTEXTPOS(m_ptSelEnd);
	popPosition(m_ptAnchorPushed, m_ptAnchor);
	ASSERT_VALIDTEXTPOS(m_ptAnchor);
	// laoran 2003/09/03
	// here is what we did before, maybe we have to do it, but test with pushed positions
	// SetSelection (ptCursorLast, ptCursorLast);
	// SetAnchor (ptCursorLast);

	if (m_bDraggingText)
	{
		popPosition(m_ptDraggedTextBeginPushed, m_ptDraggedTextBegin);
		ASSERT_VALIDTEXTPOS(m_ptDraggedTextBegin);
		popPosition(m_ptDraggedTextEndPushed, m_ptDraggedTextEnd);
		ASSERT_VALIDTEXTPOS(m_ptDraggedTextEnd);
	}
	if (m_bDropPosVisible)
	{
		popPosition(m_ptSavedCaretPosPushed, m_ptSavedCaretPos);
		ASSERT_VALIDTEXTPOS(m_ptSavedCaretPos);
	}
	if (m_bSelectionPushed)
	{
		popPosition(m_ptSavedSelStartPushed, m_ptSavedSelStart);
		ASSERT_VALIDTEXTPOS(m_ptSavedSelStart);
		popPosition(m_ptSavedSelEndPushed, m_ptSavedSelEnd);
		ASSERT_VALIDTEXTPOS(m_ptSavedSelEnd);
	}

	POINT ptLastChange;
	if (m_ptLastChangePushed.y == 0 && m_ptLastChangePushed.nToFirstReal > 0)
	{
		ptLastChange.x = -1;
		ptLastChange.y = -1;
	}
	else
	{
		popPosition(m_ptLastChangePushed, ptLastChange);
		ASSERT_VALIDTEXTPOS(ptLastChange);
	}
	static_cast<CGhostTextBuffer *>
		(m_pTextBuffer)->RestoreLastChangePos(ptLastChange);

	// restore the scrolling position
	m_nTopSubLine = m_nTopSubLinePushed;
	int nSubLineCount = GetSubLineCount();
	if (m_nTopSubLine >= nSubLineCount)
		m_nTopSubLine = nSubLineCount - 1;
	GetLineBySubLine(m_nTopSubLine, m_nTopLine);
	RecalcVertScrollBar(true);
	RecalcHorzScrollBar(true);
}

void CGhostTextView::PushCursors()
{
	pushPosition(m_ptCursorPosPushed, m_ptCursorPos);
	pushPosition(m_ptSelStartPushed, m_ptSelStart);
	pushPosition(m_ptSelEndPushed, m_ptSelEnd);
	pushPosition(m_ptAnchorPushed, m_ptAnchor);
	if (m_bDraggingText)
	{
		pushPosition(m_ptDraggedTextBeginPushed, m_ptDraggedTextBegin);
		pushPosition(m_ptDraggedTextEndPushed, m_ptDraggedTextEnd);
	}
	if (m_bDropPosVisible)
	{
		pushPosition(m_ptSavedCaretPosPushed, m_ptSavedCaretPos);
	}
	if (m_bSelectionPushed)
	{
		pushPosition(m_ptSavedSelStartPushed, m_ptSavedSelStart);
		pushPosition(m_ptSavedSelEndPushed, m_ptSavedSelEnd);
	}

	pushPosition(m_ptLastChangePushed, m_pTextBuffer->GetLastChangePos());

	// and top line positions
	m_nTopSubLinePushed = m_nTopSubLine;
}

int CGhostTextView::ComputeRealLine(int nApparentLine) const
{
	if (!m_pTextBuffer)
		return 0;
	return static_cast<CGhostTextBuffer *>
		(m_pTextBuffer)->ComputeRealLine(nApparentLine);
}

int CGhostTextView::ComputeApparentLine(int nRealLine) const
{
	return static_cast<CGhostTextBuffer *>
		(m_pTextBuffer)->ComputeApparentLine(nRealLine);
}

COLORREF CGhostTextView::GetColor(int nColorIndex)
{
	switch (nColorIndex & ~COLORINDEX_APPLYFORCE)
	{
	case COLORINDEX_HIGHLIGHTBKGND1:
		return COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF);
	case COLORINDEX_HIGHLIGHTTEXT1:
		return COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF_TEXT);
	case COLORINDEX_HIGHLIGHTBKGND2:
		return COptionsMgr::Get(OPT_CLR_WORDDIFF);
	case COLORINDEX_HIGHLIGHTTEXT2:
		return COptionsMgr::Get(OPT_CLR_WORDDIFF_TEXT);
	case COLORINDEX_HIGHLIGHTBKGND3:
		return COptionsMgr::Get(OPT_CLR_WORDDIFF_DELETED);
	case COLORINDEX_HIGHLIGHTBKGND4:
		return COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF_DELETED);
	default:
		return CCrystalTextView::GetColor(nColorIndex);
	}
}

void CGhostTextView::GetAdditionalTextBlocks(int nLineIndex, TextBlock::Array &rpBuf)
{
	DWORD const dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_DIFF) != LF_DIFF)
		return;

	if (!COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT))
		return;

	int const nLineLength = GetLineLength(nLineIndex);
	std::vector<wdiff> worddiffs;
	m_pDocument->GetWordDiffArray(nLineIndex, worddiffs);

	int const nWordDiffs = static_cast<int>(worddiffs.size());
	if (nWordDiffs == 0)
		return;

	bool const lineInCurrentDiff = IsLineInCurrentDiff(nLineIndex);

	TextBlock *pBuf = new TextBlock[nWordDiffs * 2 + 1];
	TextBlock::Array(pBuf).swap(rpBuf);
	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NONE;
	pBuf[0].m_nBgColorIndex = COLORINDEX_NONE;
	for (int i = 0; i < nWordDiffs; i++)
	{
		wdiff const &wd = worddiffs[i];
		++pBuf;
		pBuf->m_nCharPos = wd.start[m_nThisPane];
		if (lineInCurrentDiff)
		{
			if (COptionsMgr::Get(OPT_CLR_SELECTED_WORDDIFF_TEXT) != CLR_NONE)
				pBuf->m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT1 | COLORINDEX_APPLYFORCE;
			else
				pBuf->m_nColorIndex = COLORINDEX_NONE;
			if (wd.IsInsert()) // Are char/words inserted/deleted on one side?
				pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND4 | COLORINDEX_APPLYFORCE;
			else
				pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND1 | COLORINDEX_APPLYFORCE;
		}
		else
		{
			if (COptionsMgr::Get(OPT_CLR_WORDDIFF_TEXT) != CLR_NONE)
				pBuf->m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT2 | COLORINDEX_APPLYFORCE;
			else
				pBuf->m_nColorIndex = COLORINDEX_NONE;
			if (wd.IsInsert()) // Are char/words inserted/deleted on one side?
				pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND3 | COLORINDEX_APPLYFORCE;
			else
				pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND2 | COLORINDEX_APPLYFORCE;
		}
		++pBuf;
		pBuf->m_nCharPos = wd.end[m_nThisPane];
		pBuf->m_nColorIndex = COLORINDEX_NONE;
		pBuf->m_nBgColorIndex = COLORINDEX_NONE;
	}
	rpBuf.m_nBack = nWordDiffs * 2;
}

/**
 * @brief Draw selection margin.
 * @param [in] pdc         Pointer to draw context.
 * @param [in] rect        The rectangle to draw.
 * @param [in] nLineIndex  Index of line in view.
 * @param [in] nLineNumber Line number to display. if -1, it's not displayed.
 */
void CGhostTextView::DrawMargin(HSurface * pdc, const RECT & rect, int nLineIndex, int nLineNumber)
{
	int nRealLineNumber;
	if (nLineIndex < 0 || GetLineFlags(nLineIndex) & (LF_GHOST | LF_SKIPPED))
		nRealLineNumber = -1;
	else
		nRealLineNumber = ComputeRealLine(nLineIndex) + 1;
	CCrystalTextView::DrawMargin(pdc, rect, nLineIndex, nRealLineNumber);
}

bool CGhostTextView::DrawSingleLine(HSurface *pdc, const RECT &rc, int nLineIndex)
{
	if (CCrystalTextView::DrawSingleLine(pdc, rc, nLineIndex))
		return true;
	LineInfo const &li = m_pTextBuffer->GetLineInfo(nLineIndex);
	if (li.m_dwFlags & LF_GHOST)
	{
		// If this is a placeholder for a sequence of excluded lines,
		// indicate its length by drawing a label in italic letters.
		if (li.m_nSkippedLines != 0)
		{
			pdc->SelectObject(GetFont(COLORINDEX_LAST));
			pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
			pdc->DrawText(
				FormatAmount<IDS_LINE_EXCLUDED, IDS_LINES_EXCLUDED>(li.m_nSkippedLines),
				const_cast<RECT *>(&rc),
				DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
			return true;
		}
		if (m_pHatchBrush)
		{
			int const nCharWidth = GetCharWidth();
			int const nLineHeight = GetLineHeight();
			pdc->SetBrushOrgEx(
				-(m_nOffsetChar * nCharWidth),
				-(m_nTopSubLine * nLineHeight), NULL);
			pdc->FillRect(&rc, m_pHatchBrush);
			return true;
		}
	}
	return false;
}

/**
 * @brief Change font size (zoom) in views.
 * @param [in] amount Amount of change/zoom, negative number makes
 *  font smaller, positive number bigger and 0 reset the font size.
 */
void CGhostTextView::ZoomText(short amount)
{
	if (HSurface *pDC = GetDC())
	{
		const int nLogPixelsY = pDC->GetDeviceCaps(LOGPIXELSY);

		LOGFONT lf;
		GetFont(lf);

		int nPointSize = -MulDiv(lf.lfHeight, 72, nLogPixelsY);

		if (amount == 0)
		{
			nPointSize = -MulDiv(theApp.m_pMainWnd->m_lfDiff.lfHeight, 72, nLogPixelsY);
		}

		nPointSize += amount;
		if (nPointSize < 2)
			nPointSize = 2;

		lf.lfHeight = -MulDiv(nPointSize, nLogPixelsY, 72);

		int const nHatchStyle = COptionsMgr::Get(OPT_CROSS_HATCH_DELETED_LINES);

		for (int nPane = 0; nPane < MERGE_VIEW_COUNT; nPane++)
		{
			if (CCrystalTextView *const pView = m_pDocument->GetView(nPane))
			{
				pView->SetFont(lf, nHatchStyle);
			}
			if (CCrystalTextView *const pView = m_pDocument->GetDetailView(nPane))
			{
				pView->SetFont(lf, nHatchStyle);
			}
		}

		EnsureCursorVisible();

		ReleaseDC(pDC);
	}
}

/**
 * @brief Called when mouse's wheel is scrolled.
 */
BOOL CGhostTextView::OnMouseWheel(WPARAM wParam, LPARAM lParam)
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
		LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
		HWindow *pParent = GetParent();
		HWindow *pChild = NULL;
		while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
		{
			CGhostTextView *pSiblingView = static_cast<CGhostTextView *>(FromHandle(pChild));
			if (pSiblingView->GetStyle() & WS_HSCROLL)
			{
				SCROLLINFO si;
				si.cbSize = sizeof si;
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				VERIFY(pSiblingView->GetScrollInfo(SB_HORZ, &si));
				--si.nPage;
				si.nMax -= si.nPage;
				// new horz pos
				si.nPos -= zDelta / 40;
				if (si.nPos > si.nMax)
					si.nPos = si.nMax;
				if (si.nPos < si.nMin)
					si.nPos = si.nMin;
				pSiblingView->ScrollToChar(si.nPos);
				pSiblingView->UpdateSiblingScrollPos();
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}
