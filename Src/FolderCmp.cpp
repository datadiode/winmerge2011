/** 
 * @file  FolderCmp.cpp
 *
 * @brief Implementation file for FolderCmp
 */
#include "StdAfx.h"
#include "DiffUtils.h"
#include "ByteCompare.h"
#include "LogFile.h"
#include "paths.h"
#include "DiffContext.h"
#include "DiffWrapper.h"
#include "FileTransform.h"
#include "DIFF.H"
#include "FolderCmp.h"
#include "codepage_detect.h"
#include "TimeSizeCompare.h"

/**
 * @brief Copy text stat results from diffutils back into the FileTextStats structure
 */
static void CopyTextStats(const file_data * inf, FileTextStats * myTextStats)
{
	myTextStats->ncrlfs = inf->count_crlfs;
	myTextStats->ncrs = inf->count_crs;
	myTextStats->nlfs = inf->count_lfs;
	myTextStats->nzeros = inf->count_zeros;
}

FolderCmp::FolderCmp(CDiffContext *pCtxt, LONG iCompareThread)
: m_pDiffUtilsEngine(NULL)
, m_pByteCompare(NULL)
, m_pTimeSizeCompare(NULL)
, m_ndiffs(CDiffContext::DIFFS_UNKNOWN)
, m_ntrivialdiffs(CDiffContext::DIFFS_UNKNOWN)
, m_iCompareThread(iCompareThread)
, m_pCtx(pCtxt)
, m_diffFileData(FileTextEncoding::GetDefaultCodepage())
{
}

FolderCmp::~FolderCmp()
{
	delete m_pDiffUtilsEngine;
	delete m_pByteCompare;
	delete m_pTimeSizeCompare;
}

/**
 * @brief Prepare files (run plugins) & compare them, and return diffcode.
 * This is function to compare two files in folder compare. It is not used in
 * file compare.
 * @param [in] pCtxt Pointer to compare context.
 * @param [in, out] di Compared files with associated data.
 * @return Compare result code.
 */
UINT FolderCmp::prepAndCompareTwoFiles(DIFFITEM *di)
{
	int nCompMethod = m_pCtx->m_nCompMethod;

	UINT code = DIFFCODE::FILE | DIFFCODE::CMPERR; // yields a warning icon

	if (nCompMethod == CMP_CONTENT || nCompMethod == CMP_QUICK_CONTENT)
	{
		// Reset text stats
		m_diffFileData.m_textStats[0].clear();
		m_diffFileData.m_textStats[1].clear();

		String origFileName1;
		String origFileName2;
		GetComparePaths(di, origFileName1, origFileName2);

		if (m_pCtx->m_bSelfCompare || origFileName1 != origFileName2)
		{
			// store true names for diff utils patch file
			m_diffFileData.SetDisplayFilepaths(origFileName1.c_str(), origFileName2.c_str());
		
			if (!m_diffFileData.OpenFiles(origFileName1.c_str(), origFileName2.c_str()))
			{
				return 0; // yields an error icon
			}

			HANDLE osfhandle[2] =
			{
				m_diffFileData.GetFileHandle(0), m_diffFileData.GetFileHandle(1)
			};

			GuessCodepageEncoding(origFileName1.c_str(), &m_diffFileData.m_FileLocation[0].encoding, m_pCtx->m_bGuessEncoding, osfhandle[0]);
			if (osfhandle[1] != osfhandle[0])
				GuessCodepageEncoding(origFileName2.c_str(), &m_diffFileData.m_FileLocation[1].encoding, m_pCtx->m_bGuessEncoding, osfhandle[1]);
			else
				m_diffFileData.m_FileLocation[1].encoding = m_diffFileData.m_FileLocation[0].encoding;

			// If either file is larger than limit compare files by quick contents
			// This allows us to (faster) compare big binary files
			if (di->left.size.int64 > m_pCtx->m_nQuickCompareLimit ||
				di->right.size.int64 > m_pCtx->m_nQuickCompareLimit)
			{
				nCompMethod = CMP_QUICK_CONTENT;
			}
		}
		else
		{
			m_diffFileData.m_FileLocation[0].encoding.Clear();
			m_diffFileData.m_FileLocation[1].encoding.Clear();
			nCompMethod = CMP_DATE_SIZE;
		}
	}

	if (nCompMethod == CMP_CONTENT)
	{
		if (m_pDiffUtilsEngine == NULL)
			m_pDiffUtilsEngine = new CompareEngines::DiffUtils(m_pCtx);

		m_pDiffUtilsEngine->SetCodepage(
			m_diffFileData.m_FileLocation[0].encoding.m_unicoding ? 
				CP_UTF8 : m_diffFileData.m_FileLocation[0].encoding.m_codepage);
		m_pDiffUtilsEngine->SetCompareFiles(
			m_diffFileData.m_FileLocation[0].filepath,
			m_diffFileData.m_FileLocation[1].filepath);
		m_pDiffUtilsEngine->SetToDiffUtils();
		code = m_pDiffUtilsEngine->diffutils_compare_files(m_diffFileData.m_inf);
		m_ndiffs = m_pDiffUtilsEngine->m_ndiffs;
		m_ntrivialdiffs = m_pDiffUtilsEngine->m_ntrivialdiffs;
		CopyTextStats(&m_diffFileData.m_inf[0], &m_diffFileData.m_textStats[0]);
		CopyTextStats(&m_diffFileData.m_inf[1], &m_diffFileData.m_textStats[1]);

		// If unique item, it was being compared to itself to determine encoding
		// and the #diffs is invalid
		if (di->isSideRightOnly() || di->isSideLeftOnly())
		{
			m_ndiffs = CDiffContext::DIFFS_UNKNOWN;
			m_ntrivialdiffs = CDiffContext::DIFFS_UNKNOWN;
		}
	}
	else if (nCompMethod == CMP_QUICK_CONTENT)
	{
		if (m_pByteCompare == NULL)
			m_pByteCompare = new CompareEngines::ByteCompare(m_pCtx);

		m_pByteCompare->SetFileData(2, m_diffFileData.m_inf);

		// use our own byte-by-byte compare
		code = m_pByteCompare->CompareFiles(&m_diffFileData.m_FileLocation.front());

		m_diffFileData.m_textStats[0] = m_pByteCompare->m_textStats[0];
		m_diffFileData.m_textStats[1] = m_pByteCompare->m_textStats[1];

		// Quick contents doesn't know about diff counts
		// Set to special value to indicate invalid
		m_ndiffs = CDiffContext::DIFFS_UNKNOWN_QUICKCOMPARE;
		m_ntrivialdiffs = CDiffContext::DIFFS_UNKNOWN_QUICKCOMPARE;
	}
	else if (nCompMethod == CMP_DATE || nCompMethod == CMP_DATE_SIZE || nCompMethod == CMP_SIZE)
	{
		if (m_pTimeSizeCompare == NULL)
			m_pTimeSizeCompare = new CompareEngines::TimeSizeCompare(m_pCtx);

		code = m_pTimeSizeCompare->CompareFiles(nCompMethod, di);

		m_ndiffs = CDiffContext::DIFFS_UNKNOWN;
		m_ntrivialdiffs = CDiffContext::DIFFS_UNKNOWN;
	}
	else
	{
		// Print error since we should have handled by date compare earlier
		_RPTF0(_CRT_ERROR, "Invalid compare type, DiffFileData can't handle it");
	}

	// If compare tool leaves text vs. binary classification to caller, defer
	// binaryness from number of zeros.
	if ((code & DIFFCODE::TEXTFLAGS) == DIFFCODE::TEXTFLAGS)
	{
		code &= ~DIFFCODE::TEXTFLAGS;
		if (m_diffFileData.m_textStats[0].nzeros > 0)
			code |= DIFFCODE::BINSIDE1 | DIFFCODE::TEXT;
		if (m_diffFileData.m_textStats[1].nzeros > 0)
			code |= DIFFCODE::BINSIDE2 | DIFFCODE::TEXT;
		code ^= DIFFCODE::TEXT; // revert reverse logic
	}

	m_diffFileData.Reset();

	return code;
}

/**
 * @brief Get actual compared paths from DIFFITEM.
 * @param [in] di DiffItem from which the paths are created.
 * @param [out] left Gets the left compare path.
 * @param [out] right Gets the right compare path.
 * @note If item is unique, same path is returned for both.
 */
void FolderCmp::GetComparePaths(const DIFFITEM *di, String &left, String &right) const
{
	if (!di->isSideRightOnly())
	{
		// Compare file to itself to detect encoding
		left = m_pCtx->GetLeftPath();
		if (!di->left.path.empty())
			left = paths_ConcatPath(left, di->left.path);
		left = paths_ConcatPath(left, di->left.filename);
		if (di->isSideLeftOnly())
			right = left;
	}
	if (!di->isSideLeftOnly())
	{
		// Compare file to itself to detect encoding
		right = m_pCtx->GetRightPath();
		if (!di->right.path.empty())
			right = paths_ConcatPath(right, di->right.path);
		right = paths_ConcatPath(right, di->right.filename);
		if (di->isSideRightOnly())
			left = right;
	}
}
