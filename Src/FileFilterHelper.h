/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  FileFilterHelper.h
 *
 * @brief Declaration file for FileFilterHelper
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _FILEFILTERHELPER_H_
#define _FILEFILTERHELPER_H_

#include "FileFilterMgr.h"

class FilterList;
struct FileFilter;

/**
 * @brief File extension of file filter files.
 */
const TCHAR FileFilterExt[] = _T("*.flt");

/// Interface for testing files & directories for exclusion, as diff traverses file tree
extern class IDiffFilter
{
private:
	const static bool casesensitive = false;
public:
	virtual BSTR getSql() { return NULL; }
	virtual bool includeFile(LPCTSTR szFileName) { return true; }
	virtual bool includeDir(LPCTSTR szDirName) { return true; }
	virtual bool isCaseSensitive() { return casesensitive; }
	// Compare (NLS aware) two filenames, potentially ignoring character case
	virtual int collateFile(LPCTSTR szFileName1, LPCTSTR szFileName2)
	{
		return (casesensitive ? _tcscoll : _tcsicoll)(szFileName1, szFileName2);
	}
	// Compare (NLS aware) two dirnames, potentially ignoring character case
	virtual int collateDir(LPCTSTR szDirName1, LPCTSTR szDirName2)
	{
		return (casesensitive ? _tcscoll : _tcsicoll)(szDirName1, szDirName2);
	}
	bool includeFile(LPCTSTR szFileName1, LPCTSTR szFileName2)
	{
		return
		(
			(szFileName1[0] == '\0' || includeFile(szFileName1))
		&&	(szFileName2[0] == '\0' || includeFile(szFileName2))
		);
	}
	bool includeDir(LPCTSTR szDirName1, LPCTSTR szDirName2)
	{
		return
		(
			(szDirName1[0] == '\0' || includeDir(szDirName1))
		&&	(szDirName2[0] == '\0' || includeDir(szDirName2))
		);
	}
} transparentFileFilter;

/**
 * @brief Helper class for using filefilters.
 *
 * A FileFilterHelper object is the owner of any active mask, and of the file filter manager
 * This is kind of a File Filter SuperManager, taking care of both inline filters from strings
 *  and loaded file filters (the latter handled by its internal file filter manager)
 *
 * This class is mainly for handling two ways to filter files in WinMerge:
 * - File masks: *.ext lists (*.cpp *.h etc)
 * - File filters: regular expression rules in separate files
 *
 * There can be only one filter or file mask active at a time. This class
 * keeps track of selected filtering method and provides simple functions for
 * clients for querying if file is included to compare. Clients don't need
 * to care about compare methods etc details.
 */
extern class FileFilterHelper
	: public IDiffFilter
	, public FileFilterMgr
{
public:
	FileFilterHelper();
	~FileFilterHelper();

	const String &GetGlobalFilterPath() const { return m_sGlobalFilterPath; }
	const String &GetUserFilterPath() const { return m_sUserSelFilterPath; }

	void SetFileFilterPath(LPCTSTR szFileFilterPath);
	const stl::vector<FileFilter *> &GetFileFilters(String &selected) const;
	String GetFileFilterName(LPCTSTR filterPath) const;
	String GetFileFilterPath(LPCTSTR filterName) const;
	bool SetUserFilterPath(const String &filterPath);

	void ReloadUpdatedFilters();
	void LoadAllFileFilters();

	void SetMask(LPCTSTR strMask);

	bool IsUsingMask() const { return m_currentFilter == NULL; }
	String GetFilterNameOrMask() const;
	void SetFilter(const String &filter);

	// Overrides
	virtual BSTR getSql();
	virtual bool includeFile(LPCTSTR);
	virtual bool includeDir(LPCTSTR);
	virtual int collateFile(LPCTSTR, LPCTSTR);
	virtual int collateDir(LPCTSTR, LPCTSTR);

protected:
	String ParseExtensions(const String &extensions) const;

private:
	FilterList *const m_pMaskFilter;       /*< Filter for filemasks (*.cpp) */
	FileFilter *m_currentFilter;     /*< Currently selected filefilter */
	String m_sMask;   /*< File mask (if defined) "*.cpp *.h" etc */
	const String m_sGlobalFilterPath;    /*< Path for shared filters */
	String m_sUserSelFilterPath;     /*< Path for user's private filters */
} globalFileFilter;

#endif // _FILEFILTERHELPER_H_
