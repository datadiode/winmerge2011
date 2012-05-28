/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DiffList.cpp
 *
 * @brief Implementation file for DiffList class
 */
// ID line follows -- this is updated by SVN
// $Id$
#include "StdAfx.h"
#include "coretools.h"
#include "DiffList.h"
#include "DiffTextBuffer.h"
#include "MergeLineFlags.h"

/**
 * @brief Swap diff sides.
 */
void DIFFRANGE::swap_sides()
{
	stl::swap(begin0, begin1);
	stl::swap(end0, end1);
	stl::swap(dbegin0, dbegin1);
	stl::swap(dend0, dend1);
	stl::swap(blank0, blank1);
}

/**
 * @brief Default constructor, initialises difflist to 64 items.
 */
DiffList::DiffList()
: m_firstSignificant(-1)
, m_lastSignificant(-1)
{
	m_diffs.reserve(64); // Reserve some initial space to avoid allocations.
}

/**
 * @brief Removes all diffs from list.
 */
void DiffList::Clear()
{
	m_diffs.clear();
	m_firstSignificant = -1;
	m_lastSignificant = -1;
}

/**
 * @brief Returns count of items in diff list.
 * This function returns total amount of items (diffs) in list. So returned
 * count includes significant and non-significant diffs.
 * @note Use GetSignificantDiffs() to get count of non-ignored diffs.
 */
int DiffList::GetSize() const
{
	return static_cast<int>(m_diffs.size());
}

/**
 * @brief Returns count of significant diffs in the list.
 * This function returns total count of significant diffs in list. So returned
 * count doesn't include non-significant diffs.
 * @return Count of significant differences.
 */
int DiffList::GetSignificantDiffs() const
{
	int nSignificants = 0;
	const int nDiffCount = GetSize();

	for (int i = 0; i < nDiffCount; i++)
	{
		const DIFFRANGE *dfi = DiffRangeAt(i);
		if (dfi->op != OP_TRIVIAL)
		{
			++nSignificants;
		}
	}
	return nSignificants;
}

/**
 * @brief Adds given diff to end of the list.
 * Adds given difference to end of the list (append).
 * @param [in] di Difference to add.
 */
void DiffList::AddDiff(const DIFFRANGE & di)
{
	// Allocate memory for new items exponentially
	if (m_diffs.size() == m_diffs.capacity())
		m_diffs.reserve(m_diffs.size() * 2); // 0x7FFF0000
	m_diffs.push_back(di);
}

/**
 * @brief Checks if diff in given list index is significant or not.
 * @param [in] nDiff Index of DIFFRANGE to check.
 * @return TRUE if diff is significant, FALSE if not.
 */
bool DiffList::IsDiffSignificant(int nDiff) const
{
	const DIFFRANGE *dfi = DiffRangeAt(nDiff);
	return dfi->op != OP_TRIVIAL;
}

/**
 * @brief Get significant difference index of the diff.
 * This function returns the index of diff when only significant differences
 * are calculated.
 * @param [in] nDiff Index of difference to check.
 * @return Significant difference index of the diff.
 */
int DiffList::GetSignificantIndex(int nDiff) const
{
	int significants = -1;

	for (int i = 0; i <= nDiff; i++)
	{
		const DIFFRANGE *dfi = DiffRangeAt(i);
		if (dfi->op != OP_TRIVIAL)
		{
			++significants;
		}
	}
	return significants;
}

/**
 * @brief Returns copy of DIFFRANGE from diff-list.
 * @param [in] nDiff Index of DIFFRANGE to return.
 * @param [out] di DIFFRANGE returned (empty if error)
 * @return TRUE if DIFFRANGE found from given index.
 */
bool DiffList::GetDiff(int nDiff, DIFFRANGE & di) const
{
	const DIFFRANGE *dfi = DiffRangeAt(nDiff);
	if (!dfi)
	{
		DIFFRANGE empty;
		di = empty;
		return false;
	}
	di = *dfi;
	return true;
}

/**
 * @brief Return constant pointer to requested diff.
 * This function returns constant pointer to DIFFRANGE at given index.
 * @param [in] nDiff Index of DIFFRANGE to return.
 * @return Constant pointer to DIFFRANGE.
 */
const DIFFRANGE *DiffList::DiffRangeAt(int nDiff) const
{
	if (nDiff >= 0 && nDiff < GetSize())
	{
		return &m_diffs[nDiff].diffrange;
	}
	else
	{
		assert(0);
		return NULL;
	}
}

/**
 * @brief Replaces diff in list in given index with given diff.
 * @param [in] nDiff Index (0-based) of diff to be replaced
 * @param [in] di Diff to put in list.
 * @return TRUE if index was valid and diff put to list.
 */
bool DiffList::SetDiff(int nDiff, const DIFFRANGE & di)
{
	if (nDiff < GetSize())
	{
		m_diffs[nDiff].diffrange = di;
		return true;
	}
	else
		return false;
}

/**
 * @brief Checks if line is before, inside or after diff
 * @param [in] nLine Linenumber to text buffer (not "real" number)
 * @param [in] nDiff Index to diff table
 * @return -1 if line is before diff, 0 if line is in diff and
 * 1 if line is after diff.
 */
int DiffList::LineRelDiff(UINT nLine, int nDiff) const
{
	const DIFFRANGE *dfi = DiffRangeAt(nDiff);
	if (nLine < dfi->dbegin0)
		return -1;
	else if (nLine > dfi->dend0)
		return 1;
	else
		return 0;
}

/**
 * @brief Checks if line is inside given diff
 * @param [in] nLine Linenumber to text buffer (not "real" number)
 * @param [in] nDiff Index to diff table
 * @return TRUE if line is inside given difference.
 */
bool DiffList::LineInDiff(UINT nLine, int nDiff) const
{
	const DIFFRANGE *dfi = DiffRangeAt(nDiff);
	if (nLine >= dfi->dbegin0 && nLine <= dfi->dend0)
		return true;
	else
		return false;
}

/**
 * @brief Returns diff index for given line.
 * @param [in] nLine Linenumber, 0-based.
 * @return Index to diff table, -1 if line is not inside any diff.
 */
int DiffList::LineToDiff(UINT nLine) const
{
	const int nDiffCount = GetSize();
	if (nDiffCount == 0)
		return -1;

	// First check line is not before first or after last diff
	if (nLine < DiffRangeAt(0)->dbegin0)
		return -1;
	if (nLine > DiffRangeAt(nDiffCount - 1)->dend0)
		return -1;

	// Use binary search to search for a diff.
	int left = 0; // Left limit
	int middle = 0; // Compared item
	int right = nDiffCount - 1; // Right limit

	while (left <= right)
	{
		middle = (left + right) / 2;
		int result = LineRelDiff(nLine, middle);
		switch (result)
		{
		case -1: // Line is before diff in file
			right = middle - 1;
			break;
		case 0: // Line is in diff
			return middle;
			break;
		case 1: // Line is after diff in file
			left = middle + 1;
			break;
		default:
			_RPTF1(_CRT_ERROR, "Invalid return value %d from LineRelDiff(): "
				"-1, 0 or 1 expected!", result); 
			break;
		}
	}
	return -1;
}

/**
 * @brief Check if diff-list contains significant diffs.
 * @return TRUE if list has significant diffs, FALSE otherwise.
 */
bool DiffList::HasSignificantDiffs() const
{
	if (m_firstSignificant == -1)
		return false;
	return true;
}

/**
 * @brief Return previous diff index from given line.
 * @param [in] nLine First line searched.
 * @return Index for next difference or -1 if no difference is found.
 */
int DiffList::PrevSignificantDiffFromLine(UINT nLine) const
{
	int nDiff = -1;
	const int nDiffCount = GetSize();

	for (int i = nDiffCount - 1; i >= 0 ; i--)
	{
		const DIFFRANGE *dfi = DiffRangeAt(i);
		if (dfi->op != OP_TRIVIAL && dfi->dend0 <= nLine)
		{
			nDiff = i;
			break;
		}
	}
	return nDiff;
}

/**
 * @brief Return next diff index from given line.
 * @param [in] nLine First line searched.
 * @return Index for previous difference or -1 if no difference is found.
 */
int DiffList::NextSignificantDiffFromLine(UINT nLine) const
{
	int nDiff = -1;
	const int nDiffCount = GetSize();

	for (int i = 0; i < nDiffCount; i++)
	{
		const DIFFRANGE *dfi = DiffRangeAt(i);
		if (dfi->op != OP_TRIVIAL && dfi->dbegin0 >= nLine)
		{
			nDiff = i;
			break;
		}
	}
	return nDiff;
}

/**
 * @brief Construct the doubly-linked chain of significant differences
 */
void DiffList::ConstructSignificantChain()
{
	m_firstSignificant = -1;
	m_lastSignificant = -1;
	int prev = -1;
	const int nDiffCount = GetSize();

	// must be called after diff list is entirely populated
    for (int i = 0; i < nDiffCount; ++i)
	{
		if (m_diffs[i].diffrange.op == OP_TRIVIAL)
		{
			m_diffs[i].prev = -1;
			m_diffs[i].next = -1;
		}
		else
		{
			m_diffs[i].prev = prev;
			if (prev != -1)
				m_diffs[prev].next = i;
			prev = i;
			if (m_firstSignificant == -1)
				m_firstSignificant = i;
			m_lastSignificant = i;
		}
	}
}

/**
 * @brief Return index to first significant difference.
 * @return Index of first significant difference.
 */
int DiffList::FirstSignificantDiff() const
{
	return m_firstSignificant;
}

/**
 * @brief Return index of next significant diff.
 * @param [in] nDiff Index to start looking for next diff.
 * @return Index of next significant difference.
 */
int DiffList::NextSignificantDiff(int nDiff) const
{
	return m_diffs[nDiff].next;
}

/**
 * @brief Return index of previous significant diff.
 * @param [in] nDiff Index to start looking for previous diff.
 * @return Index of previous significant difference.
 */
int DiffList::PrevSignificantDiff(int nDiff) const
{
	return (int)m_diffs[nDiff].prev;
}

/**
 * @brief Return index to last significant diff.
 * @return Index of last significant difference.
 */
int DiffList::LastSignificantDiff() const
{
	return m_lastSignificant;
}

/**
 * @brief Return pointer to first significant diff.
 * @return Constant pointer to first significant difference.
 */
const DIFFRANGE *DiffList::FirstSignificantDiffRange() const
{
	if (m_firstSignificant == -1)
		return NULL;
	return DiffRangeAt(m_firstSignificant);
}

/**
 * @brief Return pointer to last significant diff.
 * @return Constant pointer to last significant difference.
 */
const DIFFRANGE *DiffList::LastSignificantDiffRange() const
{
	if (m_lastSignificant == -1)
		return NULL;
	return DiffRangeAt(m_lastSignificant);
}

/**
 * @brief Swap sides in diffrange.
 */
void DiffList::SwapSides()
{
	stl::vector<DiffRangeInfo>::iterator iter = m_diffs.begin();
	stl::vector<DiffRangeInfo>::const_iterator iterEnd = m_diffs.end();
	while (iter != iterEnd)
	{
		iter->diffrange.swap_sides();
		++iter;
	}
}

/**
 * @brief Count number of lines to add to sides (because of synch).
 * @param [out] nLeftLines Number of lines to add to left side.
 * @param [out] nRightLines Number of lines to add to right side.
 */
void DiffList::AddExtraLinesCounts(
	UINT &rnLeftLines, UINT &rnRightLines,
	CDiffTextBuffer **ptBuf, UINT nContextLines)
{
	stl::vector<DiffRangeInfo>::iterator iter = m_diffs.begin();
	stl::vector<DiffRangeInfo>::iterator iterEnd = m_diffs.end();
	UINT nLeftLines = 0;
	UINT nRightLines = 0;
	UINT end0 = 0;
	UINT end1 = 0;
	UINT skip0 = 0;
	UINT skip1 = 0;
	DIFFRANGE endDiff;
	bool bLimitedContext = true;
	if (nContextLines == UINT_MAX)
	{
		bLimitedContext = false;
		nContextLines = 0;
	}
	endDiff.begin0 = rnLeftLines + nContextLines;
	endDiff.begin1 = rnRightLines + nContextLines;
	for (;;)
	{
		DIFFRANGE &curDiff = iter != iterEnd ? iter++->diffrange : endDiff;
		// this guarantees that all the diffs are synchronized
		assert(curDiff.begin0 + nLeftLines == curDiff.begin1 + nRightLines);
		if (bLimitedContext)
		{
			LineInfo *pli = NULL;
			while (end0 + nContextLines < curDiff.begin0)
			{
				if (pli == NULL)
				{
					pli = &ptBuf[0]->m_aLines[end0];
					pli->Clear();
					pli->m_nSkippedLines = 1;
					pli->m_dwFlags = LF_GHOST;
				}
				else
				{
					++skip0;
					++pli->m_nSkippedLines;
					ptBuf[0]->m_aLines[end0].m_dwFlags = LF_SKIPPED;
				}
				++end0;
			}
			pli = NULL;
			while (end1 + nContextLines < curDiff.begin1)
			{
				if (pli == NULL)
				{
					pli = &ptBuf[1]->m_aLines[end1];
					pli->Clear();
					pli->m_nSkippedLines = 1;
					pli->m_dwFlags = LF_GHOST;
				}
				else
				{
					++skip1;
					++pli->m_nSkippedLines;
					ptBuf[1]->m_aLines[end1].m_dwFlags = LF_SKIPPED;
				}
				++end1;
			}
		}
		if (&curDiff == &endDiff)
			break;
		end0 = curDiff.end0 + 1 + nContextLines;
		end1 = curDiff.end1 + 1 + nContextLines;
		UINT nline0 = end0 - curDiff.begin0;
		UINT nline1 = end1 - curDiff.begin1;
		if (nline0 > nline1)
			nRightLines += nline0 - nline1;
		else
			nLeftLines += nline1 - nline0;
		curDiff.begin0 -= skip0;
		curDiff.end0 -= skip0;
		curDiff.begin1 -= skip1;
		curDiff.end1 -= skip1;
	}
	rnLeftLines -= skip0;
	rnLeftLines += nLeftLines;
	rnRightLines -= skip1;
	rnRightLines += nRightLines;
}

void DiffList::swap(DiffList &other)
{
	stl::swap(m_firstSignificant, other.m_firstSignificant);
	stl::swap(m_lastSignificant, other.m_lastSignificant);
	m_diffs.swap(other.m_diffs);
}
