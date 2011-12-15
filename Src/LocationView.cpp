/////////////////////////////////////////////////////////////////////////////
//
//    WinMerge: An interactive diff/merge utility
//    Copyright (C) 1997 Dean P. Grimm
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
 * @file  LocationView.cpp
 *
 * @brief Implementation file for CLocationView
 *
 */

// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"
#include "MainFrm.h"
#include "OptionsMgr.h"
#include "MergeEditView.h"
#include "LocationView.h"
#include "OptionsDef.h"
#include "MergeLineFlags.h"
#include "Bitmap.h"
#include "memdc.h"
#include "SyntaxColors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using stl::vector;

/** @brief Size of empty frame above and below bars (in pixels). */
static const int Y_OFFSET = 5;
/** @brief Size of y-margin for visible area indicator (in pixels). */
static const long INDICATOR_MARGIN = 2;
/** @brief Max pixels in view per line in file. */
static const double MAX_LINEPIX = 4.0;
/** @brief Top of difference marker, relative to difference start. */
static const int DIFFMARKER_TOP = 3;
/** @brief Bottom of difference marker, relative to difference start. */
static const int DIFFMARKER_BOTTOM = 3;
/** @brief Width of difference marker. */
static const int DIFFMARKER_WIDTH = 6;
/** @brief Minimum height of the visible area indicator */
static const int INDICATOR_MIN_HEIGHT = 5;

/** 
 * @brief Bars in location pane
 */
enum LOCBAR_TYPE
{
	BAR_NONE = -1,	/**< No bar in given coords */
	BAR_LEFT,		/**< Left side bar in given coords */
	BAR_RIGHT,		/**< Right side bar in given coords */
	BAR_YAREA,		/**< Y-Coord in bar area */
};

/////////////////////////////////////////////////////////////////////////////
// CMergeDiffDetailView

CLocationView::CLocationView(CChildFrame *pMergeDoc)
	: m_pMergeDoc(pMergeDoc)
	, m_visibleTop(-1)
	, m_visibleBottom(-1)
//	MOVEDLINE_LIST m_movedLines; //*< List of moved block connecting lines */
	, m_pSavedBackgroundBitmap(NULL)
	, m_bRecalculateBlocks(TRUE) // calculate for the first time
	, m_displayMovedBlocks(COptionsMgr::Get(OPT_CONNECT_MOVED_BLOCKS))
{
}

CLocationView::~CLocationView()
{
	if (m_pSavedBackgroundBitmap)
		m_pSavedBackgroundBitmap->DeleteObject();
}

LRESULT CLocationView::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CONTEXTMENU:
		OnContextMenu(lParam);
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(lParam);
		break;
	case WM_LBUTTONDOWN:
		SetCapture();
		OnLButtonDown(lParam);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		PAINTSTRUCT ps;
		if (HSurface *pdc = BeginPaint(&ps))
		{
			OnDraw(pdc);
			EndPaint(&ps);
		}
		return 0;
	}
	return OWindow::WindowProc(uMsg, wParam, lParam);
}

void CLocationView::SetConnectMovedBlocks(int displayMovedBlocks) 
{
	if (m_displayMovedBlocks == displayMovedBlocks)
		return;
	COptionsMgr::SaveOption(OPT_CONNECT_MOVED_BLOCKS, displayMovedBlocks);
	m_displayMovedBlocks = displayMovedBlocks;
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CLocationView diagnostics

/////////////////////////////////////////////////////////////////////////////
// CLocationView message handlers

/**
 * @brief Force recalculation and update of location pane.
 * This method forces location pane to first recalculate its data and
 * then repaint itself. This method bypasses location pane's caching
 * of the diff data.
 */
void CLocationView::ForceRecalculate()
{
	m_bRecalculateBlocks = TRUE;
	Invalidate();
}

/**
 * @brief Draw custom (non-white) background.
 * @param [in] pDC Pointer to draw context.
 */
void CLocationView::DrawBackground(HSurface *pDC)
{
	// Set brush to desired background color
	HBrush *backBrush = HBrush::CreateSolidBrush(RGB(0xe8, 0xe8, 0xf4));
	// Save old brush
	HGdiObj *pOldBrush = pDC->SelectObject(backBrush);
	RECT rect;
	pDC->GetClipBox(&rect);     // Erase the area needed
	pDC->PatBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PATCOPY);
	pDC->SelectObject(pOldBrush);
	backBrush->DeleteObject();
}

/**
 * @brief Calculate bar coordinates and scaling factors.
 */
void CLocationView::CalculateBars()
{
	CMergeEditView *const pLeftView = m_pMergeDoc->GetLeftView();
	CMergeEditView *const pRightView = m_pMergeDoc->GetRightView();
	RECT rc;
	GetClientRect(&rc);
	const int w = rc.right / 4;
	m_leftBar.left = (rc.right - 2 * w) / 3;
	m_leftBar.right = m_leftBar.left + w;
	m_rightBar.left = 2 * m_leftBar.left + w;
	m_rightBar.right = m_rightBar.left + w;
	const double hTotal = rc.bottom - (2 * Y_OFFSET); // Height of draw area
	const int nbLines = min(
		pLeftView->GetSubLineCount(), pRightView->GetSubLineCount());

	m_lineInPix = hTotal / nbLines;
	m_pixInLines = nbLines / hTotal;
	if (m_lineInPix > MAX_LINEPIX)
	{
		m_lineInPix = MAX_LINEPIX;
		m_pixInLines = 1 / MAX_LINEPIX;
	}

	m_leftBar.top = Y_OFFSET - 1;
	m_rightBar.top = Y_OFFSET - 1;
	m_leftBar.bottom = (LONG)(m_lineInPix * nbLines + Y_OFFSET + 1);
	m_rightBar.bottom = m_leftBar.bottom;
}

/**
 * @brief Calculate difference lines and coordinates.
 * This function calculates begin- and end-lines of differences when word-wrap
 * is enabled. Otherwise the value from original difflist is used. Line
 * numbers are also converted to coordinates in the window. All calculated
 * (and not ignored) differences are added to the new list.
 */
void CLocationView::CalculateBlocks()
{
	// lineposition in pixels.
	int nBeginY;
	int nEndY;

	m_diffBlocks.clear();

	const int nDiffs = m_pMergeDoc->m_diffList.GetSize();
	if (nDiffs > 0)
		m_diffBlocks.reserve(nDiffs); // Pre-allocate space for the list.

	int nDiff = m_pMergeDoc->m_diffList.FirstSignificantDiff();
	while (nDiff != -1)
	{
		DIFFRANGE diff;
		VERIFY(m_pMergeDoc->m_diffList.GetDiff(nDiff, diff));

		CMergeEditView *pView = m_pMergeDoc->GetLeftView();

		DiffBlock block;
		//there are no blanks on both side
		if ((diff.blank0 == 0) && (diff.blank1 == 0))
		{
			CalculateBlocksPixel(
				pView->GetSubLineIndex(diff.dbegin0),
				pView->GetSubLineIndex(diff.dend0),
				pView->GetSubLines(diff.dend0),	nBeginY, nEndY);

			block.top_line = diff.dbegin0;
			block.bottom_line = diff.dend0;
			block.top_coord = nBeginY;
			block.bottom_coord = nEndY;
			block.diff_index = nDiff;
			m_diffBlocks.push_back(block);
		}
		//side0 has blank lines?
		else if (diff.blank0 > 0)
		{
			//Is there a common block on side0?
			if ((int)diff.dbegin0 < diff.blank0)
			{
				CalculateBlocksPixel(
					pView->GetSubLineIndex(diff.dbegin0),
					pView->GetSubLineIndex(diff.blank0 - 1),
					pView->GetSubLines(diff.blank0 - 1), nBeginY, nEndY);

				block.top_line = diff.dbegin0;
				block.bottom_line = diff.blank0 - 1;
				block.top_coord = nBeginY;
				block.bottom_coord = nEndY;
				block.diff_index = nDiff;
				m_diffBlocks.push_back(block);
			}

			// Now the block for blank lines side0!
			CalculateBlocksPixel(
				pView->GetSubLineIndex(diff.blank0),
				pView->GetSubLineIndex(diff.dend1),
				pView->GetSubLines(diff.dend1), nBeginY, nEndY);

			block.top_line = diff.blank0;
			block.bottom_line = diff.dend1;
			block.top_coord = nBeginY;
			block.bottom_coord = nEndY;
			block.diff_index = nDiff;
			m_diffBlocks.push_back(block);
		}
		//side1 has blank lines?
		else
		{
			// Is there a common block on side1?
			if ((int)diff.dbegin0 < diff.blank1)
			{
				CalculateBlocksPixel(
					pView->GetSubLineIndex(diff.dbegin0),
					pView->GetSubLineIndex(diff.blank1 - 1),
					pView->GetSubLines(diff.blank1 - 1), nBeginY, nEndY);

				block.top_line = diff.dbegin0;
				block.bottom_line = diff.blank1 - 1;
				block.top_coord = nBeginY;
				block.bottom_coord = nEndY;
				block.diff_index = nDiff;
				m_diffBlocks.push_back(block);
			}

			// Now the block for blank lines side1!
			CalculateBlocksPixel(
				pView->GetSubLineIndex(diff.blank1),
				pView->GetSubLineIndex(diff.dend0),
				pView->GetSubLines(diff.dend0), nBeginY, nEndY);

			block.top_line = diff.blank1;
			block.bottom_line = diff.dend0;
			block.top_coord = nBeginY;
			block.bottom_coord = nEndY;
			block.diff_index = nDiff;
			m_diffBlocks.push_back(block);
		}

		nDiff = m_pMergeDoc->m_diffList.NextSignificantDiff(nDiff);
	}
	m_bRecalculateBlocks = FALSE;
}

/**
 * @brief Calculate Blocksize to pixel.
 * @param [in] nBlockStart line where block starts
 * @param [in] nBlockEnd   line where block ends 
 * @param [in] nBlockLength length of the block
 * @param [in,out] nBeginY pixel in y  where block starts
 * @param [in,out] nEndY   pixel in y  where block ends

 */
void CLocationView::CalculateBlocksPixel(int nBlockStart, int nBlockEnd,
		int nBlockLength, int &nBeginY, int &nEndY)
{
	// Count how many line does the diff block have.
	const int nBlockHeight = nBlockEnd - nBlockStart + nBlockLength;

	// Convert diff block size from lines to pixels.
	nBeginY = (int)(nBlockStart * m_lineInPix + Y_OFFSET);
	nEndY = (int)((nBlockStart + nBlockHeight) * m_lineInPix + Y_OFFSET);
}
/** 
 * @brief Draw maps of files.
 *
 * Draws maps of differences in files. Difference list is walked and
 * every difference is drawn with same colors as in editview.
 * @note We MUST use doubles when calculating coords to avoid rounding
 * to integers. Rounding causes miscalculation of coords.
 * @sa CLocationView::DrawRect()
 */
void CLocationView::OnDraw(HSurface *pdc)
{
	CMergeEditView *const pLeftView = m_pMergeDoc->GetLeftView();
	CMergeEditView *const pRightView = m_pMergeDoc->GetRightView();
	CMergeEditView *const pView = pLeftView;
	if (pLeftView == NULL || pRightView == NULL)
	{
		assert(false);
		return;
	}

	RECT rc;
	GetClientRect(&rc);
	CMemDC dc(pdc, &rc);

	COLORREF cr0 = CLR_NONE; // Left side color
	COLORREF cr1 = CLR_NONE; // Right side color
	COLORREF crt = CLR_NONE; // Text color
	BOOL bwh = FALSE;

	m_movedLines.clear();

	CalculateBars();
	DrawBackground(dc.m_pDC);

	// Draw bar outlines
	HGdiObj *oldObj = dc.SelectStockObject(BLACK_PEN);
	HBrush *brush = HBrush::CreateSolidBrush(pLeftView->GetColor(COLORINDEX_WHITESPACE));
	HGdiObj *oldBrush = dc.SelectObject(brush);
	dc.Rectangle(m_leftBar.left, m_leftBar.top, m_leftBar.right, m_leftBar.bottom);
	dc.Rectangle(m_rightBar.left, m_rightBar.top, m_rightBar.right, m_rightBar.bottom);
	dc.SelectObject(oldBrush);
	dc.SelectObject(oldObj);
	brush->DeleteObject();
	// Iterate the differences list and draw differences as colored blocks.

	// Don't recalculate blocks if we earlier determined it is not needed
	// This may save lots of processing
	if (m_bRecalculateBlocks)
		CalculateBlocks();

	int nPrevEndY = -1;
	const int nCurDiff = m_pMergeDoc->GetCurrentDiff();

	// Protect against out of bound accesses after line deletions
	const unsigned safe_line_count = min(pLeftView->GetLineCount(), pRightView->GetLineCount());

	vector<DiffBlock>::const_iterator iter = m_diffBlocks.begin();
	while (iter != m_diffBlocks.end() && iter->top_line < safe_line_count)
	{
		const bool bInsideDiff = nCurDiff == iter->diff_index;

		if ((nPrevEndY != iter->bottom_coord) || bInsideDiff)
		{
			// Draw left side block
			pLeftView->GetLineColors2(iter->top_line, 0, cr0, crt, bwh);
			RECT r0 = { m_leftBar.left, iter->top_coord, m_leftBar.right, iter->bottom_coord };
			DrawRect(dc.m_pDC, r0, cr0, bInsideDiff);

			// Draw right side block
			pRightView->GetLineColors2(iter->top_line, 0, cr1, crt, bwh);
			RECT r1 = { m_rightBar.left, iter->top_coord, m_rightBar.right, iter->bottom_coord };
			DrawRect(dc.m_pDC, r1, cr1, bInsideDiff);
		}
		nPrevEndY = iter->bottom_coord;

		// Test if we draw a connector
		BOOL bDisplayConnectorFromLeft = FALSE;
		BOOL bDisplayConnectorFromRight = FALSE;

		switch (m_displayMovedBlocks)
		{
		case DISPLAY_MOVED_FOLLOW_DIFF:
			// display moved block only for current diff
			if (!bInsideDiff)
				break;
			// two sides may be linked to a block somewhere else
			bDisplayConnectorFromLeft = TRUE;
			bDisplayConnectorFromRight = TRUE;
			break;
		case DISPLAY_MOVED_ALL:
			// we display all moved blocks, so once direction is enough
			bDisplayConnectorFromLeft = TRUE;
			break;
		default:
			break;
		}

		if (bDisplayConnectorFromLeft)
		{
			int apparent0 = iter->top_line;
			int apparent1 = m_pMergeDoc->RightLineInMovedBlock(apparent0);
			const int nBlockHeight = iter->bottom_line - iter->top_line;
			if (apparent1 != -1)
			{
				MovedLine line;
				apparent0 = pView->GetSubLineIndex(apparent0);
				apparent1 = pView->GetSubLineIndex(apparent1);
				line.ptLeft.x = m_leftBar.right;
				int leftUpper = (int) (apparent0 * m_lineInPix + Y_OFFSET);
				int leftLower = (int) ((nBlockHeight + apparent0) * m_lineInPix + Y_OFFSET);
				line.ptLeft.y = leftUpper + (leftLower - leftUpper) / 2;
				line.ptRight.x = m_rightBar.left;
				int rightUpper = (int) (apparent1 * m_lineInPix + Y_OFFSET);
				int rightLower = (int) ((nBlockHeight + apparent1) * m_lineInPix + Y_OFFSET);
				line.ptRight.y = rightUpper + (rightLower - rightUpper) / 2;
				m_movedLines.push_back(line);
			}
		}

		if (bDisplayConnectorFromRight)
		{
			int apparent1 = iter->top_line;
			int apparent0 = m_pMergeDoc->LeftLineInMovedBlock(apparent1);
			const int nBlockHeight = iter->bottom_line - iter->top_line;
			if (apparent0 != -1)
			{
				MovedLine line;
				apparent0 = pView->GetSubLineIndex(apparent0);
				apparent1 = pView->GetSubLineIndex(apparent1);
				line.ptLeft.x = m_leftBar.right;
				int leftUpper = (int) (apparent0 * m_lineInPix + Y_OFFSET);
				int leftLower = (int) ((nBlockHeight + apparent0) * m_lineInPix + Y_OFFSET);
				line.ptLeft.y = leftUpper + (leftLower - leftUpper) / 2;
				line.ptRight.x = m_rightBar.left;
				int rightUpper = (int) (apparent1 * m_lineInPix + Y_OFFSET);
				int rightLower = (int) ((nBlockHeight + apparent1) * m_lineInPix + Y_OFFSET);
				line.ptRight.y = rightUpper + (rightLower - rightUpper) / 2;
				m_movedLines.push_back(line);
			}
		}
		++iter;
	}

	if (m_displayMovedBlocks != DISPLAY_MOVED_NONE)
		DrawConnectLines(dc.m_pDC);

	if (m_pSavedBackgroundBitmap)
		m_pSavedBackgroundBitmap->DeleteObject();
	m_pSavedBackgroundBitmap = CopyRectToBitmap(dc.m_pDC, rc);

	// Since we have invalidated locationbar there is no previous
	// arearect to remove
	m_visibleTop = -1;
	m_visibleBottom = -1;
	DrawVisibleAreaRect(dc.m_pDC);
}

/** 
 * @brief Draw one block of map.
 * @param [in] pDC Draw context.
 * @param [in] r Rectangle to draw.
 * @param [in] cr Color for rectangle.
 * @param [in] bSelected Is rectangle for selected difference?
 */
void CLocationView::DrawRect(HSurface *pDC, const RECT& r, COLORREF cr, BOOL bSelected)
{
	// Draw only colored blocks
	if (cr != CLR_NONE && cr != GetSysColor(COLOR_WINDOW))
	{
		RECT drawRect = { r.left + 1, r.top, r.right - 1, r.bottom };
		// With long files and small difference areas rect may become 0-height.
		// Make sure all diffs are drawn at least one pixel height.
		if (drawRect.bottom <= drawRect.top)
			++drawRect.bottom;
		pDC->SetBkColor(cr);
		pDC->ExtTextOut(0, 0, ETO_OPAQUE, &drawRect, NULL, 0);

		if (bSelected)
		{
			DrawDiffMarker(pDC, r.top);
		}
	}
}

/**
 * @brief Capture the mouse target.
 */
void CLocationView::OnLButtonDown(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	GotoLocation(point, false);
}

/**
 * @brief Process drag action on a captured mouse.
 *
 * Reposition on every dragged movement.
 * The Screen update stress will be similar to a mouse wheeling.:-)
 */
void CLocationView::OnMouseMove(LPARAM lParam)
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	if (::GetCapture() == m_hWnd)
	{
		// Don't go above bars.
		point.y = max<long>(point.y, Y_OFFSET);

		// Vertical scroll handlers are range safe, so there is no need to
		// make sure value is valid and in range.
		// Just a random choose as both view share the same scroll bar.
		CMergeEditView *const pView = m_pMergeDoc->GetRightView();
		int nSubLine = (int) (m_pixInLines * (point.y - Y_OFFSET));
		nSubLine -= pView->GetScreenLines() / 2;
		if (nSubLine < 0)
			nSubLine = 0;

		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_POS;
		si.nPos = nSubLine;
		pView->SetScrollInfo(SB_VERT, &si);
		pView->SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, 0), NULL);
	}
}

/**
 * @brief Scroll both views to point given.
 *
 * Scroll views to given line. There is two ways to scroll, based on
 * view lines (ghost lines counted in) or on real lines (no ghost lines).
 * In most cases view lines should be used as it avoids real line number
 * calculation and is able to scroll to all lines - real line numbers
 * cannot be used to scroll to ghost lines.
 *
 * @param [in] point Point to move to
 * @param [in] bRealLine TRUE if we want to scroll using real line num,
 * FALSE if view linenumbers are OK.
 * @return TRUE if succeeds, FALSE if point not inside bars.
 */
bool CLocationView::GotoLocation(const POINT& point, bool bRealLine)
{
	RECT rc;
	GetClientRect(&rc);

	int line = -1;
	int bar = IsInsideBar(rc, point);
	if (bar == BAR_LEFT || bar == BAR_RIGHT)
	{
		line = GetLineFromYPos(point.y, bar, bRealLine);
	}
	else if (bar == BAR_YAREA)
	{
		// Outside bars, use left bar
		bar = BAR_LEFT;
		line = GetLineFromYPos(point.y, bar, FALSE);
	}
	else
		return false;

	m_pMergeDoc->GetLeftView()->GotoLine(line, bRealLine, bar);
	if (bar == BAR_LEFT || bar == BAR_RIGHT)
		m_pMergeDoc->GetView(bar)->SetFocus();

	return true;
}

/**
 * Show context menu and handle user selection.
 */
void CLocationView::OnContextMenu(LPARAM lParam) 
{
	POINT point;
	POINTSTOPOINT(point, lParam);
	if (point.x == -1 && point.y == -1)
	{
		//keystroke invocation
		point.x = point.y = 5;
		ClientToScreen(&point);
	}

	RECT rc;
	POINT pt = point;
	GetClientRect(&rc);
	ScreenToClient(&pt);

	HMenu *const pMenu = LanguageSelect.LoadMenu(IDR_POPUP_LOCATIONBAR);
	HMenu *const pSub = pMenu->GetSubMenu(0);
	C_ASSERT(ID_DISPLAY_MOVED_NONE == ID_DISPLAY_MOVED_NONE + DISPLAY_MOVED_NONE);
	C_ASSERT(ID_DISPLAY_MOVED_ALL == ID_DISPLAY_MOVED_NONE + DISPLAY_MOVED_ALL);
	C_ASSERT(ID_DISPLAY_MOVED_FOLLOW_DIFF == ID_DISPLAY_MOVED_NONE + DISPLAY_MOVED_FOLLOW_DIFF);
	pSub->CheckMenuRadioItem(
		ID_DISPLAY_MOVED_NONE,
		ID_DISPLAY_MOVED_FOLLOW_DIFF,
		ID_DISPLAY_MOVED_NONE + m_displayMovedBlocks);

	String strNum;
	int nLine = -1;
	int bar = IsInsideBar(rc, pt);

	// If cursor over bar, format string with linenumber, else disable item
	if (bar != BAR_NONE)
	{
		// If outside bar area use left bar
		if (bar == BAR_YAREA)
			bar = BAR_LEFT;
		nLine = GetLineFromYPos(pt.y, bar);
		strNum = string_format(_T("%d"), nLine + 1); // Show linenumber not lineindex
	}
	else
		pSub->EnableMenuItem(ID_LOCBAR_GOTODIFF, MF_GRAYED);
	pSub->ModifyMenu(ID_LOCBAR_GOTODIFF, MF_BYCOMMAND, ID_LOCBAR_GOTODIFF,
		LanguageSelect.FormatMessage(ID_LOCBAR_GOTOLINE_FMT, strNum.c_str()));

	// invoke context menu
	// we don't want to use the main application handlers, so we use flags TPM_NONOTIFY | TPM_RETURNCMD
	// and handle the command after TrackPopupMenu
	int command = pSub->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY  | TPM_RETURNCMD,
		point.x, point.y, GetMainFrame()->m_hWnd);

	pMenu->DestroyMenu();

	switch (command)
	{
	case ID_LOCBAR_GOTODIFF:
		m_pMergeDoc->GetLeftView()->GotoLine(nLine, true, bar);
		if (bar == BAR_LEFT || bar == BAR_RIGHT)
			m_pMergeDoc->GetView(bar)->SetFocus();
		break;
	case ID_EDIT_WMGOTO:
		GetMainFrame()->PostMessage(WM_COMMAND, ID_EDIT_WMGOTO);
		break;
	case ID_DISPLAY_MOVED_NONE:
		SetConnectMovedBlocks(DISPLAY_MOVED_NONE);
		m_pMergeDoc->SetDetectMovedBlocks(FALSE);
		break;
	case ID_DISPLAY_MOVED_ALL:
		SetConnectMovedBlocks(DISPLAY_MOVED_ALL);
		m_pMergeDoc->SetDetectMovedBlocks(TRUE);
		break;
	case ID_DISPLAY_MOVED_FOLLOW_DIFF:
		SetConnectMovedBlocks(DISPLAY_MOVED_FOLLOW_DIFF);
		m_pMergeDoc->SetDetectMovedBlocks(TRUE);
		break;
	}
}

/** 
 * @brief Calculates view/real line in file from given YCoord in bar.
 * @param [in] nYCoord ycoord in pane
 * @param [in] bar bar/file
 * @param [in] bRealLine TRUE if real line is returned, FALSE for view line
 * @return 0-based index of view/real line in file [0...lines-1]
 */
int CLocationView::GetLineFromYPos(int nYCoord, int bar, BOOL bRealLine)
{
	CMergeEditView *const pView = m_pMergeDoc->GetView(bar);

	int nSubLineIndex = (int) (m_pixInLines * (nYCoord - Y_OFFSET));

	// Keep sub-line index in range.
	if (nSubLineIndex < 0)
	{
		nSubLineIndex = 0;
	}
	else if (nSubLineIndex >= pView->GetSubLineCount())
	{
		nSubLineIndex = pView->GetSubLineCount() - 1;
	}

	// Find the real (not wrapped) line number from sub-line index.
	int nLine = 0;
	int nSubLine = 0;
	pView->GetLineBySubLine(nSubLineIndex, nLine, nSubLine);

	// Convert line number to line index.
	if (nLine > 0)
	{
		nLine -= 1;
	}

	// We've got a view line now
	if (bRealLine == FALSE)
		return nLine;

	// Get real line (exclude ghost lines)
	const int nRealLine = m_pMergeDoc->m_ptBuf[bar]->ComputeRealLine(nLine);
	return nRealLine;
}

/** 
 * @brief Determines if given coords are inside left/right bar.
 * @param rc [in] size of locationpane client area
 * @param pt [in] point we want to check, in client coordinates.
 * @return LOCBAR_TYPE area where point is.
 */
int CLocationView::IsInsideBar(const RECT& rc, const POINT& pt)
{
	int retVal = BAR_NONE;

	if (PtInRect(&m_leftBar, pt))
		retVal = BAR_LEFT;
	else if (PtInRect(&m_rightBar, pt))
		retVal = BAR_RIGHT;
	else if (pt.x >= INDICATOR_MARGIN && pt.x < (rc.right - rc.left - INDICATOR_MARGIN) &&
		pt.y > m_leftBar.top && pt.y <= m_leftBar.bottom)
	{
		retVal = BAR_YAREA;
	}

	return retVal;
}

/** 
 * @brief Draws rect indicating visible area in file views.
 *
 * @param [in] nTopLine New topline for indicator
 * @param [in] nBottomLine New bottomline for indicator
 * @todo This function dublicates too much DrawRect() code.
 */
void CLocationView::DrawVisibleAreaRect(HSurface *pClientDC, int nTopLine, int nBottomLine)
{
	CMergeEditView *const pLeftView = m_pMergeDoc->GetLeftView();
	CMergeEditView *const pRightView = m_pMergeDoc->GetRightView();
	if (nTopLine == -1)
		nTopLine = pRightView->GetTopSubLine();
	
	if (nBottomLine == -1)
	{
		const int nScreenLines = pRightView->GetScreenLines();
		nBottomLine = nTopLine + nScreenLines;
	}

	RECT rc;
	GetClientRect(&rc);
	const int nbLines = min(
		pLeftView->GetSubLineCount(), pRightView->GetSubLineCount());

	int nTopCoord = static_cast<int>(Y_OFFSET +
			(static_cast<double>(nTopLine * m_lineInPix)));
	int nBottomCoord = static_cast<int>(Y_OFFSET +
			(static_cast<double>(nBottomLine * m_lineInPix)));
	
	double xbarBottom = min<double>(nbLines / m_pixInLines + Y_OFFSET, rc.bottom - Y_OFFSET);
	int barBottom = (int)xbarBottom;
	// Make sure bottom coord is in bar range
	nBottomCoord = min(nBottomCoord, barBottom);

	// Ensure visible area is at least minimum height
	if (nBottomCoord - nTopCoord < INDICATOR_MIN_HEIGHT)
	{
		// If area is near top of file, add additional area to bottom
		// of the bar and vice versa.
		if (nTopCoord < Y_OFFSET + 20)
			nBottomCoord += INDICATOR_MIN_HEIGHT - (nBottomCoord - nTopCoord);
		else
		{
			// Make sure locationbox has min hight
			if ((nBottomCoord - nTopCoord) < INDICATOR_MIN_HEIGHT)
			{
				// If we have a high number of lines, it may be better
				// to keep the topline, otherwise the cursor can 
				// jump up and down unexpected
				nBottomCoord = nTopCoord + INDICATOR_MIN_HEIGHT;
			}
		}
	}

	// Store current values for later use (to check if area changes)
	m_visibleTop = nTopCoord;
	m_visibleBottom = nBottomCoord;

	RECT rcVisibleArea = { 2, m_visibleTop, rc.right - 2, m_visibleBottom };
	HBitmap *pBitmap = CopyRectToBitmap(pClientDC, rcVisibleArea);
	HBitmap *pDarkenedBitmap = GetDarkenedBitmap(pClientDC, pBitmap);
	DrawBitmap(pClientDC, rcVisibleArea.left, rcVisibleArea.top, pDarkenedBitmap);
	pDarkenedBitmap->DeleteObject();
	pBitmap->DeleteObject();
}

/**
 * @brief Public function for updating visible area indicator.
 *
 * @param [in] nTopLine New topline for indicator
 * @param [in] nBottomLine New bottomline for indicator
 */
void CLocationView::UpdateVisiblePos(int nTopLine, int nBottomLine)
{
	if (m_pSavedBackgroundBitmap)
	{
		int nTopCoord = static_cast<int>(Y_OFFSET +
				(static_cast<double>(nTopLine * m_lineInPix)));
		int nBottomCoord = static_cast<int>(Y_OFFSET +
				(static_cast<double>(nBottomLine * m_lineInPix)));
		if (m_visibleTop != nTopCoord || m_visibleBottom != nBottomCoord)
		{
			if (HSurface *pdc = GetDC())
			{
				{
					RECT rc;
					GetClientRect(&rc);
					CMemDC dc(pdc, &rc);
					// Clear previous visible rect
					DrawBitmap(dc.m_pDC, 0, 0, m_pSavedBackgroundBitmap);
					// Draw new visible rect
					DrawVisibleAreaRect(dc.m_pDC, nTopLine, nBottomLine);
				}
				ReleaseDC(pdc);
			}
		}
	}
}

/** 
 * @brief Draw lines connecting moved blocks.
 */
void CLocationView::DrawConnectLines(HSurface *pClientDC)
{
	HGdiObj *oldObj = pClientDC->SelectStockObject(BLACK_PEN);

	MOVEDLINE_LIST::iterator it = m_movedLines.begin();
	while (it != m_movedLines.end())
	{
		MovedLine line = *it++;
		pClientDC->MoveTo(line.ptLeft.x, line.ptLeft.y);
		pClientDC->LineTo(line.ptRight.x, line.ptRight.y);
	}

	pClientDC->SelectObject(oldObj);
}

/** 
 * @brief Draw marker for top of currently selected difference.
 * This function draws marker for top of currently selected difference.
 * This marker makes it a lot easier to see where currently selected
 * difference is in location bar. Especially when selected diffence is
 * small and it is not easy to find it otherwise.
 * @param [in] pDC Pointer to draw context.
 * @param [in] yCoord Y-coord of top of difference, -1 if no difference.
 */
void CLocationView::DrawDiffMarker(HSurface* pDC, int yCoord)
{
	POINT points[3];
	points[0].x = m_leftBar.left - DIFFMARKER_WIDTH - 1;
	points[0].y = yCoord - DIFFMARKER_TOP;
	points[1].x = m_leftBar.left - 1;
	points[1].y = yCoord;
	points[2].x = m_leftBar.left - DIFFMARKER_WIDTH - 1;
	points[2].y = yCoord + DIFFMARKER_BOTTOM;

	HGdiObj *oldObj = pDC->SelectStockObject(BLACK_PEN);
	HBrush *brushBlue = HBrush::CreateSolidBrush(RGB(0x80, 0x80, 0xff));
	HGdiObj *pOldBrush = pDC->SelectObject(brushBlue);

	pDC->SetPolyFillMode(WINDING);
	pDC->Polygon(points, 3);

	points[0].x = m_rightBar.right + 1 + DIFFMARKER_WIDTH;
	points[1].x = m_rightBar.right + 1;
	points[2].x = m_rightBar.right + 1 + DIFFMARKER_WIDTH;
	pDC->Polygon(points, 3);

	pDC->SelectObject(pOldBrush);
	brushBlue->DeleteObject();
	pDC->SelectObject(oldObj);
}
