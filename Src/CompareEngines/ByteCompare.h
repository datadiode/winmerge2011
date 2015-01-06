/**
 * @file  ByteCompare.h
 *
 * @brief Declaration file for ByteCompare
 */
#pragma once

#include "FileTextStats.h"

struct FileLocation;
struct file_data;

namespace CompareEngines
{

/**
 * @brief A quick compare -compare method implementation class.
 * This compare method compares files in small blocks. Code assumes block size
 * is in range of 32-bit int-type.
 */
class ByteCompare : public DIFFOPTIONS
{
public:
	ByteCompare(const CDiffContext *);
	void SetFileData(int items, file_data *data);
	unsigned CompareFiles(FileLocation *location);

	FileTextStats m_textStats[2];

private:
	template<class CodePoint>
	BOOL IsDBCSLeadByteEx(int, CodePoint)
	{
		return FALSE;
	}
	template<class CodePoint, int CodeShift>
	unsigned CompareFiles(FileLocation *, stl_size_t x = 1, stl_size_t j = 0);
	const CDiffContext *const m_pCtxt;
	HANDLE m_osfhandle[2];
	__int64 m_st_size[2];
};

} // namespace CompareEngines
