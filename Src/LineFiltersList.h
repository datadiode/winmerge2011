/** 
 * @file  LineFiltersList.h
 *
 * @brief Declaration file for LineFiltersList class
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _LINEFILTERS_LIST_H_
#define _LINEFILTERS_LIST_H_

/**
 @brief Structure for one line filter.
 */
struct LineFilterItem
{
	bool enabled; /**< Is filter enabled? */
	String filterStr; /**< Filter string */
	LineFilterItem() : enabled(false) { }
};

/**
 @brief List of line filters.
 */
class LineFiltersList
{
public:
	LineFiltersList();
	LineFiltersList(const LineFiltersList *list);
	~LineFiltersList();

	void AddFilter(LPCTSTR filter, bool enabled);
	int GetCount() const;
	void Empty();
	String GetAsString() const;
	const LineFilterItem & GetAt(size_t ind) const;
	bool Compare(const LineFiltersList *list) const;

	void SaveFilters();

	void Import(LPCTSTR filters);

private:
	stl::vector<LineFilterItem*> m_items; /**< List for linefilter items */
};

#endif // _LINEFILTERS_LIST_H_
