//////////////////////////////////////////////////////////////////////
/** 
 * @file  MergeDiffDetailView.cpp
 *
 * @brief Implementation file for CMergeDiffDetailView
 *
 */
// ID line follows -- this is updated by SVN
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MergeDiffDetailView.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "SyntaxColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using stl::vector;

static const UINT NROWS_INIT = 4;

/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView

/**
 * @brief Constructor.
 */
CMergeDiffDetailView::CMergeDiffDetailView(CChildFrame *pDocument, int nThisPane)
: CCrystalTextView(sizeof *this)
, m_pDocument(pDocument)
, m_nThisPane(nThisPane)
, m_lineBegin(0)
, m_lineEnd(-1)
, m_diffLength(0)
, m_displayLength(NROWS_INIT)
{
}

CMergeDiffDetailView::~CMergeDiffDetailView()
{
}

LRESULT CMergeDiffDetailView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SIZE:
		SetDisplayHeight(SHORT LOWORD(lParam));
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
 * @brief Get TextBuffer associated with view.
 * @return pointer to textbuffer associated with the view.
 */
CCrystalTextBuffer *CMergeDiffDetailView::LocateTextBuffer()
{
	return m_pDocument->m_ptBuf[m_nThisPane];
}

/**
 * @brief Update any resources necessary after a GUI language change
 */
void CMergeDiffDetailView::UpdateResources()
{
}

/**
 * @brief Set view's height.
 * @param [in] h new view height in lines
 * @todo Calculation seems suspicious...
 */
void CMergeDiffDetailView::SetDisplayHeight(int h) 
{
	m_displayLength = (h + GetLineHeight()/10) / GetLineHeight();
}

/// virtual, ensure we remain in diff
void CMergeDiffDetailView::EnsureVisible(POINT pt)
{
	if (EnsureInDiff(pt))
		SetCursorPos(pt);
	CCrystalTextView::EnsureVisible(pt);
}


/// virtual, ensure we remain in diff
void CMergeDiffDetailView::SetSelection(const POINT &ptStart, const POINT &ptEnd)
{
	POINT ptStartNew = ptStart;
	EnsureInDiff(ptStartNew);
	POINT ptEndNew = ptEnd;
	EnsureInDiff(ptEndNew);
	CCrystalTextView::SetSelection(ptStartNew, ptEndNew);
}

void CMergeDiffDetailView::OnInitialUpdate()
{
	SetFont(theApp.m_pMainWnd->m_lfDiff);
}

int CMergeDiffDetailView::GetAdditionalTextBlocks (int nLineIndex, TEXTBLOCK *pBuf)
{
	if (nLineIndex < m_lineBegin || nLineIndex > m_lineEnd)
		return 0;

	DWORD dwLineFlags = GetLineFlags(nLineIndex);
	if ((dwLineFlags & LF_DIFF) != LF_DIFF)
		return 0; // No diff

	if (!COptionsMgr::Get(OPT_WORDDIFF_HIGHLIGHT))
		return 0; // No coloring

	int nLineLength = GetLineLength(nLineIndex);
	vector<wdiff> worddiffs;
	m_pDocument->GetWordDiffArray(nLineIndex, &worddiffs);

	if (nLineLength == 0 || worddiffs.size() == 0 || // Both sides are empty
		IsSide0Empty(worddiffs, nLineLength) || IsSide1Empty(worddiffs, nLineLength))
	{
		return 0;
	}

	int nWordDiffs = worddiffs.size();

	pBuf[0].m_nCharPos = 0;
	pBuf[0].m_nColorIndex = COLORINDEX_NONE;
	pBuf[0].m_nBgColorIndex = COLORINDEX_NONE;
	for (int i = 0; i < nWordDiffs; i++)
	{
		const wdiff &wd = worddiffs[i];
		++pBuf;
		pBuf->m_nCharPos = wd.start[m_nThisPane];
		pBuf->m_nColorIndex = COLORINDEX_HIGHLIGHTTEXT1 | COLORINDEX_APPLYFORCE;
		if (wd.start[0] == wd.end[0] + 1 ||
			wd.start[1] == wd.end[1] + 1)
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
			// Line not inside diff, get colors from CrystalEditor
			CCrystalTextView::GetLineColors(nLineIndex, crBkgnd, crText);
	}
	if (nLineIndex < m_lineBegin || nLineIndex > m_lineEnd)
	{
		crBkgnd = GetColor(COLORINDEX_WHITESPACE);
		crText = GetColor(COLORINDEX_WHITESPACE);
	}
}

void CMergeDiffDetailView::OnDisplayDiff(int nDiff /*=0*/)
{
	int newlineBegin, newlineEnd;
	if (nDiff < 0 || nDiff >= m_pDocument->m_diffList.GetSize())
	{
		newlineBegin = 0;
		newlineEnd = -1;
	}
	else
	{
		DIFFRANGE curDiff;
		VERIFY(m_pDocument->m_diffList.GetDiff(nDiff, curDiff));

		newlineBegin = curDiff.dbegin0;
		ASSERT(newlineBegin >= 0);
		newlineEnd = curDiff.dend0;
	}

	if (newlineBegin == m_lineBegin && newlineEnd == m_lineEnd)
		return;
	m_lineBegin = newlineBegin;
	m_lineEnd = newlineEnd;
	m_diffLength = m_lineEnd - m_lineBegin + 1;

	// scroll to the first line of the diff
	ScrollToLine(m_lineBegin);
	// update the width of the horizontal scrollbar
	RecalcHorzScrollBar();
}

/**
 * @brief Adjust the point to remain in the displayed diff
 *
 * @return Tells if the point has been changed
 */
BOOL CMergeDiffDetailView::EnsureInDiff(POINT & pt)
{
	// first get the degenerate case out of the way
	// no diff ?
	if (m_diffLength == 0)
	{
		if (pt.y == m_lineBegin && pt.x == 0)
			return FALSE;
		pt.y = m_lineBegin;
		pt.x = 0;
		return TRUE;
	}

	// not above diff
	if (pt.y < m_lineBegin)
	{
		pt.y = m_lineBegin;
		pt.x = 0;
		return TRUE;
	}
	// diff is defined and not below diff
	if (m_lineEnd > -1 && pt.y > m_lineEnd)
	{
		pt.y = m_lineEnd;
		pt.x = GetLineLength(pt.y);
		return TRUE;
	}
	return FALSE;
}



/// virtual, ensure we remain in diff
void CMergeDiffDetailView::ScrollToSubLine(int nNewTopLine, BOOL bNoSmoothScroll, BOOL bTrackScrollBar)
{
	if (m_diffLength <= m_displayLength)
		nNewTopLine = m_lineBegin;
	else
	{
		if (nNewTopLine < m_lineBegin)
			nNewTopLine = m_lineBegin;
		if (nNewTopLine + m_displayLength - 1 > m_lineEnd)
			nNewTopLine = m_lineEnd - m_displayLength + 1;
	}
	m_nTopLine = nNewTopLine;
	
	POINT pt = GetCursorPos();
	if (EnsureInDiff(pt))
		SetCursorPos(pt);
	
	POINT ptSelStart, ptSelEnd;
	GetSelection (ptSelStart, ptSelEnd);
	if (EnsureInDiff(ptSelStart) || EnsureInDiff(ptSelEnd))
		SetSelection (ptSelStart, ptSelEnd);
	
	CCrystalTextView::ScrollToSubLine(nNewTopLine, bNoSmoothScroll, bTrackScrollBar);
}

/**
 * @brief Same purpose as the one as in MergeEditView.cpp
 * @note Nearly the same code also
 */
void CMergeDiffDetailView::UpdateSiblingScrollPos(BOOL bHorz)
{
	HWindow *pSender = reinterpret_cast<HWindow *>(m_hWnd);
	LPCTSTR pcwAtom = MAKEINTATOM(pSender->GetClassAtom());
	HWindow *pParent = pSender->GetParent();
	HWindow *pChild = NULL;
	// limit the TopLine : must be smaller than GetLineCount for all the panels
	int nNewTopLine = m_nTopLine;
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		if (pChild == pSender)
			continue;
		CMergeDiffDetailView *pSiblingView = static_cast<CMergeDiffDetailView *>(FromHandle(pChild));
		if (pSiblingView->GetLineCount() <= nNewTopLine)
			nNewTopLine = pSiblingView->GetLineCount() - 1;

	}
	if (m_nTopLine != nNewTopLine) 
	{
		// only modification from code in MergeEditView.cpp
		// Where are we now, are we still in a diff ? So set to no diff
		nNewTopLine = m_lineBegin = 0;
		m_lineEnd = -1;
		m_diffLength = 0;
		ScrollToLine(nNewTopLine);
	}
	while ((pChild = pParent->FindWindowEx(pChild, pcwAtom)) != NULL)
	{
		if (pChild == pSender) // We don't need to update ourselves
			continue;
		CMergeDiffDetailView *pSiblingView = static_cast<CMergeDiffDetailView *>(FromHandle(pChild));
		pSiblingView->OnUpdateSibling(this, bHorz);
	}
}

/**
 * @brief Same purpose as the one as in MergeDiffView.cpp
 * @note Code is the same except we cast to a pointer to a CMergeDiffDetailView
 */
void CMergeDiffDetailView::OnUpdateSibling (CCrystalTextView * pUpdateSource, BOOL bHorz)
{
	if (pUpdateSource != this)
	{
		ASSERT (pUpdateSource != NULL);
		// only modification from code in MergeEditView.cpp
		CMergeDiffDetailView *pSrcView = static_cast<CMergeDiffDetailView*>(pUpdateSource);
		if (!bHorz)  // changed this so bHorz works right
		{
			ASSERT (pSrcView->m_nTopLine >= 0);
			if (pSrcView->m_nTopLine != m_nTopLine)
			{
				ScrollToLine (pSrcView->m_nTopLine, TRUE, FALSE);
				UpdateCaret ();
				RecalcVertScrollBar(TRUE);
			}
		}
		else
		{
			ASSERT (pSrcView->m_nOffsetChar >= 0);
			if (pSrcView->m_nOffsetChar != m_nOffsetChar)
			{
				ScrollToChar (pSrcView->m_nOffsetChar, TRUE, FALSE);
				UpdateCaret ();
				RecalcHorzScrollBar(TRUE);
			}
		}
	}
}

/*
 * @brief Compute the max length of the lines inside the displayed diff
 */
int CMergeDiffDetailView::GetDiffLineLength ()
{
	int nMaxLineLength = 0;

	// we can not use GetLineActualLength below nLineCount
	// diff info (and lineBegin/lineEnd) are updated only during Rescan
	// they may get invalid just after we delete some text
	int validLineEnd = m_lineEnd;
	if (m_lineEnd >= GetLineCount())
		validLineEnd = GetLineCount() - 1;

	for (int I = m_lineBegin; I <= validLineEnd; I++)
	{
		int nActualLength = GetLineActualLength (I);
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
void CMergeDiffDetailView::RecalcHorzScrollBar (BOOL bPositionOnly /*= FALSE*/ )
{
	// Again, we cannot use nPos because it's 16-bit
	SCROLLINFO si;
	si.cbSize = sizeof si;

	const int nScreenChars = GetScreenChars();

	// note : this value differs from the value in CCrystalTextView::RecalcHorzScrollBar
	int nMaxLineLen = 0;
	if (m_pDocument->GetRightDetailView())
		nMaxLineLen = m_pDocument->GetRightDetailView()->GetDiffLineLength();
	if (m_pDocument->GetLeftDetailView())
		nMaxLineLen = max(nMaxLineLen, m_pDocument->GetLeftDetailView()->GetDiffLineLength());
	
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nOffsetChar;
	}
	else
	{
		if (nScreenChars >= nMaxLineLen && m_nOffsetChar > 0)
		{
			m_nOffsetChar = 0;
			Invalidate ();
			UpdateCaret ();
		}
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		
		// Horiz scroll limit to longest line + one screenwidth 
		si.nMax = nMaxLineLen + nScreenChars;
		si.nPage = nScreenChars;
		si.nPos = m_nOffsetChar;
	}
	if (GetStyle() & WS_HSCROLL)
	  SetScrollInfo(SB_HORZ, &si);
}

void CMergeDiffDetailView::PushCursors()
{
	// push lineBegin and the cursor
	m_lineBeginPushed = m_lineBegin;
	m_ptCursorPosPushed = m_ptCursorPos;
	// and top line positions
	m_nTopSubLinePushed = m_nTopSubLine;
}

void CMergeDiffDetailView::PopCursors()
{
	m_lineBegin = m_lineBeginPushed;
	m_lineEnd = m_lineBegin + m_diffLength - 1;

	m_ptCursorPos = m_ptCursorPosPushed;

	if (m_lineBegin >= GetLineCount())
	{
		// even the first line is invalid, stop displaying the diff
		m_lineBegin = m_nTopLine = m_nTopSubLine = 0;
		m_lineEnd = -1;
		m_diffLength = 0;
	}
	else
	{
		// just check that all positions all valid
		m_lineEnd = min(m_lineEnd, GetLineCount()-1);
		m_diffLength = m_lineEnd - m_lineBegin + 1;
		m_ptCursorPos.y = min<long>(m_ptCursorPos.y, GetLineCount()-1);
		m_ptCursorPos.x = min<long>(m_ptCursorPos.x, GetLineLength(m_ptCursorPos.y));
	}

		// restore the scrolling position
	m_nTopSubLine = m_nTopSubLinePushed;
	if (m_nTopSubLine >= GetSubLineCount())
		m_nTopSubLine = GetSubLineCount() - 1;
	int nDummy;
	GetLineBySubLine( m_nTopSubLine, m_nTopLine, nDummy );
    RecalcVertScrollBar(TRUE);

	// other positions are set to (0,0) during ResetView
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

void CMergeDiffDetailView::OnUpdateCaret(bool bMove)
{
	CCrystalTextView::OnUpdateCaret(bMove);
	m_pDocument->UpdateCmdUI();
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
