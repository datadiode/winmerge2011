/**
 * @file  FilterList.h
 *
 * @brief Declaration file for FilterList.
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _FILTERLIST_H_
#define _FILTERLIST_H_

#include "pcre.h"
#include "Common/MyCom.h"

class LineFiltersList;
struct LineFilterItem;

extern "C" size_t apply_prediffer(struct file_data *current, short side, char *buf, size_t len);

/**
 * @brief Container for one filtering rule / compiled expression.
 * This structure holds compiled regular expression and a original expression
 * as a string. We need the original expression string in case we want to
 * know which regular expression did match.
 */
struct filter_item
{
	HString *filterString; /** Original regular expression string */
	HString *injectString; /** String to inject in place of submatches */
	HString *filenameSpec; /** Optional target filename patterns */
	pcre *pRegExp; /**< Compiled regular expression */
	pcre_extra *pRegExpExtra; /**< Additional information got from regex study */
	int options;
	bool global;

	filter_item()
		: filterString(NULL)
		, injectString(NULL)
		, filenameSpec(NULL)
		, pRegExp(NULL)
		, pRegExpExtra(NULL)
		, options(PCRE_UTF8)
		, global(false)
	{
	}
	const char *assign(LPCTSTR pch, size_t cch);
	bool compile()
	{
		const char *errormsg = NULL;
		int erroroffset = 0;
		if (pcre *regexp = pcre_compile(
			filterString->A, options, &errormsg, &erroroffset, NULL))
		{
			errormsg = NULL;
			pRegExp = regexp;
			pRegExpExtra = pcre_study(regexp, 0, &errormsg);
			return true;
		}
		else
		{
			filterString->Free();
			return false;
		}
	}
	int execute(const char *subject, int length, int start,
		int options, int *offsets, int offsetcount) const
	{
		return pcre_exec(pRegExp, pRegExpExtra,
			subject, length, start, options, offsets, offsetcount);
	}
	void dispose()
	{
		filterString->Free();
		injectString->Free();
		filenameSpec->Free();
		pcre_free(pRegExp);
		pcre_free(pRegExpExtra);
	}
};

struct script_item
{
	script_item(LineFilterItem *item = NULL)
		: item(item)
		, filenameSpec(NULL)
		, object(NULL)
	{
	}
	HString *filenameSpec; /** Optional target filename patterns */
	LineFilterItem *item;
	IDispatch *object;
	CMyDispId Reset;
	CMyDispId ProcessLine;
	void dispose()
	{
		filenameSpec->Free();
		if (object)
			object->Release();
	}
};

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
	void AddFrom(LineFiltersList &);
	void RemoveAllFilters();
	bool HasRegExps() const { return !m_list.empty(); }
	bool HasPredifferRegExps() const { return !m_predifferRegExps.empty(); }
	bool HasPredifferScripts() const { return !m_predifferScripts.empty(); }
	bool HasPrediffers() const { return HasPredifferRegExps() || HasPredifferScripts(); }
	bool Match(size_t stringlen, const char *string, int codepage = CP_UTF8);

private:
	friend size_t apply_prediffer(struct file_data *current, short side, char *buf, size_t len);

	template<class iterator>
	static void dispose(iterator p, iterator q)
	{
		while (p != q)
			(p++)->dispose();
	}

	size_t ApplyPredifferRegExps(LPCTSTR filename, char *dst, const char *src, size_t len);
	BSTR ApplyPredifferScripts(LPCTSTR filename, BSTR bstr);
	void ResetPrediffers(short side);

	stl::vector<filter_item> m_list;
	stl::vector<filter_item> m_predifferRegExps;
	stl::vector<script_item> m_predifferScripts;
	bool m_utf8;
};


#endif // _FILTERLIST_H_
