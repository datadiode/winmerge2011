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
	* @brief Resultcodes we store.
	*/
	enum RESULT
	{
		RESULT_LUNIQUE = 0,
		RESULT_RUNIQUE,
		RESULT_DIFF,
		RESULT_SAME,
		RESULT_BINSAME,
		RESULT_BINDIFF,
		RESULT_LDIRUNIQUE,
		RESULT_RDIRUNIQUE,
		RESULT_SKIP,
		RESULT_DIRSKIP,
		RESULT_DIR,
		RESULT_ERROR,
		RESULT_COUNT  //THIS MUST BE THE LAST ITEM
	};

	CompareStats();
	~CompareStats();
	void AddItem(int code);
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
	static CompareStats::RESULT GetResultFromCode(UINT diffcode);

private:

	long m_counts[RESULT_COUNT]; /**< Table storing result counts */
	long m_nTotalItems; /**< Total items found to compare */
	long m_nComparedItems; /**< Compared items so far */
};

#endif // _COMPARESTATS_H_
