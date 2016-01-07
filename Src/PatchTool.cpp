/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
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
#include "StdAfx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "LanguageSelect.h"
#include "common/coretypes.h"
#include "common/coretools.h"
#include "PatchTool.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PATCHFILES::PATCHFILES(const PATCHFILES &r)
	: lfile(r.lfile)
	, rfile(r.rfile)
	, pathLeft(r.pathLeft)
	, pathRight(r.pathRight)
{
	paths_UndoMagic(lfile);
	paths_UndoMagic(rfile);
	paths_UndoMagic(pathLeft);
	paths_UndoMagic(pathRight);
}

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
 * @brief Add files with alternative paths.
 */
void CPatchTool::AddFiles(const PATCHFILES &files)
{
	m_dlgPatch.AddItem(files);
}

/** 
 * @brief Create a patch from files given.
 * @note Files can be given using AddFiles() or selecting using
 * CPatchDlg.
 */
void CPatchTool::Run()
{
	if (LanguageSelect.DoModal(m_dlgPatch) == IDOK)
	{
		// These two are from dropdown list - can't be wrong
		m_diffWrapper.outputStyle = m_dlgPatch.m_outputStyle;
		m_diffWrapper.nContext = m_dlgPatch.m_contextLines;
		// Checkbox - can't be wrong
		m_diffWrapper.bAddCommandline = m_dlgPatch.m_includeCmdLine != FALSE;
		m_diffWrapper.bAppendFiles = m_dlgPatch.m_appendFile != FALSE;

		// These are from checkboxes and radiobuttons - can't be wrong
		m_diffWrapper.nIgnoreWhitespace = m_dlgPatch.m_whitespaceCompare;
		m_diffWrapper.bIgnoreBlankLines = m_dlgPatch.m_ignoreBlankLines != FALSE;
		// Use this because non-sensitive setting can't write
		// patch file EOLs correctly
		m_diffWrapper.bIgnoreEol = false;
		m_diffWrapper.bIgnoreCase = m_dlgPatch.m_ignoreCase != FALSE;
		m_diffWrapper.bFilterCommentsLines = false;
		m_diffWrapper.bApplyLineFilters = m_dlgPatch.m_applyLineFilters != FALSE;;
		m_diffWrapper.RefreshFilters();

		if (!paths_CreateIfNeeded(m_dlgPatch.m_fileResult.c_str(), true))
		{
			LanguageSelect.MsgBox(IDS_FOLDER_NOTEXIST, MB_ICONSTOP);
			return;
		}

		// Select patch create -mode
		m_diffWrapper.SetCreatePatchFile(m_dlgPatch.m_fileResult);

		const int fileCount = m_dlgPatch.GetItemCount();
		int index = 0;
		while (index < fileCount)
		{
			const PATCHFILES &files = m_dlgPatch.GetItemAt(index);
			
			// Set up DiffWrapper
			m_diffWrapper.SetPaths(files.lfile, files.rfile);
			m_diffWrapper.SetAlternativePaths(files.pathLeft, files.pathRight);
			m_diffWrapper.SetCompareFiles(files.lfile, files.rfile);
			if (!m_diffWrapper.RunFileDiff())
			{
				LanguageSelect.MsgBox(IDS_FILEERROR, MB_ICONSTOP);
				break;
			}
			if (m_diffWrapper.m_status.bBinaries)
			{
				LanguageSelect.MsgBox(IDS_CANNOT_CREATE_BINARYPATCH, MB_ICONSTOP);
				break;
			}
			if (m_diffWrapper.m_status.bPatchFileFailed)
			{
				LanguageSelect.FormatStrings(
					IDS_FILEWRITE_ERROR, m_dlgPatch.m_fileResult.c_str()
				).MsgBox(MB_ICONSTOP);
				break;
			}
			// Append next files...
			m_diffWrapper.bAppendFiles = true;
			++index;
		}

		if (index == fileCount)
		{
			LanguageSelect.MsgBox(IDS_DIFF_SUCCEEDED, MB_ICONINFORMATION|MB_DONT_DISPLAY_AGAIN);
			if (m_dlgPatch.m_openToEditor)
			{
				CMainFrame::OpenFileToExternalEditor(m_dlgPatch.m_fileResult.c_str());
			}
		}
	}
}
