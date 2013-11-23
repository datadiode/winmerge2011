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
		if (m_diffs[i].diffrange.op != OP_TRIVIAL)
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
	return m_diffs[nDiff].diffrange.op != OP_TRIVIAL;
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
		if (m_diffs[i].diffrange.op != OP_TRIVIAL)
		{
			++significants;
		}
	}
	return significants;
}

/**
 * @brief Return constant pointer to requested diff.
 * This function returns constant pointer to DIFFRANGE at given index.
 * @param [in] nDiff Index of DIFFRANGE to return.
 * @return Constant pointer to DIFFRANGE.
 */
DIFFRANGE *DiffList::DiffRangeAt(int nDiff)
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
 * @brief Checks if line is inside given diff
 * @param [in] nLine Linenumber to text buffer (not "real" number)
 * @param [in] nDiff Index to diff table
 * @return TRUE if line is inside given difference.
 */
bool DiffList::LineInDiff(UINT nLine, int nDiff) const
{
	const DIFFRANGE &dr = m_diffs[nDiff].diffrange;
	return nLine >= dr.dbegin0 && nLine <= dr.dend0;
}

/**
 * @brief Returns diff index for given line.
 * @param [in] nLine Linenumber, 0-based.
 * @return Index to diff table, -1 if line is not inside any diff.
 */
int DiffList::LineToDiff(UINT nLine) const
{
	// Use binary search to search for a diff.
	int left = 0; // Left limit
	int right = GetSize() - 1; // Right limit
	while (left <= right)
	{
		int middle = (left + right) / 2;
		const DIFFRANGE &dr = m_diffs[middle].diffrange;
		if (nLine < dr.dbegin0) // Line is before diff in file
			right = middle - 1;
		else if (nLine > dr.dend0) // Line is after diff in file
			left = middle + 1;
		else // Line is in diff
			return middle;
	}
	return -1;
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
	int i = nDiffCount;
	while (i > 0)
	{
		--i;
		const DIFFRANGE &dr = m_diffs[i].diffrange;
		if (dr.op != OP_TRIVIAL && dr.dend0 < nLine)
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
	int i = 0;
	while (i < nDiffCount)
	{
		const DIFFRANGE &dr = m_diffs[i].diffrange;
		if (dr.op != OP_TRIVIAL && dr.dbegin0 > nLine)
		{
			nDiff = i;
			break;
		}
		++i;
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
	return m_diffs[nDiff].prev;
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
		// TODO: This assert has been observed to fire with a binary file misdetected as UCS2-LE
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

int DiffList::FinishSyncPoint(int nDiff, int nRealStartLine[])
{
	const int nDiffCount = GetSize();
	while (nDiff < nDiffCount)
	{
		DIFFRANGE &di = m_diffs[nDiff++].diffrange;
		di.begin0 += nRealStartLine[0];
		di.end0 += nRealStartLine[0];
		di.begin1 += nRealStartLine[1];
		di.end1 += nRealStartLine[1];
	}
	return nDiff;
}

void DiffList::swap(DiffList &other)
{
	stl::swap(m_firstSignificant, other.m_firstSignificant);
	stl::swap(m_lastSignificant, other.m_lastSignificant);
	m_diffs.swap(other.m_diffs);
}
