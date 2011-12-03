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
 * @file  ProjectFile.h
 *
 * @brief Declaration file ProjectFile class
 */
// ID line follows -- this is updated by CVS
// $Id$

#ifndef _PROJECT_FILE_H_
#define _PROJECT_FILE_H_

/** @brief File extension for path files */
const TCHAR PROJECTFILE_EXT[] = _T(".WinMerge");

/**
 * @brief Class for handling project files.
 *
 * This class loads and saves project files. Expat parser and SCEW wrapper for
 * expat are used for XML parsing. We use UTF-8 encoding so Unicode paths are
 * supported.
 */
class ProjectFile
{
public:
	ProjectFile();
	bool Read(LPCTSTR path, String &sError);
	bool Save(LPCTSTR path, String &sError);
	
	bool HasLeft() const;
	bool HasRight() const;
	bool HasFilter() const;
	bool HasSubfolders() const;

	String GetLeft() const;
	bool GetLeftReadOnly() const;
	String GetRight() const;
	bool GetRightReadOnly() const;
	String GetFilter() const;
	int GetSubfolders() const;

	void SetLeft(const String& sLeft, bool readOnly);
	void SetRight(const String& sRight, bool readOnly);
	void SetFilter(const String& sFilter);
	void SetSubfolders(int iSubfolder);

	static bool IsProjectFile(LPCTSTR filepath);

private:
	bool m_bHasLeft; /**< Has left path? */
	String m_leftFile; /**< Left path */
	bool m_bHasRight; /**< Has right path? */
	String m_rightFile; /**< Right path */
	bool m_bHasFilter; /**< Has filter? */
	String m_filter; /**< Filter name or mask */
	bool m_bHasSubfolders; /**< Has subfolders? */
	int m_subfolders; /**< Are subfolders included (recursive scan) */
	bool m_bLeftReadOnly; /**< Is left path opened as read-only */
	bool m_bRightReadOnly; /**< Is right path opened as read-only */
};

#endif // #ifdef _PROJECT_FILE_H_
