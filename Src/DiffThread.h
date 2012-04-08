/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  DiffThread.h
 *
 * @brief Declaration file for CDiffThread
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _DIFFTHREAD_H
#define _DIFFTHREAD_H

#include "diffcontext.h"
#include "FileLocation.h"
#include "FolderCmp.h"
#include "IAbortable.h"

class DiffThreadAbortable;

/**
 * @brief Class for threaded folder compare.
 * This class implements folder compare in two phases and in two threads:
 * - first thread collects items to compare to compare-time list
 *   (m_diffList).
 * - second threads compares items in the list.
 */
class CDiffThread : public IAbortable
{
public:
	/** @brief Thread's states. */
	enum ThreadState
	{
		THREAD_NOTSTARTED = 0, /**< Thread not started, idle. */
		THREAD_COMPARING, /**< Thread running (comparing). */
		THREAD_COMPLETED, /**< Thread has completed its task. */
	} nThreadState; /**< Thread state. */
	CDiffContext *context; /**< Compare context. */
	UINT msgUIUpdate; /**< Windows message for updating GUI. */
	HWND hWindow; /**< Window getting status updates. */
	bool bOnlyRequested; /**< Compare only requested items? */
	HANDLE hSemaphore; /**< Semaphore for synchronizing threads. */

	FolderCmp folderCmp;

	void CompareDiffItem(DIFFITEM &);
// creation and use, called on main thread
	CDiffThread();
	~CDiffThread();
	void SetContext(CDiffContext *);
	UINT CompareDirectories();
	void SetHwnd(HWND hWnd);
	void SetMessageIDs(UINT updateMsg);
	void SetCompareSelected(bool bSelected = false);

// runtime interface for main thread, called on main thread
	UINT GetThreadState() const;
	void Abort() { m_bAborting = true; }
	bool IsAborting() const { return m_bAborting; }

// runtime interface for child thread, called on child thread
	bool ShouldAbort() const;

private:
	bool m_bAborting; /**< Is compare aborting? */
	const static bool casesensitive = false;
	const String empty;
// Thread functions
	static void __cdecl DiffThreadCollect(LPVOID);
	static void __cdecl DiffThreadCompare(LPVOID);
	int DirScan_GetItems(
		const String &leftsubdir, bool bLeftUniq,
		const String &rightsubdir, bool bRightUniq,
		int depth, DIFFITEM *parent, bool bUniques);
	DIFFITEM *AddToList(const String &sLeftDir, const String &sRightDir,
		const DirItem *lent, const DirItem *rent, UINT code, DIFFITEM *parent);
	void SetDiffItemStats(DIFFITEM &);
	void StoreDiffData(const DIFFITEM &);
	bool UpdateDiffItem(DIFFITEM &);
	int DirScan_CompareItems(UINT_PTR parentdiffpos);
	int DirScan_CompareRequestedItems(UINT_PTR parentdiffpos);

	typedef stl::vector<DirItem> DirItemArray;

	void LoadAndSortFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files) const;
	void LoadFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files) const;
	void Sort(DirItemArray *dirs) const;
	int collstr(const String &, const String &) const;
};

#endif /* _DIFFTHREAD_H */
