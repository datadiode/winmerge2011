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
 *  @file DiffContext.cpp
 *
 *  @brief Implementation of CDiffContext
 */ 
// ID line follows -- this is updated by SVN
// $Id$
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <process.h>
#include "DiffContext.h"
#include "paths.h"
#include "CompareStats.h"
#include "codepage_detect.h"
#include "DiffItemList.h"
#include "Common/version.h"

/**
 * @brief Update info in item in result list from disk.
 * This function updates result list item's file information from actual
 * file in the disk. This updates info like date, size and attributes.
 * @param [in] diffpos DIFFITEM to update.
 * @param [in] bLeft Update left-side info.
 * @param [in] bRight Update right-side info.
 */
void CDiffContext::UpdateStatusFromDisk(UINT_PTR diffpos, BOOL bLeft, BOOL bRight)
{
	DIFFITEM &di = GetDiffAt(diffpos);
	if (bLeft)
	{
		di.left.ClearPartial();
		if (!di.diffcode.isSideRightOnly())
			UpdateInfoFromDiskHalf(di, TRUE);
	}
	if (bRight)
	{
		di.right.ClearPartial();
		if (!di.diffcode.isSideLeftOnly())
			UpdateInfoFromDiskHalf(di, FALSE);
	}
}

/**
 * @brief Update file information from disk for DIFFITEM.
 * This function updates DIFFITEM's file information from actual file in
 * the disk. This updates info like date, size and attributes.
 * @param [in, out] di DIFFITEM to update (selected side, see bLeft param).
 * @param [in] bLeft If TRUE left side information is updated,
 *  right side otherwise.
 * @return TRUE if file exists
 */
BOOL CDiffContext::UpdateInfoFromDiskHalf(DIFFITEM & di, BOOL bLeft)
{
	String filepath;

	if (bLeft)
		filepath = paths_ConcatPath(di.GetLeftFilepath(GetLeftPath()), di.left.filename);
	else
		filepath = paths_ConcatPath(di.GetRightFilepath(GetRightPath()), di.right.filename);

	DiffFileInfo & dfi = bLeft ? di.left : di.right;
	if (!dfi.Update(filepath.c_str()))
		return FALSE;
	UpdateVersion(di, bLeft);
	GuessCodepageEncoding(filepath.c_str(), &dfi.encoding, m_bGuessEncoding);
	return TRUE;
}

/**
 * @brief Determine if file is one to have a version information.
 * This function determines if the given file has a version information
 * attached into it in resource. This is done by comparing file extension to
 * list of known filename extensions usually to have a version information.
 * @param [in] ext Extension to check.
 * @return true if extension has version info, false otherwise.
 */
static bool CheckFileForVersion(LPCTSTR ext)
{
	if (!lstrcmpi(ext, _T(".EXE")) || !lstrcmpi(ext, _T(".DLL")) || !lstrcmpi(ext, _T(".SYS")) ||
	    !lstrcmpi(ext, _T(".DRV")) || !lstrcmpi(ext, _T(".OCX")) || !lstrcmpi(ext, _T(".CPL")) ||
	    !lstrcmpi(ext, _T(".SCR")) || !lstrcmpi(ext, _T(".LANG")))
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Load file version from disk.
 * Update fileversion for given item and side from disk. Note that versions
 * are read from only some filetypes. See CheckFileForVersion() function
 * for list of files to check versions.
 * @param [in,out] di DIFFITEM to update.
 * @param [in] bLeft If TRUE left-side file is updated, right-side otherwise.
 */
void CDiffContext::UpdateVersion(DIFFITEM & di, BOOL bLeft) const
{
	DiffFileInfo & dfi = bLeft ? di.left : di.right;
	// Check only binary files
	dfi.version.Clear();
	dfi.versionChecked = DiffFileInfo::VersionMissing;

	if (di.diffcode.isDirectory())
		return;
	
	String spath;
	if (bLeft)
	{
		if (di.diffcode.isSideRightOnly())
			return;
		LPCTSTR ext = PathFindExtension(di.left.filename.c_str());
		if (!CheckFileForVersion(ext))
			return;
		spath = di.GetLeftFilepath(GetLeftPath());
		spath = paths_ConcatPath(spath, di.left.filename);
	}
	else
	{
		if (di.diffcode.isSideLeftOnly())
			return;
		LPCTSTR ext = PathFindExtension(di.right.filename.c_str());
		if (!CheckFileForVersion(ext))
			return;
		spath = di.GetRightFilepath(GetRightPath());
		spath = paths_ConcatPath(spath, di.right.filename);
	}
	
	// Get version info if it exists
	CVersionInfo ver(spath.c_str());
	if (ver.GetFixedFileVersion(dfi.version.m_versionMS, dfi.version.m_versionLS))
		dfi.versionChecked = DiffFileInfo::VersionPresent;
}

/**
 * @brief Force compare to be single-threaded.
 * Set this to true in order to single step through entire compare process all
 * in a single thread. Either edit this line, or breakpoint & change it in
 * CompareDirectories() below.
 *
 * If you are going to debug compare procedure, you most probably need to set
 * this to true. As Visual Studio seems to have real problems with debugging
 * these threads otherwise.
 */
static bool bSinglethreaded = false;

/**
 * @brief Default constructor.
 */
CDiffContext::CDiffContext
(
	CompareStats *pCompareStats,
	HWindow *pWindow,
	LPCTSTR pszLeft,
	LPCTSTR pszRight,
	int nRecursive
) : m_piFilterGlobal(NULL),
	m_nCompMethod(0),
	m_bIgnoreSmallTimeDiff(false),
	m_pCompareStats(pCompareStats),
	m_pWindow(pWindow),
	m_bStopAfterFirstDiff(false),
	m_nRecursive(nRecursive),
	m_bWalkUniques(true),
	m_paths(2),
#pragma warning(disable:warning_this_used_in_base_member_initializer_list)
	m_folderCmp(this),
#pragma warning(default:warning_this_used_in_base_member_initializer_list)
	m_bOnlyRequested(false),
	m_hSemaphore(NULL),
	m_bAborting(false),
	m_options(NULL)
{
	m_paths[0] = paths_GetLongPath(pszLeft);
	m_paths[1] = paths_GetLongPath(pszRight);
}

/**
 * @brief Destructor, release resources.
 */
CDiffContext::~CDiffContext()
{
	ASSERT(m_hSemaphore == NULL);
}

/**
 * @brief runtime interface for child thread, called on child thread
 */
bool CDiffContext::ShouldAbort() const
{
	if (bSinglethreaded)
	{
		MSG msg;
		while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	return m_bAborting;
}

/**
 * @brief Start and run directory compare thread.
 * @return Success (1) or error for thread. Currently always 1.
 */
UINT CDiffContext::CompareDirectories(bool bOnlyRequested)
{
	m_folderCmp.Reset();

	ASSERT(m_hSemaphore == NULL);

	m_bAborting = false;
	m_bOnlyRequested = bOnlyRequested;

	m_hSemaphore = CreateSemaphore(0, 0, LONG_MAX, 0);

	if (bSinglethreaded)
	{
		if (!m_bOnlyRequested)
			DiffThreadCollect(this);
		DiffThreadCompare(this);
	}
	else
	{
		if (!m_bOnlyRequested)
			_beginthread(DiffThreadCollect, 0, this);
		_beginthread(DiffThreadCompare, 0, this);
	}

	return 1;
}

/**
 * @brief Item collection thread function.
 *
 * This thread is responsible for finding and collecting all items to compare
 * to the item list.
 * @param [in] lpParam Pointer to parameter structure.
 * @return Thread's return value.
 */
void CDiffContext::DiffThreadCollect(LPVOID lpParam)
{
	CDiffContext *myStruct = reinterpret_cast<CDiffContext *>(lpParam);

	ASSERT(!myStruct->m_bOnlyRequested);

	int depth = myStruct->m_nRecursive ? -1 : 0;

	String subdir; // blank to start at roots specified in diff context
#ifdef _DEBUG
	_CrtMemState memStateBefore;
	_CrtMemState memStateAfter;
	_CrtMemState memStateDiff;
	_CrtMemCheckpoint(&memStateBefore);
#endif

	// Build results list (except delaying file comparisons until below)
	myStruct->DirScan_GetItems(subdir, false, subdir, false,
			depth, NULL, myStruct->m_bWalkUniques);

#ifdef _DEBUG
	_CrtMemCheckpoint(&memStateAfter);
	_CrtMemDifference(&memStateDiff, &memStateBefore, &memStateAfter);
	_CrtMemDumpStatistics(&memStateDiff);
#endif

	// ReleaseSemaphore() once again to signal that collect phase is ready
	ReleaseSemaphore(myStruct->m_hSemaphore, 1, 0);
}

/**
 * @brief Folder compare thread function.
 *
 * Compares items in item list. After compare is ready
 * sends message to UI so UI can update itself.
 * @param [in] lpParam Pointer to parameter structure.
 * @return Thread's return value.
 */
void CDiffContext::DiffThreadCompare(LPVOID lpParam)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	CDiffContext *const myStruct = reinterpret_cast<CDiffContext *>(lpParam);

	// Now do all pending file comparisons
	if (myStruct->m_bOnlyRequested)
		myStruct->DirScan_CompareRequestedItems(NULL);
	else
		myStruct->DirScan_CompareItems(NULL);

	CloseHandle(myStruct->m_hSemaphore);
	myStruct->m_hSemaphore = NULL;

	// Send message to UI to update
	myStruct->m_pWindow->PostMessage(MSG_UI_UPDATE);

	CoUninitialize();
}
