/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DirDoc.cpp
 *
 * @brief Implementation file for CDirDoc
 *
 */
// ID line follows -- this is updated by SVN
// $Id$
//

#include "StdAfx.h"
#include "Merge.h"
#include "ChildFrm.h"
#include "LanguageSelect.h"
#include "HexMergeFrm.h"
#include "7zCommon.h"
#include "CompareStats.h"
#include "FilterList.h"
#include "DirFrame.h"
#include "DirView.h"
#include "MainFrm.h"
#include "coretools.h"
#include "LogFile.h"
#include "paths.h"
#include "OptionsDef.h"
#include "FileActionScript.h"
#include "LineFiltersList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Checks if there are mergedocs associated with this dirdoc.
 */
bool CDirFrame::CanFrameClose()
{
	return m_MergeDocs.empty() && m_HexMergeDocs.empty();
}

/////////////////////////////////////////////////////////////////////////////
// CDirDoc commands

/**
 * @brief Initialise directory compare for given paths.
 *
 * Initialises directory compare with paths given and recursive choice.
 * Previous compare context is first free'd.
 * @param [in] paths Paths to compare
 * @param [in] bRecursive If TRUE subdirectories are included to compare.
 */
void CDirFrame::InitCompare(LPCTSTR pszLeft, LPCTSTR pszRight, BOOL bRecursive, CTempPathContext *pTempPathContext)
{
	m_pDirView->DeleteAllItems();
	// Anything that can go wrong here will yield an exception.
	// Default implementation of operator new() never returns NULL.
	delete m_pCtxt;
	
	m_pCtxt = new CDiffContext(pszLeft, pszRight);
	m_pCtxt->m_bRecursive = !!bRecursive;

	if (pTempPathContext)
	{
		ApplyLeftDisplayRoot(pTempPathContext->m_strLeftDisplayRoot);
		ApplyRightDisplayRoot(pTempPathContext->m_strRightDisplayRoot);
		pTempPathContext->m_pParent = m_pTempPathContext;
		m_pTempPathContext = pTempPathContext;
		m_pTempPathContext->m_strLeftRoot = m_pCtxt->GetLeftPath();
		m_pTempPathContext->m_strRightRoot = m_pCtxt->GetRightPath();
	}
	
	m_bRecursive = bRecursive;
}

/**
 * @brief Tell if user may use ".." and move to parents directory.
 * This function checks if current compare's parent folders should
 * be allowed to open. I.e. if current compare folders are:
 * - C:\Work\Project1 and
 * - C:\Work\Project2
 * we check if C:\Work and C:\Work should be allowed to opened.
 * For regular folders we allow opening if both folders exist.
 * @param [out] leftParent Left parent folder to open.
 * @param [out] rightParent Right parent folder to open.
 * @return Info if opening parent folders should be enabled:
 * - No : upward RESTRICTED
 * - ParentIsRegularPath : upward ENABLED
 * - ParentIsTempPath : upward ENABLED
 */
CDirFrame::AllowUpwardDirectory::ReturnCode
CDirFrame::AllowUpwardDirectory(String &leftParent, String &rightParent)
{
	const String & left = GetLeftBasePath();
	const String & right = GetRightBasePath();

	// If we have temp context it means we are comparing archives
	if (IsArchiveFolders())
	{
		LPCTSTR lname = PathFindFileName(left.c_str());
		LPCTSTR rname = PathFindFileName(right.c_str());
        String::size_type cchLeftRoot = m_pTempPathContext->m_strLeftRoot.length();

		if (left.length() <= cchLeftRoot + 1) // + 1 balances a trailing backslash
		{
			if (m_pTempPathContext->m_pParent)
			{
				leftParent = m_pTempPathContext->m_pParent->m_strLeftRoot;
				rightParent = m_pTempPathContext->m_pParent->m_strRightRoot;
				if (GetPairComparability(leftParent.c_str(), rightParent.c_str()) != IS_EXISTING_DIR)
					return AllowUpwardDirectory::Never;
				return AllowUpwardDirectory::ParentIsTempPath;
			}
			leftParent = m_pTempPathContext->m_strLeftDisplayRoot;
			rightParent = m_pTempPathContext->m_strRightDisplayRoot;
			if (!m_pCtxt->m_piFilterGlobal->includeFile(leftParent.c_str(), rightParent.c_str()))
				return AllowUpwardDirectory::Never;
			if (lstrcmpi(lname, _T("ORIGINAL")) == 0 && lstrcmpi(rname, _T("ALTERED")) == 0)
			{
				leftParent = paths_GetParentPath(leftParent.c_str());
				rightParent = paths_GetParentPath(rightParent.c_str());
			}
			lname = PathFindFileName(leftParent.c_str());
			rname = PathFindFileName(rightParent.c_str());
			if (lstrcmpi(lname, rname) == 0)
			{
				leftParent = paths_GetParentPath(leftParent.c_str());
				rightParent = paths_GetParentPath(rightParent.c_str());
				if (GetPairComparability(leftParent.c_str(), rightParent.c_str()) != IS_EXISTING_DIR)
					return AllowUpwardDirectory::Never;
				return AllowUpwardDirectory::ParentIsTempPath;
			}
			return AllowUpwardDirectory::No;
		}
		rname = lname;
	}

	// If regular parent folders exist, allow opening them
	leftParent = paths_GetParentPath(left.c_str());
	rightParent = paths_GetParentPath(right.c_str());
	if (GetPairComparability(leftParent.c_str(), rightParent.c_str()) != IS_EXISTING_DIR)
		return AllowUpwardDirectory::Never;
	return AllowUpwardDirectory::ParentIsRegularPath;
}

/**
 * @brief Load line filters to the compare context.
 * Loads linefilters, converts them to UTF-8 and sets them for compare context.
 */
void CDirFrame::LoadLineFilterList()
{
	ASSERT(m_pCtxt);
	
	BOOL bFilters = COptionsMgr::Get(OPT_LINEFILTER_ENABLED);
	String filters = GetMainFrame()->m_pLineFilters->GetAsString();
	if (!bFilters || filters.empty())
	{
		delete m_pCtxt->m_pFilterList;
		m_pCtxt->m_pFilterList = NULL;
		return;
	}

	if (m_pCtxt->m_pFilterList)
		m_pCtxt->m_pFilterList->RemoveAllFilters();
	else
		m_pCtxt->m_pFilterList = new FilterList();

	m_pCtxt->m_pFilterList->AddRegExp(filters.c_str());
}

/**
 * @brief Perform directory comparison again from scratch
 */
void CDirFrame::Rescan()
{
	if (!m_pCtxt)
		return;

	// If we're already doing a rescan, bail out
	UINT threadState = m_diffThread.GetThreadState();
	if (threadState == CDiffThread::THREAD_COMPARING)
		return;

	waitStatusCursor.Begin(IDS_STATUS_RESCANNING);
	UpdateCmdUI<ID_REFRESH>();

	LogFile.Write(CLogFile::LNOTICE, _T("Starting directory scan:\n\tLeft: %s\n\tRight: %s\n"),
			m_pCtxt->GetLeftPath().c_str(), m_pCtxt->GetRightPath().c_str());
	m_pCompareStats->Reset();
	m_pDirView->StartCompare(m_pCompareStats);

	// Don't clear if only scanning selected items
	if (!m_bMarkedRescan)
	{
		m_pDirView->DeleteAllItems();
		m_pCtxt->RemoveAll();
	}

	LoadLineFilterList();

	m_pCtxt->m_options.nIgnoreWhitespace = COptionsMgr::Get(OPT_CMP_IGNORE_WHITESPACE);
	m_pCtxt->m_options.bIgnoreBlankLines = COptionsMgr::Get(OPT_CMP_IGNORE_BLANKLINES);
	m_pCtxt->m_options.bFilterCommentsLines = COptionsMgr::Get(OPT_CMP_FILTER_COMMENTLINES);
	m_pCtxt->m_options.bIgnoreCase = COptionsMgr::Get(OPT_CMP_IGNORE_CASE);
	m_pCtxt->m_options.bIgnoreEol = COptionsMgr::Get(OPT_CMP_IGNORE_EOL);
	m_pCtxt->m_nCurrentCompMethod = m_pCtxt->m_nCompMethod = COptionsMgr::Get(OPT_CMP_METHOD);
	m_pCtxt->m_bGuessEncoding = COptionsMgr::Get(OPT_CP_DETECT);
	m_pCtxt->m_bIgnoreSmallTimeDiff = COptionsMgr::Get(OPT_IGNORE_SMALL_FILETIME);
	m_pCtxt->m_bStopAfterFirstDiff = COptionsMgr::Get(OPT_CMP_STOP_AFTER_FIRST);
	m_pCtxt->m_nQuickCompareLimit = COptionsMgr::Get(OPT_CMP_QUICK_LIMIT);
	m_pCtxt->m_bWalkUniques = COptionsMgr::Get(OPT_CMP_WALK_UNIQUE_DIRS);
	m_pCtxt->m_pCompareStats = m_pCompareStats;
	m_pCtxt->m_bRecursive = !!m_bRecursive;

	// Set total items count since we don't collect items
	if (m_bMarkedRescan)
		m_pCompareStats->IncreaseTotalItems(m_pDirView->GetSelectedCount());

	UpdateHeaderPath(0);
	UpdateHeaderPath(1);
	// draw the headers as active ones
	m_wndFilePathBar.SetActive(0, true);
	m_wndFilePathBar.SetActive(1, true);

	// Make sure filters are up-to-date
	globalFileFilter.ReloadUpdatedFilters();
	m_pCtxt->m_piFilterGlobal = &globalFileFilter;

	// Show active filter name in statusbar
	SetFilterStatusDisplay(globalFileFilter.GetFilterNameOrMask().c_str());

	// Folder names to compare are in the compare context
	m_diffThread.SetContext(m_pCtxt);
	m_diffThread.SetHwnd(m_pDirView->m_hWnd);
	m_diffThread.SetMessageIDs(MSG_UI_UPDATE);
	m_diffThread.SetCompareSelected(!!m_bMarkedRescan);
	m_diffThread.CompareDirectories(m_pCtxt->GetLeftPath(), m_pCtxt->GetRightPath());
	m_bMarkedRescan = FALSE;
}

/**
 * @brief Determines if the user wants to see given item.
 * This function determines what items to show and what items to hide. There
 * are lots of combinations, but basically we check if menuitem is enabled or
 * disabled and show/hide matching items. For non-recursive compare we never
 * hide folders as that would disable user browsing into them. And we even
 * don't really know if folders are identical or different as we haven't
 * compared them.
 * @param [in] di Item to check.
 * @return true if item should be shown, false if not.
 * @sa CDirFrame::Redisplay()
 */
bool CDirFrame::IsShowable(const DIFFITEM &di)
{
	if (di.customFlags1 & ViewCustomFlags::HIDDEN)
		return false;
	// Treat SKIPPED as a 'super'-flag. If item is skipped and user
	// wants to see skipped items show item regardless of other flags
	if (di.diffcode.isResultFiltered())
		return COptionsMgr::Get(OPT_SHOW_SKIPPED);
	// left/right filters
	if (di.diffcode.isSideLeftOnly() && !COptionsMgr::Get(OPT_SHOW_UNIQUE_LEFT))
		return false;
	if (di.diffcode.isSideRightOnly() && !COptionsMgr::Get(OPT_SHOW_UNIQUE_RIGHT))
		return false;
	if (di.diffcode.isDirectory())
	{
		// Subfolders in non-recursive compare can only be skipped or unique
		if (m_bRecursive) // recursive mode (including tree-mode)
		{
			// ONLY filter folders by result (identical/different) for tree-view.
			// In the tree-view we show subfolders with identical/different
			// status. The flat view only shows files inside folders. So if we
			// filter by status the files inside folder are filtered too and
			// users see files appearing/disappearing without clear logic.		
			if (COptionsMgr::Get(OPT_TREE_MODE))
			{
				// result filters
				if (di.diffcode.isResultSame() && !COptionsMgr::Get(OPT_SHOW_IDENTICAL))
					return false;
				if (di.diffcode.isResultDiff() && !COptionsMgr::Get(OPT_SHOW_DIFFERENT))
					return false;
			}
		}
	}
	else
	{
		// file type filters
		if (di.diffcode.isBin() && !COptionsMgr::Get(OPT_SHOW_BINARIES))
			return false;
		// result filters
		if (di.diffcode.isResultSame() && !COptionsMgr::Get(OPT_SHOW_IDENTICAL))
			return false;
		if (di.diffcode.isResultDiff() && !COptionsMgr::Get(OPT_SHOW_DIFFERENT))
			return false;
	}
	return true;
}

/**
 * @brief Empty & reload listview (of files & columns) with comparison results
 * @todo Better solution for special items ("..")?
 */
void CDirFrame::Redisplay()
{
	if (!m_pDirView)
		return;

	// Do not redisplay an empty CDirView
	// Not only does it not have results, but AddSpecialItems will crash
	// trying to dereference null context pointer to get to paths
	if (!m_pCtxt)
		return;

	m_pDirView->Redisplay();
}

/**
 * @brief Update in-memory diffitem status from disk.
 * @param [in] diffPos POSITION of item in UI list.
 * @param [in] bLeft If TRUE left-side item is updated.
 * @param [in] bRight If TRUE right-side item is updated.
 */
void CDirFrame::UpdateStatusFromDisk(UINT_PTR diffPos, BOOL bLeft, BOOL bRight)
{
	m_pCtxt->UpdateStatusFromDisk(diffPos, bLeft, bRight);
}

/**
 * @brief Update in-memory diffitem status from disk and update view.
 * @param [in] diffPos POSITION of item in UI list.
 * @param [in] bLeft If TRUE left-side item is updated.
 * @param [in] bRight If TRUE right-side item is updated.
 * @note Do not call this function from DirView code! This function
 * calls slow DirView functions to get item position and to update GUI.
 * Use UpdateStatusFromDisk() function instead.
 */
void CDirFrame::ReloadItemStatus(UINT_PTR diffPos, BOOL bLeft, BOOL bRight)
{
	// in case just copied (into existence) or modified
	UpdateStatusFromDisk(diffPos, bLeft, bRight);
	int nIdx = m_pDirView->GetItemIndex(diffPos);
	if (nIdx != -1)
	{
		// Update view
		m_pDirView->UpdateDiffItemStatus(nIdx);
	}
}

/**
 * @brief Find the CDiffContext diffpos of an item from its left & right paths
 * @return POSITION to item, NULL if not found.
 * @note Filenames must be same, if they differ NULL is returned.
 */
UINT_PTR CDirFrame::FindItemFromPaths(LPCTSTR pathLeft, LPCTSTR pathRight)
{
	LPCTSTR file1 = PathFindFileName(pathLeft);
	LPCTSTR file2 = PathFindFileName(pathRight);

	// Filenames must be identical
	if (lstrcmpi(file1, file2) != 0)
		return NULL;

	String path1(pathLeft, file1 - pathLeft); // include trailing backslash
	String path2(pathRight, file2 - pathRight); // include trailing backslash

	String base1 = m_pCtxt->GetLeftPath();
	if (path1.compare(0, base1.length(), base1.c_str()) != 0)
		return NULL;
	path1.erase(0, base1.length()); // turn into relative path
	if (String::size_type length = path1.length())
		path1.resize(length - 1); // remove trailing backslash

	String base2 = m_pCtxt->GetRightPath();
	if (path2.compare(0, base2.length(), base2.c_str()) != 0)
		return NULL;
	path2.erase(0, base2.length()); // turn into relative path
	if (String::size_type length = path2.length())
		path2.resize(length - 1); // remove trailing backslash

	UINT_PTR pos = m_pCtxt->GetFirstDiffPosition();
	while (UINT_PTR currentPos = pos) // Save our current pos before getting next
	{
		const DIFFITEM &di = m_pCtxt->GetNextDiffPosition(pos);
		if (di.left.path == path1 &&
			di.right.path == path2 &&
			di.left.filename == file1 &&
			di.right.filename == file2)
		{
			return currentPos;
		}
	}
	return 0;
}

/**
 * @brief A new MergeDoc has been opened.
 */
void CDirFrame::AddMergeDoc(CChildFrame *pMergeDoc)
{
	m_MergeDocs.push_back(pMergeDoc);
}

/**
 * @brief A new HexMergeDoc has been opened.
 */
void CDirFrame::AddMergeDoc(CHexMergeFrame *pHexMergeDoc)
{
	m_HexMergeDocs.push_back(pHexMergeDoc);
}

/**
 * @brief MergeDoc informs us it is closing.
 */
void CDirFrame::MergeDocClosing(CChildFrame *pMergeDoc)
{
	MergeDocPtrList::iterator pos = stl::find(
		m_MergeDocs.begin(), m_MergeDocs.end(), pMergeDoc);
	if (pos != m_MergeDocs.end())
		m_MergeDocs.erase(pos);
	else
		assert(false);
}

/**
 * @brief MergeDoc informs us it is closing.
 */
void CDirFrame::MergeDocClosing(CHexMergeFrame *pHexMergeDoc)
{
	HexMergeDocPtrList::iterator pos = stl::find(
		m_HexMergeDocs.begin(), m_HexMergeDocs.end(), pHexMergeDoc);
	if (pos != m_HexMergeDocs.end())
		m_HexMergeDocs.erase(pos);
	else
		assert(false);
}

/**
 * @brief Close MergeDocs opened from DirDoc.
 *
 * Asks confirmation for docs containing unsaved data and then
 * closes MergeDocs.
 * @return TRUE if success, FALSE if user canceled or closing failed
 */
bool CDirFrame::CloseMergeDocs()
{
	while (!m_MergeDocs.empty())
	{
		CChildFrame *pMergeDoc = m_MergeDocs.front();
		if (!pMergeDoc->SaveModified())
			return false;
		pMergeDoc->SendMessage(WM_CLOSE);
	}
	while (!m_HexMergeDocs.empty())
	{
		CHexMergeFrame *pHexMergeDoc = m_HexMergeDocs.front();
		if (!pHexMergeDoc->SaveModified())
			return false;
		pHexMergeDoc->SendMessage(WM_CLOSE);
	}
	return true;
}

/**
 * @brief Obtain a merge doc to display a difference in files.
 * @param [out] pNew Set to TRUE if a new doc is created,
 * and FALSE if an existing one reused.
 * @return Pointer to CMergeDoc to use (new or existing). 
 */
CChildFrame *CDirFrame::GetMergeDocForDiff()
{
	CChildFrame *pMergeDoc = new CChildFrame(GetMainFrame());
	AddMergeDoc(pMergeDoc);
	pMergeDoc->SetDirDoc(this);
	return pMergeDoc;
}

/**
 * @brief Obtain a hex merge doc to display a difference in files.
 * @param [out] pNew Set to TRUE if a new doc is created,
 * and FALSE if an existing one reused.
 * @return Pointer to CHexMergeDoc to use (new or existing). 
 */
CHexMergeFrame *CDirFrame::GetHexMergeDocForDiff()
{
	CHexMergeFrame *pHexMergeDoc = new CHexMergeFrame(GetMainFrame());
	AddMergeDoc(pHexMergeDoc);
	pHexMergeDoc->SetDirDoc(this);
	return pHexMergeDoc;
}

/**
 * @brief Update changed item's compare status
 * @param [in] paths Paths for files we update
 * @param [in] nDiffs Total amount of differences
 * @param [in] nTrivialDiffs Amount of ignored differences
 * @param [in] bIdentical TRUE if files became identical, FALSE otherwise.
 */
void CDirFrame::UpdateChangedItem(const CChildFrame *pMergeDoc)
{
	LPCTSTR paths[2] =
	{
		pMergeDoc->m_strPath[0].c_str(),
		pMergeDoc->m_strPath[1].c_str()
	};

	UINT_PTR pos = FindItemFromPaths(paths[0], paths[1]);
	// If we failed files could have been swapped so lets try again
	if (!pos)
		pos = FindItemFromPaths(paths[1], paths[0]);
	
	// Update status if paths were found for items.
	// Fail means we had unique items compared as 'renamed' items
	// so there really is not status to update.
	if (pos)
	{
		ReloadItemStatus(pos, TRUE, TRUE);

		DIFFITEM &di = m_pCtxt->GetDiffRefAt(pos);

		di.nsdiffs = pMergeDoc->m_diffList.GetSignificantDiffs();
		di.nidiffs = pMergeDoc->m_nTrivialDiffs;

		di.diffcode.diffcode &= ~(DIFFCODE::COMPAREFLAGS | DIFFCODE::TEXTFLAGS);
		di.diffcode.diffcode |= di.nsdiffs ? DIFFCODE::DIFF : DIFFCODE::SAME;

		di.left.m_textStats = pMergeDoc->m_pFileTextStats[0];
		di.right.m_textStats = pMergeDoc->m_pFileTextStats[1];

		if (di.left.m_textStats.nzeros > 0)
			di.diffcode.diffcode |= DIFFCODE::BINSIDE1 | DIFFCODE::TEXT;
		if (di.right.m_textStats.nzeros > 0)
			di.diffcode.diffcode |= DIFFCODE::BINSIDE2 | DIFFCODE::TEXT;
		di.diffcode.diffcode ^= DIFFCODE::TEXT; // revert reverse logic
	}
}

/**
 * @brief Cleans up after directory compare
 */
void CDirFrame::CompareReady()
{
	LogFile.Write(CLogFile::LNOTICE, _T("Directory scan complete\n"));
	waitStatusCursor.End();
	UpdateCmdUI<ID_REFRESH>();
}

/**
 * @brief Set side status of diffitem
 * @note This does not update UI - ReloadItemStatus() does
 * @sa CDirFrame::ReloadItemStatus()
 */
void CDirFrame::SetDiffSide(UINT diffcode, int idx)
{
	SetDiffStatus(diffcode, DIFFCODE::SIDEFLAGS, idx);
}

/**
 * @brief Set compare status of diffitem
 * @note This does not update UI - ReloadItemStatus() does
 * @sa CDirFrame::ReloadItemStatus()
 */
void CDirFrame::SetDiffCompare(UINT diffcode, int idx)
{
	SetDiffStatus(diffcode, DIFFCODE::COMPAREFLAGS, idx);
}

/**
 * @brief Set status for diffitem
 * @param diffcode New code
 * @param mask Defines allowed set of flags to change
 * @param idx Item's index to list in UI
 */
void CDirFrame::SetDiffStatus(UINT diffcode, UINT mask, int idx)
{
	// Get position of item in DiffContext 
	UINT_PTR diffpos = m_pDirView->GetItemKey(idx);

	// TODO: Why is the update broken into these pieces ?
	// Someone could figure out these pieces and probably simplify this.

	// Update DIFFITEM code (comparison result) to DiffContext
	m_pCtxt->SetDiffStatusCode(diffpos, diffcode, mask);

	// update DIFFITEM time (and other disk info), and tell views
}

/**
 * @brief Write path and filename to headerbar
 * @note SetText() does not repaint unchanged text
 */
void CDirFrame::UpdateHeaderPath(BOOL bLeft)
{
	int nPane = 0;
	String sText;

	if (bLeft)
	{
		if (!m_strLeftDesc.empty())
			sText = m_strLeftDesc;
		else
		{
			sText = m_pCtxt->GetLeftPath();
			ApplyLeftDisplayRoot(sText);
		}
		nPane = 0;
	}
	else
	{
		if (!m_strRightDesc.empty())
			sText = m_strRightDesc;
		else
		{
			sText = m_pCtxt->GetRightPath();
			ApplyRightDisplayRoot(sText);
		}
		nPane = 1;
	}

	m_wndFilePathBar.SetText(nPane, sText.c_str(), FALSE);
}

/**
 * @brief Send signal to thread to stop current scan
 *
 * @sa CDirCompStateBar::OnStop()
 */
void CDirFrame::AbortCurrentScan()
{
	LogFile.Write(CLogFile::LNOTICE, _T("Dircompare aborted!"));
	m_diffThread.Abort();
}

/**
 * @brief Returns true if there is an active scan that hasn't been aborted.
 */
bool CDirFrame::IsCurrentScanAbortable() const
{
	return (m_diffThread.GetThreadState() == CDiffThread::THREAD_COMPARING 
		&& !m_diffThread.IsAborting());
}

/**
 * @brief Set directory description texts shown in headerbar
 */
void CDirFrame::SetDescriptions(const String &strLeftDesc, const String &strRightDesc)
{
	m_strLeftDesc = strLeftDesc;
	m_strRightDesc = strRightDesc;

	String strTitle;
	if (!m_pCtxt || m_pCtxt->GetLeftPath().empty() ||
		m_pCtxt->GetRightPath().empty())
	{
		strTitle = LanguageSelect.LoadString(IDS_DIRECTORY_WINDOW_TITLE);
	}
	else
	{
		const TCHAR strSeparator[] = _T(" - ");
		String strPath = m_pCtxt->GetLeftPath();
		ApplyLeftDisplayRoot(strPath);
		strTitle = PathFindFileName(strPath.c_str());
		strTitle += strSeparator;
		strPath = m_pCtxt->GetRightPath();
		ApplyRightDisplayRoot(strPath);
		strTitle += PathFindFileName(strPath.c_str());
	}
	SetWindowText(strTitle.c_str());
}

/**
 * @brief Replace internal root by display root (left).
 * When we have a archive file open, this function converts physical folder
 * (that is in the temp folder where archive was extracted) to the virtual
 * path for showing. The virtual path is path to the archive file, archive
 * file name and folder inside the archive.
 * @param [in, out] sText Path to convert.
 */
void CDirFrame::ApplyLeftDisplayRoot(String &sText)
{
	if (m_pTempPathContext)
	{
		sText.erase(0, m_pTempPathContext->m_strLeftRoot.length());
		// Eat the trailing backslash when returning to an archive's root level
		if (sText == _T("\\"))
			sText.clear();
		sText.insert(0, m_pTempPathContext->m_strLeftDisplayRoot.c_str());
	}
}

/**
 * @brief Replace internal root by display root (right).
 * When we have a archive file open, this function converts physical folder
 * (that is in the temp folder where archive was extracted) to the virtual
 * path for showing. The virtual path is path to the archive file, archive
 * file name and folder inside the archive.
 * @param [in, out] sText Path to convert.
 */
void CDirFrame::ApplyRightDisplayRoot(String &sText)
{
	if (m_pTempPathContext)
	{
		sText.erase(0, m_pTempPathContext->m_strRightRoot.length());
		// Eat the trailing backslash when returning to an archive's root level
		if (sText == _T("\\"))
			sText.clear();
		sText.insert(0, m_pTempPathContext->m_strRightDisplayRoot.c_str());
	}
}

void CDirFrame::SetDiffCounts(UINT diffs, UINT ignored, int idx)
{
	// Get position of item in DiffContext 
	UINT_PTR diffpos = m_pDirView->GetItemKey(idx);

	// Update diff counts
	m_pCtxt->SetDiffCounts(diffpos, diffs, ignored);
}

/**
 * @brief Update results for FileActionItem.
 * This functions is called to update DIFFITEM after FileActionItem.
 * @param [in] act Action that was done.
 * @param [in] pos List position for DIFFITEM affected.
 */
void CDirFrame::UpdateDiffAfterOperation(const FileActionItem & act, UINT_PTR pos)
{
	ASSERT(pos != NULL);
	const DIFFITEM &di = GetDiffByKey(pos);

	// Use FileActionItem types for simplicity for now.
	// Better would be to use FileAction contained, since it is not
	// UI dependent.
	switch (act.UIResult)
	{
	case FileActionItem::UI_SYNC:
		SetDiffSide(DIFFCODE::BOTH, act.context);
		if (act.dirflag)
			SetDiffCompare(DIFFCODE::NOCMP, act.context);
		else
			SetDiffCompare(DIFFCODE::SAME, act.context);
		SetDiffCounts(0, 0, act.context);
		break;

	case FileActionItem::UI_DEL_LEFT:
		if (di.diffcode.isSideLeftOnly())
		{
			RemoveDiffByKey(pos);
		}
		else
		{
			SetDiffSide(DIFFCODE::RIGHT, act.context);
			SetDiffCompare(DIFFCODE::NOCMP, act.context);
		}
		break;

	case FileActionItem::UI_DEL_RIGHT:
		if (di.diffcode.isSideRightOnly())
		{
			RemoveDiffByKey(pos);
		}
		else
		{
			SetDiffSide(DIFFCODE::LEFT, act.context);
			SetDiffCompare(DIFFCODE::NOCMP, act.context);
		}
		break;

	case FileActionItem::UI_DEL_BOTH:
		RemoveDiffByKey(pos);
		break;
	}
}

/**
 * @brief Set item's view-flag.
 * @param [in] key Item fow which flag is set.
 * @param [in] flag Flag value to set.
 * @param [in] mask Mask for possible flag values.
 */
void CDirFrame::SetItemViewFlag(UINT_PTR key, UINT flag, UINT mask)
{
	UINT curFlags = m_pCtxt->GetCustomFlags1(key);
	curFlags &= ~mask; // Zero bits masked
	curFlags |= flag;
	m_pCtxt->SetCustomFlags1(key, curFlags);
}

/**
 * @brief Set all item's view-flag.
 * @param [in] flag Flag value to set.
 * @param [in] mask Mask for possible flag values.
 */
void CDirFrame::SetItemViewFlag(UINT flag, UINT mask)
{
	UINT_PTR pos = m_pCtxt->GetFirstDiffPosition();

	while (pos != NULL)
	{
		UINT curFlags = m_pCtxt->GetCustomFlags1(pos);
		curFlags &= ~mask; // Zero bits masked
		curFlags |= flag;
		m_pCtxt->SetCustomFlags1(pos, curFlags);
		m_pCtxt->GetNextDiffPosition(pos);
	}
}

/**
 * @brief Checks if current folders are opened from archive file.
 * @return true if we are inside archive, false otherwise.
 */
bool CDirFrame::IsArchiveFolders()
{
	return m_pTempPathContext != NULL;
}
