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
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "FilterList.h"
#include "FileFilter.h"
#include "FileFilterHelper.h"
#include "Coretools.h"
#include "paths.h"

using stl::vector;

IDiffFilter transparentFileFilter;
FileFilterHelper globalFileFilter;

/**
 * @brief Constructor, creates new filtermanager.
 */
FileFilterHelper::FileFilterHelper()
: m_pMaskFilter(new FilterList)
, m_sGlobalFilterPath(GetModulePath() + _T("\\Filters"))
, m_currentFilter(NULL)
{
}

/**
 * @brief Destructor, deletes filtermanager.
 */
FileFilterHelper::~FileFilterHelper()
{
	delete m_pMaskFilter;
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
		m_currentFilter = NULL;
	}
}

/**
 * @brief Return path to filter with given name.
 * @param [in] filterName Name of filter.
 */
FileFilter *FileFilterHelper::FindFilter(LPCTSTR filterName) const
{
	FileFilter *filter = NULL;
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
 * @brief Set filemask for filtering.
 * @param [in] strMask Mask to set (e.g. *.cpp;*.h).
 */
void FileFilterHelper::SetMask(LPCTSTR mask)
{
	m_currentFilter = NULL;
	m_sMask = mask;
	// Convert user-given extension list to valid regular expression
	String regExp;
	static const TCHAR separators[] = _T(" ;|,:");
	while (int lentoken = StrCSpn(mask += StrSpn(mask, separators), separators))
	{
		regExp += &_T("|\\\\")[regExp.empty()]; // Omit the '|' if strPattern is still empty
		do switch (TCHAR c = *mask++)
		{
		case '*':
			regExp += _T(".*");
			break;
		case '?':
			regExp += _T('.');
			break;
		case '.': case '^': case '$':
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
		regExp += _T('$');
	}
	string_makelower(regExp);
	m_pMaskFilter->RemoveAllFilters();
	m_pMaskFilter->AddRegExp(regExp.c_str());
}

BSTR FileFilterHelper::getSql(int side)
{
	return m_currentFilter ? m_currentFilter->getSql(side) : NULL;
}

/**
 * @brief Check if any of filefilter rules match to filename.
 *
 * @param [in] szFileName Filename to test.
 * @return TRUE unless we're suppressing this file by filter
 */
bool FileFilterHelper::includeFile(LPCTSTR szFileName)
{
	if (m_currentFilter == NULL)
	{
		// preprend a backslash if there is none
		String strFileName = szFileName;
		string_makelower(strFileName);
		if (strFileName[0] != _T('\\'))
			strFileName.insert(strFileName.begin(), _T('\\'));
		// append a point if there is no extension
		if (strFileName.find(_T('.')) == String::npos)
			strFileName.push_back(_T('.'));

		const OString name_utf = HString::Uni(strFileName.c_str())->Oct(CP_UTF8);
		return m_pMaskFilter->Match(name_utf.ByteLen(), name_utf.A);
	}
	else
	{
		return m_currentFilter->TestFileNameAgainstFilter(szFileName);
	}
}

/**
 * @brief Check if any of filefilter rules match to directoryname.
 *
 * @param [in] szFileName Directoryname to test.
 * @return TRUE unless we're suppressing this directory by filter
 */
bool FileFilterHelper::includeDir(LPCTSTR szDirName)
{
	if (m_currentFilter == NULL)
	{
		// directories have no extension
		return true; 
	}
	else
	{
		// Add a backslash
		String strDirName(_T("\\"));
		strDirName += szDirName;

		return m_currentFilter->TestDirNameAgainstFilter(strDirName.c_str());
	}
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
	return m_currentFilter ? _T("[F] ") + m_currentFilter->name : m_sMask;
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
	String flt = filter;
	string_trim_ws(flt);
	if (LPCTSTR filterName = EatPrefix(flt.c_str(), _T("[F] ")))
	{
		m_currentFilter = FindFilter(filterName);
	}
	else
	{
		m_currentFilter = NULL;
		SetMask(flt.c_str());
	}
	return m_currentFilter;
}

/**
 * @brief Reloads the specified filter file
 * @todo How to handle an error in reloading filter?
 */
FileFilter *FileFilterHelper::ReloadFilter(FileFilter *filter)
{
	// Reload filter after changing it
	FileFilter *reloaded = ReloadFilterFromDisk(filter);
	// If it was active filter we have to re-set it
	if (reloaded != NULL && m_currentFilter == filter)
	{
		m_currentFilter = reloaded;
	}
	return reloaded;
}

/**
 * @brief Reloads all filter files
 */
FileFilter *FileFilterHelper::ReloadAllFilters()
{
	vector<FileFilter *>::const_iterator iter = m_filters.begin();
	while (iter != m_filters.end())
	{
		ReloadFilter(*iter++);
	}
	return m_currentFilter;
}

/**
 * @brief Reloads the current filter file
 */
FileFilter *FileFilterHelper::ReloadCurrentFilter()
{
	if (m_currentFilter)
	{
		ReloadFilter(m_currentFilter);
	}
	return m_currentFilter;
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
}
