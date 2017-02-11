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
static void HighlightDiffRect(CCrystalTextView * pView, const RECT &rc)
{
	if (rc.top == -1)
	{
		// Should we remove existing selection ?
	}
	else
	{
		// select the area
		// with anchor at left and caret at right
		// this seems to be conventional behavior in Windows editors
		POINT ptTopLeft = { rc.left, rc.top };
		POINT ptBottomRight = { rc.right, rc.bottom };
		pView->SetSelection(ptTopLeft, ptBottomRight);
		pView->SetCursorPos(ptBottomRight);
		pView->SetAnchor(ptTopLeft);
		// try to ensure that selected area is visible
		pView->EnsureSelectionVisible();
	}
}

/**
 * @brief Highlight difference in current line (left & right panes)
 */
void CChildFrame::Showlinediff(CCrystalTextView *pTextView, CMergeEditView *pActiveView)
{
	CCrystalTextView *pView[] = { m_pView[0], m_pView[1] };
	// If focus is owned by a detail view, then operate on detail views
	if (pTextView != pActiveView)
	{
		pView[0] = m_pDetailView[0];
		pView[1] = m_pDetailView[1];
	}
	RECT rc1, rc2;
	Computelinediff(pView[0], pView[1], pTextView->GetCursorPos().y, rc1, rc2);

	if (rc1.top == -1 && rc2.top == -1)
	{
		String caption = LanguageSelect.LoadString(IDS_LINEDIFF_NODIFF_CAPTION);
		String msg = LanguageSelect.LoadString(IDS_LINEDIFF_NODIFF);
		pTextView->MessageBox(msg.c_str(), caption.c_str(), MB_OK);
		return;
	}

	// Actually display selection areas on screen in both edit panels
	HighlightDiffRect(pView[0], rc1);
	HighlightDiffRect(pView[1], rc2);
}

/**
 * @brief Set highlight rectangle for a given difference (begin->end in line)
 */
static void SetLineHighlightRect(int begin, int end, int line, RECT &rc)
{
	rc.left = begin;
	rc.right = end;
	rc.top = rc.bottom = line;
}

/**
 * @brief Construct the highlight rectangles for diff # whichdiff
 */
static void ComputeHighlightRects(const vector<wdiff> & worddiffs, int whichdiff, int line, RECT &rc1, RECT &rc2)
{
	ASSERT(whichdiff >= 0 && whichdiff < static_cast<int>(worddiffs.size()));
	const wdiff &diff = worddiffs[whichdiff];
	SetLineHighlightRect(diff.start[0], diff.end[0], line, rc1);
	SetLineHighlightRect(diff.start[1], diff.end[1], line, rc2);
}

/**
 * @brief Returns rectangles to highlight in both views (to show differences in line specified)
 */
void CChildFrame::Computelinediff(CCrystalTextView *pView1, CCrystalTextView *pView2, int nLineIndex, RECT &rc1, RECT &rc2)
{
	// Local statics are used so we can cycle through diffs in one line
	// We store previous state, both to find next state, and to verify
	// that nothing has changed (else, we reset the cycle)
	static CCrystalTextView *lastView = NULL;
	static int lastLine = -1;
	static RECT lastRc1, lastRc2;
	static int whichdiff = -2; // last diff highlighted (-2==none, -1=whole line)

	// Only remember place in cycle if same line and same view
	if (lastView != pView1 || lastLine != nLineIndex)
	{
		lastView = pView1;
		lastLine = nLineIndex;
		whichdiff = -2; // reset to not in cycle
	}

	vector<wdiff> worddiffs;
	GetWordDiffArray(nLineIndex, worddiffs);

	if (worddiffs.empty())
	{
		// signal to caller that there was no diff
		rc1.top = -1;
		rc2.top = -1;
		return;
	}

	// Are we continuing a cycle from the same place ?
	if (whichdiff >= (int)worddiffs.size())
		whichdiff = -2; // Clearly not continuing the same cycle, reset to not in cycle
	
	// After last diff, reset to get full line again
	if (whichdiff == worddiffs.size() - 1)
		whichdiff = -2;

	// Check if current line has changed enough to reset cycle
	if (whichdiff >= 0)
	{
		// Recompute previous highlight rectangle
		RECT rc1x, rc2x;
		ComputeHighlightRects(worddiffs, whichdiff, nLineIndex, rc1x, rc2x);
		if (rc1x != lastRc1 || rc2x != lastRc2)
		{
			// Something has changed, reset cycle
			whichdiff = -2;
		}
	}

	if (whichdiff == -2)
	{
		// Highlight text between start of first and end of last diff
		wdiff const &f = worddiffs.front();
		wdiff const &b = worddiffs.back();
		SetLineHighlightRect(f.start[0], b.end[0], nLineIndex, rc1);
		SetLineHighlightRect(f.start[1], b.end[1], nLineIndex, rc2);
		whichdiff = -1;
	}
	else
	{
		// Advance to next diff (and earlier we checked for running off the end)
		++whichdiff;
		ASSERT(whichdiff < static_cast<int>(worddiffs.size()));

		// Highlight one particular diff
		ComputeHighlightRects(worddiffs, whichdiff, nLineIndex, rc1, rc2);
		lastRc1 = rc1;
		lastRc2 = rc2;
	}
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
