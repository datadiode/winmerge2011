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
void CompareStats::AddItem(int code)
{
	RESULT res = GetResultFromCode(code);
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
 * @brief Convert diffcode to compare-result.
 * @param [in] diffcode DIFFITEM.diffcode to convert.
 * @return Compare result.
 */
CompareStats::RESULT CompareStats::GetResultFromCode(UINT diffcode)
{
	DIFFCODE di = diffcode;
	
	// Test first for skipped so we pick all skipped items as such 
	if (di.isResultFiltered())
	{
		// skipped
		if (di.isDirectory())
		{
			return RESULT_DIRSKIP;
		}
		else
		{
			return RESULT_SKIP;
		}
	}
	else if (di.isSideLeftOnly())
	{
		// left-only
		if (di.isDirectory())
		{
			return RESULT_LDIRUNIQUE;
		}
		else
		{
			return RESULT_LUNIQUE;
		}
	}
	else if (di.isSideRightOnly())
	{
		// right-only
		if (di.isDirectory())
		{
			return RESULT_RDIRUNIQUE;
		}
		else
		{
			return RESULT_RUNIQUE;
		}
	}
	else if (di.isResultError())
	{
		// could be directory error ?
		return RESULT_ERROR;
	}
	// Now we know it was on both sides & compared!
	else if (di.isResultSame())
	{
		// same
		if (di.isBin())
		{
			return RESULT_BINSAME;
		}
		else
		{
			return RESULT_SAME;
		}
	}
	else
	{
		// presumably it is diff
		if (di.isDirectory())
		{
			return RESULT_DIR;
		}
		else
		{
			if (di.isBin())
			{
				return RESULT_BINDIFF;
			}
			else
			{
				return RESULT_DIFF;
			}
		}
	}
}
