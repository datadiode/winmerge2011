/**
 * @file  ByteCompare.h
 *
 * @brief Declaration file for ByteCompare
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _BYTE_COMPARE_H_
#define _BYTE_COMPARE_H_

#include "FileTextStats.h"

class CompareOptions;
class QuickCompareOptions;
class IAbortable;
struct FileLocation;
struct file_data;

namespace CompareEngines
{

/**
 * @brief A quick compare -compare method implementation class.
 * This compare method compares files in small blocks. Code assumes block size
 * is in range of 32-bit int-type.
 */
class ByteCompare
{
public:
	ByteCompare(const QuickCompareOptions &);
	~ByteCompare();

	void SetAdditionalOptions(bool stopAfterFirstDiff);
	void SetAbortable(const IAbortable *piAbortable);
	void SetFileData(int items, file_data *data);
	unsigned CompareFiles(FileLocation *location);
	void GetTextStats(int side, FileTextStats *stats);

private:
	template<class CodePoint, int CodeShift>
	unsigned CompareFiles(size_t x = 1, size_t j = 0);
	QuickCompareOptions m_options; /**< Compare options for diffutils. */
	const IAbortable *m_piAbortable;
	FileTextStats m_textStats[2];
	HANDLE m_osfhandle[2];
};

} // namespace CompareEngines

#endif // _BYTE_COMPARE_H_
