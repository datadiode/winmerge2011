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

#include "pcre.h"

struct FileFilterElement;
struct FileFilter;

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
private:

public:
	~FileFilterMgr();
	// Reload filter array from specified directory (passed to CFileFind)
	void LoadFromDirectory(LPCTSTR dir, LPCTSTR szPattern);
	// Reload an edited filter
	FileFilter *ReloadFilterFromDisk(FileFilter *pfilter);
	// Load a filter from a string
	void LoadFilterString(LPCTSTR szFilterString);
	int AddFilter(LPCTSTR szFilterFile);
	void RemoveFilter(LPCTSTR szFilterFile);

	// access to array of filters
	//String GetFilterName(int i) const;
	//String GetFilterName(const FileFilter *pFilter) const;
	//String GetFilterPath(int i) const;
	//String GetFilterDesc(int i) const;
	//String GetFilterDesc(const FileFilter *pFilter) const;
	FileFilter * GetFilterByPath(LPCTSTR szFilterName);
	String GetFullpath(FileFilter *) const;

	// methods to actually use filter
	bool TestFileNameAgainstFilter(const FileFilter *, LPCTSTR szFileName) const;
	bool TestDirNameAgainstFilter(const FileFilter *, LPCTSTR szDirName) const;

	void DeleteAllFilters();

// Implementation methods
protected:
	// Clear the list of known filters
	// Load a filter from a file (if syntax is valid)
	FileFilter *LoadFilterFile(LPCTSTR szFilepath);

// Implementation data
public:
	stl::vector<FileFilter *> m_filters; /*< List of filters loaded */
};


bool TestAgainstRegList(const stl::vector<FileFilterElement*> &filterList, LPCTSTR szTest);

#endif // FileFilterMgr_h_included
