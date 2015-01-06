/**
 * @file  TimeSizeCompare.h
 *
 * @brief Declaration file for TimeSizeCompare compare engine.
 */
#pragma once

struct DIFFITEM;

namespace CompareEngines
{

/**
 * @brief A time/size compare class.
 * This compare method compares files by their times and sizes.
 */
class TimeSizeCompare
{
public:
	TimeSizeCompare(const CDiffContext *);
	int CompareFiles(int compMethod, const DIFFITEM *di);

private:
	const CDiffContext *const m_pCtxt;
};

} // namespace CompareEngines
