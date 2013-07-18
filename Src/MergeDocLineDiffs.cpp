/** 
 * @file  MergeDocLineDiffs.cpp
 *
 * @brief Implementation file for word diff highlighting (F4) for merge edit & detail views
 *
 */
// RCS ID line follows -- this is updated by CVS
// $Id$

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

using stl::vector;

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
void CChildFrame::Showlinediff(CCrystalTextView *pTextView, CMergeEditView *pActiveView, DIFFLEVEL difflvl)
{
	CCrystalTextView *pView[] = { m_pView[0], m_pView[1] };
	// If focus is owned by a detail view, then operate on detail views
	if (pTextView != pActiveView)
	{
		pView[0] = m_pDetailView[0];
		pView[1] = m_pDetailView[1];
	}
	RECT rc1, rc2;
	Computelinediff(pView[0], pView[1], pTextView->GetCursorPos().y, rc1, rc2, difflvl);

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
static void SetLineHighlightRect(int begin, int end, int line, int width, RECT &rc)
{
	if (begin == -1)
	{
		begin = end = 0;
	}
	else
	{
		++end; // MergeDoc needs to point past end
	}
	// Chop off any reference to eol characters
	// TODO: Are we dropping the last non-eol character,
	// because MergeDoc points past the end ?
	rc.left = begin <= width ? begin : width;
	rc.right = end <= width ? end : width;
	rc.top = rc.bottom = line;
}

/**
 * @brief Construct the highlight rectangles for diff # whichdiff
 */
static void ComputeHighlightRects(const vector<wdiff> & worddiffs, int whichdiff, int line, int width1, int width2, RECT &rc1, RECT &rc2)
{
	ASSERT(whichdiff >= 0 && whichdiff < static_cast<int>(worddiffs.size()));
	const wdiff &diff = worddiffs[whichdiff];
	SetLineHighlightRect(diff.start[0], diff.end[0], line, width1, rc1);
	SetLineHighlightRect(diff.start[1], diff.end[1], line, width2, rc2);
}

/**
 * @brief Returns rectangles to highlight in both views (to show differences in line specified)
 */
void CChildFrame::Computelinediff(CCrystalTextView *pView1, CCrystalTextView *pView2, int line, RECT &rc1, RECT &rc2, DIFFLEVEL difflvl)
{
	// Local statics are used so we can cycle through diffs in one line
	// We store previous state, both to find next state, and to verify
	// that nothing has changed (else, we reset the cycle)
	static CCrystalTextView *lastView = NULL;
	static int lastLine = -1;
	static RECT lastRc1, lastRc2;
	static int whichdiff = -2; // last diff highlighted (-2==none, -1=whole line)

	// Only remember place in cycle if same line and same view
	if (lastView != pView1 || lastLine != line)
	{
		lastView = pView1;
		lastLine = line;
		whichdiff = -2; // reset to not in cycle
	}

	// We truncate diffs to remain inside line (ie, to not flag eol characters)
	const int width1 = m_pView[0]->GetLineLength(line);
	const int width2 = m_pView[1]->GetLineLength(line);

	String str1(m_pView[0]->GetLineChars(line), width1);
	String str2(m_pView[1]->GetLineChars(line), width2);

	// Options that affect comparison
	bool casitive = !m_diffWrapper.bIgnoreCase;
	int xwhite = m_diffWrapper.nIgnoreWhitespace;

	// Make the call to stringdiffs, which does all the hard & tedious computations
	vector<wdiff> worddiffs;
	int breakType = GetBreakType();
	sd_ComputeWordDiffs(str1, str2, casitive, xwhite, breakType, difflvl == BYTEDIFF, worddiffs);
	//Add a diff in case of EOL difference
	if (!m_diffWrapper.bIgnoreEol)
	{
		if (_tcscmp(pView1->GetTextBufferEol(line), pView2->GetTextBufferEol(line)))
		{
			worddiffs.push_back(wdiff(width1, width1, width2, width2));
		}
	}
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
		ComputeHighlightRects(worddiffs, whichdiff, line, width1, width2, rc1x, rc2x);
		if (rc1x != lastRc1 || rc2x != lastRc2)
		{
			// Something has changed, reset cycle
			whichdiff = -2;
		}
	}

	if (whichdiff == -2)
	{
		int begin1 = -1, end1 = -1, begin2 = -1, end2 = -1;
		// Find starting locations for both sides
		// Have to look for first valid starting location for each side
		vector<wdiff>::const_iterator it = worddiffs.begin();
		while (it != worddiffs.end())
		{
			//const wdiff * diff = *it;
			if (begin1 == -1 && it->start[0] != -1)
				begin1 = it->start[0];
			if (begin2 == -1 && it->start[1] != -1)
				begin2 = it->start[1];
			if (begin1 != -1 && begin2 != -1)
				break; // found both
			++it;
		}
		// Find ending locations for both sides
		// Have to look for last valid starting location for each side
		if (worddiffs.size() > 1)
		{
			vector<wdiff>::const_iterator it = worddiffs.end();
			do
			{
				--it;
				if (end1 == -1 && it->end[0] != -1)
					end1 = it->end[0];
				if (end2 == -1 && it->end[1] != -1)
					end2 = it->end[1];
				if (end1 != -1 && end2 != -1)
					break; // found both
			} while (it != worddiffs.begin());
		}
		SetLineHighlightRect(begin1, end1, line, width1, rc1);
		SetLineHighlightRect(begin2, end2, line, width2, rc2);
		whichdiff = -1;
	}
	else
	{
		// Advance to next diff (and earlier we checked for running off the end)
		++whichdiff;
		ASSERT(whichdiff < static_cast<int>(worddiffs.size()));

		// highlight one particular diff
		ComputeHighlightRects(worddiffs, whichdiff, line, width1, width2, rc1, rc2);
		lastRc1 = rc1;
		lastRc2 = rc2;
	}
}

/**
 * @brief Return array of differences in specified line
 * This is used by algorithm for line diff coloring
 * (Line diff coloring is distinct from the selection highlight code)
 */
void CChildFrame::GetWordDiffArray(int nLineIndex, vector<wdiff> &worddiffs)
{
	if (nLineIndex >= m_pView[0]->GetLineCount())
		return;
	if (nLineIndex >= m_pView[1]->GetLineCount())
		return;

	int i1 = m_pView[0]->GetLineLength(nLineIndex);
	int i2 = m_pView[1]->GetLineLength(nLineIndex);
	String str1(m_pView[0]->GetLineChars(nLineIndex), i1);
	String str2(m_pView[1]->GetLineChars(nLineIndex), i2);

	// Options that affect comparison
	const bool casitive = !m_diffWrapper.bIgnoreCase;
	const int xwhite = m_diffWrapper.nIgnoreWhitespace;
	const int breakType = GetBreakType(); // whitespace only or include punctuation
	const bool byteColoring = GetByteColoringOption();

	// Make the call to stringdiffs, which does all the hard & tedious computations
	sd_ComputeWordDiffs(str1, str2, casitive, xwhite, breakType, byteColoring, worddiffs);
	//Add a diff in case of EOL difference
	if (!m_diffWrapper.bIgnoreEol)
	{
		if (_tcscmp(m_pView[0]->GetTextBufferEol(nLineIndex), m_pView[1]->GetTextBufferEol(nLineIndex)))
		{
			worddiffs.push_back(wdiff(i1, i1, i2, i2));
		}
	}
}
