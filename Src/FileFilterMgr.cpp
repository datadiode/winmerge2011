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
 *  @file FileFilterMgr.cpp
 *
 *  @brief Implementation of FileFilterMgr and supporting routines
 */ 
#include "StdAfx.h"
#include "pcre.h"
#include "FileFilterMgr.h"
#include "UniFile.h"
#include "coretools.h"
#include "paths.h"

using std::vector;

/**
 * @brief Destructor, frees all filters.
 */
FileFilterMgr::~FileFilterMgr()
{
	DeleteAllFilters();
}

/**
 * @brief Load all filter files matching pattern from disk into internal filter set.
 * @param [in] dir Directory from where filters are loaded.
 * @param [in] szPattern Pattern for filters to load filters, for example "*.flt".
 */
void FileFilterMgr::LoadFromDirectory(LPCTSTR dir, LPCTSTR szPattern)
{
	const String pattern = paths_ConcatPath(dir, szPattern);
	WIN32_FIND_DATA ff;
	HANDLE h = FindFirstFile(pattern.c_str(), &ff);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			// Work around *.xyz matching *.xyz~
			if (!PathMatchSpec(ff.cFileName, szPattern))
				continue;
			String filterpath = paths_ConcatPath(dir, ff.cFileName);
			if (FileFilter *pFilter = LoadFilterFile(filterpath.c_str()))
				m_filters.push_back(pFilter);
		} while (FindNextFile(h, &ff));
		FindClose(h);
	}
}

/**
 * @brief Removes all filters from current list.
 */
void FileFilterMgr::DeleteAllFilters()
{
	while (!m_filters.empty())
	{
		FileFilter* filter = m_filters.back();
		delete filter;
		m_filters.pop_back();
	}
}

/**
 * @brief Parse a filter file, and add it to array if valid.
 *
 * @param [in] szFilePath Path (w/ filename) to file to load.
 * @param [out] error Error-code if loading failed (returned NULL).
 * @return Pointer to new filter, or NULL if error (check error code too).
 */
FileFilter *FileFilterMgr::LoadFilterFile(LPCTSTR szFilepath)
{
	FileFilter *pfilter = new FileFilter;
	pfilter->fullpath = szFilepath;
	pfilter->name = PathFindFileName(szFilepath); // Filename is the default name
	if (!pfilter->Load())
	{
		delete pfilter;
		pfilter = NULL;
	}
	return pfilter;
}

/**
 * @brief Give client back a pointer to the actual filter.
 *
 * @param [in] szFilterPath Full path to filterfile.
 * @return Pointer to found filefilter or NULL;
 * @note We just do a linear search, because this is seldom called
 */
FileFilter *FileFilterMgr::GetFilterByPath(LPCTSTR szFilterPath) const
{
	vector<FileFilter*>::const_iterator iter = m_filters.begin();
	while (iter != m_filters.end())
	{
		if (_tcsicmp((*iter)->fullpath.c_str(), szFilterPath) == 0)
			return *iter;
		++iter;
	}
	return NULL;
}
