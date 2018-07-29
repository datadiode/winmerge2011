/////////////////////////////////////////////////////////////////////////////
//    License (GPLv3+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/////////////////////////////////////////////////////////////////////////////
/**
 * @file  FileFilterHelper.h
 *
 * @brief Declaration file for FileFilterHelper
 */
#pragma once

#include "FileFilterMgr.h"

/**
 * @brief File extension of file filter files.
 */
const TCHAR FileFilterExt[] = _T("*.flt");

/// Interface for testing files & directories for exclusion, as diff traverses file tree
extern class IDiffFilter
{
public:
	virtual BSTR getSql(int side) { return NULL; }
	virtual bool includeFile(LPCTSTR szPath, LPCTSTR szFileName) { return true; }
	virtual bool includeDir(LPCTSTR szPath, LPCTSTR szDirName) { return true; }
	// Compare (NLS aware) two filenames, potentially ignoring character case
	virtual int collateFile(LPCTSTR szFileName1, LPCTSTR szFileName2, bool casesensitive = false)
	{
		return (casesensitive ? _tcscoll : _tcsicoll)(szFileName1, szFileName2);
	}
	// Compare (NLS aware) two dirnames, potentially ignoring character case
	virtual int collateDir(LPCTSTR szDirName1, LPCTSTR szDirName2, bool casesensitive = false)
	{
		return (casesensitive ? _tcscoll : _tcsicoll)(szDirName1, szDirName2);
	}
	bool includeFile(LPCTSTR szPath1, LPCTSTR szFileName1, LPCTSTR szPath2, LPCTSTR szFileName2)
	{
		return
		(
			(szFileName1[0] == '\0' || includeFile(szPath1, szFileName1))
		&&	(szFileName2[0] == '\0' || includeFile(szPath2, szFileName2))
		);
	}
	bool includeDir(LPCTSTR szPath1, LPCTSTR szDirName1, LPCTSTR szPath2, LPCTSTR szDirName2)
	{
		return
		(
			(szDirName1[0] == '\0' || includeDir(szPath1, szDirName1))
		&&	(szDirName2[0] == '\0' || includeDir(szPath2, szDirName2))
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
	, private FileFilter
{
public:
	FileFilterHelper();
	~FileFilterHelper();

	const String &GetGlobalFilterPath() const { return m_sGlobalFilterPath; }
	const String &GetUserFilterPath() const { return m_sUserSelFilterPath; }

	void SetFileFilterPath(LPCTSTR szFileFilterPath);
	const std::vector<FileFilter *> &GetFileFilters() const { return m_filters; }
	FileFilter *FindFilter(LPCTSTR);
	bool SetUserFilterPath(const String &filterPath);

	FileFilter *ReloadAllFilters();
	void ReloadCurrentFilter();
	void LoadAllFileFilters();

	bool IsUsingMask() const { return m_currentFilter == this; }
	String GetFilterNameOrMask() const;
	FileFilter *SetFilter(const String &filter);

	// Overrides
	virtual BSTR getSql(int) override;
	virtual bool includeFile(LPCTSTR, LPCTSTR) override;
	virtual bool includeDir(LPCTSTR, LPCTSTR) override;
	virtual int collateFile(LPCTSTR, LPCTSTR, bool) override;
	virtual int collateDir(LPCTSTR, LPCTSTR, bool) override;

protected:
	virtual bool CreateFromMask() override;

private:
	FileFilter *m_currentFilter;     /*< Currently selected filefilter */
	const String m_sGlobalFilterPath;    /*< Path for shared filters */
	String m_sUserSelFilterPath;     /*< Path for user's private filters */
} globalFileFilter;
