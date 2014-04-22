/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  FileFilterHelper.cpp
 *
 * @brief Implementation file for FileFilterHelper class
 */
#include "StdAfx.h"
#include "FilterList.h"
#include "FileFilterHelper.h"
#include "coretools.h"
#include "paths.h"

using stl::vector;

IDiffFilter transparentFileFilter;
FileFilterHelper globalFileFilter;

/**
 * @brief Constructor, creates new filtermanager.
 */
FileFilterHelper::FileFilterHelper()
: m_sGlobalFilterPath(GetModulePath() + _T("\\Filters"))
#pragma warning(disable:warning_this_used_in_base_member_initializer_list)
, m_currentFilter(this)
#pragma warning(default:warning_this_used_in_base_member_initializer_list)
{
}

/**
 * @brief Destructor, deletes filtermanager.
 */
FileFilterHelper::~FileFilterHelper()
{
}

/**
 * @brief Store current filter path.
 *
 * Select filter based on filepath. If filter with that path
 * is found select it. Otherwise set path to empty (default).
 * @param [in] szFileFilterPath Full path to filter to select.
 */
void FileFilterHelper::SetFileFilterPath(LPCTSTR szFileFilterPath)
{
	// Don't bother to lookup empty path
	if (*szFileFilterPath)
	{
		m_currentFilter = GetFilterByPath(szFileFilterPath);
	}
	else
	{
		m_currentFilter = this;
	}
}

/**
 * @brief Return path to filter with given name.
 * @param [in] filterName Name of filter.
 */
FileFilter *FileFilterHelper::FindFilter(LPCTSTR filterName)
{
	FileFilter *filter = this;
	filterName = EatPrefix(filterName, _T("[F] "));
	if (filterName != NULL)
	{
		vector<FileFilter *>::const_iterator iter = m_filters.begin();
		while (iter != m_filters.end())
		{
			if ((*iter)->name == filterName)
			{
				filter = *iter;
				break;
			}
			++iter;
		}
	}
	return filter;
}

/**
 * @brief Set User's filter folder.
 * @param [in] filterPath Location of User's filters.
 */
bool FileFilterHelper::SetUserFilterPath(const String &filterPath)
{
	m_sUserSelFilterPath = paths_ConcatPath(filterPath, _T("Filters\\"));
	return paths_CreateIfNeeded(m_sUserSelFilterPath.c_str());
}

/**
 * @brief Create rules from mask.
 */
bool FileFilterHelper::CreateFromMask()
{
	LPCTSTR mask = name.c_str();
	// Convert user-given extension list to valid regular expression
	static const TCHAR separators[] = _T(" ;|,");
	while (*(mask += StrSpn(mask, separators)))
	{
		String regExp;
		stl::vector<regexp_item> *filters = NULL;
		if (LPCTSTR pch = EatPrefixTrim(mask, _T("f:")))
		{
			mask = pch;
			filters = &filefilters;
			regExp = _T("/^");
		}
		else if (LPCTSTR pch = EatPrefixTrim(mask, _T("d:")))
		{
			mask = pch;
			filters = &dirfilters;
			regExp = _T("/\\\\");
		}
		else if (LPCTSTR pch = EatPrefixTrim(mask, _T("xf:")))
		{
			mask = pch;
			filters = &xfilefilters;
			regExp = _T("/^");
		}
		else if (LPCTSTR pch = EatPrefixTrim(mask, _T("xd:")))
		{
			mask = pch;
			filters = &xdirfilters;
			regExp = _T("/\\\\");
		}
		else
		{
			filters = &filefilters;
			regExp = _T("/^");
		}
		if (int lentoken = StrCSpn(mask, separators))
		{
			do switch (TCHAR c = *mask++)
			{
			case '*':
				regExp += _T(".*");
				break;
			case '?':
				regExp += _T('.');
				break;
			case '.':
				if (lentoken == 1 || lentoken == 2 && *mask == '*')
				{
					if (regExp.back() == '*')
					{
						regExp.replace(regExp.length() - 2, 1, _T("[^.]"));
					}
					regExp += _T("\\.?");
				}
				else
				{
					regExp += _T("\\.");
				}
				break;
			case '+': case '^': case '$':
			case '(': case '[': case '{':
			case ')': case ']': case '}':
			case '\\': 
				// above cases need escaping
				regExp += _T('\\');
				// fall through
			default:
				regExp += c;
				break;
			} while (--lentoken);
			regExp += _T("$/iu"); // PCRE_CASELESS | PCRE_UTF8
			regexp_item item;
			if (item.assign(regExp.c_str(), regExp.length()))
			{
				filters->push_back(item);
			}
		}
	}
	return true;
}

BSTR FileFilterHelper::getSql(int side)
{
	return m_currentFilter->getSql(side);
}

/**
 * @brief Check if any of filefilter rules match to filename.
 *
 * @param [in] szFileName Filename to test.
 * @return TRUE unless we're suppressing this file by filter
 */
bool FileFilterHelper::includeFile(LPCTSTR szFileName)
{
	return m_currentFilter->TestFileNameAgainstFilter(szFileName);
}

/**
 * @brief Check if any of filefilter rules match to directoryname.
 *
 * @param [in] szFileName Directoryname to test.
 * @return TRUE unless we're suppressing this directory by filter
 */
bool FileFilterHelper::includeDir(LPCTSTR szDirName)
{
	// Prepend a backslash
	String strDirName(_T("\\"));
	strDirName += szDirName;
	return m_currentFilter->TestDirNameAgainstFilter(strDirName.c_str());
}

int FileFilterHelper::collateFile(LPCTSTR p, LPCTSTR q)
{
	if (m_currentFilter && !m_currentFilter->fileprefilters.empty() &&
		regexp_item::indifferent(m_currentFilter->fileprefilters, p, q))
	{
		return 0;
	}
	return IDiffFilter::collateFile(p, q);
}

int FileFilterHelper::collateDir(LPCTSTR p, LPCTSTR q)
{
	if (m_currentFilter && !m_currentFilter->dirprefilters.empty() &&
		regexp_item::indifferent(m_currentFilter->dirprefilters, p, q))
	{
		return 0;
	}
	return IDiffFilter::collateDir(p, q);
}

/**
 * @brief Returns active filter (or mask string)
 * @return The active filter.
 */
String FileFilterHelper::GetFilterNameOrMask() const
{
	const FileFilter *const self = this;
	return m_currentFilter != self ? _T("[F] ") + m_currentFilter->name : name;
}

/**
 * @brief Set filter.
 *
 * Simple-to-use function to select filter. This function determines
 * filter type so caller doesn't need to care about it.
 *
 * @param [in] filter File mask or filter name.
 * @return TRUE if given filter was set, FALSE if default filter was set.
 * @note If function returns FALSE, you should ask filter set with
 * GetFilterNameOrMask().
 */
FileFilter *FileFilterHelper::SetFilter(const String &filter)
{
	// Remove leading and trailing whitespace characters from the string.
	string_trim_ws(name = filter);
	m_currentFilter = FindFilter(name.c_str());
	return m_currentFilter;
}

/**
 * @brief Reloads all filter files
 */
FileFilter *FileFilterHelper::ReloadAllFilters()
{
	vector<FileFilter *>::const_iterator iter = m_filters.begin();
	while (iter != m_filters.end())
	{
		(*iter++)->Load();
	}
	return m_currentFilter != this ? m_currentFilter : NULL;
}

/**
 * @brief Reloads the current filter file
 */
void FileFilterHelper::ReloadCurrentFilter()
{
	m_currentFilter->Load();
}

/**
 * @brief Load any known file filters
 * @todo Preserve filter selection? How?
 */
void FileFilterHelper::LoadAllFileFilters()
{
	// First delete existing filters
	DeleteAllFilters();
	// Path for shared filters
	LoadFromDirectory(m_sGlobalFilterPath.c_str(), FileFilterExt);
	// Path for user's private filters
	LoadFromDirectory(m_sUserSelFilterPath.c_str(), FileFilterExt);
	// Load the filter template, which may specify a default SQL filter
	static const TCHAR tmpl[] = _T("FileFilter.tmpl");
	fullpath = paths_ConcatPath(m_sUserSelFilterPath, tmpl);
	if (!Load())
	{
		fullpath = paths_ConcatPath(m_sGlobalFilterPath, tmpl);
		Load();
	}
	m_currentFilter = FindFilter(name.c_str());
}
