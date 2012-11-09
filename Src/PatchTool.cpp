/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  files.cpp
 *
 * @brief Code file routines
 */
// ID line follows -- this is updated by SVN
// $Id$

#include "StdAfx.h"
#include "resource.h"
#include "LanguageSelect.h"
#include "common/coretypes.h"
#include "common/coretools.h"
#include "DiffWrapper.h"
#include "PatchTool.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/**
 * @brief Default constructor.
 */
CPatchTool::CPatchTool()
{
}

/**
 * @brief Default destructor.
 */
CPatchTool::~CPatchTool()
{
}

/** 
 * @brief Adds files to list for patching.
 * @param [in] file1 First file to add.
 * @param [in] file2 Second file to add.
 */
void CPatchTool::AddFiles(LPCTSTR file1, LPCTSTR file2)
{
	PATCHFILES files;
	files.lfile = paths_UndoMagic(&String(file1).front());
	files.rfile = paths_UndoMagic(&String(file2).front());
	m_fileList.push_back(files);
}

/**
 * @brief Add files with alternative paths.
 * This function adds files with alternative paths. Alternative path is the
 * one that is added to the patch file. So while @p file1 and @p file2 are
 * paths in disk (can be temp file names), @p altPath1 and @p altPath2 are
 * "visible " paths printed to the patch file.
 * @param [in] file1 First path in disk.
 * @param [in] altPath1 First path as printed to the patch file.
 * @param [in] file2 Second path in disk.
 * @param [in] altPath2 Second path as printed to the patch file.
 */
void CPatchTool::AddFiles(LPCTSTR file1, LPCTSTR altPath1, LPCTSTR file2, LPCTSTR altPath2)
{
	PATCHFILES files;
	files.lfile = paths_UndoMagic(&String(file1).front());
	files.rfile = paths_UndoMagic(&String(file2).front());
	files.pathLeft = paths_UndoMagic(&String(altPath1).front());
	files.pathRight = paths_UndoMagic(&String(altPath2).front());
	m_fileList.push_back(files);
}

/** 
 * @brief Create a patch from files given.
 * @note Files can be given using AddFiles() or selecting using
 * CPatchDlg.
 */
int CPatchTool::CreatePatch()
{
	DIFFSTATUS status;
	bool bResult = true;
	int retVal = 0;

	// If files already inserted, add them to dialog
    for(stl::vector<PATCHFILES>::iterator iter = m_fileList.begin(); iter != m_fileList.end(); ++iter)
    {
        m_dlgPatch.AddItem(*iter);
	}

	if (ShowDialog())
	{
		String path;
		SplitFilename(m_dlgPatch.m_fileResult.c_str(), &path, NULL, NULL);
		if (!paths_CreateIfNeeded(path.c_str()))
		{
			LanguageSelect.MsgBox(IDS_FOLDER_NOTEXIST, MB_OK | MB_ICONSTOP);
			return 0;
		}

		// Select patch create -mode
		m_diffWrapper.SetCreatePatchFile(m_dlgPatch.m_fileResult);
		m_diffWrapper.SetAppendFiles(m_dlgPatch.m_appendFile);

		int fileCount = m_dlgPatch.GetItemCount();
		for (int index = 0; index < fileCount; index++)
		{
			const PATCHFILES& files = m_dlgPatch.GetItemAt(index);
			
			// Set up DiffWrapper
			m_diffWrapper.SetPaths(files.lfile, files.rfile);
			m_diffWrapper.SetAlternativePaths(files.pathLeft, files.pathRight);
			m_diffWrapper.SetCompareFiles(files.lfile, files.rfile);
			if (!m_diffWrapper.RunFileDiff())
			{
				LanguageSelect.MsgBox(IDS_FILEERROR, MB_ICONSTOP);
				bResult = false;
				break;
			}
			if (m_diffWrapper.m_status.bBinaries)
			{
				LanguageSelect.MsgBox(IDS_CANNOT_CREATE_BINARYPATCH, MB_ICONSTOP);
				bResult = false;
				break;
			}
			if (m_diffWrapper.m_status.bPatchFileFailed)
			{
				LanguageSelect.FormatMessage(
					IDS_FILEWRITE_ERROR, m_dlgPatch.m_fileResult.c_str()
				).MsgBox(MB_ICONSTOP);
				bResult = false;
				break;
			}
			// Append next files...
			m_diffWrapper.SetAppendFiles(TRUE);
		}
		
		if (bResult && fileCount > 0)
		{
			LanguageSelect.MsgBox(IDS_DIFF_SUCCEEDED, MB_ICONINFORMATION|MB_DONT_DISPLAY_AGAIN);
			m_sPatchFile = m_dlgPatch.m_fileResult;
			m_bOpenToEditor = m_dlgPatch.m_openToEditor;
			retVal = 1;
		}
	}
	m_dlgPatch.ClearItems();
	return retVal;
}

/** 
 * @brief Show patch options dialog and check options selected.
 * @return TRUE if user wants to create a patch (didn't cancel dialog).
 */
BOOL CPatchTool::ShowDialog()
{
	BOOL bRetVal = TRUE;

	if (LanguageSelect.DoModal(m_dlgPatch) == IDOK)
	{
		// There must be one filepair
		if (m_dlgPatch.GetItemCount() < 1)
			bRetVal = FALSE;

		PATCHOPTIONS patchOptions;
		// These two are from dropdown list - can't be wrong
		patchOptions.outputStyle = m_dlgPatch.m_outputStyle;
		patchOptions.nContext = m_dlgPatch.m_contextLines;
		// Checkbox - can't be wrong
		patchOptions.bAddCommandline = m_dlgPatch.m_includeCmdLine != FALSE;
		m_diffWrapper.SetPatchOptions(patchOptions);

		// These are from checkboxes and radiobuttons - can't be wrong
		m_diffWrapper.nIgnoreWhitespace = m_dlgPatch.m_whitespaceCompare;
		m_diffWrapper.bIgnoreBlankLines = m_dlgPatch.m_ignoreBlanks != FALSE;
		m_diffWrapper.SetAppendFiles(m_dlgPatch.m_appendFile);
		// Use this because non-sensitive setting can't write
		// patch file EOLs correctly
		m_diffWrapper.bIgnoreEol = false;
		m_diffWrapper.bIgnoreCase = m_dlgPatch.m_caseSensitive == FALSE;
		m_diffWrapper.bFilterCommentsLines = false;
		m_diffWrapper.RefreshFilters();
	}
	else
		return FALSE;

	return bRetVal;
}

/** 
 * @brief Returns filename and path for patch-file
 */
String CPatchTool::GetPatchFile() const
{
	return m_sPatchFile;
}

/** 
 * @brief Returns TRUE if user wants to open patch file
 * to external editor (specified in WinMerge options).
 */
BOOL CPatchTool::GetOpenToEditor() const
{
	return m_bOpenToEditor;
}
