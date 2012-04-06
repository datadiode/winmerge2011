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

class FileFilterMgr;
class FilterList;
struct FileFilter;

/**
 * @brief File extension of file filter files.
 */
const TCHAR FileFilterExt[] = _T(".flt");

/// Interface for testing files & directories for exclusion, as diff traverses file tree
class IDiffFilter
{
public:
	virtual bool includeFile(LPCTSTR szFileName) = 0;
	virtual bool includeDir(LPCTSTR szDirName) = 0;
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
};

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
extern class FileFilterHelper : public IDiffFilter
{
public:
	FileFilterHelper();
	~FileFilterHelper();

	String GetGlobalFilterPathWithCreate() const;
	String GetUserFilterPathWithCreate() const;

	FileFilterMgr * GetManager() const;
	void SetFileFilterPath(LPCTSTR szFileFilterPath);
	//void GetFileFilters(stl::vector<FileFilterInfo> * filters, String & selected) const;
	const stl::vector<FileFilter *> &GetFileFilters(String & selected) const;
	String GetFileFilterName(LPCTSTR filterPath) const;
	String GetFileFilterPath(LPCTSTR filterName) const;
	void SetUserFilterPath(LPCTSTR filterPath);

	void ReloadUpdatedFilters();
	void LoadAllFileFilters();

	void LoadFileFilterDirPattern(LPCTSTR dir, LPCTSTR szPattern);

	void SetMask(LPCTSTR strMask);

	bool IsUsingMask() const { return m_currentFilter == NULL; }
	String GetFilterNameOrMask() const;
	void SetFilter(const String &filter);

	bool includeFile(LPCTSTR szFileName);
	bool includeDir(LPCTSTR szDirName);

protected:
	String ParseExtensions(const String &extensions) const;

private:
	FilterList *const m_pMaskFilter;       /*< Filter for filemasks (*.cpp) */
	FileFilter *m_currentFilter;     /*< Currently selected filefilter */
	FileFilterMgr *const m_fileFilterMgr;  /*< Associated FileFilterMgr */
	String m_sMask;   /*< File mask (if defined) "*.cpp *.h" etc */
	String m_sGlobalFilterPath;    /*< Path for shared filters */
	String m_sUserSelFilterPath;     /*< Path for user's private filters */
} globalFileFilter;

#endif // _FILEFILTERHELPER_H_
