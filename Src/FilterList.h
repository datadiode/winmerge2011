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

/**
 * @brief Container for one filtering rule / compiled expression.
 * This structure holds compiled regular expression and a original expression
 * as a string. We need the original expression string in case we want to
 * know which regular expression did match.
 */
struct filter_item
{
	HString *filterAsString; /** Original regular expression string */
 	pcre *pRegExp; /**< Compiled regular expression */
	pcre_extra *pRegExpExtra; /**< Additional information got from regex study */

	filter_item() : filterAsString(NULL), pRegExp(NULL), pRegExpExtra(NULL) {}
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
	
	void AddRegExp(LPCTSTR regularExpression);
	void RemoveAllFilters();
	bool HasRegExps();
	bool Match(size_t stringlen, const char *string, int codepage = CP_UTF8);
	const char * GetLastMatchExpression();

private:
	stl::vector <filter_item> m_list;
	bool m_utf8;
};


#endif // _FILTERLIST_H_
