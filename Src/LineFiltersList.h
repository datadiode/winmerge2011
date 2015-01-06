/** 
 * @file  LineFiltersList.h
 *
 * @brief Declaration file for LineFiltersList class
 */
#pragma once

/**
 @brief Structure for one line filter.
 */
struct LineFilterItem
{
	int usage; /**< Is filter enabled? */
	HRESULT hr; /**< Is filter erroneous? */
	String filterStr; /**< Filter string */
	LineFilterItem() : usage(0), hr(S_OK) { }
};

/**
 @brief List of line filters.
 */
extern class LineFiltersList
{
public:

	void AddFilter(LPCTSTR filter, int usage);
	stl_size_t GetCount() const { return m_items.size(); }
	void Empty() { m_items.clear(); }
	LineFilterItem &GetAt(stl_size_t i) { return m_items[i]; }
	bool Compare(LineFiltersList &);

	void LoadFilters();
	void SaveFilters();
	void swap(LineFiltersList &other) { m_items.swap(other.m_items); }

private:
	std::vector<LineFilterItem> m_items; /**< List for linefilter items */
} globalLineFilters;
