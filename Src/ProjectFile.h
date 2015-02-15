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
#pragma once

/**
 * @brief Class for handling project files.
 *
 * This class loads and saves project files.
 * We use UTF-8 encoding so Unicode paths are supported.
 */
class ProjectFile
{
public:
	ProjectFile();
	bool Read(LPCTSTR);
	bool Save(LPCTSTR);
	
	static BOOL IsProjectFile(LPCTSTR);

public:
	String m_sOptions; /**< Obtained from <?WinMergeScript options='...' ?> */
	String m_sLeftFile; /**< Left path */
	String m_sRightFile; /**< Right path */
	String m_sFilter; /**< Filter name or mask */
	String m_sCompareAs; /**< How to compare */
	String m_sConfig; /**< Location of private config file */
	int m_nRecursive; /**< Are subfolders included (recursive scan) */
	BOOL m_bLeftPathReadOnly; /**< Is left path opened as read-only */
	BOOL m_bRightPathReadOnly; /**< Is right path opened as read-only */
};
