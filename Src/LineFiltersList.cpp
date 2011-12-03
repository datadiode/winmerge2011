/** 
 * @file  LineFiltersList.cpp
 *
 * @brief Implementation for LineFiltersList class.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "SettingStore.h"
#include "Common/RegKey.h"
#include "LineFiltersList.h"

using stl::vector;

/** @brief Registry key for saving linefilters. */
static const TCHAR FiltersRegPath[] =_T("LineFilters");

/**
 * @brief Read filter list from the options system.
 * @param [in] pOptionsMgr Pointer to options system.
 */
LineFiltersList::LineFiltersList()
{
	if (HKEY h = SettingStore.GetSectionKey(FiltersRegPath))
	{
		CRegKeyEx key = h;
		UINT count = key.ReadDword(_T("Values"), 0);
		for (UINT i = 0; i < count; i++)
		{
			TCHAR valuename[40];
			wsprintf(valuename, _T("Filter%02u"), i);
			String filter = key.ReadString(valuename, _T(""));
			wsprintf(valuename, _T("Enabled%02u"), i);
			bool enabled = key.ReadDword(valuename, 1) != 0;
			AddFilter(filter.c_str(), enabled);
		}
	}
}

/**
 * @brief Destructor, empties the list.
 */
LineFiltersList::~LineFiltersList()
{
	Empty();
}

/**
 * @brief Add new filter to the list.
 * @param [in] filter Filter string to add.
 * @param [in] enabled Is filter enabled?
 */
void LineFiltersList::AddFilter(LPCTSTR filter, bool enabled)
{
	LineFilterItem *item = new LineFilterItem();
	item->enabled = enabled;
	item->filterStr = filter;
	m_items.push_back(item);
}

/**
 * @brief Returns count of items in the list.
 * @return Count of filters in the list.
 */
int LineFiltersList::GetCount() const
{
	return m_items.size();
}

/**
 * @brief Empties the list.
 */
void LineFiltersList::Empty()
{
	while (!m_items.empty())
	{
		LineFilterItem * item = m_items.back();
		delete item;
		m_items.pop_back();
	}
}

/**
 * @brief Returns the filter list as one filter string.
 * This function returns the list of filters as one string that can be
 * given to regular expression engine as filter. Filter strings in
 * the list are separated by "|".
 * @return Filter string.
 */
String LineFiltersList::GetAsString() const
{
	String filter;
	vector<LineFilterItem*>::const_iterator iter = m_items.begin();

	while (iter != m_items.end())
	{
		if ((*iter)->enabled)
		{
			if (!filter.empty())
				filter += _T("|");
			filter += (*iter)->filterStr;
		}
		++iter;
	}
	return filter;	
}

/**
 * @brief Return filter from given index.
 * @param [in] ind Index of filter.
 * @return Filter item from the index. If the index is beyond table limit,
 *  return the last item in the list.
 */
const LineFilterItem & LineFiltersList::GetAt(size_t ind) const
{
	if (ind < m_items.size())
		return *m_items[ind];
	else
		return *m_items.back();
}

/**
 * @brief Clone filter list from another list.
 * This function clones filter list from another list. Current items in the
 * list are removed and new items added from the given list.
 * @param [in] list List to clone.
 */
LineFiltersList::LineFiltersList(const LineFiltersList *list)
{
	int count = list->GetCount();
	for (int i = 0; i < count; i++)
	{
		const LineFilterItem &item = list->GetAt(i);
		AddFilter(item.filterStr.c_str(), item.enabled);
	}
}

/**
 * @brief Compare filter lists.
 * @param [in] list List to compare.
 * @return true if lists are identical, false otherwise.
 */
bool LineFiltersList::Compare(const LineFiltersList *list) const
{
	if (list->GetCount() != GetCount())
		return false;

	for (int i = 0; i < GetCount(); i++)
	{
		const LineFilterItem &item1 = list->GetAt(i);
		const LineFilterItem &item2 = GetAt(i);

		if (item1.enabled != item2.enabled)
			return false;

		if (item1.filterStr != item2.filterStr)
			return false;
	}
	return true;
}

/**
 * @brief Save linefilters to options system.
 */
void LineFiltersList::SaveFilters()
{
	if (HKEY h = SettingStore.GetSectionKey(FiltersRegPath, CREATE_ALWAYS))
	{
		CRegKeyEx key = h;
		UINT count = m_items.size();
		key.WriteDword(_T("Values"), count);
		for (UINT i = 0; i < count; i++)
		{
			LineFilterItem *item = m_items[i];
			TCHAR valuename[40];
			wsprintf(valuename, _T("Filter%02u"), i);
			key.WriteString(valuename, item->filterStr.c_str());
			wsprintf(valuename, _T("Enabled%02u"), i);
			key.WriteDword(valuename, item->enabled);
		}
	}
}

/**
 * @brief Import old-style filter string into new system.
 * This function imports old-style (2.6.x and earlier) line filters
 * to new linefilter system. Earlier linefilters were saved as one
 * string to registry.
 * @param [in] filters String containing line filters in old-style.
 */
void LineFiltersList::Import(LPCTSTR filters)
{
	const TCHAR sep[] = _T("\r\n");
	TCHAR *p_filters = (TCHAR *)&filters[0];
	TCHAR *token;
	
	if (filters != NULL && _tcslen(filters) > 0)
	{
		// find each regular expression and add to list
		token = _tcstok(p_filters, sep);
		while (token)
		{
			AddFilter(token, TRUE);
			token = _tcstok(NULL, sep);
		}
		SaveFilters();
	}
}
