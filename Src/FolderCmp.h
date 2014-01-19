/** 
 * @file  FolderCmp.h
 *
 * @brief Declaration file for FolderCmp
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _FOLDERCMP_H_
#define _FOLDERCMP_H_

#include "DiffFileData.h"
#include "DiffUtils.h"
#include "ByteCompare.h"
#include "TimeSizeCompare.h"

class CDiffContext;
class PackingInfo;

/**
 * @brief Class implementing file compare for folder compare.
 * This class implements (called from DirScan.cpp) compare of two files
 * during folder compare. The class implements both diffutils compare and
 * quick compare.
 */
class FolderCmp
{
public:
	FolderCmp(CDiffContext *, LONG iCompareThread = -1);
	~FolderCmp();
	UINT prepAndCompareTwoFiles(DIFFITEM *);

	int m_ndiffs;
	int m_ntrivialdiffs;
	const LONG m_iCompareThread;

	DiffFileData m_diffFileData;
	
private:
	void GetComparePaths(const DIFFITEM *di, String &left, String &right) const;

	CDiffContext *const m_pCtx;
	CompareEngines::DiffUtils *m_pDiffUtilsEngine;
	CompareEngines::ByteCompare *m_pByteCompare;
	CompareEngines::TimeSizeCompare *m_pTimeSizeCompare;
};


#endif // _FOLDERCMP_H_
