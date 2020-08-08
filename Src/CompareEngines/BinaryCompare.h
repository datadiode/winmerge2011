/**
 * @file  BinaryCompare.h
 *
 * @brief Declaration file BinaryCompare
 */
#pragma once

struct FileLocation;
struct file_data;

namespace CompareEngines
{

/**
 * @brief A binary compare class.
 * This compare method compares files by their binary contents.
 */
class BinaryCompare : public DIFFOPTIONS
{
public:
	explicit BinaryCompare(const CDiffContext *);
	void SetFileData(int items, file_data *data);
	unsigned CompareFiles();
private:
	const CDiffContext *const m_pCtxt;
	HANDLE m_osfhandle[2];
	__int64 m_st_size[2];
};

} // namespace CompareEngines
