/** 
 * @file  CompareStats.cpp
 *
 * @brief Implementation of CompareStats class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "DiffItem.h"
#include "CompareStats.h"

/** 
 * @brief Constructor, initializes critical section.
 */
CompareStats::CompareStats()
{
	Reset();
}

/** 
 * @brief Destructor, deletes critical section.
 */
CompareStats::~CompareStats()
{
}

/** 
 * @brief Add compared item.
 * @param [in] code Resultcode to add.
 */
void CompareStats::AddItem(UINT code)
{
	RESULT res = GetColImage(code);
	InterlockedIncrement(&m_counts[res]);
	InterlockedIncrement(&m_nComparedItems);
	assert(m_nComparedItems <= m_nTotalItems);
}

/** 
 * @brief Return count by resultcode.
 * @param [in] result Resultcode to return.
 * @return Count of items for given resultcode.
 */
int CompareStats::GetCount(CompareStats::RESULT result) const
{
	return m_counts[result];
}

/** 
 * @brief Reset comparestats.
 * Use this function to reset stats before new compare.
 */
void CompareStats::Reset()
{
	m_nTotalItems = 0;
	m_nComparedItems = 0;
	ZeroMemory(m_counts, sizeof m_counts);
}

/**
 * @brief Return image index appropriate for this row
 */
CompareStats::RESULT CompareStats::GetColImage(const DIFFCODE &diffcode)
{
	// Must return an image index into image list created above in OnInitDialog
	if (diffcode.isResultError())
		return DIFFIMG_ERROR;
	if (diffcode.isResultAbort())
		return DIFFIMG_ABORT;
	if (diffcode.isResultFiltered())
		return diffcode.isDirectory() ? DIFFIMG_DIRSKIP : DIFFIMG_SKIP;
	if (diffcode.isSideLeftOnly())
		return diffcode.isDirectory() ? DIFFIMG_LDIRUNIQUE : DIFFIMG_LUNIQUE;
	if (diffcode.isSideRightOnly())
		return diffcode.isDirectory() ? DIFFIMG_RDIRUNIQUE : DIFFIMG_RUNIQUE;
	if (diffcode.isResultSame())
	{
		if (diffcode.isDirectory())
			return DIFFIMG_DIRSAME;
		if (diffcode.isText())
			return DIFFIMG_TEXTSAME;
		if (diffcode.isBin())
			return DIFFIMG_BINSAME;
		return DIFFIMG_SAME;
	}
	// diff
	if (diffcode.isResultDiff())
	{
		if (diffcode.isDirectory())
			return DIFFIMG_DIRDIFF;
		if (diffcode.isText())
			return DIFFIMG_TEXTDIFF;
		if (diffcode.isBin())
			return DIFFIMG_BINDIFF;
		return DIFFIMG_DIFF;
	}
	return diffcode.isDirectory() ? DIFFIMG_DIR : DIFFIMG_ABORT;
}
