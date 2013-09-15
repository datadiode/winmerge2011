/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify it
//    under the terms of the GNU General Public License as published by the
//    Free Software Foundation; either version 2 of the License, or (at your
//    option) any later version.
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file FileFilter.cpp
 *
 *  @brief Implementation of FileFilter.
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "FileFilter.h"

using stl::vector;

/**
 * @brief Destructor, frees created filter lists.
 */
FileFilter::~FileFilter()
{
	EmptyFilterList(filefilters);
	EmptyFilterList(dirfilters);
	EmptyFilterList(fileprefilters);
	EmptyFilterList(dirprefilters);
}

/**
 * @brief Test given string against given regexp list.
 *
 * @param [in] filterList List of regexps to test against.
 * @param [in] szTest String to test against regexps.
 * @return TRUE if string passes
 * @note Matching stops when first match is found.
 */
bool FileFilter::TestAgainstRegList(const vector<regexp_item> &filterList, LPCTSTR szTest)
{
	const OString compString = HString::Uni(szTest)->Oct(CP_UTF8);
	int result = -1;
	vector<regexp_item>::const_iterator iter = filterList.begin();
	while (iter != filterList.end())
	{
		pcre *regexp = iter->pRegExp;
		pcre_extra *extra = iter->pRegExpExtra;
		int ovector[30];
		result = pcre_exec(regexp, extra, compString.A, compString.ByteLen(),
			0, 0, ovector, 30);
		if (result >= 0)
			break;
		++iter;
	}
	return result >= 0;
}

/**
 * @brief Test given filename against filefilter.
 *
 * Test filename against active filefilter. If matching rule is found
 * we must first determine type of rule that matched. If we return FALSE
 * from this function directory scan marks file as skipped.
 *
 * @param [in] pFilter Pointer to filefilter
 * @param [in] szFileName Filename to test
 * @return TRUE if file passes the filter
 */
bool FileFilter::TestFileNameAgainstFilter(LPCTSTR szFileName) const
{
	return (filefilters.empty() || TestAgainstRegList(filefilters, szFileName))
		&& (xfilefilters.empty() || !TestAgainstRegList(xfilefilters, szFileName));
}

/**
 * @brief Test given directory name against filefilter.
 *
 * Test directory name against active filefilter. If matching rule is found
 * we must first determine type of rule that matched. If we return FALSE
 * from this function directory scan marks file as skipped.
 *
 * @param [in] pFilter Pointer to filefilter
 * @param [in] szDirName Directory name to test
 * @return TRUE if directory name passes the filter
 */
bool FileFilter::TestDirNameAgainstFilter(LPCTSTR szDirName) const
{
	return (dirfilters.empty() || TestAgainstRegList(dirfilters, szDirName))
		&& (xdirfilters.empty() || !TestAgainstRegList(xdirfilters, szDirName));
}

stl_size_t FileFilter::ApplyPrefilterRegExps(const vector<regexp_item> &filterList, char *dst, const char *src, stl_size_t len)
{
	stl::vector<regexp_item>::const_iterator iter = filterList.begin();
	while (iter != filterList.end())
	{
		const regexp_item &filter = *iter++;
		char *buf = dst;
		stl_size_t i = 0;
		while (i < len)
		{
			int ovector[33];
			int matches = filter.execute(src, len, i, 0, ovector, _countof(ovector) - 1);
			if ((matches <= 0) || (ovector[1] == 0))
			{
				ovector[1] = len;
				matches = 0;
			}
			int matches2 = matches * 2;
			ovector[matches2] = ovector[1];
			ovector[1] = ovector[0];
			HString *const injectString = filter.injectString;
			stl_size_t const injectLength = injectString->ByteLen();
			int index = 1;
			stl_size_t j;
			do
			{
				j = ovector[index];
				if (i < j)
				{
					stl_size_t d = j - i;
					if (index == 1 || (index & 1) == 0)
					{
						memcpy(buf, src + i, d);
						buf += d;
					}
					else if (injectLength <= d)
					{
						memcpy(buf, injectString, injectLength);
						buf += injectLength;
					}
					i = j;
				}
			} while (++index <= matches2);
			if (!filter.global)
			{
				j = len;
				if (i < j)
				{
					stl_size_t d = j - i;
					memcpy(buf, src + i, d);
					buf += d;
				}
			}
			i = j;
		}
		len = static_cast<stl_size_t>(buf - dst);
		src = dst;
	}
	return len;
}

/**
 * @brief Deletes items from filter list.
 *
 * @param [in] filterList List to empty.
 */
void FileFilter::EmptyFilterList(vector<regexp_item> &filterList)
{
	while (!filterList.empty())
	{
		regexp_item &elem = filterList.back();
		elem.dispose();
		filterList.pop_back();
	}
}
