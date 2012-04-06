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
#include "FileFilterMgr.h"
#include "FileFilterHelper.h"
#include "Coretools.h"
#include "paths.h"

using stl::vector;

/** 
 * @brief Constructor, creates new filtermanager.
 */
FileFilterHelper::FileFilterHelper()
: m_pMaskFilter(new FilterList)
, m_fileFilterMgr(new FileFilterMgr)
{
}

/** 
 * @brief Destructor, deletes filtermanager.
 */
FileFilterHelper::~FileFilterHelper()
{
	delete m_fileFilterMgr;
	delete m_pMaskFilter;
}

/** 
 * @brief Return filtermanager used.
 */
FileFilterMgr * FileFilterHelper::GetManager() const
{
	return m_fileFilterMgr;
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
		m_currentFilter = m_fileFilterMgr->GetFilterByPath(szFileFilterPath);
	}
	else
	{
		m_currentFilter = NULL;
	}
}

/**
 * @brief Get list of filters currently available.
 *
 * @param [out] filters Filter list to receive found filters.
 * @param [out] selected Filepath of currently selected filter.
 */
const stl::vector<FileFilter *> &FileFilterHelper::GetFileFilters(String & selected) const
{
	if (m_currentFilter)
	{
		selected = m_currentFilter->fullpath;
	}
	return m_fileFilterMgr->m_filters;
}

/**
 * @brief Return name of filter in given file.
 * If no filter cannot be found, return empty string.
 * @param [in] filterPath Path to filterfile.
 * @sa FileFilterHelper::GetFileFilterPath()
 */
String FileFilterHelper::GetFileFilterName(LPCTSTR filterPath) const
{
	String name;
	vector<FileFilter *>::const_iterator iter = m_fileFilterMgr->m_filters.begin();
	while (iter != m_fileFilterMgr->m_filters.end())
	{
		if ((*iter)->fullpath == filterPath)
		{
			name = (*iter)->name;
			break;
		}
		iter++;
	}
	return name;
}

/** 
 * @brief Return path to filter with given name.
 * @param [in] filterName Name of filter.
 * @sa FileFilterHelper::GetFileFilterName()
 */
String FileFilterHelper::GetFileFilterPath(LPCTSTR filterName) const
{
	String path;
	vector<FileFilter *>::const_iterator iter = m_fileFilterMgr->m_filters.begin();
	while (iter != m_fileFilterMgr->m_filters.end())
	{
		if ((*iter)->name == filterName)
		{
			path = (*iter)->fullpath;
			break;
		}
		iter++;
	}
	return path;
}

/** 
 * @brief Set User's filter folder.
 * @param [in] filterPath Location of User's filters.
 */
void FileFilterHelper::SetUserFilterPath(LPCTSTR filterPath)
{
	m_sUserSelFilterPath = paths_ConcatPath(filterPath, _T("Filters\\"));
}

/** 
 * @brief Set filemask for filtering.
 * @param [in] strMask Mask to set (e.g. *.cpp;*.h).
 */
void FileFilterHelper::SetMask(LPCTSTR strMask)
{
	if (m_currentFilter)
	{
		_RPTF0(_CRT_ERROR, "Filter mask tried to set when masks disabled!");
		return;
	}
	m_sMask = strMask;
	String regExp = ParseExtensions(strMask);
	m_pMaskFilter->RemoveAllFilters();
	m_pMaskFilter->AddRegExp(regExp.c_str());
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
		if (strFileName[0] != '\\')
			strFileName = _T("\\") + strFileName;
		// append a point if there is no extension
		if (strFileName.find('.') == -1)
			strFileName = strFileName + _T(".");

		const OString name_utf = HString::Uni(strFileName.c_str())->Oct(CP_UTF8);
		return m_pMaskFilter->Match(name_utf.ByteLen(), name_utf.A);
	}
	else
	{
		return m_fileFilterMgr->TestFileNameAgainstFilter(m_currentFilter, szFileName);
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

		return m_fileFilterMgr->TestDirNameAgainstFilter(m_currentFilter, strDirName.c_str());
	}
}

/**
 * @brief Load in all filters in a folder.
 * @param [in] dir Folder from where to load filters.
 * @param [in] sPattern Wildcard defining files to add to map as filter files.
 *   It is filemask, for example, "*.flt"
 */
void FileFilterHelper::LoadFileFilterDirPattern(LPCTSTR dir, LPCTSTR szPattern)
{
	m_fileFilterMgr->LoadFromDirectory(dir, szPattern);
}

/** 
 * @brief Convert user-given extension list to valid regular expression.
 * @param [in] Extension list/mask to convert to regular expression.
 * @return Regular expression that matches extension list.
 */
String FileFilterHelper::ParseExtensions(const String &extensions) const
{
	String strPattern;
	String ext(extensions);
	static const TCHAR pszSeps[] = _T(" ;|,:");

	ext += _T(";"); // Add one separator char to end

	size_t pos;
	while ((pos = ext.find_first_of(pszSeps)) != String::npos)
	{
		String token = ext.substr(0, pos); // Get first extension
		ext = ext.substr(pos + 1); // Remove extension + separator
		
		if (size_t lentoken = token.length())
		{
			strPattern += &_T("|\\\\")[strPattern.empty()]; // Omit the '|' if strPattern is still empty
			for (size_t postoken = 0 ; postoken < lentoken ; ++postoken)
			{
				switch (TCHAR c = token[postoken])
				{
				case '*':
					strPattern += _T(".*");
					break;
				case '?':
					strPattern += _T('.');
					break;
				case '.': case '^': case '$':
				case '(': case '[': case '{':
				case ')': case ']': case '}':
				case '\\': 
					// above cases need escaping
					strPattern += _T('\\');
					// fall through
				default:
					strPattern += c;
					break;
				}
			}
			strPattern += _T('$');
		}
	}

	string_makelower(strPattern);
	return strPattern;
}

/** 
 * @brief Returns active filter (or mask string)
 * @return The active filter.
 */
String FileFilterHelper::GetFilterNameOrMask() const
{
	String sFilter;

	if (m_currentFilter)
		sFilter = _T("[F] ") + GetFileFilterName(m_currentFilter->fullpath.c_str());
	else
		sFilter = m_sMask;

	return sFilter;
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
void FileFilterHelper::SetFilter(const String &filter)
{
	// Remove leading and trailing whitespace characters from the string.
	String flt = string_trim_ws(filter);
	if (LPCTSTR filterName = EatPrefix(flt.c_str(), _T("[F] ")))
	{
		flt = GetFileFilterPath(filterName);
		if (!flt.empty())
		{
			SetFileFilterPath(flt.c_str());
			return;
		}
	}
	SetFileFilterPath(_T(""));
	SetMask(flt.c_str());
}

/** 
 * @brief Reloads changed filter files
 *
 * Checks if filter file has been modified since it was last time
 * loaded/reloaded. If file has been modified we reload it.
 * @todo How to handle an error in reloading filter?
 */
void FileFilterHelper::ReloadUpdatedFilters()
{
	DirItem fileInfo;

	vector<FileFilter *>::const_iterator iter = m_fileFilterMgr->m_filters.begin();
	while (iter != m_fileFilterMgr->m_filters.end())
	{
		FileFilter *filter = *iter++;
		String path = filter->fullpath;
		fileInfo.Update(path.c_str());
		if (fileInfo.mtime != filter->fileinfo.mtime ||
			fileInfo.size.int64 != filter->fileinfo.size.int64)
		{
			// Reload filter after changing it
			if (FileFilter *relocatedFilter = m_fileFilterMgr->ReloadFilterFromDisk(filter))
			{
				// If it was active filter we have to re-set it
				if (m_currentFilter == filter)
					m_currentFilter = relocatedFilter;
			}
		}
	}
}

/**
 * @brief Load any known file filters
 * @todo Preserve filter selection? How?
 */
void FileFilterHelper::LoadAllFileFilters()
{
	// First delete existing filters
	m_fileFilterMgr->DeleteAllFilters();

	// Program application directory
	m_sGlobalFilterPath = GetModulePath() + _T("\\Filters");
	String pattern(_T("*"));
	pattern += FileFilterExt;
	LoadFileFilterDirPattern(m_sGlobalFilterPath.c_str(), pattern.c_str());
	LoadFileFilterDirPattern(m_sUserSelFilterPath.c_str(), pattern.c_str());
}

/**
 * @brief Return path to global filters (& create if needed), or empty if cannot create
 */
String FileFilterHelper::GetGlobalFilterPathWithCreate() const
{
	bool const exist = paths_EnsurePathExist(m_sGlobalFilterPath.c_str());
	return exist ? m_sGlobalFilterPath : String();
}

/**
 * @brief Return path to user filters (& create if needed), or empty if cannot create
 */
String FileFilterHelper::GetUserFilterPathWithCreate() const
{
	bool const exist = paths_EnsurePathExist(m_sUserSelFilterPath.c_str());
	return exist ? m_sUserSelFilterPath : String();
}
