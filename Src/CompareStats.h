/**
 *  @file CompareStats.h
 *
 *  @brief Declaration of class CompareStats
 */ 
//
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _COMPARESTATS_H_
#define _COMPARESTATS_H_

struct DIFFITEM;


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
	void Reset();
	static RESULT GetColImage(const DIFFITEM *);

private:

	long m_counts[N_DIFFIMG]; /**< Table storing result counts */
	long m_nTotalItems; /**< Total items found to compare */
	long m_nComparedItems; /**< Compared items so far */
};

#endif // _COMPARESTATS_H_
