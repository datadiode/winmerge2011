/** 
 * @file  LineFiltersList.cpp
 *
 * @brief Implementation for LineFiltersList class.
 */
#include "StdAfx.h"
#include "SettingStore.h"
#include "Common/RegKey.h"
#include "LineFiltersList.h"

using std::vector;

/** @brief Registry key for saving linefilters. */
static const TCHAR FiltersRegPath[] =_T("LineFilters");

LineFiltersList globalLineFilters;

/**
 * @brief Read filter list from the options system.
 */
void LineFiltersList::LoadFilters()
{
	if (CRegKeyEx key = SettingStore.GetSectionKey(FiltersRegPath))
	{
		DWORD count = 0;
		SettingStore.RegQueryInfoKey(key, &count);
		count /= 2;
		for (DWORD i = 0; i < count; i++)
		{
			TCHAR valuename[40];
			wsprintf(valuename, _T("Filter%02u"), i);
			String filter = key.ReadString(valuename, _T(""));
			wsprintf(valuename, _T("Enabled%02u"), i);
			int usage = key.ReadDword(valuename, 1);
			AddFilter(filter.c_str(), usage);
		}
	}
}

/**
 * @brief Add new filter to the list.
 * @param [in] filter Filter string to add.
 * @param [in] enabled Is filter enabled?
 */
void LineFiltersList::AddFilter(LPCTSTR filter, int usage)
{
	LineFilterItem item;
	item.usage = usage;
	item.filterStr = filter;
	m_items.push_back(item);
}

/**
 * @brief Compare filter lists.
 * @param [in] list List to compare.
 * @return true if lists are identical, false otherwise.
 */
bool LineFiltersList::Compare(LineFiltersList &list)
{
	if (list.GetCount() != GetCount())
		return false;

	for (stl_size_t i = 0; i < GetCount(); i++)
	{
		LineFilterItem &item1 = list.GetAt(i);
		LineFilterItem &item2 = GetAt(i);

		if (item1.usage != item2.usage)
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
	DWORD count = m_items.size();
	if (CRegKeyEx key = SettingStore.GetSectionKey(FiltersRegPath, 2 * count))
	{
		for (DWORD i = 0; i < count; i++)
		{
			const LineFilterItem &item = m_items[i];
			TCHAR valuename[40];
			wsprintf(valuename, _T("Filter%02u"), i);
			key.WriteString(valuename, item.filterStr.c_str());
			wsprintf(valuename, _T("Enabled%02u"), i);
			key.WriteDword(valuename, item.usage);
		}
	}
}
