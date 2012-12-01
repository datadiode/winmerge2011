/**
 * @file  TimeSizeCompare.cpp
 *
 * @brief Implementation file for TimeSizeCompare
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "DiffItem.h"
#include "DiffWrapper.h"
#include "DiffContext.h"
#include "TimeSizeCompare.h"

namespace CompareEngines
{

TimeSizeCompare::TimeSizeCompare(const CDiffContext *pCtxt)
	: m_pCtxt(pCtxt)
{
}

/**
 * @brief Compare two specified files, byte-by-byte
 * @param [in] compMethod Compare method used.
 * @param [in] di Diffitem info.
 * @return DIFFCODE
 */
int TimeSizeCompare::CompareFiles(int compMethod, const DIFFITEM *di)
{
	UINT code = DIFFCODE::SAME;
	if ((compMethod == CMP_DATE) || (compMethod == CMP_DATE_SIZE))
	{
		// Compare by modified date
		// Check that we have both filetimes
		if (di->left.mtime != 0 && di->right.mtime != 0)
		{
			UINT64 lower = min(di->left.mtime.castTo<UINT64>(), di->right.mtime.castTo<UINT64>());
			UINT64 upper = max(di->left.mtime.castTo<UINT64>(), di->right.mtime.castTo<UINT64>());
			UINT64 tolerance = m_pCtxt->m_bIgnoreSmallTimeDiff ? SmallTimeDiff * FileTime::TicksPerSecond : 0;
			if (upper - lower <= tolerance)
				code = DIFFCODE::SAME;
			else
				code = DIFFCODE::DIFF;
		}
		else
		{
			// Filetimes for item(s) could not be read. So we have to
			// set error status, unless we have DATE_SIZE -compare
			// when we have still hope for size compare..
			if (compMethod == CMP_DATE_SIZE)
				code = DIFFCODE::SAME;
			else
				code = DIFFCODE::CMPERR;
		}
	}
	// This is actual CMP_SIZE method..
	// If file sizes differ mark them different
	if ((compMethod == CMP_DATE_SIZE) || (compMethod == CMP_SIZE))
	{
		if (di->left.size.int64 != di->right.size.int64)
		{
			code = DIFFCODE::DIFF;
		}
	}
	return code;
}

} // namespace CompareEngines
