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
#include "Common/coretools.h"
#include "UniFile.h"

using stl::vector;

/**
 * @brief Add a single pattern (if nonempty & valid) to a pattern list.
 *
 * @param [in] filterList List where pattern is added.
 * @param [in] str Temporary variable (ie, it may be altered)
 */
static void AddFilterPattern(vector<regexp_item> &filterList, LPCTSTR psz)
{
	const char *errormsg = NULL;
	int erroroffset = 0;
	const OString regexString = HString::Uni(psz)->Oct(CP_UTF8);
	if (pcre *regexp = pcre_compile(regexString.A,
			PCRE_CASELESS, &errormsg, &erroroffset, NULL))
	{
		regexp_item elem;
		errormsg = NULL;
		pcre_extra *pe = pcre_study(regexp, 0, &errormsg);
		elem.pRegExp = regexp;
		elem.pRegExpExtra = pe;
		elem.backslash = strstr(regexString.A, "\\\\") != NULL;
		filterList.push_back(elem);
	}
}

/**
 * @brief Constructor
 */
FileFilter::FileFilter()
	: params(2)
	, rawsql(2)
{
}

/**
 * @brief Destructor, frees created filter lists.
 */
FileFilter::~FileFilter()
{
	Clear();
}

/**
 * @brief Free created filter lists.
 */
void FileFilter::Clear()
{
	EmptyFilterList(filefilters);
	EmptyFilterList(dirfilters);
	EmptyFilterList(xfilefilters);
	EmptyFilterList(xdirfilters);
	EmptyFilterList(fileprefilters);
	EmptyFilterList(dirprefilters);
}

/**
 * @brief Get SQL clause.
 */
BSTR FileFilter::getSql(int side)
{
	BSTR dst = NULL;
	switch (sqlopt[side])
	{
	case BST_CHECKED:
		dst = composeSql(side);
		break;
	case BST_INDETERMINATE:
		dst = SysAllocString(rawsql[side].c_str());
		break;
	}
	return dst;
}

/**
 * @brief Compose SQL clause from template and parameters.
 */
BSTR FileFilter::composeSql(int side)
{
	BSTR dst = SysAllocString(sql.c_str());
	LPCTSTR src = dst;
	C_ASSERT(('"' & 0x3F) == '"');
	C_ASSERT(('\'' & 0x3F) == '\'');
	TCHAR quote = '\0';
	while (const TCHAR c = *src)
	{
		if (quote <= _T('\0') && c == _T('%'))
		{
			LPCTSTR p = src + 1;
			if (LPCTSTR q = _tcschr(p, c))
			{
				String name(p, static_cast<String::size_type>(q - p));
				String value = params[side][name];
				const UINT i = static_cast<UINT>(p - dst);
				const UINT j = static_cast<UINT>(q - dst);
				const UINT n = SysStringLen(dst);
				TCHAR quote = _T('\'');
				static const TCHAR TO_TIMESTAMP[] = _T("TO_TIMESTAMP");
				if (value.empty())
				{
					value = _T("NULL");
					quote = _T(' ');
				}
				else if (i > _countof(TO_TIMESTAMP))
				{
					if (LPCTSTR q = EatPrefix(src - _countof(TO_TIMESTAMP), TO_TIMESTAMP))
					{
						if (*q == _T('('))
						{
							FileTime ft;
							if (ft.Parse(value.c_str(), 0))
							{
								NumToStr str(ft.ticksSinceBC0001(), 10);
								// LogParser wants seconds, so cut off fractional digits
								String::size_type len = str.length();
								if (len > 7)
								{
									value.assign(str.c_str(), len - 7);
									quote = _T(' ');
								}
							}
						}
					}
				}
				UINT k = value.length();
				if (BSTR tmp = SysAllocStringLen(NULL, n - (j - i) + k))
				{
					memcpy(tmp, dst, i * sizeof *tmp);
					memcpy(tmp + i, value.c_str(), k * sizeof *tmp);
					k += i;
					memcpy(tmp + k, q, (n - j) * sizeof *tmp);
					tmp[i - 1] = tmp[k] = quote;
					SysFreeString(dst);
					dst = tmp;
					p = dst + i;
					q = dst + k;
				}
				src = q;
			}
		}
		if (c == '\\')
			quote ^= 0x40;
		else if ((c == '"' || c == '\'') && (quote == '\0' || quote == c))
			quote ^= c;
		else
			quote &= 0x3F;
		++src;
	}
	return dst;
}

/**
 * @brief Test given string against given regexp list.
 *
 * @param [in] filterList List of regexps to test against.
 * @param [in] szTest String to test against regexps.
 * @return >= 0 if string passes
 * @note Matching stops when first match is found.
 */
int FileFilter::TestAgainstRegList(const vector<regexp_item> &filterList, LPCTSTR szPath, LPCTSTR szName)
{
	ASSERT(!filterList.empty());
	const char *pathString = NULL;
	UINT pathLength = 0; 
	const char *nameString = NULL;
	UINT nameLength = 0;
	const UINT lenName = static_cast<UINT>(_tcslen(szName));
	OString compString = HString::Uni(NULL, lenName + 1);
	if (LPWSTR pwc = compString.W)
	{
		*pwc++ = L'\\';
		wmemcpy(pwc, szName, lenName);
		compString.m_pStr = compString.m_pStr->Oct(CP_UTF8);
		if (compString.m_pStr)
		{
			nameString = compString.A + 1;
			nameLength = compString.ByteLen() - 1;
		}
	}
	int result = -1;
	vector<regexp_item>::const_iterator iter = filterList.begin();
	do
	{
		int ovector[30];
		if (iter->backslash)
		{
			if (pathString == NULL)
			{
				if (const UINT lenPath = static_cast<UINT>(_tcslen(szPath)))
				{
					compString.Free();
					compString.m_pStr = HString::Uni(NULL, lenPath + lenName + 2);
					if (LPWSTR pwc = compString.W)
					{
						*pwc++ = L'\\';
						wmemcpy(pwc, szPath, lenPath);
						pwc += lenPath;
						*pwc++ = L'\\';
						wmemcpy(pwc, szName, lenName);
						compString.m_pStr = compString.m_pStr->Oct(CP_UTF8);
					}
					else
					{
						nameLength = 0;
					}
				}
				pathString = compString.A;
				pathLength = compString.ByteLen();
				nameString = pathString + pathLength - nameLength;
			}
			result = iter->execute(pathString, pathLength, 0, 0, ovector, _countof(ovector));
		}
		else
		{
			result = iter->execute(nameString, nameLength, 0, 0, ovector, _countof(ovector));
		}
	} while ((result < 0) && (++iter != filterList.end()));
	return result;
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
bool FileFilter::TestFileNameAgainstFilter(LPCTSTR szPath, LPCTSTR szFileName) const
{
	return (filefilters.empty() || TestAgainstRegList(filefilters, szPath, szFileName) >= 0)
		&& (xfilefilters.empty() || TestAgainstRegList(xfilefilters, szPath, szFileName) < 0);
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
bool FileFilter::TestDirNameAgainstFilter(LPCTSTR szPath, LPCTSTR szDirName) const
{
	return (dirfilters.empty() || TestAgainstRegList(dirfilters, szPath, szDirName) >= 0)
		&& (xdirfilters.empty() || TestAgainstRegList(xdirfilters, szPath, szDirName) < 0);
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

/**
 * @brief Parse a filter file, and return true if valid.
 */
bool FileFilter::Load()
{
	UniMemFile file;
	if (!file.OpenReadOnly(fullpath.c_str()))
		return false;

	file.ReadBom(); // in case it is a Unicode file, let UniMemFile handle BOM

	Clear();
	sql.clear();

	const bool bIsMask = CreateFromMask();

	String sLine, sEol;
	bool lossy = false;
	bool bLinesLeft = true;
	bool default_include = false;
	do
	{
		// Returns false when last line is read
		bLinesLeft = file.ReadString(sLine, sEol, &lossy);
		static const TCHAR commentLeader[] = _T("##"); // Starts comment
		// Ignore lines beginning with '##'
		String::size_type pos = sLine.find(commentLeader);
		if (pos != 0)
		{
			// Find possible comment-separator '<whitespace>##'
			while (pos != String::npos && !_istspace(sLine[pos - 1]))
				pos = sLine.find(commentLeader, pos + 1);
		}
		// Remove comment and whitespaces before it
		if (pos != String::npos)
			sLine.resize(pos);
		string_trim_ws(sLine);

		if (sLine.empty())
			continue;

		if (sLine.find(':') >= sLine.find(' '))
		{
			// If there is no colon at all,
			// or a space earlier on the line,
			// append the line to the SQL clause
			sql += sLine;
			sql += sEol;
			continue;
		}

		// Assign remaining properties and rules only if not a .tmpl file
		if (bIsMask)
			continue;

		if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("name:")))
		{
			// specifies display name
			name = psz;
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("desc:")))
		{
			// specifies description
			description = psz;
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("def:")))
		{
			// specifies default
			String str = psz;
			if (PathMatchSpec(psz, _T("0;no;exclude")))
				default_include = false;
			else if (PathMatchSpec(psz, _T("1;yes;include")))
				default_include = true;
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("sqlopt-left:")))
		{
			// activates SQL clause on left side
			sqlopt[0] = static_cast<BYTE>(_ttol(psz));
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("sqlopt-right:")))
		{
			// activates SQL clause on right side
			sqlopt[1] = static_cast<BYTE>(_ttol(psz));
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("f:")))
		{
			// file filter
			AddFilterPattern(default_include ? xfilefilters : filefilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("d:")))
		{
			// directory filter
			AddFilterPattern(default_include ? xdirfilters : dirfilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("xf:")))
		{
			// file filter
			AddFilterPattern(xfilefilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("xd:")))
		{
			// directory filter
			AddFilterPattern(xdirfilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("equiv-f:")))
		{
			// file prefilter
			regexp_item item;
			if (item.assign(psz, static_cast<int>(_tcslen(psz))))
			{
				fileprefilters.push_back(item);
			}
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("equiv-d:")))
		{
			// directory prefilter
			regexp_item item;
			if (item.assign(psz, static_cast<int>(_tcslen(psz))))
			{
				dirprefilters.push_back(item);
			}
		}
	} while (bLinesLeft);

	return true;
}
