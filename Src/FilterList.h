/**
 * @file  FilterList.h
 *
 * @brief Declaration file for FilterList.
 */
#pragma once

#include "pcre.h"
#include "Common/MyCom.h"

class LineFiltersList;
struct LineFilterItem;

extern "C" size_t apply_prediffer(struct file_data *current, short side, char *buf, size_t len);

#include "RegExpItem.h"
#include "ScriptItem.h"

/**
 * @brief Regular expression list.
 * This class holds a list of regular expressions for matching strings.
 * The class also provides simple function for matching and remembers the
 * last matched expression.
 */
class FilterList
{
public:
	
	FilterList();
	~FilterList();

	void AddRegExp(LPCTSTR);
	void AddFromIniFile(LPCTSTR);
	void AddFrom(LineFiltersList &);
	void RemoveAllFilters();
	bool HasRegExps() const { return !m_list.empty(); }
	bool HasPredifferRegExps() const { return !m_predifferRegExps.empty(); }
	bool HasPredifferScripts() const { return !m_predifferScripts.empty(); }
	bool HasPrediffers() const { return HasPredifferRegExps() || HasPredifferScripts(); }
	bool Match(int stringlen, const char *string, int codepage = CP_UTF8);

private:
	friend size_t apply_prediffer(struct file_data *current, short side, char *buf, size_t len);

	template<class iterator>
	static void dispose(iterator p, iterator q)
	{
		while (p != q)
			(p++)->dispose();
	}

	HRESULT AddFilter(String &, LPCTSTR, LineFilterItem *);
	int ApplyPredifferRegExps(LPCTSTR filename, char *dst, const char *src, int len) const;
	BSTR ApplyPredifferScripts(LPCTSTR filename, BSTR bstr);
	void ResetPrediffers(short side);

	std::vector<regexp_item> m_list;
	std::vector<regexp_item> m_predifferRegExps;
	std::vector<script_item> m_predifferScripts;
	bool m_utf8;
};
