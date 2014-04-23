/**
 *  @file DirScan.cpp
 *
 *  @brief Implementation of DirScan (q.v.) and helper functions
 */ 
#include "StdAfx.h"
#include "Merge.h"
#include "LogFile.h"
#include "DiffContext.h"
#include "codepage.h"
#include "DirItem.h"
#include "paths.h"
#include "Common/coretools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Collect file- and folder-names to list.
 * This function walks given folders and adds found subfolders and files into
 * lists. There are two modes, determined by the @p depth:
 * - in non-recursive mode we walk only given folders, and add files.
 *   contained. Subfolders are added as folder items, not walked into.
 * - in recursive mode we walk all subfolders and add the files they
 *   contain into list.
 *
 * Items are tested against file filters in this function.
 * 
 * @param [in] leftsubdir Left side subdirectory under root path
 * @param [in] bLeftUniq Is left-side folder unique folder?
 * @param [in] rightsubdir Right side subdirectory under root path
 * @param [in] bRightUniq Is right-side folder unique folder?
 * @param [in] depth Levels of subdirectories to scan, -1 scans all
 * @param [in] parent Folder diff item to be scanned
 * @return 1 normally, -1 if compare was aborted
 */
int CDiffContext::DirScan_GetItems(
		const String &leftsubdir, bool bLeftUniq,
		const String &rightsubdir, bool bRightUniq,
		int depth, DIFFITEM *parent)
{
	ASSERT(!bLeftUniq || !bRightUniq); // Both folders cannot be unique
	static const TCHAR backslash[] = _T("\\");
	String sLeftDir = GetLeftPath();
	String sRightDir = GetRightPath();
	String leftsubprefix;
	String rightsubprefix;

	DirItemArray leftDirs, leftFiles, rightDirs, rightFiles;
	// Format paths for recursive compare (having basedir + subdir)
	// Hint: Could try to share cow string buffers here.
	if (!leftsubdir.empty())
	{
		sLeftDir = paths_ConcatPath(sLeftDir, leftsubdir);
		leftsubprefix = leftsubdir + backslash;
	}
	if (!bRightUniq)
	{
		LoadAndSortFiles(sLeftDir.c_str(), &leftDirs, &leftFiles, 0);
	}
	if (!rightsubdir.empty())
	{
		sRightDir = paths_ConcatPath(sRightDir, rightsubdir);
		rightsubprefix = rightsubdir + backslash;
	}
	if (!bLeftUniq)
	{
		LoadAndSortFiles(sRightDir.c_str(), &rightDirs, &rightFiles, 1);
	}

	// Allow user to abort scanning
	if (ShouldAbort())
		return -1;

	// Handle directories
	// i points to current directory in left list (leftDirs)
	// j points to current directory in right list (rightDirs)

	// If there is only one directory on each side, and no files
	// then pretend the directories have the same name
	const bool bTreatDirAsEqual =
		(leftDirs.size() == 1) &&
		(rightDirs.size() == 1) &&
		(leftFiles.size() == 0) &&
		(rightFiles.size() == 0) &&
		m_piFilterGlobal->includeDir(
			(leftsubprefix + leftDirs[0].filename).c_str(),
			(rightsubprefix + rightDirs[0].filename).c_str());

	DirItemArray::size_type i = 0, j = 0; 
	for (;;)
	{
		if (ShouldAbort())
			return -1;

		// Comparing directories leftDirs[i].name to rightDirs[j].name

		if (!bTreatDirAsEqual)
		{
			if (i < leftDirs.size() && (j == rightDirs.size() ||
					m_piFilterGlobal->collateDir(leftDirs[i].filename.c_str(), rightDirs[j].filename.c_str()) < 0))
			{
				UINT nDiffCode = DIFFCODE::LEFT | DIFFCODE::DIR;
				if (depth && m_bWalkUniques)
				{
					// Recurse into unique subfolder and get all items in it
					String leftnewsub = leftsubprefix + leftDirs[i].filename;
					String rightnewsub = rightsubprefix + leftDirs[i].filename;
					if (!m_piFilterGlobal->includeDir(leftnewsub.c_str(), _T("")))
					{
						nDiffCode |= DIFFCODE::SKIPPED;
						AddToList(leftsubdir, rightsubdir, &leftDirs[i], NULL, nDiffCode, parent);
					}
					else
					{
						DIFFITEM *me = AddToList(leftsubdir, rightsubdir, &leftDirs[i], NULL, nDiffCode, parent);
						if (DirScan_GetItems(leftnewsub, true, rightnewsub, false, depth - 1, me) == -1)
						{
							return -1;
						}
					}
				}
				else
				{
					AddToList(leftsubdir, rightsubdir, &leftDirs[i], NULL, nDiffCode, parent);
				}
				// Advance left pointer over left-only entry, and then retest with new pointers
				++i;
				continue;
			}
			if (j < rightDirs.size() && (i == leftDirs.size() ||
					m_piFilterGlobal->collateDir(leftDirs[i].filename.c_str(), rightDirs[j].filename.c_str()) > 0))
			{
				UINT nDiffCode = DIFFCODE::RIGHT | DIFFCODE::DIR;
				if (depth && m_bWalkUniques)
				{
					// Recurse into unique subfolder and get all items in it
					String leftnewsub = leftsubprefix + rightDirs[j].filename;
					String rightnewsub = rightsubprefix + rightDirs[j].filename;
					if (!m_piFilterGlobal->includeDir(_T(""), rightnewsub.c_str()))
					{
						nDiffCode |= DIFFCODE::SKIPPED;
						AddToList(leftsubdir, rightsubdir, NULL, &rightDirs[j], nDiffCode, parent);
					}
					else
					{
						DIFFITEM *me = AddToList(leftsubdir, rightsubdir, NULL, &rightDirs[j], nDiffCode, parent);
						if (DirScan_GetItems(leftnewsub, false, rightnewsub, true, depth - 1, me) == -1)
						{
							return -1;
						}
					}
				}
				else
				{
					AddToList(leftsubdir, rightsubdir, NULL, &rightDirs[j], nDiffCode, parent);
				}
				// Advance right pointer over right-only entry, and then retest with new pointers
				++j;
				continue;
			}
		}
		if (i < leftDirs.size())
		{
			ASSERT(j < rightDirs.size());
			if (!depth)
			{
				// Non-recursive compare
				// We are only interested about list of subdirectories to show - user can open them
				// TODO: scan one level deeper to see if directories are identical/different
				const UINT nDiffCode = DIFFCODE::BOTH | DIFFCODE::DIR;
				AddToList(leftsubdir, rightsubdir, &leftDirs[i], &rightDirs[j], nDiffCode, parent);
			}
			else
			{
				// Recursive compare
				String leftnewsub = leftsubprefix + leftDirs[i].filename;
				// Hint: Could try to share cow string buffers here.
				String rightnewsub = rightsubprefix + rightDirs[j].filename;
				// Test against filter so we don't include contents of filtered out directories
				// Also this is only place we can test for both-sides directories in recursive compare
				if (!m_piFilterGlobal->includeDir(leftnewsub.c_str(), rightnewsub.c_str()))
				{
					const UINT nDiffCode = DIFFCODE::BOTH | DIFFCODE::DIR | DIFFCODE::SKIPPED;
					AddToList(leftsubdir, rightsubdir, &leftDirs[i], &rightDirs[j], nDiffCode, parent);
				}
				else
				{
					const UINT nDiffCode = DIFFCODE::BOTH | DIFFCODE::DIR;
					DIFFITEM *me = AddToList(leftsubdir, rightsubdir, &leftDirs[i], &rightDirs[j], nDiffCode, parent);
					// Scan recursively all subdirectories too, we are not adding folders
					if (DirScan_GetItems(leftnewsub, false, rightnewsub, false, depth - 1, me) == -1)
					{
						return -1;
					}
				}
			}
			++i;
			++j;
			continue;
		}
		break;
	}
	// Handle files
	// i points to current file in left list (leftFiles)
	// j points to current file in right list (rightFiles)
	i = 0;
	j = 0;
	for (;;)
	{
		if (ShouldAbort())
			return -1;

		if (i < leftFiles.size() && bLeftUniq)
		{
			const int nDiffCode = DIFFCODE::LEFT | DIFFCODE::FILE;
			AddToList(leftsubdir, rightsubdir, &leftFiles[i], NULL, nDiffCode, parent);
			++i;
			continue;
		}

		if (j < rightFiles.size() && bRightUniq)
		{
			const int nDiffCode = DIFFCODE::RIGHT | DIFFCODE::FILE;
			AddToList(leftsubdir, rightsubdir, NULL, &rightFiles[j], nDiffCode, parent);
			++j;
			continue;
		}

		// Comparing file leftFiles[i].name to rightFiles[j].name
		
		if (i < leftFiles.size() && (j == rightFiles.size() ||
				m_piFilterGlobal->collateFile(leftFiles[i].filename.c_str(), rightFiles[j].filename.c_str()) < 0))
		{
			const UINT nDiffCode = DIFFCODE::LEFT | DIFFCODE::FILE;
			AddToList(leftsubdir, rightsubdir, &leftFiles[i], 0, nDiffCode, parent);
			// Advance left pointer over left-only entry, and then retest with new pointers
			++i;
			continue;
		}
		if (j < rightFiles.size() && (i == leftFiles.size() ||
				m_piFilterGlobal->collateFile(leftFiles[i].filename.c_str(), rightFiles[j].filename.c_str()) > 0))
		{
			const UINT nDiffCode = DIFFCODE::RIGHT | DIFFCODE::FILE;
			AddToList(leftsubdir, rightsubdir, 0, &rightFiles[j], nDiffCode, parent);
			// Advance right pointer over right-only entry, and then retest with new pointers
			++j;
			continue;
		}
		if (i < leftFiles.size())
		{
			ASSERT(j < rightFiles.size());
			const UINT nDiffCode = DIFFCODE::BOTH | DIFFCODE::FILE;
			AddToList(leftsubdir, rightsubdir, &leftFiles[i], &rightFiles[j], nDiffCode, parent);
			++i;
			++j;
			continue;
		}
		break;
	}
	return 1;
}

/**
 * @brief Compare DiffItems in list and add results to compare context.
 */
void CDiffContext::DirScan_CompareItems()
{
	FolderCmp folderCmp(this, InterlockedIncrement(&m_iCompareThread));
	for (;;)
	{
		DIFFITEM *di;
		WaitForSingleObject(m_hSemaphore, INFINITE);
		EnterCriticalSection(&m_csCompareThread);
		if (DIFFITEM *p = GetNextDiff(m_diCompareThread))
			di = m_diCompareThread = p;
		else
			di = NULL;
		LeaveCriticalSection(&m_csCompareThread);

		if (di == NULL || ShouldAbort())
			break;

		CompareDiffItem(folderCmp, di);

		if (!di->isResultFiltered() && (di->isResultDiff() || !di->isSideBoth()))
		{
			while (DIFFITEM *parent = di->parent)
			{
				if (!parent->isSideBoth() || parent->isResultDiff())
					break;
				EnterCriticalSection(&m_csCompareThread);
				parent->diffcode &= ~DIFFCODE::COMPAREFLAGS;
				parent->diffcode |= DIFFCODE::DIFF;
				LeaveCriticalSection(&m_csCompareThread);
				di = parent;
			}
		}
	}
}

void CDiffContext::DirScan_CompareRequestedItems()
{
	FolderCmp folderCmp(this, InterlockedIncrement(&m_iCompareThread));
	for (;;)
	{
		DIFFITEM *di;
		EnterCriticalSection(&m_csCompareThread);
		if (DIFFITEM *p = GetNextDiff(m_diCompareThread))
			di = m_diCompareThread = p;
		else
			di = NULL;
		LeaveCriticalSection(&m_csCompareThread);

		if (di == NULL || ShouldAbort())
			break;

		if (di->isScanNeeded())
		{
			bool bExists;
			EnterCriticalSection(&m_csCompareThread);
			// Clear rescan-request flag (not set by all codepaths)
			di->diffcode &= ~DIFFCODE::NEEDSCAN;
			bExists = UpdateDiffItem(di);
			LeaveCriticalSection(&m_csCompareThread);
			if (bExists)
				CompareDiffItem(folderCmp, di);
		}

		if (!di->isResultFiltered() && (di->isResultDiff() || !di->isSideBoth()))
		{
			while (DIFFITEM *parent = di->parent)
			{
				if (!parent->isSideBoth() || parent->isResultDiff())
					break;
				EnterCriticalSection(&m_csCompareThread);
				parent->diffcode &= ~DIFFCODE::COMPAREFLAGS;
				parent->diffcode |= DIFFCODE::DIFF;
				LeaveCriticalSection(&m_csCompareThread);
				di = parent;
			}
		}
	}
}

/**
 * @brief Update diffitem file/dir infos.
 *
 * Re-tests dirs/files if sides still exists, and updates infos for
 * existing sides. This assumes filenames, or paths are not changed.
 * Since in normal situations (I can think of) they cannot change
 * after first compare.
 *
 * @param [in,out] di DiffItem to update.
 * @param [out] bExists Set to
 *  - TRUE if one of items exists so diffitem is valid
 *  - FALSE if items were deleted, so diffitem is not valid
 * @param [in] pCtxt Compare context
 */
bool CDiffContext::UpdateDiffItem(DIFFITEM *di)
{
	bool bExists = false;
	// Clear side-info and file-infos
	di->left.ClearPartial();
	di->right.ClearPartial();
	di->diffcode |= DIFFCODE::BOTH;
	if (UpdateInfoFromDiskHalf(di, true))
		bExists = true;
	else
		di->diffcode &= ~DIFFCODE::LEFT;
	if (UpdateInfoFromDiskHalf(di, false))
		bExists = true;
	else
		di->diffcode &= ~DIFFCODE::RIGHT;
	return bExists;
}

/**
 * @brief Compare two diffitems and add results to difflist in context.
 *
 * This function does the actual compare for previously gathered list of
 * items. Basically we:
 * - ignore items matching filefilters
 * - add non-ignored directories (no compare for directory items)
 * - add  unique files
 * - compare files
 *
 * @param [in] di DiffItem to compare
 * @param [in,out] pCtxt Compare context: contains difflist, encoding info etc.
 * @todo For date compare, maybe we should use creation date if modification
 * date is missing?
 */
void CDiffContext::CompareDiffItem(FolderCmp &fc, DIFFITEM *di)
{
	// Is it a directory?
	if (di->isDirectory())
	{
		// 1. Test against filters
		const UINT flag = !m_piFilterGlobal->includeDir(
			di->left.filename.c_str(), di->right.filename.c_str()) ?
			DIFFCODE::SKIPPED : m_nRecursive != 0 && di->isSideBoth() ?
			DIFFCODE::INCLUDED | DIFFCODE::SAME : DIFFCODE::INCLUDED;
		EnterCriticalSection(&m_csCompareThread);
		di->diffcode |= flag;
		LeaveCriticalSection(&m_csCompareThread);
		// We don't actually 'compare' directories, just add non-ignored
		// directories to list.
	}
	else
	{
		// 1. Test against filters
		if (m_piFilterGlobal->includeFile(di->left.filename.c_str(), di->right.filename.c_str()))
		{
			di->diffcode |= DIFFCODE::INCLUDED;
			// 2. Compare two files
			if (di->isSideBoth())
			{
				m_pCompareStats->BeginCompare(di, fc.m_iCompareThread);
				// Really compare
				di->diffcode |= fc.prepAndCompareTwoFiles(di);
				SetDiffItemStats(fc, di);
			}
			// 3. Add unique files
			// We must compare unique files to itself to detect encoding
			else if (m_nCompMethod != CMP_DATE &&
				m_nCompMethod != CMP_DATE_SIZE &&
				m_nCompMethod != CMP_SIZE)
			{
				m_pCompareStats->BeginCompare(di, fc.m_iCompareThread);
				// Add possible binary flag for unique items
				di->diffcode |= fc.prepAndCompareTwoFiles(di) & DIFFCODE::BIN;
				SetDiffItemStats(fc, di);
			}
		}
		else
		{
			di->diffcode |= DIFFCODE::SKIPPED;
		}
	}
	StoreDiffData(di);
}

/**
 * @brief Send one file or directory result back through the diff context.
 * @param [in] di Data to store.
 * @param [in] pCtxt Compare context.
 * @param [in] pCmpData Folder compare data.
 */
void CDiffContext::SetDiffItemStats(const FolderCmp &fc, DIFFITEM *di)
{
	// Set text statistics
	if (di->isSideLeftOrBoth())
		di->left.m_textStats = fc.m_diffFileData.m_textStats[0];
	if (di->isSideRightOrBoth())
		di->right.m_textStats = fc.m_diffFileData.m_textStats[1];

	di->nsdiffs = fc.m_ndiffs;
	di->nidiffs = fc.m_ntrivialdiffs;

	if (!di->isSideLeftOnly())
	{
		di->right.encoding = fc.m_diffFileData.m_FileLocation[1].encoding;
	}
	
	if (!di->isSideRightOnly())
	{
		di->left.encoding = fc.m_diffFileData.m_FileLocation[0].encoding;
	}
}

void CDiffContext::StoreDiffData(const DIFFITEM *di)
{
	LogFile.Write
	(
		CLogFile::LCOMPAREDATA, _T("name=<%s>, leftdir=<%s>, rightdir=<%s>, code=%d"),
		di->left.filename.c_str(), di->left.path.c_str(), di->right.path.c_str(), di->diffcode
	);
	m_pCompareStats->AddItem(di);
}

/**
 * @brief Add one compare item to list.
 * @param [in] sLeftDir Left subdirectory.
 * @param [in] sRightDir Right subdirectory.
 * @param [in] lent Left item data to add.
 * @param [in] rent Right item data to add.
 * @param [in] code
 * @param [in] parent Parent of item to be added.
 */
DIFFITEM *CDiffContext::AddToList(
	const String &sLeftDir, const String &sRightDir,
	const DirItem *lent, const DirItem *rent,
	UINT code, DIFFITEM *parent)
{
	// We must store both paths - we cannot get paths later
	// and we need unique item paths for example when items
	// change to identical
	DIFFITEM *di = AddDiff(parent);

	di->left.path = sLeftDir;
	di->right.path = sRightDir;

	if (lent)
	{
		if (m_nRecursive == 2)
		{
			if (LPCTSTR path = EatPrefix(lent->path.c_str(), GetLeftPath().c_str()))
			{
				di->left.path = path;
			}
			else
			{
				ASSERT(FALSE);
			}
		}
		di->left.filename = lent->filename;
		di->left.mtime = lent->mtime;
		di->left.ctime = lent->ctime;
		di->left.size = lent->size;
		di->left.flags.attributes = lent->flags.attributes;
	}
	else
	{
		// Don't break CDirView::DoCopyRightToLeft()
		di->left.filename = rent->filename;
	}

	if (rent)
	{
		if (m_nRecursive == 2)
		{
			if (LPCTSTR path = EatPrefix(rent->path.c_str(), GetRightPath().c_str()))
			{
				di->right.path = path;
			}
			else
			{
				ASSERT(FALSE);
			}
		}
		di->right.filename = rent->filename;
		di->right.mtime = rent->mtime;
		di->right.ctime = rent->ctime;
		di->right.size = rent->size;
		di->right.flags.attributes = rent->flags.attributes;
	}
	else
	{
		// Don't break CDirView::DoCopyLeftToRight()
		di->right.filename = lent->filename;
	}

	di->diffcode = code;

	LogFile.Write
	(
		CLogFile::LCOMPAREDATA, _T("name=<%s>, leftdir=<%s>, rightdir=<%s>, code=%d"),
		di->left.filename.c_str(), di->left.path.c_str(), di->right.path.c_str(), code
	);
	m_pCompareStats->IncreaseTotalItems();
	ReleaseSemaphore(m_hSemaphore, 1, 0);
	return di;
}
