/**
 * @file DiffFileData.h
 *
 * @brief Declaration for DiffFileData class.
 *
 * @date  Created: 2003-08-22
 */
#pragma once

#include "Diff.h"
#include "editlib/TextBlock.h"

class CDiffWrapper;

/**
 * @brief C++ container for the structure (file_data) used by diffutils' diff_2_files(...)
 */
class DiffFileData
	: ZeroInit<DiffFileData>
	, public comparison
{
public:
	explicit DiffFileData(int codepage);
	~DiffFileData();

	bool OpenFiles(LPCTSTR szFilepath1, LPCTSTR szFilepath2);
	HANDLE GetFileHandle(int i);
	void *AllocBuffer(int i, size_t len, size_t alloc_extra);
	void Reset();
	void SetDisplayFilepaths(LPCTSTR szTrueFilepath1, LPCTSTR szTrueFilepath2);

	CDiffWrapper *m_pDiffWrapper;
	bool m_used; // whether m_inf has real data
	FileLocation m_FileLocation[2];
	FileTextStats m_textStats[2];
	String m_sDisplayFilepath[2];
	TextBlock::Cookie cookie[2];
	int parsed[2];

private:
	bool DoOpenFiles();
	DiffFileData(const DiffFileData &); // disallow copy construction
	void operator=(const DiffFileData &); // disallow assignment
};
