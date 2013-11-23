/** 
 * @file  MergeDiffDetailView.cpp
 *
 * @brief Implementation file for CMergeDiffDetailView
 *
 */
#include "StdAfx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "SyntaxColors.h"
#include "LanguageSelect.h"
#include "MergeDiffDetailView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using stl::vector;

/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView

/**
 * @brief Constructor.
 */
CMergeDiffDetailView::CMergeDiffDetailView(CChildFrame *pDocument, int nThisPane)
: CGhostTextView(pDocument, nThisPane, sizeof *this)
{
}

CMergeDiffDetailView::~CMergeDiffDetailView()
{
}

LRESULT CMergeDiffDetailView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		if (OnMouseWheel(wParam, lParam))
			return 0;
		break;
	case WM_CONTEXTMENU:
		OnContextMenu(lParam);
		break;
	}
	return CCrystalTextView::WindowProc(uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView message handlers

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CMergeDiffDetailView::UpdateResources()
{
}

/// virtual, ensure we remain in diff
void CMergeDiffDetailView::SetSelection(const POINT &ptStart, const POINT &ptEnd)
{
	POINT ptStartNew = ptStart;
	EnsureInDiff(ptStartNew);
	POINT ptEndNew = ptEnd;
	EnsureInDiff(ptEndNew);
	EnsureInDiff(m_ptCursorPos);
	CCrystalTextView::SetSelection(ptStartNew, ptEndNew);
}

int CMergeDiffDetailView::GetAdditionalTextBlocks(int nLineIndex, TEXTBLOCK *&rpBuf)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_DIFF) != LF_DIFF)
		return 0; // No diff

	if (!COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT))
		return 0; // No coloring

	int nLineLength = GetLineLength(nLineIndex);
	vector<wdiff> worddiffs;
	m_pDocument->GetWordDiffArray(nLineIndex, worddiffs);

	if (nLineLength == 0 || worddiffs.size() == 0 || // Both sides are empty
		IsSide0Empty(worddiffs, nLineLength) || IsSide1Empty(worddiffs, nLineLength))
	{
		return 0;
	}

	const int nWordDiffs = worddiffs.size();

	TEXTBLOCK *pBuf = new TEXTBLOCK[nWordDiffs * 2 + 1];
	rpBuf = pBuf;
	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NONE;
	pBuf[0].m_nBgColorIndex = COLORINDEX_NONE;
	for (int i = 0; i < nWordDiffs; i++)
	{
		const wdiff &wd = worddiffs[i];
		++pBuf;
		pBuf->m_nCharPos = wd.start[m_nThisPane];
		pBuf->m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT1 | COLORINDEX_APPLYFORCE;
		if (wd.start[0] == wd.end[0] + 1 || wd.start[1] == wd.end[1] + 1)
			// Case on one side char/words are inserted or deleted 
			pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND3 | COLORINDEX_APPLYFORCE;
		else
			pBuf->m_nBgColorIndex = COLORINDEX_HIGHLIGHTBKGND2 | COLORINDEX_APPLYFORCE;
		++pBuf;
		pBuf->m_nCharPos = wd.end[m_nThisPane] + 1;
		pBuf->m_nColorIndex = COLORINDEX_NONE;
		pBuf->m_nBgColorIndex = COLORINDEX_NONE;
	}
	return nWordDiffs * 2 + 1;
}

void CMergeDiffDetailView::DrawSingleLine(HSurface *pdc, const RECT &rc, int nLineIndex)
{
	if (nLineIndex < m_lineBegin || nLineIndex >= m_lineEnd)
	{
		pdc->SetBkColor(GetColor(COLORINDEX_WHITESPACE));
		pdc->ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0);
	}
	else
	{
		CGhostTextView::DrawSingleLine(pdc, rc, nLineIndex);
	}
}

COLORREF CMergeDiffDetailView::GetColor(int nColorIndex)
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
	default:
		return CCrystalTextView::GetColor(nColorIndex);
	}
}

/// virtual, avoid coloring the whole diff with diff color 
void CMergeDiffDetailView::GetLineColors(int nLineIndex, COLORREF &crBkgnd, COLORREF &crText)
{
	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	// Line with WinMerge flag, 
	// Lines with only the LF_DIFF/LF_TRIVIAL flags are not colored with Winmerge colors
	if (dwLineFlags & (LF_WINMERGE_FLAGS & ~LF_DIFF & ~LF_TRIVIAL & ~LF_MOVED))
	{
		crText = COptionsMgr::Get(OPT_CLR_DIFF);
		if (dwLineFlags & LF_GHOST)
		{
			crBkgnd = COptionsMgr::Get(OPT_CLR_DIFF_DELETED);
		}
	}
	else
	{
		// If no syntax hilighting
		if (!COptionsMgr::Get(OPT_SYNTAX_HIGHLIGHT))
		{
			crBkgnd = GetColor(COLORINDEX_BKGND);
			crText = GetColor(COLORINDEX_NORMALTEXT);
		}
		else
		{
			crBkgnd = CLR_NONE;
			crText = CLR_NONE;
		}
	}
	if (nLineIndex < m_lineBegin || nLineIndex >= m_lineEnd)
	{
		crBkgnd = GetColor(COLORINDEX_WHITESPACE);
		crText = GetColor(COLORINDEX_WHITESPACE);
	}
}

void CMergeDiffDetailView::OnDisplayDiff(int nDiff)
{
	int newlineBegin, newlineEnd;
	if (nDiff < 0 || nDiff >= m_pDocument->m_diffList.GetSize())
	{
		newlineBegin = 0;
		newlineEnd = 0;
	}
	else
	{
		const DIFFRANGE *curDiff = m_pDocument->m_diffList.DiffRangeAt(nDiff);
		newlineBegin = curDiff->dbegin0;
		ASSERT(newlineBegin >= 0);
		newlineEnd = curDiff->dend0 + 1;
	}

	if (newlineBegin == m_lineBegin && newlineEnd == m_lineEnd)
		return;

	m_lineBegin = newlineBegin;
	m_lineEnd = newlineEnd;

	// scroll to the first line of the diff
	ScrollToLine(m_lineBegin);
	// update scroll ranges
	RecalcHorzScrollBar();
	RecalcVertScrollBar();
}

/**
 * @brief Adjust the point to remain in the displayed diff
 *
 * @return Tells if the point has been changed
 */
bool CMergeDiffDetailView::EnsureInDiff(POINT &pt)
{
	const POINT cpt = pt;
	// not above diff
	if (pt.y < m_lineBegin)
	{
		pt.y = m_lineBegin;
		pt.x = 0;
	}
	// diff is defined and not below diff
	else if (pt.y >= m_lineEnd)
	{
		pt.y = m_lineEnd;
		pt.x = 0;
	}
	return pt != cpt;
}

/// virtual, ensure we remain in diff
void CMergeDiffDetailView::ScrollToSubLine(int nNewTopLine)
{
	const int nMaxTopLine = m_lineEnd - GetScreenLines();

	if (nNewTopLine > nMaxTopLine)
		nNewTopLine = nMaxTopLine;
	if (nNewTopLine < m_lineBegin)
		nNewTopLine = m_lineBegin;

	m_nTopLine = nNewTopLine;
	
	POINT pt = GetCursorPos();
	if (EnsureInDiff(pt))
		SetCursorPos(pt);
	
	POINT ptSelStart, ptSelEnd;
	GetSelection(ptSelStart, ptSelEnd);
	if (EnsureInDiff(ptSelStart) || EnsureInDiff(ptSelEnd))
		SetSelection(ptSelStart, ptSelEnd);
	
	CCrystalTextView::ScrollToSubLine(nNewTopLine);
}

/*
 * @brief Compute the max length of the lines inside the displayed diff
 */
int CMergeDiffDetailView::GetDiffLineLength()
{
	int nMaxLineLength = 0;
	// we can not use GetLineActualLength below nLineCount
	// diff info (and lineBegin/lineEnd) are updated only during Rescan
	// they may get invalid just after we delete some text
	int validLineEnd = GetLineCount();
	if (validLineEnd > m_lineEnd)
		validLineEnd = m_lineEnd;
	for (int i = m_lineBegin; i < validLineEnd; ++i)
	{
		int nActualLength = GetLineActualLength(i);
		if (nMaxLineLength < nActualLength)
			nMaxLineLength = nActualLength;
	}
	return nMaxLineLength;
}

/**
 * @brief Update the horizontal scrollbar
 *
 * @note The scrollbar width is the one needed for the largest view
 * @sa ccrystaltextview::RecalcHorzScrollBar()
 */
int CMergeDiffDetailView::RecalcHorzScrollBar(bool bPositionOnly)
{
	// Again, we cannot use nPos because it's 16-bit
	SCROLLINFO si;
	si.cbSize = sizeof si;
	// note : this value differs from the value in CCrystalTextView::RecalcHorzScrollBar
	int nMaxLineLen = 0;
	if (CMergeDiffDetailView *pView = m_pDocument->GetRightDetailView())
		nMaxLineLen = pView->GetDiffLineLength();
	if (CMergeDiffDetailView *pView = m_pDocument->GetLeftDetailView())
		nMaxLineLen = max(nMaxLineLen, pView->GetDiffLineLength());
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
	}
	else
	{
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nPage = GetScreenChars();
		si.nMin = 0;
		// Horiz scroll limit to longest line + one screenwidth 
		si.nMax = nMaxLineLen + si.nPage;
	}
	si.nPos = m_nOffsetChar;
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		if (pSiblingView->GetStyle() & WS_HSCROLL)
			pSiblingView->SetScrollInfo(SB_HORZ, &si);
	}
	return si.nPos;
}

/**
 * @brief Update the vertical scrollbar
 *
 * @sa ccrystaltextview::RecalcVertScrollBar()
 */
int CMergeDiffDetailView::RecalcVertScrollBar(bool bPositionOnly)
{
	if (!bPositionOnly)
	{
		// Recompute m_nScreenLines from client area shrinked by 1/2 line height
		RECT rc;
		GetClientRect(&rc);
		int lineHeight = GetLineHeight();
		if (rc.bottom >= lineHeight)
			rc.bottom -= lineHeight / 2;
		m_nScreenLines = rc.bottom / lineHeight;
		if (m_nTopSubLine > m_lineEnd - m_nScreenLines)
			ScrollToSubLine(m_lineEnd - m_nScreenLines);
	}
	SCROLLINFO si;
	si.cbSize = sizeof si;
	si.fMask = bPositionOnly ?
		SIF_POS : SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nPage = GetScreenLines();
	si.nMin = m_lineBegin;
	si.nPos = m_nTopSubLine;
	si.nMax = m_lineEnd - 1;
	LPCTSTR pcwAtom = MAKEINTATOM(GetClassAtom());
	HWindow *pParent = GetParent();
	HWindow *pChild = NULL;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		CCrystalTextView *pSiblingView = static_cast<CCrystalTextView *>(FromHandle(pChild));
		ASSERT(pSiblingView->GetStyle() & WS_VSCROLL);
		pSiblingView->SetScrollInfo(SB_VERT, &si);
	}
	return si.nPos;
}

void CMergeDiffDetailView::PushCursors()
{
	CGhostTextView::PushCursors();
	// push lineBegin and lineEnd
	m_lineBeginPushed = m_lineBegin;
	m_lineEndPushed = m_lineEnd;
}

void CMergeDiffDetailView::PopCursors()
{
	CGhostTextView::PopCursors();
	// pop lineBegin and lineEnd
	m_lineBegin = m_lineBeginPushed;
	m_lineEnd = m_lineEndPushed;
	// clip the involved coordinates to their valid ranges
	int nLineCount = GetLineCount();
	if (m_lineBegin >= nLineCount)
	{
		// even the first line is invalid, stop displaying the diff
		m_lineBegin = m_lineEnd = m_nTopLine = m_nTopSubLine = 0;
	}
	else
	{
		// just check that all positions all valid
		if (m_lineEnd > nLineCount)
			m_lineEnd = nLineCount;
		if (m_ptCursorPos.y >= nLineCount)
			m_ptCursorPos.y = nLineCount - 1;
		int nLineLength = GetLineLength(m_ptCursorPos.y);
		if (m_ptCursorPos.x > nLineLength)
			m_ptCursorPos.x = nLineLength;
	}
}

/**
 * @brief Offer a context menu
 */
void CMergeDiffDetailView::OnContextMenu(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	// Create the menu and populate it with the available functions
	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_MERGEDETAILVIEW);
	HMenu *const pSub = pMenu->GetSubMenu(0);
	// Context menu opened using keyboard has no coordinates
	if (point.x == -1 && point.y == -1)
	{
		point.x = point.y = 5;
		ClientToScreen(&point);
	}
	pSub->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		point.x, point.y, theApp.m_pMainWnd->m_pWnd);
	pMenu->DestroyMenu();
}

void CMergeDiffDetailView::OnUpdateCaret(bool bShowHide)
{
	if (bShowHide)
		return;
	m_pDocument->UpdateGeneralCmdUI();
}

/**
 * @brief Called after document is loaded.
 * This function is called from CChildFrame::OpenDocs() after documents are
 * loaded. So this is good place to set View's options etc.
 */
void CMergeDiffDetailView::DocumentsLoaded()
{
	SetTabSize(COptionsMgr::Get(OPT_TAB_SIZE),
		COptionsMgr::Get(OPT_SEPARATE_COMBINING_CHARS));
	SetViewTabs(COptionsMgr::Get(OPT_VIEW_WHITESPACE));
	bool bMixedEOL = COptionsMgr::Get(OPT_ALLOW_MIXED_EOL) ||
		m_pDocument->IsMixedEOL(m_nThisPane);
	SetViewEols(COptionsMgr::Get(OPT_VIEW_WHITESPACE), bMixedEOL);
	SetWordWrapping(FALSE);
	SetFont(theApp.m_pMainWnd->m_lfDiff);
}
