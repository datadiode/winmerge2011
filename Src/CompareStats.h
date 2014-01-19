/**
 *  @file CompareStats.h
 *
 *  @brief Declaration of class CompareStats
 */ 
#ifndef _COMPARESTATS_H_
#define _COMPARESTATS_H_

struct DIFFITEM;
class CDiffContext;

/**
 * @brief Class holding directory compare stats.
 *
 * This class is used for sharing compare stats between dir compare
 * classes. CDiffContext updates statuses. GUI (compare statepane) updates UI.
 */
class CompareStats
{
public:

	/**
	 * @brief Folder compare icon indexes.
	 * This enum defines indexes for imagelist used for folder compare icons.
	 * Note that this enum must be in synch with code in OnInitialUpdate() and
	 * GetColImage(). Also remember that icons are in resource file...
	 */
	enum RESULT
	{
		DIFFIMG_LUNIQUE,
		DIFFIMG_RUNIQUE,
		DIFFIMG_DIFF,
		DIFFIMG_SAME,
		DIFFIMG_BINSAME,
		DIFFIMG_BINDIFF,
		DIFFIMG_LDIRUNIQUE,
		DIFFIMG_RDIRUNIQUE,
		DIFFIMG_SKIP,
		DIFFIMG_DIRSKIP,
		DIFFIMG_DIRDIFF,
		DIFFIMG_DIRSAME,
		DIFFIMG_DIR,
		DIFFIMG_ERROR,
		DIFFIMG_DIRUP,
		DIFFIMG_DIRUP_DISABLE,
		DIFFIMG_ABORT,
		DIFFIMG_TEXTDIFF,
		DIFFIMG_TEXTSAME,
		N_DIFFIMG
	};

	CompareStats();
	~CompareStats();
	void SetCompareThreadCount(LONG nCompareThreads)
	{
		m_rgThreadState.resize(nCompareThreads);
	}
	void BeginCompare(const DIFFITEM *di, LONG iCompareThread)
	{
		ThreadState &rThreadState = m_rgThreadState[iCompareThread];
		rThreadState.m_nHitCount = 0;
		rThreadState.m_pDiffItem = di;
	}
	void AddItem(const DIFFITEM *);
	void IncreaseTotalItems()
	{
		InterlockedIncrement(&m_nTotalItems);
	}
	void SetTotalItems(long nTotalItems)
	{
		m_nTotalItems = nTotalItems;
	}
	long GetTotalItems() const
	{
		return m_nTotalItems;
	}
	int GetCount(CompareStats::RESULT result) const;
	int GetComparedItems() const { return m_nComparedItems; }
	const DIFFITEM *GetCurDiffItem();
	void Reset();
	static RESULT GetColImage(const DIFFITEM *);
	bool MsgWait() const
	{
		// MsgWaitForMultipleObjects() is expensive so avoid it if possible
		if (paused)
			MsgWaitForMultipleObjects(1, &m_hEvent, FALSE, INFINITE, QS_ALLINPUT);
		return paused;
	}
	void Wait() const
	{
		// WaitForSingleObject() is expensive so avoid it if possible
		if (paused)
			WaitForSingleObject(m_hEvent, INFINITE);
	}
	void Pause()
	{
		paused = true;
		ResetEvent(m_hEvent);
	}
	void Continue()
	{
		paused = false;
		SetEvent(m_hEvent);
	}
private:
	HANDLE m_hEvent;
	bool paused;
	long m_counts[N_DIFFIMG]; /**< Table storing result counts */
	long m_nTotalItems; /**< Total items found to compare */
	long m_nComparedItems; /**< Compared items so far */
	struct ThreadState
	{
		LONG m_nHitCount;
		const DIFFITEM *m_pDiffItem;
		ThreadState()
			: m_nHitCount(0)
			, m_pDiffItem(NULL)
		{
		}
	};
	stl::vector<ThreadState> m_rgThreadState;
};

#endif // _COMPARESTATS_H_
