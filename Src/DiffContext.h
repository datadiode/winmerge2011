/**
 *  @file DiffContext.h
 *
 *  @brief Declarations of CDiffContext and diff structures
 */
#pragma once

#include "CompareOptions.h"
#include "DiffFileInfo.h"
#include "DiffItemList.h"
#include "FileLocation.h"
#include "FolderCmp.h"

class IDiffFilter;
class CompareStats;

/**
 * The folder compare context.
 * This class holds data of the current folder compare. There are paths
 * to compare, filters used, compare options etc. And compare results list
 * is also contained in this class. Many compare classes and functions have
 * a pointer to instance of this class. 
 *
 * Folder compare comprises two phases executed in two threads:
 * - first thread collects items to compare to compare-time list
 *   (m_diffList).
 * - second threads compares items in the list.
 */
class CDiffContext
	: ZeroInit<CDiffContext>
	, public ListEntry
	, public DiffItemList
{
public:
	/** @brief Special values for difference counts. */
	enum
	{
		DIFFS_UNKNOWN = -1, /**< Difference count unknown (generally). */
		DIFFS_UNKNOWN_QUICKCOMPARE = -9, /**< Unknown because of quick-compare method. */
	};

	CDiffContext(CompareStats *, HWindow *,
		LPCTSTR pszLeft, LPCTSTR pszRight, int nRecursive, DWORD dwContext);
	~CDiffContext();

	void UpdateVersion(DIFFITEM *, BOOL bLeft) const;

	//@{
	/**
	 * @name Path accessor functions.
	 *
	 * These functions return left/right path associated to DiffContext.
	 * There is no setter fuctions and path can be set only via constructor.
	 * Normalized paths are preferred to use - short paths are expanded
	 * and trailing slashes removed (except from root path).
	 */
	/**
	 * Get left-side compare path in normalized form.
	 * @return full path in left-side.
	 */
	const String &GetLeftPath() const { return m_paths[0]; }
	/**
	 * Get right-side compare path in normalized form.
	 * @return full path in left-side.
	 */
	const String &GetRightPath() const { return m_paths[1]; }
	//@}

	String GetLeftFilepath(const DIFFITEM *di) const { return di->GetLeftFilepath(GetLeftPath()); }
	String GetRightFilepath(const DIFFITEM *di) const { return di->GetRightFilepath(GetRightPath()); }
	String GetLeftFilepathAndName(const DIFFITEM *di) const;
	String GetRightFilepathAndName(const DIFFITEM *di) const;

	// change an existing difference
	bool UpdateInfoFromDiskHalf(DIFFITEM *, bool bLeft, bool bMakeWritable = false);
	void UpdateStatusFromDisk(DIFFITEM *, bool bLeft, bool bRight, bool bMakeWritable = false);

	IDiffFilter *m_piFilterGlobal; /**< Interface for file filtering. */

	DIFFOPTIONS m_options; /**< Generalized compare options. */

	bool m_bGuessEncoding; /**< Guess encoding from file content. */
	bool m_bIgnoreSmallTimeDiff; /**< Ignore small timedifferences when comparing by date */

	/**
	 * Optimize compare by stopping after first difference.
	 * In some compare methods (currently quick compare) we can stop the
	 * compare right after finding the first difference. This speeds up the
	 * compare, but also causes compare statistics to be inaccurate.
	 */
	bool m_bStopAfterFirstDiff;

	/**
	 * Threshold size for switching to quick compare.
	 * When diffutils compare is selected, files bigger (in bytes) than this
	 * value are compared using Quick compare. This is because diffutils simply
	 * cannot compare large files. And large files are usually binary files.
	 */
	int m_nQuickCompareLimit;

	/**
	 * Self-compare unique files to detect encoding and EOL style.
	 *
	 * This value is true by default.
	 */
	bool m_bSelfCompare;

	/**
	 * Walk into unique folders and add contents.
	 * This enables/disables walking into unique folders. If we don't walk into
	 * unique folders, they are shown as such in folder compare results. If we
	 * walk into unique folders, we'll show all files in the unique folder and
	 * in possible subfolders.
	 *
	 * This value is true by default.
	 */
	bool m_bWalkUniques;

	/**
	 * The compare method used.
	 */
	int m_nCompMethod;

	const DWORD m_dwContext; /**< Context code used with CLearCase mrgman files */

	bool UpdateDiffItem(DIFFITEM *);
// creation and use, called on main thread
	void CompareDirectories(bool bOnlyRequested);

// runtime interface for main thread, called on main thread
	bool IsBusy() const { return m_hSemaphore != NULL; }
	void Abort() { m_bAborting = true; }
	bool IsAborting() const { return m_bAborting; }

// runtime interface for child thread, called on child thread
	bool ShouldAbort() const;

private:
	std::vector<String> m_paths; /**< (root) paths for this context */
	HANDLE m_hSemaphore; /**< Semaphore for synchronizing threads. */
	CompareStats *const m_pCompareStats; /**< Pointer to compare statistics */
	HWindow *const m_pWindow; /**< Window getting status updates. */
	DIFFITEM *m_diCompareThread;
	CRITICAL_SECTION m_csCompareThread;
	LONG m_nCompareThreads;
	LONG m_iCompareThread;
	bool m_bAborting; /**< Is compare aborting? */
	bool m_bOnlyRequested; /**< Compare only requested items? */
	const int m_nRecursive; /**< Do we include subfolders to compare? */
	const String empty;
// Thread functions
	DWORD DiffThreadCollect();
	DWORD DiffThreadCompare();
	int DirScan_GetItems(
		const String &leftsubdir, bool bLeftUniq,
		const String &rightsubdir, bool bRightUniq,
		int depth, DIFFITEM *parent);
	DIFFITEM *AddToList(const String &sLeftDir, const String &sRightDir,
		const DirItem *lent, const DirItem *rent, UINT code, DIFFITEM *parent);
	void CompareDiffItem(FolderCmp &, DIFFITEM *);
	void SetDiffItemStats(const FolderCmp &, DIFFITEM *);
	void StoreDiffData(const DIFFITEM *);
	void DirScan_CompareItems();
	void DirScan_CompareRequestedItems();

	typedef std::vector<DirItem> DirItemArray;

	void LoadAndSortFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files, int side) const;
	void LoadFiles(LPCTSTR sDir, DirItemArray *dirs, DirItemArray *files, int side) const;
	void Sort(DirItemArray *dirs) const;
};
