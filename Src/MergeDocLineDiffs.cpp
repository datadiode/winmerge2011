/** 
 * @file  MergeDocLineDiffs.cpp
 *
 * @brief Implementation file for word diff highlighting (F4) for merge edit & detail views
 *
 */
#include "StdAfx.h"
#include "Merge.h"
#include "LanguageSelect.h"

#include "MergeEditView.h"
#include "MergeDiffDetailView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using std::vector;

/**
 * @brief Display the line/word difference highlight in edit view
 */
static void HighlightDiffRect(CCrystalTextView *pView, int line, int start, int end)
{
	// select the area
	// with anchor at left and caret at right
	// this seems to be conventional behavior in Windows editors
	POINT ptTopLeft = { start, line };
	POINT ptBottomRight = { end, line };
	pView->SetSelection(ptTopLeft, ptBottomRight);
	pView->SetCursorPos(ptBottomRight);
	pView->SetAnchor(ptTopLeft);
	// try to ensure that selected area is visible
	pView->EnsureSelectionVisible();
}

/**
 * @brief Highlight difference in current line (left & right panes)
 */
void CChildFrame::Showlinediff(CGhostTextView *pTextView, CMergeEditView *pActiveView, bool reverse)
{
	wdiff wd;
	int line = Computelinediff(pTextView, wd, reverse);
	if (line == -1)
	{
		String caption = LanguageSelect.LoadString(IDS_LINEDIFF_NODIFF_CAPTION);
		String msg = LanguageSelect.LoadString(IDS_LINEDIFF_NODIFF);
		pTextView->MessageBox(msg.c_str(), caption.c_str(), MB_OK);
		return;
	}

	// Actually display selection areas on screen in both edit panels
	CCrystalTextView *pView[] = { m_pView[0], m_pView[1] };
	// If focus is owned by a detail view, then operate on detail views
	if (pTextView != pActiveView)
	{
		pView[0] = m_pDetailView[0];
		pView[1] = m_pDetailView[1];
	}
	HighlightDiffRect(pView[0], line, wd.start[0], wd.end[0]);
	HighlightDiffRect(pView[1], line, wd.start[1], wd.end[1]);
}

/**
 * @brief Find words to highlight in both views (to show differences in current line)
 */
int CChildFrame::Computelinediff(CGhostTextView *pView, wdiff &wd, bool reverse)
{
	POINT const ptCursor = pView->GetCursorPos();
	POINT ptStart, ptEnd;
	pView->GetSelection(ptStart, ptEnd);

	// If selection spans multiple lines, collapse towards cursor position
	if (ptStart.y != ptEnd.y)
		(ptCursor.y != ptStart.y ? ptStart : ptEnd) = ptCursor;

	vector<wdiff> worddiffs;
	GetWordDiffArray(ptCursor.y, worddiffs);

	if (worddiffs.empty())
		return -1; // Signal to caller that there was no diff

	wdiff const &f = worddiffs.front();
	wdiff const &b = worddiffs.back();

	int whichdiff = -2;
	int const ind = pView->m_nThisPane;
	if (ptStart.x == f.start[ind] && ptEnd.x == b.end[ind])
	{
		// Highlight either first or last diff
		whichdiff = reverse ? static_cast<int>(worddiffs.size()) - 1 : 0;
	}
	else if (reverse)
	{
		// Highlight previous diff
		vector<wdiff>::const_reverse_iterator const found = std::find_if(
			worddiffs.rbegin(), worddiffs.rend(), wdiff::prev_to(ind, ptStart.x));
		if (found != worddiffs.rend())
			whichdiff = static_cast<int>(found.base() - worddiffs.begin()) - 1;
	}
	else
	{
		// Highlight next diff
		vector<wdiff>::const_iterator const found = std::find_if(
			worddiffs.begin(), worddiffs.end(), wdiff::next_to(ind, ptEnd.x));
		if (found != worddiffs.end())
			whichdiff = static_cast<int>(found - worddiffs.begin());
	}

	if (whichdiff == -2)
	{
		// Highlight text between start of first and end of last diff
		wd.start[0] = f.start[0];
		wd.start[1] = f.start[1];
		wd.end[0] = b.end[0];
		wd.end[1] = b.end[1];
	}
	else
	{
		// Highlight one particular diff
		wd = worddiffs[whichdiff];
	}

	return ptCursor.y;
}

/**
 * @brief Return array of differences in specified line
 * This is used by algorithm for line diff coloring
 * (Line diff coloring is distinct from the selection highlight code)
 */
void CChildFrame::GetWordDiffArray(int nLineIndex, vector<wdiff> &worddiffs) const
{
	if (nLineIndex >= m_pView[0]->GetLineCount())
		return;
	if (nLineIndex >= m_pView[1]->GetLineCount())
		return;

	// We truncate diffs to remain inside line (ie, to not flag eol characters)
	int const len1 = m_pView[0]->GetLineLength(nLineIndex);
	LPCTSTR const str1 = m_pView[0]->GetLineChars(nLineIndex);
	int const len2 = m_pView[1]->GetLineLength(nLineIndex);
	LPCTSTR const str2 = m_pView[1]->GetLineChars(nLineIndex);

	// Options that affect comparison
	bool const casitive = !m_diffWrapper.bIgnoreCase;
	int const xwhite = m_diffWrapper.nIgnoreWhitespace;
	int const breakType = COptionsMgr::Get(OPT_BREAK_TYPE);
	bool const byteColoring = COptionsMgr::Get(OPT_CHAR_LEVEL);

	// Make the call to stringdiffs, which does all the hard & tedious computations
	sd_ComputeWordDiffs(str1, len1, str2, len2, casitive, xwhite, breakType, byteColoring, worddiffs);
	// Add a diff in case of EOL difference
	if (!m_diffWrapper.bIgnoreEol && IsLineMixedEOL(nLineIndex))
	{
		worddiffs.push_back(wdiff(len1, len1, len2, len2));
	}
}

bool CChildFrame::IsLineMixedEOL(int nLineIndex) const
{
	LPCTSTR p = m_pView[0]->GetTextBufferEol(nLineIndex);
	LPCTSTR q = m_pView[1]->GetTextBufferEol(nLineIndex);
	return *p != _T('\0') && *q != _T('\0') && _tcscmp(p, q) != 0;
}
