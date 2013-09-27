/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file FileFilterMgr.h
 *
 *  @brief Declaration file for FileFilterMgr
 */ 
// ID line follows -- this is updated by SVN
// $Id$

#ifndef FileFilterMgr_h_included
#define FileFilterMgr_h_included

#include "FileFilter.h"

/**
 * @brief Return values for many filter functions.
 */
enum FILTER_RETVALUE
{
	FILTER_OK = 0,  /**< Success */
	FILTER_ERROR_FILEACCESS,  /**< File could not be opened etc. */
	FILTER_NOTFOUND, /**< Filter not found */
};

/**
 * @brief File filter manager for handling filefilters.
 *
 * The FileFilterMgr loads a collection of named file filters from disk,
 * and provides lookup access by name, or array access by index, to these
 * named filters. It also provides test functions for actually using the
 * filters.
 *
 * We are using PCRE for regular expressions. Nice thing in PCRE is it supports
 * UTF-8 unicode, unlike many other libs. For ANSI builds we use just ansi
 * strings, and for unicode we must first convert strings to UTF-8.
 */
class FileFilterMgr
{
public:
	~FileFilterMgr();
	// Reload filter array from specified directory (passed to CFileFind)
	void LoadFromDirectory(LPCTSTR dir, LPCTSTR szPattern);
	// Load a filter from a string
	int AddFilter(LPCTSTR szFilterFile);
	void RemoveFilter(LPCTSTR szFilterFile);

	// access to array of filters
	FileFilter *GetFilterByPath(LPCTSTR) const;

	// Clear the list of known filters
	void DeleteAllFilters();

	// Load a filter from a file (if syntax is valid)
	static FileFilter *LoadFilterFile(LPCTSTR szFilepath);

// Implementation data
public:
	stl::vector<FileFilter *> m_filters; /*< List of filters loaded */
};

#endif // FileFilterMgr_h_included
