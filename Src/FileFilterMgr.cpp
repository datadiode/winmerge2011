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
#include "FileFilter.h"
#include "pcre.h"
#include "FileFilterMgr.h"
#include "UniFile.h"
#include "coretools.h"
#include "paths.h"

using stl::vector;

static void AddFilterPattern(vector<regexp_item> &filterList, LPCTSTR psz);

/**
 * @brief Destructor, frees all filters.
 */
FileFilterMgr::~FileFilterMgr()
{
	DeleteAllFilters();
}

/**
 * @brief Loads filterfile from disk and adds it to filters.
 * @param [in] szFilterFile Filter file to load.
 * @return FILTER_OK if succeeded or one of FILTER_RETVALUE values on error.
 */
int FileFilterMgr::AddFilter(LPCTSTR szFilterFile)
{
	int errorcode = FILTER_OK;
	if (FileFilter *pFilter = LoadFilterFile(szFilterFile))
		m_filters.push_back(pFilter);
	return errorcode;
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
			AddFilter(filterpath.c_str());
		} while (FindNextFile(h, &ff));
		FindClose(h);
	}
}

/**
 * @brief Removes filter from filterlist.
 *
 * @param [in] szFilterFile Filename of filter to remove.
 */
void FileFilterMgr::RemoveFilter(LPCTSTR szFilterFile)
{
	// Note that m_filters.GetSize can change during loop
	vector<FileFilter*>::iterator iter = m_filters.begin();
	while (iter != m_filters.end())
	{
		if (_tcsicmp((*iter)->fullpath.c_str(), szFilterFile) == 0)
		{
			delete (*iter);
			m_filters.erase(iter);
			break;
		}
		++iter;
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
		filterList.push_back(elem);
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
	UniMemFile file;
	if (!file.OpenReadOnly(szFilepath))
	{
		return NULL;
	}

	file.ReadBom(); // in case it is a Unicode file, let UniMemFile handle BOM

	FileFilter *pfilter = new FileFilter;
	pfilter->fullpath = szFilepath;
	pfilter->name = PathFindFileName(szFilepath); // Filename is the default name

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
			pfilter->sql += sLine.c_str();
			pfilter->sql += sEol.c_str();
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("name:")))
		{
			// specifies display name
			pfilter->name = psz;
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("desc:")))
		{
			// specifies description
			pfilter->description = psz;
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
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("f:")))
		{
			// file filter
			AddFilterPattern(default_include ? pfilter->xfilefilters : pfilter->filefilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("d:")))
		{
			// directory filter
			AddFilterPattern(default_include ? pfilter->xdirfilters : pfilter->dirfilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("xf:")))
		{
			// file filter
			AddFilterPattern(pfilter->xfilefilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("xd:")))
		{
			// directory filter
			AddFilterPattern(pfilter->xdirfilters, psz);
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("equiv-f:")))
		{
			// file prefilter
			regexp_item item;
			if (item.assign(psz, static_cast<int>(_tcslen(psz))))
			{
				pfilter->fileprefilters.push_back(item);
			}
		}
		else if (LPCTSTR psz = EatPrefixTrim(sLine.c_str(), _T("equiv-d:")))
		{
			// directory prefilter
			regexp_item item;
			if (item.assign(psz, static_cast<int>(_tcslen(psz))))
			{
				pfilter->dirprefilters.push_back(item);
			}
		}
	} while (bLinesLeft);

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
	return 0;
}

/**
 * @brief Return full path to filter.
 *
 * @param [in] pFilter Pointer to filter.
 * @return Full path of filter.
 */
String FileFilterMgr::GetFullpath(FileFilter * pfilter) const
{
	return pfilter->fullpath;
}

/**
 * @brief Reload filter from disk
 *
 * Reloads filter from disk. This is done by creating a new one
 * to substitute for old one.
 * @param [in] pFilter Pointer to filter to reload.
 * @return FILTER_OK when succeeds, one of FILTER_RETVALUE values on error.
 * @note Given filter (pfilter) is freed and must not be used anymore.
 */
FileFilter *FileFilterMgr::ReloadFilterFromDisk(FileFilter *pfilter)
{
	vector<FileFilter *>::iterator iter = stl::find(
		m_filters.begin(), m_filters.end(), pfilter);
	if (iter != m_filters.end())
	{
		if (FileFilter *const newfilter = LoadFilterFile(pfilter->fullpath.c_str()))
		{
			newfilter->sqlopt[0] = pfilter->sqlopt[0];
			newfilter->sqlopt[1] = pfilter->sqlopt[1];
			newfilter->params.swap(pfilter->params);
			delete pfilter;
			*iter = pfilter = newfilter;
		}
	}
	return pfilter;
}
