/** 
 * @file  FilterList.h
 *
 * @brief Implementation file for FilterList.
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "FilterList.h"
#include "pcre.h"
#include "unicoder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * @brief Constructor.
 */
 FilterList::FilterList(): m_utf8(false)
{
}

/** 
 * @brief Destructor.
 */
FilterList::~FilterList()
{
	RemoveAllFilters();
}

/** 
 * @brief Add new regular expression to the list.
 * This function adds new regular expression to the list of expressions.
 * The regular expression is compiled and studied for better performance.
 * @param [in] regularExpression Regular expression string.
 * @param [in] encoding Expression encoding.
 */
void FilterList::AddRegExp(LPCTSTR regularExpression)
{
	filter_item item;
	item.filterAsString = HString::Uni(regularExpression)->Oct(CP_UTF8);

	if (item.filterAsString->A[_tcslen(regularExpression)] != '\0')
		m_utf8 = true;

	const char * errormsg = NULL;
	int erroroffset = 0;

	pcre *regexp = pcre_compile(item.filterAsString->A, PCRE_UTF8, &errormsg,
		&erroroffset, NULL);
	if (regexp)
	{
		errormsg = NULL;
		item.pRegExp = regexp;
		item.pRegExpExtra = pcre_study(regexp, 0, &errormsg);
		m_list.push_back(item);
	}
}

/** 
 * @brief Removes all expressions from the list.
 */
void FilterList::RemoveAllFilters()
{
	m_utf8 = false;
	while (!m_list.empty())
	{
		filter_item & item = m_list.back();
		item.filterAsString->Free();
		pcre_free(item.pRegExp);
		pcre_free(item.pRegExpExtra);
		m_list.pop_back();
	}
}

/** 
 * @brief Returns if list has any expressions.
 * @return true if list contains one or more expressions.
 */
bool FilterList::HasRegExps()
{
	return !m_list.empty();
}

/** 
 * @brief Match string against list of expressions.
 * This function matches given @p string against the list of regular
 * expressions. The matching ends when first match is found, so all
 * expressions may not be matched against.
 * @param [in] string string to match.
 * @param [in] codepage codepage of string.
 * @return true if any of the expressions did match the string.
 */
bool FilterList::Match(size_t stringlen, const char *string, int codepage/*=CP_UTF8*/)
{
	const size_t count = m_list.size();
	// convert string into UTF-8
	ucr::buffer buf(0);
	if (m_utf8 && codepage != CP_UTF8)
	{
		buf.resize(stringlen * 2);
		ucr::convert(NONE, codepage, (const unsigned char *)string, 
				stringlen, UTF8, CP_UTF8, &buf);
		string = (const char *)buf.ptr;
		stringlen = buf.size;
	}
	unsigned int i = 0;
	while (i < count)
	{
		filter_item item = m_list[i];
		int ovector[30];
		pcre * regexp = item.pRegExp;
		pcre_extra * extra = item.pRegExpExtra;
		int result = pcre_exec(regexp, extra, string, stringlen, 0, 0, ovector, 30);
		if (result >= 0)
			return true;
		++i;
	}
	return false;
}
