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
#include "StdAfx.h"
#include <process.h>
#include "FileFilterHelper.h"
#include "DiffContext.h"
#include "paths.h"
#include "CompareStats.h"
#include "codepage_detect.h"
#include "Common/version.h"
#include "OptionsMgr.h"

/**
 * @brief Force compare to be single-threaded.
 * Set this to 0 in order to single step through entire compare process all
 * in a single thread. Either edit this line, or breakpoint & change it in
 * CompareDirectories() below.
 *
 * If you are going to debug compare procedure, you most probably need to set
 * this to 0. As Visual Studio seems to have real problems with debugging
 * these threads otherwise.
 */

String CDiffContext::GetLeftFilepathAndName(const DIFFITEM *di) const
{
	String path = GetLeftFilepath(di);
	return path.empty() ? path : paths_ConcatPath(path, di->left.filename);
}

String CDiffContext::GetRightFilepathAndName(const DIFFITEM *di) const
{
	String path = GetRightFilepath(di);
	return path.empty() ? path : paths_ConcatPath(path, di->right.filename);
}

/**
 * @brief Update info in item in result list from disk.
 * This function updates result list item's file information from actual
 * file in the disk. This updates info like date, size and attributes.
 * @param [in] diffpos DIFFITEM to update.
 * @param [in] bLeft Update left-side info.
 * @param [in] bRight Update right-side info.
 */
void CDiffContext::UpdateStatusFromDisk(DIFFITEM *di, bool bLeft, bool bRight, bool bMakeWritable)
{
	if (bLeft)
	{
		di->left.ClearPartial();
		if (!di->isSideRightOnly())
			UpdateInfoFromDiskHalf(di, true, bMakeWritable);
	}
	if (bRight)
	{
		di->right.ClearPartial();
		if (!di->isSideLeftOnly())
			UpdateInfoFromDiskHalf(di, false, bMakeWritable);
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
bool CDiffContext::UpdateInfoFromDiskHalf(DIFFITEM *di, bool bLeft, bool bMakeWritable)
{
	String filepath = bLeft ? GetLeftFilepathAndName(di) : GetRightFilepathAndName(di);
	DiffFileInfo &dfi = bLeft ? di->left : di->right;

	if (bMakeWritable)
	{
		DWORD attr = GetFileAttributes(filepath.c_str());
		if ((attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY)) == FILE_ATTRIBUTE_READONLY)
			SetFileAttributes(filepath.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
	}

	if (!dfi.Update(filepath.c_str()))
		return false;

	if (!di->isDirectory())
	{
		UpdateVersion(di, bLeft);
		GuessCodepageEncoding(filepath.c_str(), &dfi.encoding, m_bGuessEncoding);
	}

	return true;
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
	static const TCHAR extensions[] = _T("EXE;DLL;SYS;DRV;OCX;CPL;SCR");
	return *ext == _T('.') && PathMatchSpec(ext + 1, extensions);
}

/**
 * @brief Load file version from disk.
 * Update fileversion for given item and side from disk. Note that versions
 * are read from only some filetypes. See CheckFileForVersion() function
 * for list of files to check versions.
 * @param [in,out] di DIFFITEM to update.
 * @param [in] bLeft If TRUE left-side file is updated, right-side otherwise.
 */
void CDiffContext::UpdateVersion(DIFFITEM *di, BOOL bLeft) const
{
	DiffFileInfo &dfi = bLeft ? di->left : di->right;
	// Check only binary files
	dfi.version.Clear();
	dfi.versionChecked = DiffFileInfo::VersionMissing;

	if (di->isDirectory())
		return;
	
	String spath;
	if (bLeft)
	{
		if (di->isSideRightOnly())
			return;
		LPCTSTR ext = PathFindExtension(di->left.filename.c_str());
		if (!CheckFileForVersion(ext))
			return;
		spath = GetLeftFilepathAndName(di);
	}
	else
	{
		if (di->isSideLeftOnly())
			return;
		LPCTSTR ext = PathFindExtension(di->right.filename.c_str());
		if (!CheckFileForVersion(ext))
			return;
		spath = GetRightFilepathAndName(di);
	}
	
	// Get version info if it exists
	CVersionInfo ver(spath.c_str());
	if (ver.GetFixedFileVersion(dfi.version.m_versionMS, dfi.version.m_versionLS))
		dfi.versionChecked = DiffFileInfo::VersionPresent;
}

/**
 * @brief Default constructor.
 */
CDiffContext::CDiffContext
(
	CompareStats *pCompareStats,
	HWindow *pWindow,
	LPCTSTR pszLeft,
	LPCTSTR pszRight,
	int nRecursive,
	DWORD dwContext
) :	m_pCompareStats(pCompareStats),
	m_pWindow(pWindow),
	m_nRecursive(nRecursive),
	m_dwContext(dwContext),
	m_bWalkUniques(true),
	m_bSelfCompare(true),
	m_paths(2),
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
	if (m_nCompareThreads != 0)
	{
		m_pCompareStats->Wait();
	}
	else do
	{
		MSG msg;
		while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	} while (m_pCompareStats->MsgWait());
	return m_bAborting;
}

/**
 * @brief Start and run directory compare thread.
 * @return Success (1) or error for thread. Currently always 1.
 */
void CDiffContext::CompareDirectories(bool bOnlyRequested)
{
	ASSERT(m_hSemaphore == NULL);

	m_bAborting = false;
	m_bOnlyRequested = bOnlyRequested;

	m_hSemaphore = CreateSemaphore(0, 0, LONG_MAX, 0);
	InitializeCriticalSection(&m_csCompareThread);
	m_diCompareThread = NULL;
	
	m_nCompareThreads = COptionsMgr::Get(OPT_CMP_COMPARE_THREADS);
	if (m_nCompareThreads < 0)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		m_nCompareThreads += sysinfo.dwNumberOfProcessors;
		if (m_nCompareThreads < 0)
			m_nCompareThreads = 0;
	}
	if (m_nCompareThreads == 0)
	{
		if (!m_bOnlyRequested)
		{
			DiffThreadCollect(this);
			ReleaseSemaphore(m_hSemaphore, 1, 0);
		}
		DiffThreadCompare(this);
	}
	else
	{
		if (!m_bOnlyRequested)
			_beginthread(DiffThreadCollect, 0, this);
		int nThreads = m_nCompareThreads;
		do
		{
			if (!_beginthread(DiffThreadCompare, 0, this))
				InterlockedDecrement(&m_nCompareThreads);
		} while (--nThreads != 0);
	}
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
	try
	{
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
		myStruct->DirScan_GetItems(subdir, false, subdir, false, depth, NULL);

#ifdef _DEBUG
		_CrtMemCheckpoint(&memStateAfter);
		_CrtMemDifference(&memStateDiff, &memStateBefore, &memStateAfter);
		_CrtMemDumpStatistics(&memStateDiff);
#endif
	}
	catch (OException *e)
	{
		e->ReportError(NULL, MB_ICONSTOP | MB_TOPMOST);
		delete e;
	}

	// ReleaseSemaphore() once again to signal that collect phase is ready
	ReleaseSemaphore(myStruct->m_hSemaphore, myStruct->m_nCompareThreads, 0);
}

/**
 * @brief Folder compare thread function.
 *
 * Compares items in item list. After compare is ready
 * sends message to UI so UI can update itself.
 * @param [in] lpParam Pointer to parameter structure.
 */
void CDiffContext::DiffThreadCompare(LPVOID lpParam)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CDiffContext *const myStruct = reinterpret_cast<CDiffContext *>(lpParam);
	// Now do all pending file comparisons
	if (myStruct->m_bOnlyRequested)
		myStruct->DirScan_CompareRequestedItems();
	else
		myStruct->DirScan_CompareItems();
	if (InterlockedDecrement(&myStruct->m_nCompareThreads) <= 0)
	{
		CloseHandle(myStruct->m_hSemaphore);
		myStruct->m_hSemaphore = NULL;
		DeleteCriticalSection(&myStruct->m_csCompareThread);
		// Send message to UI to update
		myStruct->m_pWindow->PostMessage(MSG_UI_UPDATE);
	}
	CoUninitialize();
}
