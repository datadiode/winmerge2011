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
 * @file  PatchTool.h
 *
 * @brief Declaration file for PatchTool class
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _PATCHTOOL_H_
#define _PATCHTOOL_H_

#include "PatchDlg.h"

/** 
 * @brief Files used for patch creating.
 * Stores paths of two files used to create a patch. Left side file
 * is considered as "original" file and right side file as "changed" file.
 * Times are for printing filetimes to patch file.
 */
struct PATCHFILES
{
	String lfile; /**< Left file */
	String pathLeft; /**< Left path added to patch file */
	String rfile; /**< Right file */
	String pathRight; /**< Right path added to patch file */
	FileTime ltime; /**< Left time */
	FileTime rtime; /**< Right time */
	// Swap sides
	void swap_sides()
	{
		stl::swap(lfile, rfile);
		stl::swap(pathLeft, pathRight);
		stl::swap(ltime, rtime);
	}
};

/** 
 * @brief A class which creates patch files.
 * This class is used to create patch files. The files to patch can be added
 * to list before calling CreatePatch(). Or user can select files in the
 * the dialog that CreatePatch() shows.
 */
class CPatchTool
{
public:
	CPatchTool();
	~CPatchTool();

	void AddFiles(LPCTSTR file1, LPCTSTR file2);
	void AddFiles(LPCTSTR file1, LPCTSTR altPath1,
		LPCTSTR file2, LPCTSTR altPath2);
	int CreatePatch();
	String GetPatchFile() const;
	BOOL GetOpenToEditor() const;

protected:
	BOOL ShowDialog();

private:
    stl::vector<PATCHFILES> m_fileList; /**< List of files to patch. */
	CDiffWrapper m_diffWrapper; /**< DiffWrapper instance we use to create patch. */
	CPatchDlg m_dlgPatch; /**< Dialog for selecting files and options. */
	String m_sPatchFile; /**< Patch file path and filename. */
	BOOL m_bOpenToEditor; /**< Is patch file opened to external editor? */
};

#endif	// _PATCHTOOL_H_
